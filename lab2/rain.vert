#version 330 core
layout(location = 0) in vec3 position;

uniform mat4 VP;

void main() {
    gl_Position = VP * vec4(position, 1.0);
    //gl_PointSize = 2.0; // Adjust rain drop size
}