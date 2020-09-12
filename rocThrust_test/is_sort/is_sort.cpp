#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"
#include <thrust/device_vector.h>
#include <thrust/sort.h>

int main(int argc, char **argv)
{
	hipEvent_t begin,end;
	hipEventCreate(&begin);
	hipEventCreate(&end);

	std::vector<int> dim;
	dim_gen(0, 1.2e9, dim, atoi(argv[1]));

	int tmp = 2e8;
	for(int i = 0; i < dim.size(); i++)
	{
		float *A = (float*)malloc(2e8 * sizeof(int));
  		thrust::sequence(A, A + tmp);

		thrust::device_vector<float> d_A(A, A + tmp); 

		int cnt = atoi(argv[2]);
		int cnt_1 = cnt;
		
		hipEventRecord(begin);
			while(cnt_1--)
				thrust::is_sorted(d_A.begin(), d_A.end());	
		hipEventRecord(end);
		hipEventSynchronize(begin);
		hipEventSynchronize(end);
		float time = 0.0f;
		hipEventElapsedTime(&time, begin, end);
		printf("Size : %d ; Time cost : %f\n", dim[i], time/(cnt*1e3));
		free(A);
	}
	return 0;
}
