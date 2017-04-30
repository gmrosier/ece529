#include "jpeg_file.h"
#include <stdlib.h>
#include <math.h>

#define MSB(X) ((X >> 8) & 0xFF)
#define LSB(X) (X & 0xFF)

static unsigned char current_byte = 0;
static unsigned char current_bit_cnt = 0;

void write_quantization(FILE * fid, unsigned int channels)
{
    unsigned char data[4];
    unsigned char qtable[64];
    const unsigned char * read_ptrn = get_read_pattern();
    const unsigned char * yq = get_yqtable();
    const unsigned char * cq = get_cqtable();
    unsigned int length = 2 + (64 + 1);

    if (channels > 1)
        length += (64 + 1);

    // Set QTable Marker
    data[0] = 0xFF;
    data[1] = 0xDB;
    
    // Set Header Length
    data[2] = MSB(length);
    data[3] = LSB(length);
    fwrite(data, 1, 4, fid);

    // Luminance Table Info
    data[0] = 0;
    fwrite(data, 1, 1, fid);

    // Write Luminance Table
    for (int i = 0; i < 64; i++)
    {
        qtable[read_ptrn[i]] = yq[i];
    }
    fwrite(qtable, 1, 64, fid);

    if (channels > 1)
    {
        // Chrominance Table Info
        data[0] = 1;
        fwrite(data, 1, 1, fid);

        // Write Chrominance Table
        for (int i = 0; i < 64; i++)
        {
            qtable[read_ptrn[i]] = cq[i];
        }
        fwrite(qtable, 1, 64, fid);
    }
}

void write_start_of_frame(FILE * fid, unsigned int width, unsigned int height, ChannelInfo * info, unsigned int channels)
{
    unsigned short length = 8 + (3 * channels);
    unsigned char data[20];

    // Start of Frame Marker
    data[0] = 0xFF;
    data[1] = 0xC0;
    
    // Header Length
    
    data[2] = MSB(length);
    data[3] = LSB(length);
    
    // Sample Precision
    data[4] = 8;
    
    // Number of Lines
    data[5] = MSB(height);
    data[6] = LSB(height);
    
    // Samples Per Line
    data[7] = MSB(width);
    data[8] = LSB(width);
    
    // Components in Frame
    data[9] = channels;
    
    for (unsigned int i = 0; i < channels; i++)
    {
        // Component Id
        data[i*3 + 10] = (unsigned char)(i + 1);

        // Channel 0
        if (i == 0)
        {
            if (channels == 1)
            {
                // Component Samping
                data[i * 3 + 11] = 0x11;
            }
            else
            {
                // Component Samping
                data[i * 3 + 11] = 0x22;
            }

            // QTable ID
            data[i * 3 + 12] = 0;
        }
        else
        {
            // Component Samping
            data[i * 3 + 11] = 0x11;

            // QTable ID
            data[i * 3 + 12] = 1;
        }
    }

    // Write Data
    fwrite(data, 1, length+2, fid);
}

void write_huffman(FILE * fid, unsigned char id, unsigned int code_cnt, const unsigned char * lengths, const unsigned char * values)
{
    unsigned char data[5];
    unsigned short len;

    // Huffman Marker
    data[0] = 0xFF;
    data[1] = 0xC4;
    
    // Length
    len = 2 + 1 + 16 + code_cnt;
    data[2] = MSB(len);
    data[3] = LSB(len);
    
    // ID
    data[4] = id;
    fwrite(data, 1, 5, fid);

    // Write Lengths
    fwrite(lengths, 1, 16, fid);

    // Write Values
    fwrite(values, 1, code_cnt, fid);
}

void write_scan_header(FILE * fid, unsigned int channels)
{
    unsigned char data[5];
    unsigned short len = 6 + 2 * channels;

    // Scan Header Marker
    data[0] = 0xFF;
    data[1] = 0xDA;
    
    // Header Length
    data[2] = MSB(len);
    data[3] = LSB(len);
    
    // Number of Compoents in Scan
    data[4] = channels;
    fwrite(data, 1, 5, fid);

    for (unsigned int i = 0; i < channels; i++)
    {
        // Scan Component Selector
        data[0] = i + 1;

        // Hufman Table
        if (i == 0)
            data[1] = 0x00;
        else
            data[1] = 0x11;

        fwrite(data, 1, 2, fid);
    }

    // Start of Spectral Selection
    data[0] = 0;
    
    // End of Spectral Selection
    data[1] = 63;
    
    // Approximation Bit Positions
    data[2] = 0;
    fwrite(data, 1, 3, fid);
}

FILE * open_stream(const char * file_name, unsigned int width, unsigned int height, ChannelInfo * info, unsigned int channels)
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
    
    // Write Quantization Tables
    write_quantization(fid, channels);

    // Write Start Of Frame
    write_start_of_frame(fid, width, height, info, channels);

    // Write Huffman Tables
    write_huffman(fid, 0x00, get_code_count(1), get_code_lens(1, 0), get_code_values(1, 0));
    write_huffman(fid, 0x10, get_code_count(0), get_code_lens(0, 0), get_code_values(0, 0));

    if (channels > 1)
    {
        write_huffman(fid, 0x01, get_code_count(1), get_code_lens(1, 1), get_code_values(1, 1));
        write_huffman(fid, 0x11, get_code_count(0), get_code_lens(0, 1), get_code_values(0, 1));
    }

    // Write Scan Header
    write_scan_header(fid, channels);

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
            
            // Check if the byte we currently
            // outputed was 0xFF and since 
            // 0xFF is a control code we need to
            // output a 0x00 in-order to let the
            // decoder know that it was data not
            // control, see Annex F - Section F.1.2.3
            // of ISO DIS 10918-1
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
    unsigned int aWidth = 0;
    unsigned int aHeight = 0;
    for (unsigned int i = 0; i < channels; i++)
    {
        unsigned int w_div= 8;
        unsigned int h_div = 8;

        if (i != 0)
        {
            w_div *= 2;
            h_div *= 2;
        }

        aWidth = max(aWidth, ((width + w_div - 1) / w_div) * w_div);
        aHeight = max(aHeight, ((height + h_div - 1) / h_div) * h_div);
    }

    // Allocate Memory
    for (unsigned int i = 0; i < channels; i++)
    {
        info[i].width = aWidth;
        info[i].height = aHeight;

        if (i != 0)
        {
            info[i].width /= 2;
            info[i].height /= 2;
        }

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
                        unsigned int clrOffset = ((yblock / 2) * info[1].width * 8) + (yblock % 2) * 8 * 4 + (xblock / 2) * 8 * 8 + (xblock % 2) * 4 + row * 4 + i / 2;
                        if (((i + alt) % 2) == 0)
                        {
                            
                            float cb = roundf(128.0f - 0.168736f * r - 0.331264f * g + 0.5f * b);
                            info[1].data[clrOffset] = (unsigned char)cb;
                        }

                        if (((i + alt + 1) % 2) == 0)
                        {
                            float cr = roundf(128.0f + 0.5f * r - 0.418688f * g - 0.081312f * b);
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
