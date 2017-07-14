#ifndef OPENGL_H
#define OPENGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esUtil.h"
#include "EGL/eglext.h"

GLuint CreateTexture(ESContext *esContext);
int InitShaders ( ESContext *esContext );
void DrawGL ( ESContext *esContext );
void ShutDown ( ESContext *esContext );


#ifdef __cplusplus
}
#endif

#endif