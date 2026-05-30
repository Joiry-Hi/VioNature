# CLAUDE.md — VioNature 项目 AI 协作须知

## 项目概述

VioNature 是一个复古像素风高速竞技场 FPS，C++17，基于 raylib 5.6-dev（渲染）+ Jolt Physics（物理）。Quake 式高机动 + 深度武器系统 + 球面地图 + Boss 战。

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

### 编译验证
改完代码后务必 `cmake --build . --target MyShooter` 确认零错误。本项目只依赖 CMake 构建，无其他构建工具。

## 项目结构

```
src/                    # 游戏源码（核心）
  Game.h / .cpp         # 主游戏类，几乎所有逻辑在此
  GameConfig.h / .cpp   # 配置文件解析，80+ 参数
  PhysicsWorld.h / .cpp # Jolt Physics 封装
  WeaponViewModel.h / .cpp  # 武器 3D 模型渲染
  main.cpp              # 入口，窗口初始化
tools/                  # 辅助工具（ModelViewer 等）
config/                 # 配置文件
  gameplay.cfg           # 英文简洁版
  gameplay_annotated.cfg # 中文注释版（release 打包时重命名为 gameplay.cfg）
scripts/                # Python/Bash 脚本
  package-release.sh     # Windows 打包
assets/                 # 模型、纹理等资源
external/               # Git submodule：raylib + Jolt（不直接修改）
```

## 代码风格与模式

### Game.h / Game.cpp 是唯一的大文件
几乎所有游戏逻辑都在 `Game` 类中（约 5000+ 行）。不要试图拆分成多个文件——保持现状。新增功能在 Game.h 声明、Game.cpp 实现。

### 配置系统
- `GameConfig.h`：结构体字段 + 默认值
- `GameConfig.cpp`：`floats` map 注册 key → `&config.field`，加载后 clamp
- `config/gameplay.cfg`：简洁注释
- `config/gameplay_annotated.cfg`：中文详细注释
- 两个 cfg 文件必须同步更新（新增参数时两边都加）
- 命名风格：`snake_case` in cfg，`camelCase` in C++

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

### 伤害模型
玩家无血量条——任何来自敌人/弹幕/激光的伤害调用 `ApplyPlayerHit()` → 立即死亡（invincible / duel 护甲除外）。不存在 DoT（Damage over Time），所有伤害是瞬间致命。

### 位置与地图
- `IsSphericalMap()` — asteroid 或 hollow_world
- `SphericalUpAt(pos)` — 获取本地"上方"法线
- `SphericalSurfacePoint(pos, altitude)` — 将点投影到球面指定高度
- `ProjectOnSphericalTangent(v, up)` — 投影到切平面

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
- [ ] `Game.h` + `Game.cpp`（逻辑实现）
- [ ] `config/gameplay.cfg`（英文版）
- [ ] `config/gameplay_annotated.cfg`（中文版）
- [ ] `GAMEPLAY_GUIDE.md`（如有 UI/操作变更）
- [ ] `README.md`（如有重大特性变更）
- [ ] 构建验证 `cmake --build . --target MyShooter`

## 编辑注意事项

- 修改 gameplay.cfg 前先 Read——用户和 linter 可能同时修改
- 不要修改 `external/` 下的代码——那是 git submodule
- `build-windows/release/` 和 `build-sandbox/` 中的部分文件被 git 跟踪，注意 `.gitignore` 只忽略构建中间产物
