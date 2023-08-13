////////////////////////////////////////////////////////////////////////////////
// digest.c
//
// Create and manage SHA256 hashes with OpenSSL
////////////////////////////////////////////////////////////////////////////////

#include "digest.h"

#include <string.h>

#include <openssl/err.h>
#include <openssl/evp.h>

void bin_to_hex(char* dest, const uint8_t* data, size_t size)
{
    for (int i = 0; i < size; ++i) {
        sprintf(dest, "%.2X", *data++);
        dest += 2;
    }
}

bool create_digest(const char* message, size_t message_len, 
    unsigned char** digest, unsigned int* digest_len)
{
    EVP_MD_CTX* mdctx;

    if ((mdctx = EVP_MD_CTX_new()) == NULL)
        goto digest_message_err;

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
        goto digest_message_err;

    if (1 != EVP_DigestUpdate(mdctx, message, message_len))
        goto digest_message_err;

    if ((*digest = (unsigned char*)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL)
        goto digest_message_err;

    if (1 != EVP_DigestFinal_ex(mdctx, *digest, digest_len)) {
        free_digest(*digest);
        goto digest_message_err;
    }

    EVP_MD_CTX_free(mdctx);
    return true;

digest_message_err:
    // C++ exceptions are looking pretty nice, right now...
    ERR_print_errors_fp(stderr);
    EVP_MD_CTX_free(mdctx);
    return false;
}

void decode_digest(PC_DATA* pc, const char* hex_str)
{
    if (pc->pwd_digest != NULL)
        OPENSSL_free(pc->pwd_digest);

    pc->pwd_digest_len = EVP_MD_size(EVP_sha256());
    pc->pwd_digest = (unsigned char*)OPENSSL_malloc(pc->pwd_digest_len);

    hex_to_bin(pc->pwd_digest, hex_str, pc->pwd_digest_len);
}

void free_digest(unsigned char* digest)
{
    OPENSSL_free(digest);
}

void hex_to_bin(uint8_t* dest, const char* hex_str, size_t bin_size)
{
    for (int i = 0; i < bin_size; ++i) {
        char ch1 = *hex_str++;
        uint8_t hi = (ch1 >= 'A') ? (ch1 - 'A' + 10) : (ch1 - '0');
        char ch2 = *hex_str++;
        uint8_t lo = (ch2 >= 'A') ? (ch2 - 'A' + 10) : (ch2 - '0');
        uint8_t byte = (hi << 4) | (lo & 0xF);
        *dest++ = (uint8_t)byte;
    }
}

bool set_password(char* pwd, CHAR_DATA* ch)
{
    // If the operation fails, keep the old digest
    unsigned char* old_digest = ch->pcdata->pwd_digest;
    unsigned int old_digest_len = ch->pcdata->pwd_digest_len;

    // So create_digest() has no way to free the the old digest, yet (it 
    // doesn't, but what if that changes in the future? I don't want to have to
    // worry about it.
    ch->pcdata->pwd_digest = NULL;
    ch->pcdata->pwd_digest_len = 0;

    if (!create_digest(pwd, strlen(pwd), &ch->pcdata->pwd_digest,
            &ch->pcdata->pwd_digest_len)) {
        perror("set_password: Could not get digest.");

        ch->pcdata->pwd_digest = old_digest;
        ch->pcdata->pwd_digest_len = old_digest_len;
        return false;
    }

    // Ok, NOW we can throw it away.
    if (old_digest != NULL)
        free_digest(old_digest);

    return true;
}

bool validate_password(char* pwd, CHAR_DATA* ch)
{
    if (ch->pcdata->pwd_digest == NULL || ch->pcdata->pwd_digest_len == 0)
        return false;

    unsigned char* new_digest;
    unsigned int new_digest_len;
    if (!create_digest(pwd, strlen(pwd), &new_digest, &new_digest_len)) {
        perror("validate_password: Could not get digest.");
        return false;
    }

    return ch->pcdata->pwd_digest_len == new_digest_len &&
        !memcmp(new_digest, ch->pcdata->pwd_digest, new_digest_len);
}
