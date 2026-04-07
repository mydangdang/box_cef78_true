#ifndef _RANDOM_H_
#define _RANDOM_H_

VOID RandomSeed(VOID);

UINT32 WINAPI RandomGen(VOID);

VOID WINAPI RandomFill(
    OUT PVOID lpFillBuffer,
    IN UINT32 Size
    );

#endif // _RANDOM_H_

