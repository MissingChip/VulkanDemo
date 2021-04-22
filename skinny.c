
#include "skinny.h"

int main(int argc, char** argv){
    VkState v;
    XState x;
    int c = 0;
    check_instance_extensions();
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
    VK_KHR_SURFACE_EXTENSION_NAME
};
const uint32_t required_instance_extension_count = arraysize(required_instance_extensions);

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
    xcb_create_window( x->connection, XCB_COPY_FROM_PARENT, x->window, screen->root, 0, 0, v->width, v->height, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL );
    xcb_map_window(x->connection, x->window);
    xcb_flush (x->connection);
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
        VSTRUCTC(INSTANCE)
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = required_instance_extension_count,
        .ppEnabledExtensionNames = required_instance_extensions
    };
    vkCreateInstance( &instance_create_info, NULL, &v->instance );
    v->swapchain = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

int create_presentation_surface(VkState* v, XState* x){
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
        VSTRUCTKHR(XCB_SURFACE)
        x->connection,
        x->window
    };
    vkCreateXcbSurfaceKHR(v->instance, &surface_create_info, NULL, &v->present_surface);
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
    v->physical_device = physical_devices[0];
    return VK_SUCCESS;
}

int choose_queue_family(VkState* v){
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(v->physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_family_properties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(v->physical_device, &queue_family_count, queue_family_properties);

    uint32_t queue_family_index = BAD;
    for( uint32_t i = 0; i < queue_family_count; ++i ) {
        VkBool32 surface_support;
        vkGetPhysicalDeviceSurfaceSupportKHR( v->physical_device, i, v->present_surface, &surface_support );
        if( (queue_family_properties[i].queueCount > 0) &&
            (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ) {
            // Select first queue that supports graphics
            if( queue_family_index == BAD && surface_support ) {
                v->queue_family_index = i;
            }
        }
    }
    return VK_SUCCESS;
}

int create_logical_device(VkState* v){
    float queue_priorities[] = {1.0};
    uint32_t number_of_queues = 1;
    VkDeviceQueueCreateInfo device_queue_create_infos[] = {
        {
            VSTRUCTC(DEVICE_QUEUE)
            .queueFamilyIndex = v->queue_family_index,
            .queueCount = arraysize(queue_priorities),
            .pQueuePriorities = queue_priorities
        }
    };

    VkDeviceCreateInfo device_create_info = {
        VSTRUCTC(DEVICE)
        number_of_queues,
        device_queue_create_infos,
        0,
        NULL,
        required_device_extension_count,
        required_device_extensions,
        NULL
    };
    vkCreateDevice( v->physical_device, &device_create_info, NULL, &v->device );
    vkGetDeviceQueue( v->device, v->queue_family_index, 0, &v->queue);
    return VK_SUCCESS;
}

int create_semaphores(VkState* v){
    VkSemaphoreCreateInfo semaphore_create_info = { VSTRUCTC(SEMAPHORE) };
    vkCreateSemaphore( v->device, &semaphore_create_info, NULL, &v->image_available_semaphore);
    vkCreateSemaphore( v->device, &semaphore_create_info, NULL, &v->rendering_finished_semaphore);
}

int create_swapchain(VkState* v){
    VkResult r;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( v->physical_device, v->present_surface, &surface_capabilities);
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( v->physical_device, v->present_surface, &format_count, NULL);
    VkSurfaceFormatKHR surface_formats[format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR( v->physical_device, v->present_surface, &format_count, surface_formats);

    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if( image_count < 3 ) image_count = 3;
    if( (surface_capabilities.maxImageCount > 0) &&
            (image_count > surface_capabilities.maxImageCount) ) {
        image_count = surface_capabilities.maxImageCount;
    }

    VkSurfaceFormatKHR surface_format = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    VkExtent2D swapchain_extent = { v->width, v->height };
    VkImageUsageFlags swapchain_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkSurfaceTransformFlagBitsKHR swapchain_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    uint32_t present_mode_count = 0;
    r = vkGetPhysicalDeviceSurfacePresentModesKHR( v->physical_device, v->present_surface, &present_mode_count, NULL);
    VkPresentModeKHR present_modes[present_mode_count];
    r = vkGetPhysicalDeviceSurfacePresentModesKHR( v->physical_device, v->present_surface, &present_mode_count, present_modes);
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

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

    vkCreateSwapchainKHR( v->device, &swapchain_create_info, NULL, &v->swapchain);
    
    if( old_swapchain != VK_NULL_HANDLE ) {
        vkDestroySwapchainKHR( v->device, old_swapchain, NULL );
    }
    vkGetSwapchainImagesKHR( v->device, v->swapchain, &image_count, NULL );
    v->swapchain_size = image_count;
    v->swapchain_format = surface_format.format;
}

int create_render_pass(VkState* v){
    VkAttachmentDescription attachment_descriptions[] = {
        {
            .flags = 0,
            .format = v->swapchain_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
            VSTRUCTC(IMAGE_VIEW)
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
        VkFramebufferCreateInfo framebuffer_create_info = {
            VSTRUCTC(FRAMEBUFFER)
            .renderPass = v->render_pass,
            .attachmentCount = 1,
            .pAttachments = &v->image_views[i],
            .width = v->width,
            .height = v->height,
            .layers = 1
        };
        r = vkCreateFramebuffer( v->device, &framebuffer_create_info, NULL, &v->framebuffers[i] );
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
        VSTRUCTC(SHADER_MODULE)
        .codeSize = codesize,
        .pCode = (uint32_t*)contents
    };
    VkResult r;
    VkShaderModule shader_module;
    r = vkCreateShaderModule(v->device, &shader_module_create_info, NULL, &shader_module);
    return shader_module;
}
int create_pipeline(VkState* v){
    VkResult r;
    VkShaderModule vert = create_shader_module(v, "vert.spv");
    VkShaderModule frag = create_shader_module(v, "frag.spv");
    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {
        {
            VSTRUCTC(PIPELINE_SHADER_STAGE)
            VK_SHADER_STAGE_VERTEX_BIT,
            vert,
            "main",
            NULL
        },
        {
            VSTRUCTC(PIPELINE_SHADER_STAGE)
            VK_SHADER_STAGE_FRAGMENT_BIT,
            frag,
            "main",
            NULL
        }
    };
    uint32_t shader_stage_count = arraysize(shader_stage_create_infos);
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
        VSTRUCTC(PIPELINE_VERTEX_INPUT_STATE)
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
        VSTRUCTC(PIPELINE_INPUT_ASSEMBLY_STATE)
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
        VSTRUCTC(PIPELINE_VIEWPORT_STATE)
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {
        VSTRUCTC(PIPELINE_RASTERIZATION_STATE)
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
        VSTRUCTC(PIPELINE_MULTISAMPLE_STATE)
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
        VSTRUCTC(PIPELINE_COLOR_BLEND_STATE)
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // enum VkLogicOp
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {0, 0, 0, 0}
    };
    VkPipelineLayoutCreateInfo layout_create_info = {
        VSTRUCTC(PIPELINE_LAYOUT)
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };
    VkPipelineLayout pipeline_layout;
    r = vkCreatePipelineLayout( v->device, &layout_create_info, NULL, &pipeline_layout);
    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        VSTRUCTC(GRAPHICS_PIPELINE)
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
    vkDestroyPipelineLayout(v->device, pipeline_layout, NULL);
}
int create_command_pool(VkState* v){
    VkResult r;
    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        0,
        v->queue_family_index
    };
    r = vkCreateCommandPool( v->device, &command_pool_create_info, NULL, &v->command_pool);
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
    r = vkAllocateCommandBuffers( v->device, &cmd_buffer_allocate_info, v->graphics_command_buffers);
    r = record_command_buffers(v);
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
        {0.0f, 0.3f, 0.0f, 0.0f }
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
        r = vkEndCommandBuffer( buffer );
    }
    return VK_SUCCESS;
}

int draw(VkState* v){
    uint32_t image_index = 0;
    uint64_t timeout_ns = 1e10; // 10 seconds
    VkResult r;
    r = vkAcquireNextImageKHR( v->device, v->swapchain, timeout_ns, v->image_available_semaphore, VK_NULL_HANDLE, &image_index);
    switch(r) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return recreate_swapchain(v);
        default:
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
    r = vkQueueSubmit( v->queue, 1, &submit_info, VK_NULL_HANDLE);

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
    r = vkQueuePresentKHR( v->queue, &present_info );
    switch(r) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            return recreate_swapchain(v);
        default:
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
}

int disconnect_x(XState* x){
    xcb_disconnect(x->connection);
}

extern inline void vdbg(const char*, ...);
extern inline void verr(const char*, ...);
