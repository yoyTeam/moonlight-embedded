find_path(RTIMULIB_INCLUDE_DIR
  NAMES RTIMULib.h
  DOC "RtimuLib include directory"
  PATHS /usr/local/include)
mark_as_advanced(RTIMULIB_INCLUDE_DIR)

find_library(RTIMULIB_LIBRARY
  NAMES libRTIMULib.so
  DOC "Path to RtimuLib Library"
  PATHS /usr/local/lib)
mark_as_advanced(RTIMULIB_LIBRARY)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RtimuLib DEFAULT_MSG RTIMULIB_INCLUDE_DIR RTIMULIB_LIBRARY)


set(RTIMULIB_LIBRARIES ${RTIMULIB_LIBRARY})
set(RTIMULIB_INCLUDE_DIRS ${RTIMULIB_INCLUDE_DIR})
