#version 330
in vec4 v_color;
in vec3 v_normal;

uniform vec3 u_directional_light_color = vec3(0.9,0.9,0.9);
uniform vec3 u_light_direction = vec3(1,-1,-1);


out vec4 o_color;



float DiffuseIntensity(vec3 L, vec3 N) {
    float I = max(0.0,dot(-L,N));
    return I;
}

void main() {
    o_color = vec4( DiffuseIntensity(u_light_direction,v_normal)* v_color.rgb,1.0);
};
