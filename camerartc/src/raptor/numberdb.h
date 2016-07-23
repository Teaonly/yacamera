#ifndef _NUMBERDB_H_
#define _NUMBERDB_H_

#include <vector>

class PrimesDB {
public:
    static unsigned int getNext(unsigned int k);
};

class HalfDB {
public:
    static unsigned int choose(unsigned int n, unsigned int k);
    static unsigned int getNext(unsigned int k);
};

class SystematicDB {
public:
    static unsigned int get(unsigned int k);
};

class DegreeDB {
public:
    static unsigned int get(unsigned int v);
};

class GrayDB {
public:
    GrayDB(unsigned int hp);
    unsigned int get(unsigned int n);

private:    
    unsigned int countOneBits(unsigned int v);
    std::vector<unsigned int> db_;
};

#endif
