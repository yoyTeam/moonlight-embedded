#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <inttypes.h>
#include "../mapping.h"


typedef void* rtimu_t;
typedef void* rtimu_settings_t;

EXTERNC rtimu_t rtimu_new();
EXTERNC void rtimu_destroy(rtimu_t rtimu);
//EXTERNC RTIMU_DATA rtimu_get_IMUData(rtimu_t self);
EXTERNC static rtimu_t rtimu_createIMU(rtimu_settings_t settings);
EXTERNC void rtimu_init(rtimu_t rtimu);

EXTERNC float rtimu_get_fusionPose_x(rtimu_t rtimu);
EXTERNC float rtimu_get_fusionPose_y(rtimu_t rtimu);
EXTERNC float rtimu_get_fusionPose_z(rtimu_t rtimu);

EXTERNC bool rtimu_type_is_null(rtimu_t rtimu);

EXTERNC void rtimu_set_slerp_power(float power);
EXTERNC void rtimu_set_gyro_enable(bool enable);
EXTERNC void rtimu_set_accel_enable(bool enable);
EXTERNC void rtimu_set_compass_enable(bool enable);

EXTERNC bool rtimu_read(rtimu_t rtimu);

/*
* RTIMUSettings
*/

EXTERNC rtimu_settings_t rtimu_settings_new(const char *productType);

#undef EXTERNC
