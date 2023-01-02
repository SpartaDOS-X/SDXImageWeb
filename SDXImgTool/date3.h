#ifndef _DATE3_H
#define _DATE3_H

#include "types.h"

#include <string>

struct DATE3
{
protected:

public:
	DATE3();
	DATE3(int y, int m, int d);

	operator std::string();
	operator BYTE*();

	unsigned char data[3];
};

struct TIME3 : DATE3
{
public:
	operator std::string();
};

#endif //_UINT3_H
