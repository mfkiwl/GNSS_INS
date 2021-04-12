#ifndef CGLV_H
#define CGLV_H

/***************************  class CGLV  *********************************/
/*************************** 全局变量  ************************************/
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
#endif // CGLV_H
