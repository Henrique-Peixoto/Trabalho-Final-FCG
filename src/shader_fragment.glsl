#version 330 core

in vec4 position_world;
in vec4 normal;
in vec4 position_model;
in vec3 rasterized_color;
in vec2 texcoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int object_id;
uniform vec4 bbox_min;
uniform vec4 bbox_max;

uniform sampler2D PlatformTexture;

out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main() {
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    vec4 p = position_world;
    vec4 n = normalize(normal);
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));
    vec4 v = normalize(camera_position - p);

    float minx = bbox_min.x;
    float maxx = bbox_max.x;

    float miny = bbox_min.y;
    float maxy = bbox_max.y;

    float minz = bbox_min.z;
    float maxz = bbox_max.z;

    float U = (position_model.x-minx)/(maxx-minx);
    float V = (position_model.y-miny)/(maxy-miny);

    color.a = 1;

    float lambert = max(0,dot(n,l));
    vec3 Kd0 = texture(PlatformTexture, vec2(U,V)).rgb;

    color.rgb = Kd0 * (lambert + 0.01);
    //color.rgb = rasterized_color;
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}
