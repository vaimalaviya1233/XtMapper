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

char str[16];

typedef struct mouse_context {
	JavaVM  *javaVM;
    pthread_mutex_t  lock;
	int done;
	const char* dev;
    int port;
} mouseContext;
mouseContext g_ctx;

struct Socket {
    int client_fd;
    int server_fd;
} s_cts;
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


void create_socket(struct Socket *Socket, int PORT){
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    // Creating socket file descriptor
    if ((Socket->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("mouse_read: Socket creation error \n");
    }

    // Forcefully attaching socket to the port
    if (setsockopt(Socket->server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        printf("mouse_read: setsockopt error \n");
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(Socket->server_fd, (struct sockaddr*)&address,
             sizeof(address)) < 0) {
        printf("mouse_read: socket bind error \n");
    }

    printf("Waiting for overlay...\n");
    if (listen(Socket->server_fd, 3) < 0) {
        printf("mouse_read: socket listen failed \n");
    }

    if ((Socket->client_fd
                 = accept(Socket->server_fd, (struct sockaddr*)&address,
                          (socklen_t*)&addrlen)) < 0) {
        printf("mouse_read: connection failed \n");
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

void * UpdateMouse(void* context) {
    mouseContext *pctx = (mouseContext*) context;
    JavaVM *javaVM = pctx->javaVM;
    JNIEnv *env;
    jint res = (*javaVM)->GetEnv(javaVM, (void**)&env, JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
        if (JNI_OK != res) {
            return NULL;
        }
    }
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

        if ((fd = open(pctx->dev, O_RDONLY)) == -1) {
            perror("opening device");
            exit(EXIT_FAILURE);
        }

        create_socket(&s_cts, pctx->port);

        char buffer[12] = { 0 };

        read(s_cts.client_fd, buffer, 12);
        printf("%s", buffer);

        if (strcmp(buffer, "mouse_read\n") == 0) {
            struct input_event ie;
            while (read(fd, &ie, sizeof(struct input_event))) {
                send_data(&ie, s_cts.client_fd);
            }
            close(fd);
        }
        else if (strcmp(buffer, "getevent") == 0) {

        }

        close(s_cts.client_fd); // closing the connected socket
        close(s_cts.server_fd); // closing the listening socket
        return 0;
    }

    (*javaVM)->DetachCurrentThread(javaVM);
    return context;
    }


    JNIEXPORT void JNICALL
    Java_com_xtr_keymapper_Input_startMouse(JNIEnv *env, jobject instance, jstring device, jint port) {
        setlinebuf(stdout);
        g_ctx.dev = (*env)->GetStringUTFChars(env, device, 0);
        g_ctx.port = port;
        pthread_t threadInfo_;
        pthread_attr_t threadAttr_;

        pthread_attr_init(&threadAttr_);
        pthread_attr_setdetachstate(&threadAttr_, PTHREAD_CREATE_DETACHED);

        pthread_mutex_init(&g_ctx.lock, NULL);

        jclass clz = (*env)->GetObjectClass(env, instance);
        (*env)->NewGlobalRef(env, clz);
        (*env)->NewGlobalRef(env, instance);

        int result = pthread_create(&threadInfo_, &threadAttr_, UpdateMouse, &g_ctx);
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
