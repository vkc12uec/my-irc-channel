/*
 ** client.c -- a stream socket client demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool user_input (string &msg);

int main(int argc, char *argv[])
{
  int sockfd, numbytes;  
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  if (argc != 2) {
    fprintf(stderr,"usage: client hostname\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
      s, sizeof s);
  printf("client: connecting to %s\n", s);

  freeaddrinfo(servinfo); // all done with this structure

  /* send command to server 
     - server parses the req.
     - server will spawn a new irc channel : td
     */
  const char *com = "spawn ch123";
  if (send(sockfd, com , strlen(com), 0) == -1)
    perror("send1");

  if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
    perror("recv");
    exit(1);
  }

  buf[numbytes] = '\0';

  printf("client: received '%s'\n",buf);

  string msg;
  cout << "\nNow starting the channel ops";

  /* client flow:
     if any activity on STDIN, then send that to server
     else do recv calls from server
     */

  while (1) {
    if (user_input(msg)) {
      cout << "\nuser entered " << msg;
      if (send(sockfd, msg.c_str() , string::length (msg), 0) == -1){
        perror("send1");
        assert(0);
      }
    }
    else {
      // this part shud be non bloacking, hence use select here too
      if (server_receive(sockfd, msg))
        cout << "\n" << msg;

      if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
      }

      buf[numbytes] = '\0';

      printf("^'%s'\n",buf);

    }

  } // while

  close(sockfd);
  return 0;
}

bool server_receive (int sockfd, string &msg) {
  struct timeval tv;
  fd_set readfds;

  tv.tv_sec = 1;
  tv.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);

  select(sockfd+1, &readfds, NULL, NULL, &tv);

  if (FD_ISSET(sockfd, &readfds)) {
    printf("\n\t\t $$$ there is some thing from server $$$ \n\n");
    char buf[256];

      if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
      }
      buf[numbytes] = '\0';

    msg = string(buf);
    return true;
  }
  else {
    printf("\n\t\t $$$ NOTHING FROM SERVER, SELECT TIME-OUT $$$ \n\n");
    return false;
  }
}

bool user_input (string &msg) {
  msg = "";

#define STDIN 0  // file descriptor for standard input

  struct timeval tv;
  fd_set readfds;

  tv.tv_sec = 1;
  tv.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(STDIN, &readfds);

  // don't care about writefds and exceptfds:
  select(STDIN+1, &readfds, NULL, NULL, &tv);

  if (FD_ISSET(STDIN, &readfds)) {
    printf("A key was pressed!\n");
    cin >> msg;
    return true;
  }
  else {
    printf("Timed out.\n");
    return false;
  }
}

