// RecoverPool.cpp: implementation of the RecoverPool class.
//
//////////////////////////////////////////////////////////////////////

#include <cassert>
#include <cstring>
#include "RecoverPool.h"
#include "../Micro-Development-Kit/include/mdk/FixLengthInt.h"
#include "../Micro-Development-Kit/include/mdk/atom.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

RecoverPool::RecoverPool()
{
	memset(m_poolMap, 0, sizeof(unsigned long*)*(MAX_SIZE_POWER+1));
}

RecoverPool::~RecoverPool()
{
	Release();
}

int RecoverPool::GetPoolIndex(unsigned int &size)
{
    unsigned int i = 1;
	int poolIdx = 0;
	while(1) 
	{
        if (i >= size) 
		{
			size = i;
			return poolIdx;
		}
        i *= 2;
		poolIdx++;
		if ( poolIdx > MAX_SIZE_POWER ) return -1;
    }
}

unsigned long *RecoverPool::GetArray( unsigned long *pArray )
{
	if ( NULL != *pArray ) return (unsigned long*)*pArray;
#ifndef WIN32
	mdk::AutoLock lock(&m_createArrayMutex);
#endif
	if ( NULL == (unsigned long*)*pArray )
	{
		unsigned long *pArrayPool = new unsigned long[POOL_SIZE];
		if ( NULL == pArrayPool )
		{
			return NULL;
		}
		memset( pArrayPool, 0, sizeof(unsigned long) * POOL_SIZE );
		pArrayPool[UseCount] = 2;
		*pArray = (unsigned long)pArrayPool;
	}
	return (unsigned long*)*pArray;
}

void* RecoverPool::Alloc( unsigned int objectSize, int arraySize )
{
#ifdef WIN32
	mdk::AutoLock lock(&m_Mutex);
#endif
	unsigned long *pArrayPool = NULL;
	
	unsigned int allocSize = objectSize*arraySize;
	//找到尺寸所属的数组
	int poolIdx = GetPoolIndex( objectSize );
	if ( -1 == poolIdx ) return NULL;
	pArrayPool = GetArray((unsigned long*)&m_poolMap[poolIdx]);
	if ( NULL == pArrayPool ) return NULL;
	
	//从数组中分配/从系统分配，放入数组
	void* pObject = NULL;
	for ( ; true; )
	{
		pObject = AllocFromArray( allocSize, pArrayPool );
		if ( -1 == (long)pObject ) return NULL;
		if ( NULL != pObject ) return pObject;
		pArrayPool = GetArray(&pArrayPool[NextPool]);
		if ( NULL == pArrayPool ) return NULL;
	}
	
	return pObject;
}

void* RecoverPool::AllocFromArray( unsigned int size, unsigned long *pArrayPool )
{
	if ( POOL_SIZE <= pArrayPool[UseCount] ) return NULL;
	if ( POOL_SIZE <= mdk::AtomAdd( &pArrayPool[UseCount], 1) ) //使用计数+
	{
		mdk::AtomDec( &pArrayPool[UseCount], 1 );
		return NULL;
	}
	unsigned char *pBlock = NULL;
	unsigned short i = FirstBlock;
	for ( ; i < POOL_SIZE; i++ )
	{
		if ( NULL == pArrayPool[i] )
		{
			//创建新Block
#ifndef WIN32
			mdk::AutoLock lock(&m_createBlockMutex);
#endif
			if ( NULL == pArrayPool[i] )
			{
				pBlock = new unsigned char[state+prekeep+index+blockAddr+size];
				if ( NULL == pBlock ) 
				{
					return (void*)-1;
				}
				pBlock[0] = 1;
				pBlock[1] = 0;
				pBlock[2] = 0;
				pBlock[3] = 0;
				memcpy( &pBlock[state+prekeep], &i, index );
				memcpy( &pBlock[state+prekeep+index], &pArrayPool, blockAddr );
				pArrayPool[i] = (unsigned long)pBlock;
				return (void*)&pBlock[state+prekeep+index+blockAddr];
			}
		}

		pBlock = (unsigned char*)pArrayPool[i];
		if ( 0 != pBlock [0] || 0 != pBlock [1] || 0 != pBlock [2] || 0 != pBlock [3] ) continue;
		if ( 0 < mdk::AtomAdd(&pBlock[0], 1) ) continue;
		return (void*)&pBlock[state+prekeep+index+blockAddr];
	}
	return NULL;
}

void RecoverPool::Free( void *pObject )
{
	unsigned char *pBlock = (unsigned char*)pObject;
	pBlock = pBlock - (state+prekeep+index+blockAddr);
	mdk::uint64 pArrayAdd;
	memcpy( &pArrayAdd, &pBlock[state+prekeep+index], blockAddr );
	unsigned long *pArrayPool = (unsigned long *)pArrayAdd;
	unsigned short idx = 0;
	memcpy( &idx, &pBlock[state+prekeep], index );
	
	if (!( FirstBlock <= idx && idx < POOL_SIZE ))assert( false );//非法游标
	if (!( pBlock == (unsigned char *)pArrayPool[idx] ))assert( false );//非法地址
	if (!(0 != pBlock [0] || 0 != pBlock [1] || 0 != pBlock [2] || 0 != pBlock [3]))assert( false );//重复释放
	
#ifdef WIN32
	mdk::AutoLock lock(&m_Mutex);
#endif
	mdk::AtomSet( &pBlock[0], 0 );
	mdk::AtomDec( &pArrayPool[UseCount], 1 );//使用计数-
}

void RecoverPool::Release()
{
	unsigned long *pArrayPool = NULL;
	int i = 0;
	for ( ; i < 33; i++ )
	{
		pArrayPool = m_poolMap[i];
		ReleaseArray(pArrayPool);
	}
}

void RecoverPool::ReleaseArray(unsigned long *pArrayPool)
{
	if ( NULL == pArrayPool ) return;
	if ( NULL != pArrayPool[NextPool] ) ReleaseArray((unsigned long *)pArrayPool[NextPool]);
	unsigned short i = FirstBlock;
	unsigned char* pBlock;
	for ( ; i < POOL_SIZE; i++ )
	{
		pBlock = (unsigned char*)pArrayPool[i];
		if ( NULL == pBlock ) continue;
		delete[]pBlock;
		pArrayPool[i] = NULL;
	}
	delete[]pArrayPool;
	return;
}
