#include "types.h"

void exit(uint32_t exitcode) {
	asm volatile ("xor %%eax, %%eax\n"
		"int $0x80" :: "c"(exitcode) : "eax");
}

uint32_t fork() {
	uint32_t pid;
	asm volatile ("int $0x80" :"=b"(pid):"a"(1));
	return pid;
}

void sleep(uint32_t ms) {
	ms /= 10;
	asm volatile ("int $0x80" ::"a"(2), "c"(ms));
}

void itoa (char *buf, int base, int d)
     {
       char *p = buf;
       char *p1, *p2;
       unsigned long ud = d;
       int divisor = 10;
     
       /* If %d is specified and D is minus, put `-' in the head. */
       if (base == 'd' && d < 0)
         {
           *p++ = '-';
           buf++;
           ud = -d;
         }
       else if (base == 'x')
         divisor = 16;
     
       /* Divide UD by DIVISOR until UD == 0. */
       do
         {
           int remainder = ud % divisor;
     
           *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
         }
       while (ud /= divisor);
     

       /* Terminate BUF. */
       *p = 0;
     
       /* Reverse BUF. */
       p1 = buf;
       p2 = p - 1;
       while (p1 < p2)
         {
           char tmp = *p1;
           *p1 = *p2;
           *p2 = tmp;
           p1++;
           p2--;
         }
     }

void printf (const char *format, ...) {
	char **arg = (char **) &format;
	int c;
	char outbuf[255];
	int pos = 0;
	char buf[20];

	arg++;

	while ((c = *format++) != 0)
	{
		if (c != '%')
			outbuf[pos++] = c;
		else
		{
			char *p;

			c = *format++;
			switch (c)
			{
				case 'd':
				case 'u':
				case 'x':
					itoa (buf, c, *((int *) arg++));
					p = buf;
					goto string;
				break;

				case 's':
					p = *arg++;
					if (! p)
						p = "(null)";

					string:
					while (*p)
						outbuf[pos++] = *p++;
				break;

				default:
					outbuf[pos++] = (*((int *) arg++));
				break;
			}
		}
	}
	outbuf[pos] = 0;
	asm volatile ("int $0x80" :: "a"(3), "c"(outbuf));
}

int main() {
	if(fork())
		return 0;
	uint32_t pid = fork();
	volatile uint32_t seconds = 0;
	//printnum(99999);
	while(true) {
		sleep(1000);
		printf("I am %s and i am %d seconds in\n", (pid==0?"the child":"the parent"), ++seconds);
		//printnum(++seconds);
	}
	return 0;
}