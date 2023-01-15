#version 330
layout (location = 0) in vec3 input_position;
layout (location = 1) in vec3 input_color;
layout (location = 2) in vec3 input_normal;
layout (location = 3) in vec2 input_uv;


//v = vertex output
out vec2 v_uv;
out vec3 v_normal;


uniform mat4 u_pv_matrix;
uniform mat4 u_model_matrix;

void main(){
    gl_Position = u_pv_matrix * u_model_matrix * vec4( input_position, 1.0 );
    v_uv = input_uv;
    v_normal = input_normal;
}

//program ini akan jalan sebanyak titik yang ada pada object, jika titik ada sejuta maka program ini akan jalan sejuta kali
