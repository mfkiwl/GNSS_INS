#ifndef CRAVAR_H
#define CRAVAR_H
#include <cvector3.h>

class CRAvar
{
public:
    #define RAMAX MMD
    int nR0, Rmaxcount[RAMAX], Rmaxflag[RAMAX];
    double ts, R0[RAMAX], Rmax[RAMAX], Rmin[RAMAX], tau[RAMAX], r0[RAMAX];

    CRAvar(void);
    CRAvar(int nR0, int maxCount0=2);
    void set(double r0, double tau, double rmax=0.0, double rmin=0.0, int i=0);
    void set(const CVect3 &r0, const CVect3 &tau, const CVect3 &rmax=O31, const CVect3 &rmin=O31);
    void set(const CVect &r0, const CVect &tau, const CVect &rmax=On1, const CVect &rmin=On1);
    void Update(double r, double ts, int i=0);
    void Update(const CVect3 &r, double ts);
    void Update(const CVect &r, double ts);
    double operator()(int k);			// get element sqrt(R0(k))
};

#endif // CRAVAR_H
