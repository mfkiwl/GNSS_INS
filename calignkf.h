#ifndef CALIGNKF_H
#define CALIGNKF_H
#include <csinstdkf.h>

class CAlignkf:public CSINSTDKF
{
public:
    int mvnk;
    double mvnts;
    CVect3 mvn, pos0;

    CAlignkf(void);
    virtual void Init(const CSINS &sins0);
    virtual void SetMeas(void) {};
    int Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts);
    int Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts, const CVect3 &vnr);
};;

#endif // CALIGNKF_H
