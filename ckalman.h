#ifndef CKALMAN_H
#define CKALMAN_H
#include <cmat.h>
#include <cvect.h>
#include <cravar.h>

class CKalman
{
public:
    double kftk, zfdafa;
    int nq, nr, measflag, measflaglog, measstop;
    CMat Ft, Pk, Hk, Fading;
    CVect Xk, Zk, Qt, Rt, rts, RtTau, measlost, Xmax, Pmax, Pmin, Pset, Zfd, Zfd0,
        Rmax, Rmin, Rbeta, Rb,				// measurement noise R adaptive
        FBTau, FBMax, FBOne, FBXk, FBTotal;	// feedback control
    int Rmaxcount[MMD], Rmaxcount0[MMD];
    CRAvar Ravar();

    CKalman(int nq0, int nr0);
    virtual void Init(void) = 0;				// initialize Qk,Rk,P0...
    virtual void SetFt(int nnq=15) = 0;				// process matrix setting
    virtual void SetHk(int nnq=15) = 0;				// measurement matrix setting
    virtual void SetMeas(void) = 0;				// set measurement
    virtual void Feedback(double fbts)=0;			// feed back

    void RtFading(int i, double fdts);			// Rt growing if no measurment
    void TimeUpdate(double kfts, int fback=1);	// time update
    int MeasUpdate(double fading=1.0);			// measurement update
    int RAdaptive(int i, double r, double Pr); // Rt adaptive
    void RPkFading(int i);						// multiple fading
    void SetMeasFlag(int flag);					// measurement flag setting
    void XPConstrain(void);						// Xk & Pk constrain: -Xmax<Xk<Xmax, Pmin<diag(Pk)<Pmax

//    friend void fusion(double *x1, double *p1, const double *x2, const double *p2,
//                       int n=9, double *xf=NULL, double *pf=NULL);
};
#endif // CKALMAN_H
