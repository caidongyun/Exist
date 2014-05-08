#include <cstdio>
#include "../include/BStruct.h"
#include "../include/BArray.h"
using namespace std;

int main()
{
	unsigned char buf[256];
	//构造一个结构化消息
	bsp::BStruct p;
	p.Bind(buf,256);//与缓冲绑定，以后往结构里添加数据，都会到添加到buf末尾
	bool b = p["1"] = "1";//往结构里添数据
	b = p["2"] = (unsigned char)2;//往结构里添数据
	b = p["3"] = (unsigned short)3;//往结里构添数据
	b = p["4"] = (unsigned int)4;//往结构里添数据
	b = p["5"] = (float)5;//往结构里添数据
	b = p["6"] = (double)6;//往结构里添数据
	b = p["7"] = (unsigned long)7;//往结构里添数据
	b = p["8"] = (uint32)8;//往结构里添数据
	struct BSD
	{
		int i;
		short s;
	};
	bsp::BArray marray;
	marray.Bind(p.PreBuffer( "20" ), p.PreSize());
	marray[0] = "123";
	bsp::BStruct estr;
	estr.Bind(marray.PreBuffer(),marray.PreSize());
	estr["999"] = 999;
	estr["888"] = 888;
	marray[1] = &estr;//BArray中嵌套BStruct
	p["20"] = &marray;//BStruct中嵌套BArray
	BSD sdasad;
	sdasad.i = 123;
	sdasad.s = 456;
	//往结构里添struct，java等不支持strcut的语言，可能无法解析这类数据
	//建议只使用通用类型数据，char int 字符串 byte数组等
	p["13"].SetValue(&sdasad,sizeof(BSD));
	p["9"] = (uint64)9;//往结构里添数据
	p["12"].SetValue( "huoyu", 5/*不需要将\0复制进去*/ );//往结构里添字符串
	bsp::BStruct obj;//包含一个子结构
	obj.Bind(p.PreBuffer( "10" ), p.PreSize());
	obj["o1"] = 10;
	obj["o2"] = "11";
	obj["o3"] = 12;
	p["10"] = &obj;//往结构里嵌套结构
	p["11"] = 13;
	//将结构发送到网络，这里用memcpy到内存，模拟网络发送/接收
	char msg[1024];
	int nLen;
	bsp::itomem( (unsigned char*)msg, p.GetSize(), 2 );//发送出去
	memcpy( &msg[2], p.GetStream(), 2 );//发送出去
	nLen = bsp::memtoi((unsigned char*)msg, 2);
	memcpy( msg, p.GetStream(), nLen );//发送出去
	

	//接收并解析结构
	nLen = p.GetSize();//接收到结构大小
	bsp::BStruct p1;
	p1.Resolve((unsigned char*)msg,nLen);//解析
	//取数据
	string str = (string)p1["1"];
	if ( !p1["2"].IsValid() ) return 0;//检查名字为2的数据是否有效（是否存在）
	char c = p1["2"];
	short s = p1["3"];
	int i = p1["4"];
	float f = p1["5"];
	double d = p1["6"];
	long l = p1["7"];
	int32 i32 = p1["8"];
	int64 i64 = p1["9"];
	bsp::BStruct po2 = p1["10"];
	if ( !po2.IsValid() ) return 0;//检查嵌套的结构是否有效（是否成功解析）
	printf( "%s\n", str.c_str() );
	printf( "%d\n", c );
	printf( "%d\n", s );
	printf( "%d\n", i );
	printf( "%f\n", f );
	printf( "%f\n", d );
	printf( "%d\n", l );
	printf( "%d\n", i32 );
	printf( "%d\n", i64 );
	str = (string)p1["12"];
	printf( "%s\n", str.c_str() );
	BSD *ds = (BSD*)p1["13"].m_data;
	printf( "%d\n", ds->i );
	printf( "%d\n", ds->s );
	i = po2["o1"];
	str = (string)po2["o2"];
	i32 = po2["o3"];
	int i11 = p1["11"];
	printf( "%d\n", i );
	printf( "%s\n", str.c_str() );
	printf( "%d\n", i32 );
	printf( "%d\n", i11 );

	bsp::BArray marrayr;
	marrayr = p1["20"];
	bsp::BStruct estrr = marrayr[1];
	i = estrr["888"];
	printf( "%d\n", i );
	i = estrr["999"];
	printf( "%d\n", i );
	str = (string)marray[0];
	printf( "%s\n", str.c_str() );
	
	return 0;
}
