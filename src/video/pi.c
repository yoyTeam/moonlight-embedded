/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the copyright holder nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Video decode on Raspberry Pi using OpenMAX IL though the ilcient helper library
// Based upon video decode example from the Raspberry Pi firmware

#include <Limelight.h>

#include <sps.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include <ilclient.h>
#include <bcm_host.h>

#define MAX_DECODE_UNIT_SIZE 262144

static TUNNEL_T tunnel[4];
static COMPONENT_T *list[5];
static ILCLIENT_T *client;

 pthread_mutex_t   m_lock;

static COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL;

static OMX_BUFFERHEADERTYPE *buf;

static int port_settings_changed;
static int first_packet;

static void* eglImage = 0;


void printState(OMX_HANDLETYPE handle) {
    OMX_STATETYPE state;
    OMX_ERRORTYPE err;

    err = OMX_GetState(handle, &state);
    if (err != OMX_ErrorNone) {
        fprintf(stderr, "Error on getting state\n");
        exit(1);
    }
    switch (state) {
    case OMX_StateLoaded:           printf("StateLoaded\n"); break;
    case OMX_StateIdle:             printf("StateIdle\n"); break;
    case OMX_StateExecuting:        printf("StateExecuting\n"); break;
    case OMX_StatePause:            printf("StatePause\n"); break;
    case OMX_StateWaitForResources: printf("StateWait\n"); break;
    case OMX_StateInvalid:          printf("StateInvalid\n"); break;
    default:                        printf("State unknown\n"); break;
    }
}

char *err2str(int err) {
    switch (err) {
    case OMX_ErrorInsufficientResources: return "OMX_ErrorInsufficientResources";
    case OMX_ErrorUndefined: return "OMX_ErrorUndefined";
    case OMX_ErrorInvalidComponentName: return "OMX_ErrorInvalidComponentName";
    case OMX_ErrorComponentNotFound: return "OMX_ErrorComponentNotFound";
    case OMX_ErrorInvalidComponent: return "OMX_ErrorInvalidComponent";
    case OMX_ErrorBadParameter: return "OMX_ErrorBadParameter";
    case OMX_ErrorNotImplemented: return "OMX_ErrorNotImplemented";
    case OMX_ErrorUnderflow: return "OMX_ErrorUnderflow";
    case OMX_ErrorOverflow: return "OMX_ErrorOverflow";
    case OMX_ErrorHardware: return "OMX_ErrorHardware";
    case OMX_ErrorInvalidState: return "OMX_ErrorInvalidState";
    case OMX_ErrorStreamCorrupt: return "OMX_ErrorStreamCorrupt";
    case OMX_ErrorPortsNotCompatible: return "OMX_ErrorPortsNotCompatible";
    case OMX_ErrorResourcesLost: return "OMX_ErrorResourcesLost";
    case OMX_ErrorNoMore: return "OMX_ErrorNoMore";
    case OMX_ErrorVersionMismatch: return "OMX_ErrorVersionMismatch";
    case OMX_ErrorNotReady: return "OMX_ErrorNotReady";
    case OMX_ErrorTimeout: return "OMX_ErrorTimeout";
    case OMX_ErrorSameState: return "OMX_ErrorSameState";
    case OMX_ErrorResourcesPreempted: return "OMX_ErrorResourcesPreempted";
    case OMX_ErrorPortUnresponsiveDuringAllocation: return "OMX_ErrorPortUnresponsiveDuringAllocation";
    case OMX_ErrorPortUnresponsiveDuringDeallocation: return "OMX_ErrorPortUnresponsiveDuringDeallocation";
    case OMX_ErrorPortUnresponsiveDuringStop: return "OMX_ErrorPortUnresponsiveDuringStop";
    case OMX_ErrorIncorrectStateTransition: return "OMX_ErrorIncorrectStateTransition";
    case OMX_ErrorIncorrectStateOperation: return "OMX_ErrorIncorrectStateOperation";
    case OMX_ErrorUnsupportedSetting: return "OMX_ErrorUnsupportedSetting";
    case OMX_ErrorUnsupportedIndex: return "OMX_ErrorUnsupportedIndex";
    case OMX_ErrorBadPortIndex: return "OMX_ErrorBadPortIndex";
    case OMX_ErrorPortUnpopulated: return "OMX_ErrorPortUnpopulated";
    case OMX_ErrorComponentSuspended: return "OMX_ErrorComponentSuspended";
    case OMX_ErrorDynamicResourcesUnavailable: return "OMX_ErrorDynamicResourcesUnavailable";
    case OMX_ErrorMbErrorsInFrame: return "OMX_ErrorMbErrorsInFrame";
    case OMX_ErrorFormatNotDetected: return "OMX_ErrorFormatNotDetected";
    case OMX_ErrorContentPipeOpenFailed: return "OMX_ErrorContentPipeOpenFailed";
    case OMX_ErrorContentPipeCreationFailed: return "OMX_ErrorContentPipeCreationFailed";
    case OMX_ErrorSeperateTablesUsed: return "OMX_ErrorSeperateTablesUsed";
    case OMX_ErrorTunnelingUnsupported: return "OMX_ErrorTunnelingUnsupported";
    default: return "unknown error";
    }
}

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data) {
    fprintf(stderr, "OMX error %s\n", err2str(data));
}

void eos_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data) {
    fprintf(stderr, "Got eos event\n");
}

void my_fill_buffer_done(void* data, COMPONENT_T* comp)
{
if (OMX_FillThisBuffer(ilclient_get_handle(video_render), buf) != OMX_ErrorNone)
{
printf("OMX_FillThisBuffer failed in callback\n");
exit(1);
}
}

static int decoder_renderer_setup(int videoFormat, int width, int height, int redrawRate, void* context, int drFlags, void* tEglImage) {

    COMPONENT_T *clock = NULL;
    if (videoFormat != VIDEO_FORMAT_H264) {
        fprintf(stderr, "Video format not supported\n");
        return -1;
    }

    printf("\n width: %d \t height: %d\n", width, height);
    
    eglImage = tEglImage;

    if (eglImage == 0)
    {
        printf("eglImage is null.\n");
        return -2;
    }

    bcm_host_init();
    gs_sps_init(width, height);

    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;

    memset(list, 0, sizeof(list));
    memset(tunnel, 0, sizeof(tunnel));

    if((client = ilclient_init()) == NULL) {
        fprintf(stderr, "Can't initialize video\n");
        return -2;
    }

    if(OMX_Init() != OMX_ErrorNone) {
        fprintf(stderr, "Can't initialize OMX\n");
        return -2;
    }

    ilclient_set_error_callback(client,
			       error_callback,
			       NULL);

    ilclient_set_eos_callback(client,
			      eos_callback,
			      NULL);

                  // callback
    ilclient_set_fill_buffer_done_callback(client, my_fill_buffer_done, 0);


     // create video_decode
  if(ilclient_create_component(client, &video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0){
    fprintf(stderr, "Can't create video decode\n");
    return -2;
  }

  list[0] = video_decode;

  // create video_render
  if(ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0){
    fprintf(stderr, "Can't create video renderer\n");
    return -2;
  }

  list[1] = video_render;

  // create clock
   if(ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0) {
      fprintf(stderr, "Can't create clock\n");
    return -2;
   }
   list[2] = clock;

   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone) {
       fprintf(stderr, "Can't create clock paramter\n");
    return -2;
   }

   // create video_scheduler
   if(ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
       fprintf(stderr, "Can't create video scheduler\n");
    return -2;
   }
   list[3] = video_scheduler;

pthread_mutex_lock(&m_lock);
   set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
   set_tunnel(tunnel+1, video_scheduler, 11, video_render, 220);
   set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);

   // setup clock tunnel first
   if(ilclient_setup_tunnel(tunnel+2, 0, 0) != 0) {
      fprintf(stderr, "Can't create tunnel\n");
    return -2;
   } 

pthread_mutex_unlock(&m_lock);
      ilclient_change_component_state(clock, OMX_StateExecuting);

      ilclient_change_component_state(video_decode, OMX_StateIdle);

  
  memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   format.nVersion.nVersion = OMX_VERSION;
   format.nPortIndex = 130;
   format.eCompressionFormat = OMX_VIDEO_CodingAVC;

   if(
      OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
      ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0)
   {
      int port_settings_changed = 0;
      int first_packet = 1;

      ilclient_change_component_state(video_decode, OMX_StateExecuting);
   } else {
fprintf(stderr, "Can't setup video\n");
    return -2;
   }

pthread_mutex_init(&m_lock, NULL);

  return 0;
}

static void decoder_renderer_cleanup() {
    printf("\n aa \n");
   // need to flush the renderer to allow video_decode to disable its input port
    ilclient_flush_tunnels(tunnel, 1);

    ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
    
    ilclient_disable_tunnel(tunnel);
    ilclient_teardown_tunnels(tunnel);

    ilclient_state_transition(list, OMX_StateIdle);
    ilclient_state_transition(list, OMX_StateLoaded);

    ilclient_cleanup_components(list);

    OMX_Deinit();

    ilclient_destroy(client);
}

static int decoder_renderer_submit_decode_unit(PDECODE_UNIT decodeUnit) {
    if((buf = ilclient_get_input_buffer(video_decode, 130, 1)) == NULL)
    {
        fprintf(stderr, "Can't get video buffer\n");
    exit(EXIT_FAILURE);
    }
    // feed data and wait until we get port settings changed
    unsigned char *dest = buf->pBuffer;
    

  buf->nFilledLen = 0;

  buf->nOffset = 0;

  buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

  if(first_packet) {
    buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
    first_packet = 0;
  }
   pthread_mutex_lock(&m_lock);

   PLENTRY entry = gs_sps_fix(&decodeUnit->bufferList, GS_SPS_BITSTREAM_FIXUP);
  decodeUnit->bufferList = entry;
  while (entry != NULL) {
    memcpy(dest, entry->data, entry->length);
    buf->nFilledLen += entry->length;
    dest += entry->length;
    entry = entry->next;
  }
 pthread_mutex_unlock(&m_lock);
        printf("\npass 1");

pthread_mutex_lock(&m_lock);
        int a1 = ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1);
pthread_mutex_unlock(&m_lock);

printf("\npass 2");
pthread_mutex_lock(&m_lock);
int a2 = ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                       ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000);

                                                       pthread_mutex_unlock(&m_lock);
                                                       printf("\npass 3");

        if(port_settings_changed == 0 &&
            ((buf->nFilledLen > 0 && a1 == 0) ||
             (buf->nFilledLen == 0 && a2 == 0)))
        {

            printf("\npass 4");
            port_settings_changed = 1;

            pthread_mutex_lock(&m_lock);

            if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
            {
                fprintf(stderr, "Can't setup video 1\n");
                exit(EXIT_FAILURE);
            }

            ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

            // now setup tunnel to egl_render
            if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)
            {
                fprintf(stderr, "Can't setup video 2\n");
                exit(EXIT_FAILURE);
            }

            // Set egl_render to idle
            ilclient_change_component_state(video_render, OMX_StateIdle);

            // Enable the output port and tell egl_render to use the texture as a buffer
            //ilclient_enable_port(egl_render, 221); THIS BLOCKS SO CAN'T BE USED
            if (OMX_SendCommand(ILC_GET_HANDLE(video_render), OMX_CommandPortEnable, 221, NULL) != OMX_ErrorNone)
            {
               printf("OMX_CommandPortEnable failed.\n");
               exit(1);
            }

            if (OMX_UseEGLImage(ILC_GET_HANDLE(video_render), &buf, 221, NULL, eglImage) != OMX_ErrorNone)
            {
               printf("OMX_UseEGLImage failed.\n");
               exit(1);
            }

            // Set egl_render to executing
            ilclient_change_component_state(video_render, OMX_StateExecuting);


            // Request egl_render to write data to the texture buffer
            if(OMX_FillThisBuffer(ILC_GET_HANDLE(video_render), buf) != OMX_ErrorNone)
            {
               printf("OMX_FillThisBuffer failed.\n");
               exit(1);
            }

             pthread_mutex_unlock(&m_lock);
        }

        if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone) {
            printf("failed video decode failed.\n");
            exit(1);
        }   
        
                 



    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_pi = {
  .setup = decoder_renderer_setup,
  .cleanup = decoder_renderer_cleanup,
  .submitDecodeUnit = decoder_renderer_submit_decode_unit,
  .capabilities = CAPABILITY_DIRECT_SUBMIT,
};