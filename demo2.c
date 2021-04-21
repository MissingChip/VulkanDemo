
#include "demo2.h"

int main(int argc, char** argv){
    VkState v;
    XState x;
    int c = 0;
    check_instance_extensions();
    check_instance_layers();
    connect_x(&v, &x);
    create_instance(&v);
    create_presentation_surface(&v, &x);
    choose_phys_device(&v);
    choose_queue_family(&v);
    create_logical_device(&v);
    create_semaphores(&v);
    create_swapchain(&v);
    create_render_pass(&v);
    create_framebuffers(&v);
    create_pipeline(&v);
    create_command_pool(&v);
    create_command_buffers(&v);
    draw(&v);

    getchar();
    disconnect_x(&x);
    clean_vulkan_resources(&v);
}

const char* required_instance_extensions[] = {
    WINDOW_MANAGER_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME//,
    // VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const uint32_t required_instance_extension_count = arraysize(required_instance_extensions);

const char* required_instance_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};
const uint32_t required_instance_layer_count = arraysize(required_instance_layers);

int check_instance_extensions(){
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties( NULL, &extension_count, NULL);
    VkExtensionProperties extension_properties[extension_count];
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_properties);

    for(int e = 0; e < required_instance_extension_count; e++){
        const char* ext = required_instance_extensions[e];
        int found = 0;
        for(uint32_t i = 0; i < extension_count; i++){
            if(strcmp(ext, extension_properties[i].extensionName) == 0){
                found = 1;
                break;
            }
        }
        if(!found)
            VERR("Could not find extension: %s", ext);
        else
            VDBG("Found extension: %s", ext);
    }
}

int check_instance_layers(){
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties( &layer_count, NULL );
    VkLayerProperties layer_properties[layer_count];
    vkEnumerateInstanceLayerProperties( &layer_count, layer_properties );

    for(int e = 0; e < required_instance_layer_count; e++){
        const char* ext = required_instance_layers[e];
        int found = 0;
        for(uint32_t i = 0; i < layer_count; i++){
            if(strcmp(ext, layer_properties[i].layerName) == 0){
                found = 1;
                break;
            }
        }
        if(!found)
            VERR("Could not find layer: %s", ext);
        else
            VDBG("Found layer: %s", ext);
    }
}

int connect_x(VkState* v, XState* x){
    x->connection = xcb_connect(NULL, NULL);

    const xcb_setup_t      *setup  = xcb_get_setup (x->connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
    xcb_screen_t           *screen = iter.data;
    v->width = 640;
    v->height = 480;
    x->window = xcb_generate_id(x->connection); 
    xcb_create_window(
        x->connection,
        XCB_COPY_FROM_PARENT,
        x->window,
        screen->root,
        0, 0,
        v->width, v->height,
        10,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        0, NULL
    );
    xcb_map_window(x->connection, x->window);
    xcb_flush (x->connection);
}

int disconnect_x(XState* x){
    xcb_disconnect(x->connection);
}

int create_instance(VkState* v){

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "Vulkan Application Name",
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .pEngineName = "Vulkan Engine Name",
        .engineVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .apiVersion = VK_API_VERSION
    };
    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = required_instance_layer_count,
        .ppEnabledLayerNames = required_instance_layers,
        .enabledExtensionCount = required_instance_extension_count,
        .ppEnabledExtensionNames = required_instance_extensions
    };

    VDBG("Creating instance");
    VV(vkCreateInstance(
        &instance_create_info, 
        NULL, 
        &v->instance
    )) VERR("Instance creation failed");
    v->swapchain = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

int create_presentation_surface(VkState* v, XState* x){
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
        VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        NULL,
        0,
        x->connection,
        x->window
    };
    VV(
        vkCreateXcbSurfaceKHR(v->instance, &surface_create_info, NULL, &v->present_surface)
    ) VERR("Could not create presentation surface");
}

const char* required_device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const uint32_t required_device_extension_count = arraysize(required_device_extensions);

int choose_phys_device(VkState* v){
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(v->instance, &device_count, NULL);
    VkPhysicalDevice physical_devices[device_count];
    vkEnumeratePhysicalDevices(v->instance, &device_count, physical_devices);
    if(device_count == 0) VERR("No devices to choose from");
    
    VkPhysicalDeviceProperties physical_device_properties[device_count];
    VkPhysicalDeviceFeatures physical_device_features[device_count];
    VkBool32 acceptable_devices[device_count];
    for(int i = 0; i < device_count; i++){
        // Do something fancy to decide which device to use
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_properties[i]);
        vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_device_features[i]);

        // uint32_t extension_count = 0;
        // vkEnumerateDeviceExtensionProperties( physical_devices[i], NULL, &extension_count, NULL);
        // VkExtensionProperties extensions[extension_count];
        // vkEnumerateDeviceExtensionProperties( physical_devices[i], NULL, &extension_count, extensions);
        // for(int e = 0; e < required_device_extension_count; e++){
        //     const char* ext = required_device_extensions[e];
        //     int found = 0;
        //     for(uint32_t i = 0; i < extension_count; i++){
        //         if(strcmp(ext, extensions[i].extensionName) == 0){
        //             found = 1;
        //             break;
        //         }
        //     }
        //     if(!found)
        //         VERR("Could not find extension: %s", ext);
        //     else
        //         VDBG("Found extension: %s", ext);
        // }

        // if( VK_VERSION_MAJOR( physical_device_properties[i].apiVersion ) < VK_VERSION_MAJOR( VK_API_VERSION ) ||
        //     physical_device_properties[i].limits.maxImageDimension2D < 4096){
        //         VDBG("Device %d not supported", i);
        // }

        // uint32_t queue_family_count;
        // vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);

        VDBG("Device %d: \t%s", i, physical_device_properties[i].deviceName);
    }
    v->physical_device = physical_devices[0];
    return VK_SUCCESS;
}

int choose_queue_family(VkState* v){
    v->graphics_queue_family_index = BAD;
    v->present_queue_family_index = BAD;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        v->physical_device, 
        &queue_family_count, 
        NULL
    );
    VkQueueFamilyProperties queue_family_properties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(
        v->physical_device, 
        &queue_family_count, 
        queue_family_properties
    );
    VDBG("%d queues available", queue_family_count);

    uint32_t graphics_queue_family_index = BAD;
    uint32_t present_queue_family_index = BAD;
    VkBool32 queue_present_support[queue_family_count];
    for( uint32_t i = 0; i < queue_family_count; ++i ) {
        vkGetPhysicalDeviceSurfaceSupportKHR( v->physical_device, i, v->present_surface, &queue_present_support[i] );

        if( (queue_family_properties[i].queueCount > 0) &&
            (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ) {
            // Select first queue that supports graphics
            if( graphics_queue_family_index == BAD ) {
                graphics_queue_family_index = i;
            }

            // If there is queue that supports both graphics and present - prefer it
            if( queue_present_support[i] ) {
                v->graphics_queue_family_index = i;
                v->graphics_queue_family_index = i;
                break;
            }
        }
    }
    for( uint32_t i = 0; i < queue_family_count && present_queue_family_index == BAD; ++i ) {
        if(queue_present_support[i]){
            present_queue_family_index = i;
            break;
        }
    }
    if(graphics_queue_family_index == BAD || present_queue_family_index == BAD){
        ODBG(VERR("Could not find graphics/present queue"));
        return !VK_SUCCESS;
    }

    VDBG("Queues: \tGraphics: %d \tPresent: %d", graphics_queue_family_index, present_queue_family_index);

    v->graphics_queue_family_index = graphics_queue_family_index;
    v->present_queue_family_index = present_queue_family_index;
    return VK_SUCCESS;
}

int create_logical_device(VkState* v){
    VDBG("Creating logical device");
    float queue_priorities[] = {1.0};
    uint32_t number_of_queues = 1 + (v->graphics_queue_family_index != v->present_queue_family_index);
    VkDeviceQueueCreateInfo device_queue_create_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = v->graphics_queue_family_index,
            .queueCount = arraysize(queue_priorities),
            .pQueuePriorities = queue_priorities
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = v->present_queue_family_index,
            .queueCount = arraysize(queue_priorities),
            .pQueuePriorities = queue_priorities
        }
    };

    VkDeviceCreateInfo device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        NULL,
        0,
        number_of_queues,
        device_queue_create_infos,
        0,
        NULL,
        required_device_extension_count,
        required_device_extensions,
        NULL
    };
    VV(vkCreateDevice(
        v->physical_device,
        &device_create_info,
        NULL,
        &v->device
    )) VERR("Device creation failed");
    vkGetDeviceQueue( v->device, v->graphics_queue_family_index, 0, &v->graphics_queue);
    vkGetDeviceQueue( v->device, v->present_queue_family_index, 0, &v->present_queue);
    return VK_SUCCESS;
}

int create_semaphores(VkState* v){
    VkSemaphoreCreateInfo semaphore_create_info = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        NULL,
        0
    };
    vkCreateSemaphore( v->device, &semaphore_create_info, NULL, &v->image_available_semaphore);
    vkCreateSemaphore( v->device, &semaphore_create_info, NULL, &v->rendering_finished_semaphore);
}

int create_swapchain(VkState* v){
    VkResult r;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( v->physical_device, v->present_surface, &surface_capabilities);
    VV(r) VERR("Getting surface capabilities");
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( v->physical_device, v->present_surface, &format_count, NULL);
    VkSurfaceFormatKHR surface_formats[format_count];
    r = vkGetPhysicalDeviceSurfaceFormatsKHR( v->physical_device, v->present_surface, &format_count, surface_formats);
    VV(r) VERR("Getting surface formats");

    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if( image_count < 3 ) image_count = 3;
    if( (surface_capabilities.maxImageCount > 0) &&
            (image_count > surface_capabilities.maxImageCount) ) {
        image_count = surface_capabilities.maxImageCount;
    }

    VkSurfaceFormatKHR surface_format;
    if( (format_count == 1) &&
            (surface_formats[0].format == VK_FORMAT_UNDEFINED) ) {
        surface_format = (VkSurfaceFormatKHR){ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }
    else{
        VkBool32 found = VK_FALSE;
        for(int i = 0; i < format_count; i++){
            if(surface_formats[i].format == VK_FORMAT_R8G8B8_UNORM) {
                surface_format = surface_formats[i];
                found = VK_TRUE;
                break;
            }
        }
        if(found == VK_FALSE){
            surface_format = surface_formats[0];
        }
    }

    VkExtent2D swapchain_extent = { v->width, v->height };
    if( surface_capabilities.currentExtent.width == -1 ) {
        if( swapchain_extent.width < surface_capabilities.minImageExtent.width )
            swapchain_extent.width = surface_capabilities.minImageExtent.width;
        if( swapchain_extent.height < surface_capabilities.minImageExtent.height )
            swapchain_extent.height = surface_capabilities.minImageExtent.height;
        if( swapchain_extent.width > surface_capabilities.maxImageExtent.width )
            swapchain_extent.width = surface_capabilities.maxImageExtent.width;
        if( swapchain_extent.height > surface_capabilities.maxImageExtent.height )
            swapchain_extent.height = surface_capabilities.maxImageExtent.height;
    }

    VkImageUsageFlags swapchain_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if( !surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
        VERR("Swapchain is dumb and doesn't support transfer operations");

    VkSurfaceTransformFlagBitsKHR swapchain_transform = 0;
    if(surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        swapchain_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VDBG("Current, chose, supported transforms %x, %x, %x", surface_capabilities.currentTransform, swapchain_transform, surface_capabilities.supportedTransforms);

    uint32_t present_mode_count = 0;
    r = vkGetPhysicalDeviceSurfacePresentModesKHR( v->physical_device, v->present_surface, &present_mode_count, NULL);
    VV(r) VERR("Getting present modes");
    VkPresentModeKHR present_modes[present_mode_count];
    r = vkGetPhysicalDeviceSurfacePresentModesKHR( v->physical_device, v->present_surface, &present_mode_count, present_modes);
    VV(r) VERR("Getting present modes");
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VDBG("%d available present modes", present_mode_count);
    for(int i = 0; i < present_mode_count; i++){
        VDBG("Found present mode: %d", present_modes[i]);
        if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
            swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    VDBG("Chose present mode: %s", swapchain_present_mode == VK_PRESENT_MODE_FIFO_KHR ? "FIFO" : "Mailbox");

    VkSwapchainKHR old_swapchain = v->swapchain;
    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = v->present_surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swapchain_extent,
        .imageArrayLayers = 1, // Defines the number of layers in a swap chain images (that is, views); typically this value will be one but if we want to create multiview or stereo (stereoscopic 3D) images, we can set it to some higher value.
        .imageUsage = swapchain_usage,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = swapchain_transform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = swapchain_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain
    };

    VV( vkCreateSwapchainKHR( v->device, &swapchain_create_info, NULL, &v->swapchain) ) 
        VERR("Could not create swapchain");
    
    if( old_swapchain != VK_NULL_HANDLE ) {
        vkDestroySwapchainKHR( v->device, old_swapchain, NULL );
    }
    vkGetSwapchainImagesKHR( v->device, v->swapchain, &image_count, NULL );
    v->swapchain_size = image_count;
    v->swapchain_format = surface_format.format;
    VDBG("Swapchain created with %d images", image_count);
    if(image_count > MAX_IMAGE_COUNT) VERR("Too many swapchain images");
}

int create_render_pass(VkState* v){
    VkAttachmentDescription attachment_descriptions[] = {
        {
            .flags = 0,
            .format = v->swapchain_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        }
    };
    VkAttachmentReference color_attachments[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    };
    VkSubpassDescription subpass_descriptions[] = {
        {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachments = color_attachments,
            .pResolveAttachments = NULL,
            .pDepthStencilAttachment = NULL,
            .preserveAttachmentCount = 0,
            .pResolveAttachments = NULL
        }
    };
    VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = attachment_descriptions,
        .subpassCount = 1,
        .pSubpasses = subpass_descriptions,
        .dependencyCount = 0,
        .pDependencies = NULL
    };
    vkCreateRenderPass( v->device, &render_pass_create_info, NULL, &v->render_pass );
}
int create_framebuffers(VkState* v){
    uint32_t image_count = v->swapchain_size;
    VkImage images[image_count];
    vkGetSwapchainImagesKHR(v->device, v->swapchain, &image_count, images);

    for(size_t i; i < image_count; i++){
        VkResult r;
        VkImageViewCreateInfo image_view_create_info = {
            VSTRUCT(IMAGE_VIEW)
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = v->swapchain_format,
            .components = {VSWI, VSWI, VSWI, VSWI},
            .subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1, 0, 1
            }
        };
        r = vkCreateImageView( v->device, &image_view_create_info, NULL, &v->image_views[i]);
        VV(r) VERR("Failed creating image view");

        VkFramebufferCreateInfo framebuffer_create_info = {
            VSTRUCT(FRAMEBUFFER)
            .renderPass = v->render_pass,
            .attachmentCount = 1,
            .pAttachments = &v->image_views[i],
            .width = v->width,
            .height = v->height,
            .layers = 1
        };
        r = vkCreateFramebuffer( v->device, &framebuffer_create_info, NULL, &v->framebuffers[i] );
        VV(r) VERR("Failed creating framebuffer");
    }
}
VkShaderModule create_shader_module(VkState* v, const char* filename){
    FILE* file = fopen(filename, "r");
    struct stat stats;
    fstat(fileno(file), &stats);
    size_t codesize = stats.st_size;
    char contents[codesize];
    fread(contents, 1, codesize, file);
    VkShaderModuleCreateInfo shader_module_create_info = {
        VSTRUCT(SHADER_MODULE)
        .codeSize = codesize,
        .pCode = (uint32_t*)contents
    };
    VkResult r;
    VkShaderModule shader_module;
    r = vkCreateShaderModule(v->device, &shader_module_create_info, NULL, &shader_module);
    VV(r) VERR("Failed creating shader module %s", filename);
    return shader_module;
}
int create_pipeline(VkState* v){
    VkResult r;
    VkShaderModule vert = create_shader_module(v, "vert.spv");
    VkShaderModule frag = create_shader_module(v, "frag.spv");

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {
        {
            VSTRUCT(PIPELINE_SHADER_STAGE)
            VK_SHADER_STAGE_VERTEX_BIT,
            vert,
            "main",
            NULL
        },
        {
            VSTRUCT(PIPELINE_SHADER_STAGE)
            VK_SHADER_STAGE_FRAGMENT_BIT,
            frag,
            "main",
            NULL
        }
    };
    uint32_t shader_stage_count = arraysize(shader_stage_create_infos);

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
        VSTRUCT(PIPELINE_VERTEX_INPUT_STATE)
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
        VSTRUCT(PIPELINE_INPUT_ASSEMBLY_STATE)
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = v->width,
        .height = v->height,
        .minDepth = 0,
        .maxDepth = 1
    };
    VkRect2D scissor = {
        .offset = {
            0, 0
        },
        .extent = {
            v->width, v->height
        }
    };
    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        VSTRUCT(PIPELINE_VIEWPORT_STATE)
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {
        VSTRUCT(PIPELINE_RASTERIZATION_STATE)
        .depthClampEnable = VK_FALSE, // controls whether to clamp the fragment’s depth values as described in Depth Test. Enabling depth clamp will also disable clipping primitives to the z planes of the frustrum as described in Primitive Clipping.
        .rasterizerDiscardEnable = VK_FALSE, // controls whether primitives are discarded immediately before the rasterization stage
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0,
        .depthBiasClamp = 0.0,
        .depthBiasSlopeFactor = 0.0,
        .lineWidth = 1.0
    };
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {
        VSTRUCT(PIPELINE_MULTISAMPLE_STATE)
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, // multisampling selection
        .sampleShadingEnable = VK_FALSE, // *can* be used to enable sample shading (forcing multisampling, even when not on geometry edge)
        .minSampleShading = 1, // only used if sampleShadingEnable == VK_TRUE
        .pSampleMask = NULL, // only used if sampleShadingEnable == VK_TRUE
        .alphaToCoverageEnable = VK_FALSE, // only used if sampleShadingEnable == VK_TRUE
        .alphaToOneEnable = VK_FALSE // only used if sampleShadingEnable == VK_TRUE
    };
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_MAX,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
        VSTRUCT(PIPELINE_COLOR_BLEND_STATE)
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // enum VkLogicOp
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {0, 0, 0, 0}
    };

    VkPipelineLayoutCreateInfo layout_create_info = {
        VSTRUCT(PIPELINE_LAYOUT)
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };
    VkPipelineLayout pipeline_layout;
    r = vkCreatePipelineLayout( v->device, &layout_create_info, NULL, &pipeline_layout);
    VV(r) VERR("Could not create pipeline layout");

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        VSTRUCTF(GRAPHICS_PIPELINE, 0)
        .stageCount = shader_stage_count,
        .pStages = shader_stage_create_infos, // pointer to an array of stageCount VkPipelineShaderStageCreateInfo structures describing the set of the shader stages to be included in the graphics pipeline
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info, // pointer to a VkPipelineInputAssemblyStateCreateInfo structure which determines input assembly behavior, as described in **Drawing Commands**
        .pTessellationState = NULL, // pointer to a VkPipelineTessellationStateCreateInfo structure, and is ignored if the pipeline does not include a tessellation control shader stage and tessellation evaluation shader stage
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &pipeline_rasterization_state_create_info,
        .pMultisampleState = &pipeline_multisample_state_create_info,
        .pDepthStencilState = NULL, // ignored if the pipeline has rasterization disabled or if the subpass of the render pass the pipeline is created against does not use a depth/stencil attachment
        .pColorBlendState = &color_blend_state_create_info,
        .pDynamicState = NULL, // used to indicate which properties of the pipeline state object are dynamic and can be changed independently of the pipeline state. This can be NULL, which means no state in the pipeline is considered dynamic
        .layout = pipeline_layout,
        .renderPass = v->render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    r = vkCreateGraphicsPipelines( v->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &v->graphics_pipeline);
    VV(r) VERR("Could not create graphics pipeline");

    vkDestroyPipelineLayout(v->device, pipeline_layout, NULL);
}
int create_command_pool(VkState* v){
    VkResult r;
    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        0,
        v->graphics_queue_family_index
    };

    r = vkCreateCommandPool( v->device, &command_pool_create_info, NULL, &v->command_pool);
    VV(r) VERR("Command pool creation failed: %d", r);
    VDBG("Command pool created");
}
int create_command_buffers(VkState* v){
    VkResult r;
    VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        v->command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        v->swapchain_size
    };
    if(v->swapchain_size > MAX_IMAGE_COUNT) VERR("Too many swapchain images: %d/%d", v->swapchain_size, MAX_IMAGE_COUNT);
    // r = vkAllocateCommandBuffers( v->device, &cmd_buffer_allocate_info, v->present_command_buffers);
    // VV(r) VERR("Present command buffer allocation failed: %d", r);
    // if( v->present_queue != v-> graphics_queue) VERR("Present queue != graphics queue and I haven't bothered to handle that");
    r = vkAllocateCommandBuffers( v->device, &cmd_buffer_allocate_info, v->graphics_command_buffers);
    VV(r) VERR("Graphics command buffer allocation failed: %d", r);
    VDBG("Command buffers allocated");
    r = record_command_buffers(v);
    VV(r) VERR("Command buffer recording failed: %d", r);
    VDBG("Command buffers complete");
}
int record_command_buffers(VkState* v){
    VkResult r;
    uint32_t image_count = v->swapchain_size;
    VkImage images[image_count];
    vkGetSwapchainImagesKHR( v->device, v->swapchain, &image_count, images);

    VkCommandBufferBeginInfo cmd_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL
    };
    VkClearValue clear_value = {
        {1.0f, 0.3f, 0.5f, 0.0f }
    };
    VkImageSubresourceRange image_subresource_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    for(uint32_t i = 0; i < image_count; i++){
        VkCommandBuffer buffer = v->graphics_command_buffers[i];
        vkBeginCommandBuffer( buffer, &cmd_buffer_begin_info );
        if( v->graphics_queue != v->present_queue ){
            VkImageMemoryBarrier barrier_present_draw = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = NULL,
                .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = v->present_queue_family_index,
                .dstQueueFamilyIndex = v->graphics_queue_family_index,
                .image = images[i],
                .subresourceRange = image_subresource_range
            };
            vkCmdPipelineBarrier( buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_present_draw );
        }

        VkRenderPassBeginInfo render_pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = NULL,
            .renderPass = v->render_pass,
            .framebuffer = v->framebuffers[i],
            .renderArea = {{0, 0}, {v->width, v->height}},
            .clearValueCount = 1,
            .pClearValues = &clear_value
        };
        vkCmdBeginRenderPass( buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );
        vkCmdBindPipeline( buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, v->graphics_pipeline);
        vkCmdDraw( buffer, 3, 1, 0, 0);
        vkCmdEndRenderPass( buffer );

        if( v->graphics_queue != v->present_queue ){
            VkImageMemoryBarrier barrier_draw_present = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = NULL,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = v->graphics_queue_family_index,
                .dstQueueFamilyIndex = v->present_queue_family_index,
                .image = images[i],
                .subresourceRange = image_subresource_range
            };
            vkCmdPipelineBarrier( buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_draw_present );
        }
        r = vkEndCommandBuffer( buffer );
        VV(r) VERR("Could not record command buffer %d", i);
    }
    VDBG("Command buffers recorded");
    return VK_SUCCESS;
}

int draw(VkState* v){
    uint32_t image_index = 0;
    uint64_t timeout_ns = 1e10; // 10 seconds
    VkResult r;
    r = vkAcquireNextImageKHR( v->device, v->swapchain, timeout_ns, 
        v->image_available_semaphore, VK_NULL_HANDLE, &image_index);
    switch(r) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return recreate_swapchain(v);
        default:
            VERR("Unknown VKResult acquiring next image: %d", r);
            return !VK_SUCCESS;
    }

    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        1,
        &v->image_available_semaphore,
        &wait_dst_stage_mask,
        1,
        &v->graphics_command_buffers[image_index],
        1,
        &v->rendering_finished_semaphore
    };
    r = vkQueueSubmit( v->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    VV(r) VERR("Could not submit queue");

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &v->rendering_finished_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &v->swapchain,
        .pImageIndices = &image_index,
        .pResults = NULL
    };
    r = vkQueuePresentKHR( v->present_queue, &present_info );
    switch(r) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            return recreate_swapchain(v);
        default:
            VERR("Unknown VKResult acquiring next image: %d", r);
            return !VK_SUCCESS;
    }

    return VK_SUCCESS;
}

int recreate_swapchain(VkState* v){
}

int clear_swapchain(VkState* v){
    vkDeviceWaitIdle( v->device );
    vkFreeCommandBuffers( v->device, v->command_pool, v->swapchain_size, v->graphics_command_buffers);
    vkDestroyCommandPool( v->device, v->command_pool, NULL);
}

int clean_vulkan_resources(VkState* v){
    vkDeviceWaitIdle(v->device);

    clear_swapchain(v);
    vkDestroySemaphore(v->device, v->image_available_semaphore, NULL);
    vkDestroySemaphore(v->device, v->rendering_finished_semaphore, NULL);
    vkDestroySwapchainKHR(v->device, v->swapchain, NULL);
    vkDestroySurfaceKHR(v->instance, v->present_surface, NULL);

    vkDestroyDevice(v->device, NULL);

    vkDestroyInstance(v->instance, NULL);
    VDBG("Cleaned");
}

extern inline void vdbg(const char*, ...);
extern inline void verr(const char*, ...);
