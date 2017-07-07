#include <Limelight.h>

#include <sps.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ilclient.h>
#include <bcm_host.h>

#define MAX_DECODE_UNIT_SIZE 262144

static TUNNEL_T tunnel[3]; // last entry should be null
static COMPONENT_T *list[2]; // last entry should be null
static ILCLIENT_T *client;

static COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *egl_render = NULL;

static OMX_BUFFERHEADERTYPE *eglBuffer;
static unsigned char *dest;

static int port_settings_changed;
static int first_packet;

static void* eglImage = 0;

void my_fill_buffer_done(void* data, COMPONENT_T* comp)
{
    if (OMX_FillThisBuffer(ilclient_get_handle(egl_render), eglBuffer) != OMX_ErrorNone)
	{
	    printf("OMX_FillThisBuffer failed in callback\n");
	    exit(1);
	}
}

static int decoder_renderer_setup(int videoFormat, int width, int height, int redrawRate, void* context, int drFlags, void* teglImage) {
    if (videoFormat != VIDEO_FORMAT_H264) {
        fprintf(stderr, "Video format not supported\n");
        return -1;
    }

    eglImage = teglImage;

    //bcm_host_init();

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

    // callback
   ilclient_set_fill_buffer_done_callback(client, my_fill_buffer_done, 0);

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


/*
    OMX_PARAM_DATAUNITTYPE unit;

    memset(&unit, 0, sizeof(OMX_PARAM_DATAUNITTYPE));
    unit.nSize = sizeof(OMX_PARAM_DATAUNITTYPE);
    unit.nVersion.nVersion = OMX_VERSION;
    unit.nPortIndex = 130;
    unit.eUnitType = OMX_DataUnitCodedPicture;
    unit.eEncapsulationType = OMX_DataEncapsulationElementaryStream;

    if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone ||
       OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamBrcmDataUnit, &unit) != OMX_ErrorNone) {
        fprintf(stderr, "Failed to set video parameters 1\n");
        return -2;
    }*/
/*
    OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
    memset(&latencyTarget, 0, sizeof(OMX_CONFIG_LATENCYTARGETTYPE));
    latencyTarget.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    latencyTarget.nVersion.nVersion = OMX_VERSION;
    latencyTarget.nPortIndex = 220;
    latencyTarget.bEnabled = OMX_TRUE;
    latencyTarget.nFilter = 2;
    latencyTarget.nTarget = 4000;
    latencyTarget.nShift = 3;
    latencyTarget.nSpeedFactor = -135;
    latencyTarget.nInterFactor = 500;
    latencyTarget.nAdjCap = 20;

    OMX_CONFIG_DISPLAYREGIONTYPE displayRegion;
    displayRegion.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    displayRegion.nVersion.nVersion = OMX_VERSION;
    displayRegion.nPortIndex = 220;
    displayRegion.fullscreen = OMX_TRUE;
    displayRegion.mode = OMX_DISPLAY_SET_FULLSCREEN;

    if(OMX_SetParameter(ILC_GET_HANDLE(egl_render), OMX_IndexConfigLatencyTarget, &latencyTarget) != OMX_ErrorNone) {
        fprintf(stderr, "Failed to set video render parameters 2\n");
        exit(EXIT_FAILURE);
    }
    */

/*
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
       ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0) {*/
    

    if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
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

    if((eglBuffer = ilclient_get_input_buffer(video_decode, 130, 1)) == NULL){
        fprintf(stderr, "Can't get video buffer\n");
        exit(EXIT_FAILURE);
    }

    eglBuffer->nFilledLen = 0;
    eglBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

    /*if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(list[0]), eglBuffer) != OMX_ErrorNone){
        fprintf(stderr, "Can't empty video buffer\n");
        return;
    }*/

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
    if((eglBuffer = ilclient_get_input_buffer(video_decode, 130, 1)) == NULL){
        fprintf(stderr, "Can't get video buffer\n");
        exit(EXIT_FAILURE);
    }
    
    // feed data and wait until we get port settings changed
    dest = eglBuffer->pBuffer;

    eglBuffer->nFilledLen = 0;

    eglBuffer->nOffset = 0;

    eglBuffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS;

    if(first_packet) {
        eglBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME;
        first_packet = 0;
    }

    PLENTRY entry = gs_sps_fix(&decodeUnit->bufferList, GS_SPS_BITSTREAM_FIXUP);
    decodeUnit->bufferList = entry;
    while (entry != NULL) {
        memcpy(dest, entry->data, entry->length);
        eglBuffer->nFilledLen += entry->length;
        dest += entry->length;
        entry = entry->next;
    }


    if(port_settings_changed == 0 &&
       ((eglBuffer->nFilledLen > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
        (eglBuffer->nFilledLen == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                       ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0))) {

        printf("\n fuck you! bitch!");
        port_settings_changed = 1;

        if(ilclient_setup_tunnel(tunnel, 0, 0) != 0){
            fprintf(stderr, "Can't setup video\n");
            exit(EXIT_FAILURE);
        }

        // Set egl_render to idle
        ilclient_change_component_state(egl_render, OMX_StateIdle);

        // Enable the output port and tell egl_render to use the texture as a buffer
        //ilclient_enable_port(egl_render, 221); THIS BLOCKS SO CANT BE USED
        if (OMX_SendCommand(ILC_GET_HANDLE(egl_render), OMX_CommandPortEnable, 221, NULL) != OMX_ErrorNone)
        {
            printf("OMX_CommandPortEnable failed.\n");
            exit(1);
        }

        if (OMX_UseEGLImage(ILC_GET_HANDLE(egl_render), &eglBuffer, 221, NULL, eglImage) != OMX_ErrorNone)
        {
            printf("OMX_UseEGLImage failed.\n");
            exit(1);
        }

        // Set egl_render to executing
        ilclient_change_component_state(egl_render, OMX_StateExecuting);


        // Request egl_render to write data to the texture buffer
        if(OMX_FillThisBuffer(ILC_GET_HANDLE(egl_render), eglBuffer) != OMX_ErrorNone)
        {
            printf("OMX_FillThisBuffer failed.\n");
            exit(1);
        }
    }
  

    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_pi = {
        .setup = decoder_renderer_setup,
        .cleanup = decoder_renderer_cleanup,
        .submitDecodeUnit = decoder_renderer_submit_decode_unit,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};