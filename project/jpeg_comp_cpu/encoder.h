//==========================================================================
// This file contains the functions needed to encode the PEG image.
//
// Author: George Rosier (gmrosier@email.arizona.edu)
// Date: 4/02/2017
//==========================================================================

#ifndef ENCODER_H
#define ENCODER_H

#include <stdio.h>


//==========================================================================
// Macros For Min & Max if not defined elsewhere.
//==========================================================================
#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

#ifndef max
#define max(a, b) ((a > b) ? a : b)
#endif

//==========================================================================
// Structure to hold the Color Channel Information
//==========================================================================
typedef struct
{
    unsigned char * data;
    unsigned int width;
    unsigned int height;
} ChannelInfo;

//==========================================================================
// Structure to hold the Huffman Encoding Information
//==========================================================================
typedef struct
{
    unsigned short value;
    unsigned short additional;
    unsigned char  length;
    unsigned char  add_length;
} EncodeInfo;

//==========================================================================
// Provided a uniform scaling factor to the quantization table.
//
// Parameter:
//      quality - an integer from 1 to 100 that specifies the ammont of
//                scaling to apply to the quantization table.
//
//                100 = Highest Quality, Least Compression
//                50  = Normal Quality
//                1   = Least Quality, Highest Compression
//==========================================================================
void init_qtable(unsigned int quality_factor);

//==========================================================================
// Compress a full image
//
// Parameters:
//  channels - The number of channels in the image
//  info     - A pointer to an array that stores the channel information
//  fid      - The output file id
//==========================================================================
void compress_img(unsigned int channels, ChannelInfo * info, FILE * fid);

//==========================================================================
// Helper function for fetching the Huffman code length array
//
// Parameters:
//  isDC    - Flag for we are requesting the DC codes
//  channel - The codes for this specified channel
//
// Return:
//  Huffman code length array
//==========================================================================
const unsigned char * get_code_lens(unsigned char isDC, unsigned int channel);

//==========================================================================
// Helper function for getting the Huffman code values array
//
// Parameters:
//  isDC    - Flag for we are requesting the DC codes
//  channel - The codes for this specified channel
//
// Return:
//  Huffman code values array
//==========================================================================
const unsigned char * get_code_values(unsigned char isDC, unsigned int channel);

//==========================================================================
// Helper function that will get the numnber of Huffman codes
//
// Parameters:
//  isDC    - Flag for we are requesting the DC codes
//
// Return:
//  Numnber of Huffman codes
//==========================================================================
const unsigned int get_code_count(unsigned char isDC);

//==========================================================================
// Helper function for fetching the currect luminance quantization table.
//
// Return:
//  Luminance Quantization Table
//==========================================================================
const unsigned char * get_yqtable();

//==========================================================================
// Helper function for fetching the currect chrominance quantization table.
//
// Return:
//  Chrominance Quantization Table
//==========================================================================
const unsigned char * get_cqtable();

//==========================================================================
// Helper function for fetching the zigzag read pattern
//
// Return:
//  Zig-Zag Read Pattern
//==========================================================================
const unsigned char * get_read_pattern();

#endif /* ENCODER_H */
