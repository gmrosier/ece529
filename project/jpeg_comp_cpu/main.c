#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpeg_file.h"
#include "encoder.h"

int main(int argc, char * argv[])
{
    unsigned int bit_len = 512 * 512 * 16;
    unsigned int image_size_bytes = 512 * 512;
    unsigned char * image = (unsigned char *)malloc(image_size_bytes);
    FILE * fid;

    // Read File
    file_read("lena_gray.raw", image, 512, 512);
    
    // Init Q Table
    init_qtable(50.0f);

    // Write Out JPEG
    fid = open_stream("lena_gray.jpg", 512, 512);
    if (fid != NULL)
    {
        // Flush Header to Disk
        fflush(fid);

        // Compress
        compress_img(image, 512, 512, fid);

        // Close File
        close_stream(fid);
    }

    // Clean Up
    free(image);

}
