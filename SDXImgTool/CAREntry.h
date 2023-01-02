#ifndef _CARENTRY_H
#define _CARENTRY_H

#include <string>

#include "types.h"

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	BYTE status;
	WORD start;
	BYTE length[3];
	BYTE name[8+3];
	BYTE date[3];
	BYTE time[3];
} CSDXEntry;

#pragma pack(pop)

class CCAREntry 
{

public:
	int directory_position;
	BYTE status;
	TIME3 time;
	DATE3 date;
	UINT3 length;
	WORD start;
	BYTE bank;
	WORD start_img;	// offset in the bank

	std::string name;
	virtual void GetSDXEntry(CSDXEntry &sdxentry);
	std::string GetCSVEntry(void);
	std::string GetEntryText(void);

	static std::string ToDirEntryName(std::string filename);
public:
	CCAREntry(void);
	~CCAREntry(void);
};

#endif // _CARENTRY_H
