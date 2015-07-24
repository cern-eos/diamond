/*
 * File:   diamondFile.hh
 * Author: apeters
 *
 * Created on October 15, 2014, 4:09 PM
 */

#ifndef DIAMONDFILE_HH
#define	DIAMONDFILE_HH

#include <string>
#include "rio/Namespace.hh"
#include "rio/diamond_types.hh"
#include "rio/diamondMeta.hh"
#include "common/threadpool/ThreadPool.hh"

DIAMONDRIONAMESPACE_BEGIN
class diamondFile : public diamondMeta {
public:
  typedef std::map<off_t, size_t> io_map_t;

  diamondFile () : diamondMeta () { }
  diamondFile (const diamond_ino_t ino, const std::string name);
  diamondFile (const diamondFile& orig);
  diamondFile (diamondFile* orig);
  virtual ~diamondFile ();
  off_t write (const char* buffer, off_t offset, size_t size);
  int read (char* buffer, off_t offset, size_t size);
  int peek (char* &buffer, off_t offset, size_t size);
  int truncate (off_t offset);
  void release ();

  static diamond::common::ThreadPool gIO_CommitPool;
  static diamond::common::ThreadPool gIO_PrefetchPool;

  diamond::common::RWMutex mCommitMutex;
  io_map_t mCommits;

private:
  diamond::common::Bufferll mContents;
};

DIAMONDRIONAMESPACE_END

#endif	/* DIAMONDFILE_HH */

