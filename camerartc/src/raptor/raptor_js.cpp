#include <iostream>
#include <sys/time.h>
#include <stdlib.h>
#include "raptor.h"


struct JsRaptor {
    TeaonlyRaptor* raptor;
    std::vector<RaptorSymbol>  receivedSymbols;
    RaptorSymbol mySymbol;
};

extern "C" JsRaptor* initRaptor(int K) {
    struct RaptorParameter param = TeaonlyRaptor::createRaptorParameter(K);
    
    JsRaptor* jsRaptor = new JsRaptor; 
    jsRaptor->raptor = new TeaonlyRaptor(param);
    jsRaptor->receivedSymbols.clear();

    return jsRaptor;
}

extern "C" void releaseRaptor(JsRaptor* jsRaptor) {
    delete jsRaptor->raptor;
    jsRaptor->receivedSymbols.clear();
    delete jsRaptor;
}

extern "C" void pushSymbol(JsRaptor* jsRaptor, unsigned int esi, unsigned  char *block, unsigned int length) {
    RaptorSymbol newSymbol;

    newSymbol.esi = esi;
    newSymbol.data.reserve(length);
    std::copy( block, block + length, std::back_inserter(newSymbol.data));

    jsRaptor->receivedSymbols.push_back(newSymbol);
}

extern "C" int doEncode(JsRaptor* jsRaptor, unsigned char* data, unsigned int length) {
    bool ret = jsRaptor->raptor->doEncode((unsigned char *)data, length);

    if (ret == false)
        return -1;
    return 0;
}

extern "C" int doDecode(JsRaptor* jsRaptor) {
    bool ret = jsRaptor->raptor->doDecode( jsRaptor->receivedSymbols );
    if ( ret == false)
       return -1;

    return 0;
}

extern "C" int getSymbol(JsRaptor* jsRaptor, int esi, unsigned char *block) {
    jsRaptor->mySymbol.esi = esi;
    bool ret = jsRaptor->raptor->getSymbol(&(jsRaptor->mySymbol));
    if ( ret == false)
        return -1;
    
    memcpy(block, &jsRaptor->mySymbol.data[0], jsRaptor->mySymbol.data.size());
    return jsRaptor->mySymbol.data.size();
} 

extern "C" int buildPayload(JsRaptor* jsRaptor, unsigned char *buffer) { 
    RaptorSymbol symbol;
    for ( int i = 0; i < jsRaptor->raptor->K(); i++) {
        symbol.esi = i;
        if ( jsRaptor->raptor->getSymbol(&symbol) == false) {
            return -1;
        }
       
        memcpy(buffer, &(symbol.data[0]), symbol.data.size());
        buffer += symbol.data.size();
    }
    return 0;
}


