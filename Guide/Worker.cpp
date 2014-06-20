// Worker.cpp: implementation of the Worker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_LEN 2000

//数据库加入到网络中
void TCPWorker::DBJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	printf( "数据库请求登录\n" );
	//校正结点信息
	if ( NodeStatus::NotOnline != m_db.status ) //数据库已在线，进行校正，不交给下一个业务流程处理，防止破坏数据库
	{
		printf( "拒绝：数据库已在线\n" );
		NodeCheck(host, NodeRole::Unknow, 0, "数据库已在线");
		unsigned short port = msg["port"];
		string wanIP = (string)msg["wan_ip"];
		string lanIP = (string)msg["lan_ip"];
		m_log.Info("info:", "Waring:refuse Database join:Database already in net, host info:id %d; wan ip%s; lan ip%s; port%d", 
			host.ID(), wanIP.c_str(), lanIP.c_str(), port );
		return;
	}
	printf( "数据库登录\n" );
	NodeCheck(host, NodeRole::Database, 0, NULL);
	
	/*
		不检查ip是否与之前的匹配，因为ip分配策略是有可能变化的
		由用户来保证加入的数据库是同一台机器
	 */
	m_db.host = host;
	m_db.port = msg["port"];
	m_db.wanIP = (string)msg["wan_ip"];
	m_db.lanIP = (string)msg["lan_ip"];
	m_db.status = NodeStatus::LoadData;
	m_log.Info("info:", "Database join, host info:id %d; wan ip%s; lan ip%s; port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.lanIP.c_str(), m_db.port );
	return;
}

//主分片加入到网络中
void TCPWorker::MasterJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	printf( "主片登录\n" );
	NodeCheck(host, NodeRole::Master, -1, NULL);//校正结点信息
	m_master.host = host;
	m_master.wanIP = (string)msg["wan_ip"];
	m_master.lanIP = (string)msg["lan_ip"];
	m_master.port = msg["port"];
	m_master.status = NodeStatus::WaitDBReady;
	if ( NodeStatus::Serving == m_db.status ) 
	{
		SetDatabase(host);
		m_master.status = NodeStatus::LoadData;
	}
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) SetMaster(it->second->host);

	m_log.Info("info:","Master join, host info:id %d; wan ip%s; lan ip%s; port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.lanIP.c_str(), m_db.port );
	
	return;
}

//分片加入到网络中
void TCPWorker::PieceJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	printf( "分片请求登录\n" );
	//新建一个Exist分片
	Exist *pNewExist = new Exist;
	if ( NULL == pNewExist ) //返回
	{
		printf( "拒绝：Guide内存不足\n" );
		NodeCheck(host, NodeRole::Unknow, 0, "Guide内存不足");
		string wanIP = (string)msg["wan_ip"];
		string lanIP = (string)msg["lan_ip"];
		unsigned short port = msg["port"];
		m_log.Info("info:","Waring:refuse Exist join:no enough memory, host info:id %d; wan ip%s; lan ip%s; port%d", 
			host.ID(), wanIP.c_str(), lanIP.c_str(), port );
		return;
	}
	pNewExist->host = host;
	pNewExist->wanIP = (string)msg["wan_ip"];
	pNewExist->lanIP = (string)msg["lan_ip"];
	pNewExist->port = msg["port"];
	pNewExist->role = NodeRole::Piece;
	pNewExist->status = NodeStatus::Idle;
	pNewExist->pieceNo = msg["pieceNo"];
	//检查片号冲突，进行校正
	NodeList::iterator it = m_nodes.begin();
	for ( ; 0 <= pNewExist->pieceNo && it != m_nodes.end(); it++ )
	{
		if ( pNewExist->pieceNo == it->second->pieceNo )
		{
			pNewExist->pieceNo = -1;
			break;
		}
	}
	printf( "接受请求，分片号%d\n", pNewExist->pieceNo );
	NodeCheck(host, NodeRole::Piece, pNewExist->pieceNo, NULL);
	/*
		mdk保证了OnCloseConnect()完成前，socket句柄绝对不会被新连接使用，
		所以在OnCloseConnect()中删除socket句柄
		这里绝对不会出现冲突失败
	 */
	m_nodes.insert(NodeList::value_type(host.ID(), pNewExist));
	if ( NodeStatus::NotOnline != m_db.status && NodeStatus::Closing != m_db.status ) SetDatabase(host);//设置数据库
	if ( NodeStatus::NotOnline != m_master.status && NodeStatus::Closing != m_master.status ) SetMaster(host);//设置主分片
	m_log.Info("info:", "Exist join, host info:id %d; wan ip%s; lan ip%s; port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.lanIP.c_str(), m_db.port);

	if ( 0 <= pNewExist->pieceNo && NodeStatus::Serving == m_db.status ) 
	{
		StartPiece(pNewExist->pieceNo);//开始分片
	}
	
	return;
}

//返回一个新片号
int TCPWorker::NewPieceNo()
{
	int pieceNo = 0;
	NodeList::iterator it;
	bool noIsExist = false;
	unsigned int maxPieceNo = CalPieceNo(0xffffffff, m_pieceSize);
	for ( pieceNo = 0; pieceNo <= maxPieceNo; pieceNo++ )
	{
		noIsExist = false;
		it = m_nodes.begin();
		for ( ; it != m_nodes.end(); it++ )
		{
			if ( pieceNo == it->second->pieceNo )
			{
				noIsExist = true;
				break;
			}
		}
		if ( !noIsExist ) break;
	}
	if (noIsExist) return -1;
	return pieceNo;
}

//数据库就绪
void TCPWorker::DatabaseReady()
{
	printf( "数据库就绪\n" );
	m_db.status = NodeStatus::Serving;
	if ( NodeStatus::WaitDBReady == m_master.status ) 
	{
		SetDatabase(m_master.host);
		m_master.status = NodeStatus::LoadData;
	}
	
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		pExist = it->second;
		if ( pExist->pieceNo < 0 || NodeStatus::Idle != pExist->status ) continue;
		StartPiece(pExist->pieceNo);//开始分片
	}
	
	return;
}

//数据库退出，所有结点停止服务
void TCPWorker::DBExit()
{
	printf( "数据库断开\n" );
	m_db.status = NodeStatus::NotOnline;
	if ( NodeStatus::NotOnline != m_master.status ) m_master.status = NodeStatus::WaitDBReady;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) it->second->status = NodeStatus::Idle;
}

//增加一个分片，成功返回NULL,失败返回原因
char* TCPWorker::StartPiece(unsigned int pieceNo)
{
	if ( NodeStatus::Serving == m_db.status ) 
	{
		printf( "拒绝分片指令：数据库未就绪\n" );
		return "数据库未就绪";
	}
	
	Exist *pExist = NULL;
	Exist *pDefPiece = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		if ( NodeStatus::Idle != pExist->status ) 
		{
			if ( pieceNo == it->second->pieceNo ) 
			{
				printf( "拒绝分片指令：分片%d已工作\n", it->second->pieceNo );
				return "分片已工作";
			}
			continue;
		}
		if ( 0 <= pieceNo )//指定了片号
		{
			if ( pieceNo == it->second->pieceNo )
			{
				if ( NULL == pDefPiece ) pDefPiece = it->second;//第一个未启动分片设置为默认
				pExist = it->second;
				break;
			}
			if ( NULL == pExist && 0 > it->second->pieceNo ) 
				pExist = it->second;//使用一个空分片
		}
		else//没指定片,随便启动一个分片
		{
			if ( it->second->pieceNo >= 0 ) 
			{
				pExist = it->second;
				break;
			}
			if ( NULL == pExist ) pExist = it->second;//使用一个空分片
		}
	}

	if ( NULL == pExist ) 
	{
		if ( NULL == pDefPiece ) 
		{
			printf( "拒绝分片指令：无空闲结点，请增加结点到网络中\n" );
			return "无空闲结点，请增加结点到网络中";
		}
		else 
		{
			pExist = pDefPiece;
			pExist->pieceNo = pieceNo;
		}
	}
	
	if ( 0 > pExist->pieceNo ) 
	{
		pExist->pieceNo = NewPieceNo();
		if ( 0 > pExist->pieceNo ) 
		{
			printf( "拒绝分片指令：分片已满，请删除一些无用结点\n" );
			return "分片已满，请删除一些无用结点";
		}
	}
	
	pExist->status = NodeStatus::LoadData;
	NodeInit(pExist);
	it = m_pieces.find(pieceNo);
	if ( it != m_pieces.end() )
	{
		Exist* pNotOnlineExist = it->second;
		m_pieces.erase(it);
		delete pNotOnlineExist;
	}
	m_pieces.insert(PieceList::value_type(pExist->pieceNo, pExist));//加入到分片列表
	m_log.Info("info:", "Start Piece, Node info:pieceNo %d; hostid %d; wan ip%s; lan ip%s; port%d", 
		pExist->pieceNo, 
		pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	return NULL;
}

//删除一个结点，成功返回NULL，失败返回原因
char* TCPWorker::DelNode(Exist *pExist)
{
	if ( &m_db == pExist || &m_master == pExist )
	{
		if ( NodeStatus::NotOnline == pExist->status ) 
		{
			if (&m_db == pExist) 
			{
				m_log.Info("info:","Delete Database, Node info:hostid %d; wan ip%s; lan ip%s; port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
			}
			else if (&m_master == pExist) 
			{
				m_log.Info("info:", "Delete Master, Node info:hostid %d; wan ip%s; lan ip%s; port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
			}
			pExist->host.Close();
			return NULL;
		}
	}
	else if ( NodeStatus::Idle == pExist->status ) 
	{
		m_nodes.erase(pExist->host.ID());
		m_pieces.erase(pExist->host.ID());
		pExist->host.Close();
		m_log.Info("info:", "Delete Node, Node info:pieceNo %d; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
		delete pExist;
		return NULL;
	}
	//停止服务
	if ( NodeStatus::Closing == pExist->status ) return "正在停止结点上的服务，结点将在其服务停止后自动删除";
	pExist->status = NodeStatus::Closing;
	NodeExit(pExist);//通知结点退出

	if ( NodeRole::Database == pExist->role ) 
	{
		m_log.Info("info:", "Node stop service, Node info:Database; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	}
	else if ( NodeRole::Master == pExist->role ) 
	{
		m_log.Info("info:", "Node stop service, Node info:Master; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	}
	else 
	{
		m_log.Info("info:", "Node stop service, Node info:pieceNo %d; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	}
	return "准备停止结点上的服务，结点将在其服务停止后自动删除";
}
