#ifndef OPENGL_H
#define OPENGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esUtil.h"
#include "EGL/eglext.h"

GLuint CreateTexture(ESContext *esContext);
GLuint InitShaders ( ESContext *esContext, void** teglImage );
void DrawGL ( ESContext *esContext );
void ShutDown ( ESContext *esContext );
void updateIMU ( int _currentX, int _currentY );


#ifdef __cplusplus
}
#endif

#endif
