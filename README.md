# TurtleCans
Cans for turtles
##Design
Begin with handshake between Bank and ATM - share public keys, make unique key for connection

All messages have (increasing number / next number from cryptostream) attached to them to prevent replay

If any bad message detected, kill connnection

Sign messages with hash to detect tampering
###Handshake
###Message content
###Error Handling
