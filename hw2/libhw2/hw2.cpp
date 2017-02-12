#include "hw2.h"
#include <stdio.h>

//=============================================================================
//=============================================================================
void filter(float b[], int M, float a[], int N, float x[], int L, float y[])
{
	for (int i = 0; i < L; ++i)
	{
		float x_part = 0.0f;
		for (int k = 0; k < M; ++k)
		{
			if ((i - k) >= 0)
			{
				x_part += b[k] * x[i - k];
			}
		}

		float y_part = 0.0f;
		for (int k = 0; k < N; ++k)
		{
			if ((i - k - 1) >= 0)
			{
				y_part -= a[k] * y[i - k - 1];
			}
		}

		y[i] = x_part + y_part;
	}
}

//=============================================================================
//=============================================================================
void conv(float x[], int L, float h[], int M, float y[])
{
	for (int i = 0; i < L; ++i)
	{
		y[i] = 0;
		for (int k = 0; k < M; ++k)
		{
			if ((i - k) >= 0)
			{
				y[i] += h[k] * x[i - k];
			}
		}
	}
}

//=============================================================================
//=============================================================================
void printy(float y[], int length)
{
	printf("[");
	if (length > 0)
	{
		printf("%f", y[0]);
		for (int i = 1; i < length; ++i)
		{
			printf(", %f", y[i]);
		}
	}
	printf("]\n");
}
