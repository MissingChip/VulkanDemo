
demo1: demo1.c demo1.h Makefile
	gcc $< -o $@ -lvulkan -lxcb -g
