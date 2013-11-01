/*************************************************************/
/*  MoTalk - A "C" program for modem setup.                */
/*           This program is meant as an aid only and is   */
/*           not supported by IBM.                       */
/*                  compile:  cc -o motalk motalk.c       */
/*                  Usage:  motalk /dev/tty? [speed]      */
/*************************************************************/
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <termio.h>
FILE *fdr, *fdw;
int fd;
struct termio term_save, stdin_save;
void Exit(int sig)
{
   if (fdr) fclose(fdr);
   if (fdw) fclose(fdw);
   ioctl(fd, TCSETA, &term_save);
   close(fd);
   ioctl(fileno(stdin), TCSETA, &stdin_save);
}
main(int argc, char *argv[])
{
   char *b, buffer[80];
   int baud=0, num;
   struct termio term, tstdin;
   int *bufptr;
 
   if ((fd = open("/dev/ttyS3", O_RDWR | O_NDELAY)) < 0)
   {
      perror("/dev/ttyS3");
      return errno;
   }

   /* Save stdin and tty state and trap some signals */
   ioctl(fd, TCGETA, &term_save);
   ioctl(fileno(stdin), TCGETA, &stdin_save);
   signal(SIGHUP, Exit);
   signal(SIGINT, Exit);
   signal(SIGQUIT, Exit);
   signal(SIGTERM, Exit);
   /*  Set stdin to raw mode, no echo */
   ioctl(fileno(stdin), TCGETA, &tstdin);
   tstdin.c_iflag = 0;
   tstdin.c_lflag &= ~(ICANON | ECHO);
   tstdin.c_cc[VMIN] = 0;
   tstdin.c_cc[VTIME] = 0;
   ioctl(fileno(stdin), TCSETA, &tstdin);
   /*  Set tty state */
   ioctl(fd, TCGETA, &term);
   term.c_cflag |= CLOCAL|HUPCL;
   if (baud > 0)
   {
      term.c_cflag &= ~CBAUD;
      term.c_cflag |= baud;
   }
   term.c_lflag &= ~(ICANON | ECHO); /* to force raw mode */
   term.c_iflag &= ~ICRNL; /* to avoid non-needed blank lines */
   term.c_cc[VMIN] = 0;
   term.c_cc[VTIME] = 10;
   ioctl(fd, TCSETA, &term);
   fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NDELAY);
   /*  Open tty for read and write */
   if ((fdr = fopen("/dev/ttyS3", "r")) == NULL )
   {
      perror(argv[1]);
      return errno;
   }
   if ((fdw = fopen("/dev/ttyS3", "w")) == NULL )
   {
      perror("/dev/ttyS3");
      return errno;
   }
   /*  Talk to the modem */
   //puts("Ready... ^C to exit");

   write(fileno(fdw), "AT+WM=2\r\n", sizeof("AT+WM=2\r\n"));
   usleep(40000);
   //while(buffer[bufptr-3]!='O' && buffer[bufptr-2]!='K' && buffer[bufptr-1]!='\r' && buffer[bufptr-2]!='\n') {
	   if ((num = read(fileno(fdr), buffer + (char)bufptr, 80)) > 0) {
		 write(fileno(stdout), buffer, num);
		 bufptr+=num;
                  printf("bufptr = %d, num=%d",bufptr,num);
	   }
  //}
  
  printf("bufptr = %d, num=%d",bufptr,num);
  printf("%s %s %s %s ", buffer[1],buffer[2],buffer[3],buffer[4]);


   //while (1)
   //{
   //   if ((num = read(fileno(stdin), buffer, 80)) > 0)
   //      write(fileno(fdw), buffer, num);
	
   //   if ((num = read(fileno(fdr), buffer, 80)) > 0) {
   //      write(fileno(stdout), buffer, num)
   //	}

   //}
   Exit(0);
} 
