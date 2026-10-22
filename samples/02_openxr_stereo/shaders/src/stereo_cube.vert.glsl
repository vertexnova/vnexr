#version 450

layout(location = 0) in vec3 in_position;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 color;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
    gl_Position = pc.mvp * vec4(in_position, 1.0);
    out_color = pc.color;
}
