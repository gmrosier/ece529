#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpeg_file.h"
#include "encoder.h"

int main(int argc, char * argv[])
{
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    ChannelInfo info[3];
    FILE * fid;
    float quality_factor = 50.0f;

    // Process Command Line Arguments
    if ((argc != 6) && (argc != 7))
    {
        printf("Usage: %s [input raw file] [width] [height] [channels] [output file] [quality factor]\n", argv[0]);
        printf("Required:\n");
        printf("   input raw file    - Input Image File\n");
        printf("   width             - Input Image Width (Integer)\n");
        printf("   height            - Input Image Height (Integer)\n");
        printf("   channels          - Input Image Channel Count (Integer)\n");
        printf("   output file       - Output JPEG File\n\n");
        printf("Optional:\n");
        printf("   quality factor    - Image Quality Factor (Integer) [Default = 50]\n\n");
        exit(-1);
    }
    width = atoi(argv[2]);
    height = atoi(argv[3]);
    channels = atoi(argv[4]);

    if (argc == 7)
    {
        quality_factor = (float)atoi(argv[6]);
    }

    // Read File
    file_read(argv[1], width, height, channels, info);
    
    // Init Q Table
    init_qtable(quality_factor);

    // Write Out JPEG
    fid = open_stream(argv[5], width, height);
    if (fid != NULL)
    {
        // Flush Header to Disk
        fflush(fid);

        // Compress
        compress_img(info[0].data, channels, info, fid);

        // Close File
        close_stream(fid);
    }

    // Clean Up
    for (unsigned int i = 0; i < channels; i++)
    {
        free(info[i].data);
    }

}
