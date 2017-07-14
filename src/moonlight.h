#ifndef MOONLIGHT_H
#define MOONLIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

void *moonlight_streaming(void* arg);

struct thread_args {    
    int argc;
    char **argv;
    void* eglImage;
};

#ifdef __cplusplus
}
#endif

#endif