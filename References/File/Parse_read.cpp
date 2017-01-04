//
//  main.c
//  测试2
//
//  Created by mac on 12/22/15.
//  Copyright (c) 2015 mac. All rights reserved.
//
#include <iostream>
int main()
{
    char hostname[] = "host1";
    char host1[100], host2[100];
    int cost; FILE *pFile;
    char filename[] = "/Users/mac/Desktop/DartNet/lab6handout/topology/topology.dat";
    pFile = fopen(filename, "r");
    while (fscanf(pFile, "%s %s %d", host1, host2, &cost) != EOF)
    {
        printf("%s: %s %s %d\n", __func__, host1, host2, cost);
    }
    fclose(pFile);
}