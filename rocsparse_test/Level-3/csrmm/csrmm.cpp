#include "utility.hpp"

#include <rocsparse.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

int main(int argc, char* argv[])
{
    // Parse command line
    if(argc < 2)
    {
        fprintf(stderr, "%s <ndim> [<trials> <batch_size>]\n", argv[0]);
        return -1;
    }

	typedef double dataType;

	rocsparse_operation transA = rocsparse_operation_none;
	rocsparse_operation transB = rocsparse_operation_none;

    rocsparse_int mdim       = atoi(argv[1]);
	rocsparse_int ndim       = atoi(argv[2]);
	rocsparse_int kdim       = atoi(argv[3]);

	printf("Input : --mdim=%d, --ndim%d, --kdim=%d\n", mdim, ndim, kdim);
    int           trials     = 200;
    int           batch_size = 1;
	trials = trials;
	batch_size = batch_size;
    if(argc > 3)
    {
        trials = atoi(argv[4]);
    }
    if(argc > 4)
    {
        batch_size = atoi(argv[5]);
    }
	if(argc > 5)
	{
		 transA = (*(argv[6]+9) == 'N') ? rocsparse_operation_none : rocsparse_operation_transpose; 
		 transB = (*(argv[7]+9) == 'N') ? rocsparse_operation_none : rocsparse_operation_transpose;
	}
	
    // rocSPARSE handle & devie info
    rocsparse_handle handle;
    rocsparse_create_handle(&handle);

    hipDeviceProp_t devProp;
    int             device_id = 0;

    hipGetDevice(&device_id);
    hipGetDeviceProperties(&devProp, device_id);
    printf("Device: %s\n", devProp.name);

    // Generate CSR
    std::vector<rocsparse_int> hAptr;
    std::vector<rocsparse_int> hAcol;
    std::vector<dataType>      hAval;
    
	// gen_2d_laplacian is defined in 'utinity.cpp'
	// it tries to make a CSR format matrix by dimn^2 x dimn^2
	rocsparse_int m   = gen_2d_laplacian(ndim, hAptr, hAcol, hAval, rocsparse_index_base_zero);
    rocsparse_int k   = m;
    rocsparse_int nnz = hAptr[m];
	rocsparse_int n   = ndim*ndim;

	printf("size(hAptr) : %lu, size(hAcol) : %lu, size(hAval) : %lu\n",hAptr.size(),hAcol.size(),hAval.size());

	rocsparse_int sizeB = k * n;
	rocsparse_int sizeC = m * n;
    // Sample some random data
    srand(12345ULL);

    dataType halpha = static_cast<dataType>(rand()) / RAND_MAX;
    dataType hbeta  = halpha;
 
    std::vector<dataType> hB(sizeB);
	std::vector<dataType> hC(sizeC);

    for(int con = 0; con < sizeB; con++)
	{
		hB[con] = rand() * 1.0 / RAND_MAX;
	}
	
	hC = hB;
	printf("hB[10] : %f\n", hB[10]);
	printf("hC[10] : %f\n", hC[10]);
    // Matrix descriptor
    rocsparse_mat_descr descrA;
    rocsparse_create_mat_descr(&descrA);

    // Offload data to device
    rocsparse_int*   dAptr = NULL;
    rocsparse_int*   dAcol = NULL;
    dataType*        dAval = NULL;
    dataType*        dB    = NULL;
    dataType*        dC    = NULL;

    hipMalloc((void**)&dAptr, sizeof(rocsparse_int) * (m + 1));
    hipMalloc((void**)&dAcol, sizeof(rocsparse_int) * nnz);
    hipMalloc((void**)&dAval, sizeof(dataType) * nnz);
    hipMalloc((void**)&dB, sizeof(dataType) * sizeB);
    hipMalloc((void**)&dC, sizeof(dataType) * sizeC);

    hipMemcpy(dAptr, hAptr.data(), sizeof(rocsparse_int) * (m + 1), hipMemcpyHostToDevice);
    hipMemcpy(dAcol, hAcol.data(), sizeof(rocsparse_int) * nnz, hipMemcpyHostToDevice);
    hipMemcpy(dAval, hAval.data(), sizeof(dataType) * nnz, hipMemcpyHostToDevice);
    hipMemcpy(dB, hB.data(), sizeof(dataType) * sizeB, hipMemcpyHostToDevice);
	hipMemcpy(dC, hC.data(), sizeof(dataType) * sizeC, hipMemcpyHostToDevice);

	// Warm up
	for(size_t i = 0; i < 10; i++)
	{
		rocsparse_dcsrmm(handle,
						 transA,
						 transB,
						 m,
						 n,
						 kdim,
						 nnz,
						 &halpha,
						 descrA,
						 dAval,
						 dAptr,
						 dAcol,
						 dB,
						 kdim,
						 &hbeta,
						 dC,
						 m);
	}

	hipDeviceSynchronize();
	rocsparse_status rs = rocsparse_status_success;
	// Test start
	hipEvent_t start, end;
	hipEventCreate(&start);
	hipEventCreate(&end);
	
	hipEventRecord(start);
	
			rs = rocsparse_dcsrmm(handle,
						 transA,
						 transB,
						 m,
						 n,
						 k,
						 nnz,
						 &halpha,
						 descrA,
						 dAval,
						 dAptr,
						 dAcol,
						 dB,
						 k,
						 &hbeta,
						 dC,
						 m);
	hipEventRecord(end);
	hipEventSynchronize(start);
	hipEventSynchronize(end);
	float time = 0.0f;
	hipEventElapsedTime(&time, start, end);
	printf("The status : %d\n",rs);
    double bandwidth = 0;
//        = static_cast<double>(sizeof(double) * (4 * m + nnz) + sizeof(rocsparse_int) * (2 * nnz))
//         / time / 1e6;
    double gflops = static_cast<double>(2 * nnz + 2 * m)*n / (time* 1e6);
    printf("m\t\tn\t\tnnz\t\talpha\tbeta\tGFlops\tGB/s(meaningless)\tusemc\n");
    printf("%8d\t%8d\t%9d\t%0.2lf\t%0.2lf\t%0.2lf\t%0.2lf\t%0.2lf\n",
           m,
           n,
           nnz,
           halpha,
           hbeta,
           gflops,
           bandwidth,
           time);

    // Clear up on device
    hipFree(dAptr);
    hipFree(dAcol);
    hipFree(dAval);
    hipFree(dB);
    hipFree(dC);
	hipEventDestroy(start);
	hipEventDestroy(end);
    
	rocsparse_destroy_mat_descr(descrA);
    rocsparse_destroy_handle(handle);
	return rocsparse_status_success;
}
