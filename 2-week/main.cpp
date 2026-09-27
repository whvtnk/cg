// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week02" && git tag week02 && git push --tags
//
//  Тег арқылы кез келген аптадағы күйге қайта оралуға болады.
//
//  Қазіргі күйі: 2-АПТА — сетка (VBO + VAO)
// =====================================================================

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;

bool whiteBackground = false;
bool wireframeMode = false;

const int GRID_COLS = 10;
const int GRID_ROWS = 10;

const float cellColor[3] = { 0.25f, 0.55f, 0.85f };

std::vector<float> buildGridVertices(int cols, int rows) {
    std::vector<float> verts;
    verts.reserve((size_t)cols * rows * 3 * 6);

    const float left = -0.9f, right = 0.9f;
    const float bottom = -0.9f, top = 0.9f;

    auto pushVertex = [&verts](float x, float y, const float* color) {
        verts.push_back(x);
        verts.push_back(y);
        verts.push_back(0.0f);
        verts.push_back(color[0]);
        verts.push_back(color[1]);
        verts.push_back(color[2]);
    };

    for (int row = 0; row < rows; ++row) {
        float y0 = bottom + (top - bottom) * (float)row / (float)rows;
        float y1 = bottom + (top - bottom) * (float)(row + 1) / (float)rows;
        for (int col = 0; col < cols; ++col) {
            float x0 = left + (right - left) * (float)col / (float)cols;
            float x1 = left + (right - left) * (float)(col + 1) / (float)cols;

            pushVertex(x1, y1, cellColor);
            pushVertex(x0, y0, cellColor);
            pushVertex(x1, y0, cellColor);
        }
    }
    return verts;
}

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        whiteBackground = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        wireframeMode = true;
    }
}

int main() {

    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Компьютерлік графика",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    std::vector<float> gridVertices = buildGridVertices(GRID_COLS, GRID_ROWS);
    int gridVertexCount = (int)(gridVertices.size() / 6);

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 gridVertices.size() * sizeof(float),
                 gridVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int frameCount = 0;
    double lastFpsTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        processInput(window);

        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            float t = (float)glfwGetTime();
            float r = (std::sin(t * 3.0f) + 1.0f) * 0.5f * 0.3f;
            float g = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
            glClearColor(r, g, 0.35f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);

        glUseProgram(shader);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, gridVertexCount);

        frameCount++;
        double now = glfwGetTime();
        if (now - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount << "\n";
            frameCount = 0;
            lastFpsTime = now;
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


// =====================================================================
//  1-АПТА ТАПСЫРМАЛАРЫ
// =====================================================================
//  1. Терезенің өлшемін 1280x720 ет.
//
//  2. Фон түсінің өзгеру жылдамдығын арттыр.
//     Кеңес: t-ге көбейтілетін сан — жиілік. Соңындағы 0.3f — амплитуда.
//     Екеуін шатастырма.
//
//  3. Пробел басылғанда фон ақ түске айналсын.
//     Кеңес: processInput ішінде тексеріп, жаһандық айнымалыға жаз.
//     Назар: тексеруді glClear-дан БҰРЫН істе.
//
//  4. glfwSwapInterval(0) қой да, консольге FPS шығар.
//     Кеңес: әр кадрда шығарма — секундына бір рет жеткілікті,
//     әйтпесе консоль қатып қалады.
// =====================================================================
//cd ~/dev/cg-course/1-week
//cmake -B build && cmake --build build && ./build/bin/cg