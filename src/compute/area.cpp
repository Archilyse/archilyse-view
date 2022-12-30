#include "area.h"

using namespace quavis;

quavis::ComputeArea::ComputeArea(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2 &image_dim)
  : image_dim_{image_dim}
  , ComputeBaseGPU<ComputeAreaParams>(device_ptr, create_compute_stages(image_dim))
{
}

const quavis::ComputeAreaParams quavis::ComputeArea::get_parameter()
{
  ComputeAreaParams par;
  par.height = image_dim_.y;
  par.width  = image_dim_.x;
  par.r_max  = 1.0f;
  return par;
}

std::vector<quavis::ComputeShaderStage> quavis::ComputeArea::create_compute_stages(const glm::ivec2 &image_dim)
{
  std::vector<quavis::ComputeShaderStage> stages;
  ComputeShaderStage stage;
  stage.shader_code =
    R"(
		#version 450

		// constants
		#define N_LOCAL 16
		#define PI 3.1415926

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
			float r_max;
		} parameters;

		shared float tmp_local[N_LOCAL];

		void main()
		{
		  uint chunksize = parameters.width/N_LOCAL;

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

			  float d0 = imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, 0)).a > 0 ? 1 : 0;
			  float d1 = imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, 1)).a > 0 ? 1 : 0;
			  float d2 = imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, 2)).a > 0 ? 1 : 0;
			  float d3 = imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, 3)).a > 0 ? 1 : 0;
			  float d4 = imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, 4)).a > 0 ? 1 : 0;
			  float d5 = imageLoad(colorImage, ivec3(x, gl_WorkGroupID.y, 5)).a > 0 ? 1 : 0;

			  tmp += (d0+d1+d2+d3+d4+d5)*weight;
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
			float r_max;
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
