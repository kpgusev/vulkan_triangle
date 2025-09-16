#version 450

layout(location = 0) in vec3 in_color;

layout(location = 0) out vec4 out_color;

void main() {
  out_color = vec4(in_color.x, in_color.y, in_color.z, 1.0f);
}