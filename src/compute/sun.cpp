#include "sun.h"

using namespace quavis;

quavis::ComputeSun::ComputeSun(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2 &image_dim, const bool use_v2)
  : image_dim_{image_dim}
  , ComputeBaseGPU<ComputeSunParams>(device_ptr, create_compute_stages(image_dim, use_v2))
{
}

const quavis::ComputeSunParams quavis::ComputeSun::get_parameter()
{
  ComputeSunParams par;
  par.height = image_dim_.y;
  par.width  = image_dim_.x;
  par.r_max  = 1.0f;
  return par;
}

std::vector<quavis::ComputeShaderStage> quavis::ComputeSun::create_compute_stages(const glm::ivec2 &image_dim, const bool use_v2) {
	if (use_v2) { 
		return create_compute_stages_v2(image_dim);
	}
	else {
		return create_compute_stages_v1(image_dim);
	}
}

std::vector<quavis::ComputeShaderStage> quavis::ComputeSun::create_compute_stages_v1(const glm::ivec2 &image_dim)
{
  std::vector<quavis::ComputeShaderStage> stages;
  ComputeShaderStage stage;
  stage.shader_code =
    R"(
		#version 450

		// constants
		#define N_LOCAL 16
		#define PI 3.1415926

		// CIE clear sky model
		#define CIE_A -1
		#define CIE_B -0.25
		#define CIE_C 16
		#define CIE_D -3.0
		#define CIE_E 0.3

		layout (local_size_x = N_LOCAL, local_size_y = 1, local_size_z = 1) in;

		layout (binding = 0, rgba32f) uniform readonly image2DArray colorImage;
		layout (binding = 1, rg32f) uniform readonly image2DArray depthImage;
		// layout (binding = 2) buffer InputBuffer { } input;
		layout (binding = 3) buffer OutputBuffer {
		  float values[];
		} outputs;

		layout(push_constant) uniform Parameters {
			int width;
			int height;
			float sun_azimuth_rad; // 0 = north, 1/2pi = east, pi = south, 3/2pi = west
			float sun_altitude_rad; // 0 = horizon, 1/2pi = zenith
			float zenith_luminance;
		} parameters;

		shared float tmp_local[N_LOCAL];

		vec2 project(vec3 position) {
			float r, theta, phi;
			vec3 nv = position / length(position);

			if (nv.y == nv.x && nv.x == 0) {
				phi = 0;
			}
			else {
				phi = atan(nv.y, nv.x);
			}
			
			theta = asin(nv.z);

			//return vec2(phi * INV_PI, 2 * theta * INV_PI - 1);
			return vec2(phi, theta);
		}

		void main()
		{
			for (int i = 0; i < N_LOCAL; i++) {
				tmp_local[i] = 0.0;
			}
		  uint chunksize = parameters.width/N_LOCAL;

			// CIE calculations
			float Z_s = PI/2.0 - parameters.sun_altitude_rad;
			float phi0 = 1 + CIE_A * exp(CIE_B);
			float fzs = 1 + CIE_C * (exp(CIE_D*Z_s) - exp(CIE_D * PI/2.0)) + CIE_E*pow(cos(Z_s), 2);

		  // compute sum per item
		  uint xpos = gl_LocalInvocationID.x * chunksize;
			float tmp = 0.0;
		  for (uint x = xpos; x < xpos + chunksize; x++) {
			  float i,j,n,m;
			  i = float(x);
			  j = float(gl_WorkGroupID.y);
			  n = float(parameters.width);
			  m = float(parameters.height);
			  float weight =  4.0f/n/m/sqrt(pow((3.0f + 4.0f*j*(j-m)/m/m + 4.0f*i*(i-n)/n/n), 3.0f));

				// compute pixel direction (cartesian)
				float v = 2*(float(x)/parameters.width - 0.5);
				float u = 2*(float(gl_WorkGroupID.y)/parameters.height - 0.5);

				vec2 pv; // pixel direction
				vec4 rgba;
				float Z, chi, fchi, phiz, azimuth, altitude;
				for (uint i = 0; i < 6; i++) {
					if (imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, i)).r != 0) continue;

					if (i == 0) {
						pv = project(vec3(1, v, -u));
					}
					else if (i == 1) {
						pv = project(vec3(-1, v, u));
					}
					else if (i == 2) {
						pv = project(vec3(-v, 1, -u));
					}
					else if (i == 3) {
						pv = project(vec3(v, -1, -u));
					}
					else if (i == 4) {
						pv = project(vec3(u, v, 1));
					}
					else if (i == 5) {
						pv = project(vec3(-u, v, -1));
					}
					// pixel parameters
					azimuth = pv[0];
					altitude = pv[1];

					if (azimuth == 0) continue;
					if (altitude == 0) continue;

					// Compute CIE model
					Z = PI/2.0 - altitude;
					chi = acos(cos(Z_s)*cos(Z) + sin(Z_s)*sin(Z)*cos(abs(azimuth - parameters.sun_azimuth_rad)));
					fchi = 1 + CIE_C * (exp(CIE_D*chi) - exp(CIE_D * PI/2.0)) + CIE_E*pow(cos(chi), 2);
					phiz = 1 + CIE_A * exp(CIE_B / Z);

					float val = fchi*phiz/(fzs*phi0)*parameters.zenith_luminance;
					if (val > 0) tmp += val*weight;
				}
		  }
			tmp_local[gl_LocalInvocationID.x] = tmp;
		  barrier();

		  // group sum
		 for (uint stride = N_LOCAL >> 1; stride > 0; stride >>= 1) {
			if (gl_LocalInvocationID.x < stride) {
			  tmp_local[gl_LocalInvocationID.x] += tmp_local[gl_LocalInvocationID.x + stride];
			}
			barrier();
		 }

		  if (gl_LocalInvocationID.x == 0) {
			outputs.values[gl_WorkGroupID.y] = tmp_local[0];
		  }
		}
		)";
  stage.work_group_size             = glm::vec3(1, image_dim.y, 1);
  stage.input_buffer_size           = 0;
  stage.output_buffer_size          = image_dim.y * sizeof(float);
  stage.has_shader_parameters       = true;
  stage.input_color_cube            = true;
  stage.retrieve_output_buffer_size = 0;  // image_dim.y * sizeof(float);
  stages.push_back(stage);

  stage.shader_code =
    R"(
		#version 450

		// constants
		#define N_LOCAL 16
		#define PI 3.1415926

		layout (local_size_x = 1, local_size_y = N_LOCAL, local_size_z = 1) in;

		layout (binding = 0, rgba32f) uniform readonly image2DArray colorImage;
		// layout (binding = 1, rg32f) uniform readonly image2DArray depthImage;
		layout (binding = 2) buffer InputBuffer { 
			float values[];
		} inputs;

		layout (binding = 3) buffer OutputBuffer {
		  float value;
		} outputs;

		layout(push_constant) uniform Parameters {
			int width;
			int height;
			float sun_azimuth_rad; // 0 = north, 1/2pi = east, pi = south, 3/2pi = west
			float sun_altitude_rad; // 0 = horizon, 1/2pi = zenith
			float zenith_luminance;
		} parameters;

		void main()
		{
		  uint chunksize =  parameters.height/N_LOCAL;
		  uint ypos = gl_LocalInvocationID.y * chunksize;

		  float sum = 0.0;
		  for (uint y = ypos; y < ypos + chunksize; y++) {
			sum += inputs.values[y];
		  }
		  inputs.values[gl_LocalInvocationID.y] = sum;
          barrier();

		  for (uint stride = N_LOCAL >> 1; stride > 0; stride >>= 1) {
			if (gl_LocalInvocationID.y < stride) {
			   inputs.values[gl_LocalInvocationID.y] += inputs.values[gl_LocalInvocationID.y + stride];
			}
		 }
		  outputs.value = inputs.values[0];
		}

		)";
  stage.work_group_size             = glm::vec3(1, 1, 1);
  stage.input_buffer_size           = image_dim.x * sizeof(float);
  stage.output_buffer_size          = 4;
  stage.has_shader_parameters       = true;
  stage.input_color_cube            = true;
  stage.retrieve_output_buffer_size = 4;
  stages.push_back(stage);

  return stages;
}


std::vector<quavis::ComputeShaderStage> quavis::ComputeSun::create_compute_stages_v2(const glm::ivec2 &image_dim)
{
  std::vector<quavis::ComputeShaderStage> stages;
  ComputeShaderStage stage;
  stage.shader_code =
    R"(
		#version 450

		// constants
		#define N_LOCAL 16
		#define PI 3.1415926

		// CIE clear sky model
		#define CIE_A -1
		#define CIE_B -0.25
		#define CIE_C 16
		#define CIE_D -3.0
		#define CIE_E 0.3

		layout (local_size_x = N_LOCAL, local_size_y = 1, local_size_z = 1) in;

		layout (binding = 0, rgba32f) uniform readonly image2DArray colorImage;
		layout (binding = 1, rg32f) uniform readonly image2DArray depthImage;
		// layout (binding = 2) buffer InputBuffer { } input;
		layout (binding = 3) buffer OutputBuffer {
		  float values[];
		} outputs;

		layout(push_constant) uniform Parameters {
			int width;
			int height;
			float sun_azimuth_rad; // 0 = north, 1/2pi = east, pi = south, 3/2pi = west
			float sun_altitude_rad; // 0 = horizon, 1/2pi = zenith
			float zenith_luminance;
		} parameters;

		shared float tmp_local[N_LOCAL];

		vec2 project(vec3 position) {
			float r, theta, phi;
			vec3 nv = position / length(position);

			if (nv.y == nv.x && nv.x == 0) {
				phi = 0;
			}
			else {
				phi = atan(nv.y, nv.x);
			}
			
			theta = asin(nv.z);

			//return vec2(phi * INV_PI, 2 * theta * INV_PI - 1);
			return vec2(phi, theta);
		}

		void main()
		{
			for (int i = 0; i < N_LOCAL; i++) {
				tmp_local[i] = 0.0;
			}
		  uint chunksize = parameters.width/N_LOCAL;

			// CIE calculations
			float Z_s = PI/2.0 - parameters.sun_altitude_rad;
			float phi0 = 1 + CIE_A * exp(CIE_B);
			float fzs = 1 + CIE_C * (exp(CIE_D*Z_s) - exp(CIE_D * PI/2.0)) + CIE_E*pow(cos(Z_s), 2);

		  // compute sum per item
		  uint xpos = gl_LocalInvocationID.x * chunksize;
			float tmp = 0.0;
		  for (uint x = xpos; x < xpos + chunksize; x++) {
			  float i,j,n,m;
			  i = float(x);
			  j = float(gl_WorkGroupID.y);
			  n = float(parameters.width);
			  m = float(parameters.height);
			  float weight =  4.0f/n/m/sqrt(pow((3.0f + 4.0f*j*(j-m)/m/m + 4.0f*i*(i-n)/n/n), 3.0f));

				// compute pixel direction (cartesian)
				float v = 2*(float(x)/parameters.width - 0.5);
				float u = 2*(float(gl_WorkGroupID.y)/parameters.height - 0.5);

				vec2 pv; // pixel direction
				vec4 rgba;
				float Z, chi, fchi, phiz, azimuth, altitude;
				for (uint i = 0; i < 6; i++) {
					if (imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, i)).r != 0) continue;

					if (i == 0) {
						pv = project(vec3(1, v, -u));
					}
					else if (i == 1) {
						pv = project(vec3(-1, v, u));
					}
					else if (i == 2) {
						pv = project(vec3(-v, 1, -u));
					}
					else if (i == 3) {
						pv = project(vec3(v, -1, -u));
					}
					else if (i == 4) {
						pv = project(vec3(u, v, 1));
					}
					else if (i == 5) {
						pv = project(vec3(-u, v, -1));
					}
					// pixel parameters
					azimuth = pv[0];
					altitude = pv[1];

					if (azimuth == 0) continue;
					if (altitude == 0) continue;

					// Compute CIE model
					Z = PI/2.0 - altitude;
					chi = acos(cos(Z_s)*cos(Z) + sin(Z_s)*sin(Z)*cos(abs(azimuth - parameters.sun_azimuth_rad)));
					fchi = 1 + CIE_C * (exp(CIE_D*chi) - exp(CIE_D * PI/2.0)) + CIE_E*pow(cos(chi), 2);
					phiz = 1 + CIE_A * exp(CIE_B / Z);

					float val = fchi*phiz/(fzs*phi0)*parameters.zenith_luminance;
					if (val > 0) tmp += val*weight;
				}
		  }
			tmp_local[gl_LocalInvocationID.x] = tmp;
		  barrier();

		  // group sum
		 for (uint stride = N_LOCAL >> 1; stride > 0; stride >>= 1) {
			if (gl_LocalInvocationID.x < stride) {
			  tmp_local[gl_LocalInvocationID.x] += tmp_local[gl_LocalInvocationID.x + stride];
			}
			barrier();
		 }

		  if (gl_LocalInvocationID.x == 0) {
			outputs.values[gl_WorkGroupID.y] = tmp_local[0];
		  }
		}
		)";
  stage.work_group_size             = glm::vec3(1, image_dim.y, 1);
  stage.input_buffer_size           = 0;
  stage.output_buffer_size          = image_dim.y * sizeof(float);
  stage.has_shader_parameters       = true;
  stage.input_color_cube            = true;
  stage.retrieve_output_buffer_size = 0;  // image_dim.y * sizeof(float);
  stages.push_back(stage);

  stage.shader_code =
    R"(
		#version 450

		// constants
		#define N_LOCAL 16
		#define PI 3.1415926

		layout (local_size_x = 1, local_size_y = N_LOCAL, local_size_z = 1) in;

		layout (binding = 0, rgba32f) uniform readonly image2DArray colorImage;
		// layout (binding = 1, rg32f) uniform readonly image2DArray depthImage;
		layout (binding = 2) buffer InputBuffer { 
			float values[];
		} inputs;

		layout (binding = 3) buffer OutputBuffer {
		  float value;
		} outputs;

		layout(push_constant) uniform Parameters {
			int width;
			int height;
			float sun_azimuth_rad; // 0 = north, 1/2pi = east, pi = south, 3/2pi = west
			float sun_altitude_rad; // 0 = horizon, 1/2pi = zenith
			float zenith_luminance;
		} parameters;

		void main()
		{
		  uint chunksize =  parameters.height/N_LOCAL;
		  uint ypos = gl_LocalInvocationID.y * chunksize;

		  float sum = 0.0;
		  for (uint y = ypos; y < ypos + chunksize; y++) {
			sum += inputs.values[y];
		  }
		  inputs.values[gl_LocalInvocationID.y] = sum;
          barrier();

		  for (uint stride = N_LOCAL >> 1; stride > 0; stride >>= 1) {
			if (gl_LocalInvocationID.y < stride) {
			   inputs.values[gl_LocalInvocationID.y] += inputs.values[gl_LocalInvocationID.y + stride];
			}
		 }
		  outputs.value = inputs.values[0];
		}

		)";
  stage.work_group_size             = glm::vec3(1, 1, 1);
  stage.input_buffer_size           = image_dim.x * sizeof(float);
  stage.output_buffer_size          = 4;
  stage.has_shader_parameters       = true;
  stage.input_color_cube            = true;
  stage.retrieve_output_buffer_size = 4;
  stages.push_back(stage);

  return stages;
}
