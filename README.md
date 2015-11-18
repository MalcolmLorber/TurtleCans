# TurtleCans
Cans for turtles
##Design
Begin with handshake between Bank and ATM - share public keys, make unique key for connection

All messages have (increasing number / next number from cryptostream) attached to them to prevent replay

if any bad message detected, kill connnection
