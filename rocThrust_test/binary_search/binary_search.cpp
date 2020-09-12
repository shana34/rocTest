#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thrust/sequence.h>
#include <hip/hip_runtime.h>
#include "helper.hpp"
#include <thrust/device_vector.h>

int main(int argc, char **argv)
{
	hipEvent_t begin,end;
	hipEventCreate(&begin);
	hipEventCreate(&end);


	std::vector<int> dim;
	dim_gen(0, 1.2e9, dim, atoi(argv[1]));

	for(int i = 1; i < dim.size(); i++)
	{
		int *A = (int*)malloc(dim[i] * sizeof(int));
  		thrust::sequence(A, A + dim[i]);
		thrust::device_vector<int> d_A(A, A + dim[i]);	
		int cnt = atoi(argv[2]);
		int cnt_1 = cnt;
		int target[cnt];
			while(cnt_1){
				int c = cnt - (cnt_1);
				target[c] = rand() % dim[i];
				cnt_1 -= 1;
			}
		cnt_1 = cnt;
		// warm up
		thrust::binary_search(d_A.begin(), d_A.end(), target[cnt_1+1]);
		hipEventRecord(begin);
			while(cnt_1--)
				thrust::binary_search(d_A.begin(), d_A.end(), target[cnt_1+1]);

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
