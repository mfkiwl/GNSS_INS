#ifndef CQUAT_H
#define CQUAT_H
#include <basedclass.h>

class CQuat
{
public:
    double q0, q1, q2, q3;

    CQuat(void);
    CQuat(double qq0, double qq1=0.0, double qq2=0.0, double qq3=0.0);
    CQuat(const double *pdata);

    CQuat operator+(const CVect3 &phi) const;	// true quaternion add misalign angles
    CQuat operator-(const CVect3 &phi) const;	// calculated quaternion delete misalign angles
    CVect3 operator-(CQuat &quat) const;		// get misalign angles from calculated quaternion & true quaternion
    CQuat operator*(const CQuat &q) const;		// quaternion multiplication
    CVect3 operator*(const CVect3 &v) const;	// quaternion multiply vector
    CQuat& operator*=(const CQuat &q);			// quaternion multiplication
    CQuat& operator-=(const CVect3 &phi);		// calculated quaternion delete misalign angles
    void SetYaw(double yaw=0.0);				// set Euler angles to designated yaw
    void normlize(CQuat *q);				    // quaternion norm

    friend CQuat operator~(const CQuat &q);	    // quaternion conjugate
    friend CVect3 qq2phi(const CQuat &qcalcu, const CQuat &qreal);
    friend CQuat UpDown(const CQuat &q);		        // Up-Down the quaternion represented attitide
};

#endif // CQUAT_H
