#include "syscall.h"

int main()
{
    int i;
    for (i=0; i<10; i++)
    {
        Sleep(1000000);
        PrintInt(i);
    }
    return 0;
}