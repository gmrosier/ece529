#include "jpef_file.h"
#include <stdlib.h>

#define MSB(X) ((X >> 8) & 0xFF)
#define LSB(X) (X & 0xFF)

static unsigned char current_byte = 0;
static unsigned char current_bit_cnt = 0;

void write_quantization(FILE * fid)
{
    unsigned char data[5];
    unsigned char qtable[64];
    const unsigned char * read_ptrn = get_read_pattern();
    const unsigned char * q = get_qtable();

    // Set QTable Marker
    data[0] = 0xFF;
    data[1] = 0xDB;
    
    // Set Header Length
    data[2] = 0;
    data[3] = 67;
    
    // Table Info
    data[4] = 0;
    fwrite(data, 1, 5, fid);

    // Write Table
    for (int i = 0; i < 64; i++)
    {
        qtable[read_ptrn[i]] = q[i];
    }
    fwrite(qtable, 1, 64, fid);
}

void write_start_of_frame(FILE * fid)
{
    unsigned char data[13];

    // Start of Frame Marker
    data[0] = 0xFF;
    data[1] = 0xC0;
    
    // Header Length
    data[2] = 0;
    data[3] = 11;
    
    // Sample Precision
    data[4] = 8;
    
    // Number of Lines
    data[5] = MSB(512);
    data[6] = LSB(512);
    
    // Samples Per Line
    data[7] = MSB(512);
    data[8] = LSB(512);
    
    // Components in Frame
    data[9] = 1;
    
    // Component Id
    data[10] = 1;
    
    // Component Samping
    data[11] = 0x11;
    
    // QTable ID
    data[12] = 0;

    // Write Data
    fwrite(data, 1, 13, fid);
}

void write_huffman(FILE * fid, unsigned char isDC)
{
    unsigned char data[5];
    unsigned short len;

    // Huffman Marker
    data[0] = 0xFF;
    data[1] = 0xC4;
    
    // Length
    len = 19 + get_code_count(isDC);
    data[2] = MSB(len);
    data[3] = LSB(len);
    
    // Table Type
    data[4] = (isDC == 1) ? 0 : 0x10;
    fwrite(data, 1, 5, fid);

    // Table
    fwrite(get_code_lens(isDC), 1, 16, fid);
    fwrite(get_code_values(isDC), 1, get_code_count(isDC), fid);
}

void write_scan_header(FILE * fid)
{
    unsigned char data[10];

    // Scan Header Marker
    data[0] = 0xFF;
    data[1] = 0xDA;
    
    // Header Length
    data[2] = 0;
    data[3] = 8;
    
    // Number of Compoents in Scan
    data[4] = 1;
    
    // Component 1 Selector
    data[5] = 1;
    
    // Component 1 Selector
    data[6] = 0;
    
    // Start of Spectral Selection
    data[7] = 0;
    
    // End of Spectral Selection
    data[8] = 63;
    
    // Approximation Bit Positions
    data[9] = 0;
    fwrite(data, 1, 10, fid);
}

FILE * open_stream(const char * file_name, unsigned int height, unsigned int width)
{
    FILE * fid;
    unsigned char data[2];

    // Open File
    fopen_s(&fid, file_name, "wb");
    if (fid == NULL)
    {
        printf("Failed To Open File");
        return NULL;
    }

    // Write Start Of Image Marker
    data[0] = 0xFF;
    data[1] = 0xD8;
    fwrite(&data, 1, 2, fid);
    
    // Write Quantization Table
    write_quantization(fid);

    // Write Start Of Frame
    write_start_of_frame(fid);

    // Write DC Huffman Table
    write_huffman(fid, 1);

    // Write AC Huffman Table
    write_huffman(fid, 0);

    // Write Scan Header
    write_scan_header(fid);

    // Return File Pointer
    return fid;
}

void close_stream(FILE * fid)
{
    unsigned char data[2];

    // Check for Data in Buffer
    if (current_bit_cnt > 0)
    {
        fwrite(&current_byte, 1, 1, fid);
    }

    // Write End Of Image
    data[0] = 0xFF;
    data[1] = 0xD9;
    fwrite(data, 1, 2, fid);

    // Close File
    fclose(fid);
}

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
            unsigned short len_diff = code_length - rem_len;
            unsigned short value = (code >> len_diff) & mask;
            code &= (1 << len_diff) - 1;
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
            if (current_byte == 0xFF)
            {
                current_byte = 0;
                fwrite(&current_byte, 1, 1, fid);
            }
            fflush(fid);
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
