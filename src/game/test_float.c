#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef union {
	float f;
	int i;
} float_int_u;

typedef union {
	float f;
	long l;
} float_long_u;

float Q_fabs(float f) {
	int tmp = (*(int *)&f) & 0x7FFFFFFF;

	return *(float *)&tmp;
}

float Q_fabs2(float f) {
	float_int_u f_u = {f};
	float_int_u tmp_u;

	tmp_u.i = (*(int *)&f_u.i) & 0x7FFFFFFF;

	return *(float *)&tmp_u.f;
}

int main() {
	int i = 0;
	float r;
	float res1, res2;

	for (; i < 100000000; ++i) {
		r = (float)rand()/((float)RAND_MAX/100000.0);
		res1 = Q_fabs(r);
		res2 = Q_fabs2(r);
		if (res1 != res2) {
			printf("Error: %f: %f != %f\n", r, res1, res2);
			break;
		}
	}
	return 0;
}