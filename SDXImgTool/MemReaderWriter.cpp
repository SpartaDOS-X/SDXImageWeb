#include "MemReaderWriter.h"
#include <string.h>

MemReaderWriter::MemReaderWriter(BYTE* buf, size_t size)
{
	data = buf;
	length = size;
	pointer = 0;
}

size_t MemReaderWriter::Read(BYTE* buf, size_t size)
{
	memcpy(buf, data + pointer, size);
	pointer += size;
	return size;
}

size_t MemReaderWriter::Write(BYTE* buf, size_t size)
{
	memcpy(data + pointer, buf, size);
	pointer += size;
	return size;
}

void MemReaderWriter::Seek(size_t pos)
{
	pointer = pos;
}
