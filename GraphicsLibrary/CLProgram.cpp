//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "jett.h"

#include "CLProgram.h"
#include "GPUProcessor.h"
#include <sstream>


// Construction/destruction
CLProgram::CLProgram()
{
	m_pDevice = NULL;
    m_program = NULL;
}

CLProgram::~CLProgram()
{
    free();
}

void CLProgram::create( CGPUProcessor &cl, const unsigned char *program, size_t len, const stringCollection *defs, const char* build_options )
{
    String s2;
    
    if (defs)
    {
        // Compile a list of defs
        std::ostringstream s;
        stringCollection::const_iterator i = defs->begin();
        while (i != defs->end())
        {
            s << "#define " << i->first << " " << i->second << "\n";
            ++ i;
        }
        
		s2 = s.str();
    }
    
    // Create the buffer to put the program in to
    unsigned char *src = new unsigned char[ len + s2.size() + 1 ];
    memset(src, 0, len + s2.size() + 1);
    memcpy( src, s2.c_str(), s2.size() );

    memcpy( src + s2.size(), program, len );
    
    m_pDevice = &cl;
	m_program = cl.createProgram( src, build_options );
    
    delete [] src;
}

// Destroy this program
void CLProgram::free()
{
    if (m_program)
    {
        clReleaseProgram(m_program);
    }
    m_program = NULL;
}



// Create a kernel
cl_kernel CLProgram::createKernel( const char *kernel_name )
{
    cl_kernel r;
    
    int err;
    r = clCreateKernel(m_program, kernel_name, &err );
    
    if (!r || err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to create kernel");
    }
    
    return r;
}

// Get the compiled kernel
void CLProgram::saveKernelBinary( TCHAR *filename )
{
	// Get the number of devices that are supported
	cl_uint program_num_devices;
	int err = clGetProgramInfo( m_program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &program_num_devices,NULL);
	if (err != CL_SUCCESS)
	{
		throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to query program device count");
	}

	if (program_num_devices == 0)
	{
		// Can't be done, oh well
		return;
	}

	// Get the sizes of the binaries for the devices
	size_t* binaries_sizes = new size_t[program_num_devices];
	err = clGetProgramInfo( m_program, CL_PROGRAM_BINARY_SIZES, program_num_devices*sizeof(size_t), binaries_sizes, NULL );
	if (err != CL_SUCCESS)
	{
		delete[] binaries_sizes;
		throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to query program binary sizes");
	}

	// Now create the array to put the data in to		
	char **binaries = new char*[program_num_devices];
	for (size_t i = 0; i < program_num_devices; i++)
	{
		binaries[i] = new char[binaries_sizes[i]+1];
	}
	
	// .. and fetch
	err = clGetProgramInfo(m_program, CL_PROGRAM_BINARIES, program_num_devices*sizeof(char *), binaries, NULL);
	if (err != CL_SUCCESS)
	{
		for (size_t i = 0; i < program_num_devices; i++)
		{
			delete [] binaries[i];
		}

		delete[] binaries;
		delete[] binaries_sizes;
		throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to query program binaries");
	}

    FILE *fout = NULL;
	_tfopen_s( &fout, filename,_T("wb"));
	if ( !fout )
	{
        throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// Now save
	fwrite( &program_num_devices, sizeof(program_num_devices), 1, fout );
	for (size_t i = 0; i < program_num_devices; i++)
	{
		fwrite( &binaries_sizes[i], sizeof(size_t), 1, fout );
	}
	for (size_t i = 0; i < program_num_devices; i++)
	{
		fwrite( binaries[i], 1, binaries_sizes[i], fout );
	}
	fclose( fout );


	// Delete memory objects
	for (size_t i = 0; i < program_num_devices; i++)
	{
		delete [] binaries[i];
	}

	delete[] binaries;
	delete[] binaries_sizes;
}

void CLProgram::loadKernelBinary( TCHAR *filename )
{
	if (!m_pDevice)
	{
		throw jett_exception(JETT_INTERNAL_ERROR, 0, "Cannot load a kernel binary without an OpenCL device");
	}

    FILE *fin = NULL;
	_tfopen_s( &fin, filename,_T("rb"));
	if ( !fin )
	{
        throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// Now load
	cl_uint program_num_devices = 0;
	if (fread( &program_num_devices, sizeof(program_num_devices), 1, fin ) != 1)
	{
		fclose( fin );
		throw jett_exception(JETT_OPENCL_FAILURE, 0, "Failed to read kernel binary file");
	}

	// Read the sizes
	size_t* binaries_sizes = new size_t[program_num_devices];
	for (size_t i = 0; i < program_num_devices; i++)
	{
		if (fread( &binaries_sizes[i], sizeof(size_t), 1, fin ) != 1)
		{
			fclose( fin );
			throw jett_exception(JETT_OPENCL_FAILURE, 0, "Failed to read kernel binary file");
		}
	}

	// Read in the binaries
	char **binaries = new char*[program_num_devices];
	for (size_t i = 0; i < program_num_devices; i++)
	{
		binaries[i] = new char[binaries_sizes[i]+1]; 
		if (fread( binaries[i], 1, binaries_sizes[i], fin ) != binaries_sizes[i])
		{
			for (size_t j = 0; j <= i; ++j)
			{
				delete [] binaries[j];
			}
			delete[] binaries;
			delete[] binaries_sizes;
			fclose( fin );
			throw jett_exception(JETT_OPENCL_FAILURE, 0, "Failed to read kernel binary file");
		}
	}
	fclose( fin );

	if (program_num_devices != 1)
	{
		for (size_t i = 0; i < program_num_devices; i++)
		{
			delete [] binaries[i];
		}

		delete[] binaries;
		delete[] binaries_sizes;
		throw jett_exception(JETT_OPENCL_FAILURE, 0, "Only single-device program binaries are supported");
	}

	cl_device_id device = m_pDevice->get_handle();
	cl_int binary_status = CL_SUCCESS;
	free();
	m_program = m_pDevice->createProgramFromBinary(
		program_num_devices,
		&device,
		binaries_sizes,
		(const unsigned char **)binaries,
		&binary_status);

	// Delete memory objects
	for (size_t i = 0; i < program_num_devices; i++)
	{
		delete [] binaries[i];
	}

	delete[] binaries;
	delete[] binaries_sizes;

	if (binary_status != CL_SUCCESS)
	{
		throw jett_exception(JETT_OPENCL_FAILURE, binary_status, "Failed to load kernel binary");
	}
}

#if 0

#define MEM_SIZE(128)
#define MAX_BINARY_SIZE(0x100000)

int main()
{
	cl_platform_id_platform_id = NULL;
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_mem membobj = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;

	float mem[MEM_SIZE];
		
	FILE *fp;
	char fileName[] = "./kernel.clbin";
	size_t binary_size;
	char *binary_buf;
	cl_int binary_status;
	cl_int i;

/* Load kernel binary */

	fp = fopen(fileName, "r");
	if(!fp){
		fprintf(stderr,"failed to load kernel.\n");
		exit(1);
	}

	binary_buf = (char *) malloc(MAX_BINARY_SIZE);
	binary_size = fread(binary_buf,1, MAX_BINARY_SIZE, fp);
	fclose(fp);
	
	/* Initialize input data */

	for(i=0;i<MEM_SIZE; i++){
		mem[i] = i;
	}
	
	/* Get platform/device information */

	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT,1,&device_id, &ret_num_devices);
	
	/*Create OpenCl context */

	context = clCreateContext(NULL< 1, &device_id, NULL, NULL, &ret);
	
	/*create command queue */
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	/* Create memory buffer */

	memobj = clCreateBuffer(context, C_MEM_READ_WRITE, MEM_SIZE * sizeof(float),NULL, &ret);

	/* Transfer data over to the memory Buffer */

	ret = clEnqueueWriteBuffer ( command_queue, memobj, CL_TRUE, 0 , MEM_SIZE * sizeof(float), mem, 0, NULL, NULL);

	/* Create Kernel program from the kernel binary */

	program = clCreateProgramWithBinary(context, 1, &device_id, (const size_t *)&binary_size, (const unsigned char **)&binary_buf,&binary_status, &ret);

	/* Create OpenCL kernel */

	kernel = clCreateKernel(program, "VecAdd", &ret);

	printf("err : %d\n", ret);
	/*set OpenCl Kernel arguments */

	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem),(void*)&memobj);

	size_t global_work_size[3] = {MEM_SIZE, 0, 0};
	size_t local_work_size[3] = {MEM_SIZE, 0, 0};

	/* Execute OpenCL kernel */

	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_work_size, local_work_size, 0, NULL, NULL);
	
	/*Copy result from the memory buffer */

	ret = clEnqueueReadBuffer(command_queue, memobj, CL_TRUE, 0, MEM_SIZE * sizeof(float), mem, 0, NULL, NULL);

	/* Display results */

	for(i=0; i < MEM_SIZE; i++){
		printf("mem[%d]: %f\n", i, mem[i]);
	}

	/*Finalization */

	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(memobj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
	free(binary_buf);
	
	return 0;
}

	
#endif