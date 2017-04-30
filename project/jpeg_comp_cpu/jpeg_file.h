//==========================================================================
// This file contains the functions needed to output the JPEG image and all
// the required header portions.
//
// Author: George Rosier (gmrosier@email.arizona.edu)
// Date: 4/02/2017
//==========================================================================

#ifndef JPEG_FILE_H
#define JPEG_FILE_H

#include <stdio.h>
#include "encoder.h"

//================================================================================
// This function will open the output file and fill in the proper JPEG header
// information.
//
// Parameters:
//  file_name   - Output File Name
//  width       - The width of the input & output images
//  height      - The height of the input & output images
//  info        - The individual color  channel information
//  channels    - The number of color channels in the image
//
// Return:
//  It will return the file id of the opened output file
//================================================================================
FILE * open_stream(const char * file_name, unsigned int width, unsigned int height, ChannelInfo * info, unsigned int channels);

//================================================================================
// This function will close the JPEG file and write out the end of image marker.
//
// Parameters:
//  fid - Output File ID
//================================================================================
void close_stream(FILE * fid);

//================================================================================
// This function write the encoded information to the file.
//
// Parameters:
//  fid     - Output File ID
//  item    - The encoded item to be outputed to file
//================================================================================
void write_stream(FILE * fid, const EncodeInfo * item);

//================================================================================
// This function reads file with the specified parameters and stores it in the
// following format. This file will also convert a RGB image to YCrCb 4:2:2
// format. This function assumes that the input file is a raw binary format
// that is either 8-bit grayscale or 24-bit (stored in a 32-bit) RGB format.
//
//  Input Image                         Output Array
//  11111111222222223333333344444440    1111111111111111111111111111111111111111111111111111111111111111
//  11111111222222223333333344444440    2222222222222222222222222222222222222222222222222222222222222222
//  11111111222222223333333344444440    3333333333333333333333333333333333333333333333333333333333333333
//  11111111222222223333333344444440    4444444444444444444444444444444444444444444444444444444400000000
//  11111111222222223333333344444440
//  11111111222222223333333344444440
//  11111111222222223333333344444440
//  11111111222222223333333344444440
//
// Parameters:
//      file_name    - The file to open and read the image from
//      width        - The input width
//      height       - The input height
//      channels     - The number of channels in the image, the two valid values
//                     are either 1 for grayscale or 3 for 24-bit RGB.
//      channel_info - An array of structures to store the channel information
//                     in. This array needs to be at least as large as the number
//                     of channels.
//================================================================================
void file_read(const char * file_name, unsigned int width, unsigned int height,
               unsigned int channels, ChannelInfo * info);

#endif /* JPEG_FILE_H */