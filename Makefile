
SHADERS = vert.spv frag.spv

demo%: demo%.c demo%.h Makefile $(SHADERS)
	gcc $< -o $@ -lvulkan -lxcb -g

%.spv: shader.%
	glslc $^ -o $@