#ifndef BASEDCLASS_H
#define BASEDCLASS_H

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

//矩阵最大维度
#define PSINS_MATRIX_MAX_DIM	43
#define MMD		PSINS_MATRIX_MAX_DIM
#define MMD2	(MMD*MMD)

#define EPS		2.220446049e-16F
#define INF		3.402823466e+30F
//常数定义
#define PI		3.14159265358979
#define PI_2	(PI/2.0)
#define PI_4	(PI/4.0)
#define _2PI	(2.0*PI)

//常数定义
#define DEG		(PI/180.0)
#define DPS		(DEG/1.0)		// deg/s
#define SEC		(DEG/3600.0)	// arcsec
#define DPH		(DEG/3600.0)	// deg/h
#define G0		9.7803267714
#define UG		(G0/1.0e6)		// ug
#define RE		6378137.0
#define PPM		1.0e-6

//基本函数定义
#define max(x,y)        ( (x)>=(y)?(x):(y) )
#define min(x,y)        ( (x)<=(y)?(x):(y) )
#define pow2(x)			((x)*(x))
#define asinEx(x)		asin(range(x, -1.0, 1.0))
#define acosEx(x)		acos(range(x, -1.0, 1.0))

//360  <==>  180
#define CC180toC360(yaw)  ( (yaw)>0.0 ? (_2PI-(yaw)) : -(yaw) )   // counter-clockwise +-180deg -> clockwise 0~360deg for yaw
#define C360toCC180(yaw)  ( (yaw)>=PI ? (_2PI-(yaw)) : -(yaw) )   // clockwise 0~360deg -> counter-clockwise +-180deg for yaw

typedef unsigned char BYTE;
//一些函数的定义
double	r2dm(double r);
double	dm2r(double dm);
bool	IsZero(double f, double eps=(double)EPS);
int		sign(double val, double eps=(double)EPS);
double	range(double val, double minVal, double maxVal);
double	atan2Ex(double y, double x);
double  diffYaw(double yaw, double yaw0);
double	MKQt(double sR, double tau);
double randn(double mu, double sigma);

//全局变量
class CGLV;
//三维向量类
class CVect3;
//三维矩阵类
class CMat3;
//四元数类
class CQuat;
//向量类
class CVect;
//矩阵类
class CMat;

//地球参数类
class CEarth;
//定义
#define LLH(latitude,longitude,height)		CVect3((latitude)*PI/180,(longitude)*PI/180,height)
#define PRY(pitch,roll,yaw)					CVect3((pitch)*PI/180,(roll)*PI/180,(yaw)*PI/180)


//常量定义
extern const CVect3	I31, O31, Ipos;
extern const CQuat	qI;
extern const CMat3	I33, O33;
extern const CVect  On1, O1n;

//函数定义
CVect3	q2att(const CQuat &qnb);
CMat3	diag(const CVect3 &v);

//类定义
class CGLV
{
public:
    double Re, f, g0, wie;										// the Earth's parameters
    double e, e2, ep, ep2, Rp;
    double mg, ug, deg, min, sec, hur, ppm, ppmpsh;					// commonly used units
    double dps, dph, dpsh, dphpsh, ugpsh, ugpsHz, ugpg2, mpsh, mpspsh, secpsh;

    CGLV(double Re=6378137.0, double f=(1.0/298.257), double wie0=7.2921151467e-5, double g0=9.7803267714);
};

const CGLV glv;


#endif // BASEDCLASS_H
