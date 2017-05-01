//==========================================================================
// This file implements the encoder portion of the softare. It is
// responsible for taking a loaded image and encoding it into the JPEG
// image format.
//
// Author: George Rosier (gmrosier@email.arizona.edu)
// Date: 4/02/2017
//==========================================================================

#include "encoder.h"

#include <stdlib.h>
#include <math.h>

#include "tables.h"
#include "jpeg_file.h"

//==========================================================================
// Local Variables to hold the scaled quantization tables.
//==========================================================================
static unsigned char yqTable[8 * 8];
static unsigned char cqTable[8 * 8];

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
void init_qtable(unsigned int quality)
{
    quality = max(1, min(100, quality));

    if (quality < 50)
        quality = 5000 / quality;
    else
        quality = 200 - quality * 2;
    
    for (int i = 0; i < 64; i++)
    {
        unsigned int value = (unsigned int)((y_qTable[i] * quality + 50.0f) / 100.0f);
        value = max(1, min(255, value));
        yqTable[i] = (unsigned char)value;

        value = (unsigned int)((cr_qTable[i] * quality + 50.0f) / 100.0f);
        value = max(1, min(255, value));
        cqTable[i] = (unsigned char)value;
    }
}

//==========================================================================
// Helper function for fetching the currect luminance quantization table.
//
// Return:
//  Luminance Quantization Table
//==========================================================================
const unsigned char * get_yqtable()
{
    return yqTable;
}

//==========================================================================
// Helper function for fetching the currect chrominance quantization table.
//
// Return:
//  Chrominance Quantization Table
//==========================================================================
const unsigned char * get_cqtable()
{
    return cqTable;
}

//==========================================================================
// Helper function for fetching the zigzag read pattern
//
// Return:
//  Zig-Zag Read Pattern
//==========================================================================
const unsigned char * get_read_pattern()
{
    return output_pattern;
}

//==========================================================================
// Local Variables to hold the luminance DC & AC Huffman Codes
//==========================================================================
static HuffInfo y_dc_table[12];
static HuffInfo y_ac_table[256];

//==========================================================================
// Local Variables to hold the chrominance DC & AC Huffman Codes
//==========================================================================
static HuffInfo c_dc_table[12];
static HuffInfo c_ac_table[256];

//==========================================================================
// Helper function that will take the huffman table specifcation and
// populate the huffman table.
//==========================================================================
void load_huffman_table(const unsigned char * codes_per_len, const unsigned char * values, HuffInfo * table)
{
    unsigned short code = 0;
    unsigned int pos = 0;

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < codes_per_len[i]; j++)
        {
            table[values[pos]].length = i + 1;
            table[values[pos]].value = code;
            pos++;
            code++;
        }
        code <<= 1;
    }
}

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
const unsigned char * get_code_lens(unsigned char isDC, unsigned int channel)
{
    if (isDC == 1)
    {
        if (channel == 0)
        {
            return y_dc_codes_per_len;
        }
        else
        {
            return c_dc_codes_per_len;
        }
    }
    
    if (channel == 0)
    {
        return y_ac_codes_per_len;
    }

    return c_ac_codes_per_len;
}

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
const unsigned char * get_code_values(unsigned char isDC, unsigned int channel)
{
    if (isDC == 1)
    {
        if (channel == 0)
        {
            return y_dc_values;
        }
        else
        {
            return c_dc_values;
        }
    }

    if (channel == 0)
    {
        return y_ac_values;
    }

    return c_ac_values;
}

//==========================================================================
// Helper function that will get the numnber of Huffman codes
//
// Parameters:
//  isDC    - Flag for we are requesting the DC codes
//
// Return:
//  Numnber of Huffman codes
//==========================================================================
const unsigned int get_code_count(unsigned char isDC)
{
    if (isDC == 1)
        return 12;

    return 162;
}

//==========================================================================
// Row based 1-D DCT.
// 1-D DCT based on Vetterli and Ligtenberg but we will delay divide by
// two to quantization. This function performs the 1D-DCT inplace and will
// destroy input data.
//
// Parameters:
//     block - A pointer to a 8x8 pixels layed out linearly
//             8x8 Memory Layout
//                 00 = Pixel @ Row 0, Column 0
//                 [ 00, 01, ..., 07, 10, 11, ..., 77]
//
// For more information about the algorithm see
// Section 4.3.2 of JPEG: Still Image Data Comppression Standard.
//==========================================================================
void rdct1d(float * block)
{
    for (unsigned int i = 0; i < 8; i++)
    {
        unsigned int row = 8 * i;

        // Sum Terms
        float s07 = block[row] + block[7 + row];
        float s12 = block[1 + row] + block[2 + row];
        float s34 = block[3 + row] + block[4 + row];
        float s56 = block[5 + row] + block[6 + row];

        // Difference Terms
        float d07 = block[row] - block[7 + row];
        float d12 = block[1 + row] - block[2 + row];
        float d34 = block[3 + row] - block[4 + row];
        float d56 = block[5 + row] - block[6 + row];

        // Combined Terms
        float ss07s34 = s07 + s34;
        float ss12s56 = s12 + s56;
        float sd12d56 = d12 + d56;
        float dd12d56 = d12 - d56;
        float ds07s34 = s07 - s34;
        float ds12s56 = s12 - s56;

        // Combine More Terms w/ Multiply
        float C4mds12s56 = C(4) * ds12s56;
        float C4msd12d56 = C(4) * sd12d56;

        //output
        // S0 =  C4 * ((s07 + s34) + (s12 + s56))
        block[row] = (C(4) * (ss07s34 + ss12s56));

        // S1 =  C1 * (d07 + C4mds12s56) - S1 * (-d34 - C4msd12d56)
        block[1 + row] = (C(1) * (d07 + C4mds12s56) - S(1) * (-d34 - C4msd12d56));

        // S2 =  C6 * (dd12d56) + S6 * (ds07s34)
        block[2 + row] = (C(6) * dd12d56 + S(6) * ds07s34);

        // S3 =  C3 * (d07 - C4 * (ds12s56)) - S3 * (d34 - C4 * (d12 + d56))
        block[3 + row] = (C(3) * (d07 - C4mds12s56) - S(3) * (d34 - C4msd12d56));

        // S4 =  C4 * ((s07 + s34) - (s12 + s56))
        block[4 + row] = (C(4) * (ss07s34 - ss12s56));

        // S5 =  S3 * (d07 - C4 * (s12 - s56)) + C3 * (d34 - C4 * (d12 + d56))
        block[5 + row] = (S(3) * (d07 - C4mds12s56) + C(3) * (d34 - C4msd12d56));

        // S6 = -S6 * (d12 - d56) + C6 * (s07 - s34)
        block[6 + row] = (-S(6) * dd12d56 + C(6) * ds07s34);

        // S7 =  S1 * (d07 + C4 * (s12 - s56)) + C1 * (-d34 - C4 * (d12 + d56))
        block[7 + row] = (S(1) * (d07 + C4mds12s56) + C(1) * (-d34 - C4msd12d56));
    }
}

//==========================================================================
// Column based 1-D DCT.
// 1-D DCT based on Vetterli and Ligtenberg but we will delay divide by
// two to quantization. This function performs the 1D-DCT inplace and will
// destroy input data.
//
// Parameters:
//     block - A pointer to a 8x8 pixels layed out linearly
//             8x8 Memory Layout
//                 00 = Pixel @ Row 0, Column 0
//                 [ 00, 01, ..., 07, 10, 11, ..., 77]
//
// For more information about the algorithm see
// Section 4.3.2 of JPEG: Still Image Data Comppression Standard.
//==========================================================================
void cdct1d(float * block)
{
    for (unsigned int i = 0; i < 8; i++)
    {
        // Sum Terms
        float s07 = block[i] + block[56 + i];
        float s12 = block[8 + i] + block[16 + i];
        float s34 = block[24 + i] + block[32 + i];
        float s56 = block[40 + i] + block[48 + i];

        // Difference Terms
        float d07 = block[0 + i] - block[56 + i];
        float d12 = block[8 + i] - block[16 + i];
        float d34 = block[24 + i] - block[32 + i];
        float d56 = block[40 + i] - block[48 + i];

        // Combined Terms
        float ss07s34 = s07 + s34;
        float ss12s56 = s12 + s56;
        float sd12d56 = d12 + d56;
        float dd12d56 = d12 - d56;
        float ds07s34 = s07 - s34;
        float ds12s56 = s12 - s56;

        // Combine More Terms w/ Multiply
        float C4mds12s56 = C(4) * ds12s56;
        float C4msd12d56 = C(4) * sd12d56;

        //output
        // S0 =  C4 * ((s07 + s34) + (s12 + s56))
        block[i] = (C(4) * (ss07s34 + ss12s56));

        // S1 =  C1 * (d07 + C4mds12s56) - S1 * (-d34 - C4msd12d56)
        block[8 + i] = (C(1) * (d07 + C4mds12s56) - S(1) * (-d34 - C4msd12d56));

        // S2 =  C6 * (dd12d56) + S6 * (ds07s34)
        block[16 + i] = (C(6) * dd12d56 + S(6) * ds07s34);

        // S3 =  C3 * (d07 - C4 * (ds12s56)) - S3 * (d34 - C4 * (d12 + d56))
        block[24 + i] = (C(3) * (d07 - C4mds12s56) - S(3) * (d34 - C4msd12d56));

        // S4 =  C4 * ((s07 + s34) - (s12 + s56))
        block[32 + i] = (C(4) * (ss07s34 - ss12s56));

        // S5 =  S3 * (d07 - C4 * (s12 - s56)) + C3 * (d34 - C4 * (d12 + d56))
        block[40 + i] = (S(3) * (d07 - C4mds12s56) + C(3) * (d34 - C4msd12d56));

        // S6 = -S6 * (d12 - d56) + C6 * (s07 - s34)
        block[48 + i] = (-S(6) * dd12d56 + C(6) * ds07s34);

        // S7 =  S1 * (d07 + C4 * (s12 - s56)) + C1 * (-d34 - C4 * (d12 + d56))
        block[56 + i] = (S(1) * (d07 + C4mds12s56) + C(1) * (-d34 - C4msd12d56));
    }
}

//==========================================================================
// This function will call the Row 1D DCT followed by the Column 1D DCT.
// This will perform a 2D-DCT on the provided block. This function performs
// the 2D-DCT inplace and will destroy input data.
//
// Parameters:
//     block - A pointer to a 8x8 pixels layed out linearly
//             8x8 Memory Layout
//                 00 = Pixel @ Row 0, Column 0
//                 [ 00, 01, ..., 07, 10, 11, ..., 77]
//==========================================================================
void dct2d(float * block)
{
    // Row 1D-DCT
    rdct1d(block);

    // Column 1D-DCT
    cdct1d(block);
}

//==========================================================================
// This function takes in unsigned char pixel values and shifts them down
// by 128. This function also converts the input data to float.
//
// Parameters:
//  input  - A pointer to a 8x8 pixels
//  output - A pointer to a 8x8 pixels
//==========================================================================
void zero_shift(unsigned char * input, float * output)
{
    for (int i = 0; i < 8 * 8; i++)
    {
        output[i] = (float)(input[i]) - 128.0f;
    }
}

//==========================================================================
// This functio performs the quantization and readout re-ordering. It also
// divides by 4 to take out the 2x scaling in the 1D-DCT. This function will
// also convert the DCT floats to shorts.
//
// Parameters:
//  input  - A pointer to a 8x8 pixels
//  qTable - A pointer to a 8x8 table of quaniztation values
//  output - A pointer to a 8x8 pixels
//==========================================================================
void quant_zigzag(float * input, const unsigned char * qTable, short * output)
{
    for (int i = 0; i < 8 * 8; i++)
    {
        output[output_pattern[i]] = ((short)roundf(input[i] / (float)qTable[i])) / 4;;
    }
}

//==========================================================================
// This helper function will return the minimum number of bits needed to
// store the absolute value of the specified value.
//
// Parameter:
//  value - The number to find the minimum number of bits to store
//
// Return:
//  The minimum number of bits to store the specified value
//==========================================================================
int num_bits(int value)
{
    // Check for Zero
    if (value == 0)
        return 0;

    // Take Absolute Value
    value = abs(value);

    // Find Min Number of Bits Need to Store This Number
    for (int i = 1; i < 16; i++)
    {
        if ((1 << i) > value)
            return i;
    }

    return 15;
}

//==========================================================================
// This function performs the zero-run length encoding on the specified
// 8x8 block. This function also performs DPCM on the DC component.
//
// Parameters:
//  input   - a pointer to the input 8x8 block (output of quantization)
//  output  - a pointer to the output buffer
//  length  - a pointer to the length of the output RLE
//  prev_dc - a pointer to the previous DC component value. It will be
//            updated with the current DC component value after the DPCM
//            was caluclated.
//==========================================================================
void zero_rle(short * input, RLEInfo * output, unsigned int * length, short * prev_dc)
{
    // Get Diff (DCPM)
    int diff = input[0] - *prev_dc;

    // Save Last DC Value
    *prev_dc = input[0];

    // Pass-through DC
    output[0].zero_cnt = 0;
    output[0].num_bits = num_bits(diff);
    output[0].value = diff;

    // Zero Run-Length Encode AC
    int idx = 0;
    int lst_idx = 0;
    int zero_cnt = 0;
    for (int i = 1; i < 8 * 8; i++)
    {
        // Count Zeros
        if (input[i] == 0)
        {
            zero_cnt++;
        }
        else
        {
            idx++;
        }

        // Add Entry
        if (idx != lst_idx)
        {
            while (zero_cnt > 15)
            {
                output[idx].zero_cnt = 15;
                output[idx].num_bits = 0;
                output[idx].value = 0;
                idx++;
                zero_cnt -= 15;
            }

            output[idx].zero_cnt = zero_cnt;
            output[idx].num_bits = num_bits(input[i]);
            output[idx].value = input[i];
            zero_cnt = 0;
            lst_idx = idx;
        }
    }

    // Add EOF
    idx++;
    output[idx].zero_cnt = 0;
    output[idx].num_bits = 0;
    output[idx].value = 0;

    // Return RLE Length
    *length = idx + 1;
}

//==========================================================================
// Take the Run Length Encoding and encode it to a binary file using 
// the Huffman codes provided in the table.
//
// Parameters:
//  rle        - a pointer to the input buffer of run-length ecoded data
//  rle_length - the size of the rle data
//  table      - Huffman Code Table
//  fid        - output file id
//==========================================================================
void encode(RLEInfo * rle, unsigned int rle_length, const HuffInfo * table, FILE * fid)
{
    EncodeInfo item;

    for (unsigned int i = 0; i < rle_length; i++)
    {
        int num_bits = rle[i].num_bits;

        // Get Code From Number of Bits
        short additional = rle[i].value;
        if (additional < 0)
        {
            additional += (1 << num_bits) - 1;
        }
        item.additional = additional;
        item.add_length = num_bits;

        // Code Idx (Zero Run Upper Nibble, Num Bits Lower Nibble) [RRRR, SSSS]
        int code_idx = (rle[i].zero_cnt << 4) + num_bits;

        // Code
        item.value = table[code_idx].value;

        // Length
        item.length = table[code_idx].length;

        // Write
        write_stream(fid, &item);
    }
}

//==========================================================================
// Compress an 8x8 Block
//
// Parameters:
//  block   - A pointer to a 8x8 pixels layed out linearly
//            8x8 Memory Layout
//               00 = Pixel @ Row 0, Column 0
//               [ 00, 01, ..., 07, 10, 11, ..., 77]
//  prev_dc - A pointer to the location of the prev dc value
//  fid     - The output file id
//==========================================================================
void compress_8x8(unsigned char * block, const unsigned char * qTable, const HuffInfo * dc_table, const HuffInfo * ac_table, short * prev_dc, FILE * fid)
{
    float input[8 * 8];
    short zz[8 * 8];
    RLEInfo rle[256];
    unsigned int rle_length;

    // Zero-Shift
    zero_shift(block, input);

    // Calculate 2D DCT
    dct2d(input);

    // Quantization 
    quant_zigzag(input, qTable, zz);

    // Zero Run-Length Encode
    zero_rle(zz, rle, &rle_length, prev_dc);

    // DC Huffman Encoding
    encode(rle, 1, dc_table, fid);

    // AC Huffman Encoding
    encode(&rle[1], rle_length - 1, ac_table, fid);
}

//==========================================================================
// Compress a full image
//
// Parameters:
//  channels - The number of channels in the image
//  info     - A pointer to an array that stores the channel information
//  fid      - The output file id
//==========================================================================
void compress_img(unsigned int channels, ChannelInfo * info, FILE * fid)
{
    // Load Huffman Tables
    load_huffman_table(y_dc_codes_per_len, y_dc_values, y_dc_table);
    load_huffman_table(y_ac_codes_per_len, y_ac_values, y_ac_table);

    if (channels > 1)
    {
        load_huffman_table(c_dc_codes_per_len, c_dc_values, c_dc_table);
        load_huffman_table(c_ac_codes_per_len, c_ac_values, c_ac_table);
    }

    // Calcualte number x blocks
    unsigned int xblocks = info[0].width / 8;
    unsigned int yblocks = info[0].height / 8;

    // Previous DC Values
    short y_prev_dc = 0;
    short cb_prev_dc = 0;
    short cr_prev_dc = 0;
    
    // Process Blocks
    if (channels == 1)
    {
        for (unsigned i = 0; i < xblocks * yblocks; i++)
        {
            compress_8x8(&info[0].data[i * 64], yqTable, y_dc_table, y_ac_table, &y_prev_dc, fid);
        }
    }
    else
    {
        for (unsigned int row = 0; row < yblocks; row += 2)
        {
            for (unsigned int col = 0; col < xblocks; col += 2)
            {
                // Process 4 Luminance Blocks
                compress_8x8(&info[0].data[row       * xblocks       * 64 + col       * 64], yqTable, y_dc_table, y_ac_table, &y_prev_dc, fid);
                compress_8x8(&info[0].data[row       * xblocks       * 64 + (col + 1) * 64], yqTable, y_dc_table, y_ac_table, &y_prev_dc, fid);
                compress_8x8(&info[0].data[(row + 1) * xblocks       * 64 + col       * 64], yqTable, y_dc_table, y_ac_table, &y_prev_dc, fid);
                compress_8x8(&info[0].data[(row + 1) * xblocks       * 64 + (col + 1) * 64], yqTable, y_dc_table, y_ac_table, &y_prev_dc, fid);

                // Process 1 Cb Block
                compress_8x8(&info[1].data[(row / 2) * (xblocks / 2) * 64 + (col / 2) * 64], cqTable, c_dc_table, c_ac_table, &cb_prev_dc, fid);

                // Process 1 Cr Block
                compress_8x8(&info[2].data[(row / 2) * (xblocks / 2) * 64 + (col / 2) * 64], cqTable, c_dc_table, c_ac_table, &cr_prev_dc, fid);
            }
        }
    }
}
