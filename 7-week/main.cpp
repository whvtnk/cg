#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

// ---- Параметры лабораторной (меняются здесь или через -D при сборке) ----
#ifndef FOV_DEG
#define FOV_DEG 45.0f      // Задание 1: 30, 45, 90, 120
#endif
#ifndef NEAR_PLANE
#define NEAR_PLANE 0.1f    // Задание 1: 0.1, 1.0, 2.5, 4.0
#endif
#ifndef FAR_PLANE
#define FAR_PLANE 100.0f   // Задание 1: 100, 5, 2
#endif
#ifndef USE_ORTHO
#define USE_ORTHO 0        // Задание 2: 1 = glm::ortho вместо perspective
#endif
#ifndef THREE_CUBES
#define THREE_CUBES 0      // Задание 3: 1 = три куба
#endif

int gWidth  = 800;
int gHeight = 600;

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gWidth  = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
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
layout (location = 1) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

// 6 граней x 2 треугольника x 3 вершины = 36 вершин, (x y z  r g b)
float cubeVertices[] = {
    // перед (z = +0.5) — красный
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    // зад (z = -0.5) — зелёный
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    // лево (x = -0.5) — синий
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    // право (x = +0.5) — жёлтый
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
    // низ (y = -0.5) — пурпурный
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
    // верх (y = +0.5) — голубой
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
};

// Сохраняет текущий кадр в PPM (используется только с флагом --shot)
void saveScreenshot(const char* path) {
    std::vector<unsigned char> px(gWidth * gHeight * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, gWidth, gHeight, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", gWidth, gHeight);
    for (int y = gHeight - 1; y >= 0; y--) {
        std::fwrite(&px[y * gWidth * 3], 1, gWidth * 3, f);
    }
    std::fclose(f);

    // для проверки «куб пропал»: считаем пиксели, отличные от фона
    long lit = 0;
    for (size_t i = 0; i < px.size(); i += 3) {
        if (px[i] > 40 || px[i + 1] > 40 || px[i + 2] > 40) lit++;
    }
    std::cout << "non-background pixels: " << lit << "\n";
}

int main(int argc, char** argv) {
    const char* shotPath = nullptr;
    for (int i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], "--shot") == 0) shotPath = argv[i + 1];
    }

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

    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 7 - MVP", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD load failed\n";
        glfwTerminate();
        return -1;
    }

    // на HiDPI размер фреймбуфера может отличаться от размера окна
    glfwGetFramebufferSize(window, &gWidth, &gHeight);
    glViewport(0, 0, gWidth, gHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "ESC - exit\n";

    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // location'ы — один раз, до цикла
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView  = glGetUniformLocation(shader, "uView");
    int locProj  = glGetUniformLocation(shader, "uProjection");

#if THREE_CUBES
    const glm::vec3 cubePos[3]   = { {-1.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.5f, 0.0f, 0.0f} };
    const float     cubeSpeed[3] = { 0.5f, 1.0f, 2.0f };
    const int cubeCount = 3;
#else
    const int cubeCount = 1;
#endif

    int frame = 0;
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        // для --shot угол фиксирован, чтобы кадры были воспроизводимы
        float t = shotPath ? 0.6f : (float)glfwGetTime();

#if USE_ORTHO
        float a = (float)gWidth / (float)gHeight;
        glm::mat4 projection = glm::ortho(-a, a, -1.0f, 1.0f, 0.1f, 100.0f);
#else
        glm::mat4 projection = glm::perspective(glm::radians((float)FOV_DEG),
                                                (float)gWidth / (float)gHeight,
                                                (float)NEAR_PLANE, (float)FAR_PLANE);
#endif
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao);
        for (int i = 0; i < cubeCount; i++) {
#if THREE_CUBES
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePos[i]);
            model = glm::rotate(model, t * cubeSpeed[i], glm::vec3(0.0f, 1.0f, 0.0f));
#else
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.0f, 1.0f, 0.0f));
#endif
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        if (shotPath && ++frame == 3) {
            saveScreenshot(shotPath);
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
