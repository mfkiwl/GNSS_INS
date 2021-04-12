#ifndef CFILERDWT_H
#define CFILERDWT_H
#include <csins.h>
#include <caligni0.h>
#include <cmahony.h>
#include <crmemory.h>
#include <cqeahrs.h>

class CFileRdWt
{
    //文件目录
    static char dirIn[256], dirOut[256];

public:
    //文件指针
    FILE *f;

    //文件名，行，字符串
    char fname[256], line[512], sstr[64*4];
    //内存
    double buff[64];
    //浮点
    float buff32[64];
    //行数  行宽
    int columns, linelen;
    //整个大小
    long totsize, remsize;

    CFileRdWt(void);
    CFileRdWt(const char *fname0, int columns0=0);
    ~CFileRdWt();
    //文件目录
    static void Dir(const char *dirI, const char *dirO=(const char*)NULL);
    //初始化
    void Init(const char *fname0, int columns0=0);
    //读取文件
    int load(int lines=1, bool txtDelComma=1);
    //
    int loadf32(int lines=1);
    long load(BYTE *buf, long bufsize);
    long filesize(int opt=1);
    int getl(void);  // get a line

    CFileRdWt& operator<<(double d);
    CFileRdWt& operator<<(const CVect3 &v);
    CFileRdWt& operator<<(const CQuat &q);
    CFileRdWt& operator<<(const CMat3 &m);
    CFileRdWt& operator<<(const CVect &v);
    CFileRdWt& operator<<(const CMat &m);
    CFileRdWt& operator<<(const CRAvar &R);
    CFileRdWt& operator<<(const CAligni0 &aln);
    CFileRdWt& operator<<(const CSINS &sins);
    CFileRdWt& operator<<(const CRMemory &m);
    CFileRdWt& operator<<(const CMahony &ahrs);
    CFileRdWt& operator<<(const CQEAHRS &ahrs);
    CFileRdWt& operator<<(CKalman &kf);

    CFileRdWt& operator>>(double &d);
    CFileRdWt& operator>>(CVect3 &v);
    CFileRdWt& operator>>(CQuat &q);
    CFileRdWt& operator>>(CMat3 &m);
    CFileRdWt& operator>>(CVect &v);
    CFileRdWt& operator>>(CMat &m);

    friend char* time2fname(void);
};

#endif // CFILERDWT_H
