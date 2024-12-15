#include<stdio.h>

void buf_overflow(int n, char val)
{
    volatile char buf[5];
    volatile int a = 0x55;
    buf[0] = 0x5a;
    a++;

    buf[n] = val;

    printf("buf[0] =  0x%x a = 0x%x\n", buf[0], a);

}

int main()
{
    buf_overflow(88888888, 'A');

    return 0;
}
