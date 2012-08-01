/*
** pipe2.c -- a smarter pipe example
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

// modify this code to tx descriptors from parent to child on the basis of chid

int main(void)
{
	int pfds[2];
	char buf[30];

    pipe(pfds);

    if (!fork()) {
      printf(" CHILD: writing to the pipe\n");
      write(pfds[1], "test", 5);
      printf(" CHILD: exiting\n");
      exit(0);
    } else {
      printf("PARENT: reading from pipe\n");
      read(pfds[0], buf, 5);
      printf("PARENT: read \"%s\"\n", buf);
      wait(NULL);
    }

  return 0;
}



$$$$$$$$$$$$$$$$$$$$$$

todo:

the new socked descripttor shud be passed via pipe to child ... and child shud 
select() on that

$$$$$$$$$$$$$$$$$$$$$$$
