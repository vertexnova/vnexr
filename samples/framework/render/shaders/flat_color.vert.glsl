#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 color;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
    gl_Position = pc.mvp * vec4(in_position, 1.0);
    // Fixed light in model space: enough to tell the faces of a box apart without a normal matrix.
    const vec3 light_dir = normalize(vec3(0.35, 0.85, 0.45));
    float shade = 0.55 + 0.45 * max(dot(normalize(in_normal), light_dir), 0.0);
    out_color = vec4(pc.color.rgb * shade, pc.color.a);
}
