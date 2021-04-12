#ifndef CVECTOR3_H
#define CVECTOR3_H

#include <basedclass.h>

//
class CVect3
{
public:
    double i, j, k;

    CVect3(void);
    CVect3(double xyz);
    CVect3(double xx, double yy, double zz);
    CVect3(const double *pdata);
    CVect3(const float *pdata);

    CVect3& operator=(double f);							// every element equal to a same double
    CVect3& operator=(const double *pf);					// vector equal to a array
    bool IsZero(const CVect3 &v, double eps=EPS);	// psinsassert if all elements are zeros
    bool IsZeroXY(const CVect3 &v, double eps=EPS);	// psinsassert if x&&y-elements are zeros
    bool IsNaN(const CVect3 &v);						// psinsassert if any element is NaN
    CVect3 operator+(const CVect3 &v) const;				// vector addition
    CVect3 operator-(const CVect3 &v) const;				// vector subtraction
    CVect3 operator*(const CVect3 &v) const;				// vector cross multiplication
    CVect3 operator*(const CMat3 &m) const;					// row-vector multiply matrix
    CVect3 operator*(double f) const;						// vector multiply scale
    CVect3 operator/(double f) const;						// vector divide scale
    CVect3 operator/(const CVect3 &v) const;				// vector divide vect3 element by element
    CVect3& operator+=(const CVect3 &v);					// vector addition
    CVect3& operator-=(const CVect3 &v);					// vector subtraction
    CVect3& operator*=(double f);							// vector multiply scale
    CVect3& operator/=(double f);							// vector divide scale
    CVect3& operator/=(const CVect3 &v);					// vector divide vect3 element by element
//    CVect3 operator-(const CVect3 &v);				// minus

    friend CMat3 vxv(const CVect3 &v1, const CVect3 &v2);	// column-vector multiply row-vector, v1*v2'
    friend CVect3 abs(const CVect3 &v);						// abs
    friend double norm(const CVect3 &v);					// vector norm
    friend double normInf(const CVect3 &v);					// vector inf-norm
    friend double normXY(const CVect3 &v);					// vector norm of X & Y components
    friend CVect3 sqrt(const CVect3 &v);					// sqrt
    friend CVect3 pow(const CVect3 &v, int k);			// power
    friend double dot(const CVect3 &v1, const CVect3 &v2);	// vector dot multiplication
    friend CMat3 a2mat(const CVect3 &att);					// Euler angles to DCM
    friend CVect3 m2att(const CMat3 &Cnb);					// DCM to Euler angles
    friend CQuat a2qua(double pitch, double roll, double yaw);	// Euler angles to quaternion
    friend CQuat a2qua(const CVect3 &att);					// Euler angles to quaternion
    friend CVect3 q2att(const CQuat &qnb);					// quaternion to Euler angles
    friend CQuat rv2q(const CVect3 &rv);					// rotation vector to quaternion
    friend CVect3 q2rv(const CQuat &q);						// quaternion to rotation vector
    friend CMat3 askew(const CVect3 &v);					// askew matrix;
    friend double sinAng(const CVect3 &v1, const CVect3 &v2); // |sin(angle(v1,v2))|
    friend CMat3 pos2Cen(const CVect3 &pos);				// to geographical position matrix
    friend CVect3 pp2vn(const CVect3 &pos1, const CVect3 &pos0, double ts, CEarth pEth);  // position difference to velocity
    friend CVect3 MKQt(const CVect3 &sR, const CVect3 &tau);// first order Markov white-noise variance calculation
    friend CMat3 dv2att(CVect3 &va1, const CVect3 &va2, CVect3 &vb1, const CVect3 &vb2);  // attitude determination using double-vector
    friend CVect3 Alignsb(CVect3 wmm, CVect3 vmm, double latitude);  // align in static-base
    friend double MagYaw(const CVect3 &mag, const CVect3 &att, double declination);
    friend CVect3 xyz2blh(const CVect3 &xyz);				// ECEF X/Y/Z to latitude/longitude/height
    friend CVect3 blh2xyz(const CVect3 &blh);				// latitude/longitude/height to ECEF X/Y/Z
    friend CVect3 Vxyz2enu(const CVect3 &Vxyz, const CVect3 &pos);  // ECEF Vx/Vy/Vz to Ve/Vn/Vu
};

#endif // CVECTOR3_H
