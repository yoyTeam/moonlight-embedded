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

#include <ilclient.h>
#include <bcm_host.h>

#ifdef YOPENGL
#include "esUtil.h"
#include "EGL/eglext.h"
#endif

#define MAX_DECODE_UNIT_SIZE 262144

static TUNNEL_T tunnel[5];
static COMPONENT_T *list[4];
static ILCLIENT_T *client;

static COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *egl_render = NULL;

static OMX_BUFFERHEADERTYPE *eglBuffer = NULL;
static unsigned char *dest;

static int port_settings_changed;
static int first_packet;

static void* eglImage = 0;
static OMX_BUFFERHEADERTYPE *buf;


void fill_buffer_done(void* data, COMPONENT_T* comp)
{
    if (OMX_FillThisBuffer(ilclient_get_handle(egl_render), eglBuffer) != OMX_ErrorNone)
	{
	    printf("OMX_FillThisBuffer failed in callback\n");
	    return;
	}
}


static int decoder_renderer_setup(int videoFormat, int width, int height, int redrawRate, void* context, int drFlags, void* teglImage) {
  if (videoFormat != VIDEO_FORMAT_H264) {
    fprintf(stderr, "Video format not supported\n");
    return -1;
  }

  bcm_host_init();
  gs_sps_init(width, height);

  if(teglImage == 0) {
      fprintf(stderr, "eglImage format not supported\n");
      return -1;
  }

  eglImage = teglImage;

  OMX_VIDEO_PARAM_PORTFORMATTYPE format;
  OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
  COMPONENT_T *clock = NULL;

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

  ilclient_set_fill_buffer_done_callback(client, fill_buffer_done, 0);

  // create video_decode
  if(ilclient_create_component(client, &video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0){
    fprintf(stderr, "Can't create video decode\n");
    return -2;
  }

  list[0] = video_decode;

  // create egl_render
  if(ilclient_create_component(client, &egl_render, "egl_render", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0){
    fprintf(stderr, "Can't create video renderer\n");
    return -2;
  }

  list[1] = egl_render;

  set_tunnel(tunnel, video_decode, 131, egl_render, 220);

  ilclient_change_component_state(video_decode, OMX_StateIdle);

  memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
  format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
  format.nVersion.nVersion = OMX_VERSION;
  format.nPortIndex = 130;
  format.eCompressionFormat = OMX_VIDEO_CodingAVC;

  OMX_PARAM_DATAUNITTYPE unit;

  memset(&unit, 0, sizeof(OMX_PARAM_DATAUNITTYPE));
  unit.nSize = sizeof(OMX_PARAM_DATAUNITTYPE);
  unit.nVersion.nVersion = OMX_VERSION;
  unit.nPortIndex = 130;
  unit.eUnitType = OMX_DataUnitCodedPicture;
  unit.eEncapsulationType = OMX_DataEncapsulationElementaryStream;

  if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone ||
     OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamBrcmDataUnit, &unit) != OMX_ErrorNone) {
    fprintf(stderr, "Failed to set video parameters\n");
    return -2;
  }

  OMX_PARAM_PORTDEFINITIONTYPE port;

  memset(&port, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
  port.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
  port.nVersion.nVersion = OMX_VERSION;
  port.nPortIndex = 130;
  if(OMX_GetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &port) != OMX_ErrorNone) {
    fprintf(stderr, "Failed to get decoder port definition\n");
    return -2;
  }

  // Increase the buffer size to fit the largest possible frame
  port.nBufferSize = MAX_DECODE_UNIT_SIZE;

  if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &port) == OMX_ErrorNone &&
     ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0) {

    port_settings_changed = 0;
    first_packet = 1;

    ilclient_change_component_state(video_decode, OMX_StateExecuting);
  } else {
    fprintf(stderr, "Can't setup video\n");
    return -2;
  }

  return 0;
}

static void decoder_renderer_cleanup() {
  int status = 0;

  if((buf = ilclient_get_input_buffer(video_decode, 130, 1)) == NULL){
    fprintf(stderr, "Can't get video buffer\n");
    return;
  }

  buf->nFilledLen = 0;
  buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

  if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(list[0]), buf) != OMX_ErrorNone){
    fprintf(stderr, "Can't empty video buffer\n");
    return;
  }

  // need to flush the renderer to allow video_decode to disable its input port
  ilclient_flush_tunnels(tunnel, 0);

  ilclient_disable_port_buffers(list[0], 130, NULL, NULL, NULL);

  ilclient_disable_tunnel(tunnel);
  ilclient_teardown_tunnels(tunnel);

  ilclient_state_transition(list, OMX_StateIdle);
  ilclient_state_transition(list, OMX_StateLoaded);

  ilclient_cleanup_components(list);

  OMX_Deinit();

  ilclient_destroy(client);
}

static int decoder_renderer_submit_decode_unit(PDECODE_UNIT decodeUnit) {
  if((buf = ilclient_get_input_buffer(video_decode, 130, 1)) == NULL){
    fprintf(stderr, "Can't get video buffer\n");
    return -2;
  }

  // feed data and wait until we get port settings changed
  dest = buf->pBuffer;

  buf->nFilledLen = 0;

  buf->nOffset = 0;

  buf->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS;

  if(first_packet) {
    buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
    first_packet = 0;
  }

  PLENTRY entry = gs_sps_fix(&decodeUnit->bufferList, GS_SPS_BITSTREAM_FIXUP);
  decodeUnit->bufferList = entry;
  while (entry != NULL) {
    memcpy(dest, entry->data, entry->length);
    buf->nFilledLen += entry->length;
    dest += entry->length;
    entry = entry->next;
  }


  if(port_settings_changed == 0 &&
    ((buf->nFilledLen > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
    (buf->nFilledLen == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                        ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0))) {
    port_settings_changed = 1;

    if(ilclient_setup_tunnel(tunnel, 0, 0) != 0){
      fprintf(stderr, "Can't setup video\n");
      return -2;
    }

    // Set egl_render to idle
    ilclient_change_component_state(egl_render, OMX_StateIdle);

    // Enable the output port and tell egl_render to use the texture as a buffer
    //ilclient_enable_port(egl_render, 221); //THIS BLOCKS SO CANT BE USED
    if (OMX_SendCommand(ILC_GET_HANDLE(egl_render), OMX_CommandPortEnable, 221, NULL) != OMX_ErrorNone)
   	{
   	    printf("OMX_CommandPortEnable failed.\n");
   	    return -2;
   	}

    if (OMX_UseEGLImage(ILC_GET_HANDLE(egl_render), &eglBuffer, 221, NULL, eglImage) != OMX_ErrorNone)
   	{
   	    printf("OMX_UseEGLImage failed.\n");
   	    return -2;
   	}

    // Set egl_render to executing
    ilclient_change_component_state(egl_render, OMX_StateExecuting);

    // Request egl_render to write data to the texture buffer
    if(OMX_FillThisBuffer(ILC_GET_HANDLE(egl_render), eglBuffer) != OMX_ErrorNone)
   	{
   	    printf("OMX_FillThisBuffer failed.\n");
   	    return -2;
   	}

  }

  if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone){
    fprintf(stderr, "Can't empty video buffer\n");
    return -2;
  }

  return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_pi = {
  .setup = decoder_renderer_setup,
  .cleanup = decoder_renderer_cleanup,
  .submitDecodeUnit = decoder_renderer_submit_decode_unit,
  .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
