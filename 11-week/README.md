# 11-week

Құрастыру және іске қосу:

```bash
cmake -S . -B build && cmake --build build && ./build/cg

## Басқару

| Перне | Әрекет |
|-------|--------|
| тінтуір, дөңгелек, W A S D | камера (10-апта) |
| `1` `2` `3` `4` | GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER |
| `V` | `vertices[]` ішіндегі UV көбейткіші: ×1 → ×2 → ×0.1 (1, 2-тапсырма) |
| `N` | MAG: GL_LINEAR ↔ GL_NEAREST |
| `M` | MIN: GL_LINEAR_MIPMAP_LINEAR ↔ GL_LINEAR |
| `B` | MAG = GL_LINEAR_MIPMAP_LINEAR (әдейі қате, 4-тапсырма) |
| `K` | `glGenerateMipmap`-сыз жүктелген текстура (4-тапсырма, қара куб) |
| `[` `]` | `uvScale` 0.5…8.0 (5-тапсырма, `dt`-ге көбейтілген) |
| `C` | container.png ↔ awesomeface.png (Қ1) |
| `T` / `F` / `X` / `G` | tint (Қ2) / ағып жатқан су (Қ3) / анизотропия (Қ4) / еден |
| `P` | скриншот `screenshots/shot-NN.png` |
| `R` | бәрін бастапқы күйге қайтару |


## Скриншоттар

| Файл | Не көрсетеді |
|------|--------------|
| [01-basic](screenshots/01-basic.png) | базалық текстураланған кубтар |
| [02-repeat](screenshots/02-repeat.png) | 1-тапсырма, UV ×2, GL_REPEAT — әр жақта 2×2 |
| [03-mirrored](screenshots/03-mirrored.png) | GL_MIRRORED_REPEAT |
| [04-clamp-edge](screenshots/04-clamp-edge.png) | GL_CLAMP_TO_EDGE |
| [05-clamp-border](screenshots/05-clamp-border.png) | GL_CLAMP_TO_BORDER, қызыл шекара |
| [06-nearest](screenshots/06-nearest.png) | 2-тапсырма, UV ×0.1, fov 8°, GL_NEAREST — шаршылар |
| [07-linear](screenshots/07-linear.png) | сол көрініс, GL_LINEAR — жұмсақ ауысу |
| [08-no-mipmap](screenshots/08-no-mipmap.png) | 3-тапсырма, MIN = GL_LINEAR |
| [09-mipmap](screenshots/09-mipmap.png) | MIN = GL_LINEAR_MIPMAP_LINEAR |
| [10-black-cube](screenshots/10-black-cube.png) | 4-тапсырма: қара кубтар (mipmap деңгейлері жоқ) |
| [10b-bad-mag](screenshots/10b-bad-mag.png) | 4-тапсырма: MAG-қа mipmap тұрақтысы — төменде түсіндірме |
| [11-uvscale-4](screenshots/11-uvscale-4.png) | 5-тапсырма, uvScale = 4 — әр жақта 4×4 |
| [12-own-image](screenshots/12-own-image.png) | Қ1, awesomeface.png, арна: 4 |
| [13-tint](screenshots/13-tint.png) | Қ2, түспен көмкеру |
| [14-aniso-off](screenshots/14-aniso-off.png) / [15-aniso-on](screenshots/15-aniso-on.png) | Қ4, еденге қиғаш қарау, анизотропия 1 / 16 |

Барлығы `./build/cg --shots` арқылы бірдей камерамен және тоқтатылған анимациямен түсірілген.

## 1-тапсырма — орау режимдері

| Режим | Не болады | Қай жағдайға қолайлы |
|-------|-----------|----------------------|
| GL_REPEAT | сурет 2×2 қайталанады | қабырға, еден, шөп, кірпіш — шеттері бір-біріне сәйкес келетін (tileable) текстуралар; ағып жатқан су (Қ3) |
| GL_MIRRORED_REPEAT | әр келесі көшірме айнадай аударылады | шеттері сәйкес келмейтін, бірақ симметриясы маңызды емес табиғи текстуралар (тас, құм, бұлт) — тігіс жоқ |
| GL_CLAMP_TO_EDGE | шеткі пиксель созылады | бір рет салынатын сурет: UI, жапсырма (decal), skybox, шрифт атласы — шетте бөгде түс «ағып» кірмейді |
| GL_CLAMP_TO_BORDER | [0, 1]-ден тыс — шекара түсі (қызыл) | бір рет салынатын сурет, сыртқы аймағы мөлдір/белгілі түс болуы керек жерде: көлеңке картасы (сырты = «көлеңке жоқ»), проекцияланатын текстура |



## 2-тапсырма — NEAREST және LINEAR

UV ×0.1 → бір жаққа 512 · 0.1 ≈ 51 тексель созылады, fov 8° кезінде бір тексель экранда ≈ 15 пиксель.
NEAREST ең жақын бір текселді алады — айқын шаршылар; LINEAR төрт көрші текселді қашықтыққа қарай
араластырады — жұмсақ, бұлыңғыр ауысу.



## 3-тапсырма — mipmap
**Бір фрагментке неше тексель сәйкес келеді.** 512×512 текстурасы бар жақ экранда 20×20 пиксель:

- бір осьте: 512 / 20 = 25.6 тексель бір пиксельге;
- ауданы бойынша: 25.6² ≈ **655 тексель бір фрагментке**.



## 4-тапсырма — қара куб

`glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR)` нақты нәтижесі:

```
MAG = GL_LINEAR_MIPMAP_LINEAR -> glGetError() = 0x0500 (GL_INVALID_ENUM), MAG фактически = 0x2601
```



## 5-тапсырма — UV масштабы пернемен
Жылдамырағы — `vertices[]` (жұмыс алдын ала жасалған), икемдірегі — шейдер. Бір фрагментке бір `vec2 * float`
қазіргі GPU үшін тегін дерлік, сондықтан өзгеретін нәрселер (масштаб, жылжу, анимация) үшін шейдер жақсы.
