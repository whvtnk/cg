# Неделя 10 — мышь и свободная камера

Сборка: `cmake -S . -B build && cmake --build build && ./build/cg`

Управление:

| Ввод | Действие |
|------|----------|
| мышь | поворот камеры (yaw / pitch) |
| колесо | зум (fov 1°–45°) |
| W A S D | движение туда, куда смотрит камера |
| Esc | выход |

Что сделано:

- добавлены переменные `yaw = -90`, `pitch`, `lastX`, `lastY`, `firstMouse`, `fov`;
- `glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)`, `glfwSetCursorPosCallback`, `glfwSetScrollCallback`;
- `mouse_callback` — `firstMouse`, обратный `yoffset`, `sensitivity = 0.1`, ограничение pitch ±89°, `camFront` из yaw/pitch;
- `scroll_callback` — `fov -= yoffset` в пределах 1°–45°;
- в `glm::perspective` вместо постоянного `45.0f` стоит `fov`.
