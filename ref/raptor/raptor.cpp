#include <assert.h>
#include <math.h>
#include <iostream>

#include "raptor.h"
#include "numberdb.h"
#include "rprandom.h"

const unsigned int MIN_K = 4;
const unsigned int MAX_K = 8192;

struct RaptorParameter TeaonlyRaptor::createRaptorParameter(unsigned int kSymbles) {
    struct RaptorParameter param;
    
    assert(kSymbles >= MIN_K && kSymbles <= MAX_K);
    param.k = kSymbles;
    
    //Let X be the smallest positive integer such that X*(X-1) >= 2*K.
    //a = 1, b =-1 c = -2(k)
    //quadratic = ((b * -1) + math.sqrt((b * b) - (4 * a * c))) / (2 * a)
    param.x = ceil((1.0 + sqrt(1.0 - (-8.0 * param.k))) / 2.0);
    
    // Let S be the smallest prime integer such that S >= ceil(0.01*K) + X
    param.s = PrimesDB::getNext( ceil(0.01*param.k) + param.x);
    
    // Let H be the smallest integer such that choose(H,ceil(H/2) ) >= K + S
    param.h = HalfDB::getNext(param.k + param.s);

    // Let H' be ceil(H/2)
    param.hp = ceil(param.h*0.5);
   
    // Let L be k + s + h 
    param.l = param.k + param.s + param.h;

    // Let L' be the smallest prime number such that L' >= L
    param.lp = PrimesDB::getNext(param.l);      
    
    //Choose a systematic index based upon k.
    param.sindex = SystematicDB::get(param.k);

    return param;    
}

TeaonlyRaptor::TeaonlyRaptor(struct RaptorParameter& param)
        : k_(param.k), x_(param.x), s_(param.s), h_(param.h), hp_(param.hp),
          l_(param.l), lp_(param.lp), sindex_(param.sindex) {  
#if 0
    std::cout << "K = " << k_ << std::endl;
    std::cout << "X = " << x_ << std::endl;          
    std::cout << "S = " << s_ << std::endl;          
    std::cout << "H = " << h_ << std::endl;          
    std::cout << "HP = " << hp_ << std::endl;          
    std::cout << "L = " << l_ << std::endl;          
    std::cout << "LP = " << lp_ << std::endl;          
    std::cout << "sindex = " << sindex_ << std::endl;          
#endif

    matrixA_.clear();
    iSymbols_.clear();
}

bool TeaonlyRaptor::doEncode(unsigned char *payload, unsigned int length) {
    unsigned int blockSize = (length + k_ - 1) / k_;
    if ( blockSize < 1)
        return false;
    
    // Create orignal source symbols 
    std::vector<RaptorSymbol>  sourceSymbols;
    int reserveBlockSize = blockSize + 8 - blockSize % 8;
    for(unsigned int i = 0; i < k_-1; i++ ) {
        RaptorSymbol s;
        s.esi = i;
#if 1 
        s.data.reserve(reserveBlockSize);
        for (int j = 0; j < blockSize; j++) {
            s.data.push_back( payload[i*blockSize +j] );    
        }
#else
        s.data.reserve(reserveBlockSize);
        std::copy(&payload[i*blockSize], &payload[i*blockSize+blockSize], std::back_inserter(s.data));
#endif
        sourceSymbols.push_back(s);
    }
    RaptorSymbol s;
    s.data.reserve(reserveBlockSize);
    s.esi = k_ - 1;
    for ( int j = 0; j < blockSize; j++) {
        unsigned int pos = (k_-1) * blockSize + j;
        if( pos < length) {
            s.data.push_back( payload[pos] );
        } else {
            s.data.push_back(0);            //padding data
        }
    } 
    sourceSymbols.push_back(s);
    
    createMatrixA(sourceSymbols);
    
    std::vector<std::vector<unsigned int> > schdule;
    if ( resolveMatrixA(schdule) == false) {
        return false;
    }
    
    // Create intermediate symbols according to :matrixA_, schdule, source symbols
    createIntermediateSymbols(sourceSymbols, schdule);
    
    return true;
}

bool TeaonlyRaptor::doDecode(std::vector<RaptorSymbol>& symbols) {
    if ( symbols.size() < k_ ) {
        return false;
    } 

    createMatrixA(symbols);

    std::vector<std::vector<unsigned int> > schdule;
    bool ret = resolveMatrixA(schdule);
    //dumpA();
    if( ret == false ) {
        return false;
    }
    
    // Create intermediate symbols according to :matrixA_, schdule, source symbols
    createIntermediateSymbols(symbols, schdule);

    return true;
}

bool TeaonlyRaptor::getSymbol(RaptorSymbol* symbol) {
    if ( iSymbols_.size() != l_) {
        return false;
    }
    
    std::vector<unsigned int> xors;
    lt(symbol->esi, xors);
    if ( xors.size() == 0) {
        return false;
    }
    
    symbol->data = iSymbols_[xors[0]].data;
    for(int i = 1; i < xors.size(); i++) {
        xorRow( symbol->data, iSymbols_[xors[i]].data);
    }
    
    return true;
}

void TeaonlyRaptor::createIntermediateSymbols(std::vector<RaptorSymbol>& symbols,
                                              const std::vector<std::vector<unsigned int> >& schdule) {
    RaptorSymbol zeroSymbol;
    zeroSymbol.esi = -1;
    for (int i = 0; i < symbols[0].data.size(); i++) {
        zeroSymbol.data.push_back(0);
    }
    
    // Create D matrix according input symbols;
    std::vector<RaptorSymbol> matrixD;
    for(int i = 0; i < s_ + h_; i++) {
        matrixD.push_back(zeroSymbol);
    } 
    for(int i = 0; i < symbols.size(); i++) {
        matrixD.push_back(symbols[i]);
    }
    
    // reorg matrixD according schdule
    for(int i = 0; i < schdule.size(); i++) {
        if ( schdule[i][0] != i) {
            RaptorSymbol temp = matrixD[i];
            matrixD[i] = matrixD[ schdule[i][0] ];
            matrixD[ schdule[i][0] ] = temp;
        }
        int n1 = i;
        for (int j = 1; j < schdule[i].size(); j++) {
            int n2 = schdule[i][j];
            xorRow( matrixD[n2].data, matrixD[n1].data );
        }
    }
    
    // resolve the up triangle matrixA
    iSymbols_.clear();
    for (int i = 0; i < l_; i++) {
        iSymbols_.push_back(zeroSymbol);
    }
    
    iSymbols_[l_-1].esi = l_-1;
    iSymbols_[l_-1].data = matrixD[l_-1].data;
    for (int i = l_-1; i >= 1; i--) {
        int n1 = i;
        for(int j = 0; j <= i - 1; j++) {
            if ( matrixA_[j][i] == 1) {
                int n2 = j;
                xorRow( matrixD[n2].data, matrixD[n1].data );
            }
        }
        iSymbols_[i-1].esi = i-1;
        iSymbols_[i-1].data = matrixD[i-1].data;
    }
    
    
}

void TeaonlyRaptor::createMatrixA(const std::vector<RaptorSymbol>& symbols) {
    
    /*           The matrix A                */
    /*
                  K               S       H
      +-----------------------+-------+-------+
      |                       |       |       |
    S |        G_LDPC         |  I_S  | 0_SxH |
      |                       |       |       |
      +-----------------------+-------+-------+
      |                               |       |
    H |        G_Half                 |  I_H  |
      |                               |       |
      +-------------------------------+-------+
      |                                       |
      |                                       |
    K |                 G_LT                  |
      |                                       |
      |                                       |
      +---------------------------------------+
    */
    
    matrixA_.clear();
    std::vector<unsigned char> zero_row;
    for(int j = 0; j < l_; j++) {
        zero_row.push_back(0);
    }

    std::vector<std::vector<unsigned int> > xorMatrix;
    ldpc(xorMatrix);
    for (int i = 0; i < s_; i++) {
        std::vector<unsigned char> row;
        row = zero_row;
        row[k_ + i] = 1;
        for(int j = 0; j < xorMatrix[i].size(); j++) {
            row[ xorMatrix[i][j]] = 1;
        }
        matrixA_.push_back(row);
    }
    
    half(xorMatrix);
    for (int i = 0; i < h_; i++) {
        std::vector<unsigned char> row;
        row = zero_row;
        row[k_ + s_ + i] = 1;
        for(int j = 0; j < xorMatrix[i].size(); j++) {
            row[ xorMatrix[i][j]] = 1;
        }
        matrixA_.push_back(row);
    }
    
    std::vector<unsigned int> lt_row;
    for(int i=0; i < symbols.size(); i++) {
        std::vector<unsigned char> row;
        row = zero_row;
        lt(symbols[i].esi, lt_row);
        for(int i = 0; i < lt_row.size(); i++) {
            row[lt_row[i]] = 1;
        }
        matrixA_.push_back(row);
    }
    
}


bool TeaonlyRaptor::resolveMatrixA(std::vector<std::vector<unsigned int> >& schdule) {
    schdule.clear();
    
    std::vector<unsigned int> nullOp;
    // Apply Gaussian elimination to Matrix A
    for(int i = 0; i < l_; i++) {  
        std::vector<unsigned int> newOp = nullOp;
        int changeRow = -1;
        for(int j = i; j < matrixA_.size(); j++) {
            if ( matrixA_[j][i] != 0) {
                changeRow = j;
                break;
            } 
        }
        if ( changeRow == -1) {
            return false;
        }
        newOp.push_back(changeRow);
        if ( changeRow != i) {
            std::vector<unsigned char> temp;
            temp = matrixA_[i];
            matrixA_[i] = matrixA_[changeRow];
            matrixA_[changeRow] = temp;
        }

        for(int j = i+1; j < matrixA_.size(); j++) {
            if ( matrixA_[j][i] != 0) {
                xorRow( matrixA_[j], matrixA_[i]);
                newOp.push_back(j);
            }            
        }
        schdule.push_back(newOp);
    }  

    return true;
}

void TeaonlyRaptor::dumpA() {
    std::cout << "<<<<<<<<<<<<<<<<<<<<<" << std::endl;
    for(int i = 0; i < matrixA_.size(); i++) {
        for(int j = 0; j < matrixA_[i].size(); j++) {
            if ( matrixA_[i][j] ) {
                std::cout << "1";
            } else {
                std::cout << "0";
            }
        }
        std::cout << std::endl;
    }
}

void TeaonlyRaptor::ldpc(std::vector<std::vector<unsigned int> >& xorMatrix) {
    xorMatrix.clear();
    std::vector<unsigned int> emptyXor;
    for(unsigned int i = 0; i < s_; i++) {
        xorMatrix.push_back(emptyXor);
    }

    for (unsigned int i = 0; i < k_ ; i++) {
        unsigned int a = 1 + ( int(i*1.0/s_) % (s_-1));
        unsigned int b = i%s_;
        xorMatrix[b].push_back(i);
        b = (b + a) % s_;
        xorMatrix[b].push_back(i);
        b = (b + a) % s_;
        xorMatrix[b].push_back(i);
    }        
}

void TeaonlyRaptor::half(std::vector<std::vector<unsigned int> >& xorMatrix) {
    xorMatrix.clear();
    std::vector<unsigned int> emptyXor;
    for(unsigned int i = 0; i < h_; i++) {
        xorMatrix.push_back(emptyXor);
    }
    
    GrayDB gdb(hp_);
    for(unsigned int i = 0; i < h_; i++) {
        for(unsigned int j = 0; j < k_ + s_; j++) {
            if ( (1 << i) & gdb.get(j) ) {
                xorMatrix[i].push_back(j);
            } 
        }
    }
}


void TeaonlyRaptor::lt(unsigned  int esi, std::vector<unsigned int>& xorArray) {
    xorArray.clear();
    
    RaptorTriple triple = tripleGenerator(esi); 
    unsigned int d = triple.d;
    unsigned int a = triple.a;
    unsigned int b = triple.b;

    while (b >= l_) {
        b = (b + a) % lp_;
    }
    
    xorArray.push_back(b);
    for (int j = 1; (j < d) && (j < l_); j++) {
        b = (b + a) % lp_;
        while (b >= l_) {
            b = (b + a) % lp_;
        }
        xorArray.push_back(b);
    }
}

RaptorTriple TeaonlyRaptor::tripleGenerator(unsigned int x) {
    const unsigned int Q = 65521;
    unsigned int A = (53591 + sindex_*997) % Q;
    unsigned int B = 10267*(sindex_+1) % Q;
    unsigned int Y = (B + x*A) % Q;
    unsigned int v = RaptorRandom::get(Y, 0, 1<<20);
    unsigned int d = DegreeDB::get(v);
    unsigned int a = 1 + RaptorRandom::get(Y, 1, lp_ - 1);
    unsigned int b = RaptorRandom::get(Y, 2, lp_);

    RaptorTriple tri;
    tri.d = d;
    tri.a = a;
    tri.b = b;

    return tri;
}

template <class T>
void TeaonlyRaptor::xorRow(std::vector<T>& row1, std::vector<T>& row2){
    assert( row1.size() == row2.size());
    for (int i = 0; i < row1.size(); i++) {
        row1[i] = row1[i] ^ row2[i];
    }  
}

