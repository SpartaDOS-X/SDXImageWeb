#include "uint3.h"

#include <cassert>
#include <cstring>

UINT3::UINT3()
{
    d[0]=d[1]=d[2]=0;
}

UINT3::UINT3(int x)
{
    assert(x<(1l<<24));
    d[0]=x%256;
    d[1]=(x/256)%256;
    d[2]=(x/256/256)%256;
}

UINT3 operator +(UINT3 x, UINT3 y)
{
  UINT3 z;
  for(int i = 0;i < 3;i++)
    z.d[i]=x.d[i]+y.d[i];
  return z;
}


int operator ==(UINT3 x, UINT3 y)
{
  int ne = 0;
  for(int i = 0;i < 3;i++)
    ne |= x.d[i] != y.d[i];
  return !ne;
}


UINT3::operator unsigned int() const
{
	return d[0] + 256*d[1] + 256*256*d[2];
}

UINT3::operator unsigned char*()
{
	return d;
}

void UINT3::FromPtr(unsigned char* ptr)
{
	memcpy(d, ptr, 3);
}
