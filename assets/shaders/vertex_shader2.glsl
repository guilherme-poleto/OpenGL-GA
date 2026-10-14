#version 400
layout(location = 0) in vec3 vertex_posicao;

uniform mat4 model;

void main () {

    //aplicamos no vértice a matriz de transformação do modelo ("model")
    gl_Position = model * vec4(vertex_posicao, 1.0);
}