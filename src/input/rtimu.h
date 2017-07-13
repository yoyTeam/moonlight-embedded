#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <inttypes.h>
#include "mapping.h"


typedef void* rtimu_t;

EXTERNC rtimu_t rtimu_init();
EXTERNC void rtimu_destroy(rtimu_t rtimu);
//EXTERNC RTIMU_DATA rtimu_get_IMUData(rtimu_t self);

EXTERNC float rtimu_get_fusionPose_x(rtimu_t rtimu);
EXTERNC float rtimu_get_fusionPose_y(rtimu_t rtimu);
EXTERNC float rtimu_get_fusionPose_z(rtimu_t rtimu);

#undef EXTERNC
