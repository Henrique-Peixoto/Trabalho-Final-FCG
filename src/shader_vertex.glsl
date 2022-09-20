#version 330 core

// Atributos de vÈrtice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a funÁ„o BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no cÛdigo C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Atributos de vÈrtice que ser„o gerados como saÌda ("out") pelo Vertex Shader.
// ** Estes ser„o interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais ser„o recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;

void main()
{
    // A vari·vel gl_Position define a posiÁ„o final de cada vÈrtice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estar· entre -1 e 1 apÛs divis„o por w.
    // Veja slides 144 e 150 do documento "Aula_09_Projecoes.pdf".
    //
    // O cÛdigo em "main.cpp" define os vÈrtices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operaÁıes de modelagem, definiÁ„o da c‚mera, e projeÁ„o, para computar
    // as coordenadas finais em NDC (vari·vel gl_Position). ApÛs a execuÁ„o
    // deste Vertex Shader, a placa de vÌdeo (GPU) far· a divis„o por W. Veja
    // slide 189 do documento "Aula_09_Projecoes.pdf".

    gl_Position = projection * view * model * model_coefficients;

    // Como as vari·veis acima  (tipo vec4) s„o vetores com 4 coeficientes,
    // tambÈm È possÌvel acessar e modificar cada coeficiente de maneira
    // independente. Esses s„o indexados pelos nomes x, y, z, e w (nessa
    // ordem, isto È, 'x' È o primeiro coeficiente, 'y' È o segundo, ...):
    //
    //     gl_Position.x = model_coefficients.x;
    //     gl_Position.y = model_coefficients.y;
    //     gl_Position.z = model_coefficients.z;
    //     gl_Position.w = model_coefficients.w;
    //

    // Agora definimos outros atributos dos vÈrtices que ser„o interpolados pelo
    // rasterizador para gerar atributos ˙nicos para cada fragmento gerado.

    // PosiÁ„o do vÈrtice atual no sistema de coordenadas global (World).
    position_world = model * model_coefficients;

    // PosiÁ„o do vÈrtice atual no sistema de coordenadas local do modelo.
    position_model = model_coefficients;

    // Normal do vÈrtice atual no sistema de coordenadas global (World).
    // Veja slide 107 do documento "Aula_07_Transformacoes_Geometricas_3D.pdf".
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
    texcoords = texture_coefficients;
}

