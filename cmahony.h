#ifndef CMAHONY_H
#define CMAHONY_H

#include <cquat.h>
#include <cmat3.h>
#include <cvector3.h>

class CMahony
{
public:
    double tk, Kp, Ki;
    CQuat qnb;
    CMat3 Cnb;
    CVect3 exyzInt, ebMax;

    CMahony(double tau=4.0, const CQuat &qnb0=qI);
    void SetTau(double tau=4.0);
    void Update(const CVect3 &wm, const CVect3 &vm, double ts, const CVect3 &mag=O31);
    void Update(const CVect3 &gyro, const CVect3 &acc, const CVect3 &mag, double ts);
};

#endif // CMAHONY_H
