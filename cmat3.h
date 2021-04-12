#ifndef CMAT3_H
#define CMAT3_H

#include <basedclass.h>

class CMat3
{
public:
    double e00, e01, e02, e10, e11, e12, e20, e21, e22;

    CMat3(void);
    CMat3(double xx, double xy, double xz,
          double yx, double yy, double yz,
          double zx, double zy, double zz );
    CMat3(const CVect3 &v0, const CVect3 &v1, const CVect3 &v2);

    CMat3 operator+(const CMat3 &m) const;
    CMat3 operator-(const CMat3 &m) const;
    CMat3 operator*(const CMat3 &m) const;
    CMat3 operator*(double f) const;
    CVect3 operator*(const CVect3 &v) const;
    CMat3& operator+=(const CMat3 &m);
//    CMat3 operator-(const CMat3 &m);

    friend CMat3 operator~(const CMat3 &m);
    friend CMat3 pow(const CMat3 &m, int k);
    friend double trace(const CMat3 &m);
    friend double det(const CMat3 &m);
    friend CMat3 adj(const CMat3 &m);
    friend CMat3 inv(const CMat3 &m);
    friend CVect3 diag(const CMat3 &m);
    friend CMat3 diag(const CVect3 &v);
    friend CQuat m2qua(const CMat3 &Cnb);
    friend CMat3 q2mat(const CQuat &qnb);
    friend CMat3 foam(const CMat3 &B, int iter);
};

#endif // CMAT3_H
