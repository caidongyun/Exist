/********************************************************************
 *	Filename: 	MD5.h
 *	Author  :	ÀÔ«Æ
 *  Date    :	2010/01/05		17:18
 *
 *	description:
 ********************************************************************/
#ifndef MD5_H
#define MD5_H

void MD5Digest( char *pszInput, unsigned int nInputSize, char *pszOutPut );//md5º”√‹
void MD5HashFunction(unsigned char *hashKey, unsigned int &hashSize, 
					 unsigned char *key, unsigned int size );
					 
#endif // !defined(MD5_H)
