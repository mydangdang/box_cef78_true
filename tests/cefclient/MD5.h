#ifndef _MD5_H_
#define _MD5_H_

#include <Windows.h>


typedef struct {
  unsigned long state[4];
  unsigned long count[2];
  unsigned char buffer[64];
} MD5_CTX;

void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned long inputlen);
void MD5Final(unsigned char *digest, MD5_CTX *context);
unsigned char *MD5Full(unsigned char *digest, unsigned char *input, unsigned long inputlen);

PUCHAR WINAPI __MD5_LEGACY_API(
    IN PVOID Input,
    IN UINT32 InputLen,
    OUT PUCHAR Digest
    );

#endif // _MD5_H_

