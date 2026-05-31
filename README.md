# VioNature

高速复古竞技场 FPS，重度 Quake 风格影响 — 高机动性、深度武器系统、球面地图、Boss 战。C++17，基于 raylib + Jolt Physics。

![Genre](https://img.shields.io/badge/genre-arena%20FPS-blue)
![Style](https://img.shields.io/badge/style-retro%20pixel--art-purple)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)

---

## 亮点

**8 把武器 × 双模式** — 每把武器都有主模式和副模式，共 16 种战斗手段。数字键或滚轮即时切枪。

**机动性即防御。** 火箭跳、霰弹反冲、长矛投掷位移、闪现传送、飞行装置、低重力太空服——各种位移技可以叠加成空中机动链。游戏没有血条——任何攻击命中即死。活下去的唯一方式是比弹幕更快。

**四种地图** — 平坦竞技场（圆形 / 带垂直平台的方形）、球形小行星表面（重力指向球心）、空心球壳内部（站在内壁向球心战斗）。

**两个机制独特的 Boss** — 几何领主发射环形旋转弹幕，血量低于 45% 后弹幕密度和速度提升。伯利恒之星在不同地图上的位置各不相同（轨道卫星 / 高空悬停 / 球心），攻击方式为巨型追踪激光柱，预警光束先出再由伤害光束接替。

**无人机集群指挥** — 火箭筒副模式可部署战斗无人机，使用鸟群算法自主战斗。长按右键开启战术指挥界面，以 X-ray 八面体标记透视所有敌人，左键设置集合点控制无人机集结驻守。

---

## 武器

| # | 武器 | 主模式 | 副模式 |
|---|------|--------|--------|
| 1 | **激光步枪** | 高速连射弹 | 蓄力穿透光束 |
| 2 | **火焰喷射器** | 膨胀火球 | 近距热浪锥形冲击波（可弹飞敌方弹幕） |
| 3 | **火箭筒** | 爆炸火箭（可火箭跳） | 无人机仓 + 指挥界面 |
| 4 | **霰弹枪** | 9 发散射弹丸 + 反冲位移 | 弹跳玻璃碎片（一次反弹） |
| 5 | **重力钉枪** | 引力井（拉拽敌人） | 黑洞手雷（事件视界秒杀） |
| 6 | **无限手套** | 时间停止（冻结所有敌弹） | 闪现传送（清除落点范围） |
| 7 | **反冲长矛** | 高速投掷长矛（自身反冲） | 音速推进锥（定向加速） |
| 8 | **纳米构造仪** | 纳米构筑新月刃波（50 DPS） | 可站立纳米平台（临时地板） |

单击右键切换副模式；手持火箭筒时**长按**右键（>0.22s）打开无人机指挥界面。

---

## 敌人

| 敌人 | 行为 |
|------|------|
| **Skitter** 爬行者 | 基础近战冲撞 |
| **Brute** 蛮兵 | 坦克型，移动慢血量高 |
| **Wisp** 幽光 | 波浪轨迹高速闪避，难以瞄准 |
| **Spitter** 喷射者 | 保持距离远程射击 |
| **Pouncer** 跃击者 | 蓄力后弹射跳向玩家 |
| **Harrier** 骚扰者 | 空中悬停，正弦机动 + 远程射击 |
| **Blinker** 闪烁者 | 预警 → 瞬移侧后 → 高速冲刺 |
| **Boss — 几何领主** | 紫色方块 + 环绕尖刺。环形弹幕（12 发 / 激怒后 18 发），出场带护卫小怪 |
| **Boss — 伯利恒之星** | 金色正方体六芒卫星。地图不同则位置不同（轨道 / 悬停 / 球心）。巨型追踪激光（预警光束 → 伤害光束） |
| **Duelist** 决斗者 | 镜面对战 AI（仅决斗模式），会使用全部 8 种武器及时停、闪现，根据距离切换战术 |

---

## 地图

| 地图 | 说明 |
|------|------|
| `circle` | 经典平坦圆形竞技场，半径可调 |
| `square` / `square_obstacle` | 方形竞技场，含障碍物 + 多层浮空平台 |
| `asteroid` | 球形小行星表面，重力指向球心。切线速度守恒——适合轨道速度漂移 |
| `hollow_world` | 空心球壳内部，重力向外指向球壳。玩家在内壁战斗，伯利恒之星位于正中心 |

---

## 游戏模式

- **Survival（生存）** — 4 波递增难度，脚本化敌袭事件，两个 Boss 按时生成。
- **Duel（决斗）** — 1v1 对决 AI 决斗者。玩家有可配置的护甲层数吸收致命伤害。
- **Boss Rush**（`boss_rush_mode = true`）— 仅 Boss 生成，无小怪无事件，适合练习或 Boss 挑战。

---

## 无人机指挥系统

手持火箭筒（任意模式），长按右键打开战术指挥界面：

- 所有敌人以**3D 正八面体线框标记，可穿透地形显示**（X-ray 透视）。
- 界面显示活跃无人机数、敌人数、瞄准点距离、当前模式。
- **左键**设置集合点，无人机进入 集结 → 驻守 → 完成 阶段，驻守固定时间后恢复正常寻敌。
- 无人机使用 **鸟群算法**（分离 / 凝聚 / 对齐）避免扎堆重叠。

---

## 技术栈

| 层 | 库 |
|----|----|
| 渲染 & 输入 | [raylib](https://www.raylib.com/) 5.6-dev（像素管线：426×240 → `TEXTURE_FILTER_POINT` 放大） |
| 物理 | [Jolt Physics](https://github.com/jrouwe/JoltPhysics)（自封装 C++17 包装层） |
| 窗口 / 上下文 | GLFW 3.4 |
| 构建 | CMake 3.24+ |
| 语言 | C++17 |

---

## 构建

### Linux 原生编译

```bash
# 依赖：cmake, g++-13+, libx11-dev, libxcursor-dev, libxrandr-dev, libxinerama-dev
git clone --recurse-submodules <repo-url>
cd VioNature

mkdir -p build-sandbox && cd build-sandbox
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j $(nproc)
```

编译产物：`build-sandbox/MyShooter`（游戏）+ `build-sandbox/ModelViewer`（模型预览工具）。

### Windows 交叉编译（从 Linux）

```bash
# 额外依赖：mingw-w64
cd build-windows
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DJPH_USE_DX12=OFF
cmake --build . -j $(nproc)
```

或使用打包脚本一键构建 + 打包：

```bash
bash scripts/package-release.sh
```

脚本会自动完成：构建 → 收集 `MyShooter.exe` + MinGW DLL → 复制资源文件 → 以中文注释版配置替换默认配置 → 放入游戏手册 → 打包为 `build-windows/VioNature_Release.zip`。

---

## 配置

所有游戏参数位于 `config/gameplay.cfg`（英文简洁版）或 `config/gameplay_annotated.cfg`（中文注释版），80+ 可调参数覆盖：

- 地图几何、重力、玩家移动
- 全部武器伤害、速度、半径、后坐力
- 各类型敌人时序与行为
- Boss 出现时间、血量、轨道参数、激光属性
- 无人机数量、鸟群参数、集合点驻守时长
- 决斗模式护甲与无敌帧

完整操作说明与机制详解见 [GAMEPLAY_GUIDE.md](GAMEPLAY_GUIDE.md)。

---

## 操作

| 按键 | 功能 |
|------|------|
| WASD | 移动 |
| 鼠标 | 瞄准 |
| 左键 | 开火 |
| 右键（单击） | 切换武器副模式 |
| 右键（长按，手持火箭筒） | 无人机指挥界面 |
| 滚轮 | 切换武器 / 调整纳米平台距离 |
| 空格 | 跳跃 / 飞行升高 |
| Ctrl | 飞行降低 |
| Shift | 跑步 |
| 1–8 | 选择武器 |
| Z / X / C | 切换太空服 / 飞行 / 滑板 |
| P | 隐藏 HUD + 武器模型（截图模式） |
| F11 | 全屏 / 窗口切换 |
| R | 重置游戏 |

---

## Highlights

**8 weapons × 2 modes each** — every weapon has a primary fire and an alternate mode, giving 16 distinct combat tools. Weapon-switching is instant via number keys or scroll wheel.

**Mobility is your best defense.** Rocket jumping, shotgun recoil dashes, lance-throw momentum, blink teleports, flight rigs, and low-grav suits stack into air-strafe chains. The game has no health bar — every hit is lethal. Survive by out-moving everything.

**Four distinct maps** — flat arenas (circle / square with vertical platforms), a spherical asteroid surface where gravity pulls inward, and a hollow-world shell where you fight on the inside of a sphere.

**Two bosses with unique mechanics** — the Geometry Lord fires rotating ring barrages and enrages below 45% HP. The Star of Bethlehem orbits the arena like a satellite and sweeps a giant tracking laser, telegraphed by a transparent warning beam.

**Drone swarm with command interface** — the rocket launcher's alt-fire deploys combat drones that use boids flocking behavior. Long-press right-click opens a tactical overlay with X-ray enemy markers; left-click sets rally points where drones assemble and hold position.

## Weapons

| # | Weapon | Primary | Alternate |
|---|--------|---------|-----------|
| 1 | **Laser Rifle** | Rapid-fire projectiles | Charged piercing beam |
| 2 | **Flamethrower** | Expanding fireball | Close-range heatwave cone (repels projectiles) |
| 3 | **Rocket Launcher** | Explosive rockets (rocket-jump capable) | Drone canister + command interface |
| 4 | **Shotgun** | 9-pellet spread with recoil dash | Bouncing glass shards (one rebound) |
| 5 | **Gravity Nailer** | Pinned gravity well (pulls enemies) | Black hole grenade (event horizon kills) |
| 6 | **Infinity Gauntlet** | Time stop (freezes all enemies/projectiles) | Blink teleport (clears landing zone) |
| 7 | **Recoil Lance** | High-speed thrown lance (self-knockback) | Sonic thrust cone (directional boost) |
| 8 | **Nano Constructor** | Nano-constructed crescent blade wave (50 DPS) | Placeable nano-platform (temporary floor) |

Right-click toggles alt-mode; hold right-click (>0.22s) on the rocket launcher for the drone command overlay.

## Enemies

| Enemy | Behavior |
|-------|----------|
| **Skitter** | Basic melee rusher |
| **Brute** | Tanky, slow, high HP |
| **Wisp** | Erratic wave-pattern evasion, hard to track |
| **Spitter** | Keeps distance, fires aimed projectiles |
| **Pouncer** | Charges a leap that launches toward the player |
| **Harrier** | Airborne, sinusoidal strafe, ranged shots |
| **Blinker** | Telegraph → teleport to flank → high-speed dash |
| **Boss — Geometry Lord** | Purple cube with orbiting spikes. Rotating ring projectiles (12 / 18 enraged). Support spawns on arrival. |
| **Boss — Star of Bethlehem** | Golden cube-spike satellite. Orbits, hovers, or sits at world center depending on map. Fires a giant tracking laser (warning beam → damaging beam). |
| **Duelist** | Mirror-match AI (duel mode only). Uses all 8 weapons, time-stop, blink, and adapts tactics by range. |

## Maps

| Map | Description |
|-----|-------------|
| `circle` | Classic flat round arena. Configurable radius. |
| `square` / `square_obstacle` | Flat square with obstacles + multi-tier floating platforms for vertical combat. |
| `asteroid` | Spherical planetoid. Gravity points toward the core; players walk on the outer surface. Tangent velocity is conserved — great for orbital-speed strafing. |
| `hollow_world` | Hollow sphere interior. Gravity pulls outward toward the shell. Players fight on the inner surface looking inward. The Star of Bethlehem sits at the exact center. |

## Game Modes

- **Survival** — 4 escalating waves, scripted enemy surges at set times, two bosses spawn at their configured times.
- **Duel** — 1v1 against an AI Duelist. Player has configurable armor charges that absorb lethal hits.
- **Boss Rush** (`boss_rush_mode = true`) — bosses only, no regular enemies or events. Great for practice or boss-focused runs.

## Drone Command System

Holding the rocket launcher (any mode) and long-pressing right-click opens the tactical command interface:

- All enemies are marked with **3D octahedron wireframes visible through terrain** (X-ray).
- HUD shows active drone count, enemy count, range to aim point, and current mode.
- **Left-click** sets a rally point. Drones enter Assembling → Holding → Complete phases, holding position for a configurable duration before resuming normal pursuit.
- Drones use **boids flocking** (separation, cohesion, alignment) to avoid clustering.

## Tech Stack

| Layer | Library |
|-------|---------|
| Rendering & input | [raylib](https://www.raylib.com/) 5.6-dev (pixel-art pipeline: 426×240 → upscale with `TEXTURE_FILTER_POINT`) |
| Physics | [Jolt Physics](https://github.com/jrouwe/JoltPhysics) (custom C++17 wrapper) |
| Window / context | GLFW 3.4 |
| Build | CMake 3.24+ |
| Language | C++17 |

## Build

### Linux (native)

```bash
# Requires: cmake, g++-13+, libx11-dev, libxcursor-dev, libxrandr-dev, libxinerama-dev
git clone --recurse-submodules <repo-url>
cd VioNature

mkdir -p build-sandbox && cd build-sandbox
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j $(nproc)
```

Output: `build-sandbox/MyShooter` (game) + `build-sandbox/ModelViewer` (model viewer tool).

### Windows (cross-compile from Linux)

```bash
# Additionally requires: mingw-w64
cd build-windows
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DJPH_USE_DX12=OFF
cmake --build . -j $(nproc)
```

Or use the one-shot release script:

```bash
bash scripts/package-release.sh
```

This builds, collects `MyShooter.exe` + MinGW DLLs + assets, swaps in the annotated Chinese config, adds the gameplay guide, and packs everything into `build-windows/VioNature_Release.zip`.

## Configuration

All gameplay parameters live in `config/gameplay.cfg` (English) or `config/gameplay_annotated.cfg` (Chinese annotated). Over 80 tunable values covering:

- Map geometry, gravity, and player movement
- All weapon damage, speed, radius, and recoil values
- Enemy timing and behavior per type
- Boss spawn times, health, orbit parameters, and laser properties
- Drone counts, flocking parameters, and rally hold duration
- Duel mode armor and invulnerability frames

The release script packages the annotated Chinese config as the default `gameplay.cfg`.

Full controls and mechanics are documented in [GAMEPLAY_GUIDE.md](GAMEPLAY_GUIDE.md).

## Controls

| Input | Action |
|-------|--------|
| WASD | Move |
| Mouse | Aim |
| Left click | Fire |
| Right click (tap) | Toggle weapon alt-mode |
| Right click (hold, rocket launcher) | Drone command interface |
| Scroll wheel | Switch weapons / adjust nano-platform range |
| Space | Jump / fly up |
| Ctrl | Fly down |
| Shift | Run |
| 1–8 | Select weapon |
| Z / X / C | Toggle suit / flight / skates |
| P | Hide HUD + weapon model (screenshot mode) |
| F11 | Toggle fullscreen |
| R | Reset game |
