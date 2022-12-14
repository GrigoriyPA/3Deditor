#version 430 core


layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texture_coord;

out vec2 tex_coord;


void main() {
    gl_Position =  vec4(position.x, position.y, 0.0, 1.0);
    tex_coord = vec2(texture_coord.x, texture_coord.y);
}
