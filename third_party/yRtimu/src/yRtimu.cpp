#include "rtimu.h"

rtimu_t rtimu_new() {
    return new RTIMU;
}

void rtimu_destroy(rtimu_t rtimu) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    delete imu;
}

EXTERNC static rtimu_t rtimu_createIMU(rtimu_settings_t settings) {
    RTIMUSettings *msettings = static_cast<RTIMUSettings*>(settings);

    return RTIMU::createIMU(msettings);
}

EXTERNC void rtimu_init(rtimu_t rtimu) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    return imu->IMUInit();
}


/*RTIMU_DATA rtimu_get_IMUData(rtimu_t self) {
    rtimu_t* typed_self = static_cast<rtimu_t*>(self);
    return typed_self->getIMUData();
}*/


EXTERNC float rtimu_get_fusionPose_x(rtimu_t rtimu) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    return imu->getIMUData().fusionPose.x();
}

EXTERNC float rtimu_get_fusionPose_y(rtimu_t rtimu) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    return imu->getIMUData().fusionPose.y();
}

EXTERNC float rtimu_get_fusionPose_z(rtimu_t rtimu) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    return imu->getIMUData().fusionPose.z();
}

EXTERNC bool rtimu_type_is_null(rtimu_t rtimu) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    return (imu->IMUType() == RTIMU_TYPE_NULL);
}

EXTERNC void rtimu_set_slerp_power(float power) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    imu->setSlerpPower(power);  
}

EXTERNC void rtimu_set_gyro_enable(bool enable) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    imu->setGyroEnable(enable);  
}

EXTERNC void rtimu_set_accel_enable(bool enable) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    imu->setAccelEnable(enable);  
}

EXTERNC void rtimu_set_compass_enable(bool enable) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    imu->setCompassEnable(enable);
}

EXTERNC bool rtimu_read(rtimu_t rtimu) {
    RTIMU* imu = static_cast<RTIMU*>(rtimu);
    return imu->IMURead();
}

/* 
* RTIMUSettings
*/
EXTERNC rtimu_settings_t rtimu_settings_new(const char *productType) {
    return new RTIMUSettings("RTIMULib");
}