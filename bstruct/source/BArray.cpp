// BArray.cpp: implementation of the BArray class.
//
//////////////////////////////////////////////////////////////////////

#include <cstring>
#include "../include/BArray.h"
#include "../include/BStruct.h"
using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace bsp
{

BArray::BArray()
{
	m_error.m_data = NULL;
	m_error.m_size = 0;
	m_error.m_parent = NULL;
	m_finished = true;
	m_action = BArray::unknow;
	m_bValid = false;
	m_bEmpty = true;
	m_elementSize = 0;
}

BArray::~BArray()
{
}

void BArray::SetElementSize(unsigned short size)
{
	m_elementSize = size;
}

void BArray::Bind(unsigned char *pBuffer, unsigned int uSize)
{
	m_data.clear();
	if ( NULL == pBuffer || 0 >= uSize ) return;
	m_stream.Bind(pBuffer, uSize);
	if ( !m_stream.AddData(m_elementSize) ) return;
	m_action = BArray::write;
}

unsigned char* BArray::GetStream()
{
	return m_stream.GetStream();
}

unsigned int BArray::GetSize()
{
	if ( BArray::write == m_action ) return m_stream.Pos();
	else if ( BArray::read == m_action ) return m_stream.GetSize();
	else return 0;
}

unsigned short BArray::GetElementSize()
{
	return m_elementSize;
}

unsigned int BArray::GetCount()
{
	return m_data.size();
}

unsigned char* BArray::PreBuffer()
{
	if ( 0 == m_elementSize ) return &(GetStream()[m_stream.Pos()+sizeof(unsigned short)]);
	else return &(GetStream()[m_stream.Pos()]);
}

unsigned int BArray::PreSize()
{
	if ( 0 == m_elementSize ) return m_stream.GetSize() - sizeof(unsigned short) - m_stream.Pos();
	else return m_stream.GetSize() - m_stream.Pos();
}

E_VALUE BArray::operator []( int index )
{
	if ( BArray::unknow == m_action ) return m_error;
	E_VALUE data;
	data.m_parent = this;
	if ( index >= m_data.size() )
	{
		if ( index > m_data.size() ) return m_error;
		if ( BArray::write != m_action ) return m_error;
		if ( !m_finished ) return m_error;
		data.m_data = (char*)(&m_stream.GetStream()[m_stream.Pos()]);//指向字段首地址
		m_data.push_back(data.m_data);
		data.m_size = 0;
		if ( 0 == data.m_size ) data.m_data += sizeof(unsigned short);//指向数据首地址
		m_finished = false;
		return data;
	}
	data.m_data = m_data[index];//指向字段首地址
	if ( 0 == m_elementSize )
	{
		if ( BArray::read == m_action || (BArray::write == m_action && m_finished) )
		{
			if ( NULL != data.m_data ) data.m_size = memtoi((unsigned char*)data.m_data, sizeof(unsigned short));
			else data.m_size = 0;
		}
		if ( 0 != data.m_size ) data.m_data += sizeof(unsigned short);//指向数据首地址
	}
	else data.m_size = m_elementSize;
	return data;
}

bool BArray::IsValid()
{
	return m_bValid;
}

bool BArray::IsEmpty()
{
	return m_bEmpty;
}

bool BArray::Resolve(unsigned char *pBuffer, unsigned int uSize)
{
	m_data.clear();
	m_bValid = false;
	m_bEmpty = true;
	if ( NULL == pBuffer || 0 >= uSize ) return false;
	m_stream.Bind(pBuffer, uSize);
	m_action = BArray::read;
	return Resolve();
}

bool BArray::Resolve()
{
	if ( BArray::read != m_action ) return false;
	char *value = NULL;
	if ( !m_stream.GetData(&m_elementSize) ) return false;//取得数组元素长度
	if ( 0 == m_elementSize ) //变长元素，元素编码格式为：unsigned short + byte[]
	{
		unsigned short size = 0;
		while ( !m_stream.IsEnd() )
		{
			value = (char*)m_stream.GetPointer(&size);//取得数据首地址
			if ( NULL == value && 0 != size ) return false;
			if ( NULL != value ) value = value - sizeof(unsigned short);//指向字段首地址
			m_data.push_back(value);
			m_bEmpty = false;
		}
	}
	else //定长元素，元素编码格式为：byte[]
	{
		char *buf = (char*)m_stream.GetStream() + sizeof(unsigned short);
		unsigned int size = m_stream.GetSize() - sizeof(unsigned short);
		int pos = 0;
		while ( pos != size )
		{
			if ( pos + m_elementSize > size ) return false;
			value = &buf[pos];//取得字段（对于定长模式即数据）首地址
			m_data.push_back(value);
			pos += m_elementSize;
			m_bEmpty = false;
		}
	}
	m_bValid = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////
//取值操作
bool E_VALUE::IsValid()
{
	if ( NULL == m_parent ) return false;
	return true;
}

E_VALUE::operator char()
{
	if ( NULL == m_data || m_size != sizeof(char) ) return (char)0xff;
	return (char)m_data[0];
}

E_VALUE::operator short()
{
	if ( NULL == m_data || m_size != sizeof(short) ) return (short)0xffff;
	return (short)memtoi( (unsigned char*)m_data, m_size );
}

E_VALUE::operator float()
{
	if ( NULL == m_data || m_size != sizeof(float) ) return (float)0xffffffff;
	float value;
	memcpy( &value, m_data, m_size );
	return value;
}

E_VALUE::operator double()
{
	if ( NULL == m_data || m_size != sizeof(double) ) return 0xffffffff;
	double value;
	memcpy( &value, m_data, m_size );
	return value;
}

E_VALUE::operator long()
{
	if ( NULL == m_data || m_size != sizeof(long) ) return 0xffffffff;
	return(long)memtoi( (unsigned char*)m_data, m_size );
}

E_VALUE::operator int32()
{
	if ( NULL == m_data || m_size != sizeof(int32) ) return 0xffffffff;
	return (int32)memtoi( (unsigned char*)m_data, m_size );
}

E_VALUE::operator int64()
{
	if ( NULL == m_data || m_size != sizeof(int64) ) return 0xffffffff;
	return memtoi( (unsigned char*)m_data, m_size );
}

E_VALUE::operator string()
{
	if ( NULL == m_data ) return "";
	string value;
	value.assign(m_data, m_size);
	return value;
}

E_VALUE::operator BArray()
{
	BArray value;
	if ( !IsValid() ) return value;
	value.Resolve((unsigned char*)m_data, m_size);
	return value;
}

E_VALUE::operator BStruct()
{
	BStruct value;
	if ( !IsValid() ) return value;
	value.Resolve((unsigned char*)m_data, m_size);
	return value;
}

//////////////////////////////////////////////////////////////////////////
//赋值操作
bool E_VALUE::operator = ( char* value )
{
	if ( NULL == m_data ) return false;
	if ( 0 != m_parent->m_elementSize ) return false;//定长元素,不能保存变长对象
	unsigned short vLen = strlen(value);
	if ( 0 >= m_size ) 
	{
		if ( !m_parent->m_stream.AddData( value, vLen ) ) return false;
		m_size = vLen;
		m_parent->m_finished = true;
		return true;
	}
	if ( m_size != vLen ) return false;
	memcpy( m_data, value, m_size );
	return true;
}

bool E_VALUE::operator = ( char value )
{
	if ( NULL == m_data ) return false;
	if ( sizeof(char) != m_parent->m_elementSize ) return false;//元素长度与期望保存的元素长度不符合
	if ( 0 == m_size )
	{
		m_parent->m_stream.AddData(value);
		m_size = sizeof(char);
		m_parent->m_finished = true;
		return true;
	}
	m_data[0] = value;
	return true;
}

bool E_VALUE::operator = ( short value )
{
	if ( NULL == m_data ) return false;
	if ( sizeof(short) != m_parent->m_elementSize ) return false;//元素长度与期望保存的元素长度不符合
	if ( 0 == m_size )
	{
		m_parent->m_stream.AddData(value);
		m_size = sizeof(short);
		m_parent->m_finished = true;
		return true;
	}
	itomem((unsigned char*)m_data,value,m_size);
	return true;
}

bool E_VALUE::operator = ( float value )
{	
	if ( NULL == m_data ) return false;
	if ( sizeof(float) != m_parent->m_elementSize ) return false;//元素长度与期望保存的元素长度不符合
	if ( 0 == m_size )
	{
		m_parent->m_stream.AddData(value);
		m_size = sizeof(float);
		m_parent->m_finished = true;
		return true;
	}
	memcpy( m_data, &value, m_size );
	return true;
}

bool E_VALUE::operator = ( double value )
{
	if ( NULL == m_data ) return false;
	if ( sizeof(double) != m_parent->m_elementSize ) return false;//元素长度与期望保存的元素长度不符合
	if ( 0 == m_size )
	{
		m_parent->m_stream.AddData(value);
		m_size = sizeof(double);
		m_parent->m_finished = true;
		return true;
	}
	memcpy( m_data, &value, m_size );
	return true;
}

bool E_VALUE::operator = ( long value )
{
	if ( NULL == m_data ) return false;
	if ( sizeof(long) != m_parent->m_elementSize ) return false;//元素长度与期望保存的元素长度不符合
	if ( 0 == m_size )
	{
		m_parent->m_stream.AddData(value);
		m_size = sizeof(long);
		m_parent->m_finished = true;
		return true;
	}
	itomem((unsigned char*)m_data,value,m_size);
	return true;
}

bool E_VALUE::operator = ( int32 value )
{
	if ( NULL == m_data ) return false;
	if ( sizeof(int32) != m_parent->m_elementSize ) return false;//元素长度与期望保存的元素长度不符合
	if ( 0 == m_size )
	{
		m_parent->m_stream.AddData(value);
		m_size = sizeof(int32);
		m_parent->m_finished = true;
		return true;
	}
	itomem((unsigned char*)m_data,value,m_size);
	return true;
}

bool E_VALUE::operator = ( int64 value )
{
	if ( NULL == m_data ) return false;
	if ( sizeof(int64) != m_parent->m_elementSize ) return false;//元素长度与期望保存的元素长度不符合
	if ( 0 == m_size )
	{
		m_parent->m_stream.AddData(value);
		m_size = sizeof(int64);
		m_parent->m_finished = true;
		return true;
	}
	itomem((unsigned char*)m_data,value,m_size);
	return true;
}

bool E_VALUE::operator = ( BArray *value )
{
	if ( NULL == m_data ) return false;
	if ( 0 != m_parent->m_elementSize ) return false;//定长元素,不能保存变长对象
	unsigned char *pStream = value->GetStream();
	if ( NULL == pStream ) return false;
	if ( 0 >= m_size ) 
	{
		if ( !m_parent->m_stream.AddData( pStream, value->GetSize() ) ) return false;
		m_size = value->GetSize();
		m_parent->m_finished = true;
		return true;
	}
	if ( m_size != value->GetSize() ) return false;
	memcpy( m_data, pStream, m_size );
	return true;
}

bool E_VALUE::operator = ( BStruct *value )
{
	if ( NULL == m_data ) return false;
	if ( 0 != m_parent->m_elementSize ) return false;//定长元素,不能保存变长对象
	unsigned char *pStream = value->GetStream();
	if ( NULL == pStream ) return false;
	if ( 0 >= m_size ) 
	{
		if ( !m_parent->m_stream.AddData( pStream, value->GetSize() ) ) return false;
		m_size = value->GetSize();
		m_parent->m_finished = true;
		return true;
	}
	if ( m_size != value->GetSize() ) return false;
	memcpy( m_data, pStream, m_size );
	return true;
}

bool E_VALUE::SetValue( const void *value, unsigned short uSize )
{
	if ( NULL == m_data ) return false;
	if ( 0 != m_parent->m_elementSize ) return false;//定长元素,不能保存变长对象
	if ( NULL == value || 0 >= uSize ) return false;
	if ( 0 >= m_size ) 
	{
		if ( !m_parent->m_stream.AddData( (unsigned char*)value, uSize ) ) return false;
		m_size = uSize;
		m_parent->m_finished = true;
		return true;
	}
	if( m_size != uSize ) return false;
	memcpy( m_data, (unsigned char*)value, m_size );
	return true;
}

}
