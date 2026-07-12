# VioNature

高速复古竞技场 FPS，重度 Quake 风格影响 — 高机动性、深度武器系统、球面地图、Boss 战。C++17，基于 raylib + Jolt Physics。

![Genre](https://img.shields.io/badge/genre-arena%20FPS-blue)
![Style](https://img.shields.io/badge/style-retro%20pixel--art-purple)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![Version](https://img.shields.io/badge/version-V2.7--Space--Travel-orange)

---

## V2.7 Space-Travel 新特性

- **拾荒者 UFO 与超空间航道**：超级球状闪电可引来 UFO Boss；击败后进入驾驶舱，使用 UFO 武器、牵引 Essence，并跃迁到随机新世界
- **UFO 驾驶与资源互通**：UFO 独立 essence 池、Q/E 与玩家 Essence 互通、小型球状闪电与牵引光束双武器
- **超空间隧道模式**：驾驶 UFO 长按 H 进入第三人称圆柱航道，完成航行后随机抵达新地图与新模式
- **无限手套响指超级技**：同时按住左右键蓄满后松开，消耗 Essence，对场上敌人逐个进行可配置即死概率检定
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
| 6 | **无限手套** | 时间停止（冻结所有敌弹） / 闪现传送 | 双键蓄力响指超级技 |
| 7 | **朗基努斯之枪** | 红色双叉枪投掷（穿透+反冲） | AT 推进锥（无敌帧+后坐力） |
| 8 | **纳米构造仪** | 纳米构筑新月刃波（1000 DPS） | 可站立纳米平台（临时地板） |
| 9 | **神秘法杖** | 诅咒法球（追踪+DoT+魂弹链式） | 秘法护盾 + 魔法阵召唤/虫洞框架 |

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
| **Boss — 伯利恒之星** | 金色正方体六芒卫星。地图不同则位置不同（轨道 / 悬停 / 球心）。巨型追踪激光（预警光束 → 伤害光束）。**免疫 AoE 与追踪弹**，仅直击弹丸可伤 |
| **Boss — 座天使** | 白色眼球 + 黑色瞳孔 + 三层随机旋转共心环。高空游走，周期性召唤小天使群；小天使追近玩家并发射白色追踪弹。座天使会释放无伤斥力波，震开实体并让玩家短暂失重 |
| **Boss — 炽天使** | 白金光焰核心 + 六翼。高空游走，周期性朝玩家方向喷射圣火球簇；圣火球落地生成白金圣火层，会灼烧玩家和同层敌人 |
| **Boss — 拾荒者 UFO** | 任意模式中超级球状闪电爆炸后一局一次触发。延迟传送到表世界，牵引散落 Essence，收集满后撤离；击败落地后可按 `F` 进入 UFO 驾驶舱，驾驶 UFO 进入超空间航道并抵达随机新战场 |
| **Duelist** 决斗者 | 镜面对战 AI（仅决斗模式），会使用全部 9 种武器及时停、闪现、护盾，根据距离切换战术，死亡时掉落 Essence |
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
| `asteroid` | 球形小行星表面，重力指向球心；虫洞里世界为 `hollow_world` 物理 |
| `hollow_world` | 空心球壳内部，重力向外指向球壳；虫洞里世界为 `asteroid` 物理 |
| `eden` | 和平的伊甸圆地图；无武器、计时暂停，外圈白化后离开 |
| `labyrinth` | 程序化米诺陶迷宫；石墙走廊会预警重构，米诺陶按格子追猎并在直线走廊冲锋 |

---

## 表里世界与虫洞

神秘法杖召唤的大魔法阵可以被指定玩家弹种激活：电浆球、蓄力激光、火焰球、霰弹、火箭弹会让法阵变成对应弹种的紫色追踪发射框架；朗基努斯之枪可解除/关闭法阵。

黑洞榴弹命中大魔法阵时会把框架永久转化为一对紫色虫洞。玩家穿过虫洞会进入当前地图的“里世界”：小行星与地心世界互为对偶，平面地图则进入带镜像偏移的平面背面层。敌人与玩家不在同一世界时会改为追逐自己所在层的虫洞，并可通过虫洞追到玩家所在层。投射物和范围效果不做世界隔离，这是表里世界互相影响的核心玩法。

---

## 无人机指挥系统

手持火箭筒（任意模式），长按右键打开战术指挥界面：

- 所有敌人以**3D 正八面体线框标记，可穿透地形显示**（X-ray 透视）。
- 界面显示活跃无人机数、敌人数、瞄准点距离、当前模式。
- **左键**设置集合点，无人机进入 集结 → 驻守 → 完成 阶段，驻守固定时间后恢复正常寻敌。
- 无人机使用 **鸟群算法**（分离 / 凝聚 / 对齐）避免扎堆重叠。

---

## UFO 载具与超空间航道

击败拾荒者 UFO 后，它会落地停留并变成可进入载具。靠近按 **F** 进入 cockpit，普通武器隐藏，玩家改为驾驶 UFO 悬浮移动。UFO 有两种武器：小型球状闪电和牵引光束；小型球状闪电可单击右键切换追踪弹 / 青蓝激光，牵引光束可收集 Essence，也可拉拽同世界敌人。

UFO 拥有独立 essence 池。击败 UFO Boss 后，载具池会保留它已收集的 Essence，并额外带有基础储量。驾驶时按 **Q** 可把玩家 Essence 存入 UFO，按 **E** 可取回，长按会连续转移。长按 **H** 消耗 UFO essence 后进入第三人称圆柱形超空间航道，完成航行后随机抵达另一张地图与游戏模式，并从高空进入新战场；玩家 Essence、UFO essence 和当前 UFO 武器状态会保留。离开 UFO 后，它会保持当前悬浮高度等待再次进入。

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
| 构建 | CMake 3.24+ / Emscripten（Web） |
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

### Web / itch.io HTML5 构建

```bash
bash scripts/build-web.sh
```

脚本会使用 Emscripten 构建 WebAssembly 版本，并生成：

- `web-release/dist/index.html`
- `web-release/dist/vionature.html`
- `web-release/dist/vionature.js`
- `web-release/dist/vionature.wasm`
- `web-release/dist/vionature.data`
- `web-release/packages/vionature_web.zip`
- `web-release/packages/vionature_itch_html5.zip`

本地预览：

```bash
cd web-release/dist
python3 -m http.server 8080
```

然后打开 `http://localhost:8080/` 或 `http://localhost:8080/vionature.html`。

注意：`scripts/build-web.sh` 会删除并重建 `web-release/dist/`。如果终端当前就在旧的 `web-release/dist` 目录中，重建后需要重新 `cd /path/to/VioNature/web-release/dist`，否则 Python http server 可能因为当前目录已被删除而报 `FileNotFoundError`。

上传 itch.io 时使用 `web-release/packages/vionature_itch_html5.zip`，并勾选 **This file will be played in the browser**。ZIP 根目录必须直接包含 `index.html`，不能是 `web-release/dist/index.html` 这种外层目录结构。

如果要发布到带宽更敏感的 demo 站点，可以构建无音频轻量 Web 包：

```bash
bash scripts/build-web-no-audio.sh
```

该脚本会把 `assets/` 复制到临时 staging 目录，删除 `.wav`、`.mp3`、`.ogg`、`.flac` 等音频文件，并用 `VIONATURE_NO_AUDIO=ON` 编译出静音版本。输出文件位于：

- `web-release-no-audio/dist/`
- `web-release-no-audio/packages/vionature_web_no_audio.zip`
- `web-release-no-audio/packages/vionature_wakudemo_no_audio.zip`

正常 itch.io / 完整音频 Web 包仍使用 `scripts/build-web.sh`。

Web 版 shell 会自动把 16:9 游戏画面等比缩放到浏览器/itch iframe 可见区域内。进入游戏后需要点击画面一次以授权浏览器 pointer lock；之后鼠标即使被浏览器固定在画面中心，游戏也会通过相对位移正常读取瞄准输入。

### itch.io butler 一键发布

先安装 itch.io 官方命令行工具 butler，并执行一次登录：

```bash
scripts/install-butler.sh
tools/butler/butler login
```

如果系统 PATH 中已经有 butler，也可以直接：

```bash
butler login
```

然后用统一发布脚本构建并上传 Windows、Linux、Web 三个 channel：

```bash
ITCH_TARGET=yourname/vionature bash scripts/publish-itch.sh all
```

常用命令：

```bash
# 只发布 Web HTML5 版
ITCH_TARGET=yourname/vionature bash scripts/publish-itch.sh web

# 使用已构建好的 release 目录，跳过构建直接上传
ITCH_TARGET=yourname/vionature bash scripts/publish-itch.sh windows linux web --skip-build

# 带版本号发布
ITCH_TARGET=yourname/vionature VERSION=0.3.1 bash scripts/publish-itch.sh all
```

默认 channel 名称为 `windows`、`linux`、`html5`。如需改名可用环境变量覆盖，例如 `WEB_CHANNEL=web`。

---

## 配置

所有游戏参数位于 `config/gameplay.cfg`（英文简洁版）或 `config/gameplay_annotated.cfg`（中文注释版），**200+ 可调参数**覆盖：

- 地图几何、重力、玩家移动
- 全部 9 把武器伤害、速度、半径、后坐力、冷却
- 无限手套响指超级技的 essence 消耗、蓄力时间、即死概率
- Essence 生命数、无敌时间、刷新间隔
- 各类型敌人时序与行为
- Boss 与特殊遭遇的出现时间、血量、轨道、激光、UFO 载具属性
- 无人机数量、鸟群参数、集合点驻守时长
- 决斗模式护甲与无敌帧
- 音效与主旋律 BGM 音量、BGM 文件路径、循环间隔
- UFO Boss、UFO 载具、超空间航道、抵达新世界随机变体

完整操作说明与机制详解见 [GAMEPLAY_GUIDE.md](GAMEPLAY_GUIDE.md)。

### 音频配置

- `sfx_volume`：全局音效音量，范围 0–1。
- `bgm_path`：主旋律音乐文件路径，默认 `assets/BGM/Main_theme.mp3`。可换成 `assets/BGM/` 下其他 raylib 支持的流式音乐文件。
- `bgm_volume`：BGM 音量，范围 0–1；设为 0 可静音。
- `bgm_loop_gap`：单曲播完后到下一轮播放之间的等待秒数。
- `bgm_altitude_fade_start` / `bgm_altitude_fade_end` / `bgm_altitude_min_volume`：玩家离当前世界地表越远，BGM 越小；到 fade end 后降到最低保留音量。
- `bgm_back_world_volume`：进入里世界后的 BGM 音量倍率，`0.5` = 减半，`0` = 进入里世界后听不到 BGM。
- `throne_bgm_path` / `throne_bgm_volume`：座天使遭遇战 BGM 路径与音量；默认使用 `assets/BGM/Wheel of Light.mp3`。
- `seraph_bgm_path` / `seraph_bgm_volume`：炽天使遭遇战 BGM 路径与音量；默认使用 `assets/BGM/Seraphim Ignition.mp3`。若座天使与炽天使同时存在，座天使 BGM 优先播放。
- `ufo_bgm_path` / `ufo_bgm_volume`：拾荒者 UFO 遭遇战 BGM 路径与音量；文件缺失时 UFO 事件仍正常，只是静默。
- `ufo_hyperspace_bgm_path` / `ufo_hyperspace_bgm_volume`：UFO 超空间航道专属 BGM；文件缺失时航道照常运行，只是静默。
- `ufo_arrival_altitude`：UFO 超空间抵达新世界时的高空入场高度。
- `ufo_base_essence`：击败 UFO Boss 后载具池自带的 Essence 数量，会与它本局已收集的 Essence 相加并受储罐上限限制。
- `ufo_pilot_*`：UFO 载具驾驶参数，包括进入距离、悬浮高度、移动速度、独立 essence 池、Q/E 资源互通、跃迁消耗、小型球状闪电追踪弹/青蓝激光模式与牵引光束强度。
- `ufo_arrival_variant_*`：UFO 超空间抵达新世界时的地图尺度、重力、UFO 巡航高度和敌人节奏随机变体。
- `throne_*` / `cherub_*`：座天使生成、生命、高空游走、小天使召唤、白色追踪弹幕、无伤斥力波与短暂失重参数。
- `seraph_*`：炽天使生成数量、独立血条、群体散开、高空游走、圣火球簇、圣火层半径/持续时间/伤害参数。

Web / itch.io 版本会把整个 `assets/` 目录打包进 `vionature.data`，因此新增 BGM 文件后重新运行 `scripts/build-web.sh` 即可进入网页发行包。

---

## 操作

| 按键 | 功能 |
|------|------|
| WASD | 移动 |
| 鼠标 | 瞄准（Web 版需先点击画面获取鼠标锁定） |
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
| F | 进入/离开可驾驶 UFO |
| Q / E | UFO 驾驶态下存入/取回玩家 Essence |
| H（长按） | UFO 驾驶态下蓄能并进入超空间航道 |
| P | 隐藏 HUD + 武器模型（截图模式） |
| F11 | 全屏 / 窗口切换 |
| R | 重置游戏 |

---
