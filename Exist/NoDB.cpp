// NoDB.cpp: implementation of the NoDB class.
//
//////////////////////////////////////////////////////////////////////

#include "NoDB.h"

#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <direct.h>
#ifdef WIN32
#else
#include <unistd.h>
#endif

#ifdef WIN32
#else
#include <sys/time.h>
#endif

#include "../Micro-Development-Kit/include/mdk/Socket.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool NoDB::CreateDir( const char *strDir )
{
	string path = strDir;
	int startPos = 0;
	int pos = path.find( '\\', startPos );
	string dir;
	while ( true )
	{
		if ( -1 == pos ) dir = path;
		else dir.assign( path, 0, pos );
		if ( -1 == access( dir.c_str(), 0 ) )
		{
#ifdef WIN32
			if( 0 != mkdir(dir.c_str()) ) return false;
#else
			umask(0);
			if( 0 != mkdir(strDir, 0777) ) return false;
			umask(0);
			chmod(strDir,0777);
#endif
		}
		if ( -1 == pos ) break;
		startPos = pos + 1;
		pos = path.find( '\\', startPos );
	}
	
	return true;
}

bool NoDB::CreateFile( const char *strFile )
{
	if ( 0 == access( strFile, 0 ) ) 
	{
#ifndef WIN32
		umask(0);
		chmod(strFile,0777);
#endif
		return true;
	}
	
	string file = strFile;
	int pos = file.find_last_of( '\\', file.size() );
	if ( -1 != pos ) 
	{
		string dir;
		dir.assign( file, 0, pos );
		if ( !CreateDir(dir.c_str()) ) return false;
	}
	FILE *fp = fopen( strFile, "w" );
	if ( NULL == fp ) return false;
	fclose( fp );
#ifdef WIN32
#else
	umask(0);
	chmod(strFile,0777);
#endif
	return true;
}

NoDB::NoDB(char* cfgFile)
:m_cfgFile(cfgFile)
{
	m_msgBuffer = new unsigned char[MAX_MSG_SIZE];
	if ( NULL == m_msgBuffer ) 
	{
		printf( "内存不足\n" );
		return;
	}

	m_bGuideConnected = false;
	m_bNotifyReady = false;
	m_status = NodeStatus::Idle;
	m_context.m_bStop = false;
	m_context.distributed = (string)m_cfgFile["distributed"];
	m_context.guideIP = (string)m_cfgFile["guide ip"];
	m_context.guidePort = m_cfgFile["guide port"];
	m_context.databaseIP = (string)"";
	m_context.databasePort = 0;
	m_context.masterIP = (string)"";
	m_context.masterPort = 0;
	m_context.role = (string)m_cfgFile["role"];
	m_context.pieceNo = m_cfgFile["pieceNo"];
	m_context.dataRootDir = (string)m_cfgFile["data root dir"];
	m_context.port = m_cfgFile["port"];
	m_context.wanIP = (string)m_cfgFile["wan IP"];
	m_context.lanIP = (string)m_cfgFile["lan IP"];
	m_context.maxMemory = m_cfgFile["max memory"];

	if ( "true" == m_context.distributed ) 
	{
		Connect( m_context.guideIP.c_str(), m_context.guidePort );
		if ( "database" == m_context.role ) CreateDir( m_context.dataRootDir.c_str() );
	}
	else 
	{
		CreateDir( m_context.dataRootDir.c_str() );
		m_waitCheck.Notify();
	}
	m_nosqlDB.SetRemoteMode(true);
	m_bCheckVaild = true;
}

NoDB::~NoDB()
{
	if ( NULL != m_msgBuffer )
	{
		delete[]m_msgBuffer;
		m_msgBuffer = NULL;
	}
}

mdk::Logger& NoDB::GetLog()
{
	return m_log;
}

void* NoDB::Exit(void* pParam)
{
	m_context.m_bStop = true;
	Stop();
	return 0;
}

/*
 *	服务器主线程
 *  定时写文件
 */
void* NoDB::Main(void* pParam)
{
	if ( "true" == m_context.distributed ) 
	{
		if ( !m_waitCheck.Wait( 3000 ) )
		{
			printf( "无法连接向导\n" );
			t_exit.Run(mdk::Executor::Bind(&NoDB::Exit), this, NULL);
			return 0;
		}
	}
	else m_waitCheck.Wait();
	if ( !m_bCheckVaild ) return 0;
	if ( "true" == m_context.distributed ) 
	{
		printf( "分布式打开\n" );
		printf( "结点配置验证通过\n" );
		if ( "database" == m_context.role ) printf( "当前角色：数据库\n" );
		else if ( "master" == m_context.role ) printf( "当前角色：主片\n" );
		else if ( "piece" == m_context.role ) printf( "当前角色：分片\n", m_context.pieceNo );
	}
	else 
	{
		printf( "分布式未打开\n" );
	}
	m_waitMain.Notify();
	ReadData();//从硬盘读数据
	//后台循环业务
	time_t tCurTime = 0;
	while ( !m_context.m_bStop )
	{
		tCurTime = time( NULL );
		Heartbeat( tCurTime );//发送心跳
		SaveData( tCurTime );//保存数据
		Sleep( 1000 );
	}
	return 0;
}

//连接到达响应
void NoDB::OnConnect(mdk::STNetHost &host)
{
	string ip;
	int port;
	host.GetServerAddress( ip, port );
	//连接到guide
	if ( ip == m_context.guideIP && port == m_context.guidePort )
	{
		ConnectGuide(host);
		return;
	}
	//连接到数据库
	if ( ip == m_context.databaseIP && port == m_context.databasePort )
	{
		ConnectDB(host);
		return;
	}
	//连接到主片
	if ( ip == m_context.masterIP && port == m_context.masterPort )
	{
		ConnectMaster(host);
		return;
	}
	//client连接进来
}

//连接断开响应
void NoDB::OnCloseConnect(mdk::STNetHost &host)
{
	if ( host.ID() == m_guideHost.ID() ) 
	{
		printf( "Guide断开\n" );
		m_bGuideConnected = false;
		m_bNotifyReady = false;
		return;
	}
	if ( host.ID() == m_databaseHost.ID() ) 
	{
		printf( "数据库断开\n" );
		return;
	}
	if ( host.ID() == m_masterHost.ID() ) 
	{
		printf( "主片断开\n" );
		return;
	}
	printf( "client断开\n" );
}

//数据到达响应
void NoDB::OnMsg(mdk::STNetHost &host)
{
	if ( OnGuideMsg( host ) ) return;
	OnClientMsg( host );
}

//接收Guide连接上的数据并处理，如果host不是Guide则什么都不做，返回false
bool NoDB::OnGuideMsg( mdk::STNetHost &host )
{
	if ( host.ID() != m_guideHost.ID() ) return false;
	//////////////////////////////////////////////////////////////////////////
	//接收BStruct协议格式的报文
	bsp::BStruct msg;
	if ( !host.Recv(m_msgBuffer, MSG_HEAD_SIZE, false) ) return true;
	unsigned int len = bsp::memtoi(&m_msgBuffer[1], sizeof(unsigned int));
	if ( len > MAX_BSTRUCT_SIZE ) 
	{
		host.Recv( m_msgBuffer, MSG_HEAD_SIZE );
		return true;
	}
	if ( !host.Recv(m_msgBuffer, len + MSG_HEAD_SIZE) ) return true;
	DataFormat::DataFormat df = (DataFormat::DataFormat)m_msgBuffer[0];
	if ( DataFormat::BStruct != df ) return true;//只接收BStruct格式的报文
	if ( !msg.Resolve(&m_msgBuffer[MSG_HEAD_SIZE],len) ) return true;//解析报文
	
	//////////////////////////////////////////////////////////////////////////
	//处理报文
	if ( !msg["msgid"].IsValid() || sizeof(unsigned short) != msg["msgid"].m_size ) return true;
	
	unsigned short msgid = msg["msgid"];
	bool bIsInvalidMsg = true;//默认为非法报文
	switch( msgid )
	{
	case msgid::gNodeCheck :
		OnNodeCheck( host, msg );
		break;
	case msgid::gSetDatabase :
		OnSetDatabase( host, msg );
		break;
	case msgid::gSetMaster :
		OnSetMaster( host, msg );
		break;
	default:
		break;
	}
	return true;
}

/*
 *	2 byte 报文长度
 *	2 byte key长度
 *	n byte key
 *	2 byte value长度
 *	n byte value
 *	4 byte hashvalue
 */
unsigned int g_insertCount = 0;
#ifdef WIN32
SYSTEMTIME t1,t2;
#else
struct timeval tEnd,tStart;
struct timezone tz;
#endif
//接收client连接上的数据并处理，如果host是Guide则什么都不做，返回false
bool NoDB::OnClientMsg( mdk::STNetHost &host )
{
	if ( host.ID() == m_guideHost.ID() ) return false;
	unsigned char msg[600];
	unsigned short len = 2;
	
	if ( !host.Recv( msg, len, false ) ) 
	{
		return true;
	}
	memcpy( &len, msg, sizeof(unsigned short) );
	if ( 2 >= len ) //非法报文
	{
		printf( "del msg\n" );
		host.Recv( msg, len );//删除报文
		return true;
	}
	len += 2;
	if ( !host.Recv( msg, len ) ) 
	{
		return true;
	}
	unsigned short keySize = 0;
	unsigned short valueSize = 0;
	memcpy( &keySize, &msg[2], sizeof(unsigned short) );
	if ( 2 + 2 + keySize > len ) 
	{
		printf( "del msg 2 + 2 + keySize > len\n" );
		return true;//非法报文
	}
	memcpy( &valueSize, &msg[2+2+keySize], sizeof(unsigned short) );
	if ( 2 + 2 + keySize + 2 + valueSize + 4 > len ) 
	{
		printf( "del msg 2 + 2 + keySize + 2 + valueSize + 4 > len\n" );
		return true;//非法报文
	}
	unsigned char *key = &msg[2+2];
	unsigned char *value = NULL;//new unsigned char[valueSize];
	//	memcpy( value, &msg[2+2+keySize+2], valueSize );
	value = &msg[2+2+keySize+2];
	unsigned int hashValue = 0;
	memcpy( &hashValue, &msg[2+2+keySize+2+valueSize], sizeof(unsigned int) );
	RHTable::OP_R *pR = m_nosqlDB.Insert(key, keySize, value, hashValue );
	g_insertCount++;
	if ( 1 == g_insertCount )
	{
#ifdef WIN32
		GetSystemTime(&t1);
#else
		gettimeofday (&tStart , &tz);
#endif
	}
	if ( g_insertCount >= 200000 )
	{
		g_insertCount = 0;
#ifdef WIN32
		GetSystemTime(&t2);
		int s = ((t2.wHour - t1.wHour)*3600 + (t2.wMinute - t1.wMinute) * 60 
			+ t2.wSecond - t1.wSecond) * 1000 + t2.wMilliseconds - t1.wMilliseconds;
		printf( "%f\n", s / 1000.0 );
#else
		gettimeofday (&tEnd , &tz);
		printf("time:%ld\n", (tEnd.tv_sec-tStart.tv_sec)*1000+(tEnd.tv_usec-tStart.tv_usec)/1000);
#endif
	}

	return true;
}

/*
	从硬盘读取数据
	单机Exist或分布式Exist的数据库结点，调用
*/
void NoDB::ReadData()
{
	if ( "true" == m_context.distributed && "database" != m_context.role ) return;
	printf( "开始加载数据\n" );
	Listen(m_context.port);//开始提供服务
	m_status = NodeStatus::Serving;
	SendReadyNotify();//发送就绪通知
	printf( "加载完成\n" );
	printf( "开始服务\n" );
	return;
}

/*
	下载数据
	分布式Exist的非数据库结点，调用
*/
void* NoDB::DownloadData(void* pParam)
{
	if ( "false" == m_context.distributed || "database" == m_context.role ) return 0;
	printf( "开始下载数据\n" );
	m_status = NodeStatus::LoadData;
	if ( "master" == m_context.role ) 
	{
		printf( "开始服务\n" );
		Listen(m_context.port);//开始提供服务
		m_status = NodeStatus::Serving;
		SendReadyNotify();//发送就绪通知
	}

	return 0;
}

//数据持久化
void NoDB::SaveData( time_t tCurTime )
{
	static time_t tLastSave = tCurTime;
	if ( tCurTime - tLastSave <= 60 ) return;
	tLastSave = tCurTime;
}

//心跳
void NoDB::Heartbeat( time_t tCurTime )
{
	static time_t tLastHeart = tCurTime;
	if ( !m_bGuideConnected ) return; //未建立连接，不发送心跳
	if ( tCurTime - tLastHeart <= 60 ) return; //发送间隔未到1分钟，不发送
	bsp::BStruct msg;
	mdk::AutoLock lock(&m_ioLock);
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::eHeartbeat;
	SendBStruct(m_guideHost, msg);
	tLastHeart = tCurTime;
}

//连接到向导服务
void NoDB::ConnectGuide(mdk::STNetHost &host)
{
	printf( "成功连接到向导\n" );
	m_guideHost = host;
	//发送结点加入通知
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::eNodeJoin;
	if ( "database" == m_context.role ) msg["role"] = (unsigned char) NodeRole::Database;
	else if ( "master" == m_context.role ) msg["role"] = (unsigned char) NodeRole::Master;
	else if ( "piece" == m_context.role ) msg["role"] = (unsigned char) NodeRole::Piece;
	msg["wan_ip"] = m_context.wanIP.c_str();
	msg["lan_ip"] = m_context.lanIP.c_str();
	msg["port"] = (unsigned short)m_context.port;
	msg["pieceNo"] = (unsigned int)m_context.pieceNo;
	SendBStruct(m_guideHost, msg);
	m_bGuideConnected = true;
	if ( "database" == m_context.role ) SendReadyNotify();//发送就绪通知
}

//结点校正响应
void NoDB::OnNodeCheck( mdk::STNetHost &host, bsp::BStruct &msg )
{
	unsigned char role = msg["role"];
	string reason;
	if ( NodeRole::Unknow == role ) //结点角色非法，设置原因
	{
		reason = (string)msg["reason"];
		host.Close();
		printf( "结点验证失败：%s\n", reason.c_str() );
		t_exit.Run(mdk::Executor::Bind(&NoDB::Exit), this, NULL);
		m_bCheckVaild = false;
		m_waitCheck.Notify();
		return;
	}
	if ( NodeRole::Master == role ) m_context.role = "master";
	m_context.pieceNo = msg["pieceNo"];	//结点角色为分片还是主片，都要修正片号
	m_bCheckVaild = true;
	m_waitCheck.Notify();
	m_waitMain.Wait();//等待主线程激活
}

//设置数据库
void NoDB::OnSetDatabase( mdk::STNetHost &host, bsp::BStruct &msg )
{
	string wanIP = msg["wan_ip"];
	string lanIP = msg["lan_ip"];
	short port = msg["port"];
	printf( "测试数据库网络线路：内网%s,外网%s,端口%d\n",
		lanIP.c_str(), wanIP.c_str(), port );	
	//优先走内网连接
	mdk::Socket ipcheck;
	if ( !ipcheck.Init( mdk::Socket::tcp ) ) 
	{
		printf( "Error: NoDB::OnSetDatabase 无法创建测试对象\n" );
		return;
	}
	if ( ipcheck.Connect( lanIP.c_str(), port) ) 
	{
		ipcheck.Close();
		printf( "走内网线路，连接数据库\n" );	
		m_context.databaseIP = lanIP;
		m_context.databasePort = port;
		Connect( m_context.databaseIP.c_str(), m_context.databasePort );
		return;
	}
	//测试外网连通性
	ipcheck.Close();
	if ( !ipcheck.Init( mdk::Socket::tcp ) ) 
	{
		printf( "Error: NoDB::OnSetDatabase 无法创建测试对象\n" );
		return;
	}
	if ( !ipcheck.Connect(wanIP.c_str(), port) ) 
	{
		ipcheck.Close();
		printf( "无法连接数据库：内网IP%s,外网IP%s,端口%d\n", 
			lanIP.c_str(), wanIP.c_str(), port );
		return;
	}
	ipcheck.Close();
	printf( "走外网线路，连接数据库\n" );	
	m_context.databaseIP = wanIP;
	m_context.databasePort = port;
	Connect( m_context.databaseIP.c_str(), m_context.databasePort );
}

//设置主片
void NoDB::OnSetMaster( mdk::STNetHost &host, bsp::BStruct &msg )
{
	string wanIP = msg["wan_ip"];
	string lanIP = msg["lan_ip"];
	short port = msg["port"];
}

//连接到数据库
void NoDB::ConnectDB(mdk::STNetHost &host)
{
	printf( "成功连接到数据库\n" );
	m_databaseHost = host;
	t_loaddata.Run( mdk::Executor::Bind(&NoDB::DownloadData), this, 0 );//从数据库/主片上读
}

//连接到主片
void NoDB::ConnectMaster(mdk::STNetHost &host)
{
	printf( "成功连接到主片\n" );
	m_masterHost = host;
}

//发送就绪通知
void NoDB::SendReadyNotify()
{
	if ( NodeStatus::Serving != m_status ) return;
	mdk::AutoLock lock( &m_ioLock );
	if ( -1 == m_guideHost.ID() ) return;
	if ( m_bNotifyReady ) return;
	bsp::BStruct msg;
	m_bNotifyReady = true;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::eNodeReady;
	SendBStruct(m_guideHost, msg);
}

