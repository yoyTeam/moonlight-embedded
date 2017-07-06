#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

void texture_renderer_setup(void **eglTexture, int width, int height);
void texture_renderer_cleanup();
void *texture_renderer_submit_decode_unit();


static const GLbyte quadx[6*4*3] = {
   /* FRONT */
   -10, -10,  10,
   10, -10,  10,
   -10,  10,  10,
   10,  10,  10,

   /* BACK */
   -10, -10, -10,
   -10,  10, -10,
   10, -10, -10,
   10,  10, -10,

   /* LEFT */
   -10, -10,  10,
   -10,  10,  10,
   -10, -10, -10,
   -10,  10, -10,

   /* RIGHT */
   10, -10, -10,
   10,  10, -10,
   10, -10,  10,
   10,  10,  10,

   /* TOP */
   -10,  10,  10,
   10,  10,  10,
   -10,  10, -10,
   10,  10, -10,

   /* BOTTOM */
   -10, -10,  10,
   -10, -10, -10,
   10, -10,  10,
   10, -10, -10,
};

/** Texture coordinates for the quad. */
static const GLfloat texCoords[6 * 4 * 2] = {
   0.f,  0.f,
   0.f,  1.f,
   1.f,  0.f,
   1.f,  1.f,

   0.f,  0.f,
   0.f,  1.f,
   1.f,  0.f,
   1.f,  1.f,

   0.f,  0.f,
   0.f,  1.f,
   1.f,  0.f,
   1.f,  1.f,

   0.f,  0.f,
   0.f,  1.f,
   1.f,  0.f,
   1.f,  1.f,

   0.f,  0.f,
   0.f,  1.f,
   1.f,  0.f,
   1.f,  1.f,

   0.f,  0.f,
   0.f,  1.f,
   1.f,  0.f,
   1.f,  1.f
};