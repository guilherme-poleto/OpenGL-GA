#version 400
layout(location = 0) in vec3 vertex_posicao;

uniform float Tx, Ty;
uniform float Sx, Sy;
uniform float angulo;

void main () {

    float novoX = vertex_posicao.x * cos(angulo) - vertex_posicao.y * sin(angulo);
    float novoY = vertex_posicao.x * sin(angulo) + vertex_posicao.y * cos(angulo);

    float xFinal = (novoX*Sx + Tx);
    float yFinal = (novoY*Sy + Ty);

    gl_Position = vec4(xFinal, yFinal, vertex_posicao.z, 1.0);
}