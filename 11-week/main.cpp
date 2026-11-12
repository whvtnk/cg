#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image: без #define - только объявления функций, с ним - ещё и сам код (ровно в одном .cpp)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// stb_image_write - только для скриншотов (клавиша P и режим --shots)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// --- камера (9-я неделя) ---
glm::vec3 camPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f,  0.0f);

// --- мышь (10-я неделя) ---
float yaw   = -90.0f;
float pitch =   0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool  firstMouse = true;
float sensitivity = 0.1f;
float fov = 45.0f;

// WSL (WSLg) не умеет возвращать курсор в центр -> там CAPTURED вместо DISABLED (см. 10-ю неделю)
const bool wslMode = std::getenv("WSL_DISTRO_NAME") != nullptr;

int gWidth  = 800;
int gHeight = 600;

// --- текстура (11-я неделя): всё, что переключается клавишами ---
const GLint WRAP_MODES[4]  = {GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER};
const char* WRAP_NAMES[4]  = {"REPEAT", "MIRRORED_REPEAT", "CLAMP_TO_EDGE", "CLAMP_TO_BORDER"};
const float VERTEX_UV[3]   = {1.0f, 2.0f, 0.1f};   // множитель UV прямо в vertices[] (задания 1 и 2)

int   wrapMode    = 0;       // 1..4 - задание 1
bool  magNearest  = false;   // N    - задание 2
bool  useMipmap   = true;    // M    - задание 3 (MIN: LINEAR_MIPMAP_LINEAR <-> LINEAR)
bool  badMag      = false;   // B    - задание 4 (mipmap-константа в MAG, нарочно)
bool  noMipTex    = false;   // K    - задание 4, вторая причина: текстура без glGenerateMipmap
float uvScale     = 1.0f;    // [ ]  - задание 5
int   vertexUvIdx = 0;       // V    - 1.0 / 2.0 / 0.1 в vertices[]
int   texIdx      = 0;       // C    - Қ1: container.png <-> своя картинка
bool  tintOn      = false;   // T    - Қ2
bool  flowOn      = false;   // F    - Қ3
bool  aniso       = false;   // X    - Қ4
bool  floorOn     = true;    // G    - большой плоский куб-пол
bool  anisoSupported = false;
float maxAniso    = 1.0f;

bool paramsDirty = true;     // параметры текстуры поменялись -> заново glTexParameteri
bool vboDirty    = true;     // множитель UV в vertices[] поменялся -> заново glBufferSubData
bool shotRequested = false;
bool titleDirty = false;     // заголовок окна нужно обновить (см. главный цикл)

void updateFront() {
    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(dir);
}

void resetCamera() {
    camPos = glm::vec3(0.0f, 0.0f, 3.0f);
    yaw = -90.0f; pitch = 0.0f; fov = 45.0f;
    updateFront();
}

void resetTextureState() {
    wrapMode = 0; magNearest = false; useMipmap = true; badMag = false; noMipTex = false;
    uvScale = 1.0f; vertexUvIdx = 0; texIdx = 0;
    tintOn = false; flowOn = false; aniso = false; floorOn = true;
    paramsDirty = vboDirty = true;
}

void updateTitle(GLFWwindow* window) {
    char title[256];
    std::snprintf(title, sizeof(title),
        "Week 11 | wrap %s | MAG %s | MIN %s | uvScale %.2f | vertex UV x%.1f%s%s%s%s",
        WRAP_NAMES[wrapMode],
        badMag ? "LINEAR_MIPMAP_LINEAR (!)" : (magNearest ? "NEAREST" : "LINEAR"),
        useMipmap ? "LINEAR_MIPMAP_LINEAR" : "LINEAR",
        uvScale, VERTEX_UV[vertexUvIdx],
        tintOn ? " | tint" : "", flowOn ? " | flow" : "", aniso ? " | aniso" : "",
        noMipTex ? " | NO MIPMAPS (!)" : "");
    glfwSetWindowTitle(window, title);
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gWidth  = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

// одиночные нажатия: переключатели
void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (key == GLFW_KEY_MINUS) sensitivity = std::fmax(sensitivity * 0.8f, 0.005f);
    if (key == GLFW_KEY_EQUAL) sensitivity = std::fmin(sensitivity * 1.25f, 0.5f);
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4:
            wrapMode = key - GLFW_KEY_1;           paramsDirty = true; break;
        case GLFW_KEY_N: magNearest = !magNearest; paramsDirty = true; break;
        case GLFW_KEY_M: useMipmap  = !useMipmap;  paramsDirty = true; break;
        case GLFW_KEY_B: badMag     = !badMag;     paramsDirty = true; break;
        case GLFW_KEY_K: noMipTex   = !noMipTex;   paramsDirty = true; break;
        case GLFW_KEY_X: aniso      = !aniso;      paramsDirty = true; break;
        case GLFW_KEY_C: texIdx     = 1 - texIdx;  paramsDirty = true; break;
        case GLFW_KEY_V: vertexUvIdx = (vertexUvIdx + 1) % 3; vboDirty = true; break;
        case GLFW_KEY_T: tintOn  = !tintOn;  break;
        case GLFW_KEY_F: flowOn  = !flowOn;  break;
        case GLFW_KEY_G: floorOn = !floorOn; break;
        case GLFW_KEY_P: shotRequested = true; break;
        case GLFW_KEY_R: resetCamera(); resetTextureState(); break;
        }
    }
    titleDirty = true;   // сам заголовок - в главном цикле, не чаще 4 раз в секунду
}

void cursor_enter_callback(GLFWwindow*, int) { firstMouse = true; }

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    yaw   += xoffset * sensitivity;
    pitch += yoffset * sensitivity;
    updateFront();

    if (wslMode) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        if (xpos <= 1.0 || xpos >= w - 2.0 || ypos <= 1.0 || ypos >= h - 2.0) {
            glfwSetCursorPos(window, w / 2.0, h / 2.0);
            firstMouse = true;
        }
    }
}

void scroll_callback(GLFWwindow*, double, double yoffset) {
    fov -= (float)yoffset;
    if (fov <  1.0f) fov =  1.0f;
    if (fov > 45.0f) fov = 45.0f;
    titleDirty = true;
}

// непрерывные действия - каждый кадр, поэтому умножаем на dt
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float old = uvScale;
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) uvScale += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET)  == GLFW_PRESS) uvScale -= 2.0f * dt;
    if (uvScale < 0.5f) uvScale = 0.5f;
    if (uvScale > 8.0f) uvScale = 8.0f;
    if (uvScale != old) titleDirty = true;

    float speed = 2.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= glm::normalize(glm::cross(camFront, camUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += glm::normalize(glm::cross(camFront, camUp)) * speed;
}

unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    int ok;
    char log[1024];

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(fs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 1024, nullptr, log); std::cerr << log << "\n"; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vTexCoord = aTexCoord;   // просто передаём дальше, интерполируется как раньше vColor
}
)";

const char* fragmentSrc = R"(
#version 330 core
in  vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D ourTexture;   // не сама картинка, а номер текстурного блока
uniform float uvScale;          // задание 5
uniform vec3  tint;             // Қ2 (1,1,1 - без изменений)
uniform float flowTime;         // Қ3 (0 - без сдвига)

void main() {
    vec2 uv = vTexCoord * uvScale + vec2(0.0, flowTime * 0.1);
    FragColor = texture(ourTexture, uv) * vec4(tint, 1.0);
}
)";

// 36 вершин (x y z  u v). Позиции и порядок обхода (CCW снаружи) - без изменений из 10-й недели,
// три числа цвета заменены двумя UV. UV подобраны так, чтобы снаружи картинка на каждой грани
// не была зеркальной: u растёт вправо, v - вверх.
const float cubeVertices[] = {
    // перед (z = +0.5)
    -0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
    // зад (z = -0.5)
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
    // лево (x = -0.5)
    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
    // право (x = +0.5)
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
    // низ (y = -0.5)
    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
    // верх (y = +0.5)
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
};

// 5 кубов из 9-й недели + задание 3: шесть кубов около z = -40
const glm::vec3 cubePositions[] = {
    { 0.0f,  0.0f,   0.0f},
    { 2.0f,  0.5f,  -2.0f},
    {-1.8f, -0.3f,  -1.0f},
    { 1.2f, -0.8f,  -3.5f},
    {-2.2f,  1.0f,  -4.0f},
    {-4.0f,  0.5f, -38.0f},
    {-1.5f,  1.5f, -41.0f},
    { 1.0f, -0.5f, -40.0f},
    { 3.5f,  1.0f, -39.0f},
    { 0.0f,  3.0f, -42.0f},
    { 2.0f,  2.5f, -37.0f},
};
const int CUBE_N = sizeof(cubePositions) / sizeof(cubePositions[0]);

const float FLOOR_Y    = -1.5f;
const float FLOOR_SIZE = 80.0f;   // плоский куб 80 x 0.1 x 80
const int   RING_N     = 12;      // кольцо кубов вокруг (из 10-й недели)
const float RING_R     = 10.0f;

// Схема та же, что у VBO из 2-й недели: Gen -> Bind -> данные
unsigned int loadTexture(const char* path, bool mipmaps = true) {
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);   // у картинки y=0 сверху, у UV v=0 снизу

    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (data) {
        GLenum fmt = (n == 4) ? GL_RGBA : GL_RGB;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // строка RGB-картинки любой ширины (не кратной 4)
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        if (mipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);       // строго ПОСЛЕ glTexImage2D
        std::cout << path << "  " << w << "x" << h << "  арна: " << n << std::endl;
    } else {
        std::cerr << "Сурет жүктелмеді: " << path << std::endl;
    }
    stbi_image_free(data);   // данные уже в памяти GPU
    return id;
}

// применить текущие переключатели к текстуре (glTexParameteri идут в текстуру, привязанную сейчас)
void applyTexParams(unsigned int tex) {
    glBindTexture(GL_TEXTURE_2D, tex);
    while (glGetError() != GL_NO_ERROR) {}

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WRAP_MODES[wrapMode]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WRAP_MODES[wrapMode]);
    float border[] = {1.0f, 0.0f, 0.0f, 1.0f};   // красный - для CLAMP_TO_BORDER
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    if (badMag) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);   // нарочно неверно
        GLenum err = glGetError();
        GLint actual = 0;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &actual);
        std::printf("MAG = GL_LINEAR_MIPMAP_LINEAR -> glGetError() = 0x%04X%s, MAG фактически = 0x%04X\n",
                    err, err == GL_INVALID_ENUM ? " (GL_INVALID_ENUM)" : "", actual);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magNearest ? GL_NEAREST : GL_LINEAR);
    }

    if (anisoSupported)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, aniso ? maxAniso : 1.0f);
}

void uploadVertices(unsigned int vbo) {
    float scaled[sizeof(cubeVertices) / sizeof(float)];
    std::memcpy(scaled, cubeVertices, sizeof(cubeVertices));
    for (int v = 0; v < 36; v++) {
        scaled[v * 5 + 3] *= VERTEX_UV[vertexUvIdx];
        scaled[v * 5 + 4] *= VERTEX_UV[vertexUvIdx];
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(scaled), scaled);
}

void saveScreenshot(const std::string& path) {
    std::vector<unsigned char> px((size_t)gWidth * gHeight * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, gWidth, gHeight, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    stbi_flip_vertically_on_write(1);   // у OpenGL первая строка - нижняя
    if (stbi_write_png(path.c_str(), gWidth, gHeight, 3, px.data(), gWidth * 3))
        std::cout << "скриншот: " << path << std::endl;
    else
        std::cerr << "не удалось сохранить " << path << std::endl;
}

// --- режим --shots: все скриншоты для сдачи с одинаковой камерой и без анимации ---
struct Shot { const char* file; std::function<void()> setup; };

void setCamera(glm::vec3 pos, float y, float p, float f) {
    camPos = pos; yaw = y; pitch = p; fov = f;
    updateFront();
}

std::vector<Shot> makeShots() {
    // крупный план куба 0 без пола (иначе при CLAMP пол занимает пол-экрана)
    auto closeUp = [] { setCamera({0.0f, 0.2f, 2.4f}, -90.0f, -5.0f, 45.0f); vertexUvIdx = 1; floorOn = false; };
    // камера за 30 единиц от кубов у z = -40: каждый ~20 пикселей (как в вопросе к заданию 3)
    auto farView = [] { setCamera({0.0f, 0.5f, -10.0f}, -90.0f, -3.0f, 45.0f); };
    auto floorView = [] { setCamera({0.0f, -1.2f, 6.0f}, -90.0f, -4.0f, 45.0f); };
    return {
        {"01-basic.png",        [] { setCamera({0.0f, 0.3f, 3.2f}, -90.0f, -5.0f, 45.0f); }},
        {"02-repeat.png",       [=] { closeUp(); wrapMode = 0; }},
        {"03-mirrored.png",     [=] { closeUp(); wrapMode = 1; }},
        {"04-clamp-edge.png",   [=] { closeUp(); wrapMode = 2; }},
        {"05-clamp-border.png", [=] { closeUp(); wrapMode = 3; }},
        {"06-nearest.png",      [] { setCamera({0.0f, 0.0f, 3.0f}, -90.0f, 0.0f, 8.0f); vertexUvIdx = 2; magNearest = true; }},
        {"07-linear.png",       [] { setCamera({0.0f, 0.0f, 3.0f}, -90.0f, 0.0f, 8.0f); vertexUvIdx = 2; magNearest = false; }},
        {"08-no-mipmap.png",    [=] { farView(); useMipmap = false; }},
        {"09-mipmap.png",       [=] { farView(); useMipmap = true; }},
        {"10-black-cube.png",   [] { setCamera({0.0f, 0.3f, 3.2f}, -90.0f, -5.0f, 45.0f); noMipTex = true; }},
        {"10b-bad-mag.png",     [] { setCamera({0.0f, 0.3f, 3.2f}, -90.0f, -5.0f, 45.0f); badMag = true; }},
        {"11-uvscale-4.png",    [=] { closeUp(); vertexUvIdx = 0; uvScale = 4.0f; }},
        {"12-own-image.png",    [=] { closeUp(); vertexUvIdx = 0; texIdx = 1; }},
        {"13-tint.png",         [] { setCamera({0.0f, 0.3f, 3.2f}, -90.0f, -5.0f, 45.0f); tintOn = true; }},
        {"14-aniso-off.png",    [=] { floorView(); aniso = false; }},
        {"15-aniso-on.png",     [=] { floorView(); aniso = true; }},
    };
}

int main(int argc, char** argv) {
    const bool shotMode = argc > 1 && std::strcmp(argv[1], "--shots") == 0;

    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    if (shotMode) glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 11 - Textures", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    if (!shotMode) {
        glfwSetInputMode(window, GLFW_CURSOR, wslMode ? GLFW_CURSOR_CAPTURED : GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetCursorEnterCallback(window, cursor_enter_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetKeyCallback(window, key_callback);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD load failed\n";
        glfwTerminate();
        return -1;
    }
    updateTitle(window);

    glfwGetFramebufferSize(window, &gWidth, &gHeight);
    glViewport(0, 0, gWidth, gHeight);

    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    // позиция: location 0, 3 float; stride 5
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // UV: location 1 - теперь 2 компонента, stride 5, смещение те же 3 float
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Қ4: анизотропная фильтрация - расширение, проверяем, есть ли оно
    anisoSupported = GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic;
    if (anisoSupported) glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    std::cout << "anisotropy: " << (anisoSupported ? "есть, max " + std::to_string((int)maxAniso)
                                                   : std::string("нет")) << std::endl;

    std::string dir = ASSET_DIR;
    unsigned int textures[3] = {
        loadTexture((dir + "/container.png").c_str()),
        loadTexture((dir + "/awesomeface.png").c_str()),          // Қ1: своя картинка, 4 канала
        loadTexture((dir + "/container.png").c_str(), false),     // задание 4: БЕЗ glGenerateMipmap
    };
    auto currentTex = [&] { return noMipTex ? textures[2] : textures[texIdx]; };

    // location'ы - один раз, до цикла
    int locModel   = glGetUniformLocation(shader, "uModel");
    int locView    = glGetUniformLocation(shader, "uView");
    int locProj    = glGetUniformLocation(shader, "uProjection");
    int locUvScale = glGetUniformLocation(shader, "uvScale");
    int locTint    = glGetUniformLocation(shader, "tint");
    int locFlow    = glGetUniformLocation(shader, "flowTime");

    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "ourTexture"), 0);   // сэмплер -> блок 0

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    if (!shotMode)
        std::cout << "1-4 wrap | N MAG nearest/linear | M mipmap | B MAG=mipmap | K без glGenerateMipmap | [ ] uvScale\n"
                     "V UV в vertices[] x1/x2/x0.1 | C картинка | T tint | F flow | X aniso | G пол\n"
                     "P скриншот | R сброс | мышь, колесо, WASD - камера | Esc выход" << std::endl;

    std::vector<Shot> shots = makeShots();
    size_t shotIdx = 0;
    int shotFrame = 0;
    int manualShot = 0;

    float lastFrame = (float)glfwGetTime();
    float lastTitleTime = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (shotMode) {
            if (shotFrame == 0) {   // новый кадр сценария: всё сбросить и настроить
                resetCamera();
                resetTextureState();
                shots[shotIdx].setup();
                updateTitle(window);
            }
        } else {
            processInput(window, dt);
        }

        // Заголовок окна - не чаще 4 раз в секунду. Если менять его каждый кадр (пока зажата ] или [),
        // оконная система (особенно WSLg) захлёбывается сообщениями и может подвесить весь компьютер.
        if (titleDirty && currentFrame - lastTitleTime > 0.25f) {
            updateTitle(window);
            titleDirty = false;
            lastTitleTime = currentFrame;
        }

        if (paramsDirty) {
            applyTexParams(currentTex());
            paramsDirty = false;
        }
        if (vboDirty) {
            uploadVertices(vbo);
            vboDirty = false;
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // в режиме скриншотов время "заморожено" - картинка воспроизводима
        float t     = shotMode ? 2.0f : currentFrame;
        float angle = shotMode ? 1.0f : currentFrame;

        glUseProgram(shader);
        glm::vec3 tint = tintOn ? 0.5f + 0.5f * glm::sin(t * glm::vec3(1.0f, 1.3f, 1.7f)) : glm::vec3(1.0f);
        glUniform3fv(locTint, 1, glm::value_ptr(tint));
        glUniform1f(locFlow, flowOn ? t : 0.0f);
        glUniform1f(locUvScale, uvScale);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex());

        float aspect = (float)gWidth / (float)gHeight;
        glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao);
        for (int i = 0; i < CUBE_N; i++) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            model = glm::rotate(model, angle * (0.5f + (i % 5) * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        for (int i = 0; i < RING_N; i++) {
            float a = glm::radians(360.0f / RING_N * i);
            glm::vec3 p(RING_R * cos(a), FLOOR_Y + 0.5f + (i % 3), RING_R * sin(a));
            glm::mat4 model = glm::translate(glm::mat4(1.0f), p);
            model = glm::rotate(model, angle * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // пол: тот же куб, сплющенный по Y; картинка повторяется 40 раз (GL_REPEAT)
        if (floorOn) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, FLOOR_Y - 0.05f, 0.0f));
            model = glm::scale(model, glm::vec3(FLOOR_SIZE, 0.1f, FLOOR_SIZE));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1f(locUvScale, uvScale * FLOOR_SIZE / 2.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        if (shotMode) {
            if (++shotFrame >= 3) {   // пара кадров, чтобы всё точно применилось
                saveScreenshot(std::string(SHOT_DIR) + "/" + shots[shotIdx].file);
                shotFrame = 0;
                if (++shotIdx == shots.size()) glfwSetWindowShouldClose(window, true);
            }
        } else if (shotRequested) {
            char name[64];
            std::snprintf(name, sizeof(name), "/shot-%02d.png", ++manualShot);
            saveScreenshot(std::string(SHOT_DIR) + name);
            shotRequested = false;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(3, textures);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
