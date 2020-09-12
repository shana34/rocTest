#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"

int main(int argc, char **argv)
{
	hipEvent_t begin,end;
	hipEventCreate(&begin);
	hipEventCreate(&end);


	std::vector<int> dim;
	dim_gen(0, 1e9, dim, atoi(argv[1]));

	for(int i = 0; i < dim.size(); i++)
	{
		int *A = (int*)malloc(dim[i] * sizeof(int));
		hipEventRecord(begin);
  			thrust::sequence(A, A + dim[i]);
		hipEventRecord(end);
		hipEventSynchronize(begin);
		hipEventSynchronize(end);
		float time = 0.0f;
		hipEventElapsedTime(&time, begin, end);
		printf("Size : %d ; Time cost : %f\n",dim[i], time/1e3);
		free(A);
	}
	return 0;
}
