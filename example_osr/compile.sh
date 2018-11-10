g++ -std=c++14 -I thirdparty -lglfw -lvulkan example.cpp -o example

cd shaders
glslangValidator -V shader.vert
glslangValidator -V shader.frag
