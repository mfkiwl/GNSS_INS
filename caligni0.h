#ifndef CALIGNI0_H
#define CALIGNI0_H
#include <cearth.h>
#include <cquat.h>
#include <cimu.h>

class CAligni0
{
public:
    int velAid, t0, t1, t2;
    CVect3 vel0, wmm, vmm, vib0, vi0, Pib01, Pib02, Pi01, Pi02, tmpPib0, tmpPi0;
    CQuat qib0b;
    CEarth eth;
    CIMU imu;
    double tk;
    CQuat qnb0, qnb, qnbsb;

    CAligni0(const CVect3 &pos0=O31, const CVect3 &vel0=O31, int velAid=0);
    CQuat Update(const CVect3 *wm, const CVect3 *vm, int nSamples, double ts, const CVect3 &vel=O31);
};

#endif // CALIGNI0_H
