////////////////////////////////////////////////////////////////////////////////
// digest.c
//
// Create and manage SHA256 hashes with OpenSSL
////////////////////////////////////////////////////////////////////////////////

#ifndef ROM__DIGEST_H
#define ROM__DIGEST_H

#include "merc.h"

void bin_to_hex(char* dest, const uint8_t* data, size_t size);
void decode_digest(PC_DATA* pc, const char* hex_str);
bool digest_message(const char *message, size_t message_len, 
    unsigned char **digest, unsigned int *digest_len);
void free_digest(unsigned char* digest);
void hex_to_bin(uint8_t* dest, const char* hex_str, size_t bin_size);
bool set_password(char* pwd, CHAR_DATA* ch);
bool validate_password(char* pwd, CHAR_DATA* ch);

#endif // !ROM__DIGEST_H
