#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"
#include <thrust/adjacent_difference.h>
#include <thrust/device_vector.h>

int main(int argc, char **argv)
{
	hipEvent_t begin,end;
	hipEventCreate(&begin);
	hipEventCreate(&end);


	std::vector<int> dim;
	dim_gen(0, 1e8, dim, atoi(argv[1]));

	for(int i = 0; i < dim.size(); i++)
	{
		int *A = (int*)malloc(dim[i] * sizeof(int));
  		thrust::sequence(A, A + dim[i]);
		thrust::device_vector<int> d_A(A, A + dim[i]);
		thrust::device_vector<int> d_res(dim[i]);

		hipEventRecord(begin);
		hipEventRecord(end);
			thrust::adjacent_difference(d_A.begin(), d_A.end(), d_res.begin());	
		hipEventSynchronize(begin);
		hipEventSynchronize(end);
		float time = 0.0f;
		hipEventElapsedTime(&time, begin, end);
		printf("Size : %d ; Time cost : %f\n",dim[i], time/1e3);
		free(A);
	}
	return 0;
}
