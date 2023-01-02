#ifndef _CARFILE_H
#define _CARFILE_H

#include "CAREntry.h"

class CCARFile : public CCAREntry  
{
public:
	BYTE* contents;

	CCARFile();
	~CCARFile();
};

#endif // _CARFILE_H
