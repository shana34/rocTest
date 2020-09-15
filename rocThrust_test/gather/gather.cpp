#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"
#include <thrust/device_vector.h>
#include <thrust/gather.h>

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

		for(int j = 0; j < dim[i]/2; j++)
		{
			A[j] = 2*j;
			A[dim[i]/2+j] = j + 1;
		}
		int cnt = atoi(argv[2]);
		int cnt_1 = cnt;
		thrust::device_vector<int> d_map(A, A + dim[i]);
		thrust::device_vector<int> d_out(dim[i]);
		hipEventRecord(begin);
			while(cnt_1--){
				thrust::gather(d_map.begin(), d_map.end(), d_A.begin(), d_out.begin());	
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
