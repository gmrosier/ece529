#include "encoder.h"
#include "jpeg_file.h"

#include <stdlib.h>
#include <math.h>

#ifndef max
#define max(a, b) ((a > b) ? a : b)
#endif

#ifndef min
#define min(a, b) ((a <= b) ? a : b)
#endif

//const unsigned char vQTable[8 * 8] =
//{
//    16, 11, 10, 16,  24,  40,  51,  61,
//    12, 11, 14, 19,  26,  58,  60,  55,
//    14, 13, 16, 24,  40,  57,  69,  56,
//    14, 17, 22, 29,  41,  87,  80,  62,
//    18, 22, 37, 56,  68, 109, 103,  77,
//    24, 35, 55, 64,  81, 104, 113,  92,
//    49, 64, 78, 87, 103, 121, 120, 101,
//    72, 92, 95, 98, 112, 100, 103,  99
//};

const unsigned char vQTable[8 * 8] =
{
    16, 11, 10, 16,  24,  40,  51,  61,
    12, 12, 14, 19,  26,  58,  60,  55,
    14, 13, 16, 24,  40,  57,  69,  56,
    14, 17, 22, 29,  51,  87,  80,  62,
    18, 22, 37, 56,  68, 109, 103,  77,
    24, 35, 55, 64,  81, 104, 113,  92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103,  99
};

unsigned char qTable[8 * 8];

void init_qtable(float quality_factor)
{
    for (int i = 0; i < 64; i++)
    {
        unsigned int value = ((unsigned int)(vQTable[i] * (quality_factor + 50.0f))) / 100;
        value = max(1, min(255, value));
        qTable[i] = (unsigned char)value;
    }   
}

const unsigned char * get_qtable()
{
    return qTable;
}

const float angle[7] = {
    0.9808f,
    0.9239f,
    0.8315f,
    0.7071f,
    0.5556f,
    0.3827f,
    0.1951f
};

#define C(x) (angle[x-1])
#define S(x) (angle[6-(x-1)])

const unsigned char readPattern[8 * 8] =
{
    0,   1,  5,  6, 14, 15, 27, 28,
    2,   4,  7, 13, 16, 26, 29, 42,
    3,   8, 12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

const unsigned char * get_read_pattern()
{
    return readPattern;
}

typedef struct
{
    unsigned short zero_cnt;
    unsigned short num_bits;
    short value;
} rle_info;


typedef struct
{
    unsigned short value;
    unsigned char length;
} huff_info;

const unsigned char dc_codes_per_len[16] = { 0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
const unsigned char dc_values[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
huff_info dc_table[12];

const unsigned char ac_codes_per_len[16] = { 0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7D };
const unsigned char ac_values[162] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
    0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
    0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};
huff_info ac_table[251];

void load_huffman_table(const unsigned char * codes_per_len, const unsigned char * values, huff_info * table)
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

const unsigned char * get_code_lens(unsigned char isDC)
{
    if (isDC == 1)
        return dc_codes_per_len;

    return ac_codes_per_len;
}

const unsigned char * get_code_values(unsigned char isDC)
{
    if (isDC == 1)
        return dc_values;

    return ac_values;
}

const unsigned int get_code_count(unsigned char isDC)
{
    if (isDC == 1)
        return 12;

    return 162;
}

// 1-D DCT based on Vetterli and Ligtenberg
// Will delay divide by two to quantization
// because once we have an integer the divide
// can be accomplished by a right shift.
void dct1d(float * input, float * output)
{
    // Sum Terms
    float s07 = input[0] + input[7];
    float s12 = input[1] + input[2];
    float s34 = input[3] + input[4];
    float s56 = input[5] + input[6];

    // Difference Terms
    float d07 = input[0] - input[7];
    float d12 = input[1] - input[2];
    float d34 = input[3] - input[4];
    float d56 = input[5] - input[6];

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
    //output[0] = (C(4) * (ss07s34 + ss12s56)) / 2.0f;
    output[0] = (C(4) * (ss07s34 + ss12s56));

    // S1 =  C1 * (d07 + C4mds12s56) - S1 * (-d34 - C4msd12d56)
    //output[1] = (C(1) * (d07 + C4mds12s56) - S(1) * (-d34 - C4msd12d56)) / 2.0f;
    output[1] = (C(1) * (d07 + C4mds12s56) - S(1) * (-d34 - C4msd12d56));

    // S2 =  C6 * (dd12d56) + S6 * (ds07s34)
    //output[2] = (C(6) * dd12d56 + S(6) * ds07s34) / 2.0f;
    output[2] = (C(6) * dd12d56 + S(6) * ds07s34);

    // S3 =  C3 * (d07 - C4 * (ds12s56)) - S3 * (d34 - C4 * (d12 + d56))
    //output[3] = (C(3) * (d07 - C4mds12s56) - S(3) * (d34 - C4msd12d56)) / 2.0f;
    output[3] = (C(3) * (d07 - C4mds12s56) - S(3) * (d34 - C4msd12d56));

    // S4 =  C4 * ((s07 + s34) - (s12 + s56))
    //output[4] = (C(4) * (ss07s34 - ss12s56)) / 2.0f;
    output[4] = (C(4) * (ss07s34 - ss12s56));

    // S5 =  S3 * (d07 - C4 * (s12 - s56)) + C3 * (d34 - C4 * (d12 + d56))
    //output[5] = (S(3) * (d07 - C4mds12s56) + C(3) * (d34 - C4msd12d56)) / 2.0f;
    output[5] = (S(3) * (d07 - C4mds12s56) + C(3) * (d34 - C4msd12d56));

    // S6 = -S6 * (d12 - d56) + C6 * (s07 - s34)
    //output[6] = (-S(6) * dd12d56 + C(6) * ds07s34) / 2.0f;
    output[6] = (-S(6) * dd12d56 + C(6) * ds07s34);

    // S7 =  S1 * (d07 + C4 * (s12 - s56)) + C1 * (-d34 - C4 * (d12 + d56))
    //output[7] = (S(1) * (d07 + C4mds12s56) + C(1) * (-d34 - C4msd12d56)) / 2.0f;
    output[7] = (S(1) * (d07 + C4mds12s56) + C(1) * (-d34 - C4msd12d56));
}

void transpose(float * input, float * output)
{
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            output[c * 8 + r] = input[r * 8 + c];
        }
    }
}

void dct2d(float * input, float * output)
{
    float tmp[8 * 8];

    // Transpose
    transpose(input, tmp);

    // 1D-DCT
    for (int i = 0; i < 8; i++)
    {
        dct1d(&tmp[i * 8], &output[i * 8]);
    }

    // Transpose
    transpose(output, tmp);

    // 1D-DCT
    for (int i = 0; i < 8; i++)
    {
        dct1d(&tmp[i * 8], &output[i * 8]);
    }
}

void zero_shift(unsigned char * input, float * output)
{
    for (int i = 0; i < 8 * 8; i++)
    {
        output[i] = (float)(input[i]) - 128.0f;
    }
}


// Divide the 2D-DCT by the specified quantization table
// We also divide by 4 to take out the 2x scaling in the 1D-DCT
void quantization(float * input, const unsigned char * qTable, short * output)
{
    for (int i = 0; i < 8 * 8; i++)
    {
        output[i] = ((short)roundf(input[i] / (float)qTable[i])) / 4;
    }
}

void zigzag(const short * input, short * output)
{
    for (int i = 0; i < 8 * 8; i++)
    {
        output[readPattern[i]] = input[i];
    }
}

int num_bits(int value)
{
    // Check for Zero
    if (value == 0)
        return 0;

    // Take Absolute Value
    value = abs(value);

    // Find Min Number of Bits Need to Store This Number
    for (int i = 1; i < 8; i++)
    {
        if ((1 << i) > value)
            return i;
    }

    return 8;
}

void zero_rle(short * input, rle_info * output, unsigned int * length, short * prev_dc)
{
    // Get Diff
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
    *length = idx+1;
}

void encode(rle_info * rle, unsigned int rle_length, huff_info * table, FILE * fid)
{
    encode_info item;

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

void compress_8x8(unsigned char * block, short * prev_dc, FILE * fid)
{
    float tmp[8 * 8];
    float input[8 * 8];
    short q[8 * 8];
    short zz[8 * 8];
    rle_info rle[64];
    unsigned int rle_length;

    // Zero-Shift
    zero_shift(block, input);

    // Calculate 2D DCT
    dct2d(input, tmp);

    // Quantization 
    quantization(tmp, qTable, q);

    // Reorder Values
    zigzag(q, zz);

    // Zero Run-Length Encode
    zero_rle(zz, rle, &rle_length, prev_dc);

    // DC Huffman Encoding
    encode(rle, 1, dc_table, fid);

    // AC Huffman Encoding
    encode(&rle[1], rle_length - 1, ac_table, fid);
}

void compress_img(unsigned char * img, unsigned int width, unsigned int height, FILE * fid)
{
    // Load Huffman Tables
    load_huffman_table(dc_codes_per_len, dc_values, dc_table);
    load_huffman_table(ac_codes_per_len, ac_values, ac_table);

    // Calculate Bounds
    unsigned int blocks = (width * height) / (8 * 8);

    // Break Into Blocks
    short prev_dc = 0;
    for (unsigned int i = 0; i < blocks; i++)
    {
        compress_8x8(&img[i*(8 * 8)], &prev_dc, fid);
    }
}
