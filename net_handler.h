#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

SOCKET openConnect(const char*, const char*);
int sendPlaneData(SOCKET, char*);
int getPlaneData(SOCKET, char*);
void killSocket(SOCKET);

SSL_CTX* initLibSSL(void);
SSL* openSSLConnect(SOCKET, SSL_CTX*);
int sendSSLData(SSL*, char*);
int getSSLData(SSL*, char*);
void getSSLError(int);
void killSSLSocket(SSL*, SSL_CTX*);

