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


#include "alt_detect.h"


int alt_detect_dl_load(const char *lib_detect_path);
void alt_detect_dl_unload(void);

int alt_detect_dl_init(const char *config_file);
void alt_detect_dl_uninit();

int alt_detect_dl_initialized(void);

int alt_detect_dl_process_yuv420(unsigned char *image, int width, int height);
int alt_detect_dl_result_ready(void);
int alt_detect_dl_queue_empty(void);
int alt_detect_dl_get_result(float score_threshold, alt_detect_result_t *alt_detect_result);
float alt_detect_dl_get_min_score(alt_detect_result_t *alt_detect_result);
void alt_detect_dl_free_result(alt_detect_result_t *alt_detect_result);


#endif /* _ALT_DETECT_DL_H */
