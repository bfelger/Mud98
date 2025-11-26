////////////////////////////////////////////////////////////////////////////////
// digest.c
//
// Create and manage SHA256 hashes with OpenSSL
////////////////////////////////////////////////////////////////////////////////

#include "digest.h"

#include "entities/player_data.h"
#include "db.h"

#include <string.h>

#ifndef NO_OPENSSL
#include <openssl/err.h>
#include <openssl/evp.h>
#endif 

void bin_to_hex(char* dest, const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        sprintf(dest + (i * 2), "%.2X", data[i]);
    }
    dest[size * 2] = '\0';
}

bool create_digest(const char* message, size_t message_len, 
    unsigned char** digest, unsigned int* digest_len)
{
#ifndef NO_OPENSSL
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
#elif defined (NO_ENCRYPTION)
    *digest = (unsigned char*)malloc(message_len);
    if (!*digest)
        return false;

    memcpy(digest, message, message_len);
    *digest_len = (unsigned int)message_len;
    return true;
#else
    #error "You must compile with OpenSSL 3.x+, or set the NO_ENCRYPTION flag."
    return false;
#endif
}

void decode_digest(PlayerData* pc, const char* hex_str)
{
#ifndef NO_OPENSSL
    if (pc->pwd_digest != NULL)
        OPENSSL_free(pc->pwd_digest);
    free_string(pc->pwd_digest_hex);
    pc->pwd_digest_hex = str_dup(hex_str);

    pc->pwd_digest_len = EVP_MD_size(EVP_sha256());
    pc->pwd_digest = (unsigned char*)OPENSSL_malloc(pc->pwd_digest_len);
#elif defined (NO_ENCRYPTION)
    pc->pwd_digest_len = strlen(hex_str) / 2;
    pc->pwd_digest = (unsigned char*)malloc(pc->pwd_digest_len);
#else
    #error "You must compile with OpenSSL 3.x+, or set the NO_ENCRYPTION flag."
#endif

    hex_to_bin(pc->pwd_digest, hex_str, pc->pwd_digest_len);
}

void free_digest(unsigned char* digest)
{
#ifndef NO_OPENSSL
    OPENSSL_free(digest);
#elif defined (NO_ENCRYPTION)
    free(digest);
#else
    #error "You must compile with OpenSSL 3.x+, or set the NO_ENCRYPTION flag."
#endif
}

void hex_to_bin(uint8_t* dest, const char* hex_str, size_t bin_size)
{
    for (int i = 0; i < (int)bin_size; ++i) {
        char ch1 = *hex_str++;
        uint8_t hi = (ch1 >= 'A') ? (ch1 - 'A' + 10) : (ch1 - '0');
        char ch2 = *hex_str++;
        uint8_t lo = (ch2 >= 'A') ? (ch2 - 'A' + 10) : (ch2 - '0');
        uint8_t byte = (hi << 4) | (lo & 0xF);
        *dest++ = (uint8_t)byte;
    }
}

bool set_password(char* pwd, Mobile* ch)
{
    // If the operation fails, keep the old digest
    unsigned char* old_digest = ch->pcdata->pwd_digest;
    unsigned int old_digest_len = ch->pcdata->pwd_digest_len;
    char* old_digest_hex = ch->pcdata->pwd_digest_hex;

    // So create_digest() has no way to free the the old digest, yet (it 
    // doesn't, but what if that changes in the future? I don't want to have to
    // worry about it.
    ch->pcdata->pwd_digest = NULL;
    ch->pcdata->pwd_digest_len = 0;
    ch->pcdata->pwd_digest_hex = NULL;

    if (!create_digest(pwd, strlen(pwd), &ch->pcdata->pwd_digest,
            &ch->pcdata->pwd_digest_len)) {
        perror("set_password: Could not get digest.");

        ch->pcdata->pwd_digest = old_digest;
        ch->pcdata->pwd_digest_len = old_digest_len;
        ch->pcdata->pwd_digest_hex = old_digest_hex;
        return false;
    }

    char digest_buf[EVP_MAX_MD_SIZE * 2 + 1] = { 0 };
    bin_to_hex(digest_buf, ch->pcdata->pwd_digest, ch->pcdata->pwd_digest_len);
    ch->pcdata->pwd_digest_hex = str_dup(digest_buf);

    // Ok, NOW we can throw it away.
    if (old_digest != NULL)
        free_digest(old_digest);
    free_string(old_digest_hex);

    return true;
}

bool validate_password(char* pwd, Mobile* ch)
{
    PlayerData* pc = ch->pcdata;
    if (pc == NULL)
        return false;

    if (pc->pwd_digest_hex == NULL || pc->pwd_digest_hex[0] == '\0')
        return false;

    size_t stored_len = strlen(pc->pwd_digest_hex) / 2;
    unsigned char* stored_digest = (unsigned char*)malloc(stored_len);
    if (!stored_digest)
        return false;

    hex_to_bin(stored_digest, pc->pwd_digest_hex, stored_len);

    unsigned char* new_digest;
    unsigned int new_digest_len;
    if (!create_digest(pwd, strlen(pwd), &new_digest, &new_digest_len)) {
        perror("validate_password: Could not get digest.");
        free(stored_digest);
        return false;
    }

    bool match = stored_len == new_digest_len &&
        !memcmp(new_digest, stored_digest, new_digest_len);

    free_digest(new_digest);
    free(stored_digest);
    return match;
}
