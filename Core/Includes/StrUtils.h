// StrUtils.h
#ifndef ZORGE_STR_UTILS_H
#define ZORGE_STR_UTILS_H

#include "ZorgeString.h"

bool 	GuidStr(CZorgeString&);
bool 	GuidStrDash(CZorgeString&);

void 	strcpyz(char*, unsigned int, const char*);
const char* ByteToHexStr(unsigned char);
unsigned char HexCharsToByte(char, char);

#endif

