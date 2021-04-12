#ifndef CRMEMORY_H
#define CRMEMORY_H
#include "basedclass.h"

#define MAX_RECORD_BYTES 512

class CRMemory
{
public:
    BYTE *pMemStart0, *pMemStart, *pMemEnd;
    int pushLen, popLen, recordLen;
    long memLen, dataLen;
    BYTE *pMemPush, *pMemPop, pushBuf[MAX_RECORD_BYTES], popBuf[MAX_RECORD_BYTES];

    CRMemory(long recordNum, int recordLen0);
    CRMemory(BYTE *pMem, long memLen0, int recordLen0=0);
    ~CRMemory();
    bool push(const BYTE *p=(const BYTE*)NULL);
    BYTE pop(BYTE *p=(BYTE*)NULL);
    BYTE* get(int iframe);
};
#endif // CRMEMORY_H
