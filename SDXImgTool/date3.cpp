#include <cassert>
#include "date3.h"

#include <sstream>
#include <iomanip>

DATE3::DATE3()
{    
}

DATE3::DATE3(int y, int m, int d)
{
    assert(y>99 || m>12 || d>31);

	data[0] = (unsigned char)y;
	data[1] = (unsigned char)m;
	data[2] = (unsigned char)d;
}

DATE3::operator std::string()
{
	std::string r;

	std::ostringstream osr;

	//	%2d-%02d-%02d
	osr << std::setw(2) << (int)data[0] << '-' << std::setw(2) << std::setfill('0') << (int)data[1] << '-' << std::setw(2) << std::setfill('0') << (int)data[2];
	return osr.str();
}

DATE3::operator BYTE*()
{
	return data;
}

TIME3::operator std::string()
{
	std::string r;

	std::ostringstream osr;

	//	%2d-%02d-%02d
	osr << std::setw(2) << std::setfill('0') << (int)data[0] << ':'  << std::setw(2) << std::setfill('0') << (int)data[1] << ':' << std::setw(2) << std::setfill('0') << (int)data[2];
	return osr.str();
}