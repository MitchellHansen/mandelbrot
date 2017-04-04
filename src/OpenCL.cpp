#include <OpenCL.h>
#include "util.hpp"


void OpenCL::run_kernel(std::string kernel_name) {

	size_t global_work_size[2] = { static_cast<size_t>(viewport_resolution.x), static_cast<size_t>(viewport_resolution.y) };

	cl_kernel kernel = kernel_map.at(kernel_name);

	error = clEnqueueAcquireGLObjects(command_queue, 1, &buffer_map.at("viewport_image"), 0, 0, 0);
	if (vr_assert(error, "clEnqueueAcquireGLObjects"))
		return;

	//error = clEnqueueTask(command_queue, kernel, 0, NULL, NULL);
	error = clEnqueueNDRangeKernel(
		command_queue, kernel,
		2, NULL, global_work_size,
		NULL, 0, NULL, NULL);

	if (vr_assert(error, "clEnqueueNDRangeKernel"))
		return;

	clFinish(command_queue);

	// What if errors out and gl objects are never released?
	error = clEnqueueReleaseGLObjects(command_queue, 1, &buffer_map.at("viewport_image"), 0, NULL, NULL);
	if (vr_assert(error, "clEnqueueReleaseGLObjects"))
		return;

}

void OpenCL::draw(sf::RenderWindow *window) {

	window->draw(viewport_sprite);
}

void OpenCL::aquire_hardware() {

	// Get the number of platforms
	cl_uint plt_cnt = 0;
	clGetPlatformIDs(0, nullptr, &plt_cnt);

	// Get the ID's for those platforms
	std::vector<cl_platform_id> plt_buf(plt_cnt);
	clGetPlatformIDs(plt_cnt, plt_buf.data(), nullptr);

	// Populate the storage vector with the platform id's
	for (auto id : plt_buf) {
		platforms_and_devices.push_back(std::make_pair(id, std::vector<cl_device_id>()));
	}

	int device_position = 0;
	for (unsigned int i = 0; i < plt_cnt; i++) {

		cl_uint deviceIdCount = 0;
		error = clGetDeviceIDs(plt_buf[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &deviceIdCount);

		// Check to see if we even have OpenCL on this machine
		if (deviceIdCount == 0) {
			std::cout << "There appears to be no devices, or none at least supporting OpenCL" << std::endl;
			return;
		}

		// Get the device ids
		std::vector<cl_device_id> deviceIds(deviceIdCount);
		error = clGetDeviceIDs(plt_buf[i], CL_DEVICE_TYPE_ALL, deviceIdCount, deviceIds.data(), NULL);

		for (int d = 0; d < deviceIds.size(); d++) {

			device_list.emplace_back(device(deviceIds[d], plt_buf.at(i)));
			
		}
	}
}

void OpenCL::create_shared_context() {

	// Hurray for standards!
	// Setup the context properties to grab the current GL context

#ifdef linux

	cl_context_properties context_properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id,
		0
	};

#elif defined _WIN32

	HGLRC hGLRC = wglGetCurrentContext();
	HDC hDC = wglGetCurrentDC();
	cl_context_properties context_properties[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id,
		CL_GL_CONTEXT_KHR, (cl_context_properties)hGLRC,
		CL_WGL_HDC_KHR, (cl_context_properties)hDC,
		0
	};


#elif defined TARGET_OS_MAC

	CGLContextObj glContext = CGLGetCurrentContext();
	CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);
	cl_context_properties context_properties[] = {
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
		(cl_context_properties)shareGroup,
		0
	};

#endif

	// Create our shared context
	context = clCreateContext(
		context_properties,
		1,
		&device_id,
		nullptr, nullptr,
		&error
	);

	if (vr_assert(error, "clCreateContext"))
		return;

}

void OpenCL::create_command_queue() {

	// If context and device_id have initialized
	if (context && device_id) {

		command_queue = clCreateCommandQueue(context, device_id, 0, &error);

		if (vr_assert(error, "clCreateCommandQueue"))
			return;

		return;
	}
	else {
		std::cout << "Failed creating the command queue. Context or device_id not initialized";
		return;
	}
}

bool OpenCL::compile_kernel(std::string kernel_path, std::string kernel_name) {

	const char* source;
	std::string tmp;

	//Load in the kernel, and c stringify it
	tmp = read_file(kernel_path);
	source = tmp.c_str();


	size_t kernel_source_size = strlen(source);

	// Load the source into CL's data structure

	cl_program program = clCreateProgramWithSource(
		context, 1,
		&source,
		&kernel_source_size, &error
	);

	// This is not for compilation, it only loads the source
	if (vr_assert(error, "clCreateProgramWithSource"))
		return false;


	// Try and build the program
	// "-cl-finite-math-only -cl-fast-relaxed-math -cl-unsafe-math-optimizations"
	error = clBuildProgram(program, 1, &device_id, "-cl-finite-math-only -cl-fast-relaxed-math -cl-unsafe-math-optimizations", NULL, NULL);

	// Check to see if it errored out
	if (vr_assert(error, "clBuildProgram")) {

		// Get the size of the queued log
		size_t log_size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char *log = new char[log_size];

		// Grab the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		std::cout << log;
		return false;
	}

	// Done initializing the kernel
	cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &error);

	if (vr_assert(error, "clCreateKernel"))
		return false;

	// Do I want these to overlap when repeated??
	kernel_map[kernel_name] = kernel;

	return true;
}

int OpenCL::create_image_buffer(std::string buffer_name, cl_uint size, sf::Texture* texture, cl_int access_type) {
	
	if (buffer_map.count(buffer_name) > 0) {
		release_buffer(buffer_name);
	}

	int error;
	cl_mem buff = clCreateFromGLTexture(
		context, access_type, GL_TEXTURE_2D,
		0, texture->getNativeHandle(), &error);

	if (vr_assert(error, "clCreateFromGLTexture"))
		return 1;

	store_buffer(buff, buffer_name);

	return 1;
}

int OpenCL::create_buffer(std::string buffer_name, cl_uint size, void* data) {

	if (buffer_map.count(buffer_name) > 0) {
		release_buffer(buffer_name);
	}

	cl_mem buff = clCreateBuffer(
		context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		size, data, &error
	);

	if (vr_assert(error, "clCreateBuffer"))
		return -1;

	store_buffer(buff, buffer_name);

	return 1;
}

int OpenCL::create_buffer(std::string buffer_name, cl_uint size, void* data, cl_mem_flags flags) {

	if (buffer_map.count(buffer_name) > 0) {
		release_buffer(buffer_name);
	}

	cl_mem buff = clCreateBuffer(
		context, flags,
		size, data, &error
	);

	if (vr_assert(error, "clCreateBuffer"))
		return -1;

	store_buffer(buff, buffer_name);

	return 1;

}

int OpenCL::store_buffer(cl_mem buffer, std::string buffer_name) {
	
	if (buffer_map.count(buffer_name)) {
		clReleaseMemObject(buffer_map[buffer_name]);
	}
	
	buffer_map[buffer_name] = buffer;

	return 1;
}

int OpenCL::release_buffer(std::string buffer_name) {

	if (buffer_map.count(buffer_name) > 0) {

		int error = clReleaseMemObject(buffer_map.at(buffer_name));

		if (vr_assert(error, "clReleaseMemObject")) {
			std::cout << "Error releasing buffer : " << buffer_name;
			std::cout << "Buffer not removed";
			return -1;

		}
		else {
			buffer_map.erase(buffer_name);
		}

	}
	else {
		std::cout << "Error releasing buffer : " << buffer_name;
		std::cout << "Buffer not found";
		return -1;
	}

	return 1;
}

void OpenCL::assign_kernel_args() {

}

int OpenCL::set_kernel_arg(std::string kernel_name, int index, std::string buffer_name) {

	error = clSetKernelArg(
		kernel_map.at(kernel_name),
		index,
		sizeof(cl_mem),
		(void *)&buffer_map.at(buffer_name));

	if (vr_assert(error, "clSetKernelArg")) {
		std::cout << buffer_name << std::endl;
		std::cout << buffer_map.at(buffer_name) << std::endl;
		return -1;
	}
	return 1;
}

OpenCL::OpenCL(sf::Vector2i resolution) : viewport_resolution(resolution){

	viewport_texture.create(viewport_resolution.x, viewport_resolution.y);
	viewport_sprite.setTexture(viewport_texture);


}

OpenCL::~OpenCL() {

}


bool OpenCL::load_config() {

	std::ifstream input_file("device_config.bin", std::ios::binary | std::ios::in);

	if (!input_file.is_open()) {
		std::cout << "No config file..." << std::endl;
		return false;
	}

	device::packed_data data;
	input_file.read(reinterpret_cast<char*>(&data), sizeof(data));

	std::cout << "config loaded, looking for device..." << std::endl;

	for (auto d: device_list) {
		
		if (memcmp(&d, &data, sizeof(device::packed_data)) == 0) {
			std::cout << "Found saved device" << std::endl;
			device_id = d.getDeviceId();
			platform_id = d.getPlatformId();
			break;
		}
	}

	input_file.close();
	return true;
}


void OpenCL::save_config() {

	std::ofstream output_file;
	output_file.open("device_config.bin", std::ofstream::binary | std::ofstream::out | std::ofstream::trunc);

	device d(device_id, platform_id);
	d.print_packed_data(output_file);

	output_file.close();
}

bool OpenCL::init(sf::Vector4f *range)
{
	
	// Initialize opencl up to the point where we start assigning buffers
	aquire_hardware();

	if (!load_config()) {

		std::cout << "Select a device number which you wish to use" << std::endl;
		
		for (int i = 0; i < device_list.size(); i++) {

			std::cout << "\n-----------------------------------------------------------------" << std::endl;
			std::cout << "\tDevice Number : " << i << std::endl;
			std::cout << "-----------------------------------------------------------------" << std::endl;

			device_list.at(i).print(std::cout);
		}

		int selection = -1;
		
		while (selection < 0 && selection >= device_list.size()) {

			std::cout << "Device which you wish to use : ";
			std::cin >> selection;
		}

		device_id = device_list.at(selection).getDeviceId();
		platform_id = device_list.at(selection).getPlatformId();

		save_config();

	}

	create_shared_context();
	
	create_command_queue();

	while (!compile_kernel("../kernels/mandlebrot.cl", "mandlebrot")) {
		std::cin.get();
	}
	
	create_image_buffer("viewport_image", viewport_texture.getSize().x * viewport_texture.getSize().x * 4 * sizeof(float), &viewport_texture, CL_MEM_WRITE_ONLY);
	create_buffer("image_res", sizeof(sf::Vector2i), &viewport_resolution);
	create_buffer("range", sizeof(sf::Vector4f), range, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR);


	set_kernel_arg("mandlebrot", 0, "image_res");
	set_kernel_arg("mandlebrot", 1, "viewport_image");
	set_kernel_arg("mandlebrot", 2, "range");

	return true;
}


bool OpenCL::vr_assert(int error_code, std::string function_name) {

	// Just gonna do a little jump table here, just error codes so who cares
	std::string err_msg = "Error : ";

	switch (error_code) {

	case CL_SUCCESS:
		return false;

	case 1:
		return false;

	case CL_DEVICE_NOT_FOUND:
		err_msg += "CL_DEVICE_NOT_FOUND";
		break;
	case CL_DEVICE_NOT_AVAILABLE:
		err_msg = "CL_DEVICE_NOT_AVAILABLE";
		break;
	case CL_COMPILER_NOT_AVAILABLE:
		err_msg = "CL_COMPILER_NOT_AVAILABLE";
		break;
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:
		err_msg = "CL_MEM_OBJECT_ALLOCATION_FAILURE";
		break;
	case CL_OUT_OF_RESOURCES:
		err_msg = "CL_OUT_OF_RESOURCES";
		break;
	case CL_OUT_OF_HOST_MEMORY:
		err_msg = "CL_OUT_OF_HOST_MEMORY";
		break;
	case CL_PROFILING_INFO_NOT_AVAILABLE:
		err_msg = "CL_PROFILING_INFO_NOT_AVAILABLE";
		break;
	case CL_MEM_COPY_OVERLAP:
		err_msg = "CL_MEM_COPY_OVERLAP";
		break;
	case CL_IMAGE_FORMAT_MISMATCH:
		err_msg = "CL_IMAGE_FORMAT_MISMATCH";
		break;
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:
		err_msg = "CL_IMAGE_FORMAT_NOT_SUPPORTED";
		break;
	case CL_BUILD_PROGRAM_FAILURE:
		err_msg = "CL_BUILD_PROGRAM_FAILURE";
		break;
	case CL_MAP_FAILURE:
		err_msg = "CL_MAP_FAILURE";
		break;
	case CL_MISALIGNED_SUB_BUFFER_OFFSET:
		err_msg = "CL_MISALIGNED_SUB_BUFFER_OFFSET";
		break;
	case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
		err_msg = "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
		break;
	case CL_COMPILE_PROGRAM_FAILURE:
		err_msg = "CL_COMPILE_PROGRAM_FAILURE";
		break;
	case CL_LINKER_NOT_AVAILABLE:
		err_msg = "CL_LINKER_NOT_AVAILABLE";
		break;
	case CL_LINK_PROGRAM_FAILURE:
		err_msg = "CL_LINK_PROGRAM_FAILURE";
		break;
	case CL_DEVICE_PARTITION_FAILED:
		err_msg = "CL_DEVICE_PARTITION_FAILED";
		break;
	case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
		err_msg = "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
		break;
	case CL_INVALID_VALUE:
		err_msg = "CL_INVALID_VALUE";
		break;
	case CL_INVALID_DEVICE_TYPE:
		err_msg = "CL_INVALID_DEVICE_TYPE";
		break;
	case CL_INVALID_PLATFORM:
		err_msg = "CL_INVALID_PLATFORM";
		break;
	case CL_INVALID_DEVICE:
		err_msg = "CL_INVALID_DEVICE";
		break;
	case CL_INVALID_CONTEXT:
		err_msg = "CL_INVALID_CONTEXT";
		break;
	case CL_INVALID_QUEUE_PROPERTIES:
		err_msg = "CL_INVALID_QUEUE_PROPERTIES";
		break;
	case CL_INVALID_COMMAND_QUEUE:
		err_msg = "CL_INVALID_COMMAND_QUEUE";
		break;
	case CL_INVALID_HOST_PTR:
		err_msg = "CL_INVALID_HOST_PTR";
		break;
	case CL_INVALID_MEM_OBJECT:
		err_msg = "CL_INVALID_MEM_OBJECT";
		break;
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
		err_msg = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
		break;
	case CL_INVALID_IMAGE_SIZE:
		err_msg = "CL_INVALID_IMAGE_SIZE";
		break;
	case CL_INVALID_SAMPLER:
		err_msg = "CL_INVALID_SAMPLER";
		break;
	case CL_INVALID_BINARY:
		err_msg = "CL_INVALID_BINARY";
		break;
	case CL_INVALID_BUILD_OPTIONS:
		err_msg = "CL_INVALID_BUILD_OPTIONS";
		break;
	case CL_INVALID_PROGRAM:
		err_msg = "CL_INVALID_PROGRAM";
		break;
	case CL_INVALID_PROGRAM_EXECUTABLE:
		err_msg = "CL_INVALID_PROGRAM_EXECUTABLE";
		break;
	case CL_INVALID_KERNEL_NAME:
		err_msg = "CL_INVALID_KERNEL_NAME";
		break;
	case CL_INVALID_KERNEL_DEFINITION:
		err_msg = "CL_INVALID_KERNEL_DEFINITION";
		break;
	case CL_INVALID_KERNEL:
		err_msg = "CL_INVALID_KERNEL";
		break;
	case CL_INVALID_ARG_INDEX:
		err_msg = "CL_INVALID_ARG_INDEX";
		break;
	case CL_INVALID_ARG_VALUE:
		err_msg = "CL_INVALID_ARG_VALUE";
		break;
	case CL_INVALID_ARG_SIZE:
		err_msg = "CL_INVALID_ARG_SIZE";
		break;
	case CL_INVALID_KERNEL_ARGS:
		err_msg = "CL_INVALID_KERNEL_ARGS";
		break;
	case CL_INVALID_WORK_DIMENSION:
		err_msg = "CL_INVALID_WORK_DIMENSION";
		break;
	case CL_INVALID_WORK_GROUP_SIZE:
		err_msg = "CL_INVALID_WORK_GROUP_SIZE";
		break;
	case CL_INVALID_WORK_ITEM_SIZE:
		err_msg = "CL_INVALID_WORK_ITEM_SIZE";
		break;
	case CL_INVALID_GLOBAL_OFFSET:
		err_msg = "CL_INVALID_GLOBAL_OFFSET";
		break;
	case CL_INVALID_EVENT_WAIT_LIST:
		err_msg = "CL_INVALID_EVENT_WAIT_LIST";
		break;
	case CL_INVALID_EVENT:
		err_msg = "CL_INVALID_EVENT";
		break;
	case CL_INVALID_OPERATION:
		err_msg = "CL_INVALID_OPERATION";
		break;
	case CL_INVALID_GL_OBJECT:
		err_msg = "CL_INVALID_GL_OBJECT";
		break;
	case CL_INVALID_BUFFER_SIZE:
		err_msg = "CL_INVALID_BUFFER_SIZE";
		break;
	case CL_INVALID_MIP_LEVEL:
		err_msg = "CL_INVALID_MIP_LEVEL";
		break;
	case CL_INVALID_GLOBAL_WORK_SIZE:
		err_msg = "CL_INVALID_GLOBAL_WORK_SIZE";
		break;
	case CL_INVALID_PROPERTY:
		err_msg = "CL_INVALID_PROPERTY";
		break;
	case CL_INVALID_IMAGE_DESCRIPTOR:
		err_msg = "CL_INVALID_IMAGE_DESCRIPTOR";
		break;
	case CL_INVALID_COMPILER_OPTIONS:
		err_msg = "CL_INVALID_COMPILER_OPTIONS";
		break;
	case CL_INVALID_LINKER_OPTIONS:
		err_msg = "CL_INVALID_LINKER_OPTIONS";
		break;
	case CL_INVALID_DEVICE_PARTITION_COUNT:
		err_msg = "CL_INVALID_DEVICE_PARTITION_COUNT";
		break;
	case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
		err_msg = "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
		break;
	case CL_PLATFORM_NOT_FOUND_KHR:
		err_msg = "CL_PLATFORM_NOT_FOUND_KHR";
		break;
	}

	std::cout << err_msg << "  =at=  " << function_name << std::endl;
	return true;
}

OpenCL::device::device(cl_device_id device_id, cl_platform_id platform_id) {
	
	this->device_id = device_id;
	this->platform_id = platform_id;

	int error = 0;
	error = clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, 128, (void*)&data.platform_name, nullptr);
	if (vr_assert(error, "clGetPlatformInfo"))
		return;

	error = clGetDeviceInfo(device_id, CL_DEVICE_VERSION, sizeof(char) * 128, &data.opencl_version, NULL);
	error = clGetDeviceInfo(device_id, CL_DEVICE_TYPE, sizeof(cl_device_type), &data.device_type, NULL);
	error = clGetDeviceInfo(device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &data.clock_frequency, NULL);
	error = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &data.compute_units, NULL);
	error = clGetDeviceInfo(device_id, CL_DEVICE_EXTENSIONS, 1024, &data.device_extensions, NULL);
	error = clGetDeviceInfo(device_id, CL_DEVICE_NAME, 256, &data.device_name, NULL);
	error = clGetDeviceInfo(device_id, CL_DEVICE_ENDIAN_LITTLE, sizeof(cl_bool), &is_little_endian, NULL);
	
	// Check for the sharing extension
	if (std::string(data.device_extensions).find("cl_khr_gl_sharing") != std::string::npos ||
		std::string(data.device_extensions).find("cl_APPLE_gl_sharing") != std::string::npos) {
		cl_gl_sharing = true;
	}
}


void OpenCL::device::print(std::ostream& stream) {
		
	stream << "\n\tDevice ID        : " << device_id << std::endl;
	stream << "\tDevice Name      : " << data.device_name << std::endl;

	stream << "\tPlatform ID      : " << platform_id << std::endl;
	stream << "\tPlatform Name    : " << data.platform_name << std::endl;

	stream << "\tOpenCL Version   : " << data.opencl_version << std::endl;
	stream << "\tSupports sharing : " << std::boolalpha << cl_gl_sharing << std::endl;
	stream << "\tDevice Type      : ";

	if (data.device_type == CL_DEVICE_TYPE_CPU)
		stream << "CPU" << std::endl;

	else if (data.device_type == CL_DEVICE_TYPE_GPU)
		stream << "GPU" << std::endl;

	else if (data.device_type  == CL_DEVICE_TYPE_ACCELERATOR)
		stream << "Accelerator" << std::endl;

	stream << "\tIs Little Endian : " << std::boolalpha << is_little_endian << std::endl;

	stream << "\tClock Frequency  : " << data.clock_frequency << std::endl;
	stream << "\tCompute Units    : " << data.compute_units << std::endl;

	stream << "\n*Extensions*" << std::endl;
	stream << data.device_extensions << std::endl;
	stream << "\n";

}

void OpenCL::device::print_packed_data(std::ostream& stream) {
	stream.write(reinterpret_cast<char*>(&data), sizeof(data));
}
