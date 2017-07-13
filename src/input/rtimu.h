#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

typedef void* rtimu_t;

EXTERNC rtimu_t rtimu_init();
EXTERNC void rtimu_destroy(rtimu_t rtimu);
EXTERNC RTIMU_DATA rtimu_get_IMUData(rtimu_t self);

#undef EXTERNC
