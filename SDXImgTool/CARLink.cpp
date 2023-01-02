#include <cstring>

#include "CARLink.h"

CCARLink::CCARLink(CCAREntry* linkedTo)
{
	mLinkedTo = linkedTo;
	status = mLinkedTo->status;

	for (int i=0;i<3;i++)
	{
		time[i] = mLinkedTo->time[i];
	}

	date = mLinkedTo->date;

	length = 0;
	start = 0;
	bank = -1;
	start_img = 0;

	name = mLinkedTo->name;
	name.replace(10,1,"_");
}

CCARLink::~CCARLink(void)
{
}

void CCARLink::GetSDXEntry(CSDXEntry &sdxentry)
{
	mLinkedTo->GetSDXEntry(sdxentry);

	sdxentry.status = this->status | 0x40;	// bit 6 = hard link

	memcpy(sdxentry.name,this->name.data(),11);
	memcpy(sdxentry.date,this->date,3);
	memcpy(sdxentry.time,this->time,3);
}