#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

const int WINDOW_W = 500, WINDOW_H = 700;
const float BLOCK_W = 120.f / WINDOW_W * 2.f;
const float BLOCK_H = 40.f / WINDOW_H * 2.f;
const float START_SPEED = 0.8f, SPEED_GROWTH = 1.05f; // +5% за блок
const float FALL_SPEED = 1.5f;
const float TOP_MARGIN = 0.05f;          // отступ движущегося блока от верха экрана
const float TOWER_LIMIT = -0.1f;         // выше этого уровня верх башни на экране не поднимается
const float CAMERA_SMOOTHING = 5.f;
const float MIN_OVERLAP = 0.37f;         // меньше этой доли опоры - блок обрушивается
const char* HIGHSCORE_FILE = "highscore.txt";

enum State { READY, MOVING, FALLING, PAUSED, GAME_OVER };

struct Block {
    float x, y;       // центр; y - мировая (экранная = y - cameraY)
    glm::vec3 color;
    int row;          // этаж: 0 - база, записывается при установке
};

State state = READY, stateBeforePause = MOVING;
std::vector<Block> tower;   // tower[0] - база
Block cur{};                // движущийся / падающий блок
int score = 0, bestScore = 0;
bool newRecord = false, titleDirty = true;
float speed = START_SPEED, dir = 1.f, cameraY = 0.f;

// ---------- Шейдер и рисование ----------
const char* VERT_SRC = R"(#version 330 core
layout (location = 0) in vec2 aPos;
uniform vec2 offset; // центр прямоугольника
uniform vec2 scale;  // ширина и высота
void main() {
    gl_Position = vec4(aPos * scale + offset, 0.0, 1.0);
}
)";

const char* FRAG_SRC = R"(#version 330 core
out vec4 FragColor;
uniform vec4 color;
void main() {
    FragColor = color;
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        fprintf(stderr, "Ошибка шейдера: %s\n", log);
    }
    return s;
}

GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    int ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, nullptr, log);
        fprintf(stderr, "Ошибка линковки: %s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

GLint locOffset = -1, locScale = -1, locColor = -1;

// Единичный квадрат (VAO/EBO), растянутый и сдвинутый uniform-ами
void drawRect(float cx, float cy, float w, float h, glm::vec4 c) {
    glUniform2f(locOffset, cx, cy);
    glUniform2f(locScale, w, h);
    glUniform4f(locColor, c.r, c.g, c.b, c.a);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// Шрифт 5x7 только для "GAME OVER": 7 строк по 5 клеток, '#' - закрашено
const char* glyph(char c) {
    switch (c) {
    case 'G': return ".###.#...##....#.####...##...#.###.";
    case 'A': return ".###.#...##...#######...##...##...#";
    case 'M': return "#...###.###.#.##.#.##...##...##...#";
    case 'E': return "######....#....####.#....#....#####";
    case 'O': return ".###.#...##...##...##...##...#.###.";
    case 'V': return "#...##...##...##...##...#.#.#...#..";
    case 'R': return "####.#...##...#####.#.#..#..#.#...#";
    default:  return nullptr;
    }
}

// Строка по центру (cx, cy); cell - ширина клетки
void drawText(const char* text, float cx, float cy, float cell, glm::vec4 c) {
    float cellH = cell * WINDOW_W / WINDOW_H; // клетки квадратные в пикселях
    int len = (int)strlen(text);
    float advance = 6 * cell;                 // 5 клеток буквы + 1 промежуток
    float x0 = cx - (len * advance - cell) / 2, y0 = cy + 3.5f * cellH;
    for (int i = 0; i < len; i++) {
        const char* g = glyph(text[i]);
        if (!g) continue;
        for (int row = 0; row < 7; row++)
            for (int col = 0; col < 5; col++)
                if (g[row * 5 + col] == '#')
                    drawRect(x0 + i * advance + (col + 0.5f) * cell, y0 - (row + 0.5f) * cellH, cell, cellH, c);
    }
}

void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (const Block& b : tower) drawRect(b.x, b.y - cameraY, BLOCK_W, BLOCK_H, glm::vec4(b.color, 1.f));
    if (state != READY) drawRect(cur.x, cur.y - cameraY, BLOCK_W, BLOCK_H, glm::vec4(cur.color, 1.f));

    // Затемнение сцены: для альфы включён блендинг
    if (state == PAUSED || state == GAME_OVER) drawRect(0, 0, 2, 2, glm::vec4(0, 0, 0, 0.6f));
    if (state == GAME_OVER) {
        drawText("GAME OVER", 0.012f, -0.016f, 0.026f, glm::vec4(0, 0, 0, 1));            // тень
        drawText("GAME OVER", 0.f, 0.f, 0.026f, glm::vec4(1.f, 0.15f, 0.15f, 1.f));
    }
}

// ---------- Логика ----------
glm::vec3 colorForIndex(int i) {
    float t = i * 0.5f;
    // sin даёт [-1;1], приводим к [0;1]
    return glm::vec3(0.5f + 0.5f * sinf(t), 0.5f + 0.5f * sinf(t + 2.f), 0.5f + 0.5f * sinf(t + 4.f));
}

// Верх башни: максимум, а не tower.back() - блок может сесть на нижний выступ
float towerTop() {
    float top = -1.f;
    for (const Block& b : tower) top = std::max(top, b.y + BLOCK_H / 2);
    return top;
}

// Удаляет блоки с опорой меньше MIN_OVERLAP; после каждого удаления проверка
// идёт заново (лежавшие сверху остались без опоры). Возвращает число упавших.
int collapseUnstable() {
    int removed = 0;
    for (size_t i = 1; i < tower.size();) {
        Block b = tower[i];
        float overlap = 0;
        for (const Block& s : tower) {
            if (s.row != b.row - 1) continue;
            // общая часть двух отрезков: min(правые края) - max(левые края)
            float right = std::min(b.x + BLOCK_W / 2, s.x + BLOCK_W / 2);
            float left = std::max(b.x - BLOCK_W / 2, s.x - BLOCK_W / 2);
            overlap += std::max(0.f, right - left);
        }
        if (overlap / BLOCK_W < MIN_OVERLAP) {
            tower.erase(tower.begin() + i);
            removed++;
            i = 1;
        } else {
            i++;
        }
    }
    return removed;
}

void spawnBlock() {
    cur.color = colorForIndex((int)tower.size());
    cur.y = cameraY + 1.f - BLOCK_H / 2 - TOP_MARGIN;
    state = MOVING;
}

void resetGame() {
    tower = {{0.f, -1.f + BLOCK_H / 2, colorForIndex(0), 0}};
    score = 0;
    newRecord = false;
    speed = START_SPEED;
    dir = 1.f;
    cameraY = 0.f;
    cur.x = 0.f;
    spawnBlock();
    state = READY;
    titleDirty = true;
}

void endGame() {
    state = GAME_OVER;
    titleDirty = true;
    if (score > bestScore) {
        bestScore = score;
        newRecord = true;
        std::ofstream(HIGHSCORE_FILE) << bestScore << "\n";
    }
}

void dropBlock() {
    if (state == READY) { state = MOVING; titleDirty = true; }
    else if (state == MOVING) state = FALLING;
}

void togglePause() {
    if (state == MOVING || state == FALLING) { stateBeforePause = state; state = PAUSED; }
    else if (state == PAUSED) state = stateBeforePause;
    else return;
    titleDirty = true;
}

void update(float dt) {
    if (state == READY || state == PAUSED) return;

    // Камера плавно догоняет цель; dt - чтобы скорость не зависела от FPS. Ниже базы не опускается
    float target = std::max(0.f, towerTop() - TOWER_LIMIT);
    cameraY += (target - cameraY) * std::min(1.f, CAMERA_SMOOTHING * dt);

    if (state == MOVING) {
        cur.x += dir * speed * dt;
        if (cur.x + BLOCK_W / 2 > 1.f)  { cur.x = 1.f - BLOCK_W / 2;  dir = -1.f; }
        if (cur.x - BLOCK_W / 2 < -1.f) { cur.x = -1.f + BLOCK_W / 2; dir = 1.f; }
        cur.y = cameraY + 1.f - BLOCK_H / 2 - TOP_MARGIN; // блок прикреплён к верху экрана
    } else if (state == FALLING) {
        cur.y -= FALL_SPEED * dt;

        // Опора - самый высокий блок башни, пересекающийся с падающим по x
        const Block* support = nullptr;
        for (const Block& s : tower)
            if (fabsf(cur.x - s.x) < BLOCK_W && (!support || s.y > support->y)) support = &s;

        float bottom = cur.y - BLOCK_H / 2;
        if (support && bottom <= support->y + BLOCK_H / 2) {
            cur.y = support->y + BLOCK_H; // ставим ровно на опору, без проваливания за кадр
            cur.row = support->row + 1;
            tower.push_back(cur);
            score++;
            speed *= SPEED_GROWTH;
            titleDirty = true;

            int removed = collapseUnstable();
            if (removed > 0) {
                score = std::max(0, score - removed);
                endGame();
                cur.y = cameraY - 2.f; // блок уже среди упавших - прячем под экран
            } else {
                spawnBlock();
            }
        } else if (bottom + BLOCK_H < cameraY - 1.f) {
            endGame(); // промах: блок ушёл ниже экрана
        }
    } else if (cur.y + BLOCK_H / 2 > cameraY - 1.f) { // GAME_OVER: блок долетает за экран
        cur.y -= FALL_SPEED * dt;
    }
}

void updateTitle(GLFWwindow* win) {
    std::string title = "Tower Builder", s = std::to_string(score), best = std::to_string(bestScore);
    if (state == READY) title += " — Best: " + best + " — Press SPACE to start — ESC to quit";
    else if (state == PAUSED) title += " — PAUSED — Press P to resume";
    else if (state == GAME_OVER)
        title += " — GAME OVER — Score: " + s + " — Best: " + best + (newRecord ? " (NEW RECORD!)" : "") +
                 " — R to restart — ESC to quit";
    else title += " — Score: " + s + " — Best: " + best;
    glfwSetWindowTitle(win, title.c_str());
    titleDirty = false;
}

// true только в кадр, когда клавиша перешла из отпущенной в нажатую
bool justPressed(GLFWwindow* win, int key, bool& wasDown) {
    bool down = glfwGetKey(win, key) == GLFW_PRESS, pressed = down && !wasDown;
    wasDown = down;
    return pressed;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* win = glfwCreateWindow(WINDOW_W, WINDOW_H, "Tower Builder", nullptr, nullptr);
    if (!win) { fprintf(stderr, "Не удалось создать окно\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    if (!gladLoadGL(glfwGetProcAddress)) { fprintf(stderr, "GLAD не загрузился\n"); return -1; }
    glViewport(0, 0, WINDOW_W, WINDOW_H);

    GLuint program = createProgram();
    locOffset = glGetUniformLocation(program, "offset");
    locScale  = glGetUniformLocation(program, "scale");
    locColor  = glGetUniformLocation(program, "color");

    // Единичный квадрат: 4 вершины + 6 индексов (EBO)
    float vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
    unsigned int indices[] = {0, 1, 2, 2, 3, 0};
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); // привязка EBO запоминается в VAO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.05f, 0.08f, 0.25f, 1.f);

    int value = 0;
    std::ifstream in(HIGHSCORE_FILE);
    if (in >> value && value > 0) bestScore = value;
    resetGame();

    bool spaceDown = false, rDown = false, pDown = false;
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(win) && glfwGetKey(win, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        double now = glfwGetTime();
        float dt = std::min((float)(now - last), 0.05f); // после лага блок не должен "прыгать"
        last = now;

        glfwPollEvents();
        if (justPressed(win, GLFW_KEY_SPACE, spaceDown)) dropBlock();
        if (justPressed(win, GLFW_KEY_R, rDown) && state == GAME_OVER) resetGame();
        if (justPressed(win, GLFW_KEY_P, pDown)) togglePause();

        update(dt);
        if (titleDirty) updateTitle(win);

        glUseProgram(program);
        glBindVertexArray(vao);
        draw();
        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}
