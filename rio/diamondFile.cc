/* 
 * File:   diamondFile.cc
 * Author: apeters
 * 
 * Created on October 15, 2014, 4:09 PM
 */

#include "diamondFile.hh"

DIAMONDRIONAMESPACE_BEGIN

diamondFile::diamondFile (const diamond_ino_t ino, const std::string name) : diamondMeta::diamondMeta(ino, name) { }

diamondFile::diamondFile (const diamondFile& orig) : diamondMeta::diamondMeta(orig) { }

diamondFile::diamondFile (diamondFile* orig) : diamondMeta::diamondMeta(orig) { }

diamondFile::~diamondFile () { }

int 
diamondFile::read(char* buffer, off_t offset, size_t size)
{
  return mContents.readData(buffer, offset, size);
}

off_t
diamondFile::write(const char* buffer, off_t offset, size_t size)
{
  mStat.st_size = mContents.writeData(buffer, offset, size);
  mStat.st_blocks = (mStat.st_size+mStat.st_blksize-1) / mStat.st_blksize;
  return mStat.st_size;
}

int 
diamondFile::peek(char* &buffer, off_t offset, size_t size)
{
  return mContents.peekData(buffer, offset, size);
}

void
diamondFile::release()
{
  return mContents.releasePeek();
}

int
diamondFile::truncate(off_t offset)
{
  mContents.truncateData(offset);
  return 0;
}
DIAMONDRIONAMESPACE_END
