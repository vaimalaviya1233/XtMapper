#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/input.h>
#include <fcntl.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <jni.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>


char str[16];

struct Socket {
    int client_fd;
    int server_fd;
    int port;
    struct sockaddr_in address;
    int addrlen;
};

typedef struct mouse_context {
    JavaVM  *javaVM;
    pthread_mutex_t  lock;
    int done;
    const char* dev;
    struct Socket s_ctx;
} mouseContext;
mouseContext g_ctx;

int fd;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    memset(&g_ctx, 0, sizeof(g_ctx));

    g_ctx.javaVM = vm;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // JNI version not supported.
    }

    g_ctx.done = 0;
    return  JNI_VERSION_1_6;
}


void create_socket(void* context){
    struct Socket *sock = context;
    int PORT = sock->port;
    int opt = 1;
    sock->addrlen = sizeof(sock->address);
    // Creating socket file descriptor
    if ((sock->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("mouse_read: Socket creation error \n");
    }

    // Forcefully attaching socket to the port
    if (setsockopt(sock->server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        printf("mouse_read: setsockopt error \n");
    }

    sock->address.sin_family = AF_INET;
    sock->address.sin_port = htons(PORT);
    sock->address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock->server_fd, (struct sockaddr*)&(sock->address),
             sizeof(sock->address)) < 0) {
        printf("mouse_read: socket bind error \n");
    }

    if (listen(sock->server_fd, 2) < 0) {
        printf("mouse_read: socket listen failed \n");
    }
}

void send_data(struct input_event *ie, int sock)
{
    switch (ie->code) {
        case REL_X :
            sprintf(str, "REL_X %d \n", ie->value);
            send(sock, str, strlen(str), 0);
            break;
        case REL_Y :
            sprintf(str, "REL_Y %d \n", ie->value);
            send(sock, str, strlen(str), 0);
            break;
        case REL_WHEEL :
            sprintf(str, "REL_WHEEL %d \n", ie->value);
            send(sock, str, strlen(str), 0);
            break;
        case BTN_MOUSE :
            sprintf(str, "BTN_MOUSE %d \n", ie->value);
            send(sock, str, strlen(str), 0);
            break;
    }
}

void* send_mouse_events(void* context) {
    mouseContext *pctx = (mouseContext*) context;
    struct Socket *sock = &pctx->s_ctx;
    struct input_event ie;
    if ((fd = open(pctx->dev, O_RDONLY)) == -1) {
        perror("opening device");
        exit(EXIT_FAILURE);
    }
    while (read(fd, &ie, sizeof(struct input_event))) {
        send_data(&ie, sock->client_fd);
    }
    close(fd);
    return context;
}

void* send_getevent(void *context) {
    mouseContext *pctx = (mouseContext*) context;
    struct Socket *sock = &pctx->s_ctx;
    pid_t cpid = fork();
    if (cpid < 0) exit(1);  /* exit if fork() fails */
    if ( cpid ) {
        /* In the parent process: */
        close(sock->client_fd ); /* new_socket is not needed in the parent after the fork */
        waitpid( cpid, NULL, 0 ); /* wait for and reap child process */
    } else {
        /* In the child process: */
        dup2( sock->client_fd, STDOUT_FILENO );  /* duplicate socket on stdout */
        dup2( sock->client_fd, STDERR_FILENO );  /* duplicate socket on stderr too */
        close( sock->client_fd );  /* can close the original after it's duplicated */
        char* cmd = "getevent";
        char* cmd_args[] = {"getevent", "-ql", NULL};
        execvp( cmd, cmd_args );   /* execvp() the command */
    }
    return context;
}

void* init(void* context) {
    mouseContext *pctx = (mouseContext*) context;
    struct Socket *sock = &pctx->s_ctx;
    JavaVM *javaVM = pctx->javaVM;
    JNIEnv *env;
    jint res = (*javaVM)->GetEnv(javaVM, (void**)&env, JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
        if (JNI_OK != res) {
            return NULL;
        }
    }
    create_socket(&sock);
    printf("Waiting for overlay...\n");
    while(true) {
        pthread_mutex_lock(&pctx->lock);
        int done = pctx->done;
        if (pctx->done) {
            pctx->done = 0;
        }
        pthread_mutex_unlock(&pctx->lock);
        if (done) {
            break;
        }

        struct sockaddr_in address = sock->address;
        int addrlen = sock->addrlen;
        if ((sock->client_fd
                     = accept(sock->server_fd, (struct sockaddr*)&address,
                              (socklen_t*)&addrlen)) < 0) {
            printf("mouse_read: connection failed \n");
        }
        char buffer[12] = { 0 };

        read(sock->client_fd, buffer, 12);

        if (strcmp(buffer, "mouse_read\n") == 0) {
            pthread_t threadInfo_;
            pthread_attr_t threadAttr_;

            pthread_attr_init(&threadAttr_);
            pthread_attr_setdetachstate(&threadAttr_, PTHREAD_CREATE_DETACHED);
            pthread_create(&threadInfo_, &threadAttr_, send_mouse_events, &pctx);
            pthread_attr_destroy(&threadAttr_);
        }
        else if (strcmp(buffer, "getevent\n") == 0) {
            pthread_t threadInfo_;
            pthread_attr_t threadAttr_;

            pthread_attr_init(&threadAttr_);
            pthread_attr_setdetachstate(&threadAttr_, PTHREAD_CREATE_DETACHED);
            pthread_create(&threadInfo_, &threadAttr_, send_getevent, &pctx);
            pthread_attr_destroy(&threadAttr_);
        }

        close(sock->client_fd); // closing the connected socket
        close(sock->server_fd); // closing the listening socket
        return 0;
    }

    (*javaVM)->DetachCurrentThread(javaVM);
    return context;
    }

JNIEXPORT void JNICALL
    Java_com_xtr_keymapper_Input_startMouse(JNIEnv *env, jobject instance, jstring device, jint port) {
        setlinebuf(stdout);
        g_ctx.dev = (*env)->GetStringUTFChars(env, device, 0);
        g_ctx.s_ctx.port = port;
        pthread_t threadInfo_;
        pthread_attr_t threadAttr_;

        pthread_attr_init(&threadAttr_);
        pthread_attr_setdetachstate(&threadAttr_, PTHREAD_CREATE_DETACHED);

        pthread_mutex_init(&g_ctx.lock, NULL);

        jclass clz = (*env)->GetObjectClass(env, instance);
        (*env)->NewGlobalRef(env, clz);
        (*env)->NewGlobalRef(env, instance);
        int result = pthread_create(&threadInfo_, &threadAttr_, init, &g_ctx);
        assert(result == 0);
        pthread_attr_destroy(&threadAttr_);
        (void) result;
    }


JNIEXPORT void JNICALL
Java_com_xtr_keymapper_Input_setIoctl(JNIEnv *env, jclass clazz, jboolean y) {
    if ( fd != 0 ) {
        ioctl(fd, EVIOCGRAB, y);
        printf("ioctl successful\n");
    }
    else {
        printf("ioctl failed: fd not initialized\n");
    }
}
