#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

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

// --- 12-я неделя: две текстуры ---
float blend = 0.5f;          // доля второй картинки, 0..1

// режим смешивания = значение uniform int mode во фрагментном шейдере (клавиши 1..8)
enum Mode { MIX, MULTIPLY, ADD, ADD_CLAMP, DECAL, GRADIENT, SMOOTH, THREE, MODE_N };
const char* MODE_NAMES[MODE_N] = {
    "mix(a,b,blend)", "a*b", "a+b", "clamp(a+b)", "decal: mix(base,decal,decal.a)",
    "mix(a,b,uv.y)", "smoothstep(0.3,0.7,uv.y)", "3 textures (K1)"};

int  mode        = MIX;
bool brokenUnits = false;    // U - задание 1: оба glActiveTexture "закомментированы"
bool wrongId     = false;    // I - задание 2: в glUniform1i ID текстуры вместо номера блока
bool autoBlend   = false;    // B - Қ2: blend = 0.5 + 0.5 sin(t)
bool perFace     = false;    // F - Қ3: свой blend на каждой грани
bool rotateDecal = false;    // L - Қ4: логотип вращается

bool unitsDirty    = true;   // режим поменялся -> сбросить блоки и заново задать номера сэмплеров
bool titleDirty    = false;

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

void resetBlendState() {
    blend = 0.5f; mode = MIX;
    brokenUnits = wrongId = autoBlend = perFace = rotateDecal = false;
    unitsDirty = true;
}

void updateTitle(GLFWwindow* window) {
    char title[256];
    std::snprintf(title, sizeof(title), "Week 12 | %s | blend %.2f%s%s%s%s%s",
        MODE_NAMES[mode], blend,
        brokenUnits ? " | NO glActiveTexture (!)" : "", wrongId ? " | ID instead of unit (!)" : "",
        autoBlend ? " | auto" : "", perFace ? " | per face" : "", rotateDecal ? " | rotate logo" : "");
    glfwSetWindowTitle(window, title);
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gWidth  = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

// одиночные нажатия: переключатели
void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) { mode = key - GLFW_KEY_1; unitsDirty = true; }
    switch (key) {
    case GLFW_KEY_U: brokenUnits = !brokenUnits; unitsDirty = true; break;
    case GLFW_KEY_I: wrongId     = !wrongId;     unitsDirty = true; break;
    case GLFW_KEY_F: perFace     = !perFace;     break;
    case GLFW_KEY_L: rotateDecal = !rotateDecal; break;
    case GLFW_KEY_R: resetCamera(); resetBlendState(); break;
    }
    titleDirty = true;
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
}

// непрерывные действия - каждый кадр, поэтому умножаем на dt
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Қ2: B - переключатель по фронту нажатия (храним прошлое состояние клавиши, как в 10-й неделе)
    static bool bWasDown = false;
    bool bDown = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
    if (bDown && !bWasDown) { autoBlend = !autoBlend; titleDirty = true; }
    bWasDown = bDown;

    if (autoBlend) {
        blend = 0.5f + 0.5f * sin((float)glfwGetTime());
        titleDirty = true;
    } else {
        float old = blend;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            blend += 0.5f * dt;               // 0.5 единицы в секунду - не зависит от FPS
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            blend -= 0.5f * dt;
        if (blend > 1.0f) blend = 1.0f;       // иначе mix уходит в экстраполяцию
        if (blend < 0.0f) blend = 0.0f;
        if (blend != old) titleDirty = true;
    }

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

// вершинный шейдер - без изменений с 11-й недели
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
    vTexCoord = aTexCoord;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in  vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D texture1;     // номер блока 0 (не ID текстуры!)
uniform sampler2D texture2;     // номер блока 1
uniform sampler2D texture3;     // номер блока 2 - только для Қ1
uniform float blend;
uniform int   mode;             // 0..7 - клавиши 1..8
uniform float decalAngle;       // Қ4 (0 - без поворота)

// Қ4: поворот UV логотипа вокруг центра (0.5, 0.5)
vec2 decalUV() {
    vec2 c = vTexCoord - 0.5;
    float s = sin(decalAngle), co = cos(decalAngle);
    return vec2(c.x*co - c.y*s, c.x*s + c.y*co) + 0.5;
}

void main() {
    vec4 a = texture(texture1, vTexCoord);
    vec4 b = texture(texture2, vTexCoord);

    if (mode == 0) {                                   // база: линейно
        FragColor = mix(a, b, blend);
    } else if (mode == 1) {                            // задание 3.2: умножение
        FragColor = a * b;
    } else if (mode == 2) {                            // задание 3.3: сложение
        FragColor = a + b;
    } else if (mode == 3) {                            // задание 3.4: сложение с ограничением
        FragColor = clamp(a + b, 0.0, 1.0);
    } else if (mode == 4) {                            // задание 4: логотип по альфе
        vec4 base  = a;
        vec4 decal = texture(texture2, decalUV());
        FragColor = mix(base, decal, decal.a);
    } else if (mode == 5) {                            // задание 5: коэффициент из UV
        FragColor = mix(a, b, vTexCoord.y);
    } else if (mode == 6) {                            // задание 5: резче
        float t = smoothstep(0.3, 0.7, vTexCoord.y);
        FragColor = mix(a, b, t);
    } else {                                           // Қ1: три текстуры
        vec4 surface = mix(a, b, blend);
        vec4 decal   = texture(texture3, decalUV());
        FragColor = mix(surface, decal, decal.a);
    }
}
)";

// 36 вершин (x y z  u v) - без изменений с 11-й недели. Каждая грань - 6 вершин подряд (нужно для Қ3).
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

const glm::vec3 cubePositions[] = {
    { 0.0f,  0.0f,  0.0f},
    { 2.0f,  0.5f, -2.0f},
    {-1.8f, -0.3f, -1.0f},
    { 1.2f, -0.8f, -3.5f},
    {-2.2f,  1.0f, -4.0f},
};
const int CUBE_N = sizeof(cubePositions) / sizeof(cubePositions[0]);

// без изменений с 11-й недели
unsigned int loadTexture(const char* path) {
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
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << path << "  " << w << "x" << h << "  арна: " << n << std::endl;
    } else {
        std::cerr << "Сурет жүктелмеді: " << path << std::endl;
    }
    stbi_image_free(data);
    return id;
}

int main() {
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

    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 12 - Multiple textures", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    glfwSetInputMode(window, GLFW_CURSOR, wslMode ? GLFW_CURSOR_CAPTURED : GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetCursorEnterCallback(window, cursor_enter_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int units;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
    std::cout << "GL_MAX_TEXTURE_IMAGE_UNITS = " << units << std::endl;

    // шаг 1: вторая (и для задания 4 / Қ1 - третья) текстура тем же loadTexture
    std::string p1 = std::string(ASSET_DIR) + "/container.png";
    std::string p2 = std::string(ASSET_DIR) + "/pattern.png";
    std::string p3 = std::string(ASSET_DIR) + "/logo.png";
    unsigned int tex1 = loadTexture(p1.c_str());
    unsigned int tex2 = loadTexture(p2.c_str());
    unsigned int tex3 = loadTexture(p3.c_str());
    std::cout << "tex1 = " << tex1 << "   tex2 = " << tex2 << "   tex3 = " << tex3 << std::endl;

    // Қ4: при повороте UV выходит за [0,1] - там должна быть прозрачность, а не повтор логотипа
    glBindTexture(GL_TEXTURE_2D, tex3);
    float transparent[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, transparent);

    // location'ы - один раз, до цикла
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView  = glGetUniformLocation(shader, "uView");
    int locProj  = glGetUniformLocation(shader, "uProjection");
    int locBlend = glGetUniformLocation(shader, "blend");
    int locMode  = glGetUniformLocation(shader, "mode");
    int locAngle = glGetUniformLocation(shader, "decalAngle");

    // шаг 3: номера блоков (а не ID!) - один раз, ПОСЛЕ glUseProgram
    glUseProgram(shader);                                          // сначала!
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shader, "texture2"), 1);
    glUniform1i(glGetUniformLocation(shader, "texture3"), 2);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    std::cout << "1 mix | 2 a*b | 3 a+b | 4 clamp(a+b) | 5 логотип | 6 uv.y | 7 smoothstep | 8 три текстуры\n"
                 "Up/Down blend | U без glActiveTexture | I ID вместо блока | B авто-blend | F по граням\n"
                 "L вращать логотип | R сброс | мышь, колесо, WASD - камера | Esc выход" << std::endl;

    float lastFrame = (float)glfwGetTime();
    float lastTitleTime = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, dt);

        if (titleDirty && currentFrame - lastTitleTime > 0.25f) {
            updateTitle(window);
            titleDirty = false;
            lastTitleTime = currentFrame;
        }

        // Переключили режим -> состояние блоков как в только что запущенной программе:
        // все блоки пусты, активный - GL_TEXTURE0. Иначе в задании 1 в блоке 1 осталась бы
        // текстура с прошлых кадров, и ошибка не была бы видна.
        if (unitsDirty) {
            for (int u = 2; u >= 0; u--) {
                glActiveTexture(GL_TEXTURE0 + u);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            glUseProgram(shader);
            if (wrongId) {
                glUniform1i(glGetUniformLocation(shader, "texture2"), tex2);   // задание 2: нарочно неверно
                std::cout << "texture2 <- tex2 = " << tex2 << " (ID, а не номер блока)" << std::endl;
            } else {
                glUniform1i(glGetUniformLocation(shader, "texture2"), 1);
            }
            unitsDirty = false;
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = currentFrame;

        glUseProgram(shader);

        // шаг 4: привязка каждый кадр. Во втором блоке - pattern, а в задании 4 - logo
        unsigned int second = (mode == DECAL) ? tex3 : tex2;
        if (brokenUnits) {
            // glActiveTexture(GL_TEXTURE0);        // задание 1: закомментировано
            glBindTexture(GL_TEXTURE_2D, tex1);
            // glActiveTexture(GL_TEXTURE1);        // задание 1: закомментировано
            glBindTexture(GL_TEXTURE_2D, second);  // -> туда же, в блок 0, и вытесняет tex1
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, second);
            if (mode == THREE) {                    // Қ1: третий блок
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, tex3);
            }
        }

        // шаг 5: blend меняется каждый кадр -> glUniform1f в цикле
        glUniform1f(locBlend, blend);
        glUniform1i(locMode, mode);
        glUniform1f(locAngle, rotateDecal ? currentFrame : 0.0f);

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
            if (perFace) {
                // Қ3: шесть вызовов по 6 вершин, у каждой грани свой blend: 0, 0.2, ... 1.0
                for (int f = 0; f < 6; ++f) {
                    glUniform1f(locBlend, f / 5.0f);
                    glDrawArrays(GL_TRIANGLES, f * 6, 6);
                }
            } else {
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    unsigned int textures[3] = {tex1, tex2, tex3};
    glDeleteTextures(3, textures);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
