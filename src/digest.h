////////////////////////////////////////////////////////////////////////////////
// digest.h
//
// Create and manage SHA256 hashes with OpenSSL
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DIGEST_H
#define MUD98__DIGEST_H

#include "merc.h"

void bin_to_hex(char* dest, const uint8_t* data, size_t size);
bool create_digest(const char* message, size_t message_len,
    unsigned char** digest, unsigned int* digest_len);
void decode_digest(PC_DATA* pc, const char* hex_str);
void free_digest(unsigned char* digest);
void hex_to_bin(uint8_t* dest, const char* hex_str, size_t bin_size);
bool set_password(char* pwd, CHAR_DATA* ch);
bool validate_password(char* pwd, CHAR_DATA* ch);

#endif // !MUD98__DIGEST_H
