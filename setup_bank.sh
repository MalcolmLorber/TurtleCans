#!/bin/sh
apt-get update
apt-get install g++ -y
apt-get install libboost-all-dev -y
cd ./cryptopp
make static 
cd ..


