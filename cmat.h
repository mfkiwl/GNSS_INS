#ifndef CMAT_H
#define CMAT_H
#include <basedclass.h>

class CMat
{
public:
    int row, clm, rc;
    double dd[MMD2];

    CMat(void);
    CMat(int row0, int clm0);
    CMat(int row0, int clm0, double f);
    CMat(int row0, int clm0, const double *pf);

    void Clear(void);
    void SetDiag(double f, ...);
    void SetDiag2(double f, ...);
    CMat operator+(const CMat &m) const;				// matrix addition
    CMat operator-(const CMat &m) const;				// matrix subtraction
    CMat operator*(double f) const;						// matrix multiply scale
    CVect operator*(const CVect &v) const;				// matrix multiply vector
    CMat operator*(const CMat &m) const;				// matrix multiplication
    CMat& operator+=(const CMat &m0);					// matrix addition
    CMat& operator+=(const CVect &v);					// matrix + diag(vector)
    CMat& operator-=(const CMat &m0);					// matrix subtraction
    CMat& operator*=(double f);							// matrix multiply scale
    CMat& operator++();									// 1.0 + diagonal
    double& operator()(int r, int c=-1);				// get element m(r,c)
    void ZeroRow(int i);								// set i-row to 0
    void ZeroClm(int j);								// set j-column to 0
    void SetRow(int i, double f, ...);					// set i-row from n-double
    void SetRow(int i, const CVect &v);					// set i-row from vector
    void SetClm(int j, double f, ...);					// set j-column from n-double
    void SetClm(int j, const CVect &v);					// set j-column from vector
    CVect GetRow(int i) const;							// get i-row from matrix
    CVect GetClm(int j) const;							// get j-column from matrix
    void SetRowVect3(int i, int j, const CVect3 &v);	// set i-row&j...(j+2)-column from CVect3
    void SetClmVect3(int i, int j, const CVect3 &v);	// set i...(i+2)-row&j-column from CVect3
    void SetDiagVect3(int i, int j, const CVect3 &v);	// m(i,j)=v.i, m(i+1,j+1)=v.j, m(i+2,j+2)=v.k;
    CVect3 GetDiagVect3(int i, int j=-1);				// return CVect3(m(i,j), m(i+1,j+1), m(i+2,j+2))
    void SetAskew(int i, int j, const CVect3 &v);		// set i...(i+2)-row&j...(j+2)-comumn from askew CVect3
    void SetMat3(int i, int j, const CMat3 &m);			// set i...(i+2)-row&j...(j+2)-comumn from CMat3
    CMat3 GetMat3(int i, int j=-1) const;				// get CMat3 from i...(i+2)-row&j...(j+2)-comumn
    void SubAddMat3(int i, int j, const CMat3 &m);		// add i...(i+2)-row&j...(j+2)-comumn with CMat3 m

    friend CMat operator~(const CMat &m);				// matrix transposition
    friend void symmetry(CMat &m);						// matrix symmetrization
    friend double normInf(CMat &m);						// inf-norm
    friend CVect diag(const CMat &m);					// diagonal of a matrix
    friend CMat diag(const CVect &v);					// diagonal matrix
    friend void RowMul(CMat &m, const CMat &m0, const CMat &m1, int r); // m(r,:)=m0(r,:)*m1
    friend void RowMulT(CMat &m, const CMat &m0, const CMat &m1, int r); // m(r,:)=m0(r,:)*m1'
    friend void DVMDVafa(const CVect &V, CMat &M, double afa);	// M = diag(V)*M*diag(V)*afa
};




#endif // CMAT_H
