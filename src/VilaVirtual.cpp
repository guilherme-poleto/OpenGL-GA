#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const float PI = 3.14159265358979323846f;

GLFWwindow* Window = nullptr;
GLuint Shader_programm = 0;
int WIDTH = 1280;
int HEIGHT = 720;

float Tempo_entre_frames = 0.0f;

// Câmera em primeira pessoa (mesma lógica do exemplo de câmera visto em aula)
float Cam_speed = 6.0f;
glm::vec3 Cam_pos = glm::vec3(0.0f, 2.0f, 27.0f);
glm::vec3 Cam_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 Cam_up = glm::vec3(0.0f, 1.0f, 0.0f);
float Cam_yaw = 0.0f;
float Cam_pitch = -5.0f;

double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;
bool primeiro_mouse = true;

struct Malha {
    GLuint vao = 0;
    GLsizei quantidade = 0;   // número de índices, ou de vértices se não usar índices
    bool usaIndices = true;
};

Malha Cubo, Plano, Piramide, Prisma, Cilindro, Cone, Esfera;
Malha Pinheiro, Barril;

GLuint TexGrama, TexFolhas, TexTerra, TexMadeira, TexTelhado, TexPedra, TexPalha, TexPinheiro, TexBarril;

// Casas e celeiro: cada uma guarda o estado da sua porta
struct Casa {
    glm::vec3 posicao;
    float rotacao;          // graus, em torno de Y
    float escala;
    glm::vec3 corParede;
    glm::vec3 corTelhado;
    float larguraPorta;
    float alturaPorta;
    bool chamine;
    bool portaAberta;
    float anguloPorta;
};

std::vector<Casa> Casas = {
    //  posição               rot    esc    parede                 telhado                larg  alt   chaminé
    { {-9.0f, 0.0f, -7.0f},   0.0f, 1.00f, {1.00f, 1.00f, 1.00f}, {1.00f, 1.00f, 1.00f}, 1.0f, 1.9f, true,  false, 0.0f },
    { { 9.0f, 0.0f, -7.0f},   0.0f, 1.15f, {1.00f, 0.88f, 0.72f}, {0.80f, 0.85f, 1.00f}, 1.0f, 1.9f, false, false, 0.0f },
    { {-9.0f, 0.0f,  7.0f}, 180.0f, 0.95f, {0.85f, 0.95f, 1.00f}, {1.00f, 0.90f, 0.70f}, 1.0f, 1.9f, true,  false, 0.0f },
    { {-6.0f, 0.0f, 17.0f},  90.0f, 1.00f, {1.00f, 0.95f, 0.85f}, {0.90f, 0.75f, 0.75f}, 1.0f, 1.9f, false, false, 0.0f },
    { {-20.0f, 0.0f, -7.0f},  0.0f, 1.05f, {0.95f, 1.00f, 0.90f}, {1.00f, 1.00f, 1.00f}, 1.0f, 1.9f, true,  false, 0.0f },
    // celeiro: maior, vermelho e com porta larga
    { {12.0f, 0.0f,  9.0f}, 180.0f, 1.60f, {0.95f, 0.40f, 0.35f}, {0.60f, 0.60f, 0.60f}, 1.8f, 2.3f, false, false, 0.0f },
};

struct Estacao {
    const char* nome;
    glm::vec3 corGrama;
    glm::vec3 corFolhagem;
    glm::vec3 corCeu;
    float neve;
};

Estacao Estacoes[3] = {
    { "Verao",   {0.45f, 0.75f, 0.30f}, {0.30f, 0.65f, 0.25f}, {0.50f, 0.75f, 0.95f}, 0.0f  },
    { "Outono",  {0.72f, 0.66f, 0.32f}, {0.95f, 0.50f, 0.15f}, {0.78f, 0.72f, 0.62f}, 0.0f  },
    { "Inverno", {0.70f, 0.75f, 0.70f}, {0.85f, 0.88f, 0.92f}, {0.70f, 0.76f, 0.84f}, 0.85f },
};
int Estacao_atual = 0;

const float DURACAO_DIA = 90.0f;   // segundos para o sol dar uma volta completa
float Ajuste_dia = 0.0f;           // ângulo somado ao sol quando T pula para o dia ou para a noite

// ângulo do sol: 0 = nascendo, PI/2 = meio-dia, PI = se pondo
float calculaAnguloDia(float tempo) {
    return 0.35f + tempo * 2.0f * PI / DURACAO_DIA + Ajuste_dia;
}

void redimensionaCallback(GLFWwindow* window, int w, int h) {
    WIDTH = w;
    HEIGHT = h;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (primeiro_mouse) {
        lastX = xpos;
        lastY = ypos;
        primeiro_mouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensibilidade = 0.1f;
    Cam_yaw -= xoffset * sensibilidade;
    Cam_pitch += yoffset * sensibilidade;

    // Trava do pitch para evitar o gimbal lock
    if (Cam_pitch > 89.0f) Cam_pitch = 89.0f;
    if (Cam_pitch < -89.0f) Cam_pitch = -89.0f;
}

void inicializaOpenGL() {
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar o GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    Window = glfwCreateWindow(WIDTH, HEIGHT, "Vila Virtual", NULL, NULL);
    if (!Window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowSizeCallback(Window, redimensionaCallback);
    glfwSetCursorPosCallback(Window, mouse_callback);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(Window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar o GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Placa de video: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Versao do OpenGL: " << glGetString(GL_VERSION) << std::endl;
}

void adicionaVertice(std::vector<float>& v, glm::vec3 p, glm::vec3 n, glm::vec2 uv) {
    v.insert(v.end(), { p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y });
}

// Quadrilátero formado por 2 triângulos que compartilham 4 vértices (6 índices)
void adicionaQuad(std::vector<float>& v, std::vector<unsigned int>& idx,
                  glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 n) {
    unsigned int base = v.size() / 8;
    adicionaVertice(v, p0, n, { 0.0f, 0.0f });
    adicionaVertice(v, p1, n, { 1.0f, 0.0f });
    adicionaVertice(v, p2, n, { 1.0f, 1.0f });
    adicionaVertice(v, p3, n, { 0.0f, 1.0f });
    idx.insert(idx.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
}

void adicionaTriangulo(std::vector<float>& v, std::vector<unsigned int>& idx,
                       glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 n) {
    unsigned int base = v.size() / 8;
    adicionaVertice(v, p0, n, { 0.0f, 0.0f });
    adicionaVertice(v, p1, n, { 1.0f, 0.0f });
    adicionaVertice(v, p2, n, { 0.5f, 1.0f });
    idx.insert(idx.end(), { base, base + 1, base + 2 });
}

// Envia vértices e índices para a GPU: VAO + VBO + EBO
Malha criaMalha(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Malha m;
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m.quantidade = indices.size();
    m.usaIndices = true;
    return m;
}

Malha criaCubo() {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    adicionaQuad(v, idx, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0.5f, 1, 0.5f}, {-0.5f, 1, 0.5f}, {0, 0, 1});       // frente
    adicionaQuad(v, idx, {0.5f, 0, -0.5f}, {-0.5f, 0, -0.5f}, {-0.5f, 1, -0.5f}, {0.5f, 1, -0.5f}, {0, 0, -1});  // trás
    adicionaQuad(v, idx, {0.5f, 0, 0.5f}, {0.5f, 0, -0.5f}, {0.5f, 1, -0.5f}, {0.5f, 1, 0.5f}, {1, 0, 0});       // direita
    adicionaQuad(v, idx, {-0.5f, 0, -0.5f}, {-0.5f, 0, 0.5f}, {-0.5f, 1, 0.5f}, {-0.5f, 1, -0.5f}, {-1, 0, 0});  // esquerda
    adicionaQuad(v, idx, {-0.5f, 1, 0.5f}, {0.5f, 1, 0.5f}, {0.5f, 1, -0.5f}, {-0.5f, 1, -0.5f}, {0, 1, 0});     // cima
    adicionaQuad(v, idx, {-0.5f, 0, -0.5f}, {0.5f, 0, -0.5f}, {0.5f, 0, 0.5f}, {-0.5f, 0, 0.5f}, {0, -1, 0});    // baixo
    return criaMalha(v, idx);
}

Malha criaPlano() {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    adicionaQuad(v, idx, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0.5f, 0, -0.5f}, {-0.5f, 0, -0.5f}, {0, 1, 0});
    return criaMalha(v, idx);
}

// Pirâmide de base quadrada com o topo em (0, 1, 0)
Malha criaPiramide() {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    glm::vec3 topo(0.0f, 1.0f, 0.0f);
    glm::vec3 a(-0.5f, 0, 0.5f), b(0.5f, 0, 0.5f), c(0.5f, 0, -0.5f), d(-0.5f, 0, -0.5f);
    adicionaTriangulo(v, idx, a, b, topo, glm::normalize(glm::vec3(0, 0.5f, 1)));
    adicionaTriangulo(v, idx, b, c, topo, glm::normalize(glm::vec3(1, 0.5f, 0)));
    adicionaTriangulo(v, idx, c, d, topo, glm::normalize(glm::vec3(0, 0.5f, -1)));
    adicionaTriangulo(v, idx, d, a, topo, glm::normalize(glm::vec3(-1, 0.5f, 0)));
    adicionaQuad(v, idx, d, c, b, a, {0, -1, 0});
    return criaMalha(v, idx);
}

// Prisma triangular (formato da empena de uma casa), com a cumeeira ao longo de Z
Malha criaPrisma() {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    adicionaQuad(v, idx, {-0.5f, 0, 0.5f}, {-0.5f, 0, -0.5f}, {0, 1, -0.5f}, {0, 1, 0.5f}, glm::normalize(glm::vec3(-1, 0.5f, 0)));
    adicionaQuad(v, idx, {0.5f, 0, -0.5f}, {0.5f, 0, 0.5f}, {0, 1, 0.5f}, {0, 1, -0.5f}, glm::normalize(glm::vec3(1, 0.5f, 0)));
    adicionaTriangulo(v, idx, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0, 1, 0.5f}, {0, 0, 1});
    adicionaTriangulo(v, idx, {0.5f, 0, -0.5f}, {-0.5f, 0, -0.5f}, {0, 1, -0.5f}, {0, 0, -1});
    adicionaQuad(v, idx, {-0.5f, 0, -0.5f}, {0.5f, 0, -0.5f}, {0.5f, 0, 0.5f}, {-0.5f, 0, 0.5f}, {0, -1, 0});
    return criaMalha(v, idx);
}

// Tampa circular (usada pelo cilindro e pelo cone): um vértice central ligado à borda
void adicionaTampa(std::vector<float>& v, std::vector<unsigned int>& idx, int lados, float y, glm::vec3 n) {
    unsigned int centro = v.size() / 8;
    adicionaVertice(v, {0.0f, y, 0.0f}, n, {0.5f, 0.5f});
    for (int i = 0; i <= lados; i++) {
        float a = 2.0f * PI * i / lados;
        adicionaVertice(v, {0.5f * cos(a), y, 0.5f * sin(a)}, n, {0.5f + 0.5f * cos(a), 0.5f + 0.5f * sin(a)});
    }
    for (int i = 0; i < lados; i++) {
        idx.insert(idx.end(), { centro, centro + 1 + i, centro + 2 + i });
    }
}

// Cilindro de raio 0.5 e altura 1, calculado com x = r*cos(a) e z = r*sin(a)
Malha criaCilindro(int lados) {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    for (int i = 0; i <= lados; i++) {
        float a = 2.0f * PI * i / lados;
        glm::vec3 normal(cos(a), 0.0f, sin(a));
        glm::vec3 p(0.5f * cos(a), 0.0f, 0.5f * sin(a));
        float u = (float)i / lados;
        adicionaVertice(v, p, normal, {u, 0.0f});
        adicionaVertice(v, p + glm::vec3(0, 1, 0), normal, {u, 1.0f});
    }
    for (int i = 0; i < lados; i++) {
        unsigned int b = i * 2;
        idx.insert(idx.end(), { b, b + 1, b + 3, b, b + 3, b + 2 });
    }
    adicionaTampa(v, idx, lados, 0.0f, {0, -1, 0});
    adicionaTampa(v, idx, lados, 1.0f, {0, 1, 0});
    return criaMalha(v, idx);
}

// Cone de raio 0.5 e altura 1
Malha criaCone(int lados) {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    for (int i = 0; i < lados; i++) {
        float a0 = 2.0f * PI * i / lados;
        float a1 = 2.0f * PI * (i + 1) / lados;
        float meio = (a0 + a1) / 2.0f;
        glm::vec3 normal = glm::normalize(glm::vec3(cos(meio), 0.5f, sin(meio)));
        unsigned int base = v.size() / 8;
        adicionaVertice(v, {0.5f * cos(a0), 0, 0.5f * sin(a0)}, normal, {(float)i / lados, 0.0f});
        adicionaVertice(v, {0.5f * cos(a1), 0, 0.5f * sin(a1)}, normal, {(float)(i + 1) / lados, 0.0f});
        adicionaVertice(v, {0.0f, 1.0f, 0.0f}, normal, {(i + 0.5f) / lados, 1.0f});
        idx.insert(idx.end(), { base, base + 1, base + 2 });
    }
    adicionaTampa(v, idx, lados, 0.0f, {0, -1, 0});
    return criaMalha(v, idx);
}

// Esfera de raio 0.5 centralizada na origem, gerada por anéis (latitude) e setores (longitude)
Malha criaEsfera(int setores, int aneis) {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    for (int i = 0; i <= aneis; i++) {
        float phi = PI * i / aneis;
        for (int j = 0; j <= setores; j++) {
            float theta = 2.0f * PI * j / setores;
            glm::vec3 normal(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
            adicionaVertice(v, normal * 0.5f, normal, {(float)j / setores, 1.0f - (float)i / aneis});
        }
    }
    for (int i = 0; i < aneis; i++) {
        for (int j = 0; j < setores; j++) {
            unsigned int a = i * (setores + 1) + j;
            unsigned int b = a + setores + 1;
            idx.insert(idx.end(), { a, b, a + 1, a + 1, b, b + 1 });
        }
    }
    return criaMalha(v, idx);
}

// ---------------------------------------------------------------------------------------
// Carregamento de arquivos externos (modelos .obj e texturas)
// ---------------------------------------------------------------------------------------

// O executável roda dentro da pasta build, então os assets ficam em ../assets
std::string caminhoAsset(const std::string& relativo) {
    std::ifstream teste("../assets/" + relativo);
    if (teste.good()) return "../assets/" + relativo;
    return "assets/" + relativo;
}

// Leitor de .obj baseado no loadSimpleOBJ visto em aula, agora lendo também normais e UVs.
// Cada face "f v/vt/vn" usa índices (começando em 1) para buscar os dados nas listas lidas.
Malha carregaOBJ(const std::string& caminho) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normais;
    std::vector<float> vBuffer;

    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o modelo " << caminho << std::endl;
        return Malha();
    }

    std::string linha;
    while (std::getline(arquivo, linha)) {
        std::istringstream ssLinha(linha);
        std::string palavra;
        ssLinha >> palavra;

        if (palavra == "v") {
            glm::vec3 p;
            ssLinha >> p.x >> p.y >> p.z;
            vertices.push_back(p);
        } else if (palavra == "vt") {
            glm::vec2 t;
            ssLinha >> t.x >> t.y;
            texCoords.push_back(t);
        } else if (palavra == "vn") {
            glm::vec3 n;
            ssLinha >> n.x >> n.y >> n.z;
            normais.push_back(n);
        } else if (palavra == "f") {
            while (ssLinha >> palavra) {
                int vi = 0, ti = -1, ni = -1;
                std::istringstream ss(palavra);
                std::string indice;
                if (std::getline(ss, indice, '/')) vi = std::stoi(indice) - 1;
                if (std::getline(ss, indice, '/') && !indice.empty()) ti = std::stoi(indice) - 1;
                if (std::getline(ss, indice) && !indice.empty()) ni = std::stoi(indice) - 1;

                glm::vec3 p = vertices[vi];
                glm::vec3 n = ni >= 0 ? normais[ni] : glm::vec3(0.0f, 1.0f, 0.0f);
                glm::vec2 t = ti >= 0 ? texCoords[ti] : glm::vec2(0.0f);
                adicionaVertice(vBuffer, p, n, t);
            }
        }
    }

    Malha m;
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(float), vBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m.quantidade = vBuffer.size() / 8;
    m.usaIndices = false;
    std::cout << "Modelo carregado: " << caminho << " (" << m.quantidade << " vertices)" << std::endl;
    return m;
}

GLuint carregaTextura(const std::string& caminho) {
    GLuint textura;
    glGenTextures(1, &textura);
    glBindTexture(GL_TEXTURE_2D, textura);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int largura, altura, canais;
    // pede sempre 4 canais (RGBA), assim qualquer PNG/JPG da internet funciona igual
    unsigned char* dados = stbi_load(caminho.c_str(), &largura, &altura, &canais, 4);
    if (dados) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, largura, altura, 0, GL_RGBA, GL_UNSIGNED_BYTE, dados);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Erro ao carregar a textura " << caminho << std::endl;
    }
    stbi_image_free(dados);
    return textura;
}

void inicializaObjetos() {
    Cubo = criaCubo();
    Plano = criaPlano();
    Piramide = criaPiramide();
    Prisma = criaPrisma();
    Cilindro = criaCilindro(24);
    Cone = criaCone(24);
    Esfera = criaEsfera(24, 16);

    Pinheiro = carregaOBJ(caminhoAsset("Modelos3D/pinheiro.obj"));
    Barril = carregaOBJ(caminhoAsset("Modelos3D/barril.obj"));
    TexGrama = carregaTextura(caminhoAsset("tex/grass.jpg"));
    TexFolhas = carregaTextura(caminhoAsset("tex/leaves.jpg"));
    TexTerra = carregaTextura(caminhoAsset("tex/dirt.jpg"));
    TexMadeira = carregaTextura(caminhoAsset("tex/madeira.jpg"));
    TexTelhado = carregaTextura(caminhoAsset("tex/roof.jpg"));
    TexPedra = carregaTextura(caminhoAsset("tex/brick.jpg"));
    TexPalha = carregaTextura(caminhoAsset("tex/straw.jpg"));
    TexPinheiro = carregaTextura(caminhoAsset("tex/pinheiro.png"));
    TexBarril = carregaTextura(caminhoAsset("tex/barril.png"));
}

// ---------------------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------------------
void inicializaShaders() {
    // Vertex Shader: aplica Projection * View * Model e repassa normal e UV ao fragment shader
    const char* vertex_shader =
        "#version 400\n"
        "layout(location = 0) in vec3 vertex_posicao;\n"
        "layout(location = 1) in vec3 vertex_normal;\n"
        "layout(location = 2) in vec2 vertex_uv;\n"
        "uniform mat4 model, view, proj;\n"
        "uniform vec2 repeticaoUV;\n"
        "out vec3 normal;\n"
        "out vec2 uv;\n"
        "void main () {\n"
        "    normal = mat3(transpose(inverse(model))) * vertex_normal;\n"
        "    uv = vertex_uv * repeticaoUV;\n"
        "    gl_Position = proj * view * model * vec4(vertex_posicao, 1.0);\n"
        "}\n";

    // Fragment Shader: cor do objeto (uniform) multiplicada pela textura (sampler2D),
    // neve nas superfícies voltadas para cima e iluminação difusa simples do sol/lua
    const char* fragment_shader =
        "#version 400\n"
        "in vec3 normal;\n"
        "in vec2 uv;\n"
        "uniform sampler2D textura;\n"
        "uniform bool usaTextura;\n"
        "uniform bool emissivo;\n"
        "uniform vec3 corObjeto;\n"
        "uniform vec3 direcaoLuz;\n"
        "uniform vec3 corLuz;\n"
        "uniform float luzAmbiente;\n"
        "uniform float neve;\n"
        "out vec4 frag_colour;\n"
        "void main () {\n"
        "    vec3 cor = corObjeto;\n"
        "    if (usaTextura) cor *= texture(textura, uv).rgb;\n"
        "    if (emissivo) {\n"
        "        frag_colour = vec4(cor, 1.0);\n"
        "        return;\n"
        "    }\n"
        "    vec3 n = normalize(normal);\n"
        "    float cobertura = neve * smoothstep(0.4, 0.8, n.y);\n"
        "    cor = mix(cor, vec3(0.95, 0.97, 1.0), cobertura);\n"
        "    float difusa = max(dot(n, normalize(direcaoLuz)), 0.0);\n"
        "    vec3 luz = vec3(luzAmbiente) + difusa * corLuz;\n"
        "    frag_colour = vec4(cor * luz, 1.0);\n"
        "}\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    GLint success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cerr << "Erro no vertex shader:\n" << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cerr << "Erro no fragment shader:\n" << infoLog << std::endl;
    }

    Shader_programm = glCreateProgram();
    glAttachShader(Shader_programm, vs);
    glAttachShader(Shader_programm, fs);
    glLinkProgram(Shader_programm);

    glGetProgramiv(Shader_programm, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(Shader_programm, 512, NULL, infoLog);
        std::cerr << "Erro na linkagem do shader:\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    glUseProgram(Shader_programm);
    glUniform1i(glGetUniformLocation(Shader_programm, "textura"), 0);
}

GLint uniform(const char* nome) {
    return glGetUniformLocation(Shader_programm, nome);
}

// Matriz model a partir de translação, rotação em Y e escala (nesta ordem no GLM)
glm::mat4 transforma(glm::vec3 posicao, float rotacaoY, glm::vec3 escala) {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, posicao);
    m = glm::rotate(m, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::scale(m, escala);
    return m;
}

// Desenha uma malha com sua matriz model, cor e textura (textura 0 = sem textura)
void desenha(const Malha& malha, const glm::mat4& model, glm::vec3 cor, GLuint textura,
             glm::vec2 repeticao = glm::vec2(1.0f), bool emissivo = false) {
    glUniformMatrix4fv(uniform("model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(uniform("corObjeto"), 1, glm::value_ptr(cor));
    glUniform2fv(uniform("repeticaoUV"), 1, glm::value_ptr(repeticao));
    glUniform1i(uniform("usaTextura"), textura != 0);
    glUniform1i(uniform("emissivo"), emissivo);

    if (textura != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textura);
    }

    glBindVertexArray(malha.vao);
    if (malha.usaIndices) {
        glDrawElements(GL_TRIANGLES, malha.quantidade, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, malha.quantidade);
    }
}

const float CASA_LARGURA = 4.0f;
const float CASA_PROFUNDIDADE = 4.0f;
const float CASA_ALTURA = 3.0f;
const float CASA_ALTURA_TELHADO = 1.6f;

glm::mat4 matrizCasa(const Casa& casa) {
    return transforma(casa.posicao, casa.rotacao, glm::vec3(casa.escala));
}

void desenhaCasa(const Casa& casa, float noite) {
    glm::mat4 base = matrizCasa(casa);
    float L = CASA_LARGURA, P = CASA_PROFUNDIDADE, A = CASA_ALTURA, AT = CASA_ALTURA_TELHADO;

    // paredes e empena
    desenha(Cubo, glm::scale(base, glm::vec3(L, A, P)), casa.corParede, TexMadeira, {2.0f, 2.0f});
    desenha(Prisma, glm::scale(glm::translate(base, glm::vec3(0, A, 0)), glm::vec3(L, AT, P)), casa.corParede, TexMadeira, {2.0f, 1.0f});

    // telhado: duas placas inclinadas, uma de cada lado da cumeeira
    float meiaLargura = L / 2.0f;
    float comprimento = sqrt(meiaLargura * meiaLargura + AT * AT);
    float inclinacao = glm::degrees(atan2(AT, meiaLargura));
    for (int lado = -1; lado <= 1; lado += 2) {
        glm::mat4 m = base;
        m = glm::translate(m, glm::vec3(lado * meiaLargura / 2.0f, A + AT / 2.0f, 0.0f));
        m = glm::rotate(m, glm::radians(-lado * inclinacao), glm::vec3(0, 0, 1));
        m = glm::scale(m, glm::vec3(comprimento + 0.5f, 0.15f, P + 0.6f));
        desenha(Cubo, m, casa.corTelhado, TexTelhado, {3.0f, 3.0f});
    }

    if (casa.chamine) {
        desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(L * 0.25f, A + AT * 0.3f, -P * 0.2f)), glm::vec3(0.6f, 1.8f, 0.6f)),
                glm::vec3(1.0f), TexPedra);
    }

    // vão escuro da porta (aparece quando a porta abre)
    float lp = casa.larguraPorta, ap = casa.alturaPorta;
    desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(0, 0, P / 2.0f + 0.01f)), glm::vec3(lp, ap, 0.02f)),
            glm::vec3(0.08f, 0.06f, 0.05f), 0);

    // porta: gira em torno da dobradiça, que fica na borda esquerda
    glm::mat4 porta = base;
    porta = glm::translate(porta, glm::vec3(-lp / 2.0f, 0.0f, P / 2.0f + 0.05f));   // leva até a dobradiça
    porta = glm::rotate(porta, glm::radians(-casa.anguloPorta), glm::vec3(0, 1, 0)); // abre
    porta = glm::translate(porta, glm::vec3(lp / 2.0f, 0.0f, 0.0f));                 // volta para o centro da porta
    porta = glm::scale(porta, glm::vec3(lp, ap, 0.08f));
    desenha(Cubo, porta, glm::vec3(0.6f, 0.45f, 0.35f), TexMadeira);

    // janelas: escuras de dia e acesas (amarelas) à noite
    glm::vec3 corJanela = glm::mix(glm::vec3(0.2f, 0.25f, 0.3f), glm::vec3(1.0f, 0.82f, 0.4f), noite);
    for (int lado = -1; lado <= 1; lado += 2) {
        desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(lado * 1.4f, 1.3f, P / 2.0f + 0.02f)), glm::vec3(0.6f, 0.7f, 0.04f)),
                corJanela, 0, glm::vec2(1.0f), true);
        desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(lado * (L / 2.0f + 0.02f), 1.3f, 0.0f)), glm::vec3(0.04f, 0.7f, 0.8f)),
                corJanela, 0, glm::vec2(1.0f), true);
    }
}

void desenhaMoinho(glm::vec3 posicao, float rotacao, float anguloPas) {
    glm::mat4 base = transforma(posicao, rotacao, glm::vec3(1.0f));

    desenha(Cilindro, glm::scale(base, glm::vec3(3.4f, 7.0f, 3.4f)), glm::vec3(1.0f), TexPedra, {4.0f, 3.0f});
    desenha(Cone, glm::scale(glm::translate(base, glm::vec3(0, 7.0f, 0)), glm::vec3(4.2f, 2.6f, 4.2f)), glm::vec3(1.0f), TexTelhado, {4.0f, 2.0f});
    desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(0, 0, 1.68f)), glm::vec3(1.0f, 1.9f, 0.1f)), glm::vec3(0.5f, 0.35f, 0.25f), TexMadeira);

    // eixo das pás, apontando para a frente (+Z)
    glm::mat4 eixo = glm::translate(base, glm::vec3(0.0f, 6.3f, 1.7f));
    desenha(Cilindro, glm::scale(glm::rotate(eixo, glm::radians(90.0f), glm::vec3(1, 0, 0)), glm::vec3(0.5f, 0.6f, 0.5f)),
            glm::vec3(0.45f, 0.35f, 0.25f), TexMadeira);

    // 4 pás, cada uma girada 90 graus em relação à anterior, todas girando com o tempo
    for (int i = 0; i < 4; i++) {
        glm::mat4 pa = eixo;
        pa = glm::translate(pa, glm::vec3(0.0f, 0.0f, 0.6f));
        pa = glm::rotate(pa, glm::radians(anguloPas + i * 90.0f), glm::vec3(0, 0, 1));
        pa = glm::translate(pa, glm::vec3(0.0f, 0.25f, 0.0f));
        pa = glm::scale(pa, glm::vec3(0.7f, 3.4f, 0.06f));
        desenha(Cubo, pa, glm::vec3(1.0f, 0.95f, 0.85f), TexMadeira, {1.0f, 3.0f});
    }
}

void desenhaPoco(glm::vec3 posicao) {
    glm::mat4 base = glm::translate(glm::mat4(1.0f), posicao);
    glm::vec3 corMadeira(0.7f, 0.55f, 0.45f);

    desenha(Cilindro, glm::scale(base, glm::vec3(2.4f, 1.0f, 2.4f)), glm::vec3(1.0f), TexPedra, {3.0f, 1.0f});
    desenha(Cilindro, glm::scale(glm::translate(base, glm::vec3(0, 0.85f, 0)), glm::vec3(2.0f, 0.05f, 2.0f)), glm::vec3(0.2f, 0.4f, 0.7f), 0);
    for (int lado = -1; lado <= 1; lado += 2) {
        desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(lado * 1.05f, 1.0f, 0)), glm::vec3(0.18f, 1.5f, 0.18f)), corMadeira, TexMadeira);
    }
    desenha(Piramide, glm::scale(glm::translate(base, glm::vec3(0, 2.5f, 0)), glm::vec3(3.0f, 1.0f, 3.0f)), glm::vec3(1.0f), TexTelhado, {2.0f, 2.0f});

    // sarilho (cilindro deitado), corda e balde
    glm::mat4 sarilho = glm::translate(base, glm::vec3(-1.05f, 2.1f, 0));
    sarilho = glm::rotate(sarilho, glm::radians(-90.0f), glm::vec3(0, 0, 1));
    desenha(Cilindro, glm::scale(sarilho, glm::vec3(0.15f, 2.1f, 0.15f)), corMadeira, TexMadeira);
    desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(0, 1.5f, 0)), glm::vec3(0.03f, 0.6f, 0.03f)), glm::vec3(0.8f, 0.7f, 0.5f), 0);
    desenha(Cilindro, glm::scale(glm::translate(base, glm::vec3(0, 1.1f, 0)), glm::vec3(0.35f, 0.4f, 0.35f)), corMadeira, TexMadeira);
}

void desenhaArvore(glm::vec3 posicao, float escala, glm::vec3 corFolhagem) {
    glm::mat4 base = transforma(posicao, 0.0f, glm::vec3(escala));
    desenha(Cilindro, glm::scale(base, glm::vec3(0.45f, 2.2f, 0.45f)), glm::vec3(0.75f, 0.6f, 0.5f), TexMadeira, {1.0f, 2.0f});
    desenha(Esfera, glm::scale(glm::translate(base, glm::vec3(0, 3.0f, 0)), glm::vec3(2.8f, 2.4f, 2.8f)), corFolhagem, TexFolhas, {2.0f, 2.0f});
    desenha(Esfera, glm::scale(glm::translate(base, glm::vec3(0.5f, 3.9f, 0.3f)), glm::vec3(1.8f, 1.6f, 1.8f)), corFolhagem * 0.9f, TexFolhas, {2.0f, 2.0f});
}

// Um trecho de cerca: um poste no início e duas travessas até o fim
void desenhaTrechoCerca(glm::vec3 inicio, glm::vec3 fim) {
    glm::vec3 corMadeira(0.8f, 0.65f, 0.5f);
    glm::vec3 meio = (inicio + fim) * 0.5f;
    float comprimento = glm::length(fim - inicio);
    float angulo = glm::degrees(atan2(-(fim.z - inicio.z), fim.x - inicio.x));

    desenha(Cubo, glm::scale(glm::translate(glm::mat4(1.0f), inicio), glm::vec3(0.18f, 1.2f, 0.18f)), corMadeira, TexMadeira);
    for (float altura : { 0.45f, 0.9f }) {
        glm::mat4 travessa = glm::translate(glm::mat4(1.0f), meio + glm::vec3(0, altura, 0));
        travessa = glm::rotate(travessa, glm::radians(angulo), glm::vec3(0, 1, 0));
        travessa = glm::scale(travessa, glm::vec3(comprimento, 0.1f, 0.06f));
        desenha(Cubo, travessa, corMadeira, TexMadeira, {2.0f, 1.0f});
    }
}

void desenhaCercado(float xMin, float xMax, float zMin, float zMax, float porteiraX) {
    const float passo = 2.0f;
    for (float x = xMin; x < xMax; x += passo) {
        desenhaTrechoCerca({x, 0, zMin}, {x + passo, 0, zMin});
        if (fabs(x - porteiraX) > 0.01f) {
            desenhaTrechoCerca({x, 0, zMax}, {x + passo, 0, zMax});
        }
    }
    for (float z = zMin; z < zMax; z += passo) {
        desenhaTrechoCerca({xMin, 0, z}, {xMin, 0, z + passo});
        desenhaTrechoCerca({xMax, 0, z}, {xMax, 0, z + passo});
    }
    desenhaTrechoCerca({xMax, 0, zMax}, {xMax, 0, zMax});
    desenhaTrechoCerca({porteiraX, 0, zMax}, {porteiraX, 0, zMax});
}

// Fardo de feno: cilindro deitado (girado 90 graus em X)
void desenhaFeno(glm::vec3 posicao, float rotacao) {
    glm::mat4 m = transforma(posicao, rotacao, glm::vec3(1.0f));
    m = glm::translate(m, glm::vec3(0.0f, 0.7f, -0.6f));
    m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1, 0, 0));
    m = glm::scale(m, glm::vec3(1.4f, 1.2f, 1.4f));
    desenha(Cilindro, m, glm::vec3(1.0f), TexPalha, {3.0f, 1.0f});
}

// Marcador do jogador no minimapa: um corpo e uma "ponta" indicando para onde a câmera olha
void desenhaMarcadorJogador() {
    glm::mat4 base = transforma(glm::vec3(Cam_pos.x, 12.0f, Cam_pos.z), Cam_yaw, glm::vec3(1.0f));
    desenha(Cubo, glm::scale(base, glm::vec3(2.4f, 0.3f, 3.0f)), glm::vec3(1.0f, 0.15f, 0.1f), 0, glm::vec2(1.0f), true);
    desenha(Cubo, glm::scale(glm::translate(base, glm::vec3(0, 0, -2.2f)), glm::vec3(1.2f, 0.3f, 1.6f)), glm::vec3(1.0f, 0.9f, 0.2f), 0, glm::vec2(1.0f), true);
}

// ---------------------------------------------------------------------------------------
// Cena completa
// ---------------------------------------------------------------------------------------
void desenhaCena(bool minimapa, float tempo, float anguloDia, float noite) {
    const Estacao& estacao = Estacoes[Estacao_atual];

    // chão, caminhos de terra e praça de pedra
    desenha(Plano, glm::scale(glm::mat4(1.0f), glm::vec3(200.0f, 1.0f, 200.0f)), estacao.corGrama, TexGrama, {60.0f, 60.0f});
    desenha(Plano, transforma({0, 0.03f, 0}, 0, {80.0f, 1.0f, 3.0f}), glm::vec3(1.0f), TexTerra, {27.0f, 1.0f});
    desenha(Plano, transforma({0, 0.04f, 0}, 0, {3.0f, 1.0f, 80.0f}), glm::vec3(1.0f), TexTerra, {1.0f, 27.0f});
    desenha(Plano, transforma({17.0f, 0.05f, -7.75f}, 0, {2.0f, 1.0f, 12.5f}), glm::vec3(1.0f), TexTerra, {1.0f, 6.0f});
    desenha(Plano, transforma({0, 0.06f, 0}, 0, {10.0f, 1.0f, 10.0f}), glm::vec3(0.9f), TexPedra, {4.0f, 4.0f});

    for (const Casa& casa : Casas) {
        desenhaCasa(casa, noite);
    }

    desenhaMoinho({-18.0f, 0.0f, -20.0f}, 42.0f, tempo * 40.0f);
    desenhaPoco({0.0f, 0.0f, 0.0f});

    // árvores frondosas: a cor da copa depende da estação
    glm::vec3 arvores[] = {
        {-14, 0, -13}, {-4, 0, -13}, {4, 0, -15}, {15, 0, -3}, {-15, 0, 3},
        {-3, 0, 10}, {4, 0, 12}, {-14, 0, 13}, {21, 0, -4}, {-24, 0, 1},
    };
    int i = 0;
    for (glm::vec3 p : arvores) {
        float variacao = 0.85f + 0.1f * (i % 3);
        desenhaArvore(p, 0.85f + 0.15f * (i % 4), estacao.corFolhagem * variacao);
        i++;
    }

    // pinheiros (modelo .obj) contornando a vila
    glm::vec4 pinheiros[] = {   // x, z, escala, rotação
        {-28, -26, 1.6f, 0}, {-24, -29, 1.3f, 40}, {-31, -12, 1.8f, 10}, {-30, 6, 1.5f, 70},
        {-27, 20, 1.7f, 20}, {-20, 27, 1.4f, 55}, {-10, 29, 1.6f, 5}, {8, 28, 1.8f, 30},
        {18, 26, 1.5f, 80}, {28, 19, 1.7f, 15}, {30, 6, 1.4f, 45}, {29, -10, 1.6f, 60},
        {28, -29, 1.8f, 25}, {4, -29, 1.5f, 35},
    };
    for (glm::vec4 p : pinheiros) {
        desenha(Pinheiro, transforma({p.x, 0, p.y}, p.w, glm::vec3(p.z)), glm::vec3(1.0f), TexPinheiro);
    }

    // barris (modelo .obj) perto das casas e do celeiro
    glm::vec4 barris[] = {   // x, z, escala, rotação
        {-11.6f, -4.4f, 1.0f, 0}, {6.4f, -4.3f, 0.9f, 30}, {7.3f, -3.9f, 1.0f, 70},
        {8.2f, 4.6f, 1.0f, 10}, {9.1f, 4.3f, 0.9f, 50}, {15.8f, 4.5f, 1.1f, 20}, {2.6f, 3.0f, 0.8f, 0},
    };
    for (glm::vec4 b : barris) {
        desenha(Barril, transforma({b.x, 0, b.y}, b.w, glm::vec3(b.z)), glm::vec3(1.0f), TexBarril);
    }

    // plantação cercada com fardos de feno
    desenhaCercado(8.0f, 24.0f, -26.0f, -14.0f, 16.0f);
    desenhaFeno({12, 0, -18}, 20);
    desenhaFeno({15, 0, -22}, 80);
    desenhaFeno({20, 0, -17}, 0);
    desenhaFeno({21, 0, -23}, 45);
    desenhaFeno({11, 0, -24}, 110);

    if (minimapa) {
        desenhaMarcadorJogador();
    } else {
        glm::mat4 inclinacao = glm::rotate(glm::mat4(1.0f), glm::radians(-25.0f), glm::vec3(1, 0, 0));
        glm::mat4 sol = glm::rotate(inclinacao, anguloDia, glm::vec3(0, 0, 1));
        sol = glm::translate(sol, glm::vec3(45.0f, 0.0f, 0.0f));
        sol = glm::scale(sol, glm::vec3(5.0f));
        desenha(Esfera, sol, glm::vec3(1.0f, 0.9f, 0.4f), 0, glm::vec2(1.0f), true);

        glm::mat4 lua = glm::rotate(inclinacao, anguloDia + PI, glm::vec3(0, 0, 1));
        lua = glm::translate(lua, glm::vec3(45.0f, 0.0f, 0.0f));
        lua = glm::scale(lua, glm::vec3(3.0f));
        desenha(Esfera, lua, glm::vec3(0.85f, 0.88f, 0.95f), 0, glm::vec2(1.0f), true);
    }
}

bool apertouTecla(int tecla) {
    static bool anterior[GLFW_KEY_LAST + 1] = {};
    bool agora = glfwGetKey(Window, tecla) == GLFW_PRESS;
    bool apertou = agora && !anterior[tecla];
    anterior[tecla] = agora;
    return apertou;
}

void alternaPortaMaisProxima() {
    int maisProxima = -1;
    float menorDistancia = 7.0f;
    for (int i = 0; i < (int)Casas.size(); i++) {
        glm::vec3 frente = glm::vec3(matrizCasa(Casas[i]) * glm::vec4(0.0f, 0.0f, CASA_PROFUNDIDADE / 2.0f, 1.0f));
        float distancia = glm::length(frente - Cam_pos);
        if (distancia < menorDistancia) {
            menorDistancia = distancia;
            maisProxima = i;
        }
    }
    if (maisProxima >= 0) {
        Casas[maisProxima].portaAberta = !Casas[maisProxima].portaAberta;
    } else {
        std::cout << "Nenhuma porta por perto" << std::endl;
    }
}

void trataTeclado() {
    if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(Window, true);
    }

    if (apertouTecla(GLFW_KEY_F)) {
        alternaPortaMaisProxima();
    }
    if (apertouTecla(GLFW_KEY_C)) {
        Estacao_atual = (Estacao_atual + 1) % 3;
        std::cout << "Estacao: " << Estacoes[Estacao_atual].nome << std::endl;
    }
    if (apertouTecla(GLFW_KEY_T)) {
        float atual = fmod(calculaAnguloDia((float)glfwGetTime()), 2.0f * PI);
        bool dia = sin(atual) > 0.0f;
        float alvo = dia ? PI + 0.15f : 2.0f * PI + 0.35f;
        Ajuste_dia += alvo - atual;
        std::cout << (dia ? "Noite" : "Dia") << std::endl;
    }

    float velocidade = Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) velocidade *= 2.5f;

    glm::vec3 Cam_right = glm::normalize(glm::cross(Cam_front, Cam_up));
    if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS) Cam_pos += Cam_front * velocidade;
    if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS) Cam_pos -= Cam_front * velocidade;
    if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS) Cam_pos -= Cam_right * velocidade;
    if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS) Cam_pos += Cam_right * velocidade;
    if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS) Cam_pos.y += velocidade;
    if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS) Cam_pos.y -= velocidade;

    // não deixa a câmera atravessar o chão
    if (Cam_pos.y < 0.5f) Cam_pos.y = 0.5f;
}

void atualizaPortas() {
    float passo = 150.0f * Tempo_entre_frames;
    for (Casa& casa : Casas) {
        float alvo = casa.portaAberta ? 100.0f : 0.0f;
        if (casa.anguloPorta < alvo) casa.anguloPorta = glm::min(casa.anguloPorta + passo, alvo);
        if (casa.anguloPorta > alvo) casa.anguloPorta = glm::max(casa.anguloPorta - passo, alvo);
    }
}

void atualizaDirecaoCamera() {
    glm::vec3 front;
    front.x = sin(glm::radians(-Cam_yaw)) * cos(glm::radians(Cam_pitch));
    front.y = sin(glm::radians(Cam_pitch));
    front.z = -cos(glm::radians(-Cam_yaw)) * cos(glm::radians(Cam_pitch));
    Cam_front = glm::normalize(front);
}

void enviaCamera(const glm::mat4& view, const glm::mat4& proj) {
    glUniformMatrix4fv(uniform("view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uniform("proj"), 1, GL_FALSE, glm::value_ptr(proj));
}

void enviaLuz(glm::vec3 direcao, glm::vec3 cor, float ambiente) {
    glUniform3fv(uniform("direcaoLuz"), 1, glm::value_ptr(direcao));
    glUniform3fv(uniform("corLuz"), 1, glm::value_ptr(cor));
    glUniform1f(uniform("luzAmbiente"), ambiente);
}

// Laço principal
void inicializaRenderizacao() {
    double tempo_anterior = glfwGetTime();
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(Window)) {
        double tempo_frame_atual = glfwGetTime();
        Tempo_entre_frames = (float)(tempo_frame_atual - tempo_anterior);
        tempo_anterior = tempo_frame_atual;
        float tempo = (float)tempo_frame_atual;

        trataTeclado();
        atualizaPortas();
        atualizaDirecaoCamera();

        float anguloDia = calculaAnguloDia(tempo);
        float alturaSol = sin(anguloDia);
        float fatorDia = glm::smoothstep(-0.1f, 0.3f, alturaSol);
        float noite = 1.0f - fatorDia;

        glm::mat4 inclinacao = glm::rotate(glm::mat4(1.0f), glm::radians(-25.0f), glm::vec3(1, 0, 0));
        glm::vec3 posSol = glm::vec3(inclinacao * glm::vec4(cos(anguloDia), sin(anguloDia), 0.0f, 0.0f));
        glm::vec3 direcaoLuz = alturaSol > 0.0f ? posSol : -posSol;
        glm::vec3 corLuz = glm::mix(glm::vec3(0.25f, 0.3f, 0.45f), glm::vec3(0.85f, 0.8f, 0.7f), fatorDia);
        float ambiente = glm::mix(0.12f, 0.35f, fatorDia);

        const Estacao& estacao = Estacoes[Estacao_atual];
        glm::vec3 corCeu = glm::mix(glm::vec3(0.03f, 0.04f, 0.10f), estacao.corCeu, fatorDia);
        float porDoSol = glm::clamp(1.0f - fabs(alturaSol) / 0.3f, 0.0f, 1.0f);
        corCeu = glm::mix(corCeu, glm::vec3(0.95f, 0.55f, 0.3f), porDoSol * 0.6f);

        glUseProgram(Shader_programm);
        glUniform1f(uniform("neve"), estacao.neve);

        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(corCeu.r, corCeu.g, corCeu.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(Cam_pos, Cam_pos + Cam_front, Cam_up);
        glm::mat4 proj = glm::perspective(glm::radians(67.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 300.0f);
        enviaCamera(view, proj);
        enviaLuz(direcaoLuz, corLuz, ambiente);
        desenhaCena(false, tempo, anguloDia, noite);

        int tamanho = HEIGHT / 3;
        int x = WIDTH - tamanho - 15;
        int y = HEIGHT - tamanho - 15;

        glEnable(GL_SCISSOR_TEST);
        glScissor(x - 3, y - 3, tamanho + 6, tamanho + 6);
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glScissor(x, y, tamanho, tamanho);
        glClearColor(0.1f, 0.12f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        glViewport(x, y, tamanho, tamanho);
        glm::mat4 viewTopo = glm::lookAt(glm::vec3(0.0f, 80.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        glm::mat4 projOrtografica = glm::ortho(-32.0f, 32.0f, -32.0f, 32.0f, 1.0f, 200.0f);
        enviaCamera(viewTopo, projOrtografica);
        enviaLuz(glm::vec3(0.3f, 1.0f, 0.2f), glm::vec3(0.6f), 0.5f);
        desenhaCena(true, tempo, anguloDia, noite);

        glfwPollEvents();
        glfwSwapBuffers(Window);
    }

    glfwTerminate();
}

int main() {
    inicializaOpenGL();
    inicializaObjetos();
    inicializaShaders();

    std::cout << "\nControles: mouse olha | W/A/S/D anda | Q/E desce/sobe | Shift corre" << std::endl;
    std::cout << "           F abre/fecha porta | C troca estacao | T dia/noite | ESC sai\n" << std::endl;

    inicializaRenderizacao();
    return 0;
}
