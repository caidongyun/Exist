// Protocl.h: interface for the Protocl class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../Micro-Development-Kit/include/frame/netserver/STNetHost.h"
#include "../bstruct/include/BStruct.h"

#define MAX_BSTRUCT_SIZE	1048576 //BStruct结构最大长度1M
#define MAX_MSG_SIZE		MAX_BSTRUCT_SIZE + MSG_HEAD_SIZE//消息缓冲最大长度，报文头+BStruct最大大小

//数据格式
namespace DataFormat
{
	enum DataFormat
	{
		Stream = 0,//流格式
		BStruct = 1,//BStruct格式
	};
}

//报文头
typedef struct MSG_HEAD
{
	DataFormat::DataFormat dataFormat;//数据格式(unsigned char)
	unsigned int dataSize;//数据长度
}MSG_HEAD;

//报文头长度
#define MSG_HEAD_SIZE sizeof(unsigned char)+sizeof(unsigned int)

//报文id
namespace msgid
{
	enum msgid
	{
		unknow = 0,//未知报文
		eHeartbeat = 1,//心跳
		eNodeJoin = 2,//exist节点请求加入分布式系统中,校正并记录ip、角色、片号等信息,回应gNodeCheck进行校正
		gNodeCheck = 3,//结点校正指令
		gSetDatabase = 4,//设置数据库，让结点确定数据库
		gSetMaster = 5,//设置主分片，让结点确定主分片
		uCommand = 6,//人工控制指令
		gCommandResult = 7,//指令执行结果
		gCreatePiece = 8,//通知结点创建分片
		gExitNet = 9,//通知结点退出网络
		eNodeExit = 10,//结点退出网络请求
		gSystemStatus = 11,//系统状态信息通知
		eNodeReady = 12,//结点就绪通知
		mQueryExistData = 13,//查询Exist数据
	};
}

//人工控制指令
namespace UserCmd
{
	enum UserCmd
	{
		unknow = 0,//未知指令
		QueryNodeList = 1,//查询所有结点
		StartPiece = 2,//开始分片
		NodeDel = 3,//删除结点
	};
}


//发送BStruct格式的报文
inline bool SendBStruct( mdk::STNetHost &host, bsp::BStruct &msg )
{
	unsigned char *buf = msg.GetStream();
	buf -= MSG_HEAD_SIZE;
	bsp::Stream head;
	head.Bind(buf, MSG_HEAD_SIZE);
	if ( !head.AddData((unsigned char)DataFormat::BStruct) ) return false;
	if ( !head.AddData((unsigned int)msg.GetSize()) ) return false;

	return host.Send( buf, MSG_HEAD_SIZE + msg.GetSize() );
}


#endif // !defined(PROTOCOL_H)
