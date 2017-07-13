#include "rtimu.h"

#include "RTIMULib.h"

rtimu_t rtimu_init() {
    return new RTIMU;
}

void rtimu_destroy(rtimu_t rtimu) {
    rtimu_t* typed_ptr = static_cast<rtimut_t*>(rtimu);
    delete typed_ptr;
}


RTIMU_DATA rtimu_get_IMUData(rtimu_t self) {
    rtimu_t* typed_self = static_cast<rtimu_t*>(self);
    return typed_self->getIMUData();
}