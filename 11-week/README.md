# 11-апта — Текстуралар

```bash
cmake -S . -B build && cmake --build build && ./build/cg
```

## Басқару

| Перне | Әрекет |
|-------|--------|
| тінтуір, W A S D | камера |
| `1` `2` `3` `4` | REPEAT / MIRRORED_REPEAT / CLAMP_TO_EDGE / CLAMP_TO_BORDER |
| `V` | UV ×1 → ×2 → ×0.1 |
| `N` / `M` | MAG: LINEAR ↔ NEAREST / MIN: mipmap ↔ LINEAR |
| `B` / `K` | MAG-қа mipmap (қате) / mipmap-сыз текстура |
| `[` `]` | `uvScale` 0.5…8.0 |
| `C` `T` `F` `X` `G` | сурет / tint / су / анизотропия / еден |
| `P` / `R` | скриншот / reset |


## Қысқаша жауаптар

**1. Орау режимдері**
- `REPEAT` — қайталанады → еден, кірпіш, су.
- `MIRRORED_REPEAT` — айнадай аударылады → тігіс жоқ (тас, құм).
- `CLAMP_TO_EDGE` — шеткі пиксель созылады → UI, decal, skybox.
- `CLAMP_TO_BORDER` — сырты шекара түсі → көлеңке картасы.
  

https://github.com/user-attachments/assets/61ef626a-da13-42fa-8bd1-6bb2e0d8120d



**2.** NEAREST — бір тексель, айқын шаршылар. LINEAR — 4 тексель араласады, жұмсақ ауысу.


https://github.com/user-attachments/assets/9317f375-eaf8-4a93-a8a6-b7b44e5d5c19


**3.** 512×512 текстура 20×20 пиксельде: 512/20 = 25.6 → 25.6² ≈ **655 тексель / фрагмент**. Mipmap-сыз — жыпылықтау, mipmap-пен — тегіс.


https://github.com/user-attachments/assets/911226ae-c674-4dcd-8b0e-89e1b870a14e


**4.** Mipmap деңгейлері жоқ, ал MIN mipmap сұраса → текстура толық емес → **қара куб**.
MAG-қа mipmap тұрақтысын беру → `GL_INVALID_ENUM (0x0500)`, MAG `GL_LINEAR` болып қалады.


https://github.com/user-attachments/assets/e9cd07a9-0e2f-454c-89ba-18bf91dc39c3


**5.** `vertices[]` — жылдам, бірақ қатты. Шейдердегі `uv * uvScale` — тегін дерлік және икемді → шейдер жақсы.


https://github.com/user-attachments/assets/10f72967-96c1-4e01-933a-8ed0632a49b4


**6.** Қосымша тапсырмалар


https://github.com/user-attachments/assets/383ddba6-982a-48f8-9971-1d416929da29

