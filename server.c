/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fcntl.h>

static void bzero(void *address, int length)
{
	memset(address, 0, length);
}
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <io.h>
#endif
void error(const char *fmt, ...)
{
    static char buffer[512];
    va_list a_list;
    va_start(a_list, fmt);
    vsprintf(buffer, fmt, a_list);
    perror(buffer);
    va_end(a_list);
    exit(1);
}


#ifdef WIN32
#undef socket
static int mingw_socket(int domain, int type, int protocol)
{
	int sockfd;
	SOCKET s;

	s = WSASocket(domain, type, protocol, NULL, 0, 0);
	if (s == INVALID_SOCKET) {
		/*
		 * WSAGetLastError() values are regular BSD error codes
		 * biased by WSABASEERR.
		 * However, strerror() does not know about networking
		 * specific errors, which are values beginning at 38 or so.
		 * Therefore, we choose to leave the biased error code
		 * in errno so that _if_ someone looks up the code somewhere,
		 * then it is at least the number that are usually listed.
		 */
		errno = WSAGetLastError();
		return -1;
	}
	/* convert into a file descriptor */
	if ((sockfd = _open_osfhandle(s, O_RDWR|O_BINARY)) < 0) {
		closesocket(s);
		fprintf(stderr, "unable to make a socket file descriptor: %s",
			strerror(errno));
		return -1;
	}
	return sockfd;
}
#define socket mingw_socket
#undef bind
static int mingw_bind(int sockfd, struct sockaddr *sa, size_t sz)
{
	SOCKET s = (SOCKET)_get_osfhandle(sockfd);
	return bind(s, sa, sz);
}
#define bind mingw_bind

#undef listen
int mingw_listen(int sockfd, int backlog)
{
	SOCKET s = (SOCKET)_get_osfhandle(sockfd);
	return listen(s, backlog);
}
#define listen mingw_listen
#undef accept
int mingw_accept(int sockfd1, struct sockaddr *sa, socklen_t *sz)
{
	int sockfd2;

	SOCKET s1 = (SOCKET)_get_osfhandle(sockfd1);
	SOCKET s2 = accept(s1, sa, sz);

	/* convert into a file descriptor */
	if ((sockfd2 = _open_osfhandle(s2, O_RDWR|O_BINARY)) < 0) {
		int err = errno;
		closesocket(s2);
		error("unable to make a socket file descriptor: %s",
			strerror(err));
	}
	return sockfd2;
}
#define accept mingw_accept

#endif
int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
#ifdef WIN32
     WSADATA wsa;
     if (WSAStartup(MAKEWORD(2,2), &wsa)) error("WSAStartup");
#endif
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     n = write(newsockfd,"I got the connection",20);
     if (n < 0) error("ERROR writing to socket");
     close(newsockfd);
     close(sockfd);
#ifdef WIN32
     WSACleanup();
#endif
     return 0; 
}

