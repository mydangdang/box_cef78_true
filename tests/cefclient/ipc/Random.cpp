/*************************************************************************
        Copyright (c) B.K. Softlab. All rights reserved.

Module Name:

    Random.cpp

Abstract:

    This module contains help routines for random-number generator.

Notes:

*************************************************************************/

#include <windows.h>

#include "Random.h"

static LONG s_RandomSeed = 0;

VOID RandomSeed(VOID)
{
    FILETIME CurrentTime;

    GetSystemTimeAsFileTime(&CurrentTime);
    
    s_RandomSeed = (LONG)(CurrentTime.dwLowDateTime & GetTickCount());
}

UINT32 WINAPI RandomGen(VOID)
{
    LONG RandomValue = InterlockedExchangeAdd((PLONG)&s_RandomSeed, 0);

    RandomValue = RandomValue * 214013 + 2531011;

    InterlockedExchange(&s_RandomSeed, RandomValue);
    
    return (UINT32)RandomValue;
}

VOID WINAPI RandomFill(
    OUT PVOID lpFillBuffer,
    IN UINT32 Size
    )
{
    UINT32 i;
    PUINT32 RandomData = (PUINT32)lpFillBuffer;
    UINT32 Uint32Count = Size / sizeof(UINT32);
    UINT32 TailBytesCount = Size % sizeof(UINT32);

    for (i=0; i < Uint32Count; i++)
    {
        *RandomData = RandomGen();
        RandomData++;
    }
    if (TailBytesCount > 0)
    {
        UINT32 LastBlock = RandomGen();
        
        for (i=0; i < TailBytesCount; i++)
        {
            ((PUCHAR)RandomData)[i] = ((PUCHAR)&LastBlock)[i];
        }
    }
}
