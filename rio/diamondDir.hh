/* 
 * File:   diamondDir.hh
 * Author: apeters
 *
 * Created on October 15, 2014, 4:09 PM
 */

#ifndef DIAMONDDIR_HH
#define	DIAMONDDIR_HH

#include "rio/Namespace.hh"
#include "rio/diamond_types.hh"
#include "rio/diamondMeta.hh"

DIAMONDRIONAMESPACE_BEGIN

class diamondDir : public diamondMeta, public std::set<diamond_ino_t> {
public:
  diamondDir () : diamondMeta() { }
  diamondDir (const diamond_ino_t ino, const std::string name);
  diamondDir (const diamondDir& orig);
  diamondDir (diamondDir* orig);
  virtual ~diamondDir ();
  
  std::map<std::string, diamond_ino_t>& getNamesInode() { return mNamesInode; }

private:
  std::map<std::string, diamond_ino_t> mNamesInode;
};

DIAMONDRIONAMESPACE_END

#endif	/* DIAMONDDIR_HH */

