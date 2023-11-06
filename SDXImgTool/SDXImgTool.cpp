#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>

#include "CAR.h"

using namespace std;


struct CARFileInfo
{
	int directory_position;
	unsigned char status;
	unsigned char time0;
	unsigned char time1;
	unsigned char time2;
	unsigned char date0;
	unsigned char date1;
	unsigned char date2;
};

extern "C" {
	CCAR* CreateCart();
	bool OpenRom(CCAR* cart, unsigned char data[], int length);
	int GetFileCount(CCAR* cart);
	const char* GetFileName(CCAR* cart, int index);
	int GetFileSize(CCAR* cart, int index);
	const unsigned char* GetFileContents(CCAR* cart, int index);
	bool SetFileContents(CCAR* cart, int index, unsigned char data[], int length);
	int GetTotalBytes(CCAR* cart);
	int GetCapacity(CCAR* cart);
	bool SaveImage(CCAR* cart, const char* destfile);
	bool RemoveFile(CCAR* cart, const char* file);
	bool GetCarEntry(CCAR* cart, const char* file, CARFileInfo* info);
	int GetType(CCAR* cart);
	void CloseCart(CCAR* cart);
	bool InsertFile(CCAR* cart, const char* filename, unsigned char data[], int length);
}

CCAR* CreateCart()
{
	return new CCAR(true, true, true);
}

bool OpenRom(CCAR* cart, unsigned char data[], int length)
{
	if (cart == NULL)
		return false;

	std::ofstream file("SDX.ROM", std::ios::binary);
	file.write((const char*)data, length);
	file.close();

	return cart->OpenRom("SDX.ROM");
}

int GetFileCount(CCAR* cart)
{
	if (cart != NULL)
	{
		return cart->GetFileCount();
	}
}

const char* GetFileName(CCAR* cart, int index)
{
	if (cart != NULL)
	{
		CCAREntry* file = cart->GetCAREntry(index);
		if (file != NULL)
		{
			return file->name.c_str();
		}
	}

	return NULL;
}

int GetFileSize(CCAR* cart, int index)
{
	if (cart != NULL)
	{
		CCAREntry* file = cart->GetCAREntry(index);
		if (file != NULL)
		{
			return file->length;
		}
	}

	return 0;
}

const unsigned char* GetFileContents(CCAR* cart, int index)
{
	if (cart != NULL)
	{
		CCARFile* file = (CCARFile*)cart->GetCAREntry(index);
		if (file != NULL && !(file->status & 0x40))
		{
			return file->contents;
		}
	}

	return NULL;
}

bool SetFileContents(CCAR* cart, int index, unsigned char data[], int length)
{
	if (cart != NULL)
	{
		CCARFile* file = (CCARFile*)cart->GetCAREntry(index);
		if (file != NULL && !(file->status & 0x40))
		{
			delete file->contents;
			file->contents = new BYTE[length];
			file->length = length;
			MemReaderWriter writer(file->contents, length);
			return writer.Write((BYTE*)data, length) == length;
		}
	}

	return false;
}

int GetTotalBytes(CCAR* cart)
{
	if (cart != NULL)
	{
		return cart->GetTotalBytes();
	}

	return 0;
}

int GetCapacity(CCAR* cart)
{
	if (cart != NULL)
	{
		return cart->GetCapacity();
	}

	return 0;
}

bool SaveImage(CCAR* cart, const char* destfile)
{
	if (cart != NULL)
	{
		std::string filename;
		filename = destfile;
		return cart->SaveImage(filename);
	}

	return false;
}

bool RemoveFile(CCAR* cart, const char* file)
{
	if (cart != NULL)
	{
		return cart->RemoveFile(file);
	}

	return false;
}

void CloseCart(CCAR* cart)
{
	if (cart != NULL)
	{
		cart->DeleteAllFiles();
	}
}

bool GetCarEntry(CCAR* cart, const char* file, CARFileInfo* info)
{

	if (cart != NULL)
	{
		CCAREntry* entry = cart->GetCAREntry(file);
		if (entry != NULL)
		{
			info->directory_position = entry->directory_position;
			info->status = entry->status;
			info->time0 = entry->time[0];
			info->time1 = entry->time[1];
			info->time2 = entry->time[2];
			info->date0 = entry->date[0];
			info->date1 = entry->date[1];
			info->date2 = entry->date[2];
		}
		return true;
	}

	return false;
}


const char* GetFileDate(CCAREntry* entry, int index)
{
	if (entry != NULL)
	{
		return ((std::string)entry->date).c_str();
	}

	return NULL;
}

int GetType(CCAR* cart)
{
	if (cart != NULL)
	{
		return cart->GetType();
	}

	return 0;
}

bool InsertFile(CCAR* cart, const char* filename, unsigned char data[], int length)
{
	if (cart == NULL)
		return false;

	std::ofstream file(filename, std::ios::binary);
	file.write((const char*)data, length);
	file.close();

	return (cart->InsertFile(filename) != NULL);
}
