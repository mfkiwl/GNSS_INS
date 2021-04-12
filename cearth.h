#ifndef CEARTH_H
#define CEARTH_H

#include <cvector3.h>

class CEarth
{
public:
    double a, b;
    double f, e, e2;
    double wie;

    double sl, sl2, sl4, cl, tl, RMh, RNh, clRNh, f_RMh, f_RNh, f_clRNh;
    CVect3 pos, vn, wnie, wnen, wnin, gn, gcc, *pgn;

    CEarth(double a0=glv.Re, double f0=glv.f, double g0=glv.g0);
    void Update(const CVect3 &pos, const CVect3 &vn=O31);
    CVect3 vn2dpos(const CVect3 &vn, double ts=1.0) const;
};

#endif // CEARTH_H
