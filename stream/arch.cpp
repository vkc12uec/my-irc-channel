/*
 ** pipe2.c -- a smarter pipe example
 */

#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>
#include <assert.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <map>
#include <sstream>

using namespace std;
#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 10	 // how many pending connections queue will hold
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// modify this code to tx descriptors from parent to child on the basis of chid
// test it

string convertInt(int number)
{
  stringstream ss;//create a stringstream
  ss << number;//add number to the stream
  return ss.str();//return a string with the contents of the stream
}

void prnt (int i) {
  cout << ":" << i << ":";
}

#if 0
void relay (int cli_fd) {
  if (send(cli_fd, buf, nbytes, 0) == -1) {
    cout << "\n66";
    perror("send");
  }
#endif

  int main(void)
  {
    int pfds[2];
    string input="", chid="";
    map<string, int> chatid_child;    // map of chatid -> pipedes. of connection
    map<string,int>::iterator iter;
    map<string, int> mymap;
    map<string, string> firstsockfd_chid;   // chatid => firstsockfd
    map<int, int> cpid_des;   // childid  => parentTOchild descriptor

    ///// ~~~~
    int s_sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char buf[100];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((s_sockfd = socket(p->ai_family, p->ai_socktype,
              p->ai_protocol)) == -1) {
        perror("server: socket");
        continue;
      }

      if (setsockopt(s_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
      }

      if (bind(s_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(s_sockfd);
        perror("server: bind");
        continue;
      }

      break;
    }

    if (p == NULL)  {
      fprintf(stderr, "server: failed to bind\n");
      return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(s_sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
    }

    printf("server: waiting for connections...\n");
    ///// ~~~~

    int numbytes;

    while (1) {

#ifdef _old_code
      getline(cin, input);    // 12#123   sockfd#chid
      //cout << "\nchid = [" << chid <<"]";

      size_t found = input.find("#");
      string sockfd = input.substr(0, found);
      string chid = input.substr(found+1);
      cout << "\n sockfd = " << sockfd << " chid = "<< chid;
#endif

      sin_size = sizeof their_addr;
      new_fd = accept(s_sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if (new_fd == -1) {
        perror("accept");
        continue;
      }

      string sockfd = convertInt (new_fd);

      inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
      printf("server: got connection from %s\n", s);

      if ((numbytes = recv(new_fd, buf, 100-1, 0)) == -1) {
        perror("recv");
        exit(1);
      } // supposedly = "spawn chid12" or "123" (direct send chatbox#)

      buf[numbytes] = '\0';

      chid = string(buf);
      printf ("\nsockfd = [%s] chid = [%s] " , sockfd.c_str(), chid.c_str() );
      //, cout << "\n sockfd = " << sockfd << " chid = "<< chid;
      //cout.flush();

      //continue;   // remove

      iter = mymap.find(chid);

      if (iter == mymap.end()) {
        mymap.insert ( std::make_pair(chid, 1));   // present
        firstsockfd_chid.insert ( std::make_pair(chid, sockfd) );   // present
        cout << "\nServer : first time #CHID seen ";
      }
      else {
        // this is the 2nd client to same chat window
        // fork child unless child EXISTS and give it the two sock fds.

        if ( chatid_child.find (chid) == chatid_child.end() ) {
          // fork child
          pipe(pfds);
          int child = fork();
          //cout << "\n FORKED ";

          if (child) {
            //parent
            close(pfds[0]);
            cpid_des[child] = pfds[1];
            cout << "\n parent: stored des "<< cpid_des[child] << " for child " << child;
            chatid_child.insert ( std::make_pair(chid, child) ); // this is checked above
          }
          else {
            // child 
            // sockfds 1=firstsockfd_chid[chid] 2=sockfd

            //flow:
            //keep listening to your parent (for new sock fds connecting) and the older sockfds who are using the IRC channel
            //if (new sockfd from parent)
            //add it to your list_of_listening_fds
            //select on list_of_listening_fds
            //if ( any active fd in set ) 
            //- check if it hung up
            //- else bcast that

            vector <int> bcast_fds;
            bcast_fds.push_back( atoi(firstsockfd_chid[chid].c_str()) );
            bcast_fds.push_back( atoi(sockfd.c_str()) );

            int from_parent = pfds[0];
            close(pfds[1]);

            cout << "\n\tchild: I have started with sockfds " << firstsockfd_chid[chid] << " and " << sockfd << " from_parent = " << from_parent;
            cout << "\n ~~~~~~~~~~~~ FORKED  ~~~~~~~~~~~~~~";
            cout << "\n ### now try select() for new sockfds connecting ### ";

            fd_set read_fds;      /* this contains parent fd and bcast fds */
            int nbytes, fdmax;
            struct timeval tv;
            char buf[256];

            FD_ZERO(&read_fds);
            FD_SET(from_parent , &read_fds);
            FD_SET(bcast_fds.at(0) , &read_fds);
            FD_SET(bcast_fds.at(1) , &read_fds);

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            fdmax = max (bcast_fds.at(0), bcast_fds.at(1));
            fdmax = max (fdmax, from_parent);
            cout << "\nchild: fdmax = " << fdmax << " : "<< __LINE__;

            cout.flush();

            while (1) {
              if ( select(fdmax+1, &read_fds, NULL, NULL, NULL /*&tv*/ ) == -1)  {    // blocking
                //if ( select(from_parent+1, &read_fds, NULL, NULL, NULL /*&tv*/ ) == -1) 
                perror ("select");
                _exit(-10);
              }

              for (int i=0; i <= fdmax; i++) {

                if (FD_ISSET(i, &read_fds)) {

                  if (i == from_parent) {
                    // we got new sockfd from parent
                    nbytes = read(i, buf, sizeof buf);
                    //cout << "\n child: read-end is ON, nbytes = " << nbytes;
                    //FD_CLR(from_parent, &read_fds);
                    //FD_SET(from_parent, &read_fds);
                    //_exit(-10);
                    buf[nbytes] = '\0';
                    cout << "\n\t\t New sock fd received = |" << buf << "| ; adding to my lists/vec.";
                    //exit(-10);
                    bcast_fds.push_back( atoi(buf) ); // todo: update fdmax when updating bcast_fds[]
                    FD_SET (atoi(buf), &read_fds);
                  }
                  else {
                    cout << "\nhere 33";
                    // got data from the clients..
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                      // got error or connection closed by client
                      if (nbytes == 0) {
                        // connection closed
                        printf("\n\t\tCHILD: socket %d hung up\n", i);
                        assert (i != from_parent);
                        cout << "\nhere 44";
                      } else {
                        perror("recv");
                      }
                      close(i);
                      // remove from vector/read_fds
                      FD_CLR(i, &read_fds);
                      vector<int>::iterator id;

                      // todo: bcast_fds is length 0
                      id = find (bcast_fds.begin(), bcast_fds.end(), i);
                      bcast_fds.erase (id);
                      cout << "\nChild: Removed fd = " << i << " from vector/read_fds";
                    }
                    else {
                      buf[nbytes] = '\0';
                      printf ("\nChild : got smth from client [%d] : [%s]", i, buf);
                      cout << "\n my bcast vector is ";

                      for_each (bcast_fds.begin(),
                          bcast_fds.end(),
                          prnt);

#if 0
                      for_each (bcast_fds.begin(),
                          bcast_fds.end(),
                          relay());
#endif
                      for (int ii=0; ii<bcast_fds.size(); ii++) {
                        if (ii == i) continue;
                        cout << "\n Try Relay from c"<< i << " to client"<<ii;
                        if (send(bcast_fds.at(ii), buf, nbytes, 0) == -1) {
                          cout << "\n66";
                          perror("send");
                        }
                      }

#ifdef old_code
                      // we got some data from a client
                      for(int j = 0; j <= fdmax; j++) {
                        // send to everyone!
                        if (FD_ISSET(j, &read_fds)) {
                          cout << "\n55";
                          // except the listener and ourselves
                          if (j != from_parent && j != i) {
                            cout << "\n Relay from c"<< i << " to c"<<j;
                            if (send(j, buf, nbytes, 0) == -1) {
                              cout << "\n66";
                              perror("send");
                            }
                          }
                        }
                      }  // iterate over client connectinos
#endif


                      cout.flush();
                    }

                  }
                }
              }  // fds polled

#ifdef _old_code
              if ( FD_ISSET(from_parent, &read_fds)) {
                // we got new sockfd from parent
                nbytes = read(from_parent, buf, sizeof buf);
                cout << "\n child: read-end is ON, nbytes = " << nbytes;
                //FD_CLR(from_parent, &read_fds);
                //FD_SET(from_parent, &read_fds);
                //_exit(-10);
                buf[nbytes] = '\0';
                cout << "\n\t\t New sock fd received = |" << buf << "|";
                //exit(-10);
              }
              else {
                cout << "\n from_parent NOT set in FDSET ";
              }
              cout.flush();
#endif

            } // while child

            _exit(0);
          }
        }
        else {
          // parent
          cout << "\n\t\t Child for #chid "<< chid << " allready here; jsut use PIPE to transfer new sockfd " << sockfd << ", pipedes = ["<< cpid_des[chatid_child[chid]] <<"]";
          if ( write (cpid_des[chatid_child[chid]], sockfd.c_str(), sockfd.length() ) == -1 ) {
            perror ("write-1");
            _exit(-1);
          }
          cout << "\nParent : sent the new sock id to child ";
          cout.flush();
        }
      }
    } //while parent


#if 0
    if (!fork()) {
      close(pfds[1]);

      //printf(" CHILD: writing to the pipe\n");
      //write(pfds[1], "test", 5);
      //printf(" CHILD: exiting\n");
      read(pfds[0], buf, 5);
      exit(0);
    } else {
      printf("PARENT: reading from pipe\n");
      read(pfds[0], buf, 5);
      printf("PARENT: read \"%s\"\n", buf);
      wait();
    }
  }

} //while
#endif

return 0;
}

