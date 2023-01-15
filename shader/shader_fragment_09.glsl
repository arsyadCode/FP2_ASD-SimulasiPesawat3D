#version 330
in vec4 v_color;
in vec3 v_normal;
in vec2 v_uv;

uniform vec3 u_directional_light_color = vec3(0.9,0.9,0.9);
uniform vec3 u_light_direction = vec3(1,-1,-1);
uniform sampler2D u_texture; 
// texture samplers

out vec4 o_color;




float DiffuseIntensity(vec3 L, vec3 N) {
    float I = max(0.0,dot(-L,N));
    return I;
}

void main() {
    o_color = texture(u_texture, v_uv);
};
