#if !defined(AFX_CAR_H__E54DD6DA_CA39_45A5_9E8D_5E0C865B9112__INCLUDED_)
#define AFX_CAR_H__E54DD6DA_CA39_45A5_9E8D_5E0C865B9112__INCLUDED_

#include <list>

#include "CAREntry.h"
#include "CARFile.h"
#include "CARLink.h"
#include "MemReaderWriter.h"
//#include "Logger.h"

#define CARENTRYLEN			sizeof(CSDXEntry)
#define CART_NORMAL			0
#define CART_MAXFLASH1MB	1
#define CART_SDX128         2
//#define CART_MAXFLASH8MB	3
//#define CART_SDX256         4

//#define BANK0_FREESPACE		0xD00

typedef std::list<CCAREntry*> CCAREntryList;

struct SdxVersionInfo
{
	unsigned char version;
	unsigned char hwid;
};

class CCAR
{
public:
	int GetBankOffset(BYTE bankno);
	BYTE* m_hRomFile;

	WORD GetBankStartOfs(BYTE bankno, bool includeDirectory = true);
	WORD GetBankCapacity(BYTE bankno);
	WORD GetBankFree(BYTE bankno);
	bool IsUserBank(BYTE bankno);
	void DeleteAllFiles();
	int GetFreeBytes();
	int GetCapacity();
	int GetTotalBytes();

//	CObList m_Files;
	CCAREntryList m_Files;

	CCAREntry* m_DirHead;
	CCAR(bool isexpert, bool issilent, bool isverbose = true);
	virtual ~CCAR();

	bool OpenRom(std::string romfile, bool bFiles = true);
	bool SaveImage(std::string romfile);

	void SortFiles(int sort_order=0);
	bool CompareAndSwap(int pos, int sort_order);

	int GetDirectoryOfs(void);

	BYTE GetMaxBank(void);
	BYTE SetMaxBank(int maxbank);

	int GetCartSize(void);
	int GetFileCount(void);
	int MoveToTop(int file_idx);
	int MoveToTop(CCAREntry* entry);
	bool RemoveFile(CCAREntry* file);
	bool RemoveFile(std::string filename);

	CCAREntry* GetCAREntry(int file_idx);
    CCAREntry* GetCAREntry(std::string name);
	CCAREntry* EntryFromBuf(CSDXEntry* dirbuf);
	int OptimizeFreeSpace(BYTE bankno = 0xFF);
	int FillGap(unsigned int GapSize, CCAREntryList* FileList, CCAREntryList* CurrList);
	int GetUnallocatedBytes(void);
	bool AllocateBank(BYTE bankno, MemReaderWriter& outfile);

	bool InsertKernel(std::string kernelfile, int maxbanks = 0);
	CCARFile* InsertFile(std::string filename, std::string newName = "", unsigned char status = 0x08, bool forceUpdate = false);
	bool AddFromList(std::string listfile);
	bool ExtractFile(std::string filename, std::string newName = "");

	int GetType() { return m_Type; }
	int SetType(int nType, bool bSaveMaxBank = 0);

	WORD m_Bank0Area, m_Bank3Area;

	void PrintFiles();
	void PrintInformation();
	bool SaveCSV(std::string filename);

	bool limit8k;

	SdxVersionInfo* GetSdxVersionInfo();

private:

	int GetNextCarBank(BYTE bankno);
	int GetDirectoryBank();
	int GetDirectorySize();

	bool ReadFileFromRom(MemReaderWriter& memfile, CCARFile* cfile);
	bool SaveFileToRom(MemReaderWriter& outfile, CCARFile* cfile, int& bank, int& bankofs);

		// Cartridge type (0-normal, 1-maxflash 1mb, ...)
	int m_Type;
	// Maximum bank number or 0 if default
	BYTE m_MaxBank;

	bool expert;
	bool silent;
	bool verbose;
	
	SdxVersionInfo sdxVersionInfo;
};

#endif // !defined(AFX_CAR_H__E54DD6DA_CA39_45A5_9E8D_5E0C865B9112__INCLUDED_)
