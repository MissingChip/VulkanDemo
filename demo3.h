
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>       
#include <sys/types.h>
#include <sys/stat.h>
#include <xcb/xcb.h>
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xcb.h>

#ifdef __cplusplus
#define TYPEDEF
#define OC(X)
extern "C" {
#else
#define TYPEDEF typedef
#define OC(X) X
#endif

#define VV(X) if((X) != VK_SUCCESS)

#define MAX_IMAGE_COUNT 16
#define VK_API_VERSION VK_MAKE_VERSION(1, 0, 3)
#define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
#define WINDOW_MANAGER_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#define VSWI VK_COMPONENT_SWIZZLE_IDENTITY
#define VSTRUCT(NAME) .sType = VK_STRUCTURE_TYPE_##NAME##_CREATE_INFO, .pNext = NULL, .flags = 0,
#define VSTRUCTF(NAME) .sType = VK_STRUCTURE_TYPE_##NAME##_CREATE_INFO, .pNext = NULL,

#define BAD UINT32_MAX
#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define arraysize(X) (sizeof(X)/sizeof(*X))

#ifndef NDEBUG
#define ODBG(X) X
inline void vdbg(const char* fmt, ...){
    va_list argptr;
    va_start(argptr, fmt);
    char str[16];
    int w = vsnprintf(str, 16, fmt, argptr);
    va_end(argptr);
    va_start(argptr, fmt);
    char new_str[w+1];
    vsnprintf(new_str, w+1, fmt, argptr);
    va_end(argptr);
    printf("%s\n", new_str);
}
inline void verr(const char* fmt, ...){
    va_list argptr;
    va_start(argptr, fmt);
    char str[16];
    int w = vsnprintf(str, 16, fmt, argptr);
    va_end(argptr);
    va_start(argptr, fmt);
    char new_str[w+1];
    vsnprintf(new_str, w+1, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "%s\n", new_str);
    exit(1);
}
#define VDBG(...) vdbg("=DEBUG " __FILE__ ":" S__LINE__ "= " __VA_ARGS__)
#define VERR(...) verr("[VULKAN ERROR " __FILE__ ":" S__LINE__ "]: " __VA_ARGS__)
#else
#define ODBG(X)
#define VDBG(X)
inline void vdbg(const char* fmt, ...){
}
inline void verr(const char* fmt, ...){
    va_list argptr;
    va_start(argptr, fmt);
    char str[16];
    int w = vsnprintf(str, 16, fmt, argptr);
    va_end(argptr);
    va_start(argptr, fmt);
    char new_str[w+1];
    vsnprintf(new_str, w+1, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "%s\n", new_str);
    exit(1);
}
#define VDBG(...)
#define VERR(...) verr("[VULKAN ERROR " __FILE__ ":" S__LINE__ "]: " __VA_ARGS__)
#endif

OC(typedef) struct VirtFrame {
    VkCommandBuffer command_buffer;
    VkFramebuffer framebuffer;
    VkFence fence;
    VkSemaphore available;
    VkSemaphore finished;
} OC(VirtFrame);

OC(typedef) struct VkState {
    VkInstance instance;
    float width, height;
    VkPhysicalDevice physical_device;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkDevice device;
    VkSurfaceKHR present_surface;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    int swapchain_size;
    VkCommandPool command_pool;
    // VkCommandBuffer present_command_buffers[MAX_IMAGE_COUNT];
    // VkCommandBuffer graphics_command_buffers[MAX_IMAGE_COUNT];
    VkImageView image_views[MAX_IMAGE_COUNT];
    VkFramebuffer framebuffers[MAX_IMAGE_COUNT];
    VkRenderPass render_pass;
    // VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    size_t vertex_buffer_size;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_memory;
    int nvirt_frames;
    int ivirt_frame;
    VirtFrame virt_frames[MAX_IMAGE_COUNT];
} OC(VkState);

OC(typedef) struct XState {
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_intern_atom_reply_t* delete_window_atom;
} OC(XState);


OC(typedef) struct VertexData {
    float   x, y, z, w;
    float   r, g, b, a;
} OC(VertexData);

typedef uint32_t u32;

int check_instance_extensions();
int check_instance_layers();
int connect_x(VkState* v, XState* x);
int update_x(VkState* v, XState* x);
int create_instance(VkState* v);
int choose_phys_device(VkState* v);
int choose_queue_family(VkState* v);
int check_device_extensions(VkState* v);
int create_logical_device(VkState* v);
int create_presentation_surface(VkState* v, XState* x);
int create_semaphores(VkState* v);
int create_swapchain(VkState* v);
int create_command_pool(VkState* v);
// int create_command_buffers(VkState* v);
// int record_command_buffers(VkState* v);
int create_render_pass(VkState* v);
VkShaderModule create_shader_module(VkState* v, const char* filename);
int create_pipeline(VkState* v);
int prepare_frame(VkState* v, VirtFrame* current_resource, uint32_t image_index);
int create_framebuffer(VkState* v, VkFramebuffer* framebuffer, VkImageView image_view);
int create_vertex_buffer(VkState* v);
int allocate_buffer_memory(VkState* v);
int draw(VkState* v);
int recreate_swapchain(VkState* v);

int disconnect_x(XState* x);
int clean_vulkan_resources(VkState* v);

#ifdef __cplusplus
} // extern "C"
#endif
