#include "csinskf.h"
#include "csinstdkf.h"
#include "csinsgpsod.h"
#include "cmahony.h"
#include "cqeahrs.h"
#include "cfilerdwt.h"
#include "calignkf.h"
#include "crmemory.h"
#include "kfapp.h"
const CVect3 I31(1.0), O31(0.0), Ipos(1.0/RE,1.0/RE,1.0);
const CQuat  qI(1.0,0.0,0.0,0.0);
const CMat3  I33(1,0,0, 0,1,0, 0,0,1), O33(0,0,0, 0,0,0, 0,0,0);
const CVect  On1(MMD,0.0), O1n=~On1;

bool psinsassert(bool b)
{
    int res;
    if(b)	{
        res = 1;
    }
    else	{
        res = 0;
    }
    return res;
}

double r2dm(double r)	// rad to deg/min, eg. 1234.56 = 12deg+34.56min
{
    int sgn=1;
    if(r<0.0) { r=-r; sgn=0; }
    double deg = r/DEG;
    int ideg = (int)deg;
    double dm = ideg*100 + (deg-ideg)*60.0;
    return sgn ? dm : -dm;
}

double dm2r(double dm)
{
    int sgn=1;
    if(dm<0.0) { dm=-dm; sgn=0; }
    int ideg = (int)(dm/100);
    double r = ideg*DEG + (dm-ideg*100)*(DEG/60);
    return sgn ? r : -r;
}

bool IsZero(double f, double eps)
{
    return (f<eps && f>-eps);
}

// determine the sign of 'val' with the sensitivity of 'eps'
int sign(double val, double eps)
{
    int s;

    if(val<-eps)
    {
        s = -1;
    }
    else if(val>eps)
    {
        s = 1;
    }
    else
    {
        s = 0;
    }
    return s;
}

// set double value 'val' between range 'minVal' and 'maxVal'
double range(double val, double minVal, double maxVal)
{
    double res;

    if(val<minVal)
    {
        res = minVal;
    }
    else if(val>maxVal)
    {
        res = maxVal;
    }
    else
    {
        res = val;
    }
    return res;
}

double atan2Ex(double y, double x)
{
    double res;

    if((sign(y)==0) && (sign(x)==0))
    {
        res = 0.0;
    }
    else
    {
        res = atan2(y, x);
    }
    return res;
}

double diffYaw(double yaw, double yaw0)
{
    double dyaw = yaw-yaw0;
    if(dyaw>=PI) dyaw-=_2PI;
    else if(dyaw<=-PI) dyaw+=_2PI;
    return dyaw;
}

double MKQt(double sR, double tau)
{
    return sR*sR*2.0/tau;
}

CVect3 MKQt(const CVect3 &sR, const CVect3 &tau)
{
    return CVect3(sR.i*sR.i*2.0/tau.i, sR.j*sR.j*2.0/tau.j, sR.k*sR.k*2.0/tau.k);
}

double randn(double mu, double sigma)
{
#define NSUM 25
static double ssgm = sqrt(NSUM/12.0);
    double x = 0;
    for (int i=0; i<NSUM; i++)
    {
        x += (double)rand();
    }
    x /= RAND_MAX;
    x -= NSUM/2.0;  x += mu;
    x /= ssgm;		x *= sigma;
    return x;
}

CVect3 randn(const CVect3 &mu, const CVect3 &sigma)
{
    return CVect3(randn(mu.i, sigma.i), randn(mu.j, sigma.j), randn(mu.k, sigma.k));
}

/***************************  class CGLV  *********************************/
/*************************** ????????????  ************************************/
CGLV::CGLV(double Re, double f, double wie0, double g0)
{
    this->Re = Re; this->f = f; this->wie = wie0; this->g0 = g0;
    Rp = (1-f)*Re;
    e = sqrt(2*f-f*f); e2 = e*e;
    ep = sqrt(Re*Re-Rp*Rp)/Rp; ep2 = ep*ep;
    mg = g0/1000.0;
    ug = mg/1000.0;
    deg = PI/180.0;
    min = deg/60.0;
    sec = min/60.0;
    ppm = 1.0e-6;
    hur = 3600.0;
    dps = deg/1.0;
    dph = deg/hur;
    dpsh = deg/sqrt(hur);
    dphpsh = dph/sqrt(hur);
    ugpsHz = ug/sqrt(1.0);
    ugpsh = ug/sqrt(hur);
    ugpg2 = ug/g0/g0;
    mpsh = 1/sqrt(hur);
    mpspsh = 1/1/sqrt(hur);
    ppmpsh = ppm/sqrt(hur);
    secpsh = sec/sqrt(hur);
}

/*************************** end  ************************************/
/***************************  class CVect3  *********************************/
/*************************** ????????????  ************************************/
CVect3::CVect3(void)
{
}

CVect3::CVect3(double xyz)
{
    i=j=k=xyz;
}

CVect3::CVect3(double xx, double yy, double zz)
{
    i=xx;
    j=yy;
    k=zz;
}

CVect3::CVect3(const double *pdata)
{
    i=*pdata++;
    j=*pdata++;
    k=*pdata;
}

CVect3::CVect3(const float *pdata)
{
    i=(double)*pdata++;
    j=(double)*pdata++;
    k=(double)*pdata;
}

bool IsZero(const CVect3 &v, double eps)
{
    return (v.i<eps&&v.i>-eps && v.j<eps&&v.j>-eps && v.k<eps&&v.k>-eps);
}

bool sZeroXY(const CVect3 &v, double eps)
{
    return (v.i<eps&&v.i>-eps && v.j<eps&&v.j>-eps);
}

bool IsNaN(const CVect3 &v)
{
    return 0; //(_isnan(i) || _isnan(j) || _isnan(k));
}

CVect3 CVect3::operator+(const CVect3 &v) const
{
    return CVect3(this->i+v.i, this->j+v.j, this->k+v.k);
}

CVect3 CVect3::operator-(const CVect3 &v) const
{
    return CVect3(this->i-v.i, this->j-v.j, this->k-v.k);
}

CVect3 CVect3::operator*(const CVect3 &v) const
{
    return CVect3(this->j*v.k-this->k*v.j, this->k*v.i-this->i*v.k, this->i*v.j-this->j*v.i);
}

CVect3 CVect3::operator*(double f) const
{
    return CVect3(i*f, j*f, k*f);
}

CVect3 CVect3::operator*(const CMat3 &m) const
{
    return CVect3(i*m.e00+j*m.e10+k*m.e20,i*m.e01+j*m.e11+k*m.e21,i*m.e02+j*m.e12+k*m.e22);
}

CVect3 CVect3::operator/(double f) const
{
    return CVect3(i/f, j/f, k/f);
}

CVect3 CVect3::operator/(const CVect3 &v) const
{
    return CVect3(i/v.i, j/v.j, k/v.k);
}

CVect3& CVect3::operator=(double f)
{
    i = j = k = f;
    return *this;
}

CVect3& CVect3::operator=(const double *pf)
{
    i = *pf++;
    j = *pf++;
    k = *pf;
    return *this;
}

CVect3& CVect3::operator+=(const CVect3 &v)
{
    i += v.i;
    j += v.j;
    k += v.k;
    return *this;
}

CVect3& CVect3::operator-=(const CVect3 &v)
{
    i -= v.i;
    j -= v.j;
    k -= v.k;
    return *this;
}

CVect3& CVect3::operator*=(double f)
{
    i *= f;
    j *= f;
    k *= f;
    return *this;
}

CVect3& CVect3::operator/=(double f)
{
    i /= f;
    j /= f;
    k /= f;
    return *this;
}

CVect3& CVect3::operator/=(const CVect3 &v)
{
    i /= v.i;
    j /= v.j;
    k /= v.k;
    return *this;
}

CVect3 operator*(double f, const CVect3 &v)
{
    return CVect3(v.i*f, v.j*f, v.k*f);
}

//CVect3 operator-(const CVect3 &v)
//{
//    return CVect3(-v.i, -v.j, -v.k);
//}

CMat3 vxv(const CVect3 &v1, const CVect3 &v2)
{
    return CMat3(v1.i*v2.i, v1.i*v2.j, v1.i*v2.k,
                 v1.j*v2.i, v1.j*v2.j, v1.j*v2.k,
                 v1.k*v2.i, v1.k*v2.j, v1.k*v2.k);
}

CVect3 sqrt(const CVect3 &v)
{
    return CVect3(sqrt(v.i), sqrt(v.j), sqrt(v.k));
}

CVect3 pow(const CVect3 &v, int k)
{
    CVect3 pp = v;
    for(int i=1; i<k; i++)
    {
        pp.i *= v.i;
        pp.j *= v.j;
        pp.k *= v.k;
    }
    return pp;
}

CVect3 abs(const CVect3 &v)
{
    CVect3 res;
    res.i = v.i>0.0 ? v.i : -v.i;
    res.j = v.j>0.0 ? v.j : -v.j;
    res.k = v.k>0.0 ? v.k : -v.k;
    return res;
}

double norm(const CVect3 &v)
{
    return sqrt(v.i*v.i + v.j*v.j + v.k*v.k);
}

double normInf(const CVect3 &v)
{
    double i = v.i>0 ? v.i : -v.i,
           j = v.j>0 ? v.j : -v.j,
           k = v.k>0 ? v.k : -v.k;
    if(i>j)	return i>k ? i : k;
    else    return j>k ? j : k;
}

double normXY(const CVect3 &v)
{
    return sqrt(v.i*v.i + v.j*v.j);
}

double dot(const CVect3 &v1, const CVect3 &v2)
{
    return (v1.i*v2.i + v1.j*v2.j + v1.k*v2.k);
}

CQuat rv2q(const CVect3 &rv)
{
#define F1	(   2 * 1)		// define: Fk=2^k*k!
#define F2	(F1*2 * 2)
#define F3	(F2*2 * 3)
#define F4	(F3*2 * 4)
#define F5	(F4*2 * 5)
    double n2 = rv.i*rv.i+rv.j*rv.j+rv.k*rv.k, c, f;
    if(n2<(PI/180.0*PI/180.0))	// 0.017^2
    {
        double n4=n2*n2;
        c = 1.0 - n2*(1.0/F2) + n4*(1.0/F4);
        f = 0.5 - n2*(1.0/F3) + n4*(1.0/F5);
    }
    else
    {
        double n_2 = sqrt(n2)/2.0;
        c = cos(n_2);
        f = sin(n_2)/n_2*0.5;
    }
    return CQuat(c, f*rv.i, f*rv.j, f*rv.k);
}

CMat3 askew(const CVect3 &v)
{
    return CMat3(0,  -v.k, v.j,
                 v.k, 0.0,  -v.i,
                -v.j, v.i, 0);
}

CMat3 pos2Cen(const CVect3 &pos)
{
    double si = sin(pos.i), ci = cos(pos.i), sj = sin(pos.j), cj = cos(pos.j);
    return CMat3(	-sj, -si*cj,  ci*cj,
                     cj, -si*sj,  ci*sj,
                     0,   ci,     si      );	//Cen
}

CVect3 pp2vn(const CVect3 &pos1, const CVect3 &pos0, double ts, CEarth *pEth)
{
    double sl, cl, sl2, sq, sq2, RMh, RNh, clRNh;
    if(pEth)
    {
        RMh = pEth->RMh; clRNh = pEth->clRNh;
    }
    else
    {
        sl=sin(pos0.i); cl=cos(pos0.i); sl2=sl*sl;
        sq = 1-glv.e2*sl2; sq2 = sqrt(sq);
        RMh = glv.Re*(1-glv.e2)/sq/sq2+pos0.k;
        RNh = glv.Re/sq2+pos0.k;    clRNh = cl*RNh;
    }
    CVect3 vn = pos1 - pos0;
    return CVect3(vn.j*clRNh/ts, vn.i*RMh/ts, vn.k/ts);
}

double sinAng(const CVect3 &v1, const CVect3 &v2)
{
    if(IsZero(v1,EPS)||IsZero(v2,EPS)) return 0.0;
    return norm(v1*v2)/(norm(v1)*norm(v2));
}

double MagYaw(const CVect3 &mag, const CVect3 &att, double declination)
{
    CVect3 attH(att.i, att.j, 0.0);
    CVect3 magH = a2mat(attH)*mag;
    double yaw = 0.0;
    if(attH.i<(80.0*DEG)&&attH.i>-(80.0*DEG))
    {
        yaw = atan2Ex(magH.i, magH.j) + declination;
        if(yaw>PI)       yaw -= _2PI;
        else if(yaw<-PI) yaw += _2PI;
    }
    return yaw;
}

CVect3 xyz2blh(const CVect3 &xyz)
{
    double s = normXY(xyz), theta = atan2(xyz.k*glv.Re, s*glv.Rp),
        s3 = sin(theta), c3 = cos(theta);
    s3 = s3*s3*s3;
    c3 = c3*c3*c3;
    if(s<(6378137.0*1.0*DEG))  return O31;
    double L = atan2(xyz.j, xyz.i), B = atan2(xyz.k+glv.ep2*glv.Rp*s3, s-glv.e2*glv.Re*c3),
        sB = sin(B), cB = cos(B), N = glv.Re/sqrt(1-glv.e2*sB*sB);
    return CVect3(B, L, s/cB-N);
}

CVect3 blh2xyz(const CVect3 &blh)
{
    double sB = sin(blh.i), cB = cos(blh.i), sL = sin(blh.j), cL = cos(blh.j),
        N = glv.Re/sqrt(1-glv.e2*sB*sB);
    return CVect3((N+blh.k)*cB*cL, (N+blh.k)*cB*sL, (N*(1-glv.e2)+blh.k)*sB);
}

CVect3 Vxyz2enu(const CVect3 &Vxyz, const CVect3 &pos)
{
    return Vxyz*pos2Cen(pos);
}
/*************************** end  ************************************/


/***************************  class CQuat  *********************************/
/*************************** ?????????  ************************************/
CQuat::CQuat(void)
{
}

CQuat::CQuat(double qq0, double qq1, double qq2, double qq3)
{
    q0=qq0;
    q1=qq1;
    q2=qq2;
    q3=qq3;
}

CQuat::CQuat(const double *pdata)
{
    q0=*pdata++;
    q1=*pdata++;
    q2=*pdata++;
    q3=*pdata++;
}

CQuat CQuat::operator+(const CVect3 &phi) const
{
    CQuat qtmp = rv2q(-1.0*phi);
    return qtmp*(*this);
}

CQuat CQuat::operator-(const CVect3 &phi) const
{
    CQuat qtmp = rv2q(phi);
    return qtmp*(*this);
}

CVect3 CQuat::operator-(CQuat &quat) const
{
    CQuat dq;

    dq = quat*(~(*this));
    if(dq.q0<0)
    {
        dq.q0=-dq.q0;
        dq.q1=-dq.q1;
        dq.q2=-dq.q2;
        dq.q3=-dq.q3;
    }
    double n2 = acos(dq.q0), f;
    if( sign(n2)!=0 )
    {
        f = 2.0/(sin(n2)/n2);
    }
    else
    {
        f = 2.0;
    }
    return CVect3(dq.q1,dq.q2,dq.q3)*f;
}

CQuat CQuat::operator*(const CQuat &quat) const
{
    CQuat qtmp;
    qtmp.q0 = q0*quat.q0 - q1*quat.q1 - q2*quat.q2 - q3*quat.q3;
    qtmp.q1 = q0*quat.q1 + q1*quat.q0 + q2*quat.q3 - q3*quat.q2;
    qtmp.q2 = q0*quat.q2 + q2*quat.q0 + q3*quat.q1 - q1*quat.q3;
    qtmp.q3 = q0*quat.q3 + q3*quat.q0 + q1*quat.q2 - q2*quat.q1;
    return qtmp;
}

CQuat& CQuat::operator*=(const CQuat &quat)
{
    return (*this=*this*quat);
}

CQuat& CQuat::operator-=(const CVect3 &phi)
{
    CQuat qtmp = rv2q(phi);
    return (*this=qtmp*(*this));
}

CQuat operator~(const CQuat &q)
{
    return CQuat(q.q0,-q.q1,-q.q2,-q.q3);
}

CVect3 CQuat::operator*(const CVect3 &v) const
{
    CQuat qtmp;
    CVect3 vtmp;
    qtmp.q0 =         - q1*v.i - q2*v.j - q3*v.k;
    qtmp.q1 = q0*v.i           + q2*v.k - q3*v.j;
    qtmp.q2 = q0*v.j           + q3*v.i - q1*v.k;
    qtmp.q3 = q0*v.k           + q1*v.j - q2*v.i;
    vtmp.i = -qtmp.q0*q1 + qtmp.q1*q0 - qtmp.q2*q3 + qtmp.q3*q2;
    vtmp.j = -qtmp.q0*q2 + qtmp.q2*q0 - qtmp.q3*q1 + qtmp.q1*q3;
    vtmp.k = -qtmp.q0*q3 + qtmp.q3*q0 - qtmp.q1*q2 + qtmp.q2*q1;
    return vtmp;
}

void CQuat::SetYaw(double yaw)
{
    CVect3 att = q2att(*this);
    att.k = yaw;
    *this = a2qua(att);
}

void normlize(CQuat *q)
{
    double nq=sqrt(q->q0*q->q0+q->q1*q->q1+q->q2*q->q2+q->q3*q->q3);
    q->q0 /= nq;
    q->q1 /= nq;
    q->q2 /= nq;
    q->q3 /= nq;
}

CVect3 q2rv(const CQuat &q)
{
    CQuat dq;
    dq = q;
    if(dq.q0<0){
        dq.q0=-dq.q0;
        dq.q1=-dq.q1;
        dq.q2=-dq.q2;
        dq.q3=-dq.q3;
    }
    if(dq.q0>1.0) dq.q0=1.0;
    double n2 = acos(dq.q0), f;
    if(n2>1.0e-20){
        f = 2.0/(sin(n2)/n2);
    }
    else{
        f = 2.0;
    }
    return CVect3(dq.q1,dq.q2,dq.q3)*f;
}

CVect3 qq2phi(const CQuat &qcalcu, const CQuat &qreal)
{
    return q2rv(qreal*(~qcalcu));
}

CQuat UpDown(const CQuat &q)
{
    CVect3 att = q2att(q);
    att.i = -att.i; att.j += PI;
    return a2qua(att);
}
/*************************** end  ************************************/


/***************************  class CMat3  *********************************/
/*************************** ???????????????  ************************************/
CMat3::CMat3(void)
{
}

CMat3::CMat3(double xx, double xy, double xz,
          double yx, double yy, double yz,
          double zx, double zy, double zz ){
    e00=xx;e01=xy;e02=xz;
    e10=yx;e11=yy;e12=yz;
    e20=zx;e21=zy;e22=zz;
}

CMat3::CMat3(const CVect3 &v0, const CVect3 &v1, const CVect3 &v2){
    e00 = v0.i; e01 = v0.j; e02 = v0.k;
    e10 = v1.i; e11 = v1.j; e12 = v1.k;
    e20 = v2.i; e21 = v2.j; e22 = v2.k;
}

CMat3 dv2att(CVect3 &va1, const CVect3 &va2, CVect3 &vb1, const CVect3 &vb2)
{
    CVect3 a=va1*va2, b=vb1*vb2, aa=a*va1, bb=b*vb1;
    if(IsZero(va1,EPS)||IsZero(a,EPS)||IsZero(aa,EPS)||IsZero(vb1,EPS)||IsZero(b,EPS)||IsZero(bb,EPS)) return I33;
    CMat3 Ma(va1/norm(va1),a/norm(a),aa/norm(aa)), Mb(vb1/norm(vb1),b/norm(b),bb/norm(bb));
    return (~Ma)*(Mb);  //Cab
}

//CMat3 operator-(const CMat3 &m)
//{
//    return CMat3(-m.e00,-m.e01,-m.e02,-m.e10,-m.e11,-m.e12,-m.e20,-m.e21,-m.e22);
//}

CMat3 operator~(const CMat3 &m)
{
    return CMat3(m.e00,m.e10,m.e20, m.e01,m.e11,m.e21, m.e02,m.e12,m.e22);
}

CMat3 CMat3::operator*(const CMat3 &mat) const
{
    CMat3 mtmp;
    mtmp.e00 = e00*mat.e00 + e01*mat.e10 + e02*mat.e20;
    mtmp.e01 = e00*mat.e01 + e01*mat.e11 + e02*mat.e21;
    mtmp.e02 = e00*mat.e02 + e01*mat.e12 + e02*mat.e22;
    mtmp.e10 = e10*mat.e00 + e11*mat.e10 + e12*mat.e20;
    mtmp.e11 = e10*mat.e01 + e11*mat.e11 + e12*mat.e21;
    mtmp.e12 = e10*mat.e02 + e11*mat.e12 + e12*mat.e22;
    mtmp.e20 = e20*mat.e00 + e21*mat.e10 + e22*mat.e20;
    mtmp.e21 = e20*mat.e01 + e21*mat.e11 + e22*mat.e21;
    mtmp.e22 = e20*mat.e02 + e21*mat.e12 + e22*mat.e22;
    return mtmp;
}

CMat3 CMat3::operator+(const CMat3 &mat) const
{
    CMat3 mtmp;
    mtmp.e00 = e00 + mat.e00;  mtmp.e01 = e01 + mat.e01;  mtmp.e02 = e02 + mat.e02;
    mtmp.e10 = e10 + mat.e10;  mtmp.e11 = e11 + mat.e11;  mtmp.e12 = e12 + mat.e12;
    mtmp.e20 = e20 + mat.e20;  mtmp.e21 = e21 + mat.e21;  mtmp.e22 = e22 + mat.e22;
    return mtmp;
}

CMat3& CMat3::operator+=(const CMat3 &mat)
{
    this->e00 += mat.e00;  this->e01 += mat.e01;  this->e02 += mat.e02;
    this->e10 += mat.e10;  this->e11 += mat.e11;  this->e12 += mat.e12;
    this->e20 += mat.e20;  this->e21 += mat.e21;  this->e22 += mat.e22;
    return *this;
}

CMat3 CMat3::operator-(const CMat3 &mat) const
{
    CMat3 mtmp;
    mtmp.e00 = e00 - mat.e00;  mtmp.e01 = e01 - mat.e01;  mtmp.e02 = e02 - mat.e02;
    mtmp.e10 = e10 - mat.e10;  mtmp.e11 = e11 - mat.e11;  mtmp.e12 = e12 - mat.e12;
    mtmp.e20 = e20 - mat.e20;  mtmp.e21 = e21 - mat.e21;  mtmp.e22 = e22 - mat.e22;
    return mtmp;
}

CMat3 CMat3::operator*(double f) const
{
    return CMat3(e00*f,e01*f,e02*f, e10*f,e11*f,e12*f, e20*f,e21*f,e22*f);
}

CMat3 operator*(double f, const CMat3 &m)
{
    return CMat3(m.e00*f,m.e01*f,m.e02*f, m.e10*f,m.e11*f,m.e12*f, m.e20*f,m.e21*f,m.e22*f);
}

CVect3 CMat3::operator*(const CVect3 &v) const
{
    return CVect3(e00*v.i+e01*v.j+e02*v.k,e10*v.i+e11*v.j+e12*v.k,e20*v.i+e21*v.j+e22*v.k);
}

double det(const CMat3 &m)
{
    return m.e00*(m.e11*m.e22-m.e12*m.e21) - m.e01*(m.e10*m.e22-m.e12*m.e20) + m.e02*(m.e10*m.e21-m.e11*m.e20);
}

double trace(const CMat3 &m)
{
    return (m.e00+m.e11+m.e22);
}

CMat3 pow(const CMat3 &m, int k)
{
    CMat3 mm = m;
    for(int i=1; i<k; i++)	mm = mm*m;
    return mm;
}

CQuat a2qua(double pitch, double roll, double yaw)
{
    pitch /= 2.0;
    roll /= 2.0;
    yaw /= 2.0;
    double	sp = sin(pitch), sr = sin(roll), sy = sin(yaw),
            cp = cos(pitch), cr = cos(roll), cy = cos(yaw);
    CQuat qnb;
    qnb.q0 = cp*cr*cy - sp*sr*sy;
    qnb.q1 = sp*cr*cy - cp*sr*sy;
    qnb.q2 = cp*sr*cy + sp*cr*sy;
    qnb.q3 = cp*cr*sy + sp*sr*cy;
    return qnb;
}

CQuat a2qua(const CVect3 &att)
{
    return a2qua(att.i, att.j, att.k);
}

CMat3 a2mat(const CVect3 &att)
{
    double	si = sin(att.i), ci = cos(att.i),
            sj = sin(att.j), cj = cos(att.j),
            sk = sin(att.k), ck = cos(att.k);
    CMat3 Cnb;
    Cnb.e00 =  cj*ck - si*sj*sk;	Cnb.e01 =  -ci*sk;	Cnb.e02 = sj*ck + si*cj*sk;
    Cnb.e10 =  cj*sk + si*sj*ck;	Cnb.e11 =  ci*ck;	Cnb.e12 = sj*sk - si*cj*ck;
    Cnb.e20 = -ci*sj;				Cnb.e21 =  si;		Cnb.e22 = ci*cj;
    return Cnb;
}

CVect3 m2att(const CMat3 &Cnb)
{
    CVect3 att;
    att.i = asinEx(Cnb.e21);
    att.j = atan2Ex(-Cnb.e20, Cnb.e22);
    att.k = atan2Ex(-Cnb.e01, Cnb.e11);
    return att;
}

CQuat m2qua(const CMat3 &Cnb)
{
    double q0, q1, q2, q3, qq4;
    if(Cnb.e00>=Cnb.e11+Cnb.e22)
    {
        q1 = 0.5*sqrt(1+Cnb.e00-Cnb.e11-Cnb.e22);  qq4 = 4*q1;
        q0 = (Cnb.e21-Cnb.e12)/qq4; q2 = (Cnb.e01+Cnb.e10)/qq4; q3 = (Cnb.e02+Cnb.e20)/qq4;
    }
    else if(Cnb.e11>=Cnb.e00+Cnb.e22)
    {
        q2 = 0.5*sqrt(1-Cnb.e00+Cnb.e11-Cnb.e22);  qq4 = 4*q2;
        q0 = (Cnb.e02-Cnb.e20)/qq4; q1 = (Cnb.e01+Cnb.e10)/qq4; q3 = (Cnb.e12+Cnb.e21)/qq4;
    }
    else if(Cnb.e22>=Cnb.e00+Cnb.e11)
    {
        q3 = 0.5*sqrt(1-Cnb.e00-Cnb.e11+Cnb.e22);  qq4 = 4*q3;
        q0 = (Cnb.e10-Cnb.e01)/qq4; q1 = (Cnb.e02+Cnb.e20)/qq4; q2 = (Cnb.e12+Cnb.e21)/qq4;
    }
    else
    {
        q0 = 0.5*sqrt(1+Cnb.e00+Cnb.e11+Cnb.e22);  qq4 = 4*q0;
        q1 = (Cnb.e21-Cnb.e12)/qq4; q2 = (Cnb.e02-Cnb.e20)/qq4; q3 = (Cnb.e10-Cnb.e01)/qq4;
    }
    double nq = sqrt(q0*q0+q1*q1+q2*q2+q3*q3);
    q0 /= nq; q1 /= nq; q2 /= nq; q3 /= nq;
    return CQuat(q0, q1, q2, q3);
}

CVect3 q2att(const CQuat &qnb)
{
    double	q11 = qnb.q0*qnb.q0, q12 = qnb.q0*qnb.q1, q13 = qnb.q0*qnb.q2, q14 = qnb.q0*qnb.q3,
            q22 = qnb.q1*qnb.q1, q23 = qnb.q1*qnb.q2, q24 = qnb.q1*qnb.q3,
            q33 = qnb.q2*qnb.q2, q34 = qnb.q2*qnb.q3,
            q44 = qnb.q3*qnb.q3;
    CVect3 att;
    att.i = asinEx(2*(q34+q12));
    att.j = atan2Ex(-2*(q24-q13), q11-q22-q33+q44);
    att.k = atan2Ex(-2*(q23-q14), q11-q22+q33-q44);
    return att;
}

CMat3 q2mat(const CQuat &qnb)
{
    double	q11 = qnb.q0*qnb.q0, q12 = qnb.q0*qnb.q1, q13 = qnb.q0*qnb.q2, q14 = qnb.q0*qnb.q3,
            q22 = qnb.q1*qnb.q1, q23 = qnb.q1*qnb.q2, q24 = qnb.q1*qnb.q3,
            q33 = qnb.q2*qnb.q2, q34 = qnb.q2*qnb.q3,
            q44 = qnb.q3*qnb.q3;
    CMat3 Cnb;
    Cnb.e00 = q11+q22-q33-q44;  Cnb.e01 = 2*(q23-q14);     Cnb.e02 = 2*(q24+q13);
    Cnb.e10 = 2*(q23+q14);      Cnb.e11 = q11-q22+q33-q44; Cnb.e12 = 2*(q34-q12);
    Cnb.e20 = 2*(q24-q13);      Cnb.e21 = 2*(q34+q12);     Cnb.e22 = q11-q22-q33+q44;
    return Cnb;
}

CMat3 adj(const CMat3 &m)
{
    CMat3 mtmp;
    mtmp.e00 =  (m.e11*m.e22-m.e12*m.e21);
    mtmp.e10 = -(m.e10*m.e22-m.e12*m.e20);
    mtmp.e20 =  (m.e10*m.e21-m.e11*m.e20);
    mtmp.e01 = -(m.e01*m.e22-m.e02*m.e21);
    mtmp.e11 =  (m.e00*m.e22-m.e02*m.e20);
    mtmp.e21 = -(m.e00*m.e21-m.e01*m.e20);
    mtmp.e02 =  (m.e01*m.e12-m.e02*m.e11);
    mtmp.e12 = -(m.e00*m.e12-m.e02*m.e10);
    mtmp.e22 =  (m.e00*m.e11-m.e01*m.e10);
    return mtmp;
}

CMat3 inv(const CMat3 &m)
{
    CMat3 adjm = adj(m);
    double detm = m.e00*adjm.e00 + m.e01*adjm.e10 + m.e02*adjm.e20;
    return adjm*(1.0/detm);
}

CVect3 diag(const CMat3 &m)
{
    return CVect3(m.e00, m.e11, m.e22);
}

CMat3 diag(const CVect3 &v)
{
    return CMat3(v.i,0,0, 0,v.j,0, 0,0,v.k);
}

CMat3 foam(const CMat3 &B, int iter)
{
    CMat3 adjBp=adj(~B), BBp=B*(~B);
    double detB=det(B), adjBp2=trace(adjBp*(~adjBp)), B2=trace(BBp),
        lambda=sqrt(3*B2), lambda2=0, kappa=0, zeta=0, Psi=0, dPsi=0, dlambda=0;
    psinsassert(detB>0.0);
    for(int k=1; k<iter; k++)
    {
        lambda2 = lambda*lambda;
        kappa = (lambda2-B2)/2.0;
        zeta = kappa*lambda - detB;
        Psi = (lambda2-B2); Psi = Psi*Psi - 8.0*lambda*detB - 4.0*adjBp2;
        dPsi = 8.0*zeta;
        dlambda = Psi / dPsi;
        lambda = lambda - dlambda;
        if(fabs(dlambda/lambda)<1.0e-15) break;
    }
    BBp-B;
    return ((kappa+B2)*B+lambda*adjBp-BBp*B) * (1.0/zeta);
}
/*************************** end  ************************************/

/*************************** ?????????  ************************************/
/***************************  class CMat  *********************************/
CMat::CMat(void)
{
}

CMat::CMat(int row0, int clm0)
{
    row=row0; clm=clm0; rc=row*clm;
}

CMat::CMat(int row0, int clm0, double f)
{
    row=row0; clm=clm0; rc=row*clm;
    for(double *pd=dd, *pEnd=&dd[rc]; pd<pEnd; pd++)  *pd = f;
}

CMat::CMat(int row0, int clm0, const double *pf)
{
    row=row0; clm=clm0; rc=row*clm;
    memcpy(dd, pf, rc*sizeof(double));
}

void CMat::Clear(void)
{
    for(double *p=dd, *pEnd=&dd[rc]; p<pEnd; p++)	*p = 0.0;
}

CMat CMat::operator*(const CMat &m0) const
{
    psinsassert(this->clm==m0.row);
    CMat mtmp(this->row,m0.clm);
    int m=this->row, k=this->clm, n=m0.clm;
    double *p=mtmp.dd; const double *p1i=this->dd, *p2=m0.dd;
    for(int i=0; i<m; i++,p1i+=k)
    {
        for(int j=0; j<n; j++)
        {
            double f=0.0; const double *p1is=p1i, *p1isEnd=&p1i[k], *p2sj=&p2[j];
            for(; p1is<p1isEnd; p1is++,p2sj+=n)
                f += (*p1is) * (*p2sj);
            *p++ = f;
        }
    }
    return mtmp;
}

CVect CMat::operator*(const CVect &v) const
{
    psinsassert(this->clm==v.row);
    CVect vtmp(this->row);
    double *p=vtmp.dd, *pEnd=&vtmp.dd[vtmp.row]; const double *p1ij=this->dd, *p2End=&v.dd[v.row];
    for(; p<pEnd; p++)
    {
        double f=0.0; const double *p2j=v.dd;
        for(; p2j<p2End; p1ij++,p2j++)	f += (*p1ij) * (*p2j);
        *p = f;
    }
    return vtmp;
}

CMat CMat::operator+(const CMat &m0) const
{
    psinsassert(row==m0.row&&clm==m0.clm);
    CMat mtmp(row,clm);
    double *p=mtmp.dd, *pEnd=&mtmp.dd[rc]; const double *p1=this->dd, *p2=m0.dd;
    while(p<pEnd)
    { *p++ = (*p1++) + (*p2++); }
    return mtmp;
}

CMat& CMat::operator+=(const CVect &v)
{
    psinsassert(row==v.row||clm==v.clm);
    int row1 = row+1;
    double *p=dd, *pEnd=&dd[rc];
    for(const double *p1=v.dd; p<pEnd; p+=row1, p1++)	*p += *p1;
    return *this;
}

CMat CMat::operator-(const CMat &m0) const
{
    psinsassert(row==m0.row&&clm==m0.clm);
    CMat mtmp(row,clm);
    double *p=mtmp.dd, *pEnd=&mtmp.dd[rc]; const double *p1=this->dd, *p2=m0.dd;
    while(p<pEnd)
    { *p++ = (*p1++) - (*p2++); }
    return mtmp;
}

CMat CMat::operator*(double f) const
{
    CMat mtmp(row,clm);
    double *p=mtmp.dd, *pEnd=&mtmp.dd[rc]; const double *p1=this->dd;
    while(p<pEnd)
    { *p++ = (*p1++) * f; }
    return mtmp;
}

CMat& CMat::operator+=(const CMat &m0)
{
    psinsassert(row==m0.row&&clm==m0.clm);
    double *p=dd, *pEnd=&dd[rc]; const double *p1=m0.dd;
    while(p<pEnd)
    { *p++ += *p1++; }
    return *this;
}

CMat& CMat::operator-=(const CMat &m0)
{
    psinsassert(row==m0.row&&clm==m0.clm);
    double *p=dd, *pEnd=&dd[rc]; const double *p1=m0.dd;
    while(p<pEnd)
    { *p++ -= *p1++; }
    return *this;
}

CMat& CMat::operator*=(double f)
{
    double *p=dd, *pEnd=&dd[rc];
    while(p<pEnd)
    { *p++ *= f; }
    return *this;
}

CMat& CMat::operator++()
{
    int row1=row+1;
    for(double *p=dd, *pEnd=&dd[rc]; p<pEnd; p+=row1)	*p += 1.0;
    return *this;
}

CMat operator~(const CMat &m0)
{
#ifdef MAT_COUNT_STATISTIC
    ++CMat::iCount;
#endif
    CMat mtmp(m0.clm,m0.row);
    const double *pm=m0.dd;
    for(int i=0; i<m0.row; i++)
    { for(int j=i; j<m0.rc; j+=m0.row) mtmp.dd[j] = *pm++; }
    return mtmp;
}

void symmetry(CMat &m)
{
    psinsassert(m.row==m.clm);
    double *prow0=&m.dd[1], *prowEnd=&m.dd[m.clm], *pclm0=&m.dd[m.clm], *pEnd=&m.dd[m.rc];
    for(int clm1=m.clm+1; prow0<pEnd; prow0+=clm1,pclm0+=clm1,prowEnd+=m.clm)
    {
        for(double *prow=prow0,*pclm=pclm0; prow<prowEnd; prow++,pclm+=m.clm)
            *prow = *pclm = (*prow+*pclm)*0.5;
    }
}

double& CMat::operator()(int r, int c)
{
    if(c<0) c = r;
    return this->dd[r*this->clm+c];
}

void CMat::SetRow(int i, double f, ...)
{
    va_list vl;
    va_start(vl, f);
    for(double *p=&dd[i*clm], *pEnd=p+clm; p<pEnd; p++)
    { *p = f;  f = va_arg(vl, double);	}
    va_end(vl);
    return;
}

void CMat::SetRow(int i, const CVect &v)
{
    psinsassert(clm==v.clm);
    const double *p=v.dd;
    for(double *p1=&dd[i*clm],*pEnd=p1+clm; p1<pEnd; p++,p1++) *p1 = *p;
    return;
}

void CMat::SetClm(int j, double f, ...)
{
    va_list vl;
    va_start(vl, f);
    for(double *p=&dd[j], *pEnd=&p[rc]; p<pEnd; p+=clm)
    { *p = f;  f = va_arg(vl, double);	}
    va_end(vl);
    return;
}

void CMat::SetClm(int j, const CVect &v)
{
    psinsassert(row==v.row);
    const double *p=v.dd;
    for(double *p1=&dd[j],*pEnd=&dd[rc]; p1<pEnd; p++,p1+=clm) *p1 = *p;
    return;
}

void CMat::SetClmVect3(int i, int j, const CVect3 &v)
{
    double *p=&dd[i*clm+j];
    *p = v.i; p += clm;
    *p = v.j; p += clm;
    *p = v.k;
}

void CMat::SetRowVect3(int i, int j, const CVect3 &v)
{
    *(CVect3*)&dd[i*clm+j] = v;
}

void CMat::SetDiagVect3(int i, int j, const CVect3 &v)
{
    double *p=&dd[i*clm+j];
    *p = v.i;  p += clm+1;
    *p = v.j;  p += clm+1;
    *p = v.k;
}

CVect3 CMat::GetDiagVect3(int i, int j)
{
    if(j==-1) j=i;
    CVect3 v;
    double *p=&dd[i*clm+j];
    v.i = *p;  p += clm+1;
    v.j = *p;  p += clm+1;
    v.k = *p;
    return v;
}

void CMat::SetAskew(int i, int j, const CVect3 &v)
{
    double *p=&dd[i*clm+j];
    p[0] = 0.0; p[1] =-v.k; p[2] = v.j;  p += clm;
    p[0] = v.k; p[1] = 0.0; p[2] =-v.i;  p += clm;
    p[0] =-v.j; p[1] = v.i; p[2] = 0.0;
}

void CMat::SetMat3(int i, int j, const CMat3 &m)
{
    double *p=&dd[i*clm+j];
    *(CVect3*)p = *(CVect3*)&m.e00;  p += clm;
    *(CVect3*)p = *(CVect3*)&m.e10;  p += clm;
    *(CVect3*)p = *(CVect3*)&m.e20;
}

CMat3 CMat::GetMat3(int i, int j) const
{
    if(j=-1) j=i;
    CMat3 m;
    const double *p=&dd[i*clm+j];
    *(CVect3*)&m.e00 = *(CVect3*)p;  p += clm;
    *(CVect3*)&m.e10 = *(CVect3*)p;  p += clm;
    *(CVect3*)&m.e20 = *(CVect3*)p;
    return m;
}

void CMat::SubAddMat3(int i, int j, const CMat3 &m)
{
    double *p=&dd[i*clm+j];
    *(CVect3*)p += *(CVect3*)&m.e00;  p += clm;
    *(CVect3*)p += *(CVect3*)&m.e10;  p += clm;
    *(CVect3*)p += *(CVect3*)&m.e20;
}

CVect CMat::GetRow(int i) const
{
    CVect v(1, clm);
    const double *p1=&dd[i*clm], *pEnd=p1+clm;
    for(double *p=v.dd; p1<pEnd; p++,p1++) *p = *p1;
    return v;
}

CVect CMat::GetClm(int j) const
{
    CVect v(row, 1);
    const double *p1=&dd[j], *pEnd=&dd[rc];
    for(double *p=v.dd; p1<pEnd; p++,p1+=clm) *p = *p1;
    return v;
}

void CMat::ZeroRow(int i)
{
    for(double *p=&dd[i*clm],*pEnd=p+clm; p<pEnd; p++) *p = 0.0;
    return;
}

void CMat::ZeroClm(int j)
{
    for(double *p=&dd[j],*pEnd=&dd[rc]; p<pEnd; p+=clm) *p = 0.0;
    return;
}

void CMat::SetDiag(double f, ...)
{
    *this = CMat(this->row, this->clm, 0.0);
    va_list vl;
    va_start(vl, f);
    double *p=dd, *pEnd=&dd[rc];
    for(int row1=row+1; p<pEnd; p+=row1)
    { *p = f;  f = va_arg(vl, double);	}
    va_end(vl);
}

void CMat::SetDiag2(double f, ...)
{
    *this = CMat(this->row, this->clm, 0.0);
    va_list vl;
    va_start(vl, f);
    double *p=dd, *pEnd=&dd[rc];
    for(int row1=row+1; p<pEnd; p+=row1)
    { *p = f*f;  f = va_arg(vl, double);	}
    va_end(vl);
}

double normInf(CMat &m)
{
    double n1=0.0;
    for(double *p=m.dd,*pEnd=&m.dd[m.rc]; p<pEnd; p++)
    {
        if(*p>n1)	 n1 = *p;
        else if(-*p>n1) n1 = -*p;
    }
    return n1;
}

CVect diag(const CMat &m)
{
    int row1 = m.row+1;
    CVect vtmp(m.row,1);
    double *p=vtmp.dd, *pEnd=&vtmp.dd[vtmp.row];
    for(const double *p1=m.dd; p<pEnd; p++, p1+=row1)	*p = *p1;
    return vtmp;
}

void RowMul(CMat &m, const CMat &m0, const CMat &m1, int r)
{
    psinsassert(m0.clm==m1.row);
    int rc0=r*m0.clm;
    double *p=&m.dd[rc0], *pEnd=p+m0.clm; const double *p0=&m0.dd[rc0], *p0End=p0+m0.clm, *p1j=m1.dd;
    for(; p<pEnd; p++)
    {
        double f=0.0; const double *p0j=p0, *p1jk=p1j++;
        for(; p0j<p0End; p0j++,p1jk+=m1.clm)	 f += (*p0j) * (*p1jk);
        *p = f;
    }
}

void RowMulT(CMat &m, const CMat &m0, const CMat &m1, int r)
{
    psinsassert(m0.clm==m1.clm);
    int rc0=r*m0.clm;
    double *p=&m.dd[rc0], *pEnd=p+m0.clm; const double *p0=&m0.dd[rc0], *p0End=p0+m0.clm, *p1jk=m1.dd;
    for(; p<pEnd; p++)
    {
        double f=0.0; const double *p0j=p0;
        for(; p0j<p0End; p0j++,p1jk++)	 f += (*p0j) * (*p1jk);
        *p = f;
    }
}

CMat diag(const CVect &v)
{
#ifdef MAT_COUNT_STATISTIC
    ++CMat::iCount;
#endif
    int rc = v.row>v.clm ? v.row : v.clm, rc1=rc+1;
    CMat mtmp(rc,rc,0.0);
    double *p=mtmp.dd;
    for(const double *p1=v.dd, *p1End=&v.dd[rc]; p1<p1End; p+=rc1, p1++)	*p = *p1;
    return mtmp;
}

void DVMDVafa(const CVect &V, CMat &M, double afa)
{
    psinsassert(V.rc==M.row&&M.row==M.clm);
    int i = 0;
    const double *pv = V.dd;
    for(double vi=*pv, viafa=vi*afa; i<M.clm; i++,pv++,vi=*pv,viafa=vi*afa)
    {
        for(double *prow=&M.dd[i*M.clm],*prowEnd=prow+M.clm,*pclm=&M.dd[i]; prow<prowEnd; prow++,pclm+=M.row)
        {
            *prow *= vi;
            *pclm *= viafa;
        }
    }
}

/***************************  class CVect  *********************************/
CVect::CVect(void)
{
}

CVect::CVect(int row0, int clm0)
{
    if(clm0==1) { row=row0; clm=1;   }
    else		{ row=1;    clm=clm0;}
    rc = row*clm;
 }

CVect::CVect(int row0, double f)
{
    row=row0; clm=1; rc=row*clm;
    for(int i=0;i<row;i++) dd[i]=f;
}

CVect::CVect(int row0, const double *pf)
{
    row=row0; clm=1; rc=row*clm;
    memcpy(dd, pf, row*sizeof(double));
}

CVect::CVect(int row0, double f, double f1, ...)
{
    row=row0; clm=1; rc=row*clm;
    psinsassert(row<=MMD&&clm<=MMD);
    va_list vl;
    va_start(vl, f);
    for(int i=0, rc=row>clm?row:clm; i<rc; i++)
    { dd[i] = f;  f = va_arg(vl, double);	}
    va_end(vl);
}

CVect::CVect(const CVect3 &v)
{
    row=3; clm=1; rc=row*clm;
    dd[0]=v.i; dd[1]=v.j; dd[2]=v.k;
}

CVect::CVect(const CVect3 &v1, const CVect3 v2)
{
    row=6; clm=1; rc=row*clm;
    dd[0]=v1.i; dd[1]=v1.j; dd[2]=v1.k;
    dd[3]=v2.i; dd[4]=v2.j; dd[5]=v2.k;
}

CVect operator~(const CVect &v)
{
    CVect vtmp=v;
    vtmp.row=v.clm; vtmp.clm=v.row;
    return vtmp;
}

CVect CVect::operator*(const CMat &m) const
{
    psinsassert(clm==m.row);
    CVect vtmp(row,clm);
    double *p=vtmp.dd; const double *p1End=&dd[clm];
    for(int j=0; j<clm; p++,j++)
    {
        double f=0.0; const double *p1j=dd, *p2jk=&m.dd[j];
        for(; p1j<p1End; p1j++,p2jk+=m.clm)	 f += (*p1j) * (*p2jk);
        *p = f;
    }
    return vtmp;
}

CMat CVect::operator*(const CVect &v) const
{
    psinsassert(clm==v.row);
    CMat mtmp(row,v.clm);
    if(row==1 && v.clm==1)  // (1x1) = (1xn)*(nx1)
    {
        double f = 0.0;
        for(int i=0; i<clm; i++)  f += dd[i]*v.dd[i];
        mtmp.dd[0] = f;
    }
    else    // (nxn) = (nx1)*(1xn)
    {
        double *p=mtmp.dd;
        for(const double *p1=&dd[0],*p1End=&dd[rc],*p2End=&v.dd[rc]; p1<p1End; p1++)
        {
            for(const double *p2=&v.dd[0]; p2<p2End; p2++)  *p++ = *p1 * *p2;
        }
    }
    return mtmp;
}

CVect CVect::operator+(const CVect &v) const
{
    psinsassert(row==v.row&&clm==v.clm);
    const double *p2=v.dd, *p1=dd, *p1End=&dd[rc];
    CVect vtmp(row,clm);
    for(double *p=vtmp.dd; p1<p1End; p++,p1++,p2++)  { *p=*p1+*p2; }
    return vtmp;
}

CVect CVect::operator-(const CVect &v) const
{
    psinsassert(row==v.row&&clm==v.clm);
    const double *p2=v.dd, *p1=dd, *p1End=&dd[rc];
    CVect vtmp(row,clm);
    for(double *p=vtmp.dd; p1<p1End; p++,p1++,p2++)  { *p=*p1-*p2; }
    return vtmp;
}

CVect CVect::operator*(double f) const
{
    CVect vtmp(row,clm);
    const double *p1=dd,*p1End=&dd[rc];
    for(double *p=vtmp.dd; p1<p1End; p++,p1++)  { *p=*p1*f; }
    return vtmp;
}

CVect& CVect::operator=(double f)
{
    for(double *p=dd, *pEnd=&dd[rc]; p<pEnd; p++)  { *p = f; }
    return *this;
}

CVect& CVect::operator=(const double *pf)
{
    for(double *p=dd, *pEnd=&dd[rc]; p<pEnd; p++,pf++)  { *p = *pf; }
    return *this;
}

CVect& CVect::operator+=(const CVect &v)
{
    psinsassert(row==v.row&&clm==v.clm);
    const double *p1 = v.dd;
    for(double *p=dd, *pEnd=&dd[rc]; p<pEnd; p++,p1++)  { *p += *p1; }
    return *this;
}

CVect& CVect::operator-=(const CVect &v)
{
    psinsassert(row==v.row&&clm==v.clm);
    const double *p1 = v.dd;
    for(double *p=dd, *pEnd=&dd[rc]; p<pEnd; p++,p1++)  { *p -= *p1; }
    return *this;
}

CVect& CVect::operator*=(double f)
{
    for(double *p=dd, *pEnd=&dd[rc]; p<pEnd; p++)  { *p *= f; }
    return *this;
}

CVect pow(const CVect &v, int k)
{
    CVect pp = v;
    double *p, *pEnd=&pp.dd[pp.rc];
    for(int i=1; i<k; i++)
    {
        p=pp.dd;
        for(const double *p1=v.dd; p<pEnd; p++,p1++)
            *p *= *p1;
    }
    return pp;
}

CVect abs(const CVect &v)
{
    CVect res(v.row,v.clm);
    const double *p=v.dd, *pEnd=&v.dd[v.rc];
    for(double *p1=res.dd; p<pEnd; p++,p1++)  { *p1 = *p>0 ? *p : -*p; }
    return res;
}

double norm(const CVect &v)
{
    const double *p=v.dd, *pEnd=&v.dd[v.rc];
    double f=0.0;
    for(; p<pEnd; p++)  { f += (*p)*(*p); }
    return sqrt(f);
}

double normInf(const CVect &v)
{
    const double *p=v.dd, *pEnd=&v.dd[v.rc];
    double f=0.0;
    for(; p<pEnd; p++)  { if(*p>f) f=*p; else if(-*p>f) f=-*p; }
    return f;
}

double& CVect::operator()(int r)
{
    return this->dd[r];
}

void CVect::Set(double f, ...)
{
    psinsassert(rc<=MMD);
    va_list vl;
    va_start(vl, f);
    for(int i=0; i<rc; i++)
    { dd[i] = f;  f = va_arg(vl, double);	}
    va_end(vl);
}

void CVect::Set2(double f, ...)
{
    psinsassert(rc<=MMD);
    va_list vl;
    va_start(vl, f);
    for(int i=0; i<rc; i++)
    { dd[i] = f*f;  f = va_arg(vl, double);	}
    va_end(vl);
}

/***************************  class CKalman  *********************************/
CKalman::CKalman(int nq0, int nr0)
{
    psinsassert(nq0<=MMD&&nr0<=MMD);
    kftk = 0.0;
    nq = nq0; nr = nr0;
    Ft = Pk = CMat(nq,nq,0.0);
    Hk = CMat(nr,nq,0.0);  Fading = CMat(nr,nq,1.0); zfdafa = 0.1;
    Qt = Pmin = Xk = CVect(nq,0.0);  Xmax = Pmax = CVect(nq,INF);  Pset = CVect(nq,-1);
    Zk = CVect(nr,0.0);  Rt = CVect(nr,INF); rts = CVect(nr,1.0);  Zfd = CVect(nr,0.0); Zfd0 = CVect(nr,INF);
    RtTau = Rmax = CVect(nr,INF); measlost = Rmin = Rb = CVect(nr,0.0); Rbeta = CVect(nr,1.0);
    for(int i=0; i<nr; i++) { Rmaxcount[i]=0, Rmaxcount0[i]=5; }
    FBTau = FBMax = FBOne = CVect(nq,INF); FBXk = FBTotal = CVect(nq,0.0);
    measflag = measflaglog = measstop = 0;
}

void CKalman::TimeUpdate(double kfts, int fback)
{
/*	CMat Fk, FtPk;
    kftk += kfts;
    SetFt(nq);
    Fk=++(Ft*kfts);  // Fk = I+Ft*ts
    Xk = Fk * Xk;
    FtPk = Ft*Pk*kfts;
    Pk = Pk + FtPk + (~FtPk);// + FtPk*(~Ft)*kfts;
    Pk += Qt*kfts;
    if(fback)  Feedback(kfts);
*/
    CMat Fk;
    kftk += kfts;
    SetFt(nq);
    Fk=++(Ft*kfts);  // Fk = I+Ft*ts
    Xk = Fk * Xk;
    Pk = Fk*Pk*(~Fk);  Pk += Qt*kfts;
    if(fback)  Feedback(kfts);
    if(measstop>-1000000) measstop--;
}

void CKalman::SetMeasFlag(int flag)
{
    measflag = (flag==0) ? 0 : (measflag|flag);
}

int CKalman::MeasUpdate(double fading)
{
    CVect Pxz, Kk, Hi;
    SetMeas();
    if(measstop>0) measflag = 0;
    for(int i=0; i<nr; i++)
    {
        if(measflag&(0x01<<i))
        {
            Hi = Hk.GetRow(i);
            Pxz = Pk*(~Hi);
            double Pz0 = (Hi*Pxz)(0,0), r=Zk(i)-(Hi*Xk)(0,0);
            if(Rb.dd[i]>EPS)
                RAdaptive(i, r, Pz0);
            if(Zfd.dd[i]<INF/2)
                RPkFading(i);
            double Pzz = Pz0+Rt.dd[i]/rts.dd[i];
            Kk = Pxz*(1.0/Pzz);
            Xk += Kk*r;
            Pk -= Kk*(~Pxz);
        }
    }
    if(fading>1.0) Pk *= fading;
    XPConstrain();
    symmetry(Pk);
    int measres = measflag;
    measflaglog |= measres;
    SetMeasFlag(0);
    return measres;
}

int CKalman::RAdaptive(int i, double r, double Pr)
{
    double rr=r*r-Pr;
    if(rr<Rmin.dd[i])	rr = Rmin.dd[i];
//	if(rr>Rmax.dd[i])	Rt.dd[i] = Rmax.dd[i];
//	else				Rt.dd[i] = (1.0-Rbeta.dd[i])*Rt.dd[i]+Rbeta.dd[i]*rr;
    if(rr>Rmax.dd[i])	{ Rt.dd[i]=Rmax.dd[i]; Rmaxcount[i]++; }
    else				{ Rt.dd[i]=(1.0-Rbeta.dd[i])*Rt.dd[i]+Rbeta.dd[i]*rr; Rmaxcount[i]=0; }
    Rbeta.dd[i] = Rbeta.dd[i]/(Rbeta.dd[i]+Rb.dd[i]);   // beta = beta / (beta+b)
    int adptOK = (Rmaxcount[i]==0||Rmaxcount[i]>Rmaxcount0[i]) ? 1: 0;
    return adptOK;
}

void CKalman::RPkFading(int i)
{
    Zfd.dd[i] = Zfd.dd[i]*(1-zfdafa) + Zk.dd[i]*zfdafa;
    if(Zfd.dd[i]>Zfd0.dd[i] || Zfd.dd[i]<-Zfd0.dd[i])
        DVMDVafa(Fading.GetRow(i), Pk,1.0);
}

void CKalman::XPConstrain(void)
{
    int i=0, nq1=nq+1;
    for(double *px=Xk.dd,*pxmax=Xmax.dd,*p=Pk.dd,*pmin=Pmin.dd,*pminEnd=&Pmin.dd[nq],*pmax=Pmax.dd,*pset=Pset.dd;
        pmin<pminEnd; px++,pxmax++,p+=nq1,pmin++,pmax++,pset++)
    {
        if(*px>*pxmax)			// Xk constrain
        {
            *px = *pxmax;
        }
        else if(*px<-*pxmax)
        {
            *px = -*pxmax;
        }
        if(*p<*pmin)	// Pk constrain
        {
            *p = *pmin;
        }
        else if(*p>*pmax)
        {
            double sqf=sqrt(*pmax/(*p))*0.9;
            for(double *prow=&Pk.dd[i*Pk.clm],*prowEnd=prow+nq,*pclm=&Pk.dd[i]; prow<prowEnd; prow++,pclm+=nq)
            {
                *prow *= sqf;
                *pclm = *prow;
            }
            Pk.dd[i*Pk.clm+i] *= sqf;  //20200303
            break;
        }
        if(*pset>0.0)	// Pk set
        {
            if(*p<*pset)
            {
                *p = *pset;
            }
            else if(*p>*pset)
            {
                double sqf=sqrt(*pset/(*p));
                for(double *prow=&Pk.dd[i*Pk.clm],*prowEnd=prow+nq,*pclm=&Pk.dd[i]; prow<prowEnd; prow++,pclm+=nq)
                {
                    *prow *= sqf;
                    *pclm *= sqf;
                }
            }
            *pset = -1.0;
        }
        i++;
    }
}

void CKalman::Feedback(double fbts)
{
    double *pTau=FBTau.dd, *pTotal=FBTotal.dd, *pMax=FBMax.dd, *pOne=FBOne.dd, *pXk=FBXk.dd, *p=Xk.dd;
    for(int i=0; i<nq; i++, pTau++,pTotal++,pMax++,pOne++,pXk++,p++)
    {
        if(*pTau<(double)INF/2)
        {
            double afa = fbts<*pTau ? fbts/(*pTau) : 1.0;
            *pXk = *p*afa;
            if(*pXk>*pOne) *pXk=*pOne; else if(*pXk<-*pOne) *pXk=-*pOne;
            if(*pMax<(double)INF/2)
            {
                if(*pTotal+*pXk>*pMax)			*pXk = *pMax-*pTotal;
                else if(*pTotal+*pXk<-*pMax)	*pXk = -*pMax-*pTotal;
            }
            *p -= *pXk;
            *pTotal += *pXk;
        }
        else
        {
            *pXk = 0.0;
        }
    }
}

void CKalman::RtFading(int i, double fdts)
{
    double Taui=RtTau.dd[i], Rti=Rt.dd[i], Rmaxi=Rmax.dd[i];
    if(measlost.dd[i]>3.0 && Taui<INF/2 && Rti<Rmaxi)
    {
        double afa = fdts<Taui ? fdts/Taui : 1.0;
        Rti += 2*sqrt(Rmaxi*Rti)*afa;
        Rt.dd[i] = Rti;
    }
}


/***************************  class CRAvar  *********************************/
CRAvar::CRAvar()
{
}

CRAvar::CRAvar(int nR0, int maxCount0)
{
    psinsassert(nR0<RAMAX);
    this->nR0 = nR0;
    for(int i=0; i<RAMAX; i++)  { Rmaxcount[i]=maxCount0, tau[i]=INF; }
}

void CRAvar::set(double r0, double tau, double rmax, double rmin, int i)
{
    this->R0[i] = r0*r0;
    this->tau[i] = tau;
    this->r0[i] = 0.0;  Rmaxflag[i] = Rmaxcount[i];
    this->Rmax[i] = rmax==0.0 ? 100.0*this->R0[i] : rmax*rmax;
    this->Rmin[i] = rmin==0.0 ?  0.01*this->R0[i] : rmin*rmin;
}

void CRAvar::set(const CVect3 &r0, const CVect3 &tau, const CVect3 &rmax, const CVect3 &rmin)
{
    const double *pr0=&r0.i, *ptau=&tau.i, *prmax=&rmax.i, *prmin=&rmin.i;
    for(int i=0; i<3; i++,pr0++,ptau++,prmax++,prmin++)
        set(*pr0, *ptau, *prmax, *prmin, i);
}

void CRAvar::set(const CVect &r0, const CVect &tau, const CVect &rmax, const CVect &rmin)
{
    const double *pr0=r0.dd, *ptau=tau.dd, *prmax=rmax.dd, *prmin=rmin.dd;
    for(int i=0; i<nR0; i++,pr0++,ptau++,prmax++,prmin++)
        set(*pr0, *ptau, *prmax, *prmin, i);
}

void CRAvar::Update(double r, double ts, int i)
{
    if(tau[i]>INF/2) return;
    double tstau = ts>tau[i] ? 1.0 : ts/tau[i];
    double dr2=r-r0[i]; dr2=dr2*dr2; r0[i]=r;
    if(dr2>R0[i]) R0[i]=dr2; else R0[i]=(1.0-tstau)*R0[i]+tstau*dr2;
    if(R0[i]<Rmin[i]) R0[i]=Rmin[i];
    if(R0[i]>Rmax[i]) {R0[i]=Rmax[i];Rmaxflag[i]=Rmaxcount[i];} else {Rmaxflag[i]-=Rmaxflag[i]>0;}
}

void CRAvar::Update(const CVect3 &r, double ts)
{
    const double *pr=&r.i;
    for(int i=0; i<3; i++,pr++)
        Update(*pr, ts, i);
}

void CRAvar::Update(const CVect &r, double ts)
{
    const double *pr=r.dd;
    for(int i=0; i<nR0; i++,pr++)
        Update(*pr, ts, i);
}

double CRAvar::operator()(int k)
{
    return Rmaxflag[k] ? INF : sqrt(R0[k]);
}


/*****************CRmemory*************/
CRMemory::CRMemory(long recordNum, int recordLen0)
{
    BYTE *pb = (BYTE*)malloc(recordNum*recordLen0);
    *this = CRMemory(pb, recordNum*recordLen0, recordLen0);
    pMemStart0 = pMemStart;
}

CRMemory::CRMemory(BYTE *pMem, long memLen0, int recordLen0)
{
    psinsassert(recordLen0<=MAX_RECORD_BYTES);
    pMemStart0 = NULL;
    pMemStart = pMemPush = pMemPop = pMem;
    pMemEnd = pMemStart + memLen0;
    pushLen = popLen = recordLen = recordLen0;
    memLen = memLen0;
    dataLen = 0;
}

CRMemory::~CRMemory()
{
    if(pMemStart0) { free(pMemStart0); pMemStart0 = NULL; }
}

BYTE CRMemory::pop(BYTE *p)
{
    if(dataLen==0) return 0;
    popLen = recordLen==0 ? *pMemPop : recordLen;
    if(p==(BYTE*)NULL) p = popBuf;
    BYTE i;
    for(i=0; i<popLen; i++,dataLen--)
    {
        *p++ = *pMemPop++;
        if(pMemPop>=pMemEnd)  pMemPop = pMemStart;
    }
    return i;
}

BYTE* CRMemory::get(int iframe)
{
    return &pMemStart[popLen*iframe];
}

bool CRMemory::push(const BYTE *p)
{
    bool res = 1;
    if(p==(BYTE*)NULL) p = pushBuf;
    pushLen = recordLen==0 ? *p : recordLen;
    psinsassert(pushLen<=MAX_RECORD_BYTES);
    for(BYTE i=0; i<pushLen; i++,dataLen++)
    {
        *pMemPush++ = *p++;
        if(pMemPush>=pMemEnd)  pMemPush = pMemStart;
        if(pMemPush==pMemPop) { res=0; pop(); }
    }
    return res;
}

/***************************  class CSINSKF  *********************************/
CSINSKF::CSINSKF(int nq0, int nr0):CKalman(nq0,nr0)
{
    sins = CSINS(qI, O31, O31);
    this->SetFt(nq);
    // an example for SINS/GPS vn&pos measurement
    Hk(0,3) = Hk(1,4) = Hk(2,5) = 1.0;
    Hk(3,6) = Hk(4,7) = Hk(5,8) = 1.0;
}

void CSINSKF::Init(const CSINS &sins0, int grade)
{
    sins = sins0;  kftk = sins.tk;
    if (grade==-1) return; // NULL
//    sins.Rwfa.set(
//        CVect(9, 100*glv.dps,100*glv.dps,100*glv.dps, 1*glv.g0,1*glv.g0,1*glv.g0, 1*glv.g0,1*glv.g0,1*glv.g0),
//        CVect(9, 1.0,1.0,1.0, 1.0,1.0,1.0, 1.0,1.0,1.0),
//        CVect(9, 100*glv.dps,100*glv.dps,100*glv.dps, 1*glv.g0,1*glv.g0,1*glv.g0, 1*glv.g0,1*glv.g0,1*glv.g0),
//        CVect(9, 0.01*glv.dps,0.01*glv.dps,0.01*glv.dps, 0.1*glv.mg,0.1*glv.mg,0.1*glv.mg, 0.1*glv.mg,0.1*glv.mg,0.1*glv.mg)
//        );
    // a example for 15-state(phi,dvn,dpos,eb,db) KF setting
    if(grade==0) // inertial-grade
    {
    Pmax.Set2(10.0*glv.deg,10.0*glv.deg,30.0*glv.deg,    50.0,50.0,50.0,    1.0e4/glv.Re,1.0e4/glv.Re,1.0e4,
        10.0*glv.dph,10.0*glv.dph,10.0*glv.dph,    10.0*glv.mg,10.0*glv.mg,10.0*glv.mg);
    Pmin.Set2(0.01*glv.min,0.01*glv.min,0.1*glv.min,    0.01,0.01,0.1,    1.0/glv.Re,1.0/glv.Re,0.1,
        0.001*glv.dph,0.001*glv.dph,0.001*glv.dph,    1.0*glv.ug,1.0*glv.ug,5.0*glv.ug);
    Pk.SetDiag2(1.0*glv.deg,1.0*glv.deg,10.0*glv.deg,    1.0,1.0,1.0,     100.0/glv.Re,100.0/glv.Re,100.0,
        0.1*glv.dph,0.1*glv.dph,0.1*glv.dph,    1.0*glv.mg,1.0*glv.mg,1.0*glv.mg);
    Qt.Set2(0.001*glv.dpsh,0.001*glv.dpsh,0.001*glv.dpsh,    1.0*glv.ugpsHz,1.0*glv.ugpsHz,1.0*glv.ugpsHz,    0.0,0.0,0.0,
        0.0*glv.dphpsh,0.0*glv.dphpsh,0.0*glv.dphpsh,    0.0*glv.ugpsh,0.0*glv.ugpsh,0.0*glv.ugpsh);
    Xmax.Set(INF,INF,INF, INF,INF,INF, INF,INF,INF,  0.1*glv.dps,0.1*glv.dps,0.1*glv.dps,  200.0*glv.ug,200.0*glv.ug,200.0*glv.ug);
    Rt.Set2(0.2,0.2,0.6,   10.0/glv.Re,10.0/glv.Re, 30.0);
    Rmax = Rt*10000;  Rmin = Rt*0.01;  Rb = 0.9;
    FBTau.Set(1.0,1.0,10.0,     1.0,1.0,1.0,     1.0,1.0,1.0,    10.0,10.0,10.0,    10.0,10.0,10.0);
    }
    else if(grade==1) // MEMS-grade
    {
    Pmax.Set2(50.0*glv.deg,50.0*glv.deg,100.0*glv.deg,    500.0,500.0,500.0,    1.0e6/glv.Re,1.0e6/glv.Re,1.0e6,
        5000.0*glv.dph,5000.0*glv.dph,5000.0*glv.dph,    50.0*glv.mg,50.0*glv.mg,50.0*glv.mg);
    Pmin.Set2(0.5*glv.min,0.5*glv.min,3.0*glv.min,    0.01,0.01,0.1,    1.0/glv.Re,1.0/glv.Re,0.1,
        1.0*glv.dph,1.0*glv.dph,1.0*glv.dph,    50.0*glv.ug,50.0*glv.ug,50.0*glv.ug);
    Pk.SetDiag2(10.0*glv.deg,10.0*glv.deg,100.0*glv.deg,    10.0,10.0,10.0,     100.0/glv.Re,100.0/glv.Re,100.0,
        100.0*glv.dph,101.0*glv.dph,102.0*glv.dph,    1.0*glv.mg,1.01*glv.mg,10.0*glv.mg);
    Qt.Set2(1.0*glv.dpsh,1.0*glv.dpsh,1.0*glv.dpsh,    10.0*glv.ugpsHz,10.0*glv.ugpsHz,10.0*glv.ugpsHz,    0.0,0.0,0.0,
        0.0*glv.dphpsh,0.0*glv.dphpsh,0.0*glv.dphpsh,    0.0*glv.ugpsh,0.0*glv.ugpsh,0.0*glv.ugpsh);
    Xmax.Set(INF,INF,INF, INF,INF,INF, INF,INF,INF,  1.0*glv.dps,1.0*glv.dps,1.0*glv.dps,  50.0*glv.mg,50.0*glv.mg,50.0*glv.mg);
    Rt.Set2(0.2,0.2,0.6,   10.0/glv.Re,10.0/glv.Re,30.0);
    Rmax = Rt*10000;  Rmin = Rt*0.01;  Rb = 0.9;
    FBTau.Set(1.0,1.0,1.0,     1.0,1.0,1.0,     1.0,1.0,1.0,    10.0,10.0,10.0,    10.0,10.0,10.0);
    }
    Zfd0.Set(1.0,15.0,1.0, 100.0/RE,100.0/RE,100.0, INF);   zfdafa = 0.1;
    Fading(0,1)=1.01;  Fading(0,3)=1.01;	Fading(0,7)=1.01;
    Fading(1,0)=1.01;  Fading(1,4)=1.01;	Fading(1,6)=1.01;
    Fading(2,5)=1.01;  Fading(2,8)=1.01;	Fading(2,14)=1.01;
    Fading(3,0)=1.01;  Fading(3,4)=1.01;	Fading(3,6)=1.01;
    Fading(4,1)=1.01;  Fading(4,3)=1.01;	Fading(4,7)=1.01;
    Fading(5,5)=1.01;  Fading(5,8)=1.01;	Fading(5,14)=1.01;
    Zfd0 = INF;
}

void CSINSKF::SetFt(int nnq)
{
    sins.etm();
//	Ft = [ Maa    Mav    Map    -Cnb	O33
//         Mva    Mvv    Mvp     O33    Cnb
//         O33    Mpv    Mpp     O33    O33
//         zeros(6,9)  diag(-1./[ins.tauG;ins.tauA]) ];
    Ft.SetMat3(0,0,sins.Maa); Ft.SetMat3(0,3,sins.Mav); Ft.SetMat3(0,6,sins.Map); Ft.SetMat3(0,9,-1.0*sins.Cnb);
    Ft.SetMat3(3,0,sins.Mva); Ft.SetMat3(3,3,sins.Mvv); Ft.SetMat3(3,6,sins.Mvp); Ft.SetMat3(3,12,sins.Cnb);
                        NULL; Ft.SetMat3(6,3,sins.Mpv); Ft.SetMat3(6,6,sins.Mpp);
    Ft.SetDiagVect3( 9, 9, sins._betaGyro);
    Ft.SetDiagVect3(12,12, sins._betaAcc);
    if(nnq>=34) {
        CMat3 Cwx=-sins.wib.i*sins.Cnb, Cwy=-sins.wib.j*sins.Cnb, Cwz=-sins.wib.k*sins.Cnb,
              Cfx= sins.fb.i *sins.Cnb, Cfy= sins.fb.j *sins.Cnb, Cfz= sins.fb.k *sins.Cnb;
        Cfz.e00=Cfy.e01;	Cfz.e01=Cfy.e02;
        Cfz.e10=Cfy.e11;	Cfz.e11=Cfy.e12;
        Cfz.e20=Cfy.e21;	Cfz.e21=Cfy.e22;
        Ft.SetMat3(0,19, Cwx);  Ft.SetMat3(0,22, Cwy);  Ft.SetMat3(0,25, Cwz);  // dKG
        Ft.SetMat3(3,28, Cfx);  Ft.SetMat3(3,31, Cfz);  // dKA(xx,yx,zx, yy,zy, zz)
    }
}

void CSINSKF::SetHk(int nnq)
{
//	ins.CW = ins.Cnb*askew(ins.web);  ins.MpvCnb = ins.Mpv*ins.Cnb;
//	Hk(1:3,16:19) = [-ins.CW,     -ins.an];     % lever, dt
//	Hk(4:6,16:19) = [-ins.MpvCnb, -ins.Mpvvn];
    if(nnq>=18) {
        CMat3 CW=sins.Cnb*askew(sins.web), MC=sins.Mpv*sins.Cnb;
        Hk.SetMat3(0,15, -1.0*CW);
        Hk.SetMat3(3,15, -1.0*MC);
    }
    if(nnq>=19) {
        CVect3 MV=sins.Mpv*sins.vn;
        Hk.SetClmVect3(0,18, -1.0*sins.an);
        Hk.SetClmVect3(3,18, -1.0*MV);
    }
}

int CSINSKF::Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts)
{
    sins.Update(pwm, pvm, nSamples, ts);
    TimeUpdate(sins.nts);  kftk = sins.tk;
    return MeasUpdate();
}

void CSINSKF::Feedback(double fbts)
{
    CKalman::Feedback(fbts);
    sins.qnb -= *(CVect3*)&FBXk.dd[0];  sins.vn -= *(CVect3*)&FBXk.dd[ 3];  sins.pos -= *(CVect3*)&FBXk.dd[6];
    sins.eb  += *(CVect3*)&FBXk.dd[9];	sins.db += *(CVect3*)&FBXk.dd[12];
}

void CSINSKF::QtMarkovGA(const CVect3 &tauG, const CVect3 &sRG, const CVect3 &tauA=I31, const CVect3 &sRA=O31)
{
    sins.SetTauGA(tauG, tauA);
    *(CVect3*)&Qt.dd[ 9] = MKQt(sRG, sins.tauGyro);
    *(CVect3*)&Qt.dd[12] = MKQt(sRA, sins.tauAcc);
}

void CSINSKF::SetYaw(double yaw)
{
    CQuat qnn = a2qua(0,0,diffYaw(yaw,sins.att.k));
    sins.qnb = qnn*sins.qnb;  sins.Cnb = q2mat(sins.qnb);  sins.Cbn = ~sins.Cnb;
    sins.vn = qnn*sins.vn;
    *(CVect3*)&Xk.dd[0] = qnn**(CVect3*)&Xk.dd[0];
    *(CVect3*)&Xk.dd[3] = qnn**(CVect3*)&Xk.dd[3];
}

void CSINSKF::SecretAttitude(void)
{
#define SA_PCH (12*DEG)
#define SA_RLL (34*DEG)
#define SA_YAW (56*DEG)
#define SA_THR (0.1*DEG)
    if( sins.att.i<SA_PCH+SA_THR && sins.att.i>SA_PCH-SA_THR &&
        sins.att.j<SA_RLL+SA_THR && sins.att.j>SA_RLL-SA_THR &&
        sins.att.k<SA_YAW+SA_THR && sins.att.k>SA_YAW-SA_THR )
    {
        sins.att.k = SA_YAW;
        sins.qnb.SetYaw(sins.att.k);
    }
#undef SA_PCH
#undef SA_RLL
#undef SA_YAW
#undef SA_THR
}

/***************************  class CSINSTDKF  *********************************/
CSINSTDKF::CSINSTDKF(int nq0, int nr0):CSINSKF(nq0,nr0)
{
    Fk = Pk1 = CMat(nq,nq, 0.0);
    Pxz = Qk = Kk = tmeas = CVect(nr, 0.0);
    meantdts = 1.0; tdts = 0.0;
    maxStep = 2*(nq+nr)+3;
    TDReset();
}

void CSINSTDKF::TDReset(void)
{
    iter = -2;
    ifn = 0;	meanfn = O31;
    SetMeasFlag(0);
}

int CSINSTDKF::TDUpdate(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts, int nStep)
{
//	if(sins.tk>99)
//		int debugi = 1;

    sins.Update(pwm, pvm, nSamples, ts);
    Feedback(sins.nts);
    for(int j=0; j<nr; j++) measlost.dd[j] += sins.nts;

    measRes = 0;

    if(nStep<=0||nStep>=maxStep) { nStep=maxStep; }
    tdStep = nStep;

    tdts += sins.nts; kftk = sins.tk;
    meanfn = meanfn+sins.fn; ifn++;
    for(int i=0; i<nStep; i++)
    {
        if(iter==-2)			// -2: set measurements
        {
            if(ifn==0)	break;
            CVect3 vtmp=meanfn*(1.0/ifn); meanfn = O31; ifn = 0;
            sins.fn=vtmp; SetFt(nq); sins.fn = vtmp;
            SetMeas(); SetHk(nq);
        }
        else if(iter==-1)			// -1: discrete
        {
            Fk = ++(Ft*tdts); // Fk = I+Ft*ts
            Qk = Qt*tdts;
            Xk = Fk*Xk;
//			RtFading(tdts);
            meantdts = tdts; tdts = 0.0;
        }
        else if(iter<nq)		// 0 -> (nq-1): Fk*Pk
        {
            int row=iter;
            RowMul(Pk1, Fk, Pk, row);
        }
        else if(iter<2*nq)		// nq -> (2*nq-1): Fk*Pk*Fk+Qk
        {
            int row=iter-nq;
            RowMulT(Pk, Pk1, Fk, row);
            Pk.dd[nq*row+row] += Qk.dd[row];
//			if(row==nq-1) {	Pk += Qk; }
        }
        else if(iter<2*(nq+nr))	// (2*nq) -> (2*(nq+nr)-1): sequential measurement updating
        {
            if(measstop>0) measflag = 0;
            int row=(iter-2*Ft.row)/2;
            int flag = measflag&(0x01<<row);
            if(flag)
            {
//				if((iter-2*Ft.row)%2==0)
                if(iter%2==0)
                {
                    Hi = Hk.GetRow(row);
                    Pxz = Pk*(~Hi);
                    Pz0 = (Hi*Pxz)(0,0);
                    innovation = Zk(row)-(Hi*Xk)(0,0);
                    adptOKi = 1;
                    if(Rb.dd[row]>EPS)
                        adptOKi = RAdaptive(row, innovation, Pz0);
                    double Pzz = Pz0 + Rt(row)/rts(row);
                    Kk = Pxz*(1.0/Pzz);
                }
                else
                {
                    measflag ^= flag;
                    if(adptOKi)
                    {
                        measRes |= flag;
                        Xk += Kk*innovation;
                        Pk -= Kk*(~Pxz);
                        measlost.dd[row] = 0.0;
                    }
                    if(Zfd0.dd[row]<INF/2)
                    {
                        RPkFading(row);
                    }
                }
            }
            else
            {
                nStep++;
            }
            if(iter%2==0)
                RtFading(row, meantdts);
        }
        else if(iter==2*(nq+nr))	// 2*(nq+nr): Xk,Pk constrain & symmetry
        {
            XPConstrain();
            symmetry(Pk);
        }
        else if(iter>=2*(nq+nr)+1)	// 2*(nq+nr)+1: Miscellanous
        {
            Miscellanous();
            iter = -3;
        }
        iter++;
    }
    SecretAttitude();
    if(measstop>-1000000) measstop--;

    measflaglog |= measRes;
    return measRes;
}

/***************************  class CSINSGPS  *********************************/
CSINSGPSOD::CSINSGPSOD(void):CSINSTDKF(21, 10)
{
    tODInt = 0.0;  	Kod = 1.0;
    Cbo = I33;  MpkD = O33;
    Hk.SetMat3(6, 6, I33);   Hk.SetMat3(6, 18, -1.0*I33);
    Hk(9,2) = 1.0;
    measGPSvnValid = measGPSposValid = measODValid = measMAGyawValid = 0;
}

// may be copied & implemented by user
void CSINSGPSOD::Init(const CSINS &sins0, int grade)
{
    CSINSKF::Init(sins0);
    posDR = sins0.pos;
    Pmax.Set2(10.0*glv.deg, 10.0*glv.deg, 30.0*glv.deg, 50.0, 50.0, 50.0, 1.0e4/glv.Re, 1.0e4/glv.Re, 1.0e4,
        10.0*glv.dph, 10.0*glv.dph, 10.0*glv.dph, 10.0*glv.mg, 10.0*glv.mg, 10.0*glv.mg, 1*glv.deg, 0.01, 1*glv.deg, 1.0e4/glv.Re, 1.0e4/glv.Re, 1.0e4);
    Pmin.Set2(0.01*glv.min, 0.01*glv.min, 0.1*glv.min, 0.01, 0.01, 0.1, 1.0 / glv.Re, 1.0 / glv.Re, 0.1,
        0.001*glv.dph, 0.001*glv.dph, 0.001*glv.dph, 5.0*glv.ug, 5.0*glv.ug, 15.0*glv.ug, 1*glv.min, 0.0001, 0.2*glv.min, 1.0/glv.Re, 1.0/glv.Re, 1.0);
    Pk.SetDiag2(1.0*glv.deg, 1.0*glv.deg, 1.0*glv.deg, 1.0, 1.0, 1.0, 100.0 / glv.Re, 100.0 / glv.Re, 100.0,
        0.01*glv.dph, 0.01*glv.dph, 0.01*glv.dph, .10*glv.mg, .10*glv.mg, .10*glv.mg, 10.0*glv.min, 0.05, 15*glv.min, 1.0e1/glv.Re, 1.0e1/glv.Re, 1.0e1);
    Qt.Set2(0.001*glv.dpsh, 0.001*glv.dpsh, 0.001*glv.dpsh, 1.0*glv.ugpsHz, 1.0*glv.ugpsHz, 1.0*glv.ugpsHz, 0.0, 0.0, 0.0,
        0.0*glv.dphpsh, 0.0*glv.dphpsh, 0.0*glv.dphpsh, 0.0*glv.ugpsh, 0.0*glv.ugpsh, 0.0*glv.ugpsh, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    Xmax.Set(INF,INF,INF, INF,INF,INF, INF,INF,INF, 0.1*glv.dph,0.1*glv.dph,0.1*glv.dph, 200.0*glv.ug,200.0*glv.ug,200.0*glv.ug, 1*glv.deg, 0.05, 1*glv.deg, INF, INF, INF);
    Rt.Set2(0.2, 0.2, 0.6, 10.0 / glv.Re, 10.0 / glv.Re, 30.0, 10.1/RE, 10.1/RE, 10.1);
    Rmax = Rt * 100;  Rmin = Rt*0.01;  Rb = 0.9;
    FBTau.Set(1.0, 1.0, 10.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0);
//	FBTau.Set(1.0, 1.0, 10.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, INF, INF, INF);
}

void CSINSGPSOD::SetFt(int nnq)
{
    CSINSKF::SetFt(15);
    CMat3 MvkD = norm(sins.vn)*CMat3(-sins.Cnb.e02,sins.Cnb.e01,sins.Cnb.e00,
            -sins.Cnb.e12,sins.Cnb.e11,sins.Cnb.e10,-sins.Cnb.e22,sins.Cnb.e21,sins.Cnb.e20);
    Ft.SetMat3(18, 0, sins.Mpv*askew(sins.vn));
    Ft.SetMat3(18, 15, sins.Mpv*MvkD);
    Ft.SetMat3(18, 18, sins.Mpp);
}

void CSINSGPSOD::SetMeas(void)
{
    if(measGPSvnValid)	{ SetMeasFlag(000007);	}	// SINS/GPS_VEL
    if(measGPSposValid)	{ SetMeasFlag(000070);	}	// SINS/GPS_POS
    if(measODValid)		{ SetMeasFlag(000700);	}	// SINS/DR
    if(measMAGyawValid)	{ SetMeasFlag(001000);	}	// SINS/MAG_YAW
    measGPSvnValid = measGPSposValid = measODValid = measMAGyawValid = 0;
}

void CSINSGPSOD::Feedback(double fbts)
{
    CSINSKF::Feedback(fbts);
    Cbo = Cbo*a2mat(CVect3(FBXk.dd[15],0.0,FBXk.dd[17]));
    Kod *= 1 - FBXk.dd[16];
    posDR -= *(CVect3*)(&FBXk.dd[18]);
}

void CSINSGPSOD::SetMeasGPS(const CVect3 &pgps, const CVect3 &vgps)
{
    if(!IsZero(pgps,EPS))
    {
        *(CVect3*)&Zk.dd[3] = sins.pos - pgps;
        measGPSposValid = 1;
    }
    if(!IsZero(vgps,EPS))
    {
        *(CVect3*)&Zk.dd[0] = sins.vn - vgps;
        measGPSvnValid = 1;
    }
}

void CSINSGPSOD::SetMeasOD(double dSod, double ts)
{
    CVect3 dSn = sins.Cnb*(Cbo*CVect3(0,dSod*Kod,0));
    posDR = posDR + sins.eth.vn2dpos(dSn, 1.0);
    tODInt += ts;
    if(fabs(sins.wnb.k)<.5*glv.dps)
    {
        if(tODInt>1.10)
        {
            *(CVect3*)&Zk.dd[6] = sins.pos - posDR;
            tODInt = 0.0;
            measODValid = 1;
        }
    }
    else
    {
        tODInt = 0.0;
    }
}

void CSINSGPSOD::SetMeasYaw(double ymag)
{
    if(ymag>EPS || ymag<-EPS)  // !IsZero(yawGPS)
        if(sins.att.i<30*DEG && sins.att.i>-30*DEG)
        {
            *(CVect3*)&Zk.dd[9] = -diffYaw(sins.att.k, ymag);
            measMAGyawValid = 1;
        }
}

int CSINSGPSOD::Update(const CVect3 *pwm, const CVect3 *pvm, int nn, double ts)
{
    int res=TDUpdate(pwm, pvm, nn, ts, 5);
    return res;
}

/***************************  class CEarth  *********************************/
CEarth::CEarth(double a0, double f0, double g0)
{
    a = a0;	f = f0; wie = glv.wie;
    b = (1-f)*a;
    e = sqrt(a*a-b*b)/a;	e2 = e*e;
    gn = O31;  pgn = 0;
    Update(O31);
}

void CEarth::Update(const CVect3 &pos, const CVect3 &vn)
{
#ifdef PSINS_LOW_GRADE_MEMS
    this->pos = pos;  this->vn = vn;
    sl = sin(pos.i), cl = cos(pos.i), tl = sl/cl;
    double sq = 1-e2*sl*sl, sq2 = sqrt(sq);
    RMh = a*(1-e2)/sq/sq2+pos.k;	f_RMh = 1.0/RMh;
    RNh = a/sq2+pos.k;    clRNh = cl*RNh;  f_RNh = 1.0/RNh; f_clRNh = 1.0/clRNh;
//	wnie.i = 0.0,			wnie.j = wie*cl,		wnie.k = wie*sl;
//	wnen.i = -vn.j*f_RMh,	wnen.j = vn.i*f_RNh,	wnen.k = wnen.j*tl;
    wnin = wnie = wnen = O31;
    sl2 = sl*sl;
    gn.k = -( glv.g0*(1+5.27094e-3*sl2)-3.086e-6*pos.k );
    gcc = pgn ? *pgn : gn;
#else
    this->pos = pos;  this->vn = vn;
    sl = sin(pos.i), cl = cos(pos.i), tl = sl/cl;
    double sq = 1-e2*sl*sl, sq2 = sqrt(sq);
    RMh = a*(1-e2)/sq/sq2+pos.k;	f_RMh = 1.0/RMh;
    RNh = a/sq2+pos.k;    clRNh = cl*RNh;  f_RNh = 1.0/RNh; f_clRNh = 1.0/clRNh;
    wnie.i = 0.0,			wnie.j = wie*cl,		wnie.k = wie*sl;
    wnen.i = -vn.j*f_RMh,	wnen.j = vn.i*f_RNh,	wnen.k = wnen.j*tl;
    wnin = wnie + wnen;
    sl2 = sl*sl, sl4 = sl2*sl2;
    gn.k = -( glv.g0*(1+5.27094e-3*sl2+2.32718e-5*sl4)-3.086e-6*pos.k );
    gcc = pgn ? *pgn : gn;
    gcc -= (wnie+wnin)*vn;
#endif
}

CVect3 CEarth::vn2dpos(const CVect3 &vn, double ts) const
{
    return CVect3(vn.j*f_RMh, vn.i*f_clRNh, vn.k)*ts;
}

/***************************  class CIMU  *********************************/
CIMU::CIMU(void)
{
    nSamples = 1;
    preFirst = onePlusPre = true;
    phim = dvbm = wm_1 = vm_1 = O31;
}

void CIMU::Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples)
{
    static double conefactors[5][4] = {				// coning coefficients
        {2./3},										// 2
        {9./20, 27./20},							// 3
        {54./105, 92./105, 214./105},				// 4
        {250./504, 525./504, 650./504, 1375./504}	// 5
        };
    int i;
    double *pcf = conefactors[nSamples-2];
    CVect3 cm(0.0), sm(0.0), wmm(0.0), vmm(0.0);

//    psinsassert(nSamples>0 && nSamples<6);
    this->nSamples = nSamples;
    if(nSamples==1 && onePlusPre)  // one-plus-previous sample
    {
        if(preFirst) { wm_1=pwm[0]; vm_1=pvm[0]; preFirst=false; }
        cm = 1.0/12*wm_1;
        sm = 1.0/12*vm_1;
    }
    for(i=0; i<nSamples-1; i++)
    {
        cm += pcf[i]*pwm[i];
        sm += pcf[i]*pvm[i];
        wmm += pwm[i];
        vmm += pvm[i];
    }
    wm_1=pwm[i];  vm_1=pvm[i];
    wmm += pwm[i];
    vmm += pvm[i];
    phim = wmm + cm*pwm[i];
    dvbm = vmm + 1.0/2*wmm*vmm + (cm*pvm[i]+sm*pwm[i]);
}

void IMURFU(CVect3 *pwm, int nSamples, const char *str)
{
    for(int n=0; n<nSamples; n++)
    {
        CVect3 tmpwm;
        double *pw=(double*)&pwm[n].i;
        for(int i=0; i<3; i++,pw++)
        {
            switch(str[i])
            {
            case 'R':  tmpwm.i= *pw;  break;
            case 'L':  tmpwm.i=-*pw;  break;
            case 'F':  tmpwm.j= *pw;  break;
            case 'B':  tmpwm.j=-*pw;  break;
            case 'U':  tmpwm.k= *pw;  break;
            case 'D':  tmpwm.k=-*pw;  break;
            }
        }
        pwm[n] = tmpwm;
    }
}

void IMURFU(CVect3 *pwm, CVect3 *pvm, int nSamples, const char *str)
{
    IMURFU(pwm, nSamples, str);
    IMURFU(pvm, nSamples, str);
}

/***************************  class CSINS  *********************************/
CSINS::CSINS(const CVect3 &att0, const CVect3 &vn0, const CVect3 &pos0, double tk0)
{
    Init(a2qua(att0), vn0, pos0, tk0);
}

CSINS::CSINS(const CQuat &qnb0, const CVect3 &vn0, const CVect3 &pos0, double tk0)
{
    Init(qnb0, vn0, pos0, tk0);
}

void CSINS::Init(const CQuat &qnb0, const CVect3 &vn0, const CVect3 &pos0, double tk0)
{
    tk = tk0;  ts = nts = 1.0;
    velMax = 400.0; hgtMin = -RE*0.01, hgtMax = -hgtMin;
    qnb = qnb0;	vn = vn0, pos = pos0;
    Kg = Ka = I33; eb = db = Ka2 = O31;
    Maa = Mav = Map = Mva = Mvv = Mvp = Mpv = Mpp = O33;
    SetTauGA(CVect3(INF),CVect3(INF));
    CVect3 wib(0.0), fb=(~qnb)*CVect3(0,0,glv.g0);
    lvr = an = O31;
    Rwfa = CRAvar(9);
    isOpenloop = 0;
    Update(&wib, &fb, 1, 1.0); imu.preFirst = 1;
    tk = tk0;  ts = nts = 1.0; qnb = qnb0;	vn = vn0, pos = pos0;
    etm(); lever(); Extrap();
}

void CSINS::SetTauGA(const CVect3 &tauG, const CVect3 &tauA)
{
    tauGyro = tauG, tauAcc = tauA;
    _betaGyro.i = tauG.i>INF/2 ? 0.0 : -1.0/tauG.i;   // Gyro&Acc inverse correlation time for AR(1) model
    _betaGyro.j = tauG.j>INF/2 ? 0.0 : -1.0/tauG.j;
    _betaGyro.k = tauG.k>INF/2 ? 0.0 : -1.0/tauG.k;
    _betaAcc.i  = tauA.i>INF/2 ? 0.0 : -1.0/tauA.i;
    _betaAcc.j  = tauA.j>INF/2 ? 0.0 : -1.0/tauA.j;
    _betaAcc.k  = tauA.k>INF/2 ? 0.0 : -1.0/tauA.k;
}

void CSINS::Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts)
{
#ifdef PSINS_LOW_GRADE_MEMS
#pragma message("  PSINS_LOW_GRADE_MEMS")
    this->ts = ts;  nts = nSamples*ts;	tk += nts;
    double nts2 = nts/2;
    imu.Update(pwm, pvm, nSamples);
    imu.phim = Kg*imu.phim - eb*nts; imu.dvbm = Ka*imu.dvbm - db*nts;  // IMU calibration
    imu.dvbm.i -= Ka2.i*imu.dvbm.i*imu.dvbm.i/nts;
    imu.dvbm.j -= Ka2.j*imu.dvbm.j*imu.dvbm.j/nts;
    imu.dvbm.k -= Ka2.k*imu.dvbm.k*imu.dvbm.k/nts;
//	CVect3 vn01 = vn+an*nts2, pos01 = pos+eth.vn2dpos(vn01,nts2);
    if(!isOpenloop) eth.Update(pos);
    wib = imu.phim/nts; fb = imu.dvbm/nts;
    web = wib;
    wnb = wib;
    fn = qnb*fb;
    an = fn+eth.gcc;
    CVect3 vn1 = vn + an*nts;
    pos = pos + eth.vn2dpos(vn+vn1, nts2);	vn = vn1;
    qnb = qnb*rv2q(imu.phim);
    Cnb = q2mat(qnb); att = m2att(Cnb); Cbn = ~Cnb; vb = Cbn*vn;
#else
    this->ts = ts;  nts = nSamples*ts;	tk += nts;
    double nts2 = nts/2;
    imu.Update(pwm, pvm, nSamples);
    imu.phim = Kg*imu.phim - eb*nts;
    imu.dvbm = Ka*imu.dvbm - db*nts;  // IMU calibration
    CVect3 vn01 = vn+an*nts2, pos01 = pos+eth.vn2dpos(vn01,nts2);
    if(!isOpenloop) eth.Update(pos01, vn01);
    wib = imu.phim/nts; fb = imu.dvbm/nts;
    web = wib - Cbn*eth.wnie;
//	wnb = wib - (~(qnb*rv2q(imu.phim/2)))*eth.wnin;
    wnb = wib - Cbn*eth.wnin;
    fn = qnb*fb;
    an = rv2q(-1.0*eth.wnin*nts2)*fn+eth.gcc;
    CVect3 vn1 = vn + an*nts;
    pos = pos + eth.vn2dpos(vn+vn1, nts2);	vn = vn1;
    qnb = rv2q(-1.0*eth.wnin*nts)*qnb*rv2q(imu.phim);
    Cnb = q2mat(qnb); att = m2att(Cnb); Cbn = ~Cnb; vb = Cbn*vn;
#endif
    psinsassert(pos.i<85.0*PI/180 && pos.i>-85.0*PI/180);
    if(vn.i>velMax) vn.i=velMax; else if(vn.i<-velMax) vn.i=-velMax;
    if(vn.j>velMax) vn.j=velMax; else if(vn.j<-velMax) vn.j=-velMax;
    if(vn.k>velMax) vn.k=velMax; else if(vn.k<-velMax) vn.k=-velMax;
    if(pos.j>PI) pos.j-=_2PI; else if(pos.j<-PI) pos.j+=_2PI;
    if(pos.k>hgtMax) pos.k=hgtMax; else if(pos.k<hgtMin) pos.k=hgtMin;
    CVect wfa(9);
    *(CVect3*)&wfa.dd[0]=wib, *(CVect3*)&wfa.dd[3]=fb, *(CVect3*)&wfa.dd[6]=Cbn*an;
    Rwfa.Update(wfa, nts);
}

void CSINS::Extrap(const CVect3 &wm, const CVect3 &vm, double ts)
{
    if(ts<1.0e-6)  // reset
    {
        qnbE = qnb, vnE = vn, posE = pos, attE = att;
    }
    else
    {
        vnE = vnE + qnbE*vm + eth.gcc*ts;
        posE = posE + eth.vn2dpos(vnE,ts);
        qnbE = qnbE*rv2q(wm); attE = q2att(qnbE);
    }
}

void CSINS::Extrap(double extts)
{
    double k = extts/nts;
    vnE = vn + qnb*imu.dvbm*k + eth.gcc*extts;
    posE = pos + eth.vn2dpos(vn,extts);
    attE = q2att(qnb*rv2q(imu.phim*k));
}

void CSINS::lever(const CVect3 &dL)
{
    if(!IsZero(dL,EPS)) lvr = dL;
    Mpv = CMat3(0,eth.f_RMh,0, eth.f_clRNh,0,0, 0,0,1);
    CW = Cnb*askew(web), MpvCnb = Mpv*Cnb;
    vnL = vn + CW*lvr; posL = pos + MpvCnb*lvr;
}

void CSINS::etm(void)
{
#ifdef PSINS_LOW_GRADE_MEMS
    Mva = askew(fn);
    Mpv = CMat3(0,eth.f_RMh,0, eth.f_clRNh,0,0, 0,0,1);
#else
    double tl=eth.tl, secl=1.0/eth.cl, secl2=secl*secl,
        wN=eth.wnie.j, wU=eth.wnie.k, vE=vn.i, vN=vn.j;
    double f_RMh=eth.f_RMh, f_RNh=eth.f_RNh, f_clRNh=eth.f_clRNh,
        f_RMh2=f_RMh*f_RMh, f_RNh2=f_RNh*f_RNh;
    CMat3 Avn=askew(vn),
        Mp1(0,0,0, -wU,0,0, wN,0,0),
        Mp2(0,0,vN*f_RMh2, 0,0,-vE*f_RNh2, vE*secl2*f_RNh,0,-vE*tl*f_RNh2);
    Maa = askew(-1.0*eth.wnin);
    Mav = CMat3(0,-f_RMh,0, f_RNh,0,0, tl*f_RNh,0,0);
    Map = Mp1+Mp2;
    Mva = askew(fn);
    Mvv = Avn*Mav - askew(eth.wnie+eth.wnin);
    Mvp = Avn*(Mp1+Map);
    double scl = eth.sl*eth.cl;
    Mvp.e20 = Mvp.e20-glv.g0*(5.27094e-3*2*scl+2.32718e-5*4*eth.sl2*scl); Mvp.e22 = Mvp.e22+3.086e-6;
    Mpv = CMat3(0,f_RMh,0, f_clRNh,0,0, 0,0,1);
    Mpp = CMat3(0,0,-vN*f_RMh2, vE*tl*f_clRNh,0,-vE*secl*f_RNh2, 0,0,0);
#endif
}

//CAVPInterp::CAVPInterp(double ts)
//{
//    this->ts = ts;
//    ipush = 0;
//    att = vn = pos = 0.0;
//    for(int i=0; i<AVPINUM; i++) atti[i]=vni[i]=posi[i]=i;
//}

//void CAVPInterp::Push(const CVect3 &attk, const CVect3 &vnk, const CVect3 &posk)
//{
//    if(++ipush>=AVPINUM) ipush = 0;
//    atti[ipush] = attk; vni[ipush] = vnk; posi[ipush] = posk;
//}

//int CAVPInterp::Interp(double tpast)
//{
//    int res = 1, k, k1, k2;
//    if(tpast<-AVPINUM*ts||tpast>0) return (res=0);
//    k = (int)(-tpast/ts);
//    if((k2=ipush-k)<0) k2 += AVPINUM;
//    if((k1=k2-1)<0)  k1 += AVPINUM;
//    double tleft = -tpast - k*ts;
//    if(tleft>0.99*ts)
//    {	att = atti[k1], vn = vni[k1], pos = posi[k1]; }
//    else if(tleft<0.01*ts)
//    {	att = atti[k2], vn = vni[k2], pos = posi[k2]; }
//    else
//    {
//        double b=tleft/ts, a=1-b;
//        att = b*atti[k1]+a*atti[k2], vn = b*vni[k1]+a*vni[k2],	pos = b*posi[k1]+a*posi[k2];
//        if(norm(att-atti[k1])>10.0*DEG) res=0;
//        else if(fabs(pos.j-posi[k1].j)>1.0*DEG) res=0;
//    }
//    return res;
//}


/*********************  class CMahony AHRS  ************************/
CMahony::CMahony(double tau, const CQuat &qnb0)
{
    SetTau(tau);
    qnb = qnb0;
    Cnb = q2mat(qnb);
    exyzInt = O31;  ebMax = I31*glv.dps;
    tk = 0.0;
}

void CMahony::SetTau(double tau)
{
    double beta = 2.146/tau;
    Kp = 2.0f*beta, Ki = beta*beta;
}

void CMahony::Update(const CVect3 &wm, const CVect3 &vm, double ts, const CVect3 &mag)
{
    double nm;
    CVect3 acc0, mag0, exyz, bxyz, wxyz;

    nm = norm(vm)/ts;  // f (in m/s^2)
    acc0 = nm>0.1 ? vm/(nm*ts) : O31;
    nm = norm(mag);    // mag (in Gauss)
    if(nm>0.1)
    {
        mag0 = mag/nm;
        bxyz = Cnb*mag0;
        bxyz.j = normXY(bxyz); bxyz.i = 0.0;
        wxyz = (~Cnb)*bxyz;
    }
    else
    {
        mag0 = wxyz = O31;
    }
    exyz = *((CVect3*)&Cnb.e20)*acc0 + wxyz*mag0;
    exyzInt += exyz*(Ki*ts);
    qnb *= rv2q(wm-(Kp*exyz+exyzInt)*ts);
    Cnb = q2mat(qnb);
    tk += ts;
    if(exyzInt.i>ebMax.i)  exyzInt.i=ebMax.i;  else if(exyzInt.i<-ebMax.i)  exyzInt.i=-ebMax.i;
    if(exyzInt.j>ebMax.j)  exyzInt.j=ebMax.j;  else if(exyzInt.j<-ebMax.j)  exyzInt.j=-ebMax.j;
    if(exyzInt.k>ebMax.k)  exyzInt.k=ebMax.k;  else if(exyzInt.k<-ebMax.k)  exyzInt.k=-ebMax.k;
}

void CMahony::Update(const CVect3 &gyro, const CVect3 &acc, const CVect3 &mag, double ts)
{
    double nm;
    CVect3 acc0, mag0, exyz, bxyz, wxyz;

    nm = norm(acc);
    acc0 = nm>0.1 ? acc/nm : O31;
    nm = norm(mag);
    mag0 = nm>0.1 ? mag/nm : O31;
    bxyz = Cnb*mag0;
    bxyz.j = normXY(bxyz); bxyz.i = 0.0;
    wxyz = (~Cnb)*bxyz;
    exyz = *((CVect3*)&Cnb.e20)*acc0 + wxyz*mag0;
    exyzInt += exyz*(Ki*ts);
    qnb *= rv2q((gyro*glv.dps-Kp*exyz-exyzInt)*ts);
    Cnb = q2mat(qnb);
    tk += ts;
}

/*********************  class Quat&EKF based AHRS  ************************/
CQEAHRS::CQEAHRS(double ts):CKalman(7,3)
{
    double sts = sqrt(ts);
    Pmax.Set2(2.0,2.0,2.0,2.0, 1000*glv.dph,1000.0*glv.dph,1000.0*glv.dph);
    Pmin.Set2(0.001,0.001,0.001,0.001, 10.0*glv.dph,10.0*glv.dph,10.0*glv.dph);
    Pk.SetDiag2(1.0,1.0,1.0,1.0, 1000.0*glv.dph,1000.0*glv.dph,1000.0*glv.dph);
    Qt.Set2(10.0*glv.dpsh,10.0*glv.dpsh,10.0*glv.dpsh, 10.0*glv.dphpsh,10.0*glv.dphpsh,10.0*glv.dphpsh);
    Rt.Set2(100.0*glv.mg/sts,100.0*glv.mg/sts, 1.0*glv.deg/sts);
    Xk(0) = 1.0;
    Cnb = q2mat(*(CQuat*)&Xk.dd[0]);
}

void CQEAHRS::Update(const CVect3 &gyro, const CVect3 &acc, const CVect3 &mag, double ts)
{
    double q0, q1, q2, q3, wx, wy, wz, fx, fy, fz, mx, my, mz, h11, h12, h21, h22;
    q0 = Xk.dd[0];		 q1 = Xk.dd[1];		  q2 = Xk.dd[2];		q3 = Xk.dd[3];
    wx = gyro.i*glv.dps; wy = gyro.j*glv.dps; wz = gyro.k*glv.dps;
    fx = acc.i;			 fy = acc.j;		  fz = acc.k;
    mx = mag.i;			 my = mag.j;		  mz = mag.k;
    // Ft
                    0; Ft.dd[ 1] = -wx/2; Ft.dd[ 2] = -wy/2; Ft.dd[ 3] = -wz/2;  Ft.dd[ 4] =  q1/2; Ft.dd[ 5] =  q2/2; Ft.dd[ 6] =  q3/2;
    Ft.dd[ 7] =  wx/2;                 0; Ft.dd[ 9] =  wz/2; Ft.dd[10] = -wy/2;  Ft.dd[11] = -q0/2; Ft.dd[12] =  q3/2; Ft.dd[13] = -q2/2;
    Ft.dd[14] =  wy/2; Ft.dd[15] = -wz/2;                 0; Ft.dd[17] =  wx/2;  Ft.dd[18] = -q3/2; Ft.dd[18] = -q0/2; Ft.dd[20] =  q1/2;
    Ft.dd[21] =  wz/2; Ft.dd[22] =  wy/2; Ft.dd[23] = -wx/2;                 0;  Ft.dd[25] =  q2/2; Ft.dd[26] = -q1/2; Ft.dd[27] = -q0/2;
    // Hk
    h11 = fx*q0-fy*q3+fz*q2;  h12 = fx*q1+fy*q2+fz*q3;
    h21 = fx*q3+fy*q0-fz*q1;  h22 = fx*q2-fy*q1-fz*q0;
    Hk.dd[ 0] = h11*2;  Hk.dd[ 1] = h12*2;  Hk.dd[ 2] = -h22*2;  Hk.dd[ 3] = -h21*2;
    Hk.dd[ 7] = h21*2;  Hk.dd[ 8] = h22*2;  Hk.dd[ 9] =  h12*2;  Hk.dd[10] =  h11*2;
/*	CVect3 magH = Cnb*mag;
    double C11=Cnb.e11, C01=Cnb.e01, CC=C11*C11+C01*C01;
    if(normXY(magH)>0.01 && CC>0.25)  // CC>0.25 <=> pitch<60deg
    {
        double f2=2.0/CC;
        Hk.dd[14] = (q3*C11+q0*C01)*f2,  Hk.dd[15] = (-q2*C11-q1*C01)*f2,  Hk.dd[16] = (-q1*C11+q2*C01)*f2,  Hk.dd[17] = (q0*C11-q3*C01)*f2;
        Zk.dd[2] = atan2(magH.i, magH.j);
    }
    else
    {
        Hk.dd[14] = Hk.dd[15] = Hk.dd[16] = Hk.dd[17] = 0.0;
        Zk.dd[2] = 0.0;
    }*/

    SetMeasFlag(0x03);
    TimeUpdate(ts);
    MeasUpdate();
    XPConstrain();
    normlize((CQuat*)&Xk.dd[0]);
    Cnb = q2mat(*(CQuat*)&Xk.dd[0]);
}

/******************************  File Read or Write *********************************/

#include "io.h"
char* time2fname(void)
{
    static char PSINSfname[32];
    time_t tt;  time(&tt);
    tm *Time = localtime(&tt);
    strftime(PSINSfname, 32, "PSINS%Y%m%d_%H%M%S.bin", Time);
    return PSINSfname;
}

char CFileRdWt::dirIn[256] = {0}, CFileRdWt::dirOut[256] = {0};

void CFileRdWt::Dir(const char *dirI, const char *dirO)  // set dir
{
    int len = strlen(dirI);
    memcpy(dirIn, dirI, len);
    if(dirIn[len-1]!='\\') { dirIn[len]='\\'; dirIn[len+1]='\0'; }
    if(dirO)
    {
        len = strlen(dirO);
        memcpy(dirOut, dirO, len);
        if(dirOut[len-1]!='\\') { dirOut[len]='\\'; dirOut[len+1]='\0'; }
    }
    else
        memcpy(dirOut, dirIn, strlen(dirIn));
    if(_access(dirIn,0)==-1) dirIn[0]='\0';
    if(_access(dirOut,0)==-1) dirOut[0]='\0';
}

CFileRdWt::CFileRdWt()
{
    f = 0;
}

CFileRdWt::CFileRdWt(const char *fname0, int columns0)
{
    f = 0;
    Init(fname0, columns0);
    memset(buff, 0, sizeof(buff));
}

CFileRdWt::~CFileRdWt()
{
    if(f) { fclose(f); f=(FILE*)NULL; }
}

void CFileRdWt::Init(const char *fname0, int columns0)
{
    fname[0]='\0';
    int findc=0, len0=strlen(fname0);
    for(int i=0; i<len0; i++)	{ if(fname0[i]=='\\') { findc=1; break; } }
    columns = columns0;
    if(columns==0)		// file write
    {	if(dirOut[0]!=0&&findc==0)	{ strcat(fname, dirOut); } }
    else				// file read
    {	if(dirIn[0]!=0&&findc==0)	{ strcat(fname, dirIn); } }
    strcat(fname, fname0);
    if(f) this->~CFileRdWt();
    if(columns==0)				// bin file write
    {
        f = fopen(fname, "wb");
    }
    else if(columns<0)			// bin file read
    {
        f = fopen(fname, "rb");
    }
    else if(columns>0)			// txt file read
    {
        f = fopen(fname, "rt");
        if(!f) return;
        fpos_t pos;
        while(1)  // skip txt-file comments
        {
            pos = ftell(f);
            fgets(line, sizeof(line), f);
            if(feof(f)) break;
            int allSpace=1, allDigital=1;
            for(int i=0; i<sizeof(line); i++)
            {
                char c = line[i];
                if(c=='\n') break;
                if(c!=' ') allSpace = 0;
                if( !(c==' '||c==','||c==';'||c==':'||c=='+'||c=='-'||c=='.'||c=='\t'
                    ||c=='e'||c=='E'||c=='d'||c=='D'||('9'>=c&&c>='0')) )
                    allDigital = 0;
            }
            if(!allSpace && allDigital) break;
        }
        fsetpos(f, &pos);
        this->columns = columns;
        for(int i=0; i<columns; i++)
        { sstr[4*i+0]='%', sstr[4*i+1]='l', sstr[4*i+2]='f', sstr[4*i+3]=' ', sstr[4*i+4]='\0'; }
    }
    else
    {
        f = 0;
    }
    linelen = 0;
    totsize = 0;
}

int CFileRdWt::load(int lines, bool txtDelComma)
{
    if(columns<0)			// bin file read
    {
        if(lines>1)
            fseek(f, (lines-1)*(-columns)*sizeof(double), SEEK_CUR);
        //
        fread(buff, -columns, sizeof(double), f);
    }
    else					// txt file read
    {
        for(int i=0; i<lines; i++)  fgets(line, sizeof(line), f);
        if(txtDelComma)
        {
            for(char *pc=line, *pend=line+sizeof(line); pc<pend; pc++)
            {
                if(*pc==','||*pc==';'||*pc==':'||*pc=='\t:') *pc=' ';
                else if(*pc=='\0') break;
            }
        }
        if(columns<10)
            sscanf(line, sstr,
                &buff[ 0], &buff[ 1], &buff[ 2], &buff[ 3], &buff[ 4], &buff[ 5], &buff[ 6], &buff[ 7], &buff[ 8], &buff[ 9]
                );
        else if(columns<20)
            sscanf(line, sstr,
                &buff[ 0], &buff[ 1], &buff[ 2], &buff[ 3], &buff[ 4], &buff[ 5], &buff[ 6], &buff[ 7], &buff[ 8], &buff[ 9],
                &buff[10], &buff[11], &buff[12], &buff[13], &buff[14], &buff[15], &buff[16], &buff[17], &buff[18], &buff[19]
                );
        else if(columns<40)
            sscanf(line, sstr,
                &buff[ 0], &buff[ 1], &buff[ 2], &buff[ 3], &buff[ 4], &buff[ 5], &buff[ 6], &buff[ 7], &buff[ 8], &buff[ 9],
                &buff[10], &buff[11], &buff[12], &buff[13], &buff[14], &buff[15], &buff[16], &buff[17], &buff[18], &buff[19],
                &buff[20], &buff[21], &buff[22], &buff[23], &buff[24], &buff[25], &buff[26], &buff[27], &buff[28], &buff[29],
                &buff[30], &buff[31], &buff[32], &buff[33], &buff[34], &buff[35], &buff[36], &buff[37], &buff[38], &buff[39]
                );
    }
    linelen += lines;
    if(feof(f))  return 0;
    else return 1;
}

int CFileRdWt::loadf32(int lines)	// float32 bin file read
{
    if(lines>1)
        fseek(f, (lines-1)*(-columns)*sizeof(float), SEEK_CUR);
    fread(buff32, -columns, sizeof(float), f);
    for(int i=0; i<-columns; i++) buff[i]=buff32[i];
    linelen += lines;
    if(feof(f))  return 0;
    else return 1;
}

long CFileRdWt::load(BYTE *buf, long bufsize)  // load bin file
{
    long cur = ftell(f);
    fseek(f, 0L, SEEK_END);
    long rest = ftell(f)-cur;
    fseek(f, cur, SEEK_SET);
    if(bufsize>rest) bufsize=rest;
    fread(buf, bufsize, 1, f);
    return bufsize;
}

long CFileRdWt::filesize(int opt)
{
    long cur = ftell(f);
    if(totsize==0)
    {
        fseek(f, 0L, SEEK_END);
        totsize = ftell(f);
        fseek(f, cur, SEEK_SET);
    }
    remsize = totsize-cur;
    return opt ? remsize : totsize;  // opt==1 for remain_size, ==0 for total_size
}

int CFileRdWt::getl(void)	// txt file get a line
{
    fgets(line, sizeof(line), f);
    return strlen(line);
}

CFileRdWt& CFileRdWt::operator<<(double d)
{
    fwrite(&d, 1, sizeof(double), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CVect3 &v)
{
    fwrite(&v, 1, sizeof(v), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CQuat &q)
{
    fwrite(&q, 1, sizeof(q), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CMat3 &m)
{
    fwrite(&m, 1, sizeof(m), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CVect &v)
{
    fwrite(v.dd, v.clm*v.row, sizeof(double), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CMat &m)
{
    fwrite(m.dd, m.clm*m.row, sizeof(double), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CRAvar &R)
{
    fwrite(R.R0, R.nR0, sizeof(double), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CRMemory &m)
{
    fwrite(m.pMemStart0, m.memLen, 1, f);
    return *this;
}

CFileRdWt& CFileRdWt::operator<<(const CAligni0 &aln)
{
    return *this<<q2att(aln.qnb)<<aln.vib0<<aln.Pi02<<aln.tk;
}

CFileRdWt& CFileRdWt::operator<<(const CSINS &sins)
{
    return *this<<sins.att<<sins.vn<<sins.pos<<sins.eb<<sins.db<<sins.tk;
}

CFileRdWt& CFileRdWt::operator<<(CKalman &kf)
{
    *this<<kf.Xk<<diag(kf.Pk)<<kf.Rt<<(double)kf.measflaglog<<kf.kftk;
    kf.measflaglog = 0;
    return *this;
}

CFileRdWt& CFileRdWt::operator>>(double &d)
{
    fread(&d, 1, sizeof(double), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator>>(CVect3 &v)
{
    fread(&v, 1, sizeof(v), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator>>(CQuat &q)
{
    fread(&q, 1, sizeof(q), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator>>(CMat3 &m)
{
    fread(&m, 1, sizeof(m), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator>>(CVect &v)
{
    fread(v.dd, v.clm*v.row, sizeof(double), f);
    return *this;
}

CFileRdWt& CFileRdWt::operator>>(CMat &m)
{
    fread(m.dd, m.clm*m.row, sizeof(double), f);
    return *this;
}

/***************************  function AlignCoarse  *********************************/
CVect3 Alignsb(CVect3 wmm, CVect3 vmm, double latitude)
{
    double T11, T12, T13, T21, T22, T23, T31, T32, T33;
    double cl = cos(latitude), tl = tan(latitude), nn;
    CVect3 wbib = wmm / norm(wmm),  fb = vmm / norm(vmm);
    T31 = fb.i,				 T32 = fb.j,			 	T33 = fb.k;
    T21 = wbib.i/cl-T31*tl,	 T22 = wbib.j/cl-T32*tl,	T23 = wbib.k/cl-T33*tl;		nn = sqrt(T21*T21+T22*T22+T23*T23);  T21 /= nn, T22 /= nn, T23 /= nn;
    T11 = T22*T33-T23*T32,	 T12 = T23*T31-T21*T33,		T13 = T21*T32-T22*T31;		nn = sqrt(T11*T11+T12*T12+T13*T13);  T11 /= nn, T12 /= nn, T13 /= nn;
    CMat3 Cnb(T11, T12, T13, T21, T22, T23, T31, T32, T33);
    return m2att(Cnb);
}

/***************************  CAligni0  *********************************/
CAligni0::CAligni0(const CVect3 &pos0, const CVect3 &vel0, int velAid0)
{
    eth.Update(pos0);
    this->vel0 = vel0, velAid = velAid0;
    tk = 0;
    t0 = t1 = 10, t2 = 0;
    wmm = vmm = vib0 = vi0 = Pib01 = Pib02 = Pi01 = Pi02 = O31;
    qib0b = CQuat(1.0);
}

CQuat CAligni0::Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts, const CVect3 &vel)
{
    double nts = nSamples*ts;
    imu.Update(pwm, pvm, nSamples);
    wmm = wmm + imu.phim;  vmm = vmm + imu.dvbm;
    // vtmp = qib0b * (vm + 1/2 * wm X vm)
    CVect3 vtmp = qib0b*imu.dvbm;
    // vtmp1 = qni0' * [dvn+(wnin+wnie)Xvn-gn] * ts;
    tk += nts;
    CMat3 Ci0n = pos2Cen(CVect3(eth.pos.i,eth.wie*tk,0.0));
    CVect3 vtmp1 = Ci0n*(-1.0*eth.gn*nts);
    if(velAid>0)
    {
        CVect3 dv=vel-vel0;  vel0 = vel;
        if(velAid==1)		vtmp1 += Ci0n*dv;				// for GPS vn-aided
        else if(velAid==2)	vtmp -= qib0b*dv+imu.phim*vel;	// for OD vb-aided
    }
    // Pib02 = Pib02 + vib0*ts, Pi02 = Pi02 + vi0*ts
    vib0 = vib0 + vtmp,		 vi0 = vi0 + vtmp1;
    Pib02 = Pib02 + vib0*nts, Pi02 = Pi02 + vi0*nts;
    //
    if(++t2>3*t0)
    {
        t0 = t1, Pib01 = tmpPib0, Pi01 = tmpPi0;
    }
    else if(t2>2*t0 && t1==t0)
    {
        t1 = t2, tmpPib0 = Pib02, tmpPi0 = Pi02;
    }
    //
    qib0b = qib0b*rv2q(imu.phim);
    // qnb=qni0*qiib0*qib0b
    qnbsb = a2qua(Alignsb(wmm, vmm, eth.pos.i));
    if(t2<100)
    {
        qnb0 = qnb = CQuat(1.0);
    }
    else if(t2<1000)
    {
        qnb0 = qnb = qnbsb;
    }
    else
    {
        CQuat qi0ib0 = m2qua(dv2att(Pi01, Pi02, Pib01, Pib02));
        qnb0 = (~m2qua(pos2Cen(CVect3(eth.pos.i,0.0,0.0))))*qi0ib0;
        qnb = (~m2qua(Ci0n))*qi0ib0*qib0b;
    }
    return qnb;
}

/***************************  CAlignkf  *********************************/
CAlignkf::CAlignkf(void):CSINSTDKF(15, 6)
{
}

void CAlignkf::Init(const CSINS &sins0)
{
    CSINSKF::Init(sins0, -1);
    Pmax.Set2(10.0*glv.deg, 10.0*glv.deg, 30.0*glv.deg, 50.0, 50.0, 50.0, 1.0e4/glv.Re, 1.0e4/glv.Re, 1.0e4,
        10.0*glv.dph, 10.0*glv.dph, 10.0*glv.dph, 10.0*glv.mg, 10.0*glv.mg, 10.0*glv.mg);
    Pmin.Set2(0.01*glv.min, 0.01*glv.min, 0.1*glv.min, 0.001, 0.001, 0.01, 1.0 / glv.Re, 1.0 / glv.Re, 0.1,
        0.001*glv.dph, 0.001*glv.dph, 0.001*glv.dph, .50*glv.ug, .50*glv.ug, 1.50*glv.ug);
    Pk.SetDiag2(.10*glv.deg, .10*glv.deg, 10.0*glv.deg, 1.0, 1.0, 1.0, 100.0 / glv.Re, 100.0 / glv.Re, 100.0,
        0.05*glv.dph, 0.05*glv.dph, 0.01*glv.dph, .10*glv.mg, .10*glv.mg, .10*glv.mg);
    Qt.Set2(0.001*glv.dpsh, 0.001*glv.dpsh, 0.001*glv.dpsh, 1.0*glv.ugpsHz, 1.0*glv.ugpsHz, 1.0*glv.ugpsHz, 0.0, 0.0, 0.0,
        0.0*glv.dphpsh, 0.0*glv.dphpsh, 0.0*glv.dphpsh, 0.0*glv.ugpsh, 0.0*glv.ugpsh, 0.0*glv.ugpsh);
    Xmax.Set(INF,INF,INF, INF,INF,INF, INF,INF,INF, 0.1*glv.dph,0.1*glv.dph,0.1*glv.dph, 200.0*glv.ug,200.0*glv.ug,200.0*glv.ug);
    Rt.Set2(0.1, 0.1, 0.1, 100.0 / glv.Re, 100.0 / glv.Re, 300.0);
//	Rmax = Rt * 100;  Rmin = Rt*0.01;  Rb = 0.9;
    FBTau.Set(1.0, 1.0, 10.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0);
    mvnk = 0;
    mvnts = 0.0;
    mvn = O31;
    pos0 = sins.pos;
}

int CAlignkf::Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts)
{
    mvnk ++;
    mvnts += nSamples*ts;
    mvn += sins.vn;
    if(mvnts>.1)
    {
        *(CVect3*)&Zk.dd[0] = mvn*(1.0/mvnk);
        if(norm(sins.wnb)<0.1*glv.dps)
            SetMeasFlag(0007);
        mvnk = 0;
        mvnts = 0.0;
        mvn = O31;
        sins.pos = pos0;
    }
    int res=TDUpdate(pwm, pvm, nSamples, ts, 5);
    return res;
}

int CAlignkf::Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts, const CVect3 &vnr)
{
    if(norm(vnr)>0)
    {
        *(CVect3*)&Zk.dd[0] = sins.vn-vnr;
        if(norm(sins.wnb)<1.1*glv.dps)
            SetMeasFlag(0007);
    }
    int res=TDUpdate(pwm, pvm, nSamples, ts, 5);
    return res;
}

/***************************  kfapp  *********************************/

CKFApp::CKFApp(double ts):CSINSTDKF(34, 6)
{
    lvGPS = O31;
}

void CKFApp::Init(const CSINS &sins0, int grade)
{
    CSINSKF::Init(sins0);
    sins.lever(-1.0*lvGPS);
    sins.pos = sins.posL;
    Pk.SetDiag2(0.1*DEG, 0.1*DEG, 1.0*DEG,	 1.0, 1.0, 1.0,  10.0/RE, 10.0/RE, 10.0,
        0.01*DPH, 0.01*DPH, 0.01*DPH,	100.0*UG, 100.0*UG, 500.0*UG,
        1.0, 1.0, 1.0, 0.001,
        1000.0*PPM, 10.0*SEC, 10.0*SEC,  10.0*SEC, 1000.0*PPM, 10.0*SEC,  10.0*SEC, 10.0*SEC, 1000.0*PPM,
        1000.0*PPM, 10.0*SEC, 10.0*SEC,  1000.0*PPM, 10.0*SEC,  10.0*PPM
        );
    Pmin.Set2(1.1*SEC, 1.1*SEC, 10.0*SEC,	 0.001, 0.001, 0.001,  0.001/RE, 0.001/RE, 0.001,
        0.001*DPH, 0.001*DPH, 0.001*DPH,	10.0*UG, 10.0*UG, 20.0*UG,
        0.01, 0.01, 0.01,  0.0001,
        0.0*PPM, 0.0*SEC, 0.0*SEC,  0.0*SEC, 0.0*PPM, 0.0*SEC,  0.0*SEC, 0.0*SEC, 0.0*PPM,
        0.0*PPM, 0.0*SEC, 0.0*SEC,  0.0*PPM, 0.0*SEC,  0.0*PPM
        );
    Qt.Set2(0.01*glv.dpsh, 0.01*glv.dpsh, 0.01*glv.dpsh,  10.0*glv.ugpsHz, 10.0*glv.ugpsHz, 20.0*glv.ugpsHz,  0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,  0.0,
        0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,  0.0, 0.0, 0.0
        );
    FBTau.Set(1.0, 1.0, 1.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
        1.0, 1.0, 1.0,  1.0, 1.0, 1.0,
        INF, INF, INF,  INF,
        INF, INF, INF,  INF, INF, INF,  INF, INF, INF,
        INF, INF, INF,  INF, INF, INF
        );
    Rt.Set2(0.10, 0.10, 0.10, .10/RE, .10/RE, .10);
    Rmax = Rt * 100.;  Rmin = Rt*0.01;  Rb = 0.9;
    rts = 1.0;
}

void CKFApp::SetMeasGPS(const CVect3 &posGPS, const CVect3 &vnGPS, double dt)
{
    if (!IsZero(posGPS,EPS) && sins.wnb.k<30.0 * DPS && sins.wnb.k>-30.0 * DPS)
    {
        sins.lever(lvGPS);
        if (!IsZero(vnGPS,EPS))
        {
            *(CVect3*)&Zk.dd[0] = sins.vnL-sins.an*dt - vnGPS;
            SetMeasFlag(00007);
        }
        *(CVect3*)&Zk.dd[3] = sins.posL-sins.Mpv*sins.vn*dt - posGPS;
        SetMeasFlag(00070);
    }
}

int CKFApp::Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples, double ts)
{
    int res = TDUpdate(pwm, pvm, nSamples, ts, 5);
    CVect3 lv = (lvGPS+*(CVect3*)&Xk.dd[15]);
    vnRes = sins.vn + sins.Cnb*askew(sins.web)*lv + sins.an*Xk.dd[18];
    posRes = sins.pos + sins.MpvCnb*lv + sins.Mpv*sins.vn*Xk.dd[18];
    xpt.att = sins.att; xpt.vn = vnRes; xpt.pos = posRes;
    double *p=&xpt.Patt.i, *p1=&Pk.dd[0];
    for(int i=0; i<9; i++,p++,p1+=nq+1) *p = *p1;
    return res;
}

void CKFApp::Reverse(void)
{
    sins.eth.wie = -sins.eth.wie;  sins.vn = -1.0*sins.vn; sins.eb = -1.0*sins.eb;
    vnRes = -1.0*vnRes; xpt.vn = -1.0*xpt.vn;
    int idx[] = {3,4,5, 9,10,11, 18};
    for(int k=0; k<(sizeof(idx)/sizeof(int)); k++) Xk.dd[idx[k]] = -Xk.dd[idx[k]];
    TDReset();
    int idxp[] = {0,1, 3,4,5, 6,7,8, 14};
    Pset = diag(Pk);
    for(int p=0; p<(sizeof(idxp)/sizeof(int)); p++) Pset.dd[idxp[p]] = 10.0*Pset.dd[idxp[p]];
}

unsigned short swap16(unsigned short ui16)
{
    unsigned char *p = (unsigned char*)&ui16, c;
    c = p[0]; p[0] = p[1]; p[1] = c;
    return ui16;
}

unsigned char* swap24(unsigned char* puc3, unsigned char* pres)
{
    static unsigned char resc[3];
    if (pres == NULL) pres = resc;
    pres[0] = puc3[2]; pres[1] = puc3[1];  pres[2] = puc3[0];
    return pres;
}

unsigned int swap32(unsigned int ui32)
{
    unsigned char *p = (unsigned char*)&ui32, c;
    c = p[0]; p[0] = p[3]; p[3] = c;
    c = p[1]; p[1] = p[2]; p[2] = c;
    return ui32;
}

unsigned long swap64(unsigned long ui64)
{
    unsigned char *p = (unsigned char*)&ui64, c;
    c = p[0]; p[0] = p[7]; p[7] = c;
    c = p[1]; p[1] = p[6]; p[6] = c;
    c = p[2]; p[2] = p[5]; p[5] = c;
    c = p[3]; p[3] = p[4]; p[4] = c;
    return ui64;
}

unsigned char* int24(unsigned char *pchar3, int int32)
{
    unsigned char *p = (unsigned char*)&int32;
    *pchar3++ = *p++, *pchar3++ = *p++, *pchar3 = *p;
    return pchar3-2;
}

int diffint24(const unsigned char *pc1, const unsigned char *pc0)
{
    int i1, i0;
    unsigned char *p1 = (unsigned char*)&i1, *p0 = (unsigned char*)&i0;
    *p1++ = 0, *p1++ = *pc1++, *p1++ = *pc1++, *p1 = *pc1;
    *p0++ = 0, *p0++ = *pc0++, *p0++ = *pc0++, *p0 = *pc0;
    return (i1 - i0) / 256;
}
