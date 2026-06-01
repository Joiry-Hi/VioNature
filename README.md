# VioNature

高速复古竞技场 FPS，重度 Quake 风格影响 — 高机动性、深度武器系统、球面地图、Boss 战。C++17，基于 raylib + Jolt Physics。

![Genre](https://img.shields.io/badge/genre-arena%20FPS-blue)
![Style](https://img.shields.io/badge/style-retro%20pixel--art-purple)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![Version](https://img.shields.io/badge/version-V2.0--dev-orange)

---

## V2.0 新特性

- **第 9 把武器 — 神秘法杖**：诅咒法球（追踪 DoT + 灵魂法球链式反应）、秘法护盾（球壁阻挡 + 碎裂冲击波）、魔法阵（双键施法召唤 + 吸收弹幕自动反击）
- **朗基努斯之枪重制**：EVA 风格红色双叉枪模型，AT 立场橙黄色冲击波，AT 推进附带无敌帧
- **Essence 生命拾取系统**：星形八面体彩虹拾取物，额外生命 + 受伤消耗 + 无敌帧，HUD 六芒星指示器
- **史莱姆王 Boss**：地面移动 → 远跳/高跳 → 重砸冲击波，死亡分裂繁殖
- **训练假人**：教程模式专属，测试武器伤害
- **200+ 可调参数**，支持 gameplay.cfg 热配置

---

## 武器

| # | 武器 | 主模式 | 副模式 |
|---|------|--------|--------|
| 1 | **激光步枪** | 高速电浆连射 | 蓄力穿透光束 |
| 2 | **火焰喷射器** | 膨胀火球（尺寸/飞行时间可配） | 近距热浪锥形冲击波（弹飞敌方弹幕） |
| 3 | **火箭筒** | 爆炸火箭（可火箭跳） | 无人机仓 + 指挥界面 |
| 4 | **霰弹枪** | 散射弹丸 + 反冲位移 | 玻璃碎片尘埃云（鸟群算法持久伤害） |
| 5 | **重力钉枪** | 引力井（拉拽敌人） | 黑洞手雷（事件视界秒杀） |
| 6 | **无限手套** | 时间停止（冻结所有敌弹） / 闪现传送 | 右键切换模式 |
| 7 | **朗基努斯之枪** | 红色双叉枪投掷（穿透+反冲） | AT 推进锥（无敌帧+后坐力） |
| 8 | **纳米构造仪** | 纳米构筑新月刃波（1000 DPS） | 可站立纳米平台（临时地板） |
| 9 | **神秘法杖** | 诅咒法球（追踪+DoT+魂弹链式） | 秘法护盾 + 魔法阵召唤 |

单击右键切换副模式；手持火箭筒时**长按**右键（>0.22s）打开无人机指挥界面。神秘法杖按 **0** 键选中。

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
| **Boss — 几何领主** | 紫色方块 + 环绕尖刺。追踪弹幕（激怒后加速），出场带护卫小怪 |
| **Boss — 史莱姆王** | 绿色巨球。地面移动 → 长跳/高跳 → 重砸冲击波。死亡分裂为 4 个小史莱姆（最多 2 代） |
| **Boss — 伯利恒之星** | 金色正方体六芒卫星。地图不同则位置不同（轨道 / 悬停 / 球心）。巨型追踪激光（预警光束 → 伤害光束） |
| **Duelist** 决斗者 | 镜面对战 AI（仅决斗模式），会使用全部 9 种武器及时停、闪现、护盾，根据距离切换战术 |
| **Dummy** 训练假人 | 仅教程模式，不移动不攻击，用于测试武器伤害 |

---

## 游戏模式

- **Survival（生存）** — 波次递增难度，脚本化敌袭事件，三个 Boss 按时生成。
- **Tutorial（教学）** — 无敌人，自由探索测试全部 9 把武器。训练假人 + 武器切换操作提示。
- **Duel（决斗）** — 1v1 对决 AI 决斗者。玩家有可配置的护甲层数 + Essence 额外生命。
- **Boss Rush**（`boss_rush_mode = true`）— 仅 Boss 生成，无小怪无事件，适合练习或 Boss 挑战。

---

## 地图

| 地图 | 说明 |
|------|------|
| `circle` | 经典平坦圆形竞技场，半径可调 |
| `square` / `square_obstacle` | 方形竞技场，含障碍物 + 多层浮空平台 |
| `asteroid` | 球形小行星表面，重力指向球心。切线速度守恒——适合轨道速度漂移 |
| `hollow_world` | 空心球壳内部，重力向外指向球壳。玩家在内壁战斗，伯利恒之星位于正中心 |

---

## 无人机指挥系统

手持火箭筒（任意模式），长按右键打开战术指挥界面：

- 所有敌人以**3D 正八面体线框标记，可穿透地形显示**（X-ray 透视）。
- 界面显示活跃无人机数、敌人数、瞄准点距离、当前模式。
- **左键**设置集合点，无人机进入 集结 → 驻守 → 完成 阶段，驻守固定时间后恢复正常寻敌。
- 无人机使用 **鸟群算法**（分离 / 凝聚 / 对齐）避免扎堆重叠。

---

## Essence 生命系统

星形八面体（stella octangula）彩虹拾取物，在地图上定时刷新：

- **拾取** → 额外生命 +1（HUD 下方六芒星数增加）
- **受伤** → 消耗 1 条生命 + 短暂无敌帧 + 金色冲击波推开周围敌人弹幕，而非立即死亡
- **护盾感应** → 手持法杖护盾模式时，长按左键向场上所有 Essence 位置释放金色脉冲光波
- 初始生命数、无敌时间、刷新间隔、地图数量上限均可配置

---

## 拾取物

| 拾取物 | 效果 | 切换键 |
|--------|------|--------|
| **太空服**（蓝色） | 重力降至 24% | Z |
| **飞行装置**（青色） | 悬停飞行 | X |
| **滑板**（绿色） | 极低摩擦滑行 | C |
| **Essence**（彩虹星形八面体） | 额外生命 +1 | 自动拾取 |

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

所有游戏参数位于 `config/gameplay.cfg`（英文简洁版）或 `config/gameplay_annotated.cfg`（中文注释版），**200+ 可调参数**覆盖：

- 地图几何、重力、玩家移动
- 全部 9 把武器伤害、速度、半径、后坐力、冷却
- Essence 生命数、无敌时间、刷新间隔
- 各类型敌人时序与行为
- 三大 Boss 出现时间、血量、轨道参数、激光属性
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
| 滚轮 | 切换武器 / 调整纳米平台距离 / 调整闪现距离 |
| 空格 | 跳跃 / 飞行升高 |
| Ctrl | 飞行降低 |
| Shift | 跑步 |
| 1–8 | 选择武器 1–8 |
| 0 | 选择神秘法杖（武器 9） |
| Z / X / C | 切换太空服 / 飞行 / 滑板 |
| P | 隐藏 HUD + 武器模型（截图模式） |
| F11 | 全屏 / 窗口切换 |
| R | 重置游戏 |

---

## Highlights

**9 weapons × 2 modes each** — every weapon has a primary fire and an alternate mode, with the Mystic Staff adding a secret third mode (dual-button magic circle summon).

**Mobility is your best defense.** Rocket jumping, shotgun recoil dashes, spear-throw momentum, AT thrust invincibility frames, blink teleports, flight rigs, and low-grav suits stack into air-strafe chains. Any hit is lethal — unless you have Essence.

**Three bosses with unique mechanics** — the Geometry Lord fires homing barrages and enrages below 45% HP. The Slime King jumps, slams, and splits into clones on death. The Star of Bethlehem orbits like a satellite and sweeps a giant tracking laser.

**Essence life system** — collect rainbow star-octahedron pickups for extra lives. Losing a life triggers a shield-break shockwave that knocks back enemies and projectiles.

**Drone swarm with command interface** — the rocket launcher's alt-fire deploys combat drones that use boids flocking behavior. Long-press right-click opens a tactical overlay with X-ray enemy markers.

**Four distinct maps** — flat arenas, a spherical asteroid surface, and a hollow-world shell interior.

**200+ tunable parameters** — everything from weapon damage to boss spawn timing lives in one config file.

## Weapons

| # | Weapon | Primary | Alternate |
|---|--------|---------|-----------|
| 1 | **Laser Rifle** | Rapid-fire plasma bolts | Charged piercing beam |
| 2 | **Flamethrower** | Expanding fireball (size/lifetime configurable) | Heatwave cone (repels enemy projectiles) |
| 3 | **Rocket Launcher** | Explosive rockets (rocket-jump capable) | Drone canister + command interface |
| 4 | **Shotgun** | Pellet spread with recoil dash | Glass shard dust cloud (boids persistent damage) |
| 5 | **Gravity Nailer** | Gravity well (pulls enemies) | Black hole grenade (event horizon instant kill) |
| 6 | **Infinity Gauntlet** | Time Stop / Blink teleport | Right-click toggles mode |
| 7 | **Longinus Spear** | Red twin-prong spear throw (piercing + recoil) | AT thrust cone (invincibility frames + knockback) |
| 8 | **Nano Constructor** | Nano blade wave (1000 DPS) | Placeable nano-platform (temporary floor) |
| 9 | **Mystic Staff** | Curse orb (homing + DoT + soul orb chain) | Arcane shield + magic circle summon |

Tap right-click to toggle alt-mode; hold right-click (>0.22s) on rocket launcher for drone command overlay. Press **0** for Mystic Staff.

## Enemies

| Enemy | Behavior |
|-------|----------|
| **Skitter** | Basic melee rusher |
| **Brute** | Tanky, slow, high HP |
| **Wisp** | Erratic wave-pattern evasion |
| **Spitter** | Keeps distance, fires aimed projectiles |
| **Pouncer** | Charges a leap toward the player |
| **Harrier** | Airborne, sinusoidal strafe, ranged shots |
| **Blinker** | Telegraph → teleport to flank → high-speed dash |
| **Boss — Geometry Lord** | Purple cube with orbiting spikes. Homing projectile barrages, enrages below 45% HP. |
| **Boss — Slime King** | Giant green sphere. Ground movement → long/high jumps → slam shockwave. Splits into 4 offspring on death (up to 2 generations). |
| **Boss — Star of Bethlehem** | Golden cube-spike satellite. Orbits, hovers, or sits at world center depending on map. Giant tracking laser (warning beam → damaging beam). |
| **Duelist** | Mirror-match AI (duel mode only). Uses all 9 weapons, time-stop, blink, shield, and adapts tactics by range. |
| **Dummy** | Tutorial mode only. Stationary target for damage testing. |

## Game Modes

- **Survival** — Escalating waves, scripted enemy surges, three bosses spawn at configured times.
- **Tutorial** — No enemies. Free exploration of all 9 weapons with training dummies and weapon tips.
- **Duel** — 1v1 against an AI Duelist. Player has armor charges + Essence extra lives.
- **Boss Rush** (`boss_rush_mode = true`) — Bosses only, no regular enemies or events.

## Maps

| Map | Description |
|-----|-------------|
| `circle` | Classic flat round arena |
| `square` / `square_obstacle` | Flat square with obstacles + multi-tier floating platforms |
| `asteroid` | Spherical planetoid surface. Gravity toward core. Tangent velocity conserved — orbital-speed strafing. |
| `hollow_world` | Hollow sphere interior. Gravity outward toward shell. Battle on inner surface looking inward. |

## Drone Command System

Hold right-click with rocket launcher to open tactical command interface:

- All enemies marked with **3D octahedron wireframes visible through terrain** (X-ray).
- HUD shows active drone count, enemy count, range, and mode.
- **Left-click** sets a rally point. Drones enter Assembling → Holding → Complete phases.
- Drones use **boids flocking** (separation, cohesion, alignment).

## Essence Life System

Rainbow stella octangula pickups spawn periodically on the map:

- **Collect** → +1 extra life (hexagram indicator below HUD timer)
- **Hit** → consume 1 life + brief invincibility + golden shockwave knockback, instead of dying
- **Shield ping** → hold left-click in staff shield mode to emit golden pulse waves toward all Essence locations
- Starting lives, invuln duration, respawn interval, and max on map are all configurable

## Pickups

| Pickup | Effect | Toggle |
|--------|--------|--------|
| **Space Suit** (blue) | 0.24× gravity | Z |
| **Flight Rig** (cyan) | Hover flight | X |
| **Skates** (green) | Ultra-low friction sliding | C |
| **Essence** (rainbow star octahedron) | Extra life +1 | Auto-collect |

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

All gameplay parameters live in `config/gameplay.cfg` (English) or `config/gameplay_annotated.cfg` (Chinese annotated). **200+ tunable values** covering:

- Map geometry, gravity, and player movement
- All 9 weapons — damage, speed, radius, recoil, cooldown
- Essence starting lives, invuln duration, respawn timing
- Enemy timing and behavior per type
- Three bosses — spawn times, health, orbit, laser properties
- Drone counts, flocking parameters, rally hold duration
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
| Scroll wheel | Switch weapons / adjust nano-platform range / adjust blink distance |
| Space | Jump / fly up |
| Ctrl | Fly down |
| Shift | Run |
| 1–8 | Select weapons 1–8 |
| 0 | Select Mystic Staff (weapon 9) |
| Z / X / C | Toggle space suit / flight rig / skates |
| P | Hide HUD + weapon model (screenshot mode) |
| F11 | Toggle fullscreen |
| R | Reset game |
