////////////////////////////////////////////////////////////////////////////////
// socket.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__SOCKET_H
#define MUD98__SOCKET_H

#include "merc.h"

#ifndef NO_OPENSSL
#include <openssl/ssl.h>
#endif

#ifdef _MSC_VER
#include <winsock.h>
#else
#include <sys/select.h>
#define SOCKET int
#endif

typedef enum socket_type_t {
    SOCK_TELNET,
    SOCK_TLS,
} SockType;

typedef struct sock_client_t {
    SockType type;
    SOCKET fd;
} SockClient;

typedef struct sock_server_t {
    SockType type;
    SOCKET control;
} SockServer;

#ifndef NO_OPENSSL
typedef struct tls_client_t {
    SockType type;
    SOCKET fd;
    SSL* ssl;
} TlsClient;

typedef struct tls_server_t {
    SockType type;
    SOCKET control;
    SSL_CTX* ssl_ctx;
} TlsServer;
#endif

typedef struct poll_data_t {
    fd_set in_set;
    fd_set out_set;
    fd_set exc_set;
    SOCKET maxdesc;
} PollData;

#endif // !MUD98__SOCKET_H