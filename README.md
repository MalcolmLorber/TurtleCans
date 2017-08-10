# TurtleCans

Cans for turtles

## Install Instructions
1. run "sudo ./setup_bank.sh"
2. run "make rsa"
3. run "make"
4. startup the bank e.g. "./bank.out 8001"
5. startup the proxy e.g. "./proxy.out 8000 8001"
6. connect the atm e.g. "./atm.out 8000"

## Design

* Inspired by SSL/TLS
* Begin with handshake between Bank and ATM - share public keys, make unique key 
for connection
* All messages have (increasing number / next number from cryptostream) attached
to them to prevent replay
* If any bad message detected, kill connection
* Sign messages with hash to detect tampering

### Protocol
1. ATM knows the Bank's RSA public key.
2. ATM sends the Bank it's RSA public key in plain text.
3. A randomly generated number is encrypted using the Bank's RSA public key and sent to the Bank.
4. The Bank decrypts the randomly generated number using its RSA private key.
5. The Bank encrypts the decrypted randomly generated number it was sent by the ATM using the ATM's RSA public key and sends it to the ATM.
6. The ATM decrypts the message and compares the randomly generated number it originally sent to the Bank with the result of the decryption. If it is a match, it is the Bank. Otherwise, it is not the Bank so break the connection.
7. Encrypt the ATM's Diffie-Hellman public key using the Bank's RSA public key.
8. The Bank decrypts the message and sends its Diffie-Hellman public key encrypted using the ATM's RSA public key to the ATM.
9. The ATM recieves the message and decrypts it.
10. ATM sends Bank an acknowledgment
11. The Bank receives the acknowledgment and sends its the initial value as plain text to the ATM.
12. AES Cipher Feedback is used for all further communication with ATM to Bank communication using the first initial value and Bank to ATM communication using the second initial value.

### Error Handling
* When in doubt, kick the user out
