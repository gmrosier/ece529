#ifndef ENCODER_H
#define ENCODER_H

#include <stdio.h>


typedef struct
{
    unsigned char * data;
    unsigned int width;
    unsigned int height;
} ChannelInfo;

typedef struct
{
    unsigned short value;
    unsigned short additional;
    unsigned char  length;
    unsigned char  add_length;
} EncodeInfo;

void init_qtable(float quality_factor);
void compress_img(unsigned char * img, unsigned int channels, ChannelInfo * info, FILE * fid);
const unsigned char * get_code_lens(unsigned char isDC);
const unsigned char * get_code_values(unsigned char isDC);
const unsigned int get_code_count(unsigned char isDC);
const unsigned char * get_yqtable();
const unsigned char * get_cqtable();
const unsigned char * get_read_pattern();

#endif /* ENCODER_H */
