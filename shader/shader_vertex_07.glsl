#version 330
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec4 i_color;
layout (location = 2) in vec3 i_normal;


out vec4 v_color;
out vec3 v_normal;


uniform mat4 u_pv_matrix;
uniform mat4 u_model_matrix;

void main(){
    gl_Position = u_pv_matrix * u_model_matrix * vec4( i_position, 1.0 );
    v_color = i_color;
    v_normal = i_normal;
}

//program ini akan jalan sebanyak titik yang ada pada object, jika titik ada sejuta maka program ini akan jalan sejuta kali
