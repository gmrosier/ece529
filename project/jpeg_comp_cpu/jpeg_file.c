#include "jpeg_file.h"
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
    const unsigned char * q = get_yqtable();

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

void write_start_of_frame(FILE * fid, unsigned short width, unsigned short height)
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
    data[5] = MSB(height);
    data[6] = LSB(height);
    
    // Samples Per Line
    data[7] = MSB(width);
    data[8] = LSB(width);
    
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

FILE * open_stream(const char * file_name, unsigned int width, unsigned int height)
{
    FILE * fid;
    unsigned char data[2];

    // Open File
    fid = fopen(file_name, "wb");
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
    write_start_of_frame(fid, width, height);

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

void write_stream(FILE * fid, const EncodeInfo * item)
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

//================================================================================
// This function reads file with the specified parameters and stores it in the
// following format. This file will also convert a RGB image to YCrCb 4:2:0
// format. This function assumes that the input file is a raw binary format
// that is either 8-bit grayscale or 24-bit (stored in a 32-bit) RGB format.
// For the RGB images this code assumes that the data is stored in local byte
// order and that it looks like the following.
//
//  RGB Pixel Format:
//  ====================================================
//  |    BITS ||31 ... 24|23 ... 16|15 ...  8| 7 ...  0|
//  |---------||---------|---------|---------|---------|
//  |   COLOR ||  UNUSED |     RED |   GREEN |    BLUE |
//  ====================================================
//  
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
//      file_name   - The file to open and read the image from
//      image       - The a pointer to the loaded image
//      width       - The original input width, will return a size that is
//                    divisable by 8.
//      height      - The original input height, will return a size that is
//                    divisable by 8.
//      channels    - The number of channels in the image, the two valid values
//                    are either 1 for grayscale or 3 for 24-bit RGB.
//================================================================================
void file_read(const char * file_name, unsigned int width, unsigned int height,
               unsigned int channels, ChannelInfo * info)
{
    FILE * fid = NULL;
    unsigned int img_size = width * height;

    // Check for Valid # of Channels
    if ((channels != 1) && (channels != 3))
    {
        printf("Invalid # of Channels: %d (1,3 are the only valid options)\n", channels);
        exit(-1);
    }

    // Calculate Bounds
    for (unsigned int i = 0; i < channels; i++)
    {
        unsigned int w_div= 8;
        unsigned int h_div = 8;

        if (i != 0)
        {
            w_div *= 2;
            h_div *= 2;
        }

        info[i].width = ((width + w_div - 1) / w_div) * w_div;
        info[i].height = ((height + h_div - 1) / h_div) * h_div;
        info[i].data = (unsigned char *)malloc(info[i].width * info[i].height);
    }

    // Open File
    fid = fopen(file_name, "rb");
    if (fid == NULL)
    {
        printf("Failed to Open File: %s\n", file_name);
        for (unsigned int i = 0; i < channels; i++)
        {
            free(info[i].data);
        }
        exit(-1);
    }

    // Read File
    unsigned int row = 0;
    unsigned int xblock = 0;
    unsigned int yblock = 0;
    unsigned int rowCnt;

    while (img_size > 0)
    {
        // Read 1 Row of data from a file and store it an array that is orginized by block
        rowCnt = width;
        while (rowCnt > 0)
        {
            unsigned int offset = (yblock * info[0].width * 8) + (xblock * 8 * 8) + (row * 8);
            unsigned int size = min(8, rowCnt);

            if (channels == 1)
            {
                // Handle 8-bit Grayscale Image
                if (fread(&info[0].data[offset], 1, size, fid) != size)
                {
                    printf("Error Reading File\n");
                    for (unsigned int i = 0; i < channels; i++)
                    {
                        free(info[i].data);
                    }
                    exit(-1);
                }
            }
            else
            {
                // Handle 24-bit RGB Image
                unsigned int buffer[8];
                unsigned int elements = 0;
                if ((elements = fread(buffer, 4, size, fid)) != size)
                {
                    printf("Error Reading File: %d != %d\n", size, elements);
                    for (unsigned int i = 0; i < channels; i++)
                    {
                        free(info[i].data);
                    }
                    exit(-1);
                }

                // Convert to YCrCb
                for (unsigned int i = 0; i < size; i++)
                {
                    // Break RGB into individual colors
                    // Assumes that values are stored in local
                    // byte-order.
                    unsigned int r = (buffer[i] >> 16) & 0xFF;
                    unsigned int g = (buffer[i] >> 8) & 0xFF;
                    unsigned int b = buffer[i] & 0xFF;

                    // Handle Y Channel
                    float y = 0.299f * r + 0.587f * g + 0.114f * b;
                    info[0].data[offset + i] = (unsigned char)y;

                    if ((row % 2) == 0)
                    {
                        unsigned int alt = (row % 4) / 2;
                        unsigned int clrOffset = (yblock / 2) * info[1].width * 4 + (yblock % 2) * 8 * 4 + (xblock / 2) * 8 * 8 + (xblock % 2) * 4 + row * 4 + i / 2;
                        if (((i + alt) % 2) == 0)
                        {
                            
                            float cb = 128.0f - 0.168736f * r - 0.331264f * g + 0.5f * b;
                            info[1].data[clrOffset] = (unsigned char)cb;
                        }

                        if (((i + alt + 1) % 2) == 0)
                        {
                            float cr = 128.0f + 0.5f * r - 0.418688f * g - 0.081312f * b;
                            info[2].data[clrOffset] = (unsigned char)cr;
                        }
                    }
                }
            }

            rowCnt -= size;
            xblock++;
        }

        // Subtract Row for Size
        img_size -= width;

        // Reset X Block Idx
        xblock = 0;

        // Increment Row
        row++;
            
        if (row == 8)
        {
            yblock++;
            row = 0;
        }
    }

    // Close File
    fclose(fid);
}
