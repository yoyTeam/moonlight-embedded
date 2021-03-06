#include <stdlib.h>
#include <stdio.h>

#include "opengl.h"

ESContext esContext;
// Handle to a program object
GLuint programObject;

// Attribute locations
GLint  positionLoc;
GLint  texCoordLoc;

// Sampler location
GLint samplerLoc;
GLint offsetXLoc;
GLint offsetYLoc;

// Texture handle
GLuint DtextureId;

GLubyte *image;
int Dwidth, Dheight;

#define IMAGE_SIZE_WIDTH 1280
#define IMAGE_SIZE_HEIGHT 720
static int glInited = 0;

static void* eglImage = 0;
struct thread_args *mArgs = 0;
static pthread_t thread1;

int movShaderX = 0;
int movShaderY = 0;

GLuint CreateTexture(ESContext *esContext)
{

   // Texture object handle
    GLuint textureId;
   //UserData *userData = esContext->userData;

   Dwidth = esContext->width;
   Dheight = esContext->height;

   // Generate a texture object
   glGenTextures ( 1, &textureId );

   // Bind the texture object
   glBindTexture ( GL_TEXTURE_2D, textureId );

   // Load the texture
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
		  IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT,
		  0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

   // Set the filtering mode
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


   /* Create EGL Image */
   eglImage = eglCreateImageKHR(
                esContext->eglDisplay,
                esContext->eglContext,
                EGL_GL_TEXTURE_2D_KHR,
                textureId, // (EGLClientBuffer)esContext->texture,
                0);

   if (eglImage == EGL_NO_IMAGE_KHR)
   {
      printf("eglCreateImageKHR failed.\n");
      exit(1);
   }

   // Start rendering
//   pthread_create(&thread1, NULL, moonlight_streaming, mArgs);

   return textureId;
}


///
// Initialize the shader and program object
//
GLuint InitShaders ( ESContext *esContext, void** teglImage )
{

    //UserData *userData = esContext->userData;
    GLbyte vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

    GLbyte fShaderStr[] =
      "precision mediump float;                            \n"
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "uniform float offsetX;                        \n"
      "uniform float offsetY;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec2 finalCoord = v_texCoord + vec2(offsetX, offsetY); \n"
      "  vec4 inputColor = texture2D( s_texture, finalCoord ); \n"
      "  vec2 dist = (v_texCoord-0.5)*vec2(2.0,1.1);                   \n"
      "  float len = length(dist);                         \n"
      "  float radius = 1.0;                         \n"
      "  float vignette = smoothstep(radius, radius-1.0, len);   \n"
      "  gl_FragColor = inputColor;                             \n"
      "  if( len>0.5 ) gl_FragColor = vec4(0,0,0,1);                            \n"
      "}                                                   \n";

   // Load the shaders and get a linked program object
   programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the attribute locations
   positionLoc = glGetAttribLocation ( programObject, "a_position" );
   texCoordLoc = glGetAttribLocation ( programObject, "a_texCoord" );

   // Get the sampler location
   samplerLoc = glGetUniformLocation ( programObject, "s_texture" );
   offsetXLoc = glGetUniformLocation ( programObject, "offsetX" );
   offsetYLoc = glGetUniformLocation ( programObject, "offsetY" );

   // Load the texture
   DtextureId = CreateTexture (esContext);

   glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

   *teglImage = eglImage;

   return GL_TRUE;
}

void updateIMU ( int _currentX, int _currentY ){
  movShaderX = _currentX;
  movShaderY = _currentY;
}

///
// Draw a triangle using the shader pair created in Init()
//
void DrawGL ( ESContext *esContext )
{

   //UserData *userData = esContext->userData;

    GLfloat vVertices[] = { 1.0f,  1.0f, 0.0f,  // Position 0
                             0.25f,  0.0f,        // TexCoord 0
                            -1.0f, 1.0f, 0.0f,  // Position 1
                             0.25f,  1.0f,        // TexCoord 1
                             -1.0f, 0.0f, 0.0f,  // Position 2
                             0.75f,  1.0f,        // TexCoord 2
                             1.0f,  0.0f, 0.0f,  // Position 3
                             0.75f,  0.0f,         // TexCoord 3

                             1.0f,  0.0f, 0.0f,  // Position 4
                             0.25f,  0.0f,        // TexCoord 4
                             -1.0f, 0.0f, 0.0f,  // Position 5
                             0.25f,  1.0f,        // TexCoord 5
                             -1.0f, -1.0f, 0.0f,  // Position 6
                             0.75f,  1.0f,        // TexCoord 6
                             1.0f,  -1.0f, 0.0f,  // Position 7
                             0.75f,  0.0f         // TexCoord 7

                          };

    GLushort indices[] = { 0, 1, 2, 0, 2, 3,    4, 5, 6, 4, 6, 7 };


    // Set the viewport
    glViewport ( 0, 0, 1080, 1920); //esContext->width, esContext->height );


   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( programObject );

   // Load the vertex position
   glVertexAttribPointer ( positionLoc, 3, GL_FLOAT,
                           GL_FALSE, 5 * sizeof(GLfloat), vVertices );
   // Load the texture coordinate
   glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,
                           GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );

   glEnableVertexAttribArray ( positionLoc );
   glEnableVertexAttribArray ( texCoordLoc );

   // Bind the texture
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, DtextureId );

   // Set the sampler texture unit to 0
   glUniform1i ( samplerLoc, 0 );

   float scaleFactor = 0.0003;

   glUniform1f ( offsetXLoc, (float)movShaderX*scaleFactor );
   glUniform1f ( offsetYLoc, (float)movShaderY*scaleFactor );

   glDrawElements ( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, indices );

}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
   // Delete texture object
   glDeleteTextures ( 1, &DtextureId );

   // Delete program object
   glDeleteProgram ( programObject );
}
