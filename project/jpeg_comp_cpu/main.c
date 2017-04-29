#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "jpeg_file.h"
#include "encoder.h"

int main(int argc, char * argv[])
{
    TIME_TYPE t1, t2;
    double delta_us;
    unsigned int width;
    unsigned int height;
    unsigned char * image;
    FILE * fid;

    if (argc != 5)
    {
        printf("Usage: %s [input raw file] [width] [height] [output file]\n", argv[0]);
        exit(-1);
    }
    width = atoi(argv[2]);
    height = atoi(argv[3]);
    image = (unsigned char *)malloc(width * height);

    // Read File
    file_read(argv[1], image, width, height);
    
    // Init Q Table
    init_qtable(50.0f);

    // Write Out JPEG
    fid = open_stream(argv[4], width, height);
    if (fid != NULL)
    {
        // Flush Header to Disk
        fflush(fid);

        // Compress
        OS_TIMER_START(t1);
        compress_img(image, width, height, fid);
        OS_TIMER_STOP(t2);
        OS_TIMER_CALC(t1, t2, delta_us);
        printf("Image Compression Too %.3f us\n", delta_us);

        // Close File
        close_stream(fid);
    }

    // Clean Up
    free(image);

}
