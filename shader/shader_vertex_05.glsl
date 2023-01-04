#version 330 
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec4 i_color;


out vec4 v_color;


uniform mat4 u_pv_matrix;
uniform mat4 u_model_matrix;

void main(){
    gl_Position = u_pv_matrix * u_model_matrix * vec4( i_position, 1.0 );
    v_color = i_color;
}

//program ini akan jalan sebanyak titik yang ada pada object, jika titik ada sejuta maka program ini akan jalan sejuta kali