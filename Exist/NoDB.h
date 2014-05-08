// NoDB.h: interface for the NoDB class.
//
//////////////////////////////////////////////////////////////////////

#ifndef NOSQL_H
#define NOSQL_H

#include <ctime>
#include "../Micro-Development-Kit/include/frame/netserver/STNetServer.h"
#include "../Micro-Development-Kit/include/mdk/ConfigFile.h"
#include "../Micro-Development-Kit/include/mdk/Signal.h"
#include "../Micro-Development-Kit/include/mdk/Logger.h"
#include "../Micro-Development-Kit/include/mdk/Thread.h"
#include "../Micro-Development-Kit/include/mdk/Lock.h"
#include "RHTable.h"
#include <string>

#include "../bstruct/include/BArray.h"

#include "../common/common.h"
#include "../common/Protocol.h"

class NoDB : public mdk::STNetServer
{
/*
 *	NoDB上下文
 */
typedef struct NO_DB_CONTEXT
{
	mdk::STNetHost host;
	bool bConnected;
	std::string distributed;
	std::string guideIP;
	unsigned short guidePort;
	std::string databaseIP;
	unsigned short databasePort;
	std::string masterIP;
	unsigned short masterPort;
	
	std::string role;
	/*
		分片号
		distributed=true并且role=piece时必须配置，否则忽略
		piece类型的结点负责提供服务的数据所在的数据区间编号
		片号从0开始,片号的计算公式为:数据的hashvalue/分片大小
		比如总数据中hashvalue最大的1234,分片大小被设置为100,那么总数据就被分为了11个分片
		0号分片上是hashvalue为0~99的数据 0~99之间任何数字/100 = 0,
		1号分片上是hashvalue为100~199的数据 100~199之间任何数字/100 = 1,
		...
		10号分片上是hashvalue为1000~1999的数据 1000~1999之间任何数字/100 = 10,

		配置小于0的片号时,表示分片启动后,不加载数据,只是挂在网络中待机，
		等待用户指令确定对几号片的数据进行加载,提供服务
	*/
	int pieceNo;
	std::string dataRootDir;
	unsigned short port;
	std::string wanIP;
	std::string lanIP;
	unsigned int maxMemory;
	bool m_bStop;
}NO_DB_CONTEXT;

public:
	NoDB(char* cfgFile);
	virtual ~NoDB();

public:
	mdk::Logger& GetLog();//取得日志对象
		
protected:
	/*
	 *	服务器主线程
	 */
	void* Main(void* pParam);
	
	//连接到达响应
	void OnConnect(mdk::STNetHost &host);
	//数据到达响应
	void OnMsg(mdk::STNetHost &host);
	//接收Guide连接上的数据并处理，如果host不是Guide则什么都不做，返回false
	bool OnGuideMsg( mdk::STNetHost &host );
	//接收client连接上的数据并处理，如果host是Guide则什么都不做，返回false
	bool OnClientMsg( mdk::STNetHost &host );
	//连接断开响应
	void OnCloseConnect(mdk::STNetHost &host);
	//结点校正响应
	void OnNodeCheck( mdk::STNetHost &host, bsp::BStruct &msg );
	//设置数据库
	void OnSetDatabase( mdk::STNetHost &host, bsp::BStruct &msg );
	//设置主片
	void OnSetMaster( mdk::STNetHost &host, bsp::BStruct &msg );
		
private:
	//退出程序
	void* RemoteCall Exit(void* pParam);
	bool CreateDir( const char *strDir );
	bool CreateFile( const char *strFile );
	/*
		从硬盘读取数据
		单机Exist或分布式Exist的数据库结点，调用
	*/
	void ReadData();
	/*
		下载数据
		分布式Exist的非数据库结点，调用
	 */
	void* RemoteCall DownloadData(void* pParam);
	void Heartbeat( time_t tCurTime );//心跳
	void SaveData(time_t tCurTime);//数据持久化
	void ConnectGuide(mdk::STNetHost &host);//连接到向导服务
	void ConnectMaster(mdk::STNetHost &host);//连接到主片
	void ConnectDB(mdk::STNetHost &host);//连接到数据库
	void SendReadyNotify();//发送就绪通知
	
private:
	mdk::Logger m_log;
	mdk::ConfigFile m_cfgFile;
	NO_DB_CONTEXT m_context;//NoDB上下文，包含（运行模式，角色等）
	mdk::Thread t_exit;//退出程序线程
	unsigned char *m_msgBuffer;//消息缓冲
	NodeStatus::NodeStatus m_status;//状态
	mdk::Signal m_waitCheck;//等待Guide验证完毕
	mdk::Signal m_waitMain;//等待主线程激或完毕
	bool m_bCheckVaild;//验证结果为有效
	bool m_bGuideConnected;//是否连接到guide
	mdk::STNetHost m_guideHost;//guide主机
	mdk::STNetHost m_databaseHost;//数据库
	mdk::STNetHost m_masterHost;//主片
	mdk::Thread t_loaddata;//数据加载线程
	mdk::Mutex m_ioLock;//io锁
	bool m_bNotifyReady;//就绪通知已发送
	RHTable m_nosqlDB;
};

#endif // !defined NOSQL_H
