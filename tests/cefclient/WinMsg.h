#define SHOW_USED_MESSAGES 0
#include "windows.h"

char* GetMessageText(unsigned int msg);

#ifdef SHOW_USED_MESSAGES
void ShowUsedMessages(void);
#endif