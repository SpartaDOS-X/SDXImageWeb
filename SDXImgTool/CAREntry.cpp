#include "CAREntry.h"

#include <sstream>
#include <iomanip>
#include <cstring>

//#include <boost/filesystem.hpp>
//#include <boost/algorithm/string.hpp>

CCAREntry::CCAREntry(void)
{
	directory_position = 0;
}

CCAREntry::~CCAREntry(void)
{
}



std::string CCAREntry::GetEntryText()
{
	std::string atr;

	char atrchr[]="pha dsxo";

	for (BYTE i=0; i<8; i++)
	{
		if (status & (1<<i))
			atr += atrchr[i];
	}

	std::ostringstream sentry;

	sentry << std::uppercase;
	sentry << std::setw(1) << std::hex << (int)bank;
	sentry << ' ';
	sentry << std::setw(11) << name;
	sentry << ' ';
	sentry << std::left << std::setw(4) << atr;
	sentry << ' ';
	sentry << std::dec << std::setw(6) << (int)length;
	sentry << ' ';
	sentry << (std::string)date;
	sentry << ' ';
	sentry << (std::string)time;

	//Format("%1X %11s %-4s %6d %2d-%02d-%02d %02d:%02d:%02d",

	return sentry.str();

}


std::string CCAREntry::GetCSVEntry(void)
{
	std::ostringstream sentry;

	sentry << std::uppercase;
	sentry << std::setw(11) << name << ',' << (int)length << ',' << (int)bank << ',' << (std::string)date << "," << (std::string)time;

	//Format("%11s,%d,%d,%2d-%02d-%02d,%02d:%02d:%02d\n",name, length, bank,

	return sentry.str();
}

void CCAREntry::GetSDXEntry(CSDXEntry &sdxentry)
{

		sdxentry.start = this->start_img | ((WORD)this->bank << 0xD);

		memcpy(&(sdxentry.length), (BYTE*)this->length, 3); 

		if (this->bank > 7)
			sdxentry.length[2] = this->bank;
		else
			sdxentry.length[2] = 0;

//		sdxentry.length[2] = file->length / 0xFFFF;

		sdxentry.status = this->status;

		memcpy(sdxentry.name,this->name.data(),11);
		memcpy(sdxentry.date,(BYTE*)this->date,3);
		memcpy(sdxentry.time,(BYTE*)this->time,3);

}

std::string CCAREntry::ToDirEntryName(std::string filename)
{
	//boost::filesystem::path p(filename);

	std::ostringstream osr;

	size_t sep = filename.find_last_of("\\/");
	if (sep != std::string::npos)
		filename = filename.substr(sep + 1, filename.size() - sep - 1);

	for (std::string::iterator p = filename.begin(); filename.end() != p; ++p)
		*p = toupper(*p);

	std::string ex;

	size_t ext_pos = filename.find_last_of(".");

	if (ext_pos != std::string::npos)
	{
		ex = filename.substr(ext_pos + 1, 3);
	}

	std::string fn = filename.substr(0, std::min((int)ext_pos,8));

	//ex.erase(std::remove(ex.begin(), ex.end(), '.'), ex.end());

	osr  << std::setw(8) << std::left << fn  << std::setw(3) <<  std::left << ex;

	return osr.str();

}