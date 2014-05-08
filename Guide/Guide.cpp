#include <cstdio>
#include "TCPWorker.h"

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
	if ( 1 >= argc )
	{
		printf( "please set config file start like this [status status.cfg]\n" );
		return 0;
	}
	TCPWorker ser(argv[1]);
	const char *ret = ser.Start();
	if ( NULL != ret )
	{
		ser.GetLog().Error( "Guide Start error:%s", ret );
		return 0;
	}
	ser.GetLog().Run( "Guide Start" );
	ser.WaitStop();
	return 0;
}
