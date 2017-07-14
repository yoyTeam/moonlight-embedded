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
   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


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

  mArgs->eglImage = eglImage;

   // Start rendering
   pthread_create(&thread1, NULL, moonlight_streaming, mArgs);

   return textureId;
}


///
// Initialize the shader and program object
//
int InitShaders ( ESContext *esContext )
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
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord ); \n"
      "}                                                   \n";

   // Load the shaders and get a linked program object
   programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the attribute locations
   positionLoc = glGetAttribLocation ( programObject, "a_position" );
   texCoordLoc = glGetAttribLocation ( programObject, "a_texCoord" );

   // Get the sampler location
   samplerLoc = glGetUniformLocation ( programObject, "s_texture" );

   // Load the texture
   DtextureId = CreateTexture (esContext);

   glClearColor ( 0.5f, 0.5f, 0.5f, 1.0f );

   return GL_TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void DrawGL ( ESContext *esContext )
{

   //UserData *userData = esContext->userData;
   GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0
                           -1.0f, -1.0f, 0.0f,  // Position 1
                            0.0f,  1.0f,        // TexCoord 1
                            1.0f, -1.0f, 0.0f,  // Position 2
                            1.0f,  1.0f,        // TexCoord 2
                            1.0f,  1.0f, 0.0f,  // Position 3
                            1.0f,  0.0f         // TexCoord 3
                         };
   GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

   // Set the viewport
   glViewport ( 0, 0, 1280, 720); //esContext->width, esContext->height );

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

   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

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
