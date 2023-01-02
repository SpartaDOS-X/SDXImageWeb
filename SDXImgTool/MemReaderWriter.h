#ifndef _MEMREADERWRITER_H
#define _MEMREADERWRITER_H

#include "types.h"

class MemReaderWriter
{
public:
	MemReaderWriter(BYTE* buf, size_t size);
	size_t Read(BYTE* buf, size_t size);
	size_t Write(BYTE* buf, size_t size);
	void Seek(size_t pos);
private:
	BYTE* data;
	size_t length;
	size_t pointer;
};

#endif
