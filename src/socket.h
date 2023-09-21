////////////////////////////////////////////////////////////////////////////////
// socket.h
////////////////////////////////////////////////////////////////////////////////

typedef struct sock_client_t SockClient;
typedef struct sock_server_t SockServer;
typedef struct poll_data_t PollData;

#pragma once
#ifndef MUD98__SOCKET_H
#define MUD98__SOCKET_H

#ifndef USE_RAW_SOCKETS
#include <openssl/ssl.h>
#endif

#ifdef _MSC_VER
#include <winsock.h>
#else
#include <sys/select.h>
#define SOCKET int
#endif

typedef struct sock_client_t {
    SOCKET fd;
#ifndef USE_RAW_SOCKETS
    SSL* ssl;
#endif
} SockClient;

typedef struct sock_server_t {
    SOCKET control;
#ifndef USE_RAW_SOCKETS
    SSL_CTX* ssl_ctx;
#endif
} SockServer;

typedef struct poll_data_t {
    fd_set in_set;
    fd_set out_set;
    fd_set exc_set;
    SOCKET maxdesc;
} PollData;

#endif // !MUD98__SOCKET_H