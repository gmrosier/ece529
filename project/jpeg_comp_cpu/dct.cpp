#include <stdio.h>
#include <cmath>

const unsigned int vQTable[8][8] =
{
   { 16, 11, 10, 16,  24,  40,  51,  61 },
   { 12, 11, 14, 19,  26,  58,  60,  55 },
   { 14, 13, 16, 24,  40,  57,  69,  56 },
   { 14, 17, 22, 29,  41,  87,  80,  62 },
   { 18, 22, 37, 56,  68, 109, 103,  77 },
   { 24, 35, 55, 64,  81, 104, 113,  92 },
   { 49, 64, 78, 87, 103, 121, 120, 101 },
   { 72, 92, 95, 98, 112, 100, 103,  99 }
};

const float angle[7] = {
   0.9808f,
   0.9239f,
   0.8315f,
   0.7071f,
   0.5556f,
   0.3827f,
   0.1951f
};

const unsigned int readPattern[8 * 8] =
{
   0,   1,  5,  6, 14, 15, 27, 28,
   2,   4,  7, 13, 16, 26, 29, 42,
   3,   8, 12, 17, 25, 30, 41, 43,
   9,  11, 18, 24, 31, 40, 44, 53,
   10, 19, 23, 32, 39, 45, 52, 54,
   20, 22, 33, 38, 46, 51, 55, 60,
   21, 34, 37, 47, 50, 56, 59, 61,
   35, 36, 48, 49, 57, 58, 62, 63
}

#define C(x) (angle[x-1])
#define S(x) (angle[6-(x-1)])

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
   output[0] = (C(4) * (ss07s34 + ss12s56)) / 2.0f;

   // S1 =  C1 * (d07 + C4mds12s56) - S1 * (-d34 - C4msd12d56)
   output[1] = (C(1) * (d07 + C4mds12s56) - S(1) * (-d34 - C4msd12d56)) / 2.0f;

   // S2 =  C6 * (dd12d56) + S6 * (ds07s34)
   output[2] = (C(6) * dd12d56 + S(6) * ds07s34) / 2.0f;

   // S3 =  C3 * (d07 - C4 * (ds12s56)) - S3 * (d34 - C4 * (d12 + d56))
   output[3] = (C(3) * (d07 - C4mds12s56) - S(3) * (d34 - C4msd12d56)) / 2.0f;

   // S4 =  C4 * ((s07 + s34) - (s12 + s56))
   output[4] = (C(4) * (ss07s34 - ss12s56)) / 2.0f;

   // S5 =  S3 * (d07 - C4 * (s12 - s56)) + C3 * (d34 - C4 * (d12 + d56))
   output[5] = (S(3) * (d07 - C4mds12s56) + C(3) * (d34 - C4msd12d56)) / 2.0f;

   // S6 = -S6 * (d12 - d56) + C6 * (s07 - s34)
   output[6] = (-S(6) * dd12d56 + C(6) * ds07s34) / 2.0f;

   // S7 =  S1 * (d07 + C4 * (s12 - s56)) + C1 * (-d34 - C4 * (d12 + d56))
   output[7] = (S(1) * (d07 + C4mds12s56) + C(1) * (-d34 - C4msd12d56)) / 2.0f;
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
      output[i] = static_cast<float>(input[i]) - 127.0f;
   }
}

void quantization(float * input, const unsigned int * qTable, int * output)
{
   for (int i = 0; i < 8 * 8; i++)
   {
      output[i] = static_cast<int>(round((input[i] + static_cast<float>(qTable[i] >> 1))
         / static_cast<float>(qTable[i])));
   }
}

void encode(int * input, )
int main(int argc, char * argv[])
{
   unsigned char img[8][8] = 
   {
      { 1, 2, 3, 4, 5, 6, 7, 8 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 1, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 1 }
   };

   float tmp[8][8];
   float input[8][8];
   int output[8][8];

   // Zero-Shift
   zero_shift((unsigned char *)img, (float *)input);

   // Calculate 2D DCT
   dct2d((float *)input, (float *)tmp);

   // Quantization 
   quantization((float *)tmp, (const unsigned int *)vQTable, (int *)output);

   // Print Results
   printf("\nOutput:\n");
   for (int i = 0; i < 8; i++)
   {
      printf(" %4d %4d %4d %4d %4d %4d %4d %4d\n",
         output[i][0], output[i][1], output[i][2], output[i][3],
         output[i][4], output[i][5], output[i][6], output[i][7]);
   }
}