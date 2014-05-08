// TCPWorker.h: interface for the TCPWorker class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TCPWORKER_H
#define TCPWORKER_H

#include "../Micro-Development-Kit/include/frame/netserver/STNetServer.h"
#include "../Micro-Development-Kit/include/mdk/ConfigFile.h"
#include "../Micro-Development-Kit/include/mdk/Signal.h"
#include "../bstruct/include/BStruct.h"
#include "../common/common.h"

#include <vector>
#include <string>
#include <map>
using namespace std;

#define COMMAND_TITLE "mgr:\\>"

class Console;
/*
	tcp工作类
	接收tcp消息，执行业务
 */
class TCPWorker : public mdk::STNetServer
{
	typedef struct Exist
	{
		int hostid;//guide上注册的主机id
		std::string wanIP;//外网ip
		unsigned short wanPort;//外网端口
		std::string lanIP;//内网ip
		unsigned short lanPort;//内网端口
		NodeRole::NodeRole role;//角色
		NodeStatus::NodeStatus status;//状态
		unsigned int pieceNo;//片号，该结点负责保存的hashid分片上的数据
	}Exist;
public:
	TCPWorker(char *ip, unsigned short port, Console *pConsole);
	virtual ~TCPWorker();

	bool WaitConnect( unsigned long timeout );
	char* RemoteCall OnCommand(vector<string> *cmd);
	
private:
	void OnConnect(mdk::STNetHost &host);
	void OnMsg(mdk::STNetHost &host);
	void OnCloseConnect(mdk::STNetHost &host);
	void OnReply(mdk::STNetHost &host, bsp::BStruct &msg);
	void OnSystemStatus(mdk::STNetHost &host, bsp::BStruct &msg);
	//显示分布式网络信息
	bool ShowNet(vector<string> &argv);
	//进行分片
	bool StartPiece(vector<string> &argv);
	//删除结点
	bool DelNode(vector<string> &argv);
	
private:
	unsigned char *m_msgBuffer;//消息缓冲
	mdk::Signal m_sigConnected;
	mdk::STNetHost *m_guide;
	string m_guideIP;
	unsigned short m_guidePort;
	Console *m_pConsole;
	mdk::Signal m_reply;
};	

#endif // !defined TCPWORKER_H
