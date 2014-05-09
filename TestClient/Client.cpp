// SerMain.cpp : Defines the entry point for the console application.
//
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "mdk/Socket.h"
#include "../Exist/RHTable.h"
#include "mdk/atom.h"
using namespace mdk;

#include <map>
using namespace std;
unsigned int *g_hash = NULL;
unsigned int *g_key = NULL;
unsigned short *g_keySize = NULL;

#define DATA_COUNT 200000
typedef struct THREAD_CONTROL
{
	DWORD dwID;
	bool bCheck;
}THREAD_CONTROL;

map<DWORD,THREAD_CONTROL*> g_activeThread;
DWORD TMainThread( LPVOID pParam );//主线程入口
UINT uPort;
char strIP[256];
int g_client = 0;
int g_clientM = 0;
time_t g_lastdata = 0;
DWORD Client( LPVOID pParam  )
{
	int start = (int)pParam;
	DWORD dwThreadID;
	Socket sock;
	sock.Init( Socket::tcp );
	printf( "connect(%d)\n", start );
	if ( !sock.Connect( strIP, uPort ) )
	{
		printf( "connect error\n" );
		return 0;
	}

	AtomAdd(&g_client, 1);
	while ( g_client < g_clientM ) Sleep(100);
//	HANDLE hThread = CreateThread( 
//		NULL, 0, 
//		(LPTHREAD_START_ROUTINE)TMainThread,
//		(LPVOID)&sock, 0, &dwThreadID );
//	if ( NULL == hThread ) 
//	{
//		sock.Close();
//		printf( "create thread error\n" );
//		return 0;
//	}
	
	srand( time( NULL ) );
	int i = 0;
	int nSize;
	char buf[2560];
	int nSendSize = 0;
	DWORD dwID = GetCurrentThreadId();
	unsigned short len;
	while ( 1 )
	{
		int s = start * 100;
		int e = 200000;
		char *buf = new char[52200];
		char *b;
		int j = 0;
		for ( i = s; i < e; i++ )
		{
			b = &buf[522*j];
			nSize = 0;
			len = 2 + 256 + 2 + 256 + 4;
			memcpy( &b[nSize], &len, sizeof(unsigned short) );
			nSize += sizeof(unsigned short);
			
			memcpy( &b[nSize], &g_keySize[i], sizeof(unsigned short) );
			nSize += sizeof(unsigned short);
			memcpy( &b[nSize], (unsigned char*)(g_key[i]), g_keySize[i] );
			nSize += g_keySize[i];
			
			memcpy( &b[nSize], &g_keySize[i], sizeof(unsigned short) );
			nSize += sizeof(unsigned short);
			memcpy( &b[nSize], (unsigned char*)(g_key[i]), g_keySize[i] );
			nSize += g_keySize[i];
			
			memcpy( &b[nSize], &g_hash[i], sizeof(unsigned int) );
			nSize += sizeof(unsigned int);
			j++;
			if ( j < 100 ) continue;

			nSendSize = sock.Send( buf, nSize * j );
			if ( -1 == nSendSize || nSendSize != nSize * j ) 
			{
				printf( "线程(%d)退出:发送失败\n", dwID );
				return 0;
			}
			j = 0;
		}
		printf( "(%d)\n", start );
//		return 0;
//		Sleep( 100 );
		continue;
	}
	
	sock.Close();

	return 0;
}

DWORD TMainThread( LPVOID pParam )//主线程入口
{
	static int nID = 0;
	DWORD dwID = GetCurrentThreadId();
	THREAD_CONTROL *pThread = new THREAD_CONTROL;
	pThread->dwID = dwID;
	pThread->bCheck = false;
	g_activeThread.insert( map<DWORD,THREAD_CONTROL*>::value_type( dwID, pThread ) );

	char buf[256];
	int nLen;
	char c = '0' - 1;
	Socket *pSock = (Socket*)pParam;
	nLen = 25;
	nLen = pSock->Receive( buf, nLen );
	printf( "线程(%d)\n", dwID );
	if ( nLen <= 0 ) 
	{
		map<DWORD,THREAD_CONTROL*>::iterator it = g_activeThread.find( dwID );
		if ( it != g_activeThread.end() ) g_activeThread.erase( it );
		printf( "线程(%d)退出:服务器断开连接\n", dwID );
		return 0;
	}
	if ( c + 1 != buf[0] ) 
	{
		map<DWORD,THREAD_CONTROL*>::iterator it = g_activeThread.find( dwID );
		if ( it != g_activeThread.end() ) g_activeThread.erase( it );
		printf( "线程(%d)退出:漏数据\n", dwID );
		return 0;
	}
//	buf[nLen] = 0;
//	printf( buf );
	c = buf[nLen-1];
	if ( c == '9' ) c = '0' - 1;
	while ( 1 )
	{
		nLen = 25;
		nLen = pSock->Receive( buf, nLen );
		g_lastdata = time(NULL);
		if ( nLen <= 0 ) 
		{
			map<DWORD,THREAD_CONTROL*>::iterator it = g_activeThread.find( dwID );
			if ( it != g_activeThread.end() ) g_activeThread.erase( it );
			printf( "线程(%d)退出:服务器断开连接\n", dwID );
			return 0;
		}
		if ( c + 1 != buf[0] ) 
		{
			map<DWORD,THREAD_CONTROL*>::iterator it = g_activeThread.find( dwID );
			if ( it != g_activeThread.end() ) g_activeThread.erase( it );
			printf( "线程(%d)退出:漏数据\n", dwID );
			return 0;
		}
		if ( pThread->bCheck )
		{
			buf[nLen] = 0;
			printf( buf );
		}
		c = buf[nLen-1];
		if ( c == '9' ) c = '0' - 1;
	}
}

int main(int argc, char* argv[])
{
	g_hash = new unsigned int[DATA_COUNT];
	g_key = new unsigned int[DATA_COUNT];
	g_keySize = new unsigned short[DATA_COUNT];
	int j;
	char* key;
	RHTable hc;
	for ( j = 0; j < DATA_COUNT; j++ )
	{
		key = new char[256];
		sprintf(key,"%d",j);
		g_keySize[j] = strlen(key);
		g_key[j] = (unsigned int)key;
		memset( &key[g_keySize[j]], j, 256 - g_keySize[j] );
		g_keySize[j] = 256;
		g_hash[j] = hc.RemoteHash((unsigned char*)key, g_keySize[j]);
	}

	uPort = GetPrivateProfileInt( "Server", "Port", 8080, ".\\cli.ini" );
	DWORD dwSize = 255;
	GetPrivateProfileString( "Server", "IP", "127.0.0.1", strIP, dwSize, ".\\cli.ini" );
	setvbuf( stdout, (char*)NULL, _IONBF, 0 );     //added
	Socket::SocketInit();
	//	Client( NULL );
	DWORD dwThreadID;
	int i = 1;
	int nThreadCount = 0;
	char sCount[5];
	sCount[0]= getchar();
	sCount[1]= getchar();
	sCount[2]= getchar();
	getchar();
	sCount[3]= 0;
	nThreadCount = atoi( sCount );
	//	nThreadCount = 999;
	printf( "创建%d个client 按任意键开始\n", nThreadCount );
	g_clientM = nThreadCount;
	for ( i = 1; i <= nThreadCount; i++ ){
		HANDLE hThread = CreateThread( 
			NULL, 0, 
			(LPTHREAD_START_ROUTINE)Client,
			(LPVOID)(i - 1), 0, &dwThreadID );
		if ( NULL == hThread )
		{
			printf( "create client(%d) error\n", i );
		}
		if ( 0 == i % 200 ) 
		{
			printf( "create%d\n", i );
			//			getchar();
		}
	}
	char c[100];
	int nPos;
	THREAD_CONTROL* pThead;
	map<DWORD, THREAD_CONTROL*>::iterator it;
	int nCount;
	while ( 1 ){
		c[0] = getchar();
		c[1] = getchar();
		c[2] = getchar();
		c[3] = getchar();
		c[3] = 0;
		nPos = atoi( c );
		nCount = g_activeThread.size();
		if ( nPos > nCount )
		{
			printf( "创建线程%d/%d\n", nPos, nCount );
			HANDLE hThread = CreateThread( 
				NULL, 0, 
				(LPTHREAD_START_ROUTINE)Client,
				(LPVOID)NULL, 0, &dwThreadID );
		}
		else printf( "检查线程%d/%d\n", nPos, nCount );
		tm *last = localtime(&g_lastdata);
		char stime[256];
		strftime( stime, 256, "%Y-%m-%d %H:%M:%S", last );
		printf( "last data:%s\n", stime );
		Sleep( 1000 );
		it = g_activeThread.begin();
		for ( i = 1; it != g_activeThread.end(); it++, i++ )
		{
			pThead = it->second;
			if ( i == nPos ) pThead->bCheck = true;
			else pThead->bCheck = false;
		}
		
		Sleep( 1000 );
	}
	
	return 0;
}

