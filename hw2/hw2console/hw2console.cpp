// hw2console.cpp : Defines the entry point for the console application.
//
#include <stdlib.h>
#include <stdio.h>

#include "hw2.h"

int main()
{
	float y[15];
	float h[15];
	float c[15];
	float e[15];
	float i[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	float x[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	float b[] = { 0.3375f, 0, -0.3375f };
	float a[] = { -0.2326f, 0.3249f };
	
	filter(b, 3, a, 2, i, 15, h);
	printf("Impulse: ");
	printy(h, 15);
	printf("\n");

	filter(b, 3, a, 2, x, 15, y);
	printf("Step Estimate: ");
	printy(y, 15);
	printf("\n");

	conv(x, 15, h, 5, c);
	printf("Step Conv: ");
	printy(c, 15);
	printf("\n");

	for (int i = 0; i < 15; ++i)
	{
		e[i] = c[i] - y[i];
	}
	printf("Step Error: ");
	printy(e, 15);
	printf("\n");
}


