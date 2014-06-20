// TCPWorker.cpp: implementation of the TCPWorker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TCPWorker::TCPWorker(char *cfgFile)
:m_cfgFile( cfgFile )
{
	m_msgBuffer = new unsigned char[MAX_MSG_SIZE];
	m_count = 0;
	if ( NULL == m_msgBuffer ) return;
	m_db.role = NodeRole::Database;
	m_db.status = NodeStatus::NotOnline;
	m_master.role = NodeRole::Master;
	m_master.status = NodeStatus::NotOnline;
	ReadConfig();
	SetAverageConnectCount(1000);
	SetHeartTime( m_heartTime );
	Listen(m_port);
}

TCPWorker::~TCPWorker()
{
	if ( NULL != m_msgBuffer )
	{
		delete[]m_msgBuffer;
		m_msgBuffer = NULL;
	}
}

bool TCPWorker::ReadConfig()
{
	m_port = m_cfgFile[SECTION_KEY]["port"];
	m_heartTime = m_cfgFile[SECTION_KEY]["heartTime"];
	m_pieceSize = m_cfgFile[SECTION_KEY]["pieceSize"];
	
	if (  120 > m_heartTime ) m_heartTime = 120;
	if ( 0 >= m_port ) m_port = 7250;
	if ( 0 >= m_pieceSize ) m_pieceSize = 5000000;
	return true;
}

mdk::Logger& TCPWorker::GetLog()
{
	return m_log;
}

void TCPWorker::OnCloseConnect(mdk::STNetHost &host)
{
	if ( m_db.host.ID() == host.ID() )//数据库吊线
	{
		DBExit();
		return;
	}
	else if ( m_master.host.ID() == host.ID() )//主分片吊线
	{
		printf( "主片断开\n" );
		m_master.status = NodeStatus::NotOnline;
		return;
	}
	
	//结点吊线
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.find( host.ID() );
	if ( it != m_nodes.end() ) 
	{
		pExist = it->second;
		m_nodes.erase(it);
		//检查吊线结点是否是一个分片
		PieceList::iterator itPiece = m_pieces.find( pExist->pieceNo );
		if ( itPiece != m_pieces.end() && pExist == itPiece->second ) 
		{
			printf( "分片%d断开\n", pExist->pieceNo );
			pExist->status = NodeStatus::NotOnline;
			return;
		}
		//吊线结点是一个空闲结点直接释放
		printf( "空闲分片断开\n" );
		delete pExist;
	}

	//人工控制台连接断开
	return;
}

//非法报文
void TCPWorker::InvalidMsg(mdk::STNetHost &host, unsigned char *msg, unsigned short len)
{
	m_log.Info("info:", "Waring: msg format is invalid from Host(%d)", host.ID() );
	m_log.Info("info:", "%s-%d", msg, len);
	host.Close();
}

/**
 * 数据到达，回调方法
 * 
 * 派生类实现具体断开连接业务处理
 * 
*/
void TCPWorker::OnMsg(mdk::STNetHost &host)
{
	//////////////////////////////////////////////////////////////////////////
	//接收BStruct协议格式的报文
	bsp::BStruct msg;
	if ( !host.Recv(m_msgBuffer, MSG_HEAD_SIZE, false) ) return;
	unsigned int len = bsp::memtoi(&m_msgBuffer[1], sizeof(unsigned int));
	if ( len > MAX_BSTRUCT_SIZE ) 
	{
		host.Recv( m_msgBuffer, MSG_HEAD_SIZE );
		InvalidMsg(host, m_msgBuffer, MSG_HEAD_SIZE);
		return;
	}
	if ( !host.Recv(m_msgBuffer, len + MSG_HEAD_SIZE) ) return;
	DataFormat::DataFormat df = (DataFormat::DataFormat)m_msgBuffer[0];
	if ( DataFormat::BStruct != df )//只接收BStruct格式的报文
	{
		InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
		return;
	}
	if ( !msg.Resolve(&m_msgBuffer[MSG_HEAD_SIZE],len) ) 
	{
		InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
		return;//解析报文
	}

	//////////////////////////////////////////////////////////////////////////
	//处理报文
	if ( !msg["msgid"].IsValid() || sizeof(unsigned short) != msg["msgid"].m_size ) //检查有无msgid字段
	{
		InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
		return;
	}
	unsigned short msgid = msg["msgid"];
	bool bIsValidMsg = false;//默认为非法报文
	switch( msgid )
	{
	case msgid::eNodeJoin ://结点加入
		bIsValidMsg = OnNodeJoin(host, msg);
		break;
	case msgid::eNodeReady://结点就绪
		bIsValidMsg = OnNodeReady(host);
		break;
	case msgid::eNodeExit://结点退出
		bIsValidMsg = OnNodeExit(host);
		break;
	case msgid::uCommand://人工控制指令
		bIsValidMsg = OnUserCommand(host, msg);
		break;
	case msgid::eHeartbeat://心跳
		bIsValidMsg = true;
		break;
	default:
		break;
	}
	if ( !bIsValidMsg ) InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
}

//结点加入分布网络
bool TCPWorker::OnNodeJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	//////////////////////////////////////////////////////////////////////////
	/*
		检查报文合法性
		1.该有的字段有没有
		2.长度是否与协议约定相等
		3.字符串等变长字段长度必须不小于1
	*/
	if ( !msg["port"].IsValid() || sizeof(unsigned short) != msg["port"].m_size ) return false;
	if ( !msg["wan_ip"].IsValid() || 1 > msg["wan_ip"].m_size ) return false;
	if ( !msg["lan_ip"].IsValid() || 1 > msg["lan_ip"].m_size ) return false;
	if ( !msg["role"].IsValid() || sizeof(unsigned char) != msg["role"].m_size ) return false;
	if ( !msg["pieceNo"].IsValid() || sizeof(unsigned int) != msg["pieceNo"].m_size ) return false;
	
	//////////////////////////////////////////////////////////////////////////
	//业务处理
	unsigned char role = msg["role"];
	if ( NodeRole::Database == role ) //数据库加入
	{
		DBJoin(host, msg);
		return true;
	}
	if ( NodeStatus::NotOnline == m_master.status )//只要主分片没加入，第一个结点作为主分片
	{
		MasterJoin(host, msg);
		return true;
	}
	PieceJoin(host, msg);//作为分片加入

	return true;
}

//结点就绪
bool TCPWorker::OnNodeReady(mdk::STNetHost &host)
{
	if ( m_db.host.ID() == host.ID() ) 
	{
		DatabaseReady();
		return true;
	}
	if ( m_master.host.ID() == host.ID() ) 
	{
		printf( "主片就绪\n" );
		m_master.status = NodeStatus::Serving;
	}
	else
	{
		NodeList::iterator it = m_nodes.find( host.ID() );
		if ( it == m_nodes.end() ) return false;
		printf( "分片%d就绪\n", it->second->pieceNo );
		it->second->status = NodeStatus::Serving;
	}
	return true;
}

//响应结点退出请求
bool TCPWorker::OnNodeExit(mdk::STNetHost &host)
{
	Exist *pExist = NULL;
	if ( m_db.host.ID() == host.ID() ) //数据库退出
	{
		pExist = &m_db;
		DBExit();
	}
	else if ( m_master.host.ID() == host.ID() ) //主分片退出
	{
		pExist = &m_master;
		pExist->status = NodeStatus::NotOnline;
	}
	else//结点退出
	{
		NodeList::iterator it = m_nodes.find( host.ID() );
		if ( it == m_nodes.end() ) return false;
		pExist = it->second;
		pExist->status = NodeStatus::Idle;
	}
	DelNode(pExist);
	return true;
}

//响应人工控制指令
bool TCPWorker::OnUserCommand(mdk::STNetHost &host, bsp::BStruct &msg)
{
	if ( !msg["cmdCode"].IsValid() || sizeof(unsigned char) != msg["cmdCode"].m_size ) return false;
	unsigned char cmdCode = msg["cmdCode"];
	bool bIsInvalidMsg = true;//默认为非法报文
	switch( cmdCode )
	{
	case UserCmd::QueryNodeList :
		bIsInvalidMsg = OnQueryNodeList(host);
		break;
	case UserCmd::StartPiece :
		bIsInvalidMsg = OnStartPiece(host, msg);
		break;
	case UserCmd::NodeDel :
		bIsInvalidMsg = OnNodeDel(host, msg);
		break;
	default:
		break;
	}
	return bIsInvalidMsg;
}

//查询所有结点
bool TCPWorker::OnQueryNodeList(mdk::STNetHost &host)
{
	SendSystemStatus(host);
	
	return true;
}

//开始分片指令
bool TCPWorker::OnStartPiece(mdk::STNetHost &host, bsp::BStruct &msg)
{
	if ( !msg["pieceNo"].IsValid() || sizeof(unsigned int) != msg["pieceNo"].m_size ) return false;
	unsigned int pieceNo = msg["pieceNo"];
	char *errReason = StartPiece(pieceNo);
	ReplyUserResult(host, msg, errReason);
	return true;
}

//删除结点指令
bool TCPWorker::OnNodeDel(mdk::STNetHost &host, bsp::BStruct &msg)
{
	if ( !msg["hostID"].IsValid() || sizeof(int) != msg["hostID"].m_size ) return false;
	int hostID = msg["hostID"];
	NodeList::iterator it = m_nodes.find( host.ID() );
	char *errReason = NULL;
	if ( it != m_nodes.end() ) errReason = DelNode(it->second);
	else if ( m_db.host.ID() == host.ID() )  errReason = DelNode(&m_db);
	else if (m_master.host.ID() == host.ID())errReason = DelNode(&m_master);
	ReplyUserResult(host, msg, errReason);
	return true;
}
