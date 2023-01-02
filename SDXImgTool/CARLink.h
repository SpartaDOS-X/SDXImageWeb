#ifndef _CARLINK_H
#define _CARLINK_H

#include "CAREntry.h"

class CCARLink :
	public CCAREntry
{
private:
	CCAREntry* mLinkedTo;
public:
	CCAREntry* GetLinkedCAREntry() { return mLinkedTo; };

	void GetSDXEntry(CSDXEntry &sdxentry);

	CCARLink(CCAREntry* linkedTo);
	~CCARLink(void);
};

#endif // _CARLINK_H
