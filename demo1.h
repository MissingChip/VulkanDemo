
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
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

OC(typedef) struct VkState {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkDevice device;
    VkSurfaceKHR present_surface;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkSemaphore image_available_semaphore;
    VkSemaphore rendering_finished_semaphore;
    VkSwapchainKHR swapchain;
    int swapchain_size;
    VkCommandPool present_queue_command_pool;
    VkCommandBuffer present_queue_command_buffers[MAX_IMAGE_COUNT];
} OC(VkState);

OC(typedef) struct XState {
    xcb_connection_t* connection;
    xcb_window_t window;
} OC(XState);

int check_instance_extensions();
int check_instance_layers();
int connect_x(XState* x);
int create_instance(VkState* v);
int choose_phys_device(VkState* v);
int choose_queue_family(VkState* v);
int check_device_extensions(VkState* v);
int create_logical_device(VkState* v);
int create_presentation_surface(VkState* v, XState* x);
int create_semaphores(VkState* v);
int create_swapchain(VkState* v);
int create_command_pool(VkState* v);
int create_command_buffers(VkState* v);
int record_command_buffers(VkState* v);
int draw(VkState* v);
int recreate_swapchain(VkState* v);

int disconnect_x(XState* x);
int clean_vulkan_resources(VkState* v);

#ifdef __cplusplus
} // extern "C"
#endif
