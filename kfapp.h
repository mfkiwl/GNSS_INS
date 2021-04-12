#ifndef KFAPP_H
#define KFAPP_H
#include "csinstdkf.h"

typedef struct {
    CVect3 wm, vm;
    double t;
    CVect3 vngps, posgps;
    double gpsValid, dt;
} ImuGpsData;

typedef struct {
    CVect3 att, vn, pos, Patt, Pvn, Ppos;
    double t;
} FXPT;

class CKFApp:public CSINSTDKF
{
public:
    CVect3 lvGPS, vnRes, posRes;
    FXPT xpt;

    CKFApp(double ts);
    virtual void Init(const CSINS &sins0, int grade=-1);
    virtual void SetMeas(void) {}
    void SetMeasGPS(const CVect3 &posGPS, const CVect3 &vnGPS=O31, double dt=0.0);
    int Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts);
    void Reverse(void);
};

void fusion(double *x1, double *p1, const double *x2, const double *p2,
                   int n=9, double *xf=NULL, double *pf=NULL);
#endif // KFAPP_H
