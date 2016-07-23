#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "raptor.h"

int main(int argc, char* argv[]) {
    const int K = 128;
    const int rK = K + 5;
    char testString[] = "1This text is pretty long, but will be "
                        "2concatenated into just a single string. "
                        "3The disadvantage is that you have to quote "
                        "4each part, and newlines must be literal as "
                        "5concatenated into just a single string. "
                        "6The disadvantage is that you have to quote "
                        "7each part, and newlines must be literal as "
                        "8concatenated into just a single string. "
                        "9The disadvantage is that you have to quote "
                        "10each part, and newlines must be literal as "
                        "11concatenated into just a single string. "
                        "12The disadvantage is that you have to quote "
                        "13each part, and newlines must be literal as "
                        "14usual."; 
    //srandom(K);
    
    struct RaptorParameter param = TeaonlyRaptor::createRaptorParameter(K);
    TeaonlyRaptor raptorEncoder(param);  
    TeaonlyRaptor raptorDecoder(param);  
    raptorEncoder.doEncode((unsigned char *)testString, sizeof(testString));

    RaptorSymbol symbol;
    symbol.esi = 1024;
    std::vector<RaptorSymbol>  receivedSymbols;
    for(;;) {
        raptorEncoder.getSymbol(&symbol);
        if ( random() % 3) {
            receivedSymbols.push_back(symbol);
            std::cout << "R : >>>>>>" << symbol.esi << std::endl;
            if ( receivedSymbols.size() == rK ) {
                break;
            } else if (receivedSymbols.size() > rK/2 && symbol.esi < 10000) {
              symbol.esi = 10000;
            }
        }
        symbol.esi ++;
    }

    unsigned char message[1024];
    unsigned int messageLength = 0;
    if ( raptorDecoder.doDecode( receivedSymbols ) ) {
        for (int i = 0; i < K; i++) {
            symbol.esi = i;
            raptorDecoder.getSymbol(&symbol);
            for(int j = 0; j < symbol.data.size(); j++) {
                message[messageLength] = symbol.data[j];
                messageLength ++;
            }
        }
        message[messageLength] = 0;
        std::cout << "[Received Message:] " << std::endl << message << std::endl;
    } else {
        std::cout << "k + " <<  rK - K << " symbols is not enough!" << std::endl;
    }

    return 0;
}
