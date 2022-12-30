#include "groups.h"
#include <string>
#include <regex>
#include <iostream>

using namespace quavis;

quavis::ComputeGroups::ComputeGroups(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2 &image_dim)
  : image_dim_{image_dim}
  , ComputeBaseGPU<ComputeGroupsParams>(device_ptr, create_compute_stages(image_dim))
{
}

const quavis::ComputeGroupsParams quavis::ComputeGroups::get_parameter()
{
  ComputeGroupsParams par;
  par.height = image_dim_.y;
  par.width  = image_dim_.x;
	//par.vv = {1.0,0.0,0.0};
	par.field_of_view = 360.0;
  return par;
}

std::vector<quavis::ComputeShaderStage> quavis::ComputeGroups::create_compute_stages(const glm::ivec2 &image_dim)
{
	int max_groups = 32;
  std::vector<quavis::ComputeShaderStage> stages;
  ComputeShaderStage stage;
  stage.shader_code =
    R"(
		#version 450

		// constants
		#define N_LOCAL 16
		#define PI 3.1415926
		#define INV_PI 0.31830988618
		#define MAX_GROUPS 32

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
			float field_of_view;
			float view_x;
			float view_y;
			float view_z;
		} parameters;

		shared float tmp_local[N_LOCAL*MAX_GROUPS];

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
			for (int i = 0; i < N_LOCAL*MAX_GROUPS; i++) {
				tmp_local[i] = 0.0;
			}
		  uint chunksize = parameters.width/N_LOCAL;

			// parameters used to restrict the field of view
			float normalized_fov = 2 * parameters.field_of_view / 360.0;

			// compute view direction (polar)
			vec3 view_position = vec3(parameters.view_x, parameters.view_y, parameters.view_z);
			vec2 vv = project(view_position); // view direction
			
		  // compute sum per item
		  uint xpos = gl_LocalInvocationID.x * chunksize;
		  float tmp = 0.0;
		  for (uint x = xpos; x < xpos + chunksize; x++) {
			  float i,j,n,m;
			  i = float(x);
			  j = float(gl_WorkGroupID.y);
			  n = float(parameters.width);
			  m = float(parameters.height);
			  float weight = 4.0f/n/m/sqrt(pow((3.0f + 4.0f*j*(j-m)/m/m + 4.0f*i*(i-n)/n/n), 3.0f));
				//float k = log2(max(1, pow(MAX_GROUPS, 1.0 / 3.0) - 1));
        //float basis = round(max(2, pow(2, k) + 1));
				
				// compute pixel direction (cartesian)
				float u = 2*(float(x)/parameters.width - 0.5);
				float v = 2*(float(gl_WorkGroupID.y)/parameters.height - 0.5);

				float value;
				highp int bucket;
				vec4 rgba;
				vec2 pv; // pixel direction
				for (uint i = 0; i < 6; i++) {
					if (i == 0) {
						// front
						pv = project(vec3(1, u, v));
					}
					else if (i == 1) {
						// back
						pv = project(vec3(-1, -u, v));
					}
					else if (i == 2) {
						// right
						pv = project(vec3(-u, 1, v));
					}
					else if (i == 3) {
						// left
						pv = project(vec3(u, -1, v));
					}
					else if (i == 4) {
						// up
						pv = project(vec3(-v, u, 1));
					}
					else if (i == 5) {
						// down
						pv = project(vec3(v, u, -1));
					}

					//float dx = abs(pv.x - vv.x);
					//float dy = abs(pv.y - vv.y);

					float delta = acos(sin(pv.y)*sin(vv.y) + cos(pv.y)*cos(vv.y)*cos(pv.x-vv.x));
					if (delta <= parameters.field_of_view / 180.0 * 3.1415926) {
						rgba = imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, i));
			  		value = rgba.a > 0 ? weight : 0;
						bucket = int(round(rgba.r));
						//bucket = int(round(rgba.r * (basis-1) + rgba.g * (basis-1) * basis + rgba.b * (basis-1) * pow(basis, 2)));
						tmp_local[gl_LocalInvocationID.x*MAX_GROUPS + bucket] += value;
					}
				}
		  }
		  barrier();

			// group sum
			for (uint stride = N_LOCAL >> 1; stride > 0; stride >>= 1) {
				if (gl_LocalInvocationID.x < stride) {
					for (uint i = 0; i < MAX_GROUPS; i++) {
						tmp_local[gl_LocalInvocationID.x*MAX_GROUPS + i] += tmp_local[(gl_LocalInvocationID.x + stride)*MAX_GROUPS + i];
					}
				}
				barrier();
			}

		  if (gl_LocalInvocationID.x == 0) {
				for (uint i = 0; i < MAX_GROUPS; i++) {
					outputs.values[gl_WorkGroupID.y*MAX_GROUPS + i] = tmp_local[i];
				}
		  }
		}
		)";
  stage.work_group_size             = glm::vec3(1, image_dim.y, 1);
  stage.input_buffer_size           = 0;
  stage.output_buffer_size          = image_dim.y * sizeof(float) * max_groups;
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
		#define MAX_GROUPS 32

		layout (local_size_x = 1, local_size_y = N_LOCAL, local_size_z = 1) in;

		layout (binding = 0, rgba32f) uniform readonly image2DArray colorImage;
		// layout (binding = 1, rg32f) uniform readonly image2DArray depthImage;
		layout (binding = 2) buffer InputBuffer { 
			float values[];
		} inputs;

		layout (binding = 3) buffer OutputBuffer {
		  float values[];
		} outputs;

		layout(push_constant) uniform Parameters {
			int width;
			int height;
			float field_of_view;
			float view_x;
			float view_y;
			float view_z;
		} parameters;

		void main()
		{
		  uint chunksize =  parameters.height/N_LOCAL;
		  uint ypos = gl_LocalInvocationID.y * chunksize;

		  float sum[MAX_GROUPS];
			for (uint i = 0; i < MAX_GROUPS; i++) {
				sum[i] = 0.0;
			}

		  for (uint y = ypos; y < ypos + chunksize; y++) {
				for (uint i = 0; i < MAX_GROUPS; i++) {
					sum[i] += inputs.values[y*MAX_GROUPS + i];
				}
		  }

			for (uint i = 0; i < MAX_GROUPS; i++) {
				inputs.values[gl_LocalInvocationID.y*MAX_GROUPS + i] = sum[i];
			}

		  for (uint stride = N_LOCAL >> 1; stride > 0; stride >>= 1) {
				if (gl_LocalInvocationID.y < stride) {
					for (uint i = 0; i < MAX_GROUPS; i++) {
						inputs.values[gl_LocalInvocationID.y*MAX_GROUPS + i] += inputs.values[(gl_LocalInvocationID.y + stride)*MAX_GROUPS + i];
					}
				}
		  }

		  for (uint i = 0; i < MAX_GROUPS; i++) {
		 		outputs.values[i] = inputs.values[i];
		  }
		}

		)";
  stage.work_group_size             = glm::vec3(1, 1, 1);
  stage.input_buffer_size           = image_dim.x * sizeof(float) * max_groups;
  stage.output_buffer_size          = 4 * max_groups;
  stage.has_shader_parameters       = true;
  stage.input_color_cube            = true;
  stage.retrieve_output_buffer_size = 4 * max_groups;
  stages.push_back(stage);

  return stages;
}
