#include "Protocol.h"
//发送BStruct格式的报文
bool SendBStruct( mdk::STNetHost &host, bsp::BStruct &msg )
{
	unsigned char *buf = msg.GetStream();
	buf -= MSG_HEAD_SIZE;
	bsp::Stream head;
	head.Bind(buf, MSG_HEAD_SIZE);
	if ( !head.AddData((unsigned char)DataFormat::BStruct) ) return false;
	if ( !head.AddData((unsigned int)msg.GetSize()) ) return false;

	return host.Send( buf, MSG_HEAD_SIZE + msg.GetSize() );
}
