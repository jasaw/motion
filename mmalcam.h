/*
 * mmalcam.h
 *
 *    Include file for mmalcam.c
 *
 *    Copyright 2013 by Nicholas Tuckett
 *    This software is distributed under the GNU public license version 2
 *    See also the file 'COPYING'.
 */

#ifndef MMALCAM_H_
#define MMALCAM_H_

typedef struct mmalcam_context *mmalcam_context_ptr;

struct mmalcam_buffer_set {
    struct MMAL_BUFFER_HEADER_T *buffer_header;
    unsigned char *data;
};

typedef struct mmalcam_context {
    struct context *cnt;        /* pointer to parent motion
                                   context structure */
    int width;
    int height;
    int framerate;

    struct MMAL_COMPONENT_T *camera_component;
    struct MMAL_PORT_T *camera_capture_port;
    struct MMAL_POOL_T *camera_buffer_pool;
    struct MMAL_QUEUE_T *camera_buffer_queue;
    struct raspicam_camera_parameters_s *camera_parameters;
} mmalcam_context;

void mmalcam_release_buffer_header(struct MMAL_BUFFER_HEADER_T *buffer_header, unsigned char **image_buffer);
int mmalcam_start (struct context *);
int mmalcam_next (struct context *, unsigned char **, void **);
void mmalcam_cleanup (struct mmalcam_context *);

#endif /* MMALCAM_H_ */
