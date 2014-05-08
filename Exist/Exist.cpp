#include <cstdio>
#include "NoDB.h"

#ifdef WIN32
#ifdef _DEBUG
#pragma comment ( lib, "../Micro-Development-Kit/lib/mdk_d.lib" )
#pragma comment ( lib, "../bstruct/lib/bstruct_d.lib" )
#else
#pragma comment ( lib, "../Micro-Development-Kit/lib/mdk.lib" )
#pragma comment ( lib, "../bstruct/lib/bstruct.lib" )
#endif
#endif

//external in stored
int main( int argc, char** argv )
{
	if ( 1 >= argc )
	{
		printf( "please set config file start like this [exist exist.cfg]\n" );
		return 0;
	}
	NoDB db( argv[1] );
	printf( "ExistÆô¶¯\n" );
	db.Start();
	db.WaitStop();
	printf( "ExistÍË³ö\n" );
	return 0;
}
