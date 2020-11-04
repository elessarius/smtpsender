
#include "net_handler.h"

SOCKET openConnect(const char* _host, const char* port)
{
    SOCKET sd = INVALID_SOCKET;
    WSADATA wsaData;
    WORD wVer = MAKEWORD(2, 2);
    char const on = 1;
    unsigned long sMode = 0;
    struct addrinfo hints, * addr;
    int err;

    // Initialize WinSock
    err = WSAStartup(wVer, &wsaData);
    if (err != NO_ERROR)
    {
        printf("WSAStartup failed with error: %d\n", err);
        WSACleanup();
        return 0;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        WSACleanup();
    }
 
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_ALL;
    err = getaddrinfo(_host, port, &hints, &addr);
    if (err != 0)
    {
        printf("Error: %d, getaddrinfo, host=%s, port=%s\n", err, _host, port);
        WSACleanup();
        return 0;
    }

    sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sd == INVALID_SOCKET)
    {
        printf("Socket creation failed.\n");
        WSACleanup();
        return 0;
    }

    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
    {
        printf("Invalid socket.\n");
        WSACleanup();
        return 0;
    }

    if (ioctlsocket(sd, FIONBIO, &sMode) == SOCKET_ERROR)
    {
        printf("ioctlsocket failed with error: %ld\n", SOCKET_ERROR);
        WSACleanup();
        return 0;
    }

    if (connect(sd, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
            closesocket(sd);

        printf("Failed to establish connection with server\n");
        WSACleanup();
        return 0;
    }

    return sd;
}


SSL* openSSLConnect(SOCKET sock, SSL_CTX* ctx)
{
    int res = 1;
    SSL* ssl = NULL;

    ctx = initLibSSL();
    if (ctx == NULL) {
        return NULL;
    }

    ssl = SSL_new(ctx);
    if (ssl == NULL) {
        puts("Error SSL_new");
        return NULL;
    }

    res = SSL_set_fd(ssl, (int)sock);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    if (res == 0) {
        res = SSL_get_error(ssl, res);
        printf("SSL Socket Error %d\n", res);
        return  NULL;
    }

    SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
    SSL_set_connect_state(ssl);

    res = SSL_connect(ssl);
    if (res <= 0)
    {
        int errcode = SSL_get_error(ssl, res);
        getSSLError(errcode);
        return  NULL;
    }

    return ssl;
}


int sendSSLData(SSL* ssl, char* cmd)
{
    size_t size_buf;
    int res = 0;
    int ssl_err;

    if (strlen(cmd) <= 0)
        return res;

    size_buf = strlen(cmd);
    res = SSL_write(ssl, cmd, (int)size_buf);
    ssl_err = SSL_get_error(ssl, res);
    getSSLError(ssl_err);
    printf("C: %s", cmd);

    return res;
}


int getSSLData(SSL* ssl, char* recive)
{
    int res;
    int ssl_err;

    res = SSL_read(ssl, recive, 1024);
    ssl_err = SSL_get_error(ssl, res);
    getSSLError(ssl_err);
    printf("S: %s", recive);
    memset(recive, 0, strlen(recive));

    return res;
}


int sendPlaneData(SOCKET sock, char* buff)
{
    if (send(sock, buff, (int)strlen(buff), 0) == SOCKET_ERROR)
    {
        printf("Error send data: %d\n", SOCKET_ERROR);
        return 0;
    }

    printf("C: %s\n", buff);
    return 1;
}


int getPlaneData(SOCKET sock, char* cmd)
{
    int res = 0;

    if ((res = recv(sock, cmd, sizeof(cmd), 0)) == SOCKET_ERROR)
    {
        printf("Error recv data: %d\n", SOCKET_ERROR);
        return res;
    }

    printf("S: %s\n", cmd);
    return res;
}


SSL_CTX* initLibSSL(void)
{
    SSL_CTX* ctx = NULL;
    
    OPENSSL_init_ssl(0, 0);
    OPENSSL_init_crypto(0xc, 0);
    OPENSSL_init_ssl(0x200001, 0);

    ctx = SSL_CTX_new(TLS_client_method());
    if (ctx != NULL) {
        SSL_CTX_set_verify(ctx, 0, 0);
    }
    else {
        printf("SSL CTX failed\n");
    }

    return ctx;
}


void getSSLError(int ssl_err)
{
    switch (ssl_err)
    {
    case SSL_ERROR_NONE: break;        // Cannot happen if err <=0
    case SSL_ERROR_ZERO_RETURN: fprintf(stderr, "SSL connect returned 0."); break;
    case SSL_ERROR_WANT_READ: fprintf(stderr, "SSL connect: Read Error."); break;
    case SSL_ERROR_WANT_WRITE: fprintf(stderr, "SSL connect: Write Error."); break;
    case SSL_ERROR_WANT_CONNECT: fprintf(stderr, "SSL connect: Error connect."); break;
    case SSL_ERROR_WANT_ACCEPT: fprintf(stderr, "SSL connect: Error accept."); break;
    case SSL_ERROR_WANT_X509_LOOKUP: fprintf(stderr, "SSL connect error: X509 lookup."); break;
    case SSL_ERROR_SYSCALL: fprintf(stderr, "SSL connect: Error in system call."); break;
    case SSL_ERROR_SSL: fprintf(stderr, "SSL connect: Protocol Error.");
        break;
    default: fprintf(stderr, "Failed SSL connect.");
    }
}


void killSocket(SOCKET s)
{
    closesocket(s);
    WSACleanup();
}

void killSSLSocket(SSL* ssl, SSL_CTX* ctx)
{
    SSL_free(ssl);
    SSL_CTX_free(ctx);
}