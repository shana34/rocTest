#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"
#include <thrust/device_vector.h>
#include <thrust/inner_product.h>

int main(int argc, char **argv)
{
	hipEvent_t begin,end;
	hipEventCreate(&begin);
	hipEventCreate(&end);


	std::vector<int> dim;
	dim_gen(0, 1.2e8, dim, atoi(argv[1]));

	for(int i = 0; i < dim.size(); i++)
	{
		float *A = (float*)malloc(dim[i] * sizeof(float));
		for(int j = 0; j < dim[i]; j++)
		{
			srand(j);
			A[j] = rand() * 1.0 / RAND_MAX;
		}
		int cnt = atoi(argv[2]);
		int cnt_1 = cnt;
		float res = 0.0f;
		hipEventRecord(begin);
		
			while(cnt_1--){
				res = thrust::inner_product(A, A + dim[i], A, 0.0f);	
				hipDeviceSynchronize();
			}
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
