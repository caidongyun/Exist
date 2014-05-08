// Console.cpp: implementation of the Console class.
//
//////////////////////////////////////////////////////////////////////

#include "Console.h"
#include <cstring>
using namespace std;

void split( vector<string> &des, const char *str,char *spliter )
{
	des.clear();
	string temp = str;
	char *item;
	item = strtok( (char*)temp.data(), spliter );  
	while( item != NULL )  
	{    
		des.push_back(item);
		item = strtok( NULL, spliter );  
	}
	temp = "";
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Console::Console(char *prompt, int cmdMaxSize)
{
	m_bRun = false;
	m_printInfo = true;
	if ( NULL == prompt ) m_printInfo = false;
	m_cmdMaxSize = (256>cmdMaxSize?256:cmdMaxSize);
	m_prompt = prompt;
}

Console::~Console()
{

}

bool Console::Start( mdk::MethodPointer method, void *pObj )
{
	m_bRun = true;

	m_isMemberMethod = true;
	m_method = method;
	m_pObj = pObj;
	m_fun = NULL;
	return m_inputThread.Run(mdk::Executor::Bind(&Console::InputThread), this, NULL);
}

bool Console::Start( mdk::FuntionPointer fun )
{
	m_bRun = true;
	m_isMemberMethod = false;
	m_method = 0;
	m_pObj = NULL;
	m_fun = fun;
	return m_inputThread.Run(mdk::Executor::Bind(&Console::InputThread), this, NULL);
}

void Console::Stop()
{
	m_bRun = false;
	m_inputThread.Stop( 3000 );
}

void Console::WaitStop()
{
	m_inputThread.WaitStop();
}

void* Console::InputThread(void*)
{
	char *cmd = new char[m_cmdMaxSize];
	if ( NULL == cmd ) return NULL;
	//接受用户指令输入
	PrintBuf();
	int cmdSize = 0;
	bool valid = true;
	char *ret = NULL;
	while( m_bRun )
	{
		valid = true;
		if ( m_printInfo ) 	printf( "%s", m_prompt.c_str() );
		while ( fgets(cmd, m_cmdMaxSize, stdin) )
		{
			cmdSize = strlen( cmd );
			if ( '\n' == cmd[cmdSize-1] ) break;
			valid = false;
		}
		if ( !m_bRun ) return NULL; 
		if ( !valid ) 
		{
			Print("Invalid command");
			continue;
		}
		Print( ExecuteCommand(cmd) );
	}

	return NULL;
}

char* Console::ExecuteCommand(char *cmd)
{
	split( m_cmd, cmd, " \t\n" );
	if ( 0 >= m_cmd.size() ) return "";
	if ( m_isMemberMethod ) return (char*)mdk::Executor::CallMethod(m_method, m_pObj, &m_cmd);
	else return (char*)m_fun(&m_cmd);
}

void Console::PrintBuf()
{
	if ( !m_printInfo ) return;
	int i = 0;
	mdk::AutoLock lock(&m_printLock);
	int count = m_waitPrint.size();
	for ( i = 0; i < count; i++ ) printf( "%s\n", m_waitPrint[i].c_str() );
	fflush(stdout);
}

void Console::Print( const char* info )
{
	if ( !m_printInfo ) return;
	mdk::AutoLock lock(&m_printLock);
	if ( NULL != info && '\0' != info[0] )
	{
		if ( !m_bRun ) m_waitPrint.push_back(info);
		else 
		{
			printf( "%s\n", info );
			fflush(stdout);
		}
	}
}

void Console::Line()
{
	Print("_______________________________________________________________________________");
	fflush(stdout);
}
