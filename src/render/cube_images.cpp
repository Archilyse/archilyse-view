// 
#include "cube_images.h"

#include "./anvil.h"

using namespace std;

namespace quavis {

CubeImages::CubeImages(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2& render_size_, const CubeImages::Usages usage)
  : device_ptr_(device_ptr)
  , render_size_(render_size_)
  , usage_(usage)
{
  // for now switch later derive maybe
  switch (usage_) {
    case quavis::CubeImages::USAGE_COLOR_ATTACHMENT:
      image_format_         = VK_FORMAT_R32G32B32A32_SFLOAT;
      compute_image_format_ = VK_FORMAT_R32G32B32A32_SFLOAT;
      image_usage_          = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
      image_aspects_        = VK_IMAGE_ASPECT_COLOR_BIT;
      image_layout_         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      memory_features_      = 0;
      tiling_               = VK_IMAGE_TILING_OPTIMAL;
      break;
    case quavis::CubeImages::USAGE_DEPTH_ATTACHMENT:
      image_format_         = VK_FORMAT_D32_SFLOAT;
      compute_image_format_ = VK_FORMAT_R32G32_SFLOAT;
      image_usage_          = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      image_aspects_        = VK_IMAGE_ASPECT_DEPTH_BIT;
      image_layout_         = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      memory_features_      = 0;
      tiling_               = VK_IMAGE_TILING_OPTIMAL;
      break;
    case quavis::CubeImages::USAGE_COLOR_TEXTURE:
      image_format_         = VK_FORMAT_R32G32B32A32_SFLOAT;
      compute_image_format_ = VK_FORMAT_R32G32B32A32_SFLOAT;
      image_usage_          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      image_aspects_        = VK_IMAGE_ASPECT_COLOR_BIT;
      image_layout_         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      memory_features_      = 0;
      tiling_               = VK_IMAGE_TILING_OPTIMAL;
      break;
    default:
      throw std::logic_error("usage not implemented");
  }

  images_ = Anvil::Image::create_nonsparse(device_ptr_, VK_IMAGE_TYPE_2D, get_image_format(), tiling_, get_image_usage(), render_size_.x,
                                           render_size_.y, 1, /* in_base_mipmap_depth */
                                           6,                 /* in_n_layers          */
                                           VK_SAMPLE_COUNT_1_BIT, Anvil::QUEUE_FAMILY_GRAPHICS_BIT | Anvil::QUEUE_FAMILY_COMPUTE_BIT,
                                           VK_SHARING_MODE_EXCLUSIVE, false,             /* in_use_full_mipmap_chain */
                                           memory_features_,                             /* in_memory_features  */
                                           Anvil::IMAGE_CREATE_FLAG_CUBE_COMPATIBLE_BIT, /* in_create_flags          */
                                           get_image_layout(), nullptr);
}

CubeImages::CubeImages(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const std::vector<std::shared_ptr<ImageCPU>>& faces)
  : CubeImages(device_ptr, faces[0]->get_dimensions(), Usages::USAGE_COLOR_TEXTURE)
{
  upload_images(faces);
}

void CubeImages::prepare_for_render(std::shared_ptr<Anvil::PrimaryCommandBuffer>& command_buffer, std::shared_ptr<Anvil::Queue>& queue)
{
  auto view = get_view_texture_array();

  Anvil::ImageBarrier image_barrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, true, /* in_by_region_barrier */
                                    this->get_image_layout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, queue->get_queue_family_index(),
                                    queue->get_queue_family_index(), images_, view->get_subresource_range());

  // lets hope the command_buffer is submitted
  image_layout_ = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  command_buffer->record_pipeline_barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_FALSE, /* in_by_region */
                                          0,       /* in_memory_barrier_count        */
                                          nullptr, /* in_memory_barrier_ptrs         */
                                          0,       /* in_buffer_memory_barrier_count */
                                          nullptr, /* in_buffer_memory_barrier_ptrs  */
                                          1,       /* in_image_memory_barrier_count  */
                                          &image_barrier);
}

std::shared_ptr<Anvil::ImageView> CubeImages::get_view_cubemap()
{
  return Anvil::ImageView::create_cube_map(device_ptr_,
                                           images_,  // image memory
                                           0,        // base_layer
                                           0,        // mipmap layer
                                           1,        // mipmap count
                                           get_image_aspects(), images_->get_image_format(), VK_COMPONENT_SWIZZLE_IDENTITY,
                                           VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY);
}

std::shared_ptr<Anvil::ImageView> CubeImages::get_view_single_face(uint32_t face)
{
  if (view_cube_map_ != nullptr) return view_cube_map_;

  return view_cube_map_ = Anvil::ImageView::create_2D(device_ptr_,
                                                      images_,  // image memory
                                                      face,     // base_layer
                                                      0,        // mipmap layer
                                                      1,        // mipmap count
                                                      get_image_aspects(), images_->get_image_format(), VK_COMPONENT_SWIZZLE_IDENTITY,
                                                      VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY);
}

std::shared_ptr<Anvil::ImageView> CubeImages::get_view_texture_array(uint32_t baseLayer /*= 0*/, uint32_t layers /*= 6*/)
{
  if (view_texture_array_ != nullptr) return view_texture_array_;

  return view_texture_array_ =
           Anvil::ImageView::create_2D_array(device_ptr_,
                                             images_,  // image memory
                                             baseLayer, layers, 0, 1, get_image_aspects(), images_->get_image_format(), VK_COMPONENT_SWIZZLE_IDENTITY,
                                             VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY);
}

void CubeImages::clear_images(const glm::vec4& color)
{
  auto device{device_ptr_.lock()};
  auto command_pool{device->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)};

  auto command_buffer{command_pool->alloc_primary_level_command_buffer()};
  command_buffer->start_recording(true, false);

  VkImageSubresourceRange irange{get_subressource_range_cube()};

  switch (usage_) {
    case quavis::CubeImages::USAGE_COLOR_ATTACHMENT:
      VkClearColorValue clearColor;

      clearColor.float32[0] = color.x;
      clearColor.float32[1] = color.y;
      clearColor.float32[2] = color.z;
      clearColor.float32[3] = color.w;
      command_buffer->record_clear_color_image(images_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &irange);
      break;
    case quavis::CubeImages::USAGE_DEPTH_ATTACHMENT:
      VkClearDepthStencilValue clearDepth;
      clearDepth.depth = color.x;
      command_buffer->record_clear_depth_stencil_image(images_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepth, 1, &irange);
      break;
  }

  auto queue = device->get_universal_queue(0);
  command_buffer->stop_recording();
  queue->submit_command_buffer(command_buffer, true, nullptr);
}

std::shared_ptr<Anvil::ImageView> CubeImages::get_storage_image_view(std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer,
                                                                     std::shared_ptr<Anvil::Queue> queue)
{
  if (images_->get_image_usage() & VK_IMAGE_USAGE_STORAGE_BIT && image_format_ == compute_image_format_) {
    auto view = get_view_texture_array();

    Anvil::ImageBarrier image_barrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, true, /* in_by_region_barrier */
                                      this->get_image_layout(), VK_IMAGE_LAYOUT_GENERAL, queue->get_queue_family_index(),
                                      queue->get_queue_family_index(), images_, view->get_subresource_range());

    // lets hope the command_buffer is submitted
    image_layout_ = VK_IMAGE_LAYOUT_GENERAL;

    command_buffer->record_pipeline_barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_FALSE, /* in_by_region */
                                            0,       /* in_memory_barrier_count        */
                                            nullptr, /* in_memory_barrier_ptrs         */
                                            0,       /* in_buffer_memory_barrier_count */
                                            nullptr, /* in_buffer_memory_barrier_ptrs  */
                                            1,       /* in_image_memory_barrier_count  */
                                            &image_barrier);

    return view;
  }

  auto staging = Anvil::Image::create_nonsparse(
    device_ptr_, VK_IMAGE_TYPE_2D, compute_image_format_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    render_size_.x, render_size_.y, 1,                                                        /* in_base_mipmap_depth */
    8,                                                                                        /* in_n_layers          */
    VK_SAMPLE_COUNT_1_BIT, Anvil::QUEUE_FAMILY_COMPUTE_BIT, VK_SHARING_MODE_EXCLUSIVE, false, /* in_use_full_mipmap_chain */
    0,                                                                                        /* in_memory_features  */
    0,                                                                                        /* in_create_flags          */
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, nullptr);

  VkImageBlit region;

  region.srcOffsets[0].x = 0;
  region.srcOffsets[0].y = 0;
  region.srcOffsets[0].z = 0;

  region.srcOffsets[1].x = render_size_.x;
  region.srcOffsets[1].y = render_size_.y;
  region.srcOffsets[1].z = 5;

  region.dstOffsets[0] = region.srcOffsets[0];
  region.dstOffsets[1] = region.srcOffsets[1];

  region.srcSubresource.aspectMask     = get_image_aspects();
  region.srcSubresource.baseArrayLayer = 0;
  region.srcSubresource.layerCount     = 6;
  region.srcSubresource.mipLevel       = 0;
  region.dstSubresource                = region.srcSubresource;

  command_buffer->record_blit_image(images_, image_layout_, staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);

  auto view = Anvil::ImageView::create_2D_array(device_ptr_,
                                                staging,  // image memory
                                                0, 6, 0, 1, get_image_aspects(), staging->get_image_format(), VK_COMPONENT_SWIZZLE_IDENTITY,
                                                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY);

  Anvil::ImageBarrier image_barrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, true, /* in_by_region_barrier */
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, queue->get_queue_family_index(),
                                    queue->get_queue_family_index(), staging, view->get_subresource_range());

  command_buffer->record_pipeline_barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_FALSE, /* in_by_region */
                                          0,       /* in_memory_barrier_count        */
                                          nullptr, /* in_memory_barrier_ptrs         */
                                          0,       /* in_buffer_memory_barrier_count */
                                          nullptr, /* in_buffer_memory_barrier_ptrs  */
                                          1,       /* in_image_memory_barrier_count  */
                                          &image_barrier);

  return view;
}

// TODO much better implementation

std::shared_ptr<ImageCPU> CubeImages::retrieve_images(bool pretty)
{
  auto device{device_ptr_.lock()};

  auto command_pool{device->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)};
  auto queue = device->get_universal_queue(0);

  // 		auto command_buffer{ command_pool->alloc_primary_level_command_buffer() };
  // 		command_buffer->start_recording(true, false);

  images_->change_image_layout(queue, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, image_layout_, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, get_subressource_range_cube());
  image_layout_ = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

  auto flat = get_flattened(pretty);

  queue = device->get_universal_queue(0);
  flat->change_image_layout(queue, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_IMAGE_LAYOUT_GENERAL, flat->get_subresource_range());

  const auto memory_req = flat->get_memory_requirements();
  auto memory           = flat->get_memory_block();

  auto dim   = flat->get_image_extent_2D(0);
  auto image = std::make_shared<ImageCPU>(glm::ivec2(dim.width, dim.height));

  // memory->map(0, memory_req.size, &mapedPixels);
  // memcpy(&pixels[0], mapedPixels, memory_req.size);
  // memory->unmap();
  memory->read(0, memory_req.size, image->get_pixel_data());

  return image;
}

std::shared_ptr<Anvil::Image> CubeImages::get_flattened(bool nice)
{
  auto staging = Anvil::Image::create_nonsparse(
    device_ptr_, VK_IMAGE_TYPE_2D, get_image_format(), VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    nice ? render_size_.x * 4 : render_size_.x * 3, nice ? render_size_.y * 3 : render_size_.y * 2, 1, /* in_base_mipmap_depth */
    1,                                                                                                 /* in_n_layers          */
    VK_SAMPLE_COUNT_1_BIT, Anvil::QUEUE_FAMILY_GRAPHICS_BIT, VK_SHARING_MODE_EXCLUSIVE, false,         /* in_use_full_mipmap_chain */
    Anvil::MEMORY_FEATURE_FLAG_MAPPABLE,                                                               /* in_memory_features  */
    0,                                                                                                 /* in_create_flags          */
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, nullptr);

  auto device{device_ptr_.lock()};

  auto command_pool{device->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)};
  auto queue = device->get_universal_queue(0);

  auto command_buffer{command_pool->alloc_primary_level_command_buffer()};
  command_buffer->start_recording(true, false);

  std::vector<VkImageCopy> regions;
  VkImageCopy region;
  region.srcOffset.x               = 0;
  region.srcOffset.y               = 0;
  region.srcOffset.z               = 0;
  region.dstOffset.z               = 0;
  region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = get_image_aspects();
  region.dstSubresource.baseArrayLayer                                = 0;
  region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
  region.srcSubresource.mipLevel = region.dstSubresource.mipLevel = 0;
  region.extent.depth                                             = 1;
  region.extent.width                                             = render_size_.y;
  region.extent.height                                            = render_size_.x;

  if (nice) {
    for (auto i = 0; i < 6; i++) {
      switch (i) {
        case 0:
          region.dstOffset.x = 2 * render_size_.x;
          region.dstOffset.y = 1 * render_size_.y;
          break;
        case 1:
          region.dstOffset.x = 0 * render_size_.x;
          region.dstOffset.y = 1 * render_size_.y;
          break;
        case 2:
          region.dstOffset.x = 1 * render_size_.x;
          region.dstOffset.y = 0 * render_size_.y;
          break;
        case 3:
          region.dstOffset.x = 1 * render_size_.x;
          region.dstOffset.y = 2 * render_size_.y;
          break;
        case 4:
          region.dstOffset.x = 1 * render_size_.x;
          region.dstOffset.y = 1 * render_size_.y;
          break;
        case 5:
          region.dstOffset.x = 3 * render_size_.x;
          region.dstOffset.y = 1 * render_size_.y;
          break;
      }
      region.srcSubresource.baseArrayLayer = i;

      regions.push_back(region);  // copy happens
    }
  } else {
    for (auto i = 0; i < 6; i++) {
      region.dstOffset.x                   = (i % 3) * render_size_.x;
      region.dstOffset.y                   = (i / 3) * render_size_.y;
      region.srcSubresource.baseArrayLayer = i;

      regions.push_back(region);  // copy happens
    }
  }

  command_buffer->record_copy_image(images_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    static_cast<uint32_t>(regions.size()), regions.data());

  command_buffer->stop_recording();

  queue->submit_command_buffer(command_buffer, true, nullptr);

  return staging;
}

void CubeImages::upload_images(const std::vector<std::shared_ptr<ImageCPU>>& faces)
{
  assert(faces.size() == 6);

  // create a flat image all images below (behind) each other
  // if max image size limit is reached (prob 16k / 6) ~= 2048 then rewrite this algo
  auto flat = Anvil::Image::create_nonsparse(
    device_ptr_, VK_IMAGE_TYPE_2D, get_image_format(), VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    render_size_.x, render_size_.y * 6, 1,                                                     /* in_base_mipmap_depth */
    1,                                                                                         /* in_n_layers          */
    VK_SAMPLE_COUNT_1_BIT, Anvil::QUEUE_FAMILY_GRAPHICS_BIT, VK_SHARING_MODE_EXCLUSIVE, false, /* in_use_full_mipmap_chain */
    Anvil::MEMORY_FEATURE_FLAG_MAPPABLE,                                                       /* in_memory_features  */
    0,                                                                                         /* in_create_flags          */
    VK_IMAGE_LAYOUT_GENERAL, nullptr);

  auto device{device_ptr_.lock()};

  auto command_pool{device->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)};
  auto queue = device->get_universal_queue(0);

  auto command_buffer{command_pool->alloc_primary_level_command_buffer()};
  command_buffer->start_recording(true, false);

  // copy to flat image
  auto memory         = flat->get_memory_block();
  VkDeviceSize offset = 0;

  for (const auto& face : faces) {
    assert(render_size_ == face->get_dimensions());
    memory->write(offset, face->image_byte_size(), face->get_pixel_data());
    offset += face->image_byte_size();
  }

  images_->change_image_layout(queue, VK_ACCESS_SHADER_READ_BIT, image_layout_, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               get_subressource_range_cube());

  // now copy it to the cube faces
  std::vector<VkImageCopy> regions;
  VkImageCopy region;
  region.dstOffset.x               = 0;
  region.dstOffset.y               = 0;
  region.dstOffset.z               = 0;
  region.srcOffset.x               = 0;
  region.srcOffset.z               = 0;
  region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = get_image_aspects();
  region.srcSubresource.baseArrayLayer                                = 0;
  region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
  region.srcSubresource.mipLevel = region.dstSubresource.mipLevel = 0;
  region.extent.depth                                             = 1;
  region.extent.width                                             = render_size_.y;
  region.extent.height                                            = render_size_.x;

  for (auto i = 0; i < 6; i++) {
    region.srcOffset.y                   = i * render_size_.y;
    region.dstSubresource.baseArrayLayer = i;

    regions.push_back(region);  // copy happens
  }

  command_buffer->record_copy_image(flat, VK_IMAGE_LAYOUT_GENERAL, images_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    static_cast<uint32_t>(regions.size()), regions.data());
  command_buffer->stop_recording();

  queue->submit_command_buffer(command_buffer, true, nullptr);

  images_->change_image_layout(queue, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, image_layout_,
                               get_subressource_range_cube());
}

const VkFormat CubeImages::get_image_format() const
{
  return image_format_;
}

const VkImageUsageFlags CubeImages::get_image_usage() const
{
  return image_usage_;
}

const VkImageAspectFlagBits CubeImages::get_image_aspects() const
{
  return image_aspects_;
}

const VkImageLayout CubeImages::get_image_layout() const
{
  return image_layout_;
}

VkImageSubresourceRange CubeImages::get_subressource_range_cube() const
{
  VkImageSubresourceRange range;
  range.aspectMask     = get_image_aspects();
  range.baseArrayLayer = 0;
  range.baseMipLevel   = 0;
  range.layerCount     = 6;
  range.levelCount     = 1;

  return range;
}

VkImageSubresourceRange CubeImages::get_subressource_range_face(uint32_t face) const
{
  VkImageSubresourceRange range;
  range.aspectMask     = get_image_aspects();
  range.baseArrayLayer = face;
  range.baseMipLevel   = 0;
  range.layerCount     = 1;
  range.levelCount     = 1;

  return range;
}

VkImageSubresourceRange CubeImages::get_subressource_range_face(uint32_t baseLayer, uint32_t layers) const
{
  VkImageSubresourceRange range;
  range.aspectMask     = get_image_aspects();
  range.baseArrayLayer = baseLayer;
  range.baseMipLevel   = 0;
  range.layerCount     = layers;
  range.levelCount     = 1;

  return range;
}

}  // namespace quavis
