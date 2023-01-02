#ifndef _UINT3_H
#define _UINT3_H

class UINT3
{
protected:

public:
	UINT3();
	UINT3(int x);

	friend UINT3 operator +(UINT3, UINT3);
	friend int operator ==(UINT3, UINT3);

	operator unsigned int() const;
	operator unsigned char*();
	
	void FromPtr(unsigned char* ptr);

	unsigned char d[3];

};

#endif //_UINT3_H
