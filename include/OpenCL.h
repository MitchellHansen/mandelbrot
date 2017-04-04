#pragma once

#include <string>
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <iostream>
#include "Vector4.hpp"
#include <string.h>
#include <memory>

#ifdef linux
#include <CL/cl.h>
#include <CL/opencl.h>
#include <GL/glx.h>

#elif defined _WIN32
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl_gl.h>
#include <CL/cl.h>
#include <CL/opencl.h>

// Note: windows.h must be included before Gl/GL.h
#include <windows.h>
#include <GL/GL.h>

#elif defined TARGET_OS_MAC
#include <OpenCL/opencl.h>

#endif


class OpenCL {
	
public:

	OpenCL();
	~OpenCL();

	// command queues are associated with a device and context, so for multi-gpu applications you would need
	// multiple command queues 
	
	
	// CONTEXTS
	// - An OpenCL context is created with one or more devices. Contexts are used by the OpenCL runtime 
	// for managing objects such as command - queues, memory, program and kernel objects and for executing 
	// kernels on one or more devices specified in the context.
	// - Contexts cannot be created using more than one platform!

	bool init();

	bool compile_kernel(std::string kernel_path, std::string kernel_name);

	// Create an image buffer from an SF texture. Access Type is the read/write specifier required by OpenCL
	bool create_image_buffer(std::string buffer_name, sf::Texture* texture, cl_int access_type);

	// Have CL create and manage the texture for the image buffer. Access Type is the read/write specifier required by OpenCL
	bool create_image_buffer(std::string buffer_name, sf::Vector2i size, cl_int access_type);

	// Create a buffer with CL_MEM_READ_ONLY and CL_MEM_COPY_HOST_PTR
	int create_buffer(std::string buffer_name, cl_uint size, void* data);

	// Create a buffer with user defined data access flags
	int create_buffer(std::string buffer_name, cl_uint size, void* data, cl_mem_flags flags);

	int set_kernel_arg(std::string kernel_name, int index, std::string buffer_name);
	
	void run_kernel(std::string kernel_name, sf::Vector2i work_size);

	void draw(sf::RenderWindow *window);

	class device {

	public:

		#pragma pack(push, 1)
		struct packed_data {

			cl_device_type device_type;
			cl_uint clock_frequency;
			char opencl_version[64];
			cl_uint compute_units;
			char device_extensions[1024];
			char device_name[256];
			char platform_name[128];
		};
		#pragma pack(pop)
		
		device(cl_device_id device_id, cl_platform_id platform_id);
		device(const device& d);
		void print(std::ostream& stream) const;
		void print_packed_data(std::ostream& stream);

		cl_device_id getDeviceId() const { return device_id; };
		cl_platform_id getPlatformId() const { return platform_id; };

	private:

		packed_data data;

		cl_device_id device_id;
		cl_platform_id platform_id;

		cl_bool is_little_endian = false;
		bool cl_gl_sharing = false;

	};

private:


	int error = 0;


	// The device which we have selected according to certain criteria
	cl_platform_id platform_id;
	cl_device_id device_id;

	// The GL shared context and its subsiquently generated command queue
	cl_context context;
	cl_command_queue command_queue;

	// Maps which contain a mapping from "name" to the host side CL memory object
	std::unordered_map<std::string, cl_kernel> kernel_map;
	std::unordered_map<std::string, cl_mem> buffer_map;
	std::unordered_map<std::string, std::pair<sf::Sprite, std::unique_ptr<sf::Texture>>> image_map;
	std::vector<device> device_list;

	// Query the hardware on this machine and store the devices
	bool aquire_hardware();

	// After aquiring hardware, create a shared context using platform specific CL commands
	bool create_shared_context();

	// Command queues must be created with a valid context
	bool create_command_queue();


	// Store a cl_mem object in the buffer map <string:name, cl_mem:buffer>
	bool store_buffer(cl_mem buffer, std::string buffer_name);

	// Using CL release the memory object and remove the KVP associated with the buffer name
	bool release_buffer(std::string buffer_name);

	bool load_config();
	void save_config();

	static bool vr_assert(int error_code, std::string function_name);
	
};
