#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform texture2D u_atlas;
layout(set = 0, binding = 1) uniform sampler u_sampler;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 color;
    vec4 params;  // x: distance range in atlas pixels, y: atlas width, z: atlas height
} pc;

layout(location = 0) out vec4 out_color;

// The same math as vnegfx's MSDF text shader. A single-channel distance field stored in all channels gives the same
// median, so a baked SDF atlas and a real MSDF atlas render through this one shader.
float medianOf(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main() {
    float field = medianOf(texture(sampler2D(u_atlas, u_sampler), v_uv).rgb);
    // How many screen pixels the field's range covers: sharp at any distance and size.
    vec2 unit_range = vec2(pc.params.x) / pc.params.yz;
    vec2 screen_tex_size = vec2(1.0) / fwidth(v_uv);
    float px_range = max(0.5 * dot(unit_range, screen_tex_size), 1.0);
    float alpha = clamp((field - 0.5) * px_range + 0.5, 0.0, 1.0) * pc.color.a;
    if (alpha < 0.004) {
        discard;
    }
    // Premultiplied alpha, to match the blend state.
    out_color = vec4(pc.color.rgb * alpha, alpha);
}
