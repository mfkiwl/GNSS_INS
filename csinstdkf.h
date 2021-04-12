#ifndef CSINSTDKF_H
#define CSINSTDKF_H

#include <csinskf.h>

class CSINSTDKF:public CSINSKF
{
public:
    double meantdts, tdts, Pz0, innovation;
    int iter, ifn, adptOKi, measRes, tdStep, maxStep;
    CMat Fk, Pk1;
    CVect Pxz, Qk, Kk, Hi, tmeas;
    CVect3 meanfn;

    CSINSTDKF(int nq0, int nr0);
    void TDReset(void);
    int TDUpdate(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts, int nStep=1);  // Time-Distributed Update

};

#endif // CSINSTDKF_H
