#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"
#include <thrust/device_vector.h>
#include <thrust/scan.h>

int main(int argc, char **argv)
{
	hipEvent_t begin,end;
	hipEventCreate(&begin);
	hipEventCreate(&end);


	std::vector<int> dim;
	dim_gen(0, 1.2e9, dim, atoi(argv[1]));

	for(int i = 0; i < dim.size(); i++)
	{
		int *A = (int*)malloc(dim[i] * sizeof(int));
  		thrust::sequence(A, A + dim[i]);

		int cnt = atoi(argv[2]);
		int cnt_1 = cnt;
		
		thrust::maximum<int> binary_op;
		hipEventRecord(begin);
			while(cnt_1--)
				thrust::inclusive_scan(A, A + dim[i], A, binary_op);	
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
