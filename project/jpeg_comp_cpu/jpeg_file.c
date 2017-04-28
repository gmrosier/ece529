#include "jpef_file.h"
#include <stdlib.h>

void write_quantization(FILE * fid)
{
    unsigned short sdata;
    unsigned char * cdata = (unsigned char *)&sdata;
    const unsigned char * qtable;

    // Write QTable Marker
    sdata = 0xFFDB;
    fwrite(&sdata, 2, 1, fid);

    // Header Length
    sdata = 67;
    fwrite(&sdata, 2, 1, fid);

    // Table Info
    cdata[0] = 0;
    fwrite(cdata, 1, 1, fid);

    // Write Table
    qtable = get_qtable();
    fwrite(qtable, 1, 65, fid);
}

void write_start_of_frame(FILE * fid)
{
    unsigned short sdata;
    unsigned char * cdata = (unsigned char *)&sdata;

    // Write Start of Frame Marker
    sdata = 0xFFC0;
    fwrite(&sdata, 2, 1, fid);

    // Write Header Length
    sdata = 11;
    fwrite(&sdata, 2, 1, fid);

    // Write Sample Precision
    cdata[0] = 8;
    fwrite(cdata, 1, 1, fid);

    // Write Number of Lines
    sdata = 512;
    fwrite(&sdata, 2, 1, fid);

    // Write Samples Per Line
    sdata = 512;
    fwrite(&sdata, 2, 1, fid);

    // Write Components in Frame
    cdata[0] = 1;
    fwrite(cdata, 1, 1, fid);

    // Write Component Id
    cdata[0] = 1;
    fwrite(cdata, 1, 1, fid);

    // Write Component Samping
    cdata[0] = 0x11;
    fwrite(cdata, 1, 1, fid);

    // Write QTable ID
    cdata[0] = 0;
    fwrite(cdata, 1, 1, fid);
}

void write_huffman(FILE * fid)
{
    unsigned short sdata;
    unsigned char * cdata = (unsigned char *)&sdata;

    // Write Huffman Marker
    sdata = 0xFFC4;
    fwrite(&sdata, 2, 1, fid);

    // Write Length
    sdata = 6 + 34 + get_code_count(1) + get_code_count(0);
    fwrite(&sdata, 2, 1, fid);

    // Write DC Table
    cdata[0] = 0;
    fwrite(cdata, 1, 1, fid);
    fwrite(get_code_lens(1), 1, 16, fid);
    fwrite(get_code_values(1), 1, get_code_count(1), fid);

    // Write AC Table
    cdata[0] = 0x10;
    fwrite(cdata, 1, 1, fid);
    fwrite(get_code_lens(0), 1, 16, fid);
    fwrite(get_code_values(0), 1, get_code_count(0), fid);
}

void write_scan_header(FILE * fid)
{
    unsigned short sdata;
    unsigned char * cdata = (unsigned char *)&sdata;

    // Write Scan Header Marker
    sdata = 0xFFDA;
    fwrite(&sdata, 2, 1, fid);

    // Write Header Length
    sdata = 8;
    fwrite(&sdata, 2, 1, fid);

    // Write Number of Compoents in Scan
    cdata[0] = 1;
    fwrite(cdata, 1, 1, fid);

    // Write Component 1 Selector
    cdata[0] = 1;
    fwrite(cdata, 1, 1, fid);

    // Write Component 1 Selector
    cdata[0] = 0;
    fwrite(cdata, 1, 1, fid);

    // Write Start of Spectral Selection
    cdata[0] = 0;
    fwrite(cdata, 1, 1, fid);

    // Write End of Spectral Selection
    cdata[0] = 63;
    fwrite(cdata, 1, 1, fid);

    // Write Approximation Bit Positions
    cdata[0] = 0;
    fwrite(cdata, 1, 1, fid);
}

FILE * open_stream(const char * file_name, unsigned int height, unsigned int width)
{
    FILE * fid;
    
    // Open File
    fopen_s(&fid, file_name, "wb");
    if (fid == NULL)
    {
        printf("Failed To Open File");
        return NULL;
    }

    // Write Start Of Image Marker
    unsigned short sdata = 0xFFD8;
    fwrite(&sdata, 2, 1, fid);
    
    // Write Quantization Table
    write_quantization(fid);

    // Write Start Of Frame
    write_start_of_frame(fid);

    // Write Huffman Table
    write_huffman(fid);

    // Write Scan Header
    write_scan_header(fid);

    // Return File Pointer
    return fid;
}

void close_stream(FILE * fid)
{
    unsigned short sdata;

    // Write End Of Image
    sdata = 0xFFD9;
    fwrite(&sdata, 2, 1, fid);

    // Close File
    fclose(fid);
}


static unsigned char current_byte = 0;
static unsigned char current_bit_cnt = 0;

void write_stream(FILE * fid, const encode_info * item)
{
    unsigned int code_length = item->length + item->add_length;
    unsigned int code = (item->value & ((1 << item->length) - 1)) << item->add_length;
    code += item->additional & ((1 << item->add_length) - 1);
    
    while (code_length > 0)
    {
        unsigned int rem_len = 8 - current_bit_cnt;
        if (code_length > rem_len)
        {
            unsigned short mask = ((1 << rem_len) - 1);
            unsigned short value = (code >> (code_length - rem_len)) & mask;
            current_byte += (unsigned char)value;
            current_bit_cnt = 8;
            code_length -= rem_len;
        }
        else
        {
            unsigned short mask = ((1 << code_length) - 1);
            unsigned short value = (code & mask) << (rem_len - code_length);
            current_byte += (unsigned char)value;
            current_bit_cnt += code_length;
            code_length = 0;
        }

        if (current_bit_cnt == 8)
        {
            fwrite(&current_byte, 1, 1, fid);
            current_byte = 0;
            current_bit_cnt = 0;
        }
    }
}

void file_read(const char * file_name, unsigned char * image, unsigned int width, unsigned int height)
{
    FILE * fid = NULL;

    // Calculate Bounds
    unsigned int img_size = width * height;
    unsigned int xblocks = width / 8;

    // Open File
    fopen_s(&fid, file_name, "rb");
    if (fid == NULL)
    {
        printf("Failed to Open File: %s\n", file_name);
        exit(-1);
    }

    // Read File
    unsigned int row = 0;
    unsigned int xblock = 0;
    unsigned int yblock = 0;
    unsigned int offset;

    while (img_size > 0)
    {
        offset = (yblock * xblocks * 8 * 8) + (xblock * 8 * 8) + (row * 8);
        if (fread(&image[offset], 1, 8, fid) != 8)
        {
            printf("Error Reading File\n");
            fclose(fid);
            exit(-2);
        }

        img_size -= 8;
        xblock++;

        if (xblock == xblocks)
        {
            row++;
            xblock = 0;

            if (row == 8)
            {
                row = 0;
                yblock++;
            }
        }

    }

    // Close File
    fclose(fid);
}
