/*    alt_detect_dl.h
 *
 *    Include file for alt_detect_dl.c
 *      Author: Joo Saw
 *      This software is distributed under the GNU public license version 2
 *      See also the file 'COPYING'.
 *
 */

#ifndef _ALT_DETECT_DL_H
#define _ALT_DETECT_DL_H

#include <time.h>

#include "alt_detect.h"


#define TIMESTAMP_BUFFER_LENGTH     16


typedef struct
{
    struct timespec request_ts;
    struct timespec result_ts[TIMESTAMP_BUFFER_LENGTH];
    struct timespec latency[TIMESTAMP_BUFFER_LENGTH];
    unsigned int result_ts_index;
} alt_detect_stats_t;


int alt_detect_dl_load(const char *lib_detect_path);
void alt_detect_dl_unload(void);

int alt_detect_dl_init(const char *config_file);
void alt_detect_dl_uninit();

int alt_detect_dl_initialized(void);

int alt_detect_dl_process_yuv420(int id, struct timeval *timestamp,
                                 unsigned char *image, int width, int height,
                                 alt_detect_stats_t *stats);
int alt_detect_dl_result_ready(int id);
int alt_detect_dl_get_result(int id, float score_threshold,
                             alt_detect_result_t *alt_detect_result,
                             alt_detect_stats_t *stats);
float alt_detect_dl_get_min_score(alt_detect_result_t *alt_detect_result);
void alt_detect_dl_free_result(alt_detect_result_t *alt_detect_result);

void alt_detect_get_stats(alt_detect_stats_t *stats, double *fps, double *latency);


#endif /* _ALT_DETECT_DL_H */
