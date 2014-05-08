// TCPWorker.h: interface for the TCPWorker class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TCPWORKER_H
#define TCPWORKER_H

#include "../Micro-Development-Kit/include/frame/netserver/STNetServer.h"
#include "../Micro-Development-Kit/include/mdk/ConfigFile.h"
#include "../Micro-Development-Kit/include/mdk/Logger.h"
#include "../bstruct/include/BStruct.h"
#include "../common/common.h"

#include <vector>
#include <map>
using namespace std;

#define MAX_BSTRUCT_SIZE	1048576 //BStruct结构最大长度1M
#define MAX_MSG_SIZE		MAX_BSTRUCT_SIZE + MSG_HEAD_SIZE//消息缓冲最大长度，报文头+BStruct最大大小

/*
	tcp工作类
	接收tcp消息，执行业务
 */
class TCPWorker : public mdk::STNetServer
{
	typedef struct Exist
	{
		mdk::STNetHost host;//主机
		std::string wanIP;//外网ip
		std::string lanIP;//内网ip
		unsigned short port;//服务端口
		NodeRole::NodeRole role;//角色
		NodeStatus::NodeStatus status;//状态
		unsigned int pieceNo;//片号0开始 = hashid/片大小，该结点负责保存的hashid分片上的数据 
	}Exist;
	/*
	 *	结点映射表
	 *	key = host.ID()
	 */
	typedef map<int, Exist*> NodeList;
	/*
	 *	分片映射表
	 *	key = 分片id
	 */
	typedef map<int, Exist*> PieceList;
public:
	TCPWorker( char *cfgFile );
	virtual ~TCPWorker();
	mdk::Logger& GetLog();

private:
	bool ReadConfig();
	void InvalidMsg(mdk::STNetHost &host, unsigned char *msg, unsigned short len);//非法报文
	void OnMsg(mdk::STNetHost &host);
	void OnCloseConnect(mdk::STNetHost &host);
	//////////////////////////////////////////////////////////////////////////
	//报文响应
	bool OnNodeJoin(mdk::STNetHost &host, bsp::BStruct &msg);//结点加入分布网络
	bool OnNodeReady(mdk::STNetHost &host);//结点就绪
	bool OnNodeExit(mdk::STNetHost &host);//结点退出	
	bool OnUserCommand(mdk::STNetHost &host, bsp::BStruct &msg);//响应人工控制指令
	bool OnQueryNodeList(mdk::STNetHost &host);//查询所有结点
	bool OnStartPiece(mdk::STNetHost &host, bsp::BStruct &msg);//开始分片指令
	bool OnNodeDel(mdk::STNetHost &host, bsp::BStruct &msg);//删除结点指令
	
private:
	//////////////////////////////////////////////////////////////////////////
	//业务处理Worker.cpp中定义
	void DBJoin(mdk::STNetHost &host, bsp::BStruct &msg);//数据库加入到网络中
	void MasterJoin(mdk::STNetHost &host, bsp::BStruct &msg);//主分片加入到网络中
	void PieceJoin(mdk::STNetHost &host, bsp::BStruct &msg);//分片加入到网络中
	void DatabaseReady();//数据库就绪
	void DBExit();//数据库退出
	char* StartPiece(unsigned int pieceNo);//开始分片，成功返回NULL,失败返回原因
	char* DelNode(Exist *pExist);//删除一个结点，成功返回NULL,失败返回原因
	int NewPieceNo();//返回一个新片号
		
	//////////////////////////////////////////////////////////////////////////
	//回复消息ReplyMsg.cpp中定义
	void NodeCheck(mdk::STNetHost &host, unsigned char role, int pieceNo, char *reason );//结点信息校正
	void SetDatabase(mdk::STNetHost &host);//设置数据库
	void SetMaster(mdk::STNetHost &host);//设置主分片
	void NodeInit(Exist *pExist);//通知exist结点初始化分片
	void NodeStop(Exist *pExist);//通知exist结点停止服务
	void NodeExit(Exist *pExist);//通知exist结点退出网络

	/*
		用户控制台指令操作结果回复
		reason=NULL表示操作成功
		msg该回复对应的操作指令
	*/
	void ReplyUserResult(mdk::STNetHost &host, bsp::BStruct &msg, char *reason);
	void SendSystemStatus(mdk::STNetHost &host);//发送系统状态（所有结点信息）
	

private:
	mdk::Logger m_log;//运行log
	mdk::ConfigFile m_cfgFile;//配置文件
	Exist m_db;//持久化数据库
	Exist m_master;//主分片
	NodeList m_nodes;//在线结点列表
	PieceList m_pieces;//分片列表
	int m_count;//最大exist数
	int m_pieceSize;//片大小
	int m_port;//接受exist进程连接的端口
	int m_heartTime;//exist心跳时间
	unsigned char *m_msgBuffer;//消息缓冲

};

#endif // !defined TCPWORKER_H
