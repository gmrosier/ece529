#include "encoder.h"

#include <stdlib.h>
#include <math.h>

#include "tables.h"
#include "jpeg_file.h"

unsigned char yqTable[8 * 8];
unsigned char cqTable[8 * 8];

void init_qtable(float quality_factor)
{
    for (int i = 0; i < 64; i++)
    {
        unsigned int valuey = ((unsigned int)(y_qTable[i] * (quality_factor + 50.0f))) / 100;
        valuey = max(1, min(255, valuey));
        yqTable[i] = (unsigned char)valuey;

        unsigned int valuec = ((unsigned int)(cr_qTable[i] * (quality_factor + 50.0f))) / 100;
        valuec = max(1, min(255, valuec));
        cqTable[i] = (unsigned char)valuec;
    }
}

const unsigned char * get_yqtable()
{
    return yqTable;
}

const unsigned char * get_cqtable()
{
    return cqTable;
}

const unsigned char * get_read_pattern()
{
    return output_pattern;
}

HuffInfo dc_table[12];
HuffInfo ac_table[251];

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

const unsigned char * get_code_lens(unsigned char isDC)
{
    if (isDC == 1)
        return y_dc_codes_per_len;

    return y_ac_codes_per_len;
}

const unsigned char * get_code_values(unsigned char isDC)
{
    if (isDC == 1)
        return y_dc_values;

    return y_ac_values;
}

const unsigned int get_code_count(unsigned char isDC)
{
    if (isDC == 1)
        return 12;

    return 162;
}

// Row based 1-D DCT.
//  1-D DCT based on Vetterli and Ligtenberg
//  but we will delay divide by two to quantization
//  because once we have an integer the divide
//  can be accomplished by 1 divide instead of two
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

// Column based 1-D DCT.
//  1-D DCT based on Vetterli and Ligtenberg
//  but we will delay divide by two to quantization
//  because once we have an integer the divide
//  can be accomplished by 1 divide instead of two
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

// Wrapper Function to call both the
// row and column based 1D-DCTs.
void dct2d(float * block)
{
    // Row 1D-DCT
    rdct1d(block);

    // Column 1D-DCT
    cdct1d(block);
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
// We also store the data in the zig-zag order
void quant_zigzag(float * input, const unsigned char * qTable, short * output)
{
    for (int i = 0; i < 8 * 8; i++)
    {
        output[output_pattern[i]] = ((short)roundf(input[i] / (float)qTable[i])) / 4;
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

// Scan the 8x8 and convert it to a run length encoded stram of values
void zero_rle(short * input, RLEInfo * output, unsigned int * length, short * prev_dc)
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
    *length = idx + 1;
}

// Take the Run Length Encoding and encode it to a binary file using 
// the Huffman codes provided in the table.
void encode(RLEInfo * rle, unsigned int rle_length, HuffInfo * table, FILE * fid)
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

// Compress an 8x8 Block
void compress_8x8(unsigned char * block, short * prev_dc, FILE * fid)
{
    float input[8 * 8];
    short zz[8 * 8];
    RLEInfo rle[64];
    unsigned int rle_length;

    // Zero-Shift
    zero_shift(block, input);

    // Calculate 2D DCT
    dct2d(input);

    // Quantization 
    quant_zigzag(input, yqTable, zz);

    // Zero Run-Length Encode
    zero_rle(zz, rle, &rle_length, prev_dc);

    // DC Huffman Encoding
    encode(rle, 1, dc_table, fid);

    // AC Huffman Encoding
    encode(&rle[1], rle_length - 1, ac_table, fid);
}

// Compress A Full Image
void compress_img(unsigned char * img, unsigned int channels, ChannelInfo * info, FILE * fid)
{
    // Load Huffman Tables
    load_huffman_table(y_dc_codes_per_len, y_dc_values, dc_table);
    load_huffman_table(y_ac_codes_per_len, y_ac_values, ac_table);

    // Calculate Bounds
    unsigned int blocks = (info[0].width * info[0].height) / (8 * 8);

    // Break Into Blocks
    short prev_dc = 0;
    for (unsigned int i = 0; i < blocks; i++)
    {
        compress_8x8(&img[i*(8 * 8)], &prev_dc, fid);
    }
}
