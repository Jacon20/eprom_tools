#include <stdio.h>

void main(void)
{
    int c;

    for(c=0; c<80; putchar(255), c++);
    for(c=0; c<=255; putchar(c++));

    exit(0);
}
