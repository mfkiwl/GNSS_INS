#ifndef CSINS_H
#define CSINS_H
#include <cearth.h>
#include <cquat.h>
#include <cmat3.h>
#include <cimu.h>
#include <cravar.h>

class CSINS	// sizeof(CSINS)~=3k bytes
{
public:
    double ts, nts, tk, velMax, hgtMin, hgtMax;
    CEarth eth;
    CIMU imu;
    CQuat qnb;
    CMat3 Cnb, Cnb0, Cbn, Kg, Ka;
    CVect3 wib, fb, fn, an, web, wnb, att, vn, vb, pos, eb, db, Ka2, tauGyro, tauAcc, _betaGyro, _betaAcc;
    CMat3 Maa, Mav, Map, Mva, Mvv, Mvp, Mpv, Mpp;	// for etm
    CVect3 lvr, vnL, posL; CMat3 CW, MpvCnb;		// for lever arm
    CQuat qnbE; CVect3 attE, vnE, posE;				// for extrapolation
    CRAvar Rwfa;
    bool isOpenloop;

    CSINS(const CVect3 &att0, const CVect3 &vn0=O31, const CVect3 &pos0=O31, double tk0=0.0);
    CSINS(const CQuat &qnb0=qI, const CVect3 &vn0=O31, const CVect3 &pos0=O31, double tk0=0.0);
    void Init(const CQuat &qnb0=qI, const CVect3 &vn0=O31, const CVect3 &pos0=O31, double tk0=0.0);    // initialization using quat attitude, velocity & position
    void SetTauGA(const CVect3 &tauG, const CVect3 &tauA);
    void Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts);		// SINS update using Gyro&Acc samples
    void Extrap(const CVect3 &wm=O31, const CVect3 &vm=O31, double ts=0.0);			// SINS fast extrapolation using 1 Gyro&Acc sample
    void Extrap(double extts);			// SINS fast extrapolation using previous Gyro&Acc sample
    void lever(const CVect3 &dL=O31);		// lever arm
    void etm(void);							// SINS error transform matrix coefficients
};

#endif // CSINS_H
