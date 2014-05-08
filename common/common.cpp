
#include "common.h"

//×´Ì¬ÃèÊö
char* DesStatus(unsigned char status)
{
	if ( NodeStatus::Idle == status) return "Idle";
	else if ( NodeStatus::LoadData == status) return "LoadData";
	else if ( NodeStatus::Serving == status) return "Serving";
	else if ( NodeStatus::Closing == status) return "Closing";
	else if ( NodeStatus::WaitDBReady == status) return "WaitDBReady";
	else if ( NodeStatus::NotOnline == status) return "NotOnline";
	return "unknow";
}

//½ÇÉ«ÃèÊö
char* DesRole(unsigned char status)
{
	if ( NodeRole::Unknow == status) return "Unknow";
	else if ( NodeRole::Database == status) return "Database";
	else if ( NodeRole::Master == status) return "Master";
	else if ( NodeRole::Piece == status) return "Piece";
	return "unknow";
}

//hashidÂäÔÚ¼¸ºÅÆ¬ÉÏ
unsigned int CalPieceNo( unsigned int hashid, unsigned int pieceSize )
{
	return hashid/pieceSize;
}

