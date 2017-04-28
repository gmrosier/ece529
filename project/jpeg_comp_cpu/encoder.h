#ifndef ENCODER_H
#define ENCODER_H

#include <stdio.h>

typedef struct
{
    unsigned short value;
    unsigned short additional;
    unsigned char  length;
    unsigned char  add_length;
} encode_info;

void compress_img(unsigned char * img, unsigned int width, unsigned int height, FILE * fid);
const unsigned char * get_code_lens(unsigned char isDC);
const unsigned char * get_code_values(unsigned char isDC);
const unsigned int get_code_count(unsigned char isDC);
const unsigned char * get_qtable();

#endif /* ENCODER_H */
