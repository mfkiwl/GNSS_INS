#include <QCoreApplication>
#include <kfapp.h>
#include <cfilerdwt.h>

#define FRQ				200
#define TS				(1.0/FRQ)
#define PathIn			"B://NavData/Demo0/"
#define PathOut			"B://NavData/Demo0/"
#define FileIn			"imugps.bin"
#define FileOut1		"posres.bin"
#define FileOut2		"fusion.bin"
#define SkipT			20
#define AlignT			100
#define NavigationT		2400

void fusion(double *x1, double *p1, const double *x2, const double *p2, int n, double *xf, double *pf)
{
    if(xf==NULL) { xf=x1; pf=p1; }
    double *x10, *xf0;
    CVect3 att1;
    if(n<100) {  // n<100 for att(1:3), n>100 for no_att(1:3)
        x10 = x1, xf0 = xf;
        att1 = *(CVect3*)x1;
        *(CVect3*)x2 = qq2phi(a2qua(*(CVect3*)x2),a2qua(att1));
        *(CVect3*)x1 = O31;
    }
    int j;
    for(j=(n>100)?100:0; j<n; j++,x1++,p1++,x2++,p2++,xf++,pf++)
    {
        double p1p2 = *p1+*p2;
        *xf = (*p1**x2 + *p2**x1)/p1p2;
        *pf = *p1**p2/p1p2;
    }
    if(j<100) {
        *(CVect3*)xf0 = q2att(a2qua(att1)+*(CVect3*)xf0);
        if(xf0!=x10) *(CVect3*)x10 = att1;
    }
}

//********主函数*********//
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //静态方法获取文件输入输出目录
    CFileRdWt::Dir(PathIn, PathOut);
    //初始化方法 columns=-15
    CFileRdWt fimu(FileIn,-15), fres(FileOut1);
    //初始化位置
    CVect3 pos0;
    //结构体数据
    //指向 fimu 的 buff的数列头（指向buff数列）
    //也就是  pIMU = fimu.buff
    ImuGpsData *pImuGps = (ImuGpsData*)&fimu.buff[0];

    //读取文件 向后移动 skip*FQR*double 字节
    fimu.load(SkipT*FRQ);

    //找到GPS数据 位置
    while(pImuGps->posgps.i<0.1) fimu.load(1);	pos0 = pImuGps->posgps;

    // initial alignment
    printf("Initial alignment...\n");

    //惯性系初始对准
    CAligni0 aln(pImuGps->posgps);

    //利用跳过的时间进行初始对准
    for(int ii=0; ii<AlignT*FRQ; ii++){
        fimu.load(1);
        aln.Update(&pImuGps->wm, &pImuGps->vm, 1, TS);
    }

    //卡尔曼滤波
    CKFApp kf(TS);
    kf.lvGPS = CVect3(1.0, 0.5, 0.5);
    //初始化kalman
    kf.Init(CSINS(aln.qnb, O31, pos0, pImuGps->t));

    // 读取数据
    printf("Data loading...\n");
    int records = fimu.filesize()/(15*sizeof(double))-(SkipT+AlignT)*FRQ;

    //
    records = min(records, NavigationT*FRQ);
    CRMemory memImu(records, 15*sizeof(double));
    records = fimu.load(memImu.get(0), records*15*sizeof(double)) / (15*sizeof(double));
    CRMemory memFusion(records, sizeof(FXPT));

    // forward filtering
    printf("Forward filtering...\n");
    int i;
    for(i=0; i<records; i++)
    {
        pImuGps = (ImuGpsData*)memImu.get(i);
        kf.Update(&pImuGps->wm, &pImuGps->vm, 1, TS);
        if(pImuGps->gpsValid>0.1 && (pImuGps->t<700 || pImuGps->t>1000))
        {
            kf.SetMeasGPS(pImuGps->posgps, pImuGps->vngps, pImuGps->dt);
        }
        if(i%20==0 || pImuGps->gpsValid>0.1)
        {
            fres <<kf.sins.att<<kf.vnRes<<kf.posRes<<kf.sins.eb<<kf.sins.db<<pImuGps->vngps<<pImuGps->posgps
                 <<kf<<pImuGps->t;
        }
        memFusion.push((BYTE*)&kf.xpt);
        if((i+1)%(10*FRQ)==0) printf("\r\t%d (%d%%)\t", (i+1)/FRQ, 100*(i+1)/records);
    }

    // backward filtering
    kf.Reverse();
    printf("Data length(s) = %.3f.\nBackward filtering...\n", (float)records/FRQ);
    for(i--; i>=0; i--)
    {
        pImuGps = (ImuGpsData*)memImu.get(i);
        pImuGps->wm = (pImuGps->wm.operator*(-1.0));  pImuGps->vngps = pImuGps->vngps.operator*(-1.0);
        if(pImuGps->gpsValid>0.1 && (pImuGps->t<700 || pImuGps->t>1000))
        {
            kf.SetMeasGPS(pImuGps->posgps, pImuGps->vngps, -pImuGps->dt);
        }
        if(i%20==0 || pImuGps->gpsValid>0.1)
        {
            fres <<kf.sins.att<<kf.vnRes<<kf.posRes<<kf.sins.eb<<kf.sins.db<<pImuGps->vngps<<pImuGps->posgps
                 <<kf<<pImuGps->t;
        }
        FXPT *pxpt = (FXPT*)memFusion.get(i);  pxpt->t = pImuGps->t;
        kf.xpt.vn = kf.xpt.vn.operator*(-1.0);
        fusion(&pxpt->att.i, &pxpt->Patt.i, &kf.xpt.att.i, &kf.xpt.Patt.i, 9);
        kf.Update(&pImuGps->wm, &pImuGps->vm, 1, TS);
        if(i%(10*FRQ)==0) printf("\r\t%d (%d%%)\t", i/FRQ, 100-100*i/records);
    }
    CFileRdWt ffsn(FileOut2); ffsn<<memFusion;
    printf("\nFusion finished!\n");


    return a.exec();
}
