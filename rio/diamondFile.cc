/*
 * File:   diamondFile.cc
 * Author: apeters
 *
 * Created on October 15, 2014, 4:09 PM
 */

#include "diamondFile.hh"
#include "diamondCache.hh"
#include "common/Logging.hh"
#include "common/Timing.hh"

DIAMONDRIONAMESPACE_BEGIN

diamond::common::ThreadPool diamond::rio::diamondFile::gIO_CommitPool;
diamond::common::ThreadPool diamond::rio::diamondFile::gIO_PrefetchPool;

diamondFile::diamondFile(const diamond_ino_t ino, const std::string name) : diamondMeta::diamondMeta(ino, name)
{
  mCommitMutex.SetBlocking(true);
}

diamondFile::diamondFile(const diamondFile& orig) : diamondMeta::diamondMeta(orig)
{
}

diamondFile::diamondFile(diamondFile* orig) : diamondMeta::diamondMeta(orig)
{
}

diamondFile::~diamondFile()
{
}

int
diamondFile::read(char* buffer, off_t offset, size_t size)
{
  return mContents.readData(buffer, offset, size);
}

off_t
diamondFile::write(const char* buffer, off_t offset, size_t size)
{
  mStat.st_size = mContents.writeData(buffer, offset, size);
  mStat.st_blocks = (mStat.st_size + mStat.st_blksize - 1) / mStat.st_blksize;

  bool do_schedule = false;
  {
    {
      diamond::common::RWMutexWriteLock fLock(mCommitMutex);
      if (!mCommits.size())
        do_schedule = true;

      if (mCommits.count(offset)) {
        if (size > mCommits[offset])
          mCommits[offset] = size;
      } else {
        mCommits[offset] = size;
      }
    }
    if (do_schedule) {
      diamond_ino_t inode = mIno;
      struct timespec then;
      diamond::common::Timing::GetTimeSpec(then);
      gIO_CommitPool.Enqueue([inode, then]()
      {
        // delay the processing by 50ms
        struct timespec now;
        diamond::common::Timing::GetTimeSpec(now);
          int delay = ((now.tv_sec - then.tv_sec)*1000) + ((now.tv_nsec - then.tv_nsec) / 1000);
          diamond_static_info("msg=\"enqueue\" ino=%08x", inode);
        if ((delay < 50) && (delay > 0))
          diamond::common::Timing::Wait(50 - delay);
          //gBackend.datasync(mIno)
        });
    }
  }

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
