#ifndef CSINSKF_H
#define CSINSKF_H

#include <ckalman.h>
#include <csins.h>

class CSINSKF:public CKalman
{
public:
    CSINS sins;

    CSINSKF(int nq0, int nr0);
    virtual void Init(void) {}
    virtual void Init(const CSINS &sins0, int grade=-1);
    virtual void SetFt(int nnq=15);
    virtual void SetHk(int nnq=15);
    virtual void Feedback(double fbts);

    int Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts);	// KF Time&Meas Update
    void QtMarkovGA(const CVect3 &tauG, const CVect3 &sRG, const CVect3 &tauA, const CVect3 &sRA);

    virtual void Miscellanous(void) {}
    virtual void SecretAttitude(void);
    void SetYaw(double yaw);
};

#endif // CSINSKF_H
