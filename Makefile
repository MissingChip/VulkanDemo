
SHADERS = vert.spv frag.spv

demo%: demo%.c demo%.h Makefile $(SHADERS)
	gcc $< -o $@ -lvulkan -lxcb -g

skinny: skinny.c skinny.h Makefile $(SHADERS)
	gcc $< -o $@ -lvulkan -lxcb -g

frag.spv: frag.glsl
	glslc -fshader-stage=frag $^ -o $@

vert.spv: vert.glsl
	glslc -fshader-stage=vert $^ -o $@