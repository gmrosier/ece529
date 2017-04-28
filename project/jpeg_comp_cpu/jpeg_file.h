#ifndef JPEG_FILE_H
#define JPEG_FILE_H

#include <stdio.h>
#include "encoder.h"

FILE * open_stream(const char * file_name, unsigned int height, unsigned int width);
void close_stream(FILE * fid);
void write_stream(FILE * fid, const encode_info * item);

void file_read(const char * file_name, unsigned char * image, unsigned int width, unsigned int height);

#endif /* JPEG_FILE_H */