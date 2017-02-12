// hw2console.cpp : Defines the entry point for the console application.
//
#include <stdlib.h>
#include <stdio.h>

#include "hw2.h"

int main()
{
	float y[15];
	float x[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	float b[] = { 0.3375f, 0, -0.3375f };
	float a[] = { -0.2326f, 0.3249f };

	filter(b, 3, a, 2, x, 15, y);
	printf("[ %f", y[0]);
	for (int i = 1; i < 15; ++i)
	{
		printf(", %f", y[i]);
	}
	printf("]\n");
}

