void *moonlight_streaming(void* arg);

struct thread_args {    
    int argc;
    char **argv;
    void* eglImage;
};