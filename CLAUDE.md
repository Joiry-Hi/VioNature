# CLAUDE.md — VioNature 项目 AI 协作须知

## 项目概述

VioNature 是一个复古像素风高速竞技场 FPS，C++17，基于 raylib 5.6-dev（渲染）+ Jolt Physics（物理）。Quake 式高机动 + 深度武器系统 + 球面地图 + Boss 战。

当前项目更接近“FPS 实验室”：大量武器/道具/地图物理/表里世界机制可通过 `config/gameplay.cfg` 调参验证。实现仍以 `Game` 类拥有状态为主，但 `Game.cpp` 已按职责拆成多个实现文件。

## 构建

### Linux 原生（开发调试用）
```bash
cd build-sandbox
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j $(nproc)
# 产物：MyShooter + ModelViewer
```

### Windows 交叉编译 + 打包
```bash
cd build-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake -DCMAKE_BUILD_TYPE=Release -DJPH_USE_DX12=OFF
cmake --build . -j $(nproc)
# 或一键：
bash scripts/package-release.sh
```

### Web / itch.io HTML5 构建
```bash
bash scripts/build-web.sh
# 本地预览：
cd web-release/dist
python3 -m http.server 8080
```

Web 构建产物：
- `web-release/dist/index.html`
- `web-release/dist/vionature.html`
- `web-release/dist/vionature.js`
- `web-release/dist/vionature.wasm`
- `web-release/dist/vionature.data`
- `web-release/packages/vionature_web.zip`
- `web-release/packages/vionature_itch_html5.zip`

上传 itch.io 使用 `web-release/packages/vionature_itch_html5.zip`，并确保 ZIP 根目录直接包含 `index.html`。不要上传外层目录包。`scripts/build-web.sh` 会删除并重建 `web-release/dist/`，如果终端正在旧 `web-release/dist` 中，重建后必须重新 `cd` 进去再开 HTTP server。

### Web 无音频轻量包（wakudemo / 低带宽 demo）
```bash
bash scripts/build-web-no-audio.sh
# 本地预览：
cd web-release-no-audio/dist
python3 -m http.server 8080
```

该脚本用于带宽敏感站点：复制 `config/` 与 `assets/` 到 `web-release-no-audio/staging/`，删除 `.wav/.mp3/.ogg/.flac/.m4a/.aac` 等音频文件，并以 `VIONATURE_NO_AUDIO=ON` 编译。它不影响正常 `web-release/` 和 itch.io 完整音频包。

无音频构建产物：
- `web-release-no-audio/dist/index.html`
- `web-release-no-audio/dist/vionature.html`
- `web-release-no-audio/dist/vionature.js`
- `web-release-no-audio/dist/vionature.wasm`
- `web-release-no-audio/dist/vionature.data`
- `web-release-no-audio/packages/vionature_web_no_audio.zip`
- `web-release-no-audio/packages/vionature_wakudemo_no_audio.zip`

上传 wakudemo 优先使用 `web-release-no-audio/packages/vionature_wakudemo_no_audio.zip`。如果只是改玩法/渲染且无需声音，仍建议跑一次该脚本确认轻量包可构建。

### itch.io butler 发布
```bash
# 首次本机安装/登录
scripts/install-butler.sh
tools/butler/butler login

# 构建并上传 windows / linux / html5 三个 channel
ITCH_TARGET=yourname/vionature bash scripts/publish-itch.sh all

# 只上传 Web，或跳过构建直接上传现有 release 目录
ITCH_TARGET=yourname/vionature bash scripts/publish-itch.sh web
ITCH_TARGET=yourname/vionature bash scripts/publish-itch.sh windows linux web --skip-build
```

默认 channel：`windows`、`linux`、`html5`。可用 `WINDOWS_CHANNEL`、`LINUX_CHANNEL`、`WEB_CHANNEL` 覆盖；可用 `VERSION=0.3.1` 传给 butler `--userversion`。
`publish-itch.sh` 会优先使用 `tools/butler/butler`，找不到时再使用 PATH 中的 `butler`。

### 编译验证
改完代码后务必 `cmake --build . --target MyShooter` 确认零错误。本项目只依赖 CMake 构建，无其他构建工具。

## 项目结构

```
src/                    # 游戏源码（核心）
  Game.h                # 主 Game 类、状态、嵌套类型、成员声明
  Game.cpp              # 构造/析构、Reset/ClearWorld、主 Update/Draw
  GameMath.h            # 共享 inline 数学/helper
  GamePlayer.cpp        # 玩家输入、视角、移动、受击、时停/闪现、玩家向量
  GameWeapons.cpp       # 武器切换、射击入口、玩家武器发射 helper、武器名
  GameProjectiles.cpp   # 投射物、爆炸、热浪、重力井、纳米刀波/平台、魔法阵弹幕
  GameEnemies.cpp       # 敌人 AI、波次、Duelist、Boss、Bethlehem
  GameWorld.cpp         # 地图、碰撞、拾取、球面/平面世界、虫洞/表里世界
  GameRender.cpp        # 全部 Draw* 视觉和 HUD
  GameConfig.h / .cpp   # 配置文件解析，200+ 参数
  PhysicsWorld.h / .cpp # Jolt Physics 封装
  WeaponViewModel.h / .cpp  # 武器 3D 模型渲染
  main.cpp              # 入口，窗口初始化
web/
  shell.html            # Web/itch HTML shell，启动 StartGame、画面缩放、加载状态
tools/                  # 辅助工具（ModelViewer 等）
config/                 # 配置文件
  gameplay.cfg           # 英文简洁版
  gameplay_annotated.cfg # 中文注释版（release 打包时重命名为 gameplay.cfg）
scripts/                # Python/Bash 脚本
  package-release.sh     # Windows 打包
  build-web.sh           # 完整音频 Web/itch.io HTML5 包
  build-web-no-audio.sh  # 无音频轻量 Web 包（wakudemo / 低带宽 demo）
assets/                 # 模型、纹理等资源
  BGM/                  # 流式播放的背景音乐，例如 Main_theme.mp3
  SFX/                  # wav 音效与 SFX.txt 说明
external/               # Git submodule：raylib + Jolt（不直接修改）
```

## 代码风格与模式

### Game 类仍是状态拥有者，但实现已按职责拆分
不要把新功能塞回 `Game.cpp`。新增/修改功能时优先放入对应实现文件：

- 玩家移动、受击、时停、闪现：`GamePlayer.cpp`
- 玩家武器输入和发射入口：`GameWeapons.cpp`
- 投射物、范围效果、魔法阵弹幕、爆炸：`GameProjectiles.cpp`
- 敌人、Duelist、Boss、波次：`GameEnemies.cpp`
- 地图、拾取、碰撞、球面/平面物理、虫洞/表里世界：`GameWorld.cpp`
- 绘制和 HUD：`GameRender.cpp`

第一轮重构只做了“机械分层”，没有抽 ECS 或系统对象。继续开发时不要大规模迁移成员所有权；若要抽 `WorldSurface` / `ProjectileSystem` / `EnemySystem`，应单独规划。

### 配置系统
- `GameConfig.h`：结构体字段 + 默认值
- `GameConfig.cpp`：`floats` map 注册 key → `&config.field`，加载后 clamp
- `config/gameplay.cfg`：简洁注释
- `config/gameplay_annotated.cfg`：中文详细注释
- 两个 cfg 文件必须同步更新（新增参数时两边都加）
- 命名风格：`snake_case` in cfg，`camelCase` in C++
- 音频参数：`sfx_volume`、`bgm_path`、`bgm_volume`、`bgm_loop_gap`、`bgm_altitude_fade_start`、`bgm_altitude_fade_end`、`bgm_altitude_min_volume`、`bgm_back_world_volume`。BGM 用 `MusicStream` 流式播放，默认文件在 `assets/BGM/Main_theme.mp3`，完整 Web 构建会随整个 `assets/` 目录进入 `.data` 包；无音频 Web 构建会剔除音频文件并定义 `VIONATURE_NO_AUDIO`。

### 武器模式系统
每把有副模式的武器的模式通过右侧 enum 管理，右键切换模式：
```cpp
enum class ShotgunMode { Pellet, GlassShard };
ShotgunMode shotgunMode_ = ShotgunMode::Pellet;
```
新增武器模式时遵循这个模式。

### 渲染管线
- 像素画风格：所有渲染到 426×240 `RenderTexture`，然后 `TEXTURE_FILTER_POINT` 放大到屏幕
- 3D 渲染在 `BeginMode3D` / `EndMode3D` 之间
- 2D HUD 在 `EndMode3D` 之后、`EndTextureMode` 之前
- X-ray 穿透效果：用第二个 `BeginMode3D`/`EndMode3D` 对 + `rlDisableDepthTest()`

### Web 版启动、缩放与输入
- Web 版不依赖 Emscripten 自动调用 `main()`；`web/shell.html` 在 runtime 初始化后通过 `Module.ccall('StartGame')` 显式启动游戏
- `CMakeLists.txt` 的 Web 链接参数包含 `-sINVOKE_RUN=0` 和 `-sEXPORTED_RUNTIME_METHODS=ccall`
- Web preload 目录可用 CMake cache 覆盖：`VIONATURE_WEB_CONFIG_DIR` 和 `VIONATURE_WEB_ASSETS_DIR`；`scripts/build-web-no-audio.sh` 依赖这两个参数把 staging 资源包进 `/config` 与 `/assets`
- `VIONATURE_NO_AUDIO=ON` 会定义 `VIONATURE_NO_AUDIO`，跳过音频加载/播放；用于无音频轻量 Web 包，不要用于正常完整音频发布
- `main.cpp` 中 `StartGame()` / `GameMainLoop()` 使用 `EMSCRIPTEN_KEEPALIVE`，并为 Web 主循环保持 `Game` 对象生命周期
- `web/shell.html` 使用 16:9 容器将 canvas 等比缩放到浏览器/itch iframe 可见区域，避免画面被裁剪
- Web 鼠标视角不要直接依赖 raylib `GetMouseDelta()`：浏览器/itch 会把鼠标锁在画面中心。`main.cpp` 安装 JS pointer-lock 输入桥，累积 `mousemove.movementX/Y`；`GamePlayer.cpp` 的 `UpdateLook()` 在 `PLATFORM_WEB` 下消费 `ConsumeWebMouseDeltaX/Y()`
- 修改 Web shell 或入口后需要重新运行 `scripts/build-web.sh`，并上传新生成的 `web-release/packages/vionature_itch_html5.zip`；如果目标是 wakudemo/低带宽 demo，还要运行 `scripts/build-web-no-audio.sh` 并上传 `web-release-no-audio/packages/vionature_wakudemo_no_audio.zip`

### 伤害模型
玩家无血量条——任何来自敌人/弹幕/激光的伤害调用 `ApplyPlayerHit()` → 立即死亡（invincible / duel 护甲除外）。不存在 DoT（Damage over Time），所有伤害是瞬间致命。

### 位置与地图
- `IsSphericalMap()` — asteroid 或 hollow_world
- `IsHollowPhysicsForWorld(world)` — 判断某个 world 是否按地心世界内壁物理解释
- `SphericalUpAt(pos, world)` — 获取指定表/里世界的本地"上方"法线
- `SphericalSurfacePoint(pos, altitude, world)` — 将点投影到指定世界的球面高度
- `ProjectOnSphericalTangent(v, up)` — 投影到切平面
- `FlatGroundYForWorld(world)` / `FlatUpForWorld(world)` — 平面地图表/里世界地面与朝向

### 表里世界与虫洞
- `playerWorld_`：0 = 表世界，1 = 里世界
- `Enemy::world`：敌人当前遵循的物理层
- `WormholePortal`：保存 front/back 入口、镜像平面、传送冷却、关联法阵索引
- 球面地图对偶：`asteroid` 的 world 1 使用 `hollow_world` 物理；`hollow_world` 的 world 1 使用 `asteroid` 物理
- 平面地图的里世界使用镜像/偏移的平面层；`kFlatBackWorldDepth` 在 `GameMath.h`
- 敌人与玩家不同 world 时应索敌自己世界的虫洞；同 world 后正常索敌玩家
- 玩家与敌人接触伤害只应在同 world 时发生；投射物/爆炸/黑洞/刀波/无人机不做世界隔离，这是设计目标
- 虫洞关闭逻辑在 `CloseWormhole()` / `CloseWormholeAlongSegment()`，朗基努斯之枪相关命中需调用它

### 魔法阵激活规则
- 只有玩家的电浆球、蓄力激光、火焰球、霰弹、火箭弹能激活大魔法阵
- 黑洞榴弹命中法阵框架会转化为虫洞
- 朗基努斯之枪可解除/关闭法阵或虫洞
- 魔化弹幕应尽量继承原型武器的寿命、伤害、射速等基础参数；法阵本身只提供射速倍率和追踪参数
- 激光和电浆弹要保持区分：蓄力光束使用 `activatedByLaserBeam`，普通电浆弹使用 `ProjectileKind::LaserShot`

### 物理
- 玩家/敌人/弹丸都有 Jolt physics body
- 运动学实体（如伯利恒之星）无物理体，用纯数学计算位置
- 自定义碰撞检测用 `DistancePointToSegment()` 做点-线段距离

### 命名约定
- 成员变量：`trailingUnderscore_`
- 局部变量：`camelCase`
- 配置字段：`camelCase`
- 配置文件 key：`snake_case`
- 函数：`PascalCase()`
- 常量：`kPascalCase` 或 `ALL_CAPS`

## 已知注意事项

### 滚轮与 eventText 冲突
当滚轮用于调节数值（闪现距离、平台距离）时，**不要设置 eventText_**，否则会导致 HUD 闪烁。改为在 HUD 固定位置静态显示当前值。

### 只读属性提示
Release zip 包解压后 .cfg 文件可能带只读属性。文档中已注明用户需先取消只读再用记事本编辑。

### 性能敏感点
- 弹丸数 × 弹丸数的 O(n²) 算法（如玻璃碎片鸟群）需要速度阈值 + 成形时间双重兜底
- 持续性开火可能导致大量弹丸堆积，注意设置合理的寿命上限

### 文件更新检查清单
新增功能后需确认：
- [ ] `GameConfig.h` + `GameConfig.cpp`（字段、解析、clamp）
- [ ] `Game.h` + 对应 `Game*.cpp`（逻辑实现）
- [ ] `config/gameplay.cfg`（英文版）
- [ ] `config/gameplay_annotated.cfg`（中文版）
- [ ] `GAMEPLAY_GUIDE.md`（如有 UI/操作变更）
- [ ] `README.md`（如有重大特性变更）
- [ ] 构建验证 `cmake --build build-sandbox -j2`
- [ ] Smoke test `build-sandbox/MyShooter --smoke-test`
- [ ] 如影响 Web 完整版：`scripts/build-web.sh`，并检查 `web-release/packages/vionature_itch_html5.zip` 根目录含 `index.html`
- [ ] 如影响 wakudemo / 低带宽 Web demo：`scripts/build-web-no-audio.sh`，并检查 `web-release-no-audio/packages/vionature_wakudemo_no_audio.zip`

## 编辑注意事项

- 修改 gameplay.cfg 前先 Read——用户和 linter 可能同时修改
- 不要修改 `external/` 下的代码——那是 git submodule
- `build-windows/release/` 和 `build-sandbox/` 中的部分文件被 git 跟踪，注意 `.gitignore` 只忽略构建中间产物
