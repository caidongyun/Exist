#ifndef NET_MSG_H
#define NET_MSG_H

#ifdef WIN32
#else
#include <sys/types.h>
#endif

typedef int					int32;
typedef unsigned int		uint32;
#ifdef WIN32
typedef __int64				int64;
typedef unsigned __int64	uint64;
#else
typedef int64_t				int64;
typedef u_int64_t			uint64;
#endif

namespace bsp
{

/**
* 流类
* 构造/解析消息流
* 构造消息，首先绑定保存消息的缓冲，然后调用AddData往里面添加数据
* 解析消息，首先绑定要解析的消息缓冲，然后调用GetData取得参数
*/
class Stream
{
public:
	Stream();
	virtual ~Stream();

public:
	void Bind(unsigned char *pBuffer, unsigned int uSize);//绑定一个缓冲区，在此之上做转换
	//取得缓冲容量(对应创建数据流)/消息长度(对应解析数据流)
	unsigned int GetSize();
//////////////////////////////////////////////////////////////////////////
//组装消息流，用于输出
	//取得组装好的数据流,并在头部设置数据长度
	unsigned char* GetStream();
	//取得当前游标位置（对于组装消息即数据长度，对于解析消息即下一个数据的开始位置）
	unsigned int Pos();
	//组装方法，填充参数到流
	/**
	 * 增加1个参数到消息中,char、unsigned char、int等各种类型的重载
	 * 成功返回true，失败返回false
	 * ※直接传递常量，将自动匹配到4~nbyte中最小的类型
	 * 比如AddData( 1 )将认为是插入一个int，占4byte，如果希望只占1byte，请使用AddData((char)1)
	 * 比如AddData( 0xffffffff )将认为是插入一个unsigned int，占4byte
	 * 比如AddData( 0xffffffff ff)将认为是插入一个int64，占8byte
	*/
	bool AddData(char value);
	inline bool AddData( unsigned char value )
	{
		return AddData( (char)value );
	}
	bool AddData(short value);
	inline bool AddData( unsigned short value )
	{
		return AddData( (short)value );
	}
	bool AddData(float value);
	bool AddData(double value);
	bool AddData(long value);
	inline bool AddData( unsigned long value )
	{
		return AddData( (long)value );
	}
	bool AddData(int32 value);
	inline bool AddData( uint32 value )
	{
		return AddData( (int32)value );
	}
	bool AddData(int64 value);
	inline bool AddData( uint64 value )
	{
		return AddData( (int64)value );
	}
	
	/**
	 * 增加1个结构型参数到消息中
	 * void*表示结构地址
	 * unsigned short表示结构大小，
	 * byte流和字符串视为特殊的数据结构
	 * 
	 * 成功返回true，失败返回false
	 */
	bool AddData(const void* pStruct, unsigned short uSize);
	
//////////////////////////////////////////////////////////////////////////
//解析绑定的数据流，取得消息参数
	//已读到末尾
	bool IsEnd();
	/**
	* 从流中取得一个数据char、unsigned char、int等各种类型的重载
	* 成功返回true，失败返回false
	*/
	bool GetData(char *value);
	inline bool GetData(unsigned char *value)
	{
		return GetData((char*) value);
	}
	bool GetData(short *value);
	inline bool GetData(unsigned short *value)
	{
		return GetData((short*) value);
	}
	bool GetData(float *value);
	bool GetData(double *value);
	bool GetData(long *value);
	inline bool GetData(unsigned long *value)
	{
		return GetData((long*) value);
	}
	bool GetData(int32 *value);
	inline bool GetData(uint32 *value)
	{
		return GetData((int32*) value);
	}
	bool GetData(int64 *value);
	inline bool GetData(uint64 *value)
	{
		return GetData((int64*) value);
	}
	/**
	 * 从消息中取得变长对象
	 * 成功返回true，失败返回false
	 * 
	 * pStruct保存对象的缓冲
	 * uSize输入缓冲容量，返回实际读出大小
	 * 
	 */
	bool GetData(void* pStruct, short *uSize);
	bool GetData(void* pStruct, unsigned short *uSize)
	{
		return GetData( pStruct, (short*)uSize );
	}
	/**
	 * 从消息中取得变长对象的指针
	 * 成功返回对象指针，失败返回NULL
	 * uSize返回地址大小
	*/
	unsigned char* GetPointer( short *uSize );
	unsigned char* GetPointer( unsigned short *uSize )
	{
		return GetPointer( (short*)uSize );
	}
protected:
	//绑定的缓冲区
	unsigned char* m_pMsgBuffer;
	//缓冲大小/被解析消息长度
	unsigned int m_uSize;
	//消息填充/解析到的位置
	unsigned int m_uPos;
	
};

//将buf字节按照低字节在前，高字节在后的顺序组合成一个整数
uint64 memtoi( unsigned char *buf, int size );
//将一个整数，按照低字节在前，高字节在后的顺序保存到buf
void itomem( unsigned char *buf, uint64 value, int size );
}

#endif //NET_MSG_H
