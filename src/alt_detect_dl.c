/*    alt_detect_dl.c
 *
 *    Detect changes in a video stream.
 *      Author: Joo Saw
 *    This software is distributed under the GNU public license version 2
 *    See also the file 'COPYING'.
 *
 */
#include <stdio.h>
#include <dlfcn.h>

#include "translate.h"
#include "logger.h"
#include "alt_detect_dl.h"


typedef struct
{
    void *handle;
    int (*alt_detect_init)(const char *);
    void (*alt_detect_uninit)();
    int (*alt_detect_process_yuv420)(unsigned char *, int, int);
    int (*alt_detect_result_ready)(void);
    int (*alt_detect_queue_empty)(void);
    int (*alt_detect_get_result)(float, alt_detect_result_t *);
    void (*alt_detect_free_result)(alt_detect_result_t *);
    const char *(*alt_detect_err_msg)(void);
    int init;
} lib_detect_info;


static lib_detect_info libdetect = {.handle=NULL, .init=0};


int alt_detect_dl_initialized(void)
{
    return ((libdetect.handle) && (libdetect.init));
}


int alt_detect_dl_init(const char *config_file)
{
    if (libdetect.handle) {
        if (!libdetect.init) {
            if (libdetect.alt_detect_init(config_file)) {
                const char *errmsg = libdetect.alt_detect_err_msg();
                MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), errmsg);
                return -1;
            }
            libdetect.init = 1;
        }
    }
    return 0;
}


void alt_detect_dl_uninit()
{
    if (libdetect.handle) {
        if (libdetect.init) {
            libdetect.alt_detect_uninit();
            libdetect.init = 0;
        }
    }
}


int alt_detect_dl_process_yuv420(unsigned char *image, int width, int height)
{
    int ret = 0;
    if (libdetect.handle) {
        ret = libdetect.alt_detect_process_yuv420(image, width, height);
        if (ret) {
            const char *errmsg = libdetect.alt_detect_err_msg();
            MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), errmsg);
        }
    }
    return ret;
}


int alt_detect_dl_result_ready(void)
{
    int ret = 0;
    if (libdetect.handle) {
        ret = libdetect.alt_detect_result_ready();
        if (ret < 0) {
            const char *errmsg = libdetect.alt_detect_err_msg();
            MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), errmsg);
            ret = 0;
        }
    }
    return ret;
}


int alt_detect_dl_queue_empty(void)
{
    if (libdetect.handle)
        return libdetect.alt_detect_queue_empty();
    return 0;
}


// returns number of results, negative for error
int alt_detect_dl_get_result(float score_threshold, alt_detect_result_t *alt_detect_result)
{
    int ret = 0;
    if (libdetect.handle) {
        ret = libdetect.alt_detect_get_result(score_threshold, alt_detect_result);
        if (ret < 0) {
            const char *errmsg = libdetect.alt_detect_err_msg();
            MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), errmsg);
        }
    }
    return ret;
}


void alt_detect_dl_free_result(alt_detect_result_t *alt_detect_result)
{
    if (libdetect.handle)
        libdetect.alt_detect_free_result(alt_detect_result);
}


// return negative number if no result found
float alt_detect_dl_get_min_score(alt_detect_result_t *alt_detect_result)
{
    float min = -1;
    int i;

    if (alt_detect_result->num_objs > 0)
        min = alt_detect_result->objs[0].score;
    for (i = 1; i < alt_detect_result->num_objs; i++) {
        if (alt_detect_result->objs[i].score < min)
            min = alt_detect_result->objs[i].score;
    }
    return min;
}


static int alt_detect_dl_load_sym(void **func, void *handle, char *symbol)
{
    char *sym_error;

    *func = dlsym(handle, symbol);
    if ((sym_error = dlerror()) != NULL)
    {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), sym_error);
        return -1;
    }
    return 0;
}


int alt_detect_dl_load(const char *lib_detect_path)
{
    if (libdetect.handle) {
        return 0;
    }

    int err = 0;

    libdetect.handle = dlopen(lib_detect_path, RTLD_LAZY);
    if (!libdetect.handle) {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), dlerror());
        return -1;
    }

    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_init),   libdetect.handle, "alt_detect_init");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_uninit), libdetect.handle, "alt_detect_uninit");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_process_yuv420), libdetect.handle, "alt_detect_process_yuv420");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_queue_empty), libdetect.handle, "alt_detect_queue_empty");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_result_ready), libdetect.handle, "alt_detect_result_ready");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_get_result), libdetect.handle, "alt_detect_get_result");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_free_result), libdetect.handle, "alt_detect_free_result");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_err_msg), libdetect.handle, "alt_detect_err_msg");
    if (err) {
        alt_detect_dl_unload();
        return -1;
    }

    return 0;
}


void alt_detect_dl_unload(void)
{
    if (libdetect.handle) {
        dlclose(libdetect.handle);
        libdetect.handle = NULL;
    }
}
