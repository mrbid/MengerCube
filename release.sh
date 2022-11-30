clang main.c glad_gl.c -I inc -Ofast -lglfw -lm -o menger
clang main_trails.c glad_gl.c -I inc -Ofast -lglfw -lm -o menger_trails
upx menger
upx menger_trails