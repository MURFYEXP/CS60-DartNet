#include <stdio.h>
#include <string.h>

int main()
{
    int buf[3];
    int data[5] = {1, 2, 3, 4, 5};
    memcpy(buf, data, sizeof(data));
    
}