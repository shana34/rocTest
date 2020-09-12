/*
Copyright (c) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "helpers.hpp"
#include <hip/hip_runtime.h>
#include <math.h>
#include <rocblas.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

int main(int argc, char** argv)
{
    helpers::ArgParser options("MNKab");
    if(!options.validArgs(argc, argv))
        return EXIT_FAILURE;

    rocblas_status rstatus = rocblas_status_success;
    typedef float dataType;

    rocblas_int M = options.M;
    rocblas_int N = options.N;
    rocblas_int K = options.K;

    float alpha = options.alpha;

    // args start with 7
	rocblas_fill      fillA = (*(argv[7]+9)=='U') ? rocblas_fill_upper:rocblas_fill_lower;
    rocblas_operation tranA = (*(argv[8]+9)=='N') ? rocblas_operation_none:rocblas_operation_transpose;
	rocblas_side      sideX = (*(argv[9]+9)=='L') ? rocblas_side_left:rocblas_side_right;
	rocblas_diagonal  diagA = (*(argv[10]+9)=='U')? rocblas_diagonal_unit:rocblas_diagonal_non_unit;

	printf("fillA : %d, tranA : %d sideX : %d, diagA : %d\n", fillA, tranA, sideX, diagA);

    rocblas_int lda, ldb, sizeA, sizeB;

	lda = M; ldb = M; sizeA = N*lda; sizeB = M*lda;	

    // using rocblas API
    rocblas_handle handle;
    rstatus = rocblas_create_handle(&handle);
    //CHECK_ROCBLAS_STATUS(rstatus);

    // Naming: dX is in GPU (device) memory. hK is in CPU (host) memory

    std::vector<dataType> hA(sizeA, 1);
    std::vector<dataType> hB(sizeB);
    std::vector<dataType> hX(sizeB, 1);

    for(int k = 0; k < sizeA; k++){
		hA[k] = (k % lda) * 1.0;
    }
	int offset = 0;
    for(int k = 0; k < sizeB; k++){
		offset = k/ldb + 1;
		hB[k] = ((M - offset + 1)*(M - offset + 2)/2) * 1.0;
    }
/*
    for(int i = 0; i < lda; i++){
		for(int j = 0; j < ldb; j++){
			printf("A[%d][%d] = %f\t",i,j,hA[i*lda+j]);
		}
		std::cout << '\n'; 
    }
*/
    hX = hB;

    {
        // allocate memory on device
        helpers::DeviceVector<dataType> dA(sizeA);
        helpers::DeviceVector<dataType> dB(sizeB);
        helpers::DeviceVector<dataType> dX(sizeB);

        if(!dA || !dB)
        {
            CHECK_HIP_ERROR(hipErrorOutOfMemory);
            return EXIT_FAILURE;
        }

        // copy data from CPU to device
        CHECK_HIP_ERROR(hipMemcpy(
            dA, static_cast<void*>(hA.data()), sizeof(dataType) * sizeA, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(
            dB, static_cast<void*>(hB.data()), sizeof(dataType) * sizeB, hipMemcpyHostToDevice));

        // enable passing alpha parameter from pointer to host memory
        rstatus = rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host);
        //CHECK_ROCBLAS_STATUS(rstatus);

        // asynchronous calculation on device, returns before finished calculations
        hipEvent_t start, start_1, end, end_1;
        hipEventCreate(&start);
		hipEventCreate(&start_1);
		hipEventCreate(&end);
		hipEventCreate(&end_1);

		hipEventRecord(start_1);
			rstatus = rocblas_strsm(handle, sideX, fillA, tranA, diagA, M, N, &alpha, dA, lda, dB, ldb);
		hipEventRecord(end_1);
		hipEventSynchronize(end_1);
		
		printf("rocblas_status : %d\n", rstatus);
		
		hipEventRecord(start);
			rstatus = rocblas_strsm(handle, sideX, fillA, tranA, diagA, M, N, &alpha, dA, lda, dB, ldb);
		hipEventRecord(end);
		hipEventSynchronize(end);

		printf("rocblas_status : %d\n", rstatus);

		float time_ms   = 0.0f;
		float time_ms_1 = 0.0f;
		hipEventElapsedTime(&time_ms, start, end);
		hipEventElapsedTime(&time_ms_1, start_1, end_1);

		hipEventDestroy(start);
		hipEventDestroy(end);
		hipEventDestroy(start_1);
		hipEventDestroy(end_1);

		// double ops = 2*M*N*K;
		double tflps = 1e-9 * M * K * N/time_ms;
		// check that calculation was launched correctly on device, not that result
		// was computed yet
		//CHECK_ROCBLAS_STATUS(rstatus); 
		std::cout << "Time(s) : first : " << time_ms_1*1e-3 << std::endl;
		std::cout << "Time(s) : " << time_ms*1e-3 << std::endl;
		std::cout << "Time-diff(s) : " << (time_ms_1 - time_ms)*1e-3 << std::endl;
		std::cout << "Tflops  : " << tflps << std::endl;

    } // release device memory via helpers::DeviceVector destructors

    std::cout << "M, N, K, lda, ldb = " << M << ", " << N << ", " << K << ", " << lda << ", "
              << ldb << std::endl;
/*
    // calculate gold standard using CPU
    helpers::matMatMult<dataType>(hAlpha,
                                  hBeta,
                                  M,
                                  N,
                                  K,
                                  hA.data(),
                                  strideA1,
                                  strideA2,
                                  hB.data(),
                                  strideB1,
                                  strideB2,
                                  hGold.data(),
                                  1,
                                  ldc);

    dataType maxRelativeError = helpers::maxRelativeError(hC, hGold);
    dataType eps              = std::numeric_limits<dataType>::epsilon();
    dataType tolerance        = 10;
    if(maxRelativeError > eps * tolerance)
    {
        std::cout << "FAIL";
    }
    else
    {
        std::cout << "PASS";
    }
    std::cout << ": max. relative err. = " << maxRelativeError << std::endl;

    rstatus = rocblas_destroy_handle(handle);
    //CHECK_ROCBLAS_STATUS(rstatus);
*/
    return EXIT_SUCCESS;
}
