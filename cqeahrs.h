#ifndef CQEAHRS_H
#define CQEAHRS_H
#include <ckalman.h>
#include <cmat3.h>

class CQEAHRS:public CKalman
{
public:
    CMat3 Cnb;

    CQEAHRS(double ts);
    void Update(const CVect3 &gyro, const CVect3 &acc, const CVect3 &mag, double ts);
};

#endif // CQEAHRS_H
