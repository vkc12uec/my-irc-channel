/*
 ** pipe2.c -- a smarter pipe example
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
using namespace std;


// modify this code to tx descriptors from parent to child on the basis of chid
// test it

int main(void)
{
  int pfds[2];
  char buf[30];
  string input="", chid="";
  map<string, int> chatid_child;    // map of chatid -> pipedes. of connection
  map<string,int>::iterator iter;
  map<string, int> mymap;
  map<string, string> firstsockfd_chid;   // chatid => firstsockfd
  map<int, int> cpid_des;   // childid  => parentTOchild descriptor

  while (1) {
    getline(cin, input);    // 12#123   sockfd#chid
    //cout << "\nchid = [" << chid <<"]";

    size_t found = input.find("#");
    string sockfd = input.substr(0, found);
    string chid = input.substr(found+1);

    cout << "\n sockfd = " << sockfd << " chid = "<< chid;

    iter = mymap.find(chid);

    if (iter == mymap.end()) {
      mymap.insert ( std::make_pair(chid, 1));   // present
      firstsockfd_chid.insert ( std::make_pair(chid, sockfd) );   // present
    }
    else {
      // this is the 2nd client to same chat window
      // fork child and give it the two sock fds.

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
          close(pfds[1]);
          int from_parent = pfds[0];
          cout << "\n\tchild: I have started with sockfds " << firstsockfd_chid[chid] << " and " << sockfd;
          cout << "\n ~~~~~~~~~~~~ FORKED  ~~~~~~~~~~~~~~";
          cout << "\n ### now try select() for new sockfds connecting ### ";

          fd_set read_fds;  
          int nbytes;
          struct timeval tv;

          FD_ZERO(&read_fds);
          FD_SET(from_parent , &read_fds);

          tv.tv_sec = 1;
          tv.tv_usec = 0;

          char buf[256];

          while (1) {
            if ( select(from_parent+1, &read_fds, NULL, NULL, &tv ) == -1)  {
              perror ("select");
              _exit(-10);
            }

            if ( FD_ISSET(from_parent, &read_fds)) {
              // we got new sockfd from parent
              nbytes = recv(from_parent, buf, sizeof buf, 0);
              cout << "\n child: read-end is ON, nbytes = " << nbytes;
              FD_CLR(from_parent, &read_fds);
              FD_SET(from_parent, &read_fds);
              //_exit(-10);
            }
            buf[nbytes] = '\0';
            cout << "\n\t\t New sock fd received = |" << buf << "|";
            exit(-10);
          } // while 

          _exit(0);
        }
      }
      else {
        // child already forked send the new sockfd to child using pipe
        cout << "\n\t\t Child for #chid "<< chid << " allready here; jsut use PIPE to transfer new sockfd " << sockfd << ", pipedes = ["<< cpid_des[chatid_child[chid]] <<"]";
        //fflush (cout);
        cout.flush();
        //_exit(-1);
        if ( write (cpid_des[chatid_child[chid]], sockfd.c_str(), sockfd.length() ) == -1 ) {
          perror ("write-1");
          _exit(-1);
        }
        //sleep (10);
        //_exit(-1);
        cout << "\nParent waiting";
        while (1) { }
      }
    }
  } //while


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

