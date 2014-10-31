/* 
 * File:   diamondDir.cc
 * Author: apeters
 * 
 * Created on October 15, 2014, 4:09 PM
 */

#include "diamondDir.hh"

DIAMONDRIONAMESPACE_BEGIN

diamondDir::diamondDir (const diamond_ino_t ino, const std::string name) : diamondMeta::diamondMeta(ino, name) { }

diamondDir::diamondDir (const diamondDir& orig) : diamondMeta::diamondMeta(orig) { }

diamondDir::diamondDir (diamondDir* orig) : diamondMeta::diamondMeta(orig) { mNamesInode = orig->getNamesInode(); }

diamondDir::~diamondDir () { }

DIAMONDRIONAMESPACE_END
