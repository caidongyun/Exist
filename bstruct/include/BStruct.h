// BStruct.h: interface for the BStruct class.
//
//////////////////////////////////////////////////////////////////////

#ifndef BSTRUCT_H
#define BSTRUCT_H

#include "Stream.h"
#include <map>
#include <string>

/*
 *  二进制结构
 *	是将二进制数据结构化的保存在数据流中的通信协议
 *	成员由名字-值组成,类似xml可以定义复杂成员（嵌套）
 *
 *	结构的二进制格式=成员1~成员n的总长度(unsigned short)+成员1+成员2+成员3+成员4+...+成员n,成员之间无分割符
 *	最多256个成员，如果不够可以将部分成员整合为一个BStruct嵌套进去
 *
 *	成员格式=成员名长度(unsigned short) + 成员名(string) 
 *			+ 值长度(unsigned short) + 值(byte流)
 *
 *	因为每个成员，和成员值都是2进制流的表示，所以可以把n个成员，作为一个值
 *	比如某个成员teacher，它的值可以是成员1+成员2+...+成员n
 *
 *	※关于short int等类型，的二进制存在形式的字节序
 *	对于简单类型成员：以低位在前，高位在后
 *		比如BStruct msg; msg["prot"] = (short)0xf1f2;这个就是低位在前，在结构中就是0xf2f1
 *	对于变长数据，比如struct中的int 等成员，字节序为机器默认，请自行测试
 *
 *	※对于float double，为memcpy转换进byte流，非c/c++client请自行转换
 *
 *
 *	范例
 *
 *	构造结构
 *	范例1,简单成员：
 *	unsigned char buf[256];
 *	BStruct msg;
 *	msg.Bind(buf,256);
 *	msg["ip"] = "192.168.0.1";
 *	msg["port"] = (short)8888;
 *	//※赋值有可能会失败，可以这样检查if ( !(msg["ip"] = "192.168.0.1") )
 *	//失败原因只有4个且都是不正确的使用BStruct引起，所以检查赋值结果其实没有实际意义，
 *	//因为你已经用法就错误了，即使检查捕获了错误，报文也是非法的，程序虽然避免了崩溃的危险，
 *	//但逻辑上已经是不可用的。
 *	//唯一意义就是，调试阶段检查可以帮助发现代码漏洞，即时修正
 *	//1.buf为NULL或长度不足
 *	//2.当前模式是不是创建（即没有调用Bind()或Bind()之后又调用Resolve()模式被覆盖为解析
 *	//3.已经存在同名成员，且当前数据长度与新写入长度不相同，不允许修改数据
 *	//4.创建了新成员，没有完成赋值msg["new_member"];这样就会创建新成员不做赋值
 *	
 *	范例2,复杂数据结构：
 *	结构包含3个数据:配置文件，心跳时间，连接地址，其中连接地址是一个复杂数据结构，包含ip port2个数据
 *	unsigned char buf[256];
 *	BStruct msg;
 *	msg.Bind(buf,256);
 *	if ( !msg["cfg_file"].IsValid() )//检查cfg_file是否存在
 *	//由于是范例以下不检查数据合法性，实际使用时建议都检查
 *	msg["cfg_file"] = "etc\ser.cfg";//设置配置文件
 *	msg["heart_time"] = (char)60;//设置心跳时间
 *	BStruct adr;
 *	adr.Bind(msg.PreBuffer("ser"),msg.PreSize());//绑定到buf末尾
 *	adr["ip"] = "192.168.0.1";
 *	adr["port"] = (short)8888;
 *	msg["connect_adr"] = adr;//设置连接地址
 *
 *	范例3,复杂数据结构填充方式2，只适用于c/c++：
 *	结构内容同上
 *	unsigned char buf[256];
 *	BStruct msg;
 *	msg.Bind(buf,256);
 *	msg["cfg_file"] = "etc\ser.cfg";//设置配置文件
 *	msg["heart_time"] = (char)60;//设置心跳时间
 *	struct adr
 *	{
 *		char ip[21];
 *		short port;
 *	}
 *	adr connect_adr;
 *	memcpy( connect_adr.ip, "192.168.0.1", strlen("192.168.0.1") );
 *	connect_adr.prot = 8888;
 *	msg["connect_adr"].SetValue(&connect_adr,sizeof(adr));//设置连接地址
 *
 *	解析结构
 *
 *	范例1：解析构造范例1中结构：
 *	BStruct msg;
 *	unsigned buf;//任意方式（网络\文件\内存）得到了一个完整的结构
 *	unsigned short size;//结构长度
 *	msg.Resolve(buf,size);//结构到内存
 *	string str = msg["ip"];//取得192.168.0.1
 *	short port = msg["port"];//取得8888
 *	
 *	范例2：解析构造范例2中结构：
 *	BStruct msg;
 *	unsigned buf;//任意方式（网络\文件\内存）得到了一个完整的结构
 *	unsigned short size;//结构长度
 *	msg.Resolve(buf,size);//结构到内存
 *	string cfg = msg["cfg_file"];//取得etc\ser.cfg
 *	char ht = msg["heart_time"];//取得60;
 *	BStruct adr = msg["connect_adr"];//取得复杂成员
 *	string ip = adr["ip"];//取得192.168.0.1
 *	short port = adr["port"];//取得8888
 *	
 *	范例3：解析构造范例3中结构：
 *	BStruct msg;
 *	unsigned buf;//任意方式（网络\文件\内存）得到了一个完整的结构
 *	unsigned short size;//结构长度
 *	msg.Resolve(buf,size);//结构到内存
 *	string cfg = msg["cfg_file"];//取得etc\ser.cfg
 *	char ht = msg["heart_time"];//取得60;
 *	struct adr
 *	{
 *		char ip[21];
 *		short port;
 *	}
 *	adr *connect_adr = msg["connect_adr"].m_data;//取得复杂数据结构地址
 *	connect_adr->ip;//192.168.0.1
 *	connect_adr->port;//取得8888
*/
namespace bsp
{

class BArray;
class BStruct;
/*
 *	成员值
 */
struct M_VALUE
{
	char *m_data;//成员的值的地址，对于struct，byte流等对象就是对象地址
	unsigned short m_size;//成员长度，对于struct，byte流等对象就是对象大小
public:
	M_VALUE(){}
	~M_VALUE(){}

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

	operator BStruct();
	bool operator = ( BStruct *value );
	operator BArray();
	bool operator = ( BArray *value );
		
private:
	BStruct *m_parent;
	friend class BStruct;
};

class BStruct  
{
private:
	enum action
	{
		unknow = 0,
		write = 1,
		read = 2
	};
	friend struct M_VALUE;
public:
	BStruct();
	virtual ~BStruct();

public:
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
	M_VALUE operator []( std::string name );
	unsigned char* GetStream();//取得数据流
	unsigned int GetSize();//取得数据流长度
	/*
		为保存某个变长对象对象准备缓冲，返回缓冲首地址
		与PreSize()一起使用，得到刚准备的缓冲的容量
		对象填充完毕后，必须使用使用[]操作符，将对象保存到结构中，
		才能再次调用PreBuffer为新成员准备缓冲
		例：
		unsigned char buf[1024]//消息缓冲
		BStruct msg(buf,1024,BStruct::write);//创建一个结构，使用buf作为缓冲
		//创建一个client成员，使用msg末尾的buf作为item的缓冲
		//将成员名字client保存到msg的缓冲（buf）的末尾，
		//所以不能连续调用PreBuffer，必须完成后续步骤
		BStruct item(msg.PreBuffer("client"),msg.PreSize(),BStruct::write);
		填充item，自由填充，超长会自动报错
		msg["client"] = item;//将item连接到msg数据流的末尾
	*/
	unsigned char* PreBuffer( char *key );
	unsigned int PreSize();//准备的缓冲的容量
	bool IsValid();//有效返回true,无效返回false
	bool IsEmpty();//空返回true
private:
	bool Resolve();//解析绑定的数据流
private:
	Stream m_stream;
	M_VALUE m_error;//操作失败时返回错误数据
	std::map<std::string, char*> m_dataMap;
	bool m_finished;
	action m_action;
	bool m_bValid;
	bool m_bEmpty;
};

}
#endif // !defined(BSTRUCT_H)
