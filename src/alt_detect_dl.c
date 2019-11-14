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
#include <time.h>
#include <string.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>

#include "translate.h"
#include "logger.h"
#include "alt_detect_dl.h"


#define TIMESTAMP_BUFFER_LENGTH     16


typedef struct
{
    void *handle;
    int (*alt_detect_init)(const char *);
    void (*alt_detect_uninit)();
    int (*alt_detect_process_scaled_bgr)(unsigned char *, int, int);
    int (*alt_detect_result_ready)(void);
    int (*alt_detect_queue_empty)(void);
    int (*alt_detect_get_result)(float, int, int, int, int, alt_detect_result_t *);
    void (*alt_detect_free_result)(alt_detect_result_t *);
    const char *(*alt_detect_err_msg)(void);
    void (*alt_detect_get_input_width_height)(int *, int *);
    int init;
    int input_width;
    int input_height;
    struct timespec request_ts;
    struct timespec result_ts[TIMESTAMP_BUFFER_LENGTH];
    struct timespec latency[TIMESTAMP_BUFFER_LENGTH];
    unsigned int result_ts_index;
} lib_detect_info;


static lib_detect_info libdetect = {.handle=NULL, .init=0};


static void get_scaled_image_dimensions(int width, int height, int *scaled_width, int *scaled_height)
{
    double scale_h = (double)libdetect.input_height / height;
    double scale_w = (double)libdetect.input_width  / width;
    double scale   = MIN(scale_h, scale_w);
    *scaled_width  = (int)(width * scale);
    *scaled_height = (int)(height * scale);
}


static unsigned char *scale_image(unsigned char *src_img, int width, int height, int scaled_width, int scaled_height)
{
    uint8_t *src_data[4] = {0};
    uint8_t *dst_data[4] = {0};
    int src_linesize[4] = {0};
    int dst_linesize[4] = {0};
    int src_w = width;
    int src_h = height;
    int dst_w = scaled_width;
    int dst_h = scaled_height;
    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;
    struct SwsContext *sws_ctx = NULL;
    //unsigned char *processed_img = NULL;
    //int i;
    //unsigned int processed_img_len;

    // create scaling context
    sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
                             dst_w, dst_h, dst_pix_fmt,
                             SWS_BICUBIC, NULL, NULL, NULL);
    if (!sws_ctx)
    {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO
            ,_("Impossible to create scale context for image conversion fmt:%s s:%dx%d -> fmt:%s s:%dx%d"),
            av_get_pix_fmt_name(src_pix_fmt), src_w, src_h,
            av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h);
        goto end;
    }

    int srcNumBytes = av_image_fill_arrays(src_data, src_linesize, src_img,
                                           src_pix_fmt, src_w, src_h, 1);
    if (srcNumBytes < 0)
    {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO
            ,_("Failed to fill image arrays: code %d"),
            srcNumBytes);
        goto end;
    }

    int dst_bufsize;
    if ((dst_bufsize = av_image_alloc(dst_data, dst_linesize,
                       dst_w, dst_h, dst_pix_fmt, 1)) < 0)
    {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO,_("Failed to allocate dst image"));
        goto end;
    }

    // convert to destination format
    sws_scale(sws_ctx, (const uint8_t * const*)src_data,
              src_linesize, 0, src_h, dst_data, dst_linesize);

    //*processed_img_len = dst_w * dst_h * 3 * sizeof(unsigned char);
    //processed_img = malloc(*processed_img_len);
    //if (processed_img == NULL)
    //{
    //    MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO,_("Failed to allocate memory"));
    //    goto end;
    //}
    //// scale BGR range from 0-255 to -1.0 to 1.0
    //for (i = 0; i < dst_w * dst_h * 3; i++)
    //{
    //    float value = dst_data[0][i];
    //    processed_img[i] = (value - 127.5) * 0.007843;
    //}


    //FILE *wrfile;

    //wrfile = fopen("/tmp/yuv420p.raw", "wb");
    //fwrite(src_img, 1, srcNumBytes, wrfile);
    //fclose(wrfile);

    // use gnuplot to show scaled image
    // gnuplot> plot 'scaled_bgr24.raw' binary array=(300,300) flipy format='%uchar%uchar%uchar' using 3:2:1 with rgbimage
    //wrfile = fopen("/tmp/scaled_bgr24.raw", "wb");
    //fwrite(dst_data[0], 1, dst_bufsize, wrfile);
    //fclose(wrfile);

    // gnuplot> plot 'scaled_bgr_float.raw' binary array=(300,300) flipy format='%float%float%float' using ($3*127.5+127.5):($2*127.5+127.5):($1*127.5+127.5) with rgbimage
    //wrfile = fopen("/tmp/scaled_bgr_float.raw", "wb");
    //fwrite(processed_img, *processed_img_len, 1, wrfile);
    //fclose(wrfile);

end:
    //av_freep(&dst_data[0]);
    if (sws_ctx)
        sws_freeContext(sws_ctx);
    return dst_data[0];
}


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
            libdetect.alt_detect_get_input_width_height(&libdetect.input_width, &libdetect.input_height);
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
        int scaled_width = 0;
        int scaled_height = 0;
        get_scaled_image_dimensions(width, height, &scaled_width, &scaled_height);
        unsigned char *scaled_img = scale_image(image, width, height, scaled_width, scaled_height);
        if (scaled_img) {
            ret = libdetect.alt_detect_process_scaled_bgr(scaled_img, scaled_width, scaled_height);
            av_freep(&scaled_img);
            if (ret) {
                const char *errmsg = libdetect.alt_detect_err_msg();
                MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), errmsg);
            }
            clock_gettime(CLOCK_MONOTONIC, &libdetect.request_ts);
        } else
            ret = -1;
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


static void alt_detect_profile_store_result_ts(void)
{
    if (clock_gettime(CLOCK_MONOTONIC, &libdetect.result_ts[libdetect.result_ts_index]) == 0)
    {
        libdetect.latency[libdetect.result_ts_index].tv_sec  = libdetect.result_ts[libdetect.result_ts_index].tv_sec - libdetect.request_ts.tv_sec;
        libdetect.latency[libdetect.result_ts_index].tv_nsec = libdetect.result_ts[libdetect.result_ts_index].tv_nsec - libdetect.request_ts.tv_nsec;
        //printf("request tv_sec, tv_nsec: %lld.%.09ld\n", (long long)libdetect.request_ts.tv_sec, libdetect.request_ts.tv_nsec);
        //printf("result  tv_sec, tv_nsec: %lld.%.09ld\n", (long long)libdetect.result_ts[libdetect.result_ts_index].tv_sec, libdetect.result_ts[libdetect.result_ts_index].tv_nsec);
        //printf("latency tv_sec, tv_nsec: %lld.%.09ld\n", (long long)libdetect.latency[libdetect.result_ts_index].tv_sec, libdetect.latency[libdetect.result_ts_index].tv_nsec);

        libdetect.result_ts_index++;
        if (libdetect.result_ts_index >= TIMESTAMP_BUFFER_LENGTH)
            libdetect.result_ts_index = 0;
    }
}


void alt_detect_get_stats(double *fps, double *latency)
{
    struct timespec diff;
    double sum = 0;
    unsigned int oldest_ts_index = libdetect.result_ts_index;
    unsigned int newest_ts_index = libdetect.result_ts_index - 1;
    if (newest_ts_index >= TIMESTAMP_BUFFER_LENGTH)
        newest_ts_index = TIMESTAMP_BUFFER_LENGTH - 1;
    if ((libdetect.result_ts[newest_ts_index].tv_sec > 0) && (libdetect.result_ts[oldest_ts_index].tv_sec > 0))
    {
        diff.tv_sec = libdetect.result_ts[newest_ts_index].tv_sec - libdetect.result_ts[oldest_ts_index].tv_sec;
        diff.tv_nsec = libdetect.result_ts[newest_ts_index].tv_nsec - libdetect.result_ts[oldest_ts_index].tv_nsec;
        *fps = TIMESTAMP_BUFFER_LENGTH / (diff.tv_sec + (double)diff.tv_nsec/1.0e9);

        int i;
        for (i = 0; i < TIMESTAMP_BUFFER_LENGTH; i++) {
            sum += libdetect.latency[i].tv_sec + (double)libdetect.latency[i].tv_nsec/1.0e9;
        }
        //printf("sum latency tv_sec, tv_nsec: %lld.%.09ld\n", (long long)sum.tv_sec, sum.tv_nsec);
        *latency = sum / TIMESTAMP_BUFFER_LENGTH;
        //printf("sum latency %fs\n", *latency);
    }
}


// returns number of results, negative for error
int alt_detect_dl_get_result(float score_threshold, int org_width, int org_height,
                             alt_detect_result_t *alt_detect_result)
{
    int ret = 0;
    if (libdetect.handle) {
        int scaled_width = 0;
        int scaled_height = 0;
        get_scaled_image_dimensions(org_width, org_height, &scaled_width, &scaled_height);
        ret = libdetect.alt_detect_get_result(score_threshold, org_width, org_height,
                                              scaled_width, scaled_height, alt_detect_result);
        if (ret < 0) {
            const char *errmsg = libdetect.alt_detect_err_msg();
            MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("%s"), errmsg);
        }
        alt_detect_profile_store_result_ts();
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
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_process_scaled_bgr), libdetect.handle, "alt_detect_process_scaled_bgr");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_queue_empty), libdetect.handle, "alt_detect_queue_empty");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_result_ready), libdetect.handle, "alt_detect_result_ready");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_get_result), libdetect.handle, "alt_detect_get_result");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_free_result), libdetect.handle, "alt_detect_free_result");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_err_msg), libdetect.handle, "alt_detect_err_msg");
    err |= alt_detect_dl_load_sym((void **)(&libdetect.alt_detect_get_input_width_height), libdetect.handle, "alt_detect_get_input_width_height");
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
