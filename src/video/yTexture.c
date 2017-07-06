#include <assert.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <VG/openvg.h>
#include <VG/vgu.h>

typedef struct
{
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
} EGL_STATE_T;


EGL_STATE_T state, *p_state = &state;

typedef struct
{
   // Handle to a program object
   GLuint programObject;

} UserData;

void init_egl(EGL_STATE_T *state)
{
    EGLint num_configs;
    EGLBoolean result;

    //bcm_host_init();

    static const EGLint attribute_list[] =
	{
	    EGL_RED_SIZE, 8,
	    EGL_GREEN_SIZE, 8,
	    EGL_BLUE_SIZE, 8,
	    EGL_ALPHA_SIZE, 8,
	    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	    EGL_SAMPLES, 1,
	    EGL_NONE
	};

    static const EGLint context_attributes[] =
	{
	    EGL_CONTEXT_CLIENT_VERSION, 2,
	    EGL_NONE
	};

    // get an EGL display connection
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // initialize the EGL display connection
    result = eglInitialize(state->display, NULL, NULL);

    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(state->display, attribute_list, &state->config, 1, &num_configs);
    assert(EGL_FALSE != result);

    // Choose the OpenVG API
    result = eglBindAPI(EGL_OPENVG_API);
    assert(EGL_FALSE != result);

    // create an EGL rendering context
    state->context = eglCreateContext(state->display, 
				      state->config,
				      NULL, // EGL_NO_CONTEXT, 
				      NULL);
				      // breaks if we use this: context_attributes);
    assert(state->context!=EGL_NO_CONTEXT);
}



void texture_renderer_setup(void **eglTexture, int width, int height) {
    printf("\n Texture egl init\n");

bcm_host_init();

    memset(state, 0, sizeof(*state));

    init_ogl(state);

    init_model_proj(state);

    init_textures(state);


   *eglTexture = eglImage;
}

void texture_renderer_cleanup() {
    printf("\n Texture egl cleanup\n");

    exit_func();
}

void *texture_renderer_submit_decode_unit() {
    printf("\n Texture egl decode unit\n");

    

    while (!terminate)
   {
      update_model(state);
      redraw_scene(state);
   }
   texture_renderer_cleanup();

    return (void *)1;
}