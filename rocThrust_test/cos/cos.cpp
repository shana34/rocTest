#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <thrust/copy.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"
#include <thrust/device_vector.h>

struct is_even
{
	__host__ __device__
	bool operator()(const int x)
	{
		return (x%2) == 0;
	}
};

int main(int argc, char **argv)
{
	hipEvent_t begin,end;
	hipEventCreate(&begin);
	hipEventCreate(&end);


	std::vector<int> dim;
	dim_gen(0, 1e8, dim, atoi(argv[1]));

	for(int i = 1; i < dim.size(); i++)
	{
		int *A = (int*)malloc(dim[i] * sizeof(int));
  		thrust::sequence(A, A + dim[i]);
		int res[dim[i]];
		int cnt = atoi(argv[2]);
		int cnt_1 = cnt;
		
		hipEventRecord(begin);
		hipEventRecord(end);
			while(cnt_1--)
				thrust::copy_if(A, A+dim[i], res, is_even());	
		hipEventSynchronize(begin);
		hipEventSynchronize(end);
		float time = 0.0f;
		hipEventElapsedTime(&time, begin, end);
		printf("Size : %d ; Time cost : %f\n", dim[i], time/(cnt*1e3));
		free(A);
	}
	return 0;
}
