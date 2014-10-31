#ifndef DIAMONDTYPES_HH
#define DIAMONDTYPES_HH

#include <string>

typedef std::string diamond_ino_t ;

//#define DIAMOND_INODE(x) (x)
#define DIAMOND_INODE(x) std::to_string((x))
#define DIAMOND_TO_INODE(x) strtoull((x).c_str(), 0, 10)
#endif

