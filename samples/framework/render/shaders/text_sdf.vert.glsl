#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 color;
    vec4 params;  // x: distance range in atlas pixels, y: atlas width, z: atlas height
} pc;

layout(location = 0) out vec2 v_uv;

void main() {
    gl_Position = pc.mvp * vec4(in_position, 1.0);
    v_uv = in_uv;
}
