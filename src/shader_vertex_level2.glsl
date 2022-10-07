#version 330 core

layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;
layout (location = 3) in vec3 color_coefficients;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
#define PLATFORM 0
#define PLAYER 1
#define KEY 2
uniform int object_id;

out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec3 rasterized_color;
out vec3 vertex_color;
out vec2 texcoords;

void main() {
    // Para uso no Gouraud shading
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    gl_Position = projection * view * model * model_coefficients;

    position_world = model * model_coefficients;
    position_model = model_coefficients;
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    vec4 n = normalize(normal);
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));
    vec4 v = normalize(camera_position - position_world);
    vec4 r = -l+2*n*(dot(n, l));
    vec4 h = (v+l)/normalize(v+l);

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    int qLine; // Expoente especular para o modelo de iluminação de Phong

    if (object_id == PLATFORM) {
        Kd = vec3(0.3,0.01,0.0);
        Ks = vec3(0.01,0.0,0.0);
        Ka = vec3(0.2,0.1,0.0);
    } else if (object_id == PLAYER) {
        Kd = vec3(0.3,0.3,0.3);
        Ks = vec3(0.01,0.0,0.0);
        Ka = vec3(0.2,0.1,0.0);
    } else if (object_id == KEY) {
        Kd = vec3(1.0,1.0,0.0);
        Ks = vec3(0.01,0.01,0.0);
        Ka = vec3(1.0,1.0,0.0);
        qLine = 2;
    }

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0);

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.2,0.2,0.2);

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd*I*max(0, dot(n, l));

    // Termo ambiente
    vec3 ambient_term = Ka*Ia;

    // Termo especular de Blinn-Phong
    vec3 specular_term = Ks*I*pow(max(dot(n, h), 0), qLine);
    
    vertex_color = color_coefficients + ambient_term + lambert_diffuse_term + specular_term;

    texcoords = texture_coefficients;

    rasterized_color = color_coefficients;
}

