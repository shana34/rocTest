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
    rocblas_status rstatus = rocblas_status_success;

    typedef float dataType;

    rocblas_int M    = atoi(argv[1]);
    rocblas_int N    = atoi(argv[2]); 
    rocblas_int incx = 1;
    rocblas_int incy = 1; 

    // float hAlpha = options.alpha;
    // float hBeta  = options.beta;

    rocblas_fill      fillA = (*(argv[3]+9)=='U') ? rocblas_fill_upper     : rocblas_fill_lower;
    rocblas_operation tranA = (*(argv[4]+9)=='N') ? rocblas_operation_none : rocblas_operation_transpose;
	rocblas_diagonal  diagA = (*(argv[5]+9)=='U') ? rocblas_diagonal_unit  : rocblas_diagonal_non_unit;
    
	size_t sizeX, dimX, absIncx;
    size_t sizeB, dimB;

    if(tranA == rocblas_operation_none)
    {
        dimX = N;
        dimB = M;
    }
    else // transpose
    {
        dimX = M;
        dimB = N;
    }
    rocblas_int lda   = M;
    size_t      sizeA = lda * size_t(N);

    absIncx = incx >= 0 ? incx : -incx;

    sizeX = dimX * absIncx;
    sizeB = dimB;

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    std::vector<dataType> hA(sizeA);
    std::vector<dataType> hx(sizeX, 0);
    std::vector<dataType> hb(sizeB);

    std::vector<dataType> hbGold(hb);

	int   x = 0, y = 0;
	int off = 0;
    for(size_t k = 0; k < sizeA; k++){
		x = k / dimX + 1;
		y = k % dimX + 1;
		off = ((y - x) > 0) ? (y - x) : (x - y);
		hA[k] = 1 + off;
    }

    for(size_t k = 0; k < sizeB; k++){
		hb[k] = (dimX - k)*(dimX - k + 1) / 2;
    }

	printf("M : %d, N : %d\n", M, N);
	printf("sizeA %lu: sizeX %lu: sizeB: %lu\n",sizeA, sizeX, sizeB);

    // print input
    // std::cout << "Input Vectors (X)" << std::endl;
    // helpers::printVector(hA);

    // using rocblas API
    rocblas_handle handle;
    rstatus = rocblas_create_handle(&handle);
    // CHECK_ROCBLAS_STATUS(rstatus);

    {
        // Naming: dx is in GPU (device) memory. hK is in CPU (host) memory

        // allocate memory on device
        helpers::DeviceVector<dataType> dA(sizeA);
        helpers::DeviceVector<dataType> dx(sizeX);
        helpers::DeviceVector<dataType> db(sizeB);

        if((!dA && sizeA) || (!dx && sizeX) || (!db && sizeB))
        {
            CHECK_HIP_ERROR(hipErrorOutOfMemory);
            return EXIT_FAILURE;
        }

        // time data to device, computation, and data from device back to host

        // copy data from CPU to device
        CHECK_HIP_ERROR(hipMemcpy(dA, hA.data(), sizeof(dataType) * sizeA, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(dx, hx.data(), sizeof(dataType) * sizeX, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(db, hb.data(), sizeof(dataType) * sizeB, hipMemcpyHostToDevice));

        // enable passing alpha and beta parameters from pointer to host memory
        // rstatus = rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host);

        // asynchronous calculation on device, returns before finished calculations
		hipEvent_t start, start_1, end, end_1;
		hipEventCreate(&start);
		hipEventCreate(&start_1);
		hipEventCreate(&end);
		hipEventCreate(&end_1);

		hipEventRecord(start_1);
					  rocblas_strsv(handle, fillA, tranA, diagA, M, dA, lda, dx, 1);
		hipEventRecord(end_1);
		hipEventSynchronize(end_1);

		int hot_call = 50;
		int hot      = hot_call;
		hipEventRecord(start);
		while(hot_call){
			rstatus = rocblas_strsv(handle, fillA, tranA, diagA, M, dA, lda, dx, 1);
			hot_call -= 1;
		}
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

        // check that calculation was launched correctly on device, not that result
        // was computed yet
        CHECK_STATUS(rstatus);

        // fetch device memory results, automatically blocked until results ready
        CHECK_HIP_ERROR(hipMemcpy(hb.data(), db, sizeof(dataType) * sizeB, hipMemcpyDeviceToHost));
		std::cout << "Time cost(1) : " << time_ms_1 << std::endl;
		std::cout << "Time cost    : " << time_ms << std::endl;
		std::cout << "Tflops : " << ((1e-9)*(M*M)*hot)/time_ms << std::endl;
		
    } // release device memory via helpers::DeviceVector destructors

    std::cout << "M, N, lda, incx, incy = " << M << ", " << N << ", " << lda << ", " << incx << \
		", " << incy << std::endl;
	 
    // print input
	// std::cout << "Output Vector Y = alpha*Identity*X(random,...) + beta*Y(1,1,...)" << std::endl;
    // helpers::printVector(hb);

    // calculate expected result using CPU
/*
    for(size_t i = 0; i < sizeB; i++)
    {
        // matrix is identity so just doing simpler calculation over vectors
        hbGold[i] = hAlpha * 1.0 * hx[i] + hBeta * hbGold[i];
    }

    dataType maxRelativeError = helpers::maxRelativeError(hb, hbGold);
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
*/
    rstatus = rocblas_destroy_handle(handle);
    // CHECK_ROCBLAS_STATUS(rstatus);

    return EXIT_SUCCESS;
}
