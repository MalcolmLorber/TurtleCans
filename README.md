# TurtleCans

Cans for turtles

##Install Instructions
1. run "sudo ./setup_bank.sh"
2. run "make rsa"
3. run "make"
4. startup the bank e.g. "./bank.out 8001"
5. startup the proxy e.g. "./proxy.out 8000 8001"
6. connect the atm e.g. "./atm.out 8000"

##Design

* Inspired by SSL/TLS
* Begin with handshake between Bank and ATM - share public keys, make unique key 
for connection
* All messages have (increasing number / next number from cryptostream) attached
to them to prevent replay
* If any bad message detected, kill connection
* Sign messages with hash to detect tampering

###Handshake

###Message content

###Error Handling
