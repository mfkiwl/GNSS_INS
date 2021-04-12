#ifndef CVECT_H
#define CVECT_H
#include <basedclass.h>

class CVect
{
public:
    int row, clm, rc;
    double dd[MMD];

    CVect(void);
    CVect(int row0, int clm0=1);
    CVect(int row0, double f);
    CVect(int row0, double f, double f1, ...);
    CVect(int row0, const double *pf);
    CVect(const CVect3 &v);
    CVect(const CVect3 &v1, const CVect3 v2);

    void Set(double f, ...);
    void Set2(double f, ...);
    CVect operator+(const CVect &v) const;		// vector addition
    CVect operator-(const CVect &v) const;		// vector subtraction
    CVect operator*(double f) const;			// vector multiply scale
    CVect& operator=(double f);					// every element equal to a same double
    CVect& operator=(const double *pf);			// vector equal to a array
    CVect& operator+=(const CVect &v);			// vector addition
    CVect& operator-=(const CVect &v);			// vector subtraction
    CVect& operator*=(double f);				// vector multiply scale
    CVect operator*(const CMat &m) const;		// row-vector multiply matrix
    CMat operator*(const CVect &v) const;		// 1xn vector multiply nx1 vector, or nx1 vector multiply 1xn vector
    double& operator()(int r);					// vector element

    friend CVect operator~(const CVect &v);
    friend CVect abs(const CVect &v);			// vector abs
    friend double norm(const CVect &v);			// vector norm
    friend double normInf(const CVect &v);		// inf-norm
    friend CVect pow(const CVect &v, int k);	// power
};

#endif // CVECT_H
