#include <cstdio>
#include "TCPWorker.h"
#include "../common/Console.h"

#ifdef WIN32
#ifdef _DEBUG
#pragma comment ( lib, "../Micro-Development-Kit/lib/mdk_d.lib" )
#pragma comment ( lib, "../bstruct/lib/bstruct_d.lib" )
#else
#pragma comment ( lib, "../Micro-Development-Kit/lib/mdk.lib" )
#pragma comment ( lib, "../bstruct/lib/bstruct.lib" )
#endif
#endif

//×´Ì¬·þÎñÆ÷
int main( int argc, char** argv )
{
	if ( 2 >= argc )
	{
		printf( "please set connect adder like this [mgr 127.0.0.1 8888]\n" );
		return 0;
	}
	
	Console console(COMMAND_TITLE, 256);
	TCPWorker ser(argv[1], atoi(argv[2]),&console);
	const char *ret = ser.Start();
	if ( NULL != ret )
	{
		printf( "Start error:%s", ret );
		return 0;
	}
	if ( !ser.WaitConnect( 3000 ) ) 
	{
		printf( "connect timeout:%s:%s\n", argv[1], argv[2] );
		return 0;
	}
	console.Start(mdk::Executor::Bind(&TCPWorker::OnCommand), &ser);
	ser.WaitStop();
	console.Stop();
	
	return 0;
}
