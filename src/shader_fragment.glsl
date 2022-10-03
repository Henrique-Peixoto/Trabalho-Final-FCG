#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;
in vec3 rasterized_color;
in vec3 vertex_color;
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define PLATFORM 0
#define PLAYER 1
#define KEY 2

#define SPHERE 3// -> 0
#define BUNNY  4// -> 1
#define PLANE  5// -> 2
#define WALL 6// -> 3
#define CRATE 7// -> 4
#define BARREL 8// -> 5
uniform int object_id; 

uniform vec4 bbox_min;
uniform vec4 bbox_max;
uniform vec4 camera_view;


// Variáveis para acesso das imagens de textura
//uniform sampler2D TextureImage0;
uniform sampler2D SphereTexture;

//uniform sampler2D TextureImage1;
uniform sampler2D BunnyTexture;

//uniform sampler2D TextureImage2;
uniform sampler2D PlaneTexture;

//uniform sampler2D TextureImage3;
uniform sampler2D WallTexture;

//uniform sampler2D TextureImage4;
uniform sampler2D CrateTexture;

//uniform sampler2D TextureImage5;
uniform sampler2D BarrelTexture;

uniform sampler2D PlatformTexture;
uniform sampler2D PlayerTexture;
uniform sampler2D KeyTexture;

out vec4 color;

// Constantes
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main() {
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    //vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));
    

     //vec4 spotlight_position = camera_position;
    //vec4 spotlight_direction = camera_view;
    vec4 spotlight_position = vec4(0.0f,2.0f,1.0f,1.0f);
    vec4 spotlight_direction = vec4(0.0f,-1.0f,0.0f,0.0f);
    float spotlight_angle = 15.0f;

    vec4 l = normalize(camera_position-p);

    // Vetor que define o sentido da câmera e m relação ao ponto atual.
    vec4 v = normalize(camera_position - p);
    vec4 r = -l+2*n*(dot(n, l));
    vec4 h = (v+l)/normalize(v+l);

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    int qLine; // Expoente especular para o modelo de iluminação de Phong

    float U;
    float V;

    if (object_id == PLATFORM) {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        //float miny = bbox_min.y;
        //float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x-minx)/(maxx-minx);
        V = (position_model.z-minz)/(maxz-minz);
    }

    if ( object_id == SPHERE )
    {
        // PREENCHA AQUI as coordenadas de textura da esfera, computadas com
        // projeção esférica EM COORDENADAS DO MODELO. Utilize como referência
        // o slides 134-150 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // A esfera que define a projeção deve estar centrada na posição
        // "bbox_center" definida abaixo.

	

        // Você deve utilizar:
        //   função 'length( )' : comprimento Euclidiano de um vetor
        //   função 'atan( , )' : arcotangente. Veja https://en.wikipedia.org/wiki/Atan2.
        //   função 'asin( )'   : seno inverso.
        //   constante M_PI
        //   variável position_model

	
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

	vec4 ppoint = bbox_center + normalize(position_model - bbox_center);
	vec4 pVec = ppoint - bbox_center;

	float theta = atan(pVec.x,pVec.z);
	float phi = asin(pVec.y);

	

        U = (theta+M_PI)/(2*M_PI);
        V = (phi+M_PI_2)/M_PI;
	//U = 0.0;
	//V = 0.0;
    }
    else if ( object_id == BUNNY )
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slides 99-104 do documento Aula_20_Mapeamento_de_Texturas.pdf,
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slides 158-160 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // Veja também a Questão 4 do Questionário 4 no Moodle.

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;


        U = (position_model.x - minx)/(maxx - minx);
        V = (position_model.y - miny)/(maxy - miny);
    }
    else if ( object_id == PLANE)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;
    }else if(object_id == WALL){
        U = texcoords.x;
        V = texcoords.y;
    }else if(object_id == CRATE){
        U = texcoords.x;
        V = texcoords.y;
    }else if(object_id == BARREL){
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        vec4 ppoint = bbox_center + normalize(position_model - bbox_center);
        vec4 pVec = ppoint - bbox_center;

        float theta = atan(pVec.x,pVec.z);
        float phi = asin(pVec.y);

	

        U = (theta+M_PI)/(2*M_PI);
        V = (phi+M_PI_2)/M_PI;
        
    }

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    //vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    //vec3 Kd1 = texture(TextureImage1, vec2(U,V)).rgb;
    //vec3 Kd2 = texture(TextureImage2, vec2(U,V)).rgb;
    //vec3 Kd3 = texture(TextureImage3, vec2(U,V)).rgb;
    //vec3 Kd4 = texture(TextureImage4, vec2(U,V)).rgb;
    vec3 Kd0 = texture(SphereTexture, vec2(U,V)).rgb;
    vec3 Kd1 = texture(BunnyTexture, vec2(U,V)).rgb;
    vec3 Kd2 = texture(PlaneTexture, vec2(U,V)).rgb;
    vec3 Kd3 = texture(WallTexture, vec2(U,V)).rgb;
    vec3 Kd4 = texture(CrateTexture, vec2(U,V)).rgb;
    vec3 Kd5 = texture(BarrelTexture, vec2(U,V)).rgb;

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0);
    // Espectro da luz ambiente
    vec3 Ia = vec3(0.1,0.1,0.1); 

    vec3 ambient_term = Ia;

    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

    float lambert2 = max(0,dot(-n,l));

    //f(dot(normalize(p-spotlight_position),normalize(spotlight_direction)) < cos(spotlight_angle*3.14/180)){
    //   I = vec3(0.2,0.2,0.2);
    //}

        
    vec3 lambert_diffuse_term = Kd0 * I * (lambert + 0.01) ;
    if(object_id == WALL){
        lambert_diffuse_term = Kd2 * I * (lambert + 0.01);

    }else if(object_id == CRATE){
         lambert_diffuse_term = Kd3 * I * (lambert + 0.01);
    }else if(object_id == BARREL){
        lambert_diffuse_term = Kd4 * I * (lambert + 0.01);
    }

    color.rgb = lambert_diffuse_term + ambient_term;

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    //vec3 Kd0;
    if (object_id == PLATFORM) {
        Kd = vec3(0.3,0.01,0.0);
        Ks = vec3(0.01,0.0,0.0);
        Ka = vec3(0.2,0.1,0.0);
        Kd0 = texture(PlatformTexture, vec2(U,V)).rgb;
    } else if (object_id == PLAYER) {
        Kd = vec3(0.3,0.3,0.3);
        Ks = vec3(0.01,0.0,0.0);
        Ka = vec3(0.2,0.1,0.0);
        Kd0 = texture(PlayerTexture, vec2(U,V)).rgb;
    } else if (object_id == KEY) {
        Kd = vec3(1.0,1.0,0.0);
        Ks = vec3(0.01,0.01,0.0);
        Ka = vec3(1.0,1.0,0.0);
        Kd0 = texture(KeyTexture, vec2(U,V)).rgb;
        qLine = 2;
    }

    // Termo difuso utilizando a lei dos cossenos de Lambert
    //vec3 lambert_diffuse_term = Kd*I*max(0, dot(n, l));

    // Termo ambiente
    //vec3 ambient_term = Ka*Ia;

    // Termo especular de Blinn-Phong
    vec3 specular_term = Ks*I*pow(dot(n, h), qLine);

    if (object_id == KEY) {
        // Aplicando o modelo de iluminação de Blinn-Phong no objeto da chave
        color.rgb = Kd0 + ambient_term + lambert_diffuse_term + specular_term + vertex_color;
    } else if (object_id != PLAYER) {
        // Aplicando o modelo de iluminação de Phong nos objetos da cena
        color.rgb = Kd0 + ambient_term + lambert_diffuse_term + vertex_color;
    } else {
        color.rgb = vertex_color + Kd0;
    }

    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 

