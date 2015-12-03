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
    while(true){
        //InvertibleRSAFunction is used directly only because the private key
        //won't actually be used to perform any cryptographic operation;
        //otherwise, an appropriate typedef'ed type from rsa.h would have been
        //used.
        AutoSeededRandomPool rng;
        InvertibleRSAFunction privkey;
        privkey.Initialize(rng, 2048);
 
        //With the current version of Crypto++, MessageEnd() needs to be called
        //explicitly because Base64Encoder doesn't flush its buffer on
        //destruction.
        std::stringstream ss;
        ss<<"./privkey"<<i<<".txt";
        std::cout<<ss.str().c_str()<<std::endl;
        Base64Encoder privkeysink(new FileSink(ss.str().c_str()));
        privkey.DEREncode(privkeysink);
        privkeysink.MessageEnd();
 
        //Suppose we want to store the public key separately, possibly because
        //we will be sending the public key to a third party.
        RSAFunction pubkey(privkey);
        
        std::stringstream ss2;
        ss2<<"./pubkey"<<i<<".txt";
        Base64Encoder pubkeysink(new FileSink(ss2.str().c_str()));
        pubkey.DEREncode(pubkeysink);
        pubkeysink.MessageEnd();
        std::cout<<"generated"<<std::endl;
        i++;
    }
}
