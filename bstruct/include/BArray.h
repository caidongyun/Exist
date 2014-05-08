// BArray.h: interface for the BArray class.
//
//////////////////////////////////////////////////////////////////////

#ifndef BARRAY_H
#define BARRAY_H

#include "Stream.h"
#include <string>
#include <vector>

/*
 *  二进制对象数组
 *	元素可以是普通的char int float等也可以是BStruct、数组、byte[]
 *	编码格式：元素大小(unsigned short,0表示元素大小变长)+元素1+...+元素n
 *	元素编码格式：如果元素大小非0（定长元素），值
 *	元素编码格式：如果元素大小为0（变长元素），值长度(unsigned short) + 值	
 *
 *
 *	※关于short int等类型，的二进制存在形式的字节序
 *	对于简单类型成员：以低位在前，高位在后
 *		比如BArray msg; msg["prot"] = (short)0xf1f2;这个就是低位在前，在结构中就是0xf2f1
 *	对于变长数据，比如struct中的int 等成员，字节序为机器默认，请自行测试
 *
 *	※对于float double，为memcpy转换进byte流，非c/c++client请自行转换
 *
 *
 *	范例
 *
 *	构造结构
 *	unsigned char buf[256];
 *	BArray msg;
 *	msg.Bind(buf,256);
 *	msg[1] = "192.168.0.1";
 *	msg[2] = (short)8888;
 *
 *	解析结构
 *	BArray msg;
 *	unsigned buf;//任意方式（BStruct成员\网络\文件\内存）得到了一个完整的数组
 *	unsigned short size;//数组二进制流长度
 *	msg.Resolve(buf,size);//结构到内存
 *	msg.GetCount();//元素个数
 *	string str = msg[1];//取得192.168.0.1
 *	short port = msg[2];//取得8888
*/
namespace bsp
{

class BArray;
class BStruct;
/*
 *	成员值
 */
struct E_VALUE
{
	char *m_data;//成员的值的地址，对于struct，byte流等对象就是对象地址
	unsigned short m_size;//成员长度，对于struct，byte流等对象就是对象大小
public:
	E_VALUE(){}
	~E_VALUE(){}

	bool IsValid();//有效返回true,无效返回false
	//设置变长数据到成员中，取变长对象值直接访问m_data m_size
	bool SetValue( const void *value, unsigned short uSize );
	operator std::string();
	bool operator = ( char *value );
	bool operator = ( const char *value )
	{
		return *this = (char*)value;
	}
	operator char();
	operator unsigned char()
	{
		return (char)*this;
	}
	bool operator = ( char value );
	bool operator = ( unsigned char value )
	{
		return *this = (char)value;
	}
	operator short();
	operator unsigned short()
	{
		return (short)*this;
	}
	bool operator = ( short value );
	bool operator = ( unsigned short value )
	{
		return *this = (short)value;
	}
	operator float();
	bool operator = ( float value );
	operator double();
	bool operator = ( double value );
	operator long();
	operator unsigned long()
	{
		return (long)*this;
	}
	bool operator = ( long value );
	bool operator = ( unsigned long value )
	{
		return *this = (long)value;
	}
	operator int32();
	operator uint32()
	{
		return (int32)*this;
	}
	bool operator = ( int32 value );
	bool operator = ( uint32 value )
	{
		return *this = (int32)value;
	}
	operator int64();
	operator uint64()
	{
		return (int64)*this;
	}
	bool operator = ( int64 value );
	bool operator = ( uint64 value )
	{
		return *this = (int64)value;
	}

	operator BArray();
	bool operator = ( BArray *value );
	operator BStruct();
	bool operator = ( BStruct *value );
		
private:
	BArray *m_parent;
	friend class BArray;
};

class BArray  
{
private:
	enum action
	{
		unknow = 0,
		write = 1,
		read = 2
	};
	friend struct E_VALUE;
public:
	BArray();
	virtual ~BArray();

public:
	/*
		设置元素长度，不设置默认为变长元素，设置了为定长元素
		※必须在Bind()前调用，Bind()会将该信息编码进入流中，Bind()再设置就无效了
		※对于解析，SetElementSize()无效，Resolve()会从流中得到数组元素类型是定长还是变长，
		覆盖SetElementSize()结果
	*/
	void SetElementSize(unsigned short size);
	void Bind(unsigned char *pBuffer, unsigned int uSize);//绑定缓冲,用户填充
	//解析数据流，解析后使用[]操作符直接取出对应成员结果
	bool Resolve(unsigned char *pBuffer, unsigned int uSize);
	/*
	 *	取得成员name，做取值/赋值操作
	 *	赋值：
	 *		可以直接将char short int...基本类型保存到结构中
	 *		成功返回true失败返回false，一般是长度不够
	 *		比如可以向下面这样检查赋值操作是否成功
	 *		if ( !(msg["port"] = 8080) ) ...;
	 *		对于数据结构，可以使用SetValue进行赋值，比如
	 *		if ( !msg["ip"].SetValue( "127.0.0.1", 9 ) ) ...;
	 *
	 *		※1：如果直接传递常量，将自动匹配到4~nbyte中最小的类型
	 *		比如msg["port"] = 1;将认为是插入一个int，占4byte，如果希望只占1byte，请msg["port"] = (char)1;
	 *		比如msg["port"] = 0xffffffff将认为是插入一个unsigned int，占4byte
	 *		比如msg["port"] = 0xffffffff ff将认为是插入一个int64，占8byte
	 *
	 *		※2：成员第一次被赋值后，长度就确定了，以后只可以修改为相同大小的值，
	 *		否则触发assert断言
	 *
	 *		※3：没有调用Bind()，不可以赋制，否则触发assert断言
	 */
	E_VALUE operator []( int index );
	unsigned char* GetStream();//取得数据流
	unsigned int GetSize();//取得数据流长度
	unsigned short GetElementSize();//取得元素长度
	unsigned int GetCount();//取得数组大小
   /*
		为保存某个变长对象对象准备缓冲，返回缓冲首地址
		与PreSize()一起使用，得到刚准备的缓冲的容量
		对象填充完毕后，必须使用使用[]操作符，将对象保存到结构中，
		才能再次调用PreBuffer为新成员准备缓冲
		例：
		unsigned char buf[1024]//消息缓冲
		BArray msg(buf,1024,BArray::write);//创建一个结构，使用buf作为缓冲
		//创建一个client成员，使用msg末尾的buf作为item的缓冲
		//将成员名字client保存到msg的缓冲（buf）的末尾，
		//所以不能连续调用PreBuffer，必须完成后续步骤
		BArray item(msg.PreBuffer("client"),msg.PreSize(),BArray::write);
		填充item，自由填充，超长会自动报错
		msg["client"] = item;//将item连接到msg数据流的末尾
	*/
	unsigned char* PreBuffer();
	unsigned int PreSize();//准备的缓冲的容量
	bool IsValid();//有效返回true,无效返回false
	bool IsEmpty();//空返回true
private:
	bool Resolve();//解析绑定的数据流
private:
	Stream m_stream;
	std::vector<char*> m_data;
	E_VALUE m_error;//操作失败时返回错误数据
	bool m_finished;
	action m_action;
	bool m_bValid;
	bool m_bEmpty;
	unsigned short m_elementSize;
};

}
#endif // !defined(BARRAY_H)
