#ifndef CSINSGPSOD_H
#define CSINSGPSOD_H

#include <csinstdkf.h>

class CSINSGPSOD:public CSINSTDKF	// sizeof(CSINSGPSOD)~=30k bytes
{
public:
    CVect3 posDR;
    CMat3 Cbo, MpkD;			// Cbo: from body-frame to OD-frame
    double Kod, tODInt;
    bool measGPSvnValid, measGPSposValid, measODValid, measMAGyawValid;

    CSINSGPSOD(void);
    virtual void Init(const CSINS &sins0, int grade=-1);
    virtual void SetFt(int nnq);
    virtual void SetMeas(void);
    virtual void Feedback(double fbts);
    void SetMeasGPS(const CVect3 &pgps=O31, const CVect3 &vgps=O31);
    void SetMeasOD(double dSod, double ts);
    void SetMeasYaw(double ymag);
    int Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts);
};

#endif // CSINSGPSOD_H
