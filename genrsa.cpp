/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 12-02-2015
Filename: genrsa.cpp
*******************************************************************************/

//Standard library
#include <iostream>
#include <sstream>
//Crypto++
#include "cryptopp/rsa.h"
#include "cryptopp/osrng.h"
#include "cryptopp/base64.h"
#include "cryptopp/files.h"

using namespace CryptoPP;

int main() {
    int i=0;
    
    AutoSeededRandomPool rng;
    

    InvertibleRSAFunction privkey;
    privkey.Initialize(rng, 2048);
    
    Base64Encoder privkeysink(new FileSink("./privkey.txt"));
    privkey.DEREncode(privkeysink);
    privkeysink.MessageEnd();
    
    RSAFunction pubkey(privkey);
    
    Base64Encoder pubkeysink(new FileSink("./pubkey.txt"));
    pubkey.DEREncode(pubkeysink);
    pubkeysink.MessageEnd();
                             

    InvertibleRSAFunction atmprivkey;
    atmprivkey.Initialize(rng, 2048);
    
    Base64Encoder atmprivkeysink(new FileSink("./atmprivkey.txt"));
    atmprivkey.DEREncode(atmprivkeysink);
    atmprivkeysink.MessageEnd();
    
    RSAFunction atmpubkey(atmprivkey);
    
    Base64Encoder atmpubkeysink(new FileSink("./atmpubkey.txt"));
    atmpubkey.DEREncode(atmpubkeysink);
    atmpubkeysink.MessageEnd();
}
