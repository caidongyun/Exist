// Worker.cpp: implementation of the Worker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//数据库加入到网络中
void TCPWorker::DBJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	//校正结点信息
	if ( NodeStatus::NotOnline != m_db.status ) //数据库已在线，进行校正，不交给下一个业务流程处理，防止破坏数据库
	{
		NodeCheck(host, NodeRole::Unknow, 0, "数据库已在线");
		string wanIP = (string)msg["wan_ip"];
		unsigned short wanPort = msg["wan_port"];
		string lanIP = (string)msg["lan_ip"];
		unsigned short lanPort = msg["lan_port"];
		m_log.Run( "Waring:refuse Database join:Database already in net, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			host.ID(), wanIP.c_str(), wanPort, lanIP.c_str(), lanPort );
		return;
	}
	NodeCheck(host, NodeRole::Database, 0, NULL);
	
	/*
		不检查ip是否与之前的匹配，因为ip分配策略是有可能变化的
		由用户来保证加入的数据库是同一台机器
	 */
	m_db.wanIP = (string)msg["wan_ip"];
	m_db.wanPort = msg["wan_port"];
	m_db.lanIP = (string)msg["lan_ip"];
	m_db.lanPort = msg["lan_port"];
	m_db.status = NodeStatus::LoadData;
	m_log.Run( "Database join, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.wanPort, m_db.lanIP.c_str(), m_db.lanPort );
	return;
}

//主分片加入到网络中
void TCPWorker::MasterJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	NodeCheck(host, NodeRole::Master, 0, NULL);//校正结点信息
	m_master.wanIP = (string)msg["wan_ip"];
	m_master.wanPort = msg["wan_port"];
	m_master.lanIP = (string)msg["lan_ip"];
	m_master.lanPort = msg["lan_port"];
	m_master.status = NodeStatus::WaitDBReady;
	if ( NodeStatus::Serving == m_db.status ) 
	{
		m_master.status = NodeStatus::LoadData;
		SetDatabase(host);
	}
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) SetMaster(it->second->host);

	m_log.Run( "Master join, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.wanPort, m_db.lanIP.c_str(), m_db.lanPort );
	
	return;
}

//分片加入到网络中
void TCPWorker::PieceJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	//新建一个Exist分片
	Exist *pNewExist = new Exist;
	if ( NULL == pNewExist ) //返回
	{
		NodeCheck(host, NodeRole::Unknow, 0, "Guide内存不足");
		string wanIP = (string)msg["wan_ip"];
		unsigned short wanPort = msg["wan_port"];
		string lanIP = (string)msg["lan_ip"];
		unsigned short lanPort = msg["lan_port"];
		m_log.Run( "Waring:refuse Exist join:no enough memory, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			host.ID(), wanIP.c_str(), wanPort, lanIP.c_str(), lanPort );
		return;
	}
	pNewExist->host = host;
	pNewExist->wanIP = (string)msg["wan_ip"];
	pNewExist->wanPort = msg["wan_port"];
	pNewExist->lanIP = (string)msg["lan_ip"];
	pNewExist->lanPort = msg["lan_port"];
	pNewExist->role = NodeRole::Piece;
	pNewExist->status = NodeStatus::Idle;
	pNewExist->pieceNo = msg["pieceNo"];
	//检查片号冲突，进行校正
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		if ( pNewExist->pieceNo == it->second->pieceNo )
		{
			pNewExist->pieceNo = -1;
			break;
		}
	}
	NodeCheck(host, NodeRole::Piece, pNewExist->pieceNo, NULL);
	/*
		mdk保证了OnCloseConnect()完成前，socket句柄绝对不会被新连接使用，
		所以在OnCloseConnect()中删除socket句柄
		这里绝对不会出现冲突失败
	 */
	m_nodes.insert(NodeList::value_type(host.ID(), pNewExist));
	if ( NodeStatus::NotOnline != m_db.status && NodeStatus::Closing != m_db.status ) SetDatabase(host);//设置数据库
	if ( NodeStatus::NotOnline != m_master.status && NodeStatus::Closing != m_master.status ) SetMaster(host);//设置主分片
	m_log.Run( "Exist join, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.wanPort, m_db.lanIP.c_str(), m_db.lanPort);

	if ( 0 <= pNewExist->pieceNo && NodeStatus::Serving == m_db.status ) 
	{
		StartPiece(pNewExist->pieceNo, pNewExist->host.ID());//开始分片
	}
	
	return;
}

//数据库就绪
void TCPWorker::DatabaseReady()
{
	m_db.status = NodeStatus::Serving;
	if ( NodeStatus::WaitDBReady == m_master.status ) m_master.status = NodeStatus::LoadData;
	SetDatabase(m_master.host);
	
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		pExist = it->second;
		if ( pExist->pieceNo < 0 || NodeStatus::Idle != pExist->status ) continue;
		StartPiece(pExist->pieceNo, pExist->host.ID());//开始分片
	}
	
	return;
}

//数据库退出，所有结点停止服务
void TCPWorker::DBExit()
{
	m_db.status = NodeStatus::NotOnline;
	if ( NodeStatus::NotOnline != m_master.status ) m_master.status = NodeStatus::WaitDBReady;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) it->second->status = NodeStatus::Idle;
}

//增加一个分片，成功返回NULL,失败返回原因
char* TCPWorker::StartPiece(unsigned int pieceNo, unsigned int hostID)
{
	if ( NodeStatus::Serving == m_db.status ) return "数据库未就绪";
	
	Exist *pExist = NULL;
	if ( hostID >= 0 )
	{
		NodeList::iterator it = m_nodes.find( hostID );
		if ( it == m_nodes.end() ) return "指定结点不存在";
		pExist = it->second;
		if ( NodeStatus::Idle != pExist->status ) return "指定结点非空闲";
	}
	else
	{
		NodeList::iterator it = m_nodes.begin();
		for ( ; it != m_nodes.end(); it++ )
		{
			if ( NodeStatus::Idle != pExist->status ) continue;
			pExist = it->second;
			//不检查结点预先分配的片号是否与当前指定片号相同，直接覆盖，以人工为准
			break;
		}
	}
	if ( NULL == pExist ) return "无空闲结点，请增加结点到网络中";
	pExist->pieceNo = pieceNo;
	pExist->status = NodeStatus::LoadData;
	NodeInit(pExist);
	PieceList::iterator it = m_pieces.find(pieceNo);
	if ( it != m_pieces.end() )
	{
		Exist* pNotOnlineExist = it->second;
		m_pieces.erase(it);
		delete pNotOnlineExist;
	}
	m_pieces.insert(PieceList::value_type(pExist->pieceNo, pExist));//加入到分片列表
	m_log.Run( "Start Piece, Node info:pieceNo %d; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		pExist->pieceNo, 
		pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
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
				m_log.Run( "Delete Database, Node info:hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
			}
			else if (&m_master == pExist) 
			{
				m_log.Run( "Delete Master, Node info:hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
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
		m_log.Run( "Delete Node, Node info:pieceNo %d; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
		delete pExist;
		return NULL;
	}
	//停止服务
	if ( NodeStatus::Closing == pExist->status ) return "正在停止结点上的服务，结点将在其服务停止后自动删除";
	pExist->status = NodeStatus::Closing;
	NodeExit(pExist);//通知结点退出

	if ( NodeRole::Database == pExist->role ) 
	{
		m_log.Run( "Node stop service, Node info:Database; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
	}
	else if ( NodeRole::Master == pExist->role ) 
	{
		m_log.Run( "Node stop service, Node info:Master; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
	}
	else 
	{
		m_log.Run( "Node stop service, Node info:pieceNo %d; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
	}
	return "准备停止结点上的服务，结点将在其服务停止后自动删除";
}

