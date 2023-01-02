
#include "CARFile.h"

CCARFile::CCARFile()
{
	contents = NULL;
	bank = 0xFF;				// no bank selected
}

CCARFile::~CCARFile()
{
	if (contents) delete contents;
}
