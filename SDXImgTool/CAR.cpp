#include <ctime>
#include <algorithm>
#include <streambuf>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>

//#include <boost/lexical_cast.hpp>
//#include <boost/filesystem.hpp>
//#include <boost/tokenizer.hpp>
//#include <boost/foreach.hpp>
//#include <boost/algorithm/string.hpp>

using namespace std;

#include "CAR.h"
//#include "Logger.h"

#pragma pack(1)

BYTE CarHeader[16]; // = {0x8D, 0xE0, 0xD5, 0x4C, 0x78, 0xA0, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xF0, 0xBF};

static unsigned int MinGap = 0xFFFF;  //do optymalizacji - minimalna wolna przestrzen w banq
static long int Iterations = 0;
static CCAREntryList OptimalList;

struct Sector
{
	BYTE data_format_version;
	WORD main_directory_ptr;
	WORD total_sectors;	// number of banks
	WORD free_sectors;
	BYTE bmap_len;
	WORD bmap_begin;
	WORD filef, dirf;
	BYTE volume_name[8];
	BYTE numtrk;
	BYTE bps;
	BYTE ver;
	WORD rnbps;
	WORD fmapen;
	WORD sector_per_cluster;
	BYTE seq, rnd;
	WORD bootp;
	BYTE lock;
} Car1Sector;

void *memichr(const void *buf, int c, size_t buf_len)
{
	BYTE *p = (BYTE *) buf;
	BYTE b_lower = (BYTE) c;
	BYTE b_upper = (BYTE) c;

	if ((b_lower >= 'A') && (b_lower <= 'Z'))
		b_lower = (BYTE) (b_lower + ('a' - 'A'));

	if ((b_upper >= 'a') && (b_upper <= 'z'))
		b_upper = (BYTE) (b_upper - ('a' - 'A'));

	for (unsigned int i = 0; i < buf_len; i++)
	{
		if ((*p == b_lower) || (*p == b_upper))
			return p;
		p++;
	}
	return NULL;
}


int memicmp(const void *s1, const void *s2, size_t n)
{
	if (n != 0) {
		const unsigned char *p1 = (const unsigned char *)s1, *p2 = (const unsigned char *)s2;
		do {
			if (toupper(*p1) != toupper(*p2))
				return (*p1 - *p2);
			p1++;
			p2++;
		} while (--n != 0);
	}
	return 0;
}

void *memimem(const void *buf, 
	size_t buf_len, 
	const void *byte_sequence, 
	size_t byte_sequence_len)
{
	BYTE *bf = (BYTE *)buf;
	BYTE *bs = (BYTE *)byte_sequence;
	BYTE *p  = bf;

	while (byte_sequence_len <= (buf_len - (p - bf)))
	{
		unsigned int b = *bs & 0xFF;
		if ((p = (BYTE *) memichr(p, b, buf_len - (p - bf))) != NULL)
		{
			if ((memicmp(p, byte_sequence, byte_sequence_len)) == 0)
				return p;
			else  
				p++;
		}
		else  
		{
			break;
		}
	}
	return NULL;
}


void SetCurrentTime(CCAREntry* entry)
{

	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	entry->date[0] = timeinfo->tm_mday;
	entry->date[1] = timeinfo->tm_mon+1;
	entry->date[2] = timeinfo->tm_year % 100;

	entry->time[0] = timeinfo->tm_hour;
	entry->time[1] = timeinfo->tm_min;
	entry->time[2] = timeinfo->tm_sec;

}


bool AnswerYes()
{
	std::string yesno;
	char yn;

	cin >> yn;

	if (toupper(yn) == 'Y')
		return true;

	return false;

}


CCAR::CCAR(bool isexpert, bool issilent, bool isverbose)
{

	m_DirHead = new CCAREntry();

	SetCurrentTime(m_DirHead);

	SetType(CART_SDX128);

	m_Bank0Area = 0xD00-16;	// default for older images
	m_Bank3Area = 0;

	m_hRomFile = NULL;

	limit8k = false;
	expert = isexpert;
	silent = issilent;
	verbose = isverbose;

}

CCAR::~CCAR()
{
	if (m_DirHead) delete m_DirHead;
	DeleteAllFiles();

}

int CCAR::GetTotalBytes()
{
	int total = 0;

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
		total += (*iter)->length;

	if (!limit8k)
	{
		total += GetDirectorySize();	// for continuous CAR: add size of directory
	}

	return total;
}

int CCAR::GetCapacity()
{
	int capacity = 0;

	for (int i=0; i<=GetMaxBank(); i++)
		capacity += GetBankCapacity(i);
	return capacity;
}

int CCAR::GetFreeBytes()
{
	return GetCapacity()-GetTotalBytes();
}

void CCAR::DeleteAllFiles()
{

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
		delete (*iter);

	m_Files.clear();
}

WORD CCAR::GetBankFree(BYTE bankno)
{
	WORD used = 0;

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;

		if (file->bank == bankno)
			used += file->length;
	}

	return (GetBankCapacity(bankno) - used);
}

WORD CCAR::GetBankCapacity(BYTE bankno)
{
	switch(bankno)
	{
	case 0:
		return m_Bank0Area;
	case 1:
		return 0;
	case 2:
		if (limit8k)
			return 0x2000 - (m_Files.size() + 1) * CARENTRYLEN - 16; //+2, bo ka¿dy nowy plik zmniejsza
		else
			return 0x2000 - 16;
	case 3:
		return m_Bank3Area;
	default:
		return 0x2000 - 16;
	}
}

WORD CCAR::GetBankStartOfs(BYTE bankno, bool includeDirectory)
{
	switch(bankno)
	{
	case 0:
		return 0x2000-m_Bank0Area-16;
	case 1:
		return 0x2000;  // bank not used
	case 3:
		return 0x2000-m_Bank3Area-16;
	case 2:
		if (limit8k)
			return includeDirectory ? ((m_Files.size() + 1) * CARENTRYLEN) : 0;
		else
			return 0;
	default:
		return 0;
	}

}

bool CCAR::AllocateBank(BYTE bankno, MemReaderWriter& outfile)
{
	bool allocated = false;

	outfile.Seek(GetBankOffset(bankno)+GetBankStartOfs(bankno));

	int bankofs = 0;

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;

		if (file->bank != 0xFF)
			continue;				// already allocated

		CCARFile* cfile;

		if ((cfile = dynamic_cast<CCARFile*>(file)) )
		{

			if ((GetBankStartOfs(bankno) + bankofs + (int)cfile->length) > (0x2000 - 16))	// do we exceed bank size ?
			{
				continue;
			}

			outfile.Write(cfile->contents,cfile->length);
			cfile->bank = bankno;
			cfile->start_img = GetBankStartOfs(bankno)+bankofs;

			if (!silent)
				cout << "File: " << cfile->name << " Len: " << (int)cfile->length << " Banks: Start: " << (int)cfile->bank << " at " << cfile->start_img;
	
			bankofs += (int)cfile->length;

			if (!silent)
				cout << " End bank: " << (int)bankno  << " at " << GetBankStartOfs(bankno)+bankofs << endl;

			allocated = true;
		}
	}

	return allocated;
}

bool CCAR::SaveImage(std::string destfile)
{

	BYTE* outdata = new BYTE[GetCartSize()];

	MemReaderWriter outfile(outdata, GetCartSize());

	// wyzerowanie ca³ego romu (wartoœci¹ $FF)
	memset(outdata, 0xFF, GetCartSize());

	// kopiowanie plików

	int bankofs;

	if (limit8k)
	{

		for (BYTE i=0; i<=GetMaxBank(); i++)
		{
			outfile.Seek(GetBankOffset(i)+GetBankStartOfs(i));

			bankofs = GetBankStartOfs(i);

			for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
			{
				CCAREntry* file = *iter;

				CCARFile* cfile;

				if ((cfile = dynamic_cast<CCARFile*>(file) ))
				{

					if (cfile->bank != i)
						continue;

					if ((bankofs + (int)cfile->length) > 0x2000)	// do we exceed bank size ?
					{
						if (limit8k)
						{
							std::cerr << "Bank size overflow for " << cfile->name << endl;
							return false;
						}				
					}

					BYTE *buf = new BYTE[cfile->length];

					outfile.Write(cfile->contents,cfile->length);		
					cfile->start_img = bankofs;
					bankofs += cfile->length;

					delete[] buf;
				}
			}

		}
	}
	else
	{	

		int bank = 0;

		// clear completely bank settings
		for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
		{
			CCAREntry* file = *iter;
			file->bank = 0xFF;
		}

		bankofs = 0;


		// reserve space for directory
		int unallocated = GetDirectorySize();
		int part_size = min(unallocated, GetBankCapacity(bank) - bankofs);
		bankofs += part_size;
		unallocated -= part_size;

		while (unallocated > 0)			// the file expands accross banks
		{
			bank = GetNextCarBank(bank);

			if (bank == -1)				// no more room in the continuous bank area 
			{
				std::cerr << "No room for directory ! " << endl;
				return false;
			}

			bankofs = 0;
			part_size = min(unallocated, GetBankCapacity(bank) - bankofs);

			bankofs += part_size;
			unallocated -= part_size;

		}


		for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
		{
			CCAREntry* file = *iter;
			
			if (file->bank != 0xFF)
				continue;				// already allocated

			CCARFile* cfile;

			if ((cfile = dynamic_cast<CCARFile*>(file) ))
			{

				cfile->bank = bank;
				cfile->start_img = GetBankStartOfs(bank)+bankofs;

				if (!SaveFileToRom(outfile, cfile, bank, bankofs))
				{
					std::cerr << "Cannot save file " << file->name << endl;
					return false;
				}

			}
		}

	}

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;
		if (file->bank == 0xFF)
		{
			std::cerr << "No room to allocate file " << file->name << endl;
			return false;
		}
	}

	// aktualizacja linków
	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;
		CCARLink* clink;

		if ((clink = dynamic_cast<CCARLink*>(file) ))
		{
			clink->bank = clink->GetLinkedCAREntry()->bank;
			clink->start_img = clink->GetLinkedCAREntry()->start_img;
			//clink->length = clink->GetLinkedCARFile()->length;
		}
	}

	// zachowanie katalogu

	CCARFile dirfile;
	dirfile.bank = GetDirectoryBank();
	dirfile.start_img = GetBankStartOfs(dirfile.bank);
	dirfile.length = ((m_Files.size() + 1) * CARENTRYLEN);
	dirfile.contents = new BYTE[dirfile.length];
	dirfile.name = "Main directory";

	// nag³ówka
	CCAREntry dirhead;
	dirhead.status = 0x08;
	dirhead.start = 0x4000;
	dirhead.bank = 2;
	dirhead.start_img = 0x4000;
	dirhead.length = ((m_Files.size()+1) * CARENTRYLEN); 
	dirhead.name = "MAIN       ";

	SetCurrentTime(&dirhead);

	CSDXEntry sdxentry;

	dirhead.GetSDXEntry(sdxentry);

	memcpy(dirfile.contents, &sdxentry, CARENTRYLEN);

	int dirbank = GetDirectoryBank();
	int dirofs = 0;

	// i plików
	int dirpos = 1;
	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;
		file->GetSDXEntry(sdxentry);
		memcpy(dirfile.contents + (dirpos* CARENTRYLEN), &sdxentry, CARENTRYLEN);

		dirpos++;
	}

	SaveFileToRom(outfile, &dirfile, dirbank, dirofs);

	// kopiowanie sta³ych banków kernela

	BYTE *buf = new BYTE[0x2000];

	MemReaderWriter memfile(m_hRomFile, GetCartSize());
	memfile.Seek(GetBankOffset(1));
	memfile.Read(buf,0x2000);
	outfile.Seek(GetBankOffset(1));
	outfile.Write(buf,0x2000);		

	memfile.Seek(GetBankOffset(3));
	memfile.Read(buf,0x2000);
	outfile.Seek(GetBankOffset(3));
	outfile.Write(buf,0x2000 - m_Bank3Area - 16);		

	memfile.Seek(GetBankOffset(0));
	memfile.Read(buf,0x2000 - m_Bank0Area - 16);
	outfile.Seek(GetBankOffset(0));
	outfile.Write(buf,0x2000 - m_Bank0Area - 16);		

	delete[] buf;

	// wpisanie nag³ówków kartridge'a
	memfile.Seek(GetBankOffset(1)+0x1FF0);
	memfile.Read(CarHeader,sizeof(CarHeader));

	for (BYTE i=0; i<=GetMaxBank(); i++)
	{
		CarHeader[0x09]=i;
		outfile.Seek(GetBankOffset(i) + 0x2000 - sizeof(CarHeader));
		outfile.Write((BYTE*)&CarHeader,sizeof(CarHeader));
	}

	// wpisanie w³aœciwej wielkoœci CAR: (4.48 i wy¿sze)
	memfile.Seek(GetBankOffset(3) + 0x0000);
	memfile.Read((BYTE*)&Car1Sector,sizeof(Car1Sector));

	if (!memcmp(Car1Sector.volume_name, "Cart ",5))	// sector 1 in bank 3 ? (4.48 and above)
	{
		Car1Sector.total_sectors = this->GetMaxBank()+1;
		outfile.Seek(GetBankOffset(3) + 0x0000);
		outfile.Write((BYTE*)&Car1Sector,sizeof(Car1Sector));
	}

	//	destfile.Close();

	// tworzenie pliku

	ofstream out(destfile.data(), std::ofstream::binary);

	if( !out.good() )
	{
		std::cerr << "Cannot save the image !" << endl;
		return false;
	}

	out.write((char*)outdata,GetCartSize());
	out.close();

	delete[] outdata;

	if (!silent)
		cout << "Image saved as: " << destfile << endl;

	return true;

}

int CCAR::GetBankOffset(BYTE bankno)
{
	switch (GetType())
	{
	case CART_NORMAL:
		return (7-bankno)*0x2000;
	case CART_SDX128:
		return (GetMaxBank()-bankno)*0x2000;
		//case CART_SDX256:
		//	return (0x10*(bankno/0x10) + (0xF - bankno%0x10)) * 0x2000;
	default:		// ascending banks
		return (bankno)*0x2000;
	}
	return 0;
}


bool CCAR::IsUserBank(BYTE bankno)
{

	switch (GetType())
	{
	case CART_NORMAL:
		return false;
	default:
		return limit8k ? bankno >= 0x0F : 1;
	}
	return 0;
}


bool CCAR::OpenRom(std::string romfile, bool bFiles)
{
	CSDXEntry dirbuf;
	CCAREntry *file;
	int i;

	std::ifstream inpfile(romfile.data(), std::ifstream::binary);

	if( !inpfile.good() )
	{
		std::cerr << "Cannot open the image !" << endl;
		return false;
	}

	limit8k = false;

	// get length of file:
	inpfile.seekg (0, inpfile.end);
	int length = inpfile.tellg();

	inpfile.seekg (0, inpfile.beg);

	if (!silent)
	{
		cout << "Image size " << length << endl;
	}

	DeleteAllFiles();

	switch(length)
	{
	case 0x10000:		// old 64k cartridge
		SetType(CART_NORMAL);
		break;
	default:
		{
			SetType(1);	// assume ascending banks

			char* buf = new char[3];
			// read first 3 bytes to recognize the type
			inpfile.read(buf, 3);
			if (memcmp(buf, "SDX", 3))				// signature not found ?
			{
				inpfile.seekg(-0x2000,inpfile.end);	// go to the last bank
				inpfile.read(buf, 3);				// read 3 bytes
				if (!memcmp(buf, "SDX", 3))			// check signature
					SetType(2);						// descending banks if found
				else
				{
					std::cerr << "Bad image format (not SDX) !" << endl;
					return false;					// not SDX 4.4 cart
				}
			}

			if (length != GetCartSize())
			{
				if (length > GetCartSize())
					SetMaxBank(length / 0x2000 - 1);	// increase cart size
			}
		}
		break;
	}

	if (!silent)
	{
		cout << "Cart type set to " << this->m_Type << endl;
		cout << "Cart size " << GetCartSize() << endl;
	}

	if (m_hRomFile)
		delete[] m_hRomFile;

	m_hRomFile = new BYTE[GetCartSize()];

	inpfile.seekg (0, inpfile.beg);
	inpfile.read((char*)m_hRomFile,length);

	if (!inpfile)
	{
		inpfile.close();
		std::cerr << "Cannot read the image !" << endl;
		return false;
	}

	MemReaderWriter memfile(m_hRomFile, GetCartSize());

	BYTE *rbuf = new BYTE[0x2000];	// bank size

	// seek signature in bank 0

	memfile.Seek(GetBankOffset(0));
	memfile.Read(rbuf, 0x2000);

	BYTE *spos = (BYTE*)memimem(rbuf,0x2000,"SPARTA  DOS",11);
	if (!spos)
		m_Bank0Area = 0;
	else
	{
		spos += 28;
		m_Bank0Area = (rbuf + 0x2000 - 16) - spos;	// skip some stuff
	
	}

	memfile.Seek(GetBankOffset(2));
	memfile.Read(rbuf, 0x2000);
	// do we have a directory in bank 2 ?
	BYTE* mpos = (BYTE*)memimem(rbuf, 0x2000, "MAIN       ", 11);
	if (mpos)
	{	
		limit8k = true;
	}



	// seek signature in bank 3

	memfile.Seek(GetBankOffset(3));
	memfile.Read(rbuf, 0x2000);

	spos = (BYTE*)memimem(rbuf,0x2000,"SdX",3);
	if (!spos)
		m_Bank3Area = 0;
	else
	{
		spos += 5;
		m_Bank3Area = (rbuf + 0x2000) - spos - 16;	// skip some stuff
	}

	delete[] rbuf;

	if (!bFiles)	// skip file processing
		return true;

	memfile.Seek(GetDirectoryOfs());
	memfile.Read((BYTE*)&dirbuf, CARENTRYLEN);  // read directory header

	if (((dirbuf.status != 0x08) || (strncmp("MAIN", (const char*)dirbuf.name ,4))))
	{
		if (!expert)
		{
			std::cerr << "No directory found! (not an SDX image?)" << endl;
			return false;
		}
		else
		{
			if (!silent)
				cout << "Blank image found (no files)" << endl;
			return true;		// stop here, skip directory processing
		}

	}


	// check directory header

	m_DirHead = EntryFromBuf(&dirbuf);

	int dirlen = ((int)m_DirHead->length - CARENTRYLEN) / CARENTRYLEN;

	if (dirlen==0 && !expert)
	{
		std::cerr << "No files in the image!" << endl;
		return false;
	}

	if (verbose)
	{
		cout << "Number of files " << dirlen << endl;
	}

	CCARFile dirfile;

	dirfile.bank = GetDirectoryBank();
	dirfile.start = GetBankStartOfs(dirfile.bank, false) + CARENTRYLEN;
	dirfile.length = (int)dirlen * CARENTRYLEN;

	if (verbose)
	{
		cout << "Loading directory at " << GetBankStartOfs(dirfile.bank) << endl;
	}

	if (!ReadFileFromRom(memfile, &dirfile))
	{
		std::cerr << "Cannot read directory !" << endl;
		return false;
	}

	for (int i = 0; i<dirlen; i++)
	{
		file = EntryFromBuf((CSDXEntry*)(dirfile.contents + i*CARENTRYLEN));
		if (file)
		{
			m_Files.push_back(file);			// add entry to the directory list
			if (verbose)
			{
				cout << "File " << i << ": " << file->name << endl;
			}
		}

	}

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;
		CCARFile* cfile;

		if ((cfile = dynamic_cast<CCARFile*>(file) ))
		{	
			if (!ReadFileFromRom(memfile, cfile))
			{
				std::cerr << "Cannot read file " << cfile->name << " from ROM !" << endl;
				return false;
			}

			if (verbose)
			{
				cout << "Loaded : " << file->name << endl;
			}

		}

	}

	//    SortFiles();
	return true;
}


bool CCAR::ReadFileFromRom(MemReaderWriter& memfile, CCARFile* cfile)
{
	cfile->contents = new BYTE[cfile->length];

	BYTE* buf = cfile->contents;

	memfile.Seek(GetBankOffset(cfile->bank) + cfile->start);

	int bank = cfile->bank;
	int to_read = (int)cfile->length;

	int part_size = min(to_read, 0x2000 - 16 - cfile->start);

	memfile.Read(buf, part_size);

	to_read -= part_size;

	while (to_read > 0)			// the file expands accross banks
	{
		bank = GetNextCarBank(bank);
		memfile.Seek(GetBankOffset(bank) + GetBankStartOfs(bank));

		if (bank == -1)				// no more room in the continuous bank area 
		{
			cfile->bank = 0xFF;
			break;
		}

		buf += part_size;

		part_size = min(to_read, (int)GetBankCapacity(bank));

		memfile.Read(buf, part_size);

		to_read -= part_size;

	}

	return true;
}


bool CCAR::SaveFileToRom(MemReaderWriter& outfile, CCARFile* cfile, int &bank, int &bankofs)
{
	if (verbose)
		cout << "File: " << cfile->name << " Len: " << (int)cfile->length << " Banks: Start: " << (int)cfile->bank << " at " << cfile->start_img;

	outfile.Seek(GetBankOffset(bank) + GetBankStartOfs(bank, false) + bankofs);

	int unallocated = (int)cfile->length;

	int part_size = min(unallocated, GetBankCapacity(bank) - bankofs);

	BYTE* buf = cfile->contents;

	outfile.Write(buf, part_size);

	bankofs += part_size;
	buf += part_size;

	unallocated -= part_size;

	while (unallocated > 0)			// the file expands accross banks
	{
		bank = GetNextCarBank(bank);

		if (bank == -1)				// no more room in the continuous bank area 
		{
			cfile->bank = 0xFF;
			break;
		}

		bankofs = 0;

		outfile.Seek(GetBankOffset(bank) + GetBankStartOfs(bank) + bankofs);

		part_size = min(unallocated, GetBankCapacity(bank) - bankofs);

		outfile.Write(buf, part_size);

		bankofs += part_size;
		buf += part_size;

		unallocated -= part_size;

	}

	if (bank == -1)
		return false;

	if (verbose)
		cout << " End bank: " << bank << " at " << GetBankStartOfs(bank) + bankofs - 1 << endl;

	return true;

}

bool SortByName(const CCAREntry* a, const CCAREntry* b) { return a->name < b->name; }
bool SortByBank(const CCAREntry* a, const CCAREntry* b) { return a->bank < b->bank; }
bool SortByLength(const CCAREntry* a, const CCAREntry* b) { return a->length < b->length; }
bool SortByLengthDesc(const CCAREntry* a, const CCAREntry* b) { return a->length > b->length; }

void CCAR::SortFiles(int sort_order)
{

	switch (sort_order)
	{
	case 0:
		m_Files.sort(SortByName);
		break;
	case 1:
		m_Files.sort(SortByBank);
		break;
	case 2:
		m_Files.sort(SortByLength);
		break;
	case 3:
		m_Files.sort(SortByLengthDesc);
		break;
	default:
		return;
	}

	if (sort_order != 0)
		return;

	// dla alfabetycznego wg nazwy ustal specjaln¹ kolejnoœæ (MENU.COM)

	CCAREntry* menucom = this->GetCAREntry("MENU    COM");
	this->MoveToTop(menucom);
}

int CCAR::GetDirectoryBank(void)
{
	if (limit8k)
		return 2;
	else
		return 0;
}

int CCAR::GetDirectorySize(void)
{
	return (m_Files.size() + 1) * CARENTRYLEN;
}

int CCAR::GetDirectoryOfs(void)
{
	if (!limit8k)
		return GetBankOffset(GetDirectoryBank()) + GetBankStartOfs(0);
	else
		return GetBankOffset(GetDirectoryBank());	// dir starts at offset 0 in bank 2
}

int CCAR::SetType(int nType, bool bSaveMaxBank)
{
	m_Type = nType;

	if (!bSaveMaxBank)
		SetMaxBank(0);	// reset max bank

	return m_Type;
}


BYTE CCAR::GetMaxBank(void)
{
	if (m_MaxBank > 0)
		return m_MaxBank;
	else
	{
		switch(GetType())
		{
		case CART_NORMAL:
			return 0x7;
		case CART_MAXFLASH1MB:
		case CART_SDX128:
			return 0xF;
		}
	}
	return 0;
}

BYTE CCAR::SetMaxBank(int maxbank)
{
	m_MaxBank = maxbank;
	return m_MaxBank;
}

int CCAR::GetCartSize(void)
{
	return (GetMaxBank() + 1 ) * 0x2000;
}

int CCAR::GetFileCount(void)
{
	return m_Files.size();
}

int CCAR::MoveToTop(int file_idx)
{
	CCAREntry* entry = this->GetCAREntry(file_idx);

	return MoveToTop(entry);
}

int CCAR::MoveToTop(CCAREntry* entry)
{
	if (entry != NULL)
	{
		m_Files.remove(entry);
		m_Files.push_front(entry);
		return 0;
	}
	else
	{
		return -1;
	}
}

bool CCAR::RemoveFile(CCAREntry* file)
{
	m_Files.remove(file);
	delete file;
	return true;
}


bool CCAR::RemoveFile(std::string filename)
{
	if (CCAREntry* file = GetCAREntry(filename))
	{
		if (!silent)
		{
		cout << "Do you want to delete the file " << file->name << " ? ";

		if (!AnswerYes())
			return false;
		}

		m_Files.remove(file);
		
		if (!silent)
		{
			cout << file->name << " deleted from the image." << endl;
		}
		delete file;
		return true;
	}
	else
	{
		if (!expert)
		{
			std::cerr << filename << " cannot be deleted !" << endl;
			return false;
		}

		if (verbose)
			cout << filename << " not found, skipping delete." << endl;

		return true;
	}

}

CCAREntry* CCAR::GetCAREntry(int file_idx)
{
	if (m_Files.size() > (unsigned int)file_idx)
	{
		std::list<CCAREntry*>::iterator it = m_Files.begin();
		std::advance(it, file_idx);
		return *it;
	}

	return NULL;
}

CCAREntry* CCAR::EntryFromBuf(CSDXEntry* dirbuf)
{
	char fname[12];

	CCAREntry* file;

	if (dirbuf->status == 0xFF)
		return NULL;

	if (dirbuf->status & 0x40)	// link
	{
		file = (CCAREntry*) new CCARLink(NULL);	
	}
	else
	{
		file = (CCAREntry*) new CCARFile();	
	}

	file->status = dirbuf->status;

	memcpy(fname,dirbuf->name,11);
	fname[11] = 0;
	file->name = fname;

	file->start = dirbuf->start & 0x1FFF;

	if (dirbuf->length[2])
		file->bank = dirbuf->length[2];
	else
		file->bank = dirbuf->start >> (8+5);	// extract hi byte and bank portion

	file->length = UINT3(dirbuf->length[0] + 256*dirbuf->length[1] );

	memcpy(file->date.data,dirbuf->date,3);
	memcpy(file->time.data,dirbuf->time,3);

	return file;
}

int CCAR::OptimizeFreeSpace(BYTE bankno)
{
	SortFiles(2);

	CCAREntryList curList;

	int from, to;

	if (bankno==0xFF)	// all
	{
		for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
		{
			CCAREntry* file = *iter;
			file->bank = 0xFF;
		}
		from=0;
		to=GetMaxBank();
		return 0;
	}
	else
	{
		from=to=bankno;
	}

	for (int i=from; i<=to; i++)
	{
		Iterations = 0;
		//		MinGap = GetBankCapacity(i);
		MinGap = GetBankFree(i);
		OptimalList.clear();
		curList.clear();

		FillGap(MinGap,&m_Files, &curList);

		//		sOptimal.Format("Free space for bank %d is %d bytes",i, MinGap);
		//		AfxMessageBox(sOptimal,MB_OK | MB_ICONINFORMATION);

		for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
		{
			CCAREntry* file = *iter;
			file->bank = i;
		}

	}

	return 0;
}

int CCAR::FillGap(unsigned int GapSize, CCAREntryList* FileList, CCAREntryList* CurrList)
{
	Iterations++;

	if (!(FileList->size())) return 0;

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;

		if ((file->bank == 0xFF) && (file->length<=GapSize))
		{
			CCAREntryList PathList;
			PathList.clear();
			PathList.insert(PathList.end(), CurrList->begin(), CurrList->end());
			PathList.push_back(file);

			GapSize = GapSize-(file->length);		// nowa mniejsza dziura
			if (GapSize < MinGap)					// mamy nowe minimum?
			{
				MinGap = GapSize;
				OptimalList.clear();
				OptimalList.insert(OptimalList.end(), PathList.begin(), PathList.end());
			}
			//			if (MinGap<10)	return 1;

			// Wype³nij dziurê pozosta³ymi plikami
			CCAREntryList sonFileList;
			sonFileList.insert(sonFileList.end(), FileList->begin(), FileList->end());
			sonFileList.remove(file);

			FillGap(GapSize,&sonFileList, &PathList);
		}

	}

	return 1;
}

int CCAR::GetUnallocatedBytes(void)
{
	int total = 0;

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;
		if (file->bank == 0xFF)
			total += file->length;
	}

	return total;
}

CCAREntry* CCAR::GetCAREntry(std::string name)
{
	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;

		if (file && (!file->name.compare(name)))
		{
			return file;
		}
	}

	return NULL;
}

void CCAR::PrintFiles()
{
	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;
		cout << file->GetEntryText() << endl;
	}
}

bool CCAR::InsertKernel(std::string kernelfile, int maxbanks)
{
	bool res = false;

	CCAR kernelCart(expert, silent);

	std::ifstream insfile(kernelfile.data(), std::ifstream::binary);

	if( !insfile.good() )
	{
		std::cerr << "Cannot open the kernel image !" << endl;
		return false;
	}

	BYTE buf[0x2000];

	// read header to recognize the type and set new type
	insfile.read((char*)buf, 3);
	if (memcmp(buf, "SDX", 3))
		this->SetType(2, true);
	else
		this->SetType(1, true);

	insfile.close();


	if (!kernelCart.OpenRom(kernelfile, false))
	{
		std::cerr << "Bad kernel file !" << endl;
		return false;
	};

	if ((kernelCart.GetBankCapacity(0) < (this->GetBankCapacity(0) - this->GetBankFree(0))) && limit8k)
	{
		std::cerr << "Kernel bank 0 does not fit !" << endl;
		return false;
	}

	if (kernelCart.GetBankCapacity(3) < (this->GetBankCapacity(3) - this->GetBankFree(3)))
	{
		std::cerr << "Kernel bank 3 does not fit !" << endl;
		return false;
	}


	if (maxbanks == 0)	// new cart size not specified, so make it same as the reference
	{
		if (this->GetMaxBank() != kernelCart.GetMaxBank())
		{
			BYTE* newRomFile = new BYTE[kernelCart.GetCartSize()];
			memcpy(newRomFile, m_hRomFile, this->GetCartSize());

			this->SetMaxBank(kernelCart.GetMaxBank());

			if (m_hRomFile)
				delete[] m_hRomFile;
			m_hRomFile = newRomFile;

			if (!silent)
				cout << "Image size set to " << (this->GetCartSize())/1024 << "kB." << endl;
		}
	}

	// Remove files that don't fit
	CCAREntry* file = NULL;
	int idx = 0;
	while((file = this->GetCAREntry(idx++)))
	{
		if (file->bank > this->GetMaxBank())
			this->RemoveFile(file);
	}

	MemReaderWriter kernelImage(kernelCart.m_hRomFile, kernelCart.GetCartSize());
	MemReaderWriter updatedImage(m_hRomFile, GetCartSize());

	// copy bank 0
	int bytesToCopy = 0x2000 - kernelCart.GetBankCapacity(0);
	kernelImage.Seek(kernelCart.GetBankOffset(0));
	kernelImage.Read(buf, bytesToCopy);
	updatedImage.Seek(GetBankOffset(0));
	updatedImage.Write(buf, bytesToCopy);
	this->m_Bank0Area = kernelCart.GetBankCapacity(0);

	// copy bank 1
	kernelImage.Seek(kernelCart.GetBankOffset(1));
	kernelImage.Read(buf,0x2000);
	updatedImage.Seek(GetBankOffset(1));
	updatedImage.Write(buf,0x2000);

	// copy bank 3
	kernelImage.Seek(kernelCart.GetBankOffset(3));
	kernelImage.Read(buf,0x2000);
	updatedImage.Seek(GetBankOffset(3));
	updatedImage.Write(buf,0x2000 - kernelCart.GetBankCapacity(3));
	this->m_Bank3Area = kernelCart.GetBankCapacity(3);

	if (!silent)
	{
		cout << "Kernel updated from: " << kernelfile << endl;
	}

	return true;
}


CCARFile* CCAR::InsertFile(std::string filename, std::string newName, unsigned char status, bool forceUpdate)
{
	CCARFile* file = new CCARFile();

	file->status = status;

	if (newName.empty())
		newName = filename;

	file->name = CCAREntry::ToDirEntryName(newName);

	file->start = 0;

	struct stat st;
	if ( stat (filename.data(), &st) )
	{
		std::cerr << "File not found: " << filename << " !" << endl;
		return NULL;
	}

	struct tm      *tm;
	tm = localtime(&st.st_mtime);

	file->date[0] = tm->tm_mday;
	file->date[1] = tm->tm_mon + 1;
	file->date[2] = tm->tm_year % 100;

	file->time[0] = tm->tm_hour;
	file->time[1] = tm->tm_min;
	file->time[2] = tm->tm_sec;

	file->length = (int)st.st_size;

	// read the file
	std::ifstream insfile(filename.data(), std::ifstream::binary);

	if( !insfile.good() )
	{
		std::cerr << "Cannot open the file " << filename << " !" << endl;
		return NULL;
	}

	file->contents  = new BYTE[file->length];

	insfile.read((char*)file->contents,file->length);

	if (!file->name.compare("CONFIG  SYS"))
	{
		if (file->contents[(int)file->length-1] != 0x9B)
		{
			file->contents = (BYTE*)realloc(file->contents, (int)file->length+1);
			file->contents[file->length] = 0x9B;	// force EOL at the end of CONFIG.SYS
			file->length = UINT3((int)(file->length)+1);
#ifdef USER_MODE
#else
			if (verbose)
				cout << "Extra EOL will be added to CONFIG.SYS." << endl;
#endif
		}
	}

	insfile.close();

	CCAREntry *oldfile = NULL;	
	bool updated = false;

	if ((oldfile = GetCAREntry(file->name)))	// file exists
	{

		if (!expert)
		{

			if (!IsUserBank(oldfile->bank))
			{
				std::cerr << "File " << file->name << " already exists !" << endl;
				return NULL;
			}

		}

		if (!silent)
		{
			cout << "File " << file->name << " already exists ! Do you want to replace it ? ";

			if (!AnswerYes())
				return NULL;
		}


		long date1 = 0, date2 = 0, time1 = 0, time2 = 0;

		int i;

		for (i=2; i>=0; i--)
		{
			date1 += file->date[i] << i*8;
			date2 += oldfile->date[i] << i*8;
		}

		if (date1 < 0x460000)
			date1 += 0x20000000;	// dodaæ 2000 jeœli rok < 70

		if (date2 < 0x460000)
			date2 += 0x20000000;	// dodaæ 2000 jeœli rok < 70

		for (i=2; i>=0; i--)
		{
			time1 += file->time[i] << (2-i)*8;
			time2 += oldfile->time[i] << (2-i)*8;
		}

		// has content changed ?
		if (file->length == oldfile->length)
		{			
			CCARFile* oldfile_f = (CCARFile*)oldfile;

			if (oldfile_f != NULL)
			{
				if (memcmp(file->contents, oldfile_f->contents, file->length) == 0) // same contents
				{
					if (date1 == date2 && time1 == time2)
					{
						if (!silent)
							cout << "File " << file->name << ", no change." << endl;
						return file;
					}
					else
					{
						if (!silent)
						{
							cout << "Do you want only to update timestamp of " << file->name << " ? ";

							if (!AnswerYes())
								return NULL;
						}

						if (forceUpdate)
						{
							oldfile_f->date = file->date;
							oldfile_f->time = file->time;

							if (!silent)
							{
								cout << "File " << file->name << " timestamp updated to " << string(oldfile_f->date) << " " << string(oldfile_f->time) << "." << endl;
							}
						}
						return oldfile_f;
					}
				}
				else	// files differ
				{

					if ((date1 < date2) || ((date1 == date2) && (time1 < time2)))	// newer ?
					{
						if (!silent)
						{
							cout << "Do you want to replace the newer file " << file->name << "? ";

							if (!AnswerYes())
								return NULL;
						}
						else
						{
							if (!forceUpdate)
							{
								if (!silent)
									cout << "File " << filename << " is older (" << string(file->time) << " " << string(oldfile->time) << "), skipping." << endl;
								return file;
							}
						}
					}

				}
			}
								
		}

		file->bank = oldfile->bank;

		if (file->status != oldfile->status && !silent)
			cout << "File " << file->name << " status set to " << std::hex << std::uppercase << file->status << "." << endl;
		RemoveFile(oldfile);

		updated = true;
	}

	if (limit8k)
	{
		if (((int)file->length > (0x2000-16)) || ((int)file->length > GetFreeBytes()))
		{
			std::cerr << "File " << file->name << " is too large !" << endl;
			return NULL;
		}
	}
	else
	{
		if (((int)file->length > 65535) || ((int)file->length > GetFreeBytes()))
		{
			std::cerr << "File " << file->name << " is too large !" << endl;
			return NULL;
		}
	}

	// gdy tryb uzytkownika, a plik sie nie zmiesci w user bank to blad

	if (!expert)
	{
		if (limit8k)
		{
			file->bank = 0xFF;

			for (int bank=0; bank<=GetMaxBank(); bank++)
			{
				if (!IsUserBank(bank))
					continue;
				if (file->length <= GetBankFree(bank))
				{
					file->bank = bank;
					break;
				}
			}
			
			if (file->bank == 0xFF)
			{
				std::cerr << "No room for the file " << file->name << " in user bank !" << endl;
				return NULL;
			}
		}
		else
		{			
			if ((int)file->length > GetFreeBytes())
			{
				std::cerr << "No room for the file " << file->name << " !" << endl;
				return NULL;
			}
		}

		if (GetBankFree(2) < CARENTRYLEN)
		{
			std::cerr << "No room for directory entry for file " << filename << " !" << endl;
			return NULL;
		}

	}
	else
	{
		if (limit8k)
		{
			// manual bank selection in expert mode
			if ((file->bank==0xFF) || (GetBankFree(file->bank) < (int)file->length))
			{
				int bankno = 0xFF;

				int bestfit = GetBankCapacity(bankno) + 1;

				std::ostringstream osr;

				bool bankavail = false;

				for(int i=0; i <= GetMaxBank(); i++)
				{

					int roomleft = (int)GetBankFree(i) - (int)file->length;

					if (i==2)
						roomleft-=CARENTRYLEN;		// bank 2 - directory

					if (roomleft >= 0)	//enough room ?
					{
						if (roomleft < bestfit)
						{
							bankno = i;
							bestfit = roomleft;
						}

						osr << setw(2) << std::hex << std::uppercase << i << " " << (IsUserBank(i) ? "(User) " : " ") << "\t";
						bankavail = true;

					}
				}

				if (bankno == 0xFF)
				{
					std::cerr  << "No bank to fit the file " << file->name << " !"  << endl;
					return NULL;
				}
				else
				{
					if (!silent)
					{
						cout << "Available banks for file " << file->name << ": "  << endl << osr.str() << endl;
						cout << "Enter bank number (default " << setw(2) << std::hex << std::uppercase << bankno << ") : ";

						std::string sbank;
						cin >> sbank;
						if (!sbank.empty())
						{
							std::stringstream ss;
							ss << std::hex << sbank;
							ss >> bankno;
						}
					}
				}

				if (file->bank != bankno && !silent)
					cout << "File " << file->name << " bank set to " << std::hex << std::uppercase << bankno << "." << endl;

				file->bank=bankno;
			}
		}
		else
		{
			// If 8k is no longer the limit, we do not set the bank here
			// This will be done when the directory is constructed 
			if ((int)file->length > GetFreeBytes() + CARENTRYLEN)
			{
				std::cerr << "No room for the file " << file->name << " !" << endl;
				return NULL;
			}

		}
	}

	if (!silent)
	{
		if (updated)
			cout << "File " << file->name << " updated." << endl;
		else
			cout << "File " << file->name << " added to the image." << endl;
	}

	m_Files.push_back(file);

	return file;

}



bool CCAR::AddFromList(std::string listfile)
{

	// read the file
	std::ifstream insfile(listfile.data());

	if( !insfile.good() )
	{		
		std::cerr << "Cannot open file list from " << listfile << " !" << endl;
		return false;
	}

	if (!silent)
		cout << "Processing file list: " << listfile << endl;

	std::string line;
	while (std::getline(insfile, line))
	{
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());	// for Linux

		if (line.empty())
			continue;

		if (line.at(0) == ';')	// just comment
			continue;

		if (line.at(0) == '-')	// remove the file
		{
			if (!RemoveFile(line.substr(1,11)))
				return false;
			continue;
		}

		if (line.find("#include")==0)	// include other list
		{
			if (!AddFromList(line.substr(9)))
				return false;
			continue;
		}


		if (line.at(0) == '+')	// add file ("+" is optional)
		{
			line = line.substr(1);	// skip +, start processing from the second char
		}

		int curTok = 0;
		int iBank = -1;
		unsigned char bStatus = 0x08;
		bool forceUpdate = false;

		std::string ifilename;
		std::string inewfilename;

		//boost::char_separator<char> sep(",","",boost::keep_empty_tokens);
		//boost::tokenizer< boost::char_separator<char> > tokens(line, sep);

		//BOOST_FOREACH(std::string t, tokens)
		std::istringstream ss(line);
		std::string t;

		while (std::getline(ss, t, ','))
		{
			switch(curTok)
			{
			case 0:		// filename
				ifilename = t;
				inewfilename = "";
				break;
			case 1:
				if (!t.empty())
				{
					inewfilename = t;
				}
				break;
			case 2:		// status
				if (!t.empty())
				{
					if (t[0]=='$')
						bStatus = strtol(t.substr(1).c_str(), 0, 16); //std::stoi(t.substr(1),0,16);
					else
						bStatus = strtol(t.c_str(), 0, 10);  //std::stoi(t,0,10);
				}
				break;
			case 3:		// bank
				if (toupper(t[0]) == 'U')
					iBank = -1;
				else
				{
					if (!t.empty())
						iBank = atoi(t.c_str());
				}
				break;
			case 4:		// force update
				if (toupper(t[0] == 'T'))
					forceUpdate = true;
				break;
			}

			curTok++;

		};  


		if (!InsertFile(ifilename, inewfilename, bStatus, forceUpdate))
			return false;

	}

	insfile.close();

	if (!silent)
		cout << "File list " << listfile << " processed." << endl;

	return true;
}

bool CCAR::SaveCSV(std::string filename)
{

	ofstream out(filename.data());

	if( !out.good() )
	{
		std::cerr << "Cannot create CSV file !" << endl;
		return false;
	}

	for( std::list<CCAREntry*>::iterator iter=m_Files.begin(); iter != m_Files.end(); iter++ )
	{
		CCAREntry* file = *iter;
		out << file->GetCSVEntry() << endl;
	}

	out.close();

	if (!silent)
		cout << "CSV saved in " << filename << endl;

	return true;

}


void CCAR::PrintInformation()
{

	cout << "----- General info -----" << endl;

	std::string sitem;

	switch(GetType())
	{
	case CART_NORMAL:
		sitem = "SpartaDOS X 4.2x";
		break;
	case CART_MAXFLASH1MB:
		sitem = "Maxflash/Turbo Freezer/SicCart/Ultimate/SIDE";
		break;
	case CART_SDX128:
		sitem = "SpartaDOS X / intSDX";
		break;
	default:
		sitem = GetType();
		break;
	}

	cout << "Type and size: " << sitem << GetCartSize()/1024 << " kB" << endl;

	cout << "Created on: " << (std::string)this->m_DirHead->date << " " << (std::string)this->m_DirHead->time << endl;

	int userfree = 0;

	for (int i=0; i<=GetMaxBank(); i++)
		if (IsUserBank(i))
			userfree += GetBankFree(i);

	cout << "Free bytes in user area (max): " << userfree << endl; 

	cout << "Free directory entries: " << GetBankFree(2) / 23 << endl;

	cout << "----- Statistics -----" << endl;

	cout << "Number of files: " << GetFileCount() << endl;
	cout << "Total capacity (bytes): " << GetCapacity() << endl;
	cout << "Total free bytes: " << GetFreeBytes() << endl;
//	cout << "Unallocated bytes: " << GetUnallocatedBytes() << endl;

	cout << "----- Bank usage -----" << endl;

	for (int i=0; i<=GetMaxBank(); i++)
	{
		cout << "Bank " << std::hex << i << " " << GetBankFree(i) << " of " << GetBankCapacity(i) << endl;
	}

	return;
}

bool CCAR::ExtractFile(std::string filename, std::string newName)
{
	CCARFile* file = (CCARFile*)GetCAREntry(filename);

	if (file == NULL)
	{
		std::cerr << "File not found: " << filename << " !" << endl;
		return false;
	}

	BYTE *buf = new BYTE[file->length];

	std::ostringstream filenamedot;

	std::string fname = filename.substr(0, 8);
	fname.erase(fname.find_last_not_of(" \n\r\t") + 1);

	std::string fext = filename.substr(8, 3);
	fext.erase(fext.find_last_not_of(" \n\r\t") + 1);

	filenamedot << fname << "." << fext;

	ofstream out(filenamedot.str().c_str(), std::ofstream::binary);

	if (!out.good())
	{
		std::cerr << "Cannot save the file " << filename << " !" << endl;
		return false;
	}

	out.write((char*)file->contents, file->length);
	out.close();

	if (!silent)
		cout << "File extracted to " << filenamedot.str() << endl;

	return true;
}

int CCAR::GetNextCarBank(BYTE bankno)
{
	if (bankno == GetMaxBank())
		return -1;

	if (bankno == 0)
		return 2;

	if (bankno == 2)
		return 4;

	return bankno+1;
}
