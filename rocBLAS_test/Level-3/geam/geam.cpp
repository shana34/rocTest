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
    float beta  = options.beta;
	
    rocblas_operation tranA = (*(argv[7]+9)=='N')? rocblas_operation_none:rocblas_operation_transpose;
    rocblas_operation tranB = (*(argv[8]+9)=='N')? rocblas_operation_none:rocblas_operation_transpose;

	printf("transA : %d, transB : %d\n", tranA, tranB);

    rocblas_int lda, ldb, ldc, sizeA, sizeB, sizeC;
	rocblas_int strideA1, strideA2, strideB1, strideB2;

    if(tranA == rocblas_operation_none)
    {
        lda      = M;
        sizeA    = K * lda;
        strideA1 = 1;
        strideA2 = lda;
    }
    else
    {
        lda      = K;
        sizeA    = M * lda;
        strideA1 = lda;
        strideA2 = 1;
    }
    if(tranB == rocblas_operation_none)
    {
        ldb      = K;
        sizeB    = N * ldb;
        strideB1 = 1;
        strideB2 = ldb;
    }
    else
    {
        ldb      = N;
        sizeB    = K * ldb;
        strideB1 = ldb;
        strideB2 = 1;
    }
    ldc   = M;
    sizeC = N * ldc;

    // using rocblas API
    rocblas_handle handle;
    rstatus = rocblas_create_handle(&handle);
    //CHECK_ROCBLAS_STATUS(rstatus);

    // Naming: dX is in GPU (device) memory. hK is in CPU (host) memory

    std::vector<dataType> hA(sizeA);
    std::vector<dataType> hB(sizeB);
    std::vector<dataType> hC(sizeC, 0);

    for(int k = 0; k < sizeA; k++){
		hA[k] = rand()*1.0 / RAND_MAX ;
    }
    for(int k = 0; k < sizeB; k++){
		hB[k] = rand()*1.0 / RAND_MAX ;
    }
    
/*
    for(int i = 0; i < lda; i++){
	for(int j = 0; j < ldb; j++){
		printf("A[%d][%d] = %f \n",i,j,hA[i*lda+j]);
	}
    }
*/

    {
        // allocate memory on device
        helpers::DeviceVector<dataType> dA(sizeA);
        helpers::DeviceVector<dataType> dB(sizeB);
        helpers::DeviceVector<dataType> dC(sizeC);

        if(!dA || !dB || !dC)
        {
            CHECK_HIP_ERROR(hipErrorOutOfMemory);
            return EXIT_FAILURE;
        }

        // copy data from CPU to device
        CHECK_HIP_ERROR(hipMemcpy(
            dA, static_cast<void*>(hA.data()), sizeof(dataType) * sizeA, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(
            dB, static_cast<void*>(hB.data()), sizeof(dataType) * sizeB, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(
            dC, static_cast<void*>(hC.data()), sizeof(dataType) * sizeC, hipMemcpyHostToDevice));

        // enable passing alpha parameter from pointer to host memory
        // rstatus = rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host);
        // CHECK_ROCBLAS_STATUS(rstatus);

        // asynchronous calculation on device, returns before finished calculations
        hipEvent_t start, start_1, end, end_1;
        hipEventCreate(&start);
		hipEventCreate(&start_1);
		hipEventCreate(&end);
		hipEventCreate(&end_1);

		hipEventRecord(start_1);
			rocblas_sgeam(handle, tranA, tranB, M, N, &alpha, dA, lda, &beta, dB, ldb, dC, ldc);
		hipEventRecord(end_1);
		hipEventSynchronize(end_1);

		hipEventRecord(start);
			rocblas_sgeam(handle, tranA, tranB, M, N, &alpha, dA, lda, &beta, dB, ldb, dC, ldc);
		hipEventRecord(end);
		hipEventSynchronize(end);
		float time_ms   = 0.0f;
		float time_ms_1 = 0.0f;
		hipEventElapsedTime(&time_ms, start, end);
		hipEventElapsedTime(&time_ms_1, start_1, end_1);

		hipEventDestroy(start);
		hipEventDestroy(end);
		hipEventDestroy(start_1);
		hipEventDestroy(end_1);

		// double ops = 3*M*N;
		double tflps = 1e-9 * 3 * M * N/time_ms;
		// check that calculation was launched correctly on device, not that result
		// was computed yet
		//CHECK_ROCBLAS_STATUS(rstatus); 
		std::cout << "Time(s) : first : " << time_ms_1*1e-3 << std::endl;
		std::cout << "Time(s) : " << time_ms*1e-3 << std::endl;
		std::cout << "Time-diff(s) : " << (time_ms_1 - time_ms)*1e-3 << std::endl;
		std::cout << "Tflops  : " << tflps << std::endl;
		// fetch device memory results, automatically blocked until results ready
		CHECK_HIP_ERROR(hipMemcpy(hC.data(), dC, sizeof(dataType) * sizeC, hipMemcpyDeviceToHost));

    } // release device memory via helpers::DeviceVector destructors

    std::cout << "M, N, K, lda, ldb, ldc = " << M << ", " << N << ", " << K << ", " << lda << ", "
              << ldb << ", " << ldc << std::endl;
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
