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
		//printf("y[%d] [x contribution] = %f\n", i, x_part);

		float y_part = 0.0f;
		for (int k = 0; k < N; ++k)
		{
			if ((i - k - 1) >= 0)
			{
				y_part -= a[k] * y[i - k - 1];
			}
		}
		//printf("y[%d] [y contribution] = %f\n", i, y_part);

		y[i] = x_part + y_part;
		//printf("y[%d] = %f\n\n", i, y[i]);
	}
}

//=============================================================================
//=============================================================================
void conv(float x[], int L, float h[], int M, float y[])
{
}
