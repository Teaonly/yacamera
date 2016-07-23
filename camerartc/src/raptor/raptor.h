#ifndef _TEAONLYRAPTOR_H_
#define _TEAONLYRAPTOR_H_

#include <bitset>
#include <vector>

struct RaptorParameter {
    unsigned int k;
    unsigned int x;
    unsigned int s;
    unsigned int h;
    unsigned int hp;
    unsigned int l;
    unsigned int lp;
    unsigned int sindex;
};

class RaptorSymbol {
public:
    int esi; 
    std::vector<unsigned char> data;
};

class RaptorTriple {
public:   
    unsigned int d;
    unsigned int a;
    unsigned int b;
};


class TeaonlyRaptor {
public:    
    static struct RaptorParameter createRaptorParameter(unsigned int kSymbles);
    
public:
    TeaonlyRaptor(struct RaptorParameter& param);

    unsigned int K() {
        return k_;
    }
    bool doEncode(unsigned char *payload, unsigned int length);
    bool doDecode(std::vector<RaptorSymbol>& symbols);
    bool getSymbol(RaptorSymbol* symbol);

private:
    void createMatrixA(const std::vector<RaptorSymbol>& symbols);
    bool resolveMatrixA(std::vector<std::vector<unsigned int> >& schdule);
    void createIntermediateSymbols(std::vector<RaptorSymbol>& symbols,
                                   const std::vector<std::vector<unsigned int> >& schdule);
    void dumpA();
    
    RaptorTriple tripleGenerator(unsigned int x);
    void ldpc(std::vector<std::vector<unsigned int> >& xorMatrix);
    void half(std::vector<std::vector<unsigned int> >& xorMatrix);
    void lt(unsigned  int esi, std::vector<unsigned int>& xorArray);
    
    template <class T>
    void xorRow(std::vector<T>& row1, std::vector<T>& row2);

private:
    const unsigned int k_;        //Integer number of source symbols
    const unsigned int x_;        //Let X be the smallest positive integer such that X*(X-1) >= 2*K.
    const unsigned int s_;        //Let S be the smallest prime integer such that S >= ceil(0.01*K) + X
    const unsigned int h_;        //Let H be the smallest integer such that choose(H,ceil(H/2)) >= K + S
    const unsigned int hp_;       //Let H' be ceil(H/2)
    const unsigned int l_;        //Let L be k + s + h
    const unsigned int lp_;       //Let L' be the smallest prime number such that L' >= L
    const unsigned int sindex_;   //Choose a systematic index based upon k. 

    std::vector<std::vector<unsigned char> > matrixA_;
    
    std::vector<RaptorSymbol>   iSymbols_;
};

#endif
