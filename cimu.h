#ifndef CIMU_H
#define CIMU_H
#include <cvector3.h>

class CIMU
{
public:
    int nSamples;
    bool preFirst, onePlusPre;
    CVect3 phim, dvbm, wm_1, vm_1;

    CIMU(void);
    void Update(const CVect3 *pwm, const CVect3 *pvm, int nSamples);

    friend void IMURFU(CVect3 *pwm, int nSamples, const char *str);
    friend void IMURFU(CVect3 *pwm, CVect3 *pvm, int nSamples, const char *str);
};

#endif // CIMU_H
