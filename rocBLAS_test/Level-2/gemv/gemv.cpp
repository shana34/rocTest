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
    helpers::ArgParser options("MNabxy");
    //if(!options.validArgs(argc, argv))
    //    return EXIT_FAILURE;

    rocblas_status rstatus = rocblas_status_success;

    typedef float dataType;

    rocblas_int M    = atoi(argv[1]);
    rocblas_int N    = atoi(argv[2]);
    rocblas_int incx = options.incx;
    rocblas_int incy = options.incy;

    float hAlpha = options.alpha;
    float hBeta  = options.beta;

    const rocblas_operation transA = rocblas_operation_none;

    size_t sizeX, dimX, absIncx;
    size_t sizeY, dimY, absIncy;

    if(transA == rocblas_operation_none)
    {
        dimX = N;
        dimY = M;
    }
    else // transpose
    {
        dimX = M;
        dimY = N;
    }
    rocblas_int lda   = M;
    size_t      sizeA = lda * size_t(N);

    absIncx = incx >= 0 ? incx : -incx;
    absIncy = incy >= 0 ? incy : -incy;

    sizeX = dimX * absIncx;
    sizeY = dimY * absIncy;

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    std::vector<dataType> hA(sizeA);
    std::vector<dataType> hX(sizeX);
    std::vector<dataType> hY(sizeY, 1);

    std::vector<dataType> hYGold(hY);

    for(size_t k = 0; k < sizeA; k++){
		hA[k] = rand()*1.0 / RAND_MAX ;
    }

    for(size_t k = 0; k < sizeX; k++){
		hX[k] = rand()*1.0 / RAND_MAX ;
    }
	printf("M : %d, N : %d\n", M, N);
	printf("sizeA %lu: sizeX %lu: sizeY: %lu\n",sizeA, sizeX, sizeY);

    // print input
    // std::cout << "Input Vectors (X)" << std::endl;
    // helpers::printVector(hX);

    // using rocblas API
    rocblas_handle handle;
    rstatus = rocblas_create_handle(&handle);
    // CHECK_ROCBLAS_STATUS(rstatus);

    {
        // Naming: dX is in GPU (device) memory. hK is in CPU (host) memory

        // allocate memory on device
        helpers::DeviceVector<dataType> dA(sizeA);
        helpers::DeviceVector<dataType> dX(sizeX);
        helpers::DeviceVector<dataType> dY(sizeY);

        if((!dA && sizeA) || (!dX && sizeX) || (!dY && sizeY))
        {
            CHECK_HIP_ERROR(hipErrorOutOfMemory);
            return EXIT_FAILURE;
        }

        // time data to device, computation, and data from device back to host

        // copy data from CPU to device
        CHECK_HIP_ERROR(hipMemcpy(dA, hA.data(), sizeof(dataType) * sizeA, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(dX, hX.data(), sizeof(dataType) * sizeX, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(dY, hY.data(), sizeof(dataType) * sizeY, hipMemcpyHostToDevice));

        // enable passing alpha and beta parameters from pointer to host memory
        // rstatus = rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host);

        // asynchronous calculation on device, returns before finished calculations
		hipEvent_t start, start_1, end, end_1;
		hipEventCreate(&start);
		hipEventCreate(&start_1);
		hipEventCreate(&end);
		hipEventCreate(&end_1);

		hipEventRecord(start_1);
					  rocblas_sgemv(handle, transA, M, N, &hAlpha, dA, lda, dX, 1, &hBeta, dY, 1);
		hipEventRecord(end_1);
		hipEventSynchronize(end_1);

		hipEventRecord(start);

			rstatus = rocblas_sgemv(handle, transA, M, N, &hAlpha, dA, lda, dX, 1, &hBeta, dY, 1);

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
        CHECK_HIP_ERROR(hipMemcpy(hY.data(), dY, sizeof(dataType) * sizeY, hipMemcpyDeviceToHost));
		std::cout << "Time cost(1) : " << time_ms_1 << std::endl;
		std::cout << "Time cost    : " << time_ms << std::endl;
		std::cout << "Tflops : " << ((1e-9)*(2*M*N+2*M))/time_ms << std::endl;
		
    } // release device memory via helpers::DeviceVector destructors

    std::cout << "M, N, lda, incx, incy = " << M << ", " << N << ", " << lda << ", " << incx << \
		", " << incy << std::endl;
	 
    // print input
	// std::cout << "Output Vector Y = alpha*Identity*X(random,...) + beta*Y(1,1,...)" << std::endl;
    // helpers::printVector(hY);
/*
    // calculate expected result using CPU
    for(size_t i = 0; i < sizeY; i++)
    {
        // matrix is identity so just doing simpler calculation over vectors
        hYGold[i] = hAlpha * 1.0 * hX[i] + hBeta * hYGold[i];
    }

    dataType maxRelativeError = helpers::maxRelativeError(hY, hYGold);
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
