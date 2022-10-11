#version 330 core

in vec4 position_world;
in vec4 normal;
in vec4 position_model;
in vec3 rasterized_color;
in vec2 texcoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define PLATFORM 0
#define PLAYER 1
#define KEY 2
#define CANNON 3
uniform int object_id;

uniform vec4 bbox_min;
uniform vec4 bbox_max;

uniform sampler2D PlatformTexture;
uniform sampler2D KeyTexture;
uniform sampler2D CannonTexture;
uniform sampler2D PlayerTexture;

out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

// TODO: aplicar iluminação diferenciada na chave
void main() {
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    vec4 p = position_world;
    vec4 n = normalize(normal);
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));
    vec4 v = normalize(camera_position - p);
    vec4 r = -l+2*n*(dot(n, l));
    vec4 h = normalize(v+l); // ou: (v+l)/length(v+l)

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    int qLine; // Expoente especular para o modelo de iluminação de Phong

    float U;
    float V;

    if (object_id == PLATFORM || object_id == CANNON) {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        //float miny = bbox_min.y;
        //float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x-minx)/(maxx-minx);
        V = (position_model.z-minz)/(maxz-minz);
    }

    color.a = 1;

    vec3 Kd0;
    if (object_id == PLATFORM) {
        Kd = vec3(0.3,0.01,0.0);
        Ks = vec3(0.01,0.0,0.0);
        Ka = vec3(0.2,0.1,0.0);
        Kd0 = texture(PlatformTexture, vec2(U,V)).rgb;
    }else if (object_id == PLAYER) {
        Kd = vec3(0.3,0.3,0.3);
        Ks = vec3(0.01,0.0,0.0);
        Ka = vec3(0.2,0.1,0.0);
        Kd0 = texture(PlayerTexture, vec2(U,V)).rgb;
    } else if (object_id == KEY){
        Kd = vec3(1.0,1.0,0.0);
        Ks = vec3(0.01,0.01,0.0);
        Ka = vec3(1.0,1.0,0.0);
        Kd0 = texture(KeyTexture, vec2(U,V)).rgb;
        qLine = 2;
    }else if(object_id == CANNON){
       Kd = vec3(0.02,0.02,0.02);
        Ks = vec3(0.02,0.02,0.02);
        Ka = vec3(0.02,0.02,0.02);
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
    vec3 specular_term = Ks*I*pow(max(0,dot(n, h)), qLine);

    if (object_id == KEY) {
        color.rgb = Kd0 + ambient_term + lambert_diffuse_term + specular_term;
    } else if (object_id == CANNON){
        color.rgb = ambient_term + lambert_diffuse_term;
    }else{
        color.rgb = Kd0 + ambient_term + lambert_diffuse_term;
    }

    //color.rgb = rasterized_color;
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}
