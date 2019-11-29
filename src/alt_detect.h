/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** Author: Joo Saw
**
** -------------------------------------------------------------------------*/

#ifndef _ALT_DETECT_H_
#define _ALT_DETECT_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int x;
    int y;
    int id;
} alt_detect_point_t;


typedef struct
{
    alt_detect_point_t p[2];
} alt_detect_line_t;


typedef struct
{
    alt_detect_line_t *lines;
    alt_detect_point_t *points;
    int num_lines;
    int num_points;
    float score;
} alt_detect_obj_t;


typedef struct
{
    alt_detect_obj_t *objs;
    int num_objs;
    struct timeval timestamp;
} alt_detect_result_t;


// all model specific configurations go into config file, things like
// path to model, classes/id to detect, hardware that runs the model
extern int alt_detect_init(const char *config_file);

extern void alt_detect_uninit(void);

// Insert an image into the processing queue for alternate detection.
// id        : image source ID. Must be >= 0.
// timestamp : image timestamp. Set to NULL if not used.
// image     : image in YUV420 format
// width     : image width
// height    : image height
// return 0 on success
extern int alt_detect_process_yuv420(int id,
                                     struct timeval *timestamp,
                                     unsigned char *image,
                                     int width,
                                     int height);

extern int alt_detect_result_ready(int id);

// caller frees memory by calling alt_detect_free_results
// alt_detect_result will be initialized
extern int alt_detect_get_result(int id, float score_threshold,
                                 alt_detect_result_t *alt_detect_result);

// safe to call with null pointer
extern void alt_detect_free_result(alt_detect_result_t *alt_detect_result);

extern const char *alt_detect_err_msg(void);



extern int alt_detect_save_yuv420(unsigned char *image, int width, int height,
                                  const char *filename);

extern int alt_detect_render_save_yuv420(unsigned char *image, int width, int height,
                                         alt_detect_result_t *alt_detect_result,
                                         const char *filename);


#ifdef __cplusplus
}  /* End of the 'extern "C"' block */
#endif

#endif // _ALT_DETECT_H_
