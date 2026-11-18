// Генератор картинок для 12-й недели:
//   pattern.png - 512x512, 3 канала: шахматка 8x8 с кругами (бесшовная, 512 / 64 = 8 клеток)
//   logo.png    - 512x512, 4 канала: значок-призма на прозрачном фоне, края мягкие
// Запуск: ./build/gen_assets            (пишет в assets/)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

const int N = 512;

struct V2 { float x, y; };
struct C3 { float r, g, b; };

C3 lerp(C3 a, C3 b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

float distSegment(V2 p, V2 a, V2 b) {
    float vx = b.x - a.x, vy = b.y - a.y;
    float t = clamp01(((p.x - a.x) * vx + (p.y - a.y) * vy) / (vx * vx + vy * vy));
    float dx = p.x - (a.x + vx * t), dy = p.y - (a.y + vy * t);
    return std::sqrt(dx * dx + dy * dy);
}

// знаковое расстояние до выпуклого многоугольника: < 0 внутри, > 0 снаружи
float sdConvex(const std::vector<V2>& poly, V2 p) {
    float d = 1e9f;
    int pos = 0, neg = 0;
    for (size_t i = 0; i < poly.size(); i++) {
        V2 a = poly[i], b = poly[(i + 1) % poly.size()];
        d = std::min(d, distSegment(p, a, b));
        float cr = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
        (cr >= 0 ? pos : neg)++;
    }
    bool inside = pos == 0 || neg == 0;
    return inside ? -d : d;
}

unsigned char u8(float v) { return (unsigned char)std::lround(clamp01(v / 255.0f) * 255.0f); }

void makePattern(const std::string& path) {
    std::vector<unsigned char> img(N * N * 3);
    const C3 cellA = {28, 92, 140}, cellB = {18, 48, 88}, dot = {245, 196, 64};
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            int cx = x / 64, cy = y / 64;
            C3 c = ((cx + cy) % 2) ? cellA : cellB;
            float dx = x % 64 + 0.5f - 32.0f, dy = y % 64 + 0.5f - 32.0f;
            float r = std::sqrt(dx * dx + dy * dy);
            float k = clamp01(19.0f - r);          // круг радиуса ~19 px, край 1 px
            c = lerp(c, dot, k);
            unsigned char* p = &img[(y * N + x) * 3];
            p[0] = u8(c.r); p[1] = u8(c.g); p[2] = u8(c.b);
        }
    stbi_write_png(path.c_str(), N, N, 3, img.data(), N * 3);
    std::printf("%s  %dx%d  арна: 3\n", path.c_str(), N, N);
}

void makeLogo(const std::string& path) {
    // треугольная призма в перспективе: передняя грань (треугольник) + боковая (четырёхугольник)
    const std::vector<V2> front = {{106, 404}, {286, 404}, {196, 128}};
    const std::vector<V2> side  = {{286, 404}, {406, 352}, {316, 76}, {196, 128}};
    const V2 seamA = {286, 404}, seamB = {196, 128};   // общее ребро
    const float SOFT = 15.5f;                          // ширина мягкого края, px

    std::vector<unsigned char> img(N * N * 4);
    int partial = 0;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            V2 p = {x + 0.5f, y + 0.5f};
            float dF = sdConvex(front, p), dS = sdConvex(side, p);
            float sd = std::min(dF, dS);
            float a = clamp01(0.5f - sd / SOFT);

            float t = clamp01((p.y - 76.0f) / 328.0f);   // вертикальный градиент
            C3 c = (dF <= dS) ? lerp(C3{255, 214, 72}, C3{238, 104, 36}, t)     // перед: жёлтый -> оранжевый
                              : lerp(C3{196, 64, 128}, C3{104, 30, 96}, t);     // бок: малиновый -> фиолетовый
            float seam = clamp01(2.5f - distSegment(p, seamA, seamB));          // светлое ребро
            c = lerp(c, C3{255, 244, 214}, seam * 0.9f);

            // в прозрачной зоне RGB = цвет ближайшей грани: при LINEAR-фильтрации нет тёмной каймы
            unsigned char* q = &img[(y * N + x) * 4];
            q[0] = u8(c.r); q[1] = u8(c.g); q[2] = u8(c.b);
            q[3] = (unsigned char)std::lround(a * 255.0f);
            if (q[3] != 0 && q[3] != 255) partial++;
        }
    stbi_write_png(path.c_str(), N, N, 4, img.data(), N * 4);
    std::printf("%s  %dx%d  арна: 4  (аралық альфа: %d пиксель)\n", path.c_str(), N, N, partial);
}

int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : ASSET_DIR;
    makePattern(dir + "/pattern.png");
    makeLogo(dir + "/logo.png");
    return 0;
}
