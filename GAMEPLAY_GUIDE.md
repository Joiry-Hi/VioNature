# VioNature 操作指南

> ## ⚠ 重要：游戏参数通过配置文件调节
>
> 本游戏几乎所有参数（200+ 项）都可通过 **`config/gameplay.cfg`** 自定义调整，包括但不限于：
> - 武器伤害、射速、弹速、爆炸半径、冷却时间
> - 敌人血量、刷新间隔、Boss 属性
> - 玩家移动速度、重力、跳跃高度
> - 地图类型与尺寸
> - 游戏模式（生存 / 教学 / 决斗 / Boss Rush）
>
> **修改方法**：用**记事本**（或任意纯文本编辑器）直接打开 `config/gameplay.cfg`，修改数值后保存，重启游戏即可生效。文件内每条参数均有英文注释说明其用途。
>
> > **常见问题**：从 zip 包解压出的文件可能带有"只读"属性，导致无法保存修改。
> > **解决**：右键点击 `gameplay.cfg` → **属性** → 取消勾选"只读" → 确定，即可正常编辑保存。
>
> 详细参数列表见文末「配置文件参考」章节。**强烈建议在游戏过程中按自己手感持续调整参数。**

### 游戏内控制台（快捷调参）

游戏内置开发控制台，无需退出游戏即可实时修改所有参数：

1. 按 **`~`**（Tab 上方的键）打开控制台
2. 输入 `key = value` 格式的命令，回车执行，例如 `gravity = 15` 即时降低重力
3. **Tab** 键自动补全参数名（从 219 个参数中匹配）；**↑↓** 键浏览候选/历史
4. 输入完整参数名时右侧自动显示当前值
5. 修改在当前游戏局内生效；要永久保存需编辑 `config/gameplay.cfg`

这让你能像炼金术士一样实时调配游戏参数——先通过控制台找到最舒适的手感，再固化到配置文件。

---

## 基础操作

| 按键 | 功能 |
|------|------|
| **W A S D** | 前后左右移动 |
| **鼠标移动** | 瞄准（上下/左右旋转视角；网页版需先点击画面获取鼠标锁定） |
| **鼠标左键** | 开火（按住连射） |
| **鼠标右键** | 武器特殊功能（蓄力/切换模式/长按指挥界面） |
| **鼠标滚轮** | 切换武器 / 调整纳米平台投放距离 / 调节闪现距离 |
| **空格键** | 跳跃 / 飞行模式下升高 |
| **Ctrl** | 飞行模式下降低高度 |
| **Shift** | 跑步（加速移动） |
| **数字键 1-8** | 直接切换武器 1-8 |
| **数字键 0** | 直接切换到神秘法杖（武器 9） |
| **~** | 打开/关闭内置开发控制台（实时调参） |
| **P** | 隐藏/显示全部 HUD 和手持武器模型 |
| **F11** | 全屏 / 窗口切换 |
| **R** | 重新开始（重置游戏） |
| **Z** | 切换太空服（低重力模式）开/关 |
| **X** | 切换飞行装置 开/关 |
| **C** | 切换滑板 开/关 |
| **F** | 靠近击落/停泊 UFO 时进入驾驶舱；驾驶 UFO 时离开 |
| **Q / E** | 驾驶 UFO 时，将玩家 Essence 存入 UFO / 从 UFO 取回 |
| **H（长按）** | 驾驶 UFO 时蓄能并进入超空间航道 |

### UFO 驾驶与超空间控制

| 按键 | 功能 |
|------|------|
| **W A S D** | 驾驶 UFO 前后左右移动；超空间中 A/D 沿圆柱隧道环向移动 |
| **鼠标移动** | UFO cockpit 视角 / 火控方向 |
| **鼠标左键** | 使用当前 UFO 武器/子模式：小型球状闪电、青蓝激光或牵引光束 |
| **鼠标右键** | UFO 小型球状闪电武器内切换主/副模式：追踪弹 / 青蓝激光 |
| **鼠标滚轮** | 切换 UFO 武器 |
| **空格 / Ctrl** | UFO 升高 / 降低；超空间中调节离隧道内壁高度 |
| **Shift** | UFO 加速移动 |
| **H（长按）** | 消耗 UFO essence，进入超空间航道并前往随机新世界 |
| **Q / E（可长按）** | 玩家 Essence 与 UFO 独立 essence 池互通 |

### Web / 浏览器版操作提示

- 首次进入网页版本时，先**点击游戏画面一次**，浏览器会把鼠标锁定到 canvas，用于 FPS 相对视角输入。
- 如果切出页面、按 Esc、或浏览器释放了鼠标锁定，再点击画面即可重新获得控制。
- 网页版会自动按 16:9 等比缩放画面，适应浏览器窗口或 itch.io 嵌入框；画面两侧/上下出现黑边是正常的完整显示模式。

## 武器详解

VioNature 共有 **9 把武器**，每把武器都可以通过 **单击右键**（< 0.22 秒）在主/副模式之间切换。

### 1. 激光步枪 (LASER)
- **主模式：电浆连射** — 高速低伤害弹，射速极高（0.075s/发），适合持续压制
- **副模式：蓄力光束** — 右键切换，按住左键蓄力，松手发射穿透光束，伤害随蓄力条增长
- **超级模式（Essence 消耗）** — 手持激光枪时**同时按住左右键**开始充能，每 1 秒消耗 1 essence（可配置），屏幕震动 + 金色粒子爆发。累计消耗达到阈值（默认 5）后进入"超级充能"状态。松一键暂停充能，进度保留。松开双键发射：
  - **达到阈值** → 发射**球状闪电**：彩虹变色球体缓慢飞行，每隔 0.35s 自动向最近敌人发射彩色激光，expire 时巨大彩虹爆炸（复用火箭弹爆炸外形但大得多）
  - **未达阈值** → 发射**彩虹光束**：持续伤害，宽度随充能增加，持续时间与消耗 essence 数正相关，玩家可旋转扫射
- HUD 模式标识：P / L

### 2. 火焰喷射器 (FLAME)
- **主模式：火焰弹 (F)** — 中距离弹道，半径随飞行时间膨胀，覆盖范围大。飞行时间与最大尺寸可通过配置调节（`flame_lifetime` / `flame_max_radius`）
- **副模式：凝固汽油弹 (NAPALM)** — 发射一枚弹跳榴弹，在地面弹跳滚动。引信到期（可配）或碰到敌人/法阵框架时爆燃：范围伤害 + 生成地面火圈（持续伤害）+ 范围内敌人被"点燃"
- **点燃机制** — 敌人着火后持续扣血（DPS 可配），身上冒火焰粒子。每隔一定时间向身边敌人**连锁引燃**（传播距离和冷却可配）。离开火场后仍持续灼烧
- HUD 模式标识：F / NAPALM

### 3. 火箭筒 (ROCKET)
- **主模式：火箭弹** — 经典爆炸武器，命中/超时爆炸，附带火箭跳位移
- **副模式：无人机仓 (D)** — 发射一枚大口径榴弹式部署仓，弹道为抛物线，落地后固定 1 秒放出中型战斗无人机（四旋翼）。无人机自动寻找最近敌人，用机枪扫射（小型弹，无反弹，落地消失）+ 每 2 秒发射一枚火箭弹。无人机数量上限可通过配置调整（默认 10，无硬上限），每架持续 300 秒
- **无人机鸟群算法** — 无人机使用 boids 集群算法，具有分离（防止重叠）、凝聚（朝集群中心靠拢）、对齐（匹配集群速度）三种行为力，可在配置中分别调整各力的半径和强度
- **长按右键：无人机指挥界面 (> 0.22s)** — 手持火箭筒时，长按右键打开战术指挥界面（无暗化处理，非瞄准镜）。界面显示：无人机数量/上限、敌人数、目标距离、当前模式。所有敌人以正八面体线框标记附着在 3D 空间中，**不被障碍物遮挡**（X-ray 透视效果）。按 **鼠标左键** 设置无人机集合点
- **无人机集合点系统** — 设置集合点后，所有无人机进入"集结中"(ASSEMBLING) 状态，飞向集合点。全部到齐后进入"驻守"(HOLDING) 阶段，在集合点附近维持鸟群状态。驻守时间到后恢复"完成"(COMPLETE)，无人机恢复正常寻敌行为。驻守时间可通过 `drone_rally_hold_time` 配置。在驻守完成前重新设置集合点会刷新计时
- HUD 模式标识：D（火箭模式不显示）

### 4. 霰弹枪 (SHOTGUN)
- **主模式：霰弹 (P)** — 多发射击弹丸，附带后坐力位移（可叠加入移动链）
- **副模式：玻璃碎片 / 尘埃云 (G)** — 多发碎片出膛后受空气阻力逐渐减速，降到阈值速度后以鸟群算法聚拢成球形尘埃云，悬浮在配置高度持续造成伤害。出膛速度、阻力系数、云半径、分离距离、聚拢力度、成形时间等均可配置
- HUD 模式标识：P / G

### 5. 重力钉枪 (NAIL)
- **主模式：重力钉 (N)** — 命中或落地后产生牵引力场，持续拉拽周围敌人并造成伤害
- **副模式：黑洞手雷 (BH)** — 投掷黑洞，更大范围 + 更强吸力 + 中心"事件视界"秒杀区域。黑洞对玩家也有视界判定
- HUD 模式标识：N / BH

### 6. 无限手套 (GAUNT)
- **右键切换模式**：TimeStop (TS) | Blink (B)
- **TS 模式 — 左键：时间停止** — 冻结所有敌人、弹幕、力场
- **B 模式 — 左键：闪现传送** — 向面朝方向传送；**滚轮**对数调节距离（可配上下限）；落点处以 3 个相互垂直的虚线圆球标出预计位置；左下角 HUD 固定显示当前闪现距离
- **超级技：响指 (SNAP)** — 切到无限手套时，**同时按住左右键**蓄力；蓄满后松开两个键消耗 essence，对场上所有普通敌人逐个进行生死检定。每个敌人若随机结果低于 `gauntlet_snap_kill_chance` 则即死，否则存活。消耗量、蓄力时间和即死概率均可配置
- HUD 模式标识：TS / B（时停激活中显示 T）

### 7. 朗基努斯之枪 (SPEAR)
- **主模式：投掷 (S)** — 高速红色双叉长枪，穿透伤害 + 玩家反向反冲（空中投掷可位移）
- **副模式：AT 推进 (T)** — 前方锥形伤害 + AT 立场橙黄色冲击波 + 强力后坐力位移。**推进期间短暂无敌**（时长可通过 `longinus_spear_thrust_invuln` 配置，默认 0.5 秒）
- HUD 模式标识：S / T

### 8. 纳米构造仪 (NANO)
- **主模式：纳米刃波 (B)** — 纳米机器人构筑新月形刃波，缓慢推进但伤害极高（1000/s），持续约 10s
- **副模式：纳米平台 (P)** — 在前方面向投射一块可站立的临时平台，持续 300s。滚轮可调整投放距离（0.35x~1.85x）
- **水滴铸造（究极技能）** — 手持纳米构造仪，**站定 + 同时按住左右键**逐步消耗 essence（默认 10 个可配）。玩家身前的半成品呈现为金色钻石框架，进度通过透明度与内光球指示。**铸造可中断续铸**：松开按键后，半成品框架保留在原处，玩家可靠近已存在的半成品（4m 内）继续消耗 essence 铸造。完成后水滴激活：
  - **平面/地心世界**：向敌人高速直冲（80m/s），极高接触伤害（500）并**直接穿透**，自动索敌，有金色残影尾迹
  - **小行星地图**：悬浮于地表一定高度（可配），切线方向冲撞敌人
  - 水滴间存在**斥力**（可配），自动均匀分布在战场上
  - 水滴不过虫洞，不跨表里世界
- HUD 模式标识：B / P

### 9. 神秘法杖 (STAFF)  — 按 0 键切换
- **主模式：诅咒法球 (C)** — 紫色追踪灵弹，命中敌人后施加 DoT 持续诅咒伤害，诅咒致死时释放灵魂法球链式反应。DPS 叠加倍率、追踪速率均可配置
- **副模式：秘法护盾 (S)** — 右键切换至护盾模式。左键部署球形护盾，阻挡敌人并推开，被敌方弹幕击中后碎裂释放紫色冲击波推开范围内敌人与弹幕。碎裂后进入冷却。**护盾激活后长按左键**可向场上所有 Essence 拾取物释放金色脉冲光波，揭示位置
- **魔法阵召唤** — 手持法杖、站立地面、静止时，**同时长按左右键** 3 秒完成施法（消耗 1 essence），脚下生成五角星魔法阵 + 浮空正八面体框架。法阵只响应指定玩家弹种：**电浆球、火焰球、火箭弹、霰弹**会把框架激活为对应弹种的紫色追踪版本，并继承原武器的基础寿命、伤害、射速等参数。**凝固汽油弹**（无论罐体或爆炸）和**玻璃碎片**也能激活法阵——汽油弹激活后法阵自动向外发射弹跳汽油弹榴弹，玻璃碎片激活后发射追踪碎片弹幕。朗基努斯之枪可解除/关闭法阵
- **虫洞激活** — 黑洞榴弹命中大魔法阵框架时，会把法阵永久转化为紫色虫洞入口，并在”另一面世界”生成对应出口。玩家或敌人接触虫洞会切换到另一物理层；朗基努斯之枪可关闭虫洞，使里世界入口湮灭
- HUD 模式标识：C / S / SA（护盾激活）/ SC（护盾冷却）

## 敌人图鉴

| 敌人 | 外观 | 血量 | 行为特征 |
|------|------|------|-----------|
| **Skitter** 爬行者 | 红色小球 | 1 | 基础近战敌人，直冲玩家，碰触造成伤害 |
| **Brute** 蛮兵 | 橙色大球 | 4 | 坦克型，移动慢但血量高 |
| **Wisp** 幽光 | 蓝色球 | 1.5 | 高速移动，呈波浪轨迹闪避，难以命中。第 25s 触发"幽光突袭"事件（×5） |
| **Spitter** 喷射者 | 绿色球 | 2.2 | 远程攻击，自动与玩家保持距离并射击。第 52s 触发"喷射伏击"（×3） |
| **Pouncer** 跃击者 | 紫色小球 | 1.8 | 蓄力后猛然弹射跳向玩家。第 82s 触发"跃击突袭"（×4） |
| **Harrier** 骚扰者 | 青色小球 | 1.4 | 空中悬停，正弦机动 + 远程射击。Wave 4 触发"骚扰群"事件（×3） |
| **Blinker** 闪烁者 | 粉色球 | 2.1 | 蓄力预警 → 瞬移到玩家侧后 → 高速冲刺。Wave 4 触发"闪烁打击"事件（×2） |
| **Boss** 几何领主 | 紫色大球 | 1000 | Wave 2 开始后第 50s 出现，发射追踪弹幕，血量低于 45% 后弹幕加速 |
| **Boss** 史莱姆王 | 绿色大球 | 200 | 第 100s 出现，地面移动 → 远跳/高跳 → 重砸冲击波。死亡分裂为 4 个小史莱姆，最多分裂 2 代 |
| **Duelist** 决斗者 | 金色球 | 100 | 仅在 Duel 模式出现，会使用玩家的全部 9 种武器，根据距离选择不同策略，会切换武器、蓄力、闪现、火箭跳；死亡时掉落 Essence |
| **Boss** 伯利恒之星 | 金色正方体 + 六芒 | 200（可配） | 按地图类型出现在不同位置：小行星地图沿赤道轨道环绕，平面地图高悬中心上方，空心世界位于球心。攻击方式为巨型激光柱，分预警（无伤透明追踪）和伤害（低透明确认命中）两阶段，预警激光初始锁定玩家后缓慢旋转追踪，伤害阶段继续保持追踪。出现时间、轨道参数、激光数据均可配置。**特殊免疫**：不计入敌人计数，不受 AoE（爆炸/冲击波/重力井/热浪/纳米刃波）伤害，追踪弹不以其为索敌目标——仅有弹丸直接命中可造成伤害，为其赋予"高维天外实体"的独特定位 |
| **Boss** 座天使 | 白色眼球 + 黑瞳 + 三层共心环 | 900（可配） | 高空游走，三层白色圆环随机旋转；周期性召唤小天使群。小天使以鸟群逻辑追近玩家并发射白色追踪弹。座天使会释放无伤白色斥力波，震开周围实体并让玩家短暂失重 |
| **Boss** 炽天使 | 白金光焰核心 + 六翼 | 850（可配） | 高空游走，周期性朝玩家方向喷射放大版球形霰弹式圣火球簇。圣火球命中地表后生成白金圣火层，会持续灼烧玩家和同 world 敌人 |
| **Boss** 拾荒者 UFO | 低模圆盘飞碟 | 360（可配） | 任意模式中玩家超级球状闪电爆炸后，一局一次触发入侵。主 BGM 静音，延迟后 UFO 在表世界降临并播放专属遭遇战 BGM。它优先牵引并收集散落 Essence，收集满后撤离；无 Essence 或威胁接近时发射小型追踪球状闪电，击败后会缓慢坠落并停在地面，玩家可按 `F` 进入驾驶舱，再通过超空间航道前往随机新战场 |
| **Dummy** 训练假人 | 灰色球 | 10（可配） | 仅 Tutorial 模式，不移动不攻击，用于测试武器伤害 |

## 拾取物

| 拾取物 | 效果 | 切换键 |
|--------|------|--------|
| **太空服**（蓝色） | 重力降至 24%（0.24x），可手动开关 | Z |
| **飞行装置**（青色） | 悬停飞行，空格升高 / Ctrl 降低，可手动开关 | X |
| **滑板**（绿色） | 地面摩擦大幅降低，保持高动量滑动，可手动开关 | C |
| **Essence**（彩虹星形八面体） | 额外生命 +1，HUD 下方显示六芒星数 | 自动拾取 |

太空服、飞行装置、滑板在开局时自动分布在地图半周位置。Essence 为定时刷新拾取物（默认 45 秒间隔），非开局必刷。

## Essence 生命系统

星形八面体彩虹拾取物在地图上定时刷新，拾取后增加额外生命：

- **显示**：HUD 顶部 TIME 下方以实心六芒星排列显示当前生命数（≤6 个星，>6 显示 +N）。六芒星随游戏时间循环变换色彩
- **触发**：受到致命伤害时，若拥有 Essence → 消耗 1 条生命 + 短暂无敌帧（默认 1.5 秒）+ 金色冲击波推开周围敌人与弹幕，**不死亡**
- **拾取**：碰触 Essence 拾取物 → 生命 +1，显示 "ESSENCE +1"
- **感应**：手持法杖、护盾模式激活时，**长按左键**向场上所有 Essence 位置持续释放金色脉冲光波，远距离可见
- 初始生命数、无敌时间、刷新间隔、地图上限均可通过配置调整

## UFO 载具与超空间航道

拾荒者 UFO 是 Boss，也是后续跨地图旅行的入口。玩家用超级球状闪电引来 UFO 后，若在它收集满 Essence 撤离前击败它，UFO 会坠落并停在地面；靠近后按 **F** 可以进入驾驶舱。

- **驾驶态**：普通玩家武器和手持模型隐藏，画面切换为 UFO cockpit / 火控界面。UFO 使用 world 0 物理悬浮，球形地图会按当前表世界地表法线保持高度
- **UFO 武器**：滚轮切换小型球状闪电 / 牵引光束。小型球状闪电武器内可单击右键切换追踪弹 / 青蓝激光两种模式，左键发射当前模式；牵引光束可拉取 Essence 与同世界敌人，拉敌人只改变位置不造成直接伤害
- **独立 Essence 池**：UFO 有自己的 essence 储罐，左下角显示当前/上限；右上角 `P:n` 显示玩家当前 Essence。击败 UFO Boss 后，载具池会保留它已收集的 Essence，并额外带有 `ufo_base_essence` 初始储量。按 **Q** 存入 1 个玩家 Essence，按 **E** 取回 1 个，长按可连续快速转移
- **超空间跃迁**：驾驶时长按 **H** 蓄能，达到阈值后消耗 UFO essence，进入第三人称圆柱隧道航道。A/D 沿隧道环向移动，Space/Ctrl 调整高度；撞到障碍只扣 UFO essence，不会立即失败
- **抵达新世界**：航道计时完成后随机进入新的地图与模式，并在新地图高空继续处于 UFO 驾驶态，玩家可自行下降。玩家 Essence、UFO essence、累计收集数和 UFO 武器模式会保留；旧战场实体会清空重建
- **停泊高度**：驾驶态按 F 离开后，UFO 会在当前高度附近悬浮等待，不会自动回落到默认巡航高度
- **调试开局**：`ufo_start_with_vehicle = true` 可让玩家开局直接驾驶 UFO；`ufo_debug_local_jump_enabled = true` 时启用 J 键本地图短距随机传送

## 地图类型（通过 config/gameplay.cfg 的 `map_type` 切换）

| 地图 | 说明 |
|------|------|
| **circle** | 经典圆形平整竞技场，半径可调，外围散布装饰性柱子 |
| **square** / **square_obstacle** | 方形竞技场，含 8 个碰撞障碍物 + 7 个低层浮台 + 6 个高层浮台（最高 y≈29），垂直分层战斗空间 |
| **asteroid** | 球形小行星表面，重力指向球心，玩家站在球面外侧战斗 |
| **hollow_world** | 空心球壳内部，重力指向球壳外，玩家站在球壳内侧向球心战斗 |
| **eden** | 和平的伊甸圆地图，函数地形中央隆起；无武器、计时暂停，走出外圈后离开 |
| **labyrinth** | 程序化米诺陶迷宫；石墙阻挡玩家/投射物，迷宫会预警重构，米诺陶会寻路追猎并沿直线走廊冲锋 |

## 表里世界与虫洞

当黑洞榴弹激活神秘法杖的大魔法阵后，法阵会变成一对紫色虫洞。虫洞不是独立副本，而是同一空间的“双重物理解释”：

- **小行星 ↔ 地心世界**：`asteroid` 的里世界按 `hollow_world` 物理运行；`hollow_world` 的里世界按 `asteroid` 物理运行
- **平面地图 ↔ 平面背面层**：`circle` / `square_obstacle` 的里世界仍使用平面物理，但整体镜像偏移，方便形成“穿过地表到另一面”的感觉
- **玩家穿越**：接触当前世界的虫洞入口会传送到另一端，传送有冷却，避免反复触发
- **敌人追洞**：敌人与玩家不在同一世界时，会把自己所在世界的虫洞当作索敌目标；穿过虫洞后恢复正常追玩家
- **伤害规则**：玩家和敌人的直接接触伤害只在同一世界生效；投射物、爆炸、黑洞、刀波、无人机等不做世界隔离，可以跨表里层互相影响
- **关闭机制**：朗基努斯之枪命中虫洞/法阵区域可关闭传送门，清除虫洞状态

## 游戏模式（通过 config/gameplay.cfg 的 `game_mode` 切换）

### Survival（生存模式）
- 波次递增难度，敌人刷新速度逐渐加快
- 第 25s、52s、82s 触发脚本化大规模敌袭事件
- 第 50s Boss "几何领主" 降临，第 100s "史莱姆王" 登场
- 击败 Boss 即视为通关，但游戏不结束，敌人持续刷新

### Tutorial（教学模式）
- 无敌人刷新，可自由探索地图、测试全部 9 把武器。有训练假人可供测试伤害
- 首次切换到每把武器时显示操作提示（主/副模式、特殊功能）
- 拾取装备后提示 Z/X/C 切换键
- 适合初次上手或熟悉新武器机制

### Duel（决斗模式）
- 1v1 对抗 AI "决斗者"
- 玩家有 2 层护甲（可配置），每层提供一次无敌帧（0.75s）
- AI 会使用全部 9 种武器 + 所有特殊能力（时间停止、闪现、火箭跳、法杖护盾等）
- 消灭决斗者即获胜

### Boss Rush 模式（通过 `boss_rush_mode = true` 开启）
- 仅 Boss 按各自设定时间生成，无普通敌人和脚本事件
- Geometry Lord 不再召唤护卫小怪
- 史莱姆王和伯利恒之星按各自 spawn_time 出现
- 对 Duel 模式无影响（duel 不受此参数控制）

## HUD 说明

```
TIME 12.3  SCORE 340  E 8  W SPEAR:S  G 0.24x  SUIT  FPS 60
WORLD FRONT  WAVE 2                       SLIME KING [=========     ]
DUEL: ARMOR 2
```

- **TIME**：生存时间（秒）
- **SCORE**：累计得分
- **E**：当前存活敌人数
- **W**：当前武器名:模式缩写
- **G**：当前重力倍率
- **状态词**：GROUND（地面）/ AIR（空中）/ SUIT（太空服）/ FLIGHT（飞行）/ SKATE（滑板）/ STOP（时停中）/ GOD（无敌模式）
- **六芒星**：TIME 下方，实心金色六芒星（色相循环）表示当前 Essence 生命数。受击无敌时变橙色。≤6 个星，>6 显示 +N
- **WAVE / 事件文字**：当前波次或最近的事件提示
- **WORLD FRONT / WORLD BACK**：当前所在表世界 / 里世界
- **Boss/决斗者血条**：仅显示 Boss 和决斗者
- **决斗者当前武器**：显示 Duelist 正在使用的武器
- **UFO cockpit**：驾驶 UFO 时左下角显示 UFO essence 储罐，右上角 `P:n` 显示玩家 Essence；HUD 同时显示当前 UFO 武器、H 键跃迁蓄能进度与超空间航道进度

## 高级技巧

- **火箭跳**：对脚下发射火箭，利用爆炸冲量获得额外高度
- **空中位移链**：火箭跳 → 空中霰弹反冲 → 长枪反冲，可实现极高速度的三段位移
- **钩爪替代**：虽然游戏没有钩爪，但可以反冲向敌人发射重力钉 / 黑洞，利用引力井拉拽获得额外空中控制
- **纳米刃波钓鱼**：发射纳米刃波后，可以用闪现或火箭跳换位，让追逐的敌人撞入刀波范围
- **时停 combo**：时停 → 贴脸发射全部火箭 → 黑洞 → 解冻后瞬间爆发
- **无人机战术**：在不同位置部署无人机形成交叉火力，长按右键打开指挥界面设置集合点，让无人机集群驻守关键位置。对 Boss 战可在不同角度设置多个集合点，配合自身游击形成包围火力网
- **角动量保持**：在球形地图（asteroid / hollow_world）中，切线速度不受重力影响，可利用切线加速获得极高的环绕速度
- **AT 推进无敌帧**：朗基努斯之枪 AT 推进期间无敌，可用来穿越弹幕或硬吃 Boss 攻击
- **法阵阵地战**：在固定位置召唤魔法阵后，向法阵持续射击以填充吸收弹药，法阵会自动向附近敌人发射追踪弹幕
- **虫洞引诱**：用黑洞榴弹把法阵转化为虫洞后，可以先进入里世界，引诱表世界敌人追逐虫洞并穿越，再用黑洞/刀波/无人机在出口处布置交叉火力
- **表里夹击**：投射物不按世界隔离，意味着你可以在里世界布置弹幕，让它们影响表世界敌人；这也是虫洞系统最实验性的部分
- **护盾感应 Essence**：手持法杖护盾模式时，长按左键向场上所有 Essence 位置释放金色脉冲波，快速定位拾取物
- **Essence 换血**：有额外生命时可以故意吃伤害触发无敌帧 + 冲击波推开周围敌人，创造输出窗口
- **UFO 银行**：驾驶 UFO 时可用 Q/E 在玩家生命 Essence 与 UFO 跃迁燃料之间调配资源。超空间抵达新世界后两边资源都会保留，适合把一次战场胜利转化为下一张地图的开局优势

---

## 配置文件参考

配置文件位于游戏目录下的 `config/gameplay.cfg`。下面列出主要参数分类及其作用。完整列表及最新默认值请直接查看该文件（内含英文注释）。

### 游戏模式与地图

| 参数 | 说明 | 可选值 |
|------|------|--------|
| `game_mode` | 游戏模式 | survival / tutorial / duel |
| `boss_rush_mode` | Boss Rush 模式开关 | true / false |
| `map_type` | 地图类型 | circle / square_obstacle / asteroid / hollow_world / eden / labyrinth |

### 地图几何

| 参数 | 说明 |
|------|------|
| `circle_radius` | 圆形竞技场半径 |
| `eden_play_radius` / `eden_map_radius` | 伊甸内圈半径与离开半径 |
| `eden_height_scale` / `eden_height_epsilon` | 伊甸函数高度场缩放与中心软化项 |
| `asteroid_radius` | 小行星球面半径 |
| `asteroid_player_altitude` / `_enemy_altitude` | 小行星表面玩家/敌人高度 |
| `hollow_world_radius` | 空心球壳内半径 |
| `hollow_world_player_altitude` / `_enemy_altitude` | 球壳内玩家/敌人距壳距离 |
| `labyrinth_width` / `labyrinth_height` / `_cell_size` | 米诺陶迷宫网格尺寸与单格大小 |
| `labyrinth_shift_interval` / `_warning_time` | 迷宫重构间隔与预警时间 |
| `labyrinth_minotaur_*` | 米诺陶生成、生命、追踪与冲锋参数 |

### 表里世界与虫洞

| 参数 | 说明 |
|------|------|
| `wormhole_player_cooldown` | 玩家虫洞传送冷却 |
| `wormhole_enemy_cooldown` | 敌人虫洞传送冷却 |
| `wormhole_trigger_radius` | 虫洞触发半径 |
| `wormhole_visual_radius` | 虫洞视觉半径 |

### 玩家移动

| 参数 | 说明 |
|------|------|
| `gravity` | 重力加速度 |
| `walk_speed` / `run_speed` | 步行/跑步速度 |
| `ground_acceleration` / `air_acceleration` | 地面/空中加速度 |
| `jump_speed` | 跳跃初速度 |

### 武器参数（部分关键项）

**激光步枪 (6a)**: `plasma_damage`, `plasma_speed`, `plasma_cooldown`, `laser_charge_damage`, `laser_beam_range` 等；超级模式: `super_essence_interval`（消耗间隔）, `super_essence_threshold`（充能阈值）, `super_ball_speed` / `_lifetime` / `_radius`（球状闪电）, `super_ball_fire_interval` / `_beam_damage` / `_beam_range`（球状闪电激光）, `super_ball_explosion_radius` / `_damage`（球状闪电爆炸）, `super_rainbow_beam_base_life` / `_life_per_essence` / `_width_base` / `_width_per_essence`（彩虹光束）

**火焰喷射器 (6b)**: `flame_damage`, `flame_lifetime`, `flame_max_radius`；凝固汽油弹: `napalm_speed`, `napalm_fuse`, `napalm_bounce_restitution`, `napalm_explosion_radius` / `_damage`, `napalm_ignite_duration` / `_dps`, `napalm_spread_radius` / `_interval`（连锁引燃）；火场: `heatwave_fire_patch_lifetime` / `_radius` / `_damage` / `_height`

**火箭筒 (6c)**: `rocket_impact_damage`, `rocket_explosion_damage`, `rocket_explosion_radius`, `rocket_jump_impulse`

**霰弹枪 (6d)**: `shotgun_pellet_damage`, `shotgun_pellet_count`, `glass_shard_damage`, `glass_shard_linger_time`, `glass_shard_cloud_radius` 等

**重力钉枪 (6e)**: `gravity_nail_damage`, `gravity_well_radius`, `gravity_well_force`, `black_hole_radius`, `black_hole_event_horizon_radius` 等

**无限手套 (6f)**: `blink_distance`, `blink_distance_min` / `_max`, `blink_clear_radius`, `time_stop_enabled`, `blink_enabled`；响指超级技: `gauntlet_snap_essence_cost`, `gauntlet_snap_charge_time`, `gauntlet_snap_kill_chance`

**朗基努斯之枪 (6g)**: `longinus_spear_damage`, `longinus_spear_speed`, `longinus_spear_impulse`, `longinus_spear_thrust_damage`, `longinus_spear_thrust_force`, `longinus_spear_thrust_range`, `longinus_spear_thrust_impulse`, `longinus_spear_shockwave_damage`, `longinus_spear_shockwave_force`, `longinus_spear_shockwave_radius`, `longinus_spear_thrust_invuln`（推进无敌时长，秒）

**纳米构造仪 (6h)**: `nano_blade_damage`, `nano_blade_lifetime`, `nano_blade_radius`, `nano_platform_range`, `nano_platform_lifetime` 等；水滴: `water_droplet_essence_cost`（铸造消耗）, `water_droplet_speed` / `_damage` / `_lifetime` / `_radius`, `water_droplet_hover_altitude`, `water_droplet_craft_interval` / `_resume_range`, `water_droplet_turn_rate`, `water_droplet_separation_mult` / `_repulsion_strength`（斥力）

**神秘法杖 (6i)**: `curse_orb_direct_damage`, `curse_orb_dps`, `curse_orb_max_stack_mult`, `curse_orb_speed`, `curse_orb_turn_rate`, `soul_orb_count`, `soul_orb_damage_scale`, `mystic_staff_shield_radius`, `mystic_staff_shield_cooldown`, `mystic_staff_shockwave_radius`, `magic_circle_lifetime`, `magic_circle_radius`, `magic_circle_fire_interval`, `magic_circle_fire_rate_mult`, `magic_circle_homing_turn_rate` 等

### Boss 参数

| 参数 | 说明 |
|------|------|
| `boss_spawn_time` | 几何领主出现时间（秒） |
| `boss_health` | 几何领主血量 |
| `slime_king_spawn_time` | 史莱姆王出现时间 |
| `slime_king_health` / `_radius` / `_speed` | 史莱姆王属性 |
| `slime_king_long_jump_speed` / `_high_jump_speed` / `_slam_speed` | 史莱姆王跳跃/重砸速度 |
| `slime_king_spherical_gravity` / `_surface_damping` | 球形地图重力与阻尼 |
| `slime_king_split_count` / `_max_generations` | 死亡分裂数量/代数 |
| `bethlehem_spawn_time` / `_health` | 伯利恒之星出现时间/血量 |
| `bethlehem_laser_duration` / `_cooldown` / `_damage` | 伯利恒之星激光参数 |
| `throne_enabled` / `_spawn_time` / `_health` | 座天使开关、出现时间和血量 |
| `throne_summon_interval` / `_summon_count` / `_max_cherubs` | 座天使召唤小天使的节奏与数量上限 |
| `cherub_*` | 小天使生命、移动、分离、射程和追踪弹参数 |
| `throne_pulse_*` / `_antigravity_*` | 座天使无伤斥力波与玩家短暂失重参数 |
| `seraph_enabled` / `_spawn_time` / `_health` / `_spawn_count` | 炽天使开关、出现时间、单体血量和独立个体数量 |
| `seraph_separation_*` / `_attack_stagger` | 多只炽天使之间的散开行为与攻击错峰 |
| `seraph_fireball_*` / `seraph_fire_layer_*` | 炽天使圣火球簇与圣火层参数 |
| `ufo_enabled` | 是否启用拾荒者 UFO 入侵 |
| `ufo_spawn_delay` / `_health` | UFO 触发后延迟/血量 |
| `ufo_collect_required` / `_tractor_range` / `_tractor_strength` | UFO 收集 Essence 数量与牵引参数 |
| `ufo_base_essence` | 击败 UFO Boss 后载具池自带的 Essence 数量 |
| `ufo_attack_interval` / `_orb_speed` / `_orb_explosion_radius` | UFO 小型球状闪电攻击参数 |
| `ufo_pilot_orb_laser_damage` / `_range` / `_width` | 玩家驾驶 UFO 时小型球状闪电副模式青蓝激光参数 |
| `ufo_bgm_path` / `_volume` | UFO 遭遇战 BGM 路径与音量 |
| `ufo_hyperspace_bgm_path` / `_volume` | UFO 超空间航道 BGM 路径与音量 |
| `ufo_arrival_altitude` | UFO 跃迁抵达新世界时的高空入场高度 |
| `ufo_arrival_variant_enabled` / `_world_variance` / `_enemy_variance` | UFO 跃迁抵达新世界时的地图尺度、重力、UFO 巡航、敌人节奏等随机变体 |
| `ufo_pilot_*` | UFO 载具进入、悬浮、移动、独立 essence 池、Q/E 资源互通、跃迁和驾驶武器参数 |

### 无人机 (section 10)

| 参数 | 说明 |
|------|------|
| `drone_max_count` | 同时存活的无人机上限 |
| `drone_lifetime` | 每架无人机存活时间 |
| `drone_bullet_damage` / `_speed` / `_shoot_interval` | 无人机机炮属性 |
| `drone_rocket_interval` / `_range` | 无人机火箭发射间隔/射程 |
| `drone_separation_radius` / `_force` | 鸟群分离力 |
| `drone_flocking_radius` / `_force` | 鸟群凝聚力 |
| `drone_rally_hold_time` | 集合点驻守时间 |

### Essence 生命系统 (section 11)

| 参数 | 说明 |
|------|------|
| `starting_essence` | 初始生命数（0 = 无额外生命） |
| `essence_hit_invuln` | 损失 Essence 后无敌时间（秒） |
| `essence_respawn_time` | Essence 拾取物重生间隔（秒） |
| `essence_max_on_map` | 地图上同时存在的 Essence 数量上限 |

### 调试与其他

| 参数 | 说明 |
|------|------|
| `invincible` | 无敌模式（true = GOD 模式，不受任何伤害） |
| `dummy_max_count` / `_health` | Tutorial 模式假人数量/血量 |

### 音频

| 参数 | 说明 |
|------|------|
| `sfx_volume` | 全局音效音量，0 = 静音，1 = 原始音量 |
| `bgm_path` | 主旋律 BGM 文件路径，默认 `assets/BGM/Main_theme.mp3` |
| `bgm_volume` | BGM 音量，0 = 静音，1 = 原始音量 |
| `bgm_loop_gap` | BGM 播完一遍后，到下一遍开始前的间隔秒数 |
| `bgm_altitude_fade_start` / `_end` | 离当前世界地表多高时开始/完成 BGM 衰减 |
| `bgm_altitude_min_volume` | 玩家远离地表后 BGM 保留的最低音量倍率 |
| `bgm_back_world_volume` | 进入里世界后的 BGM 音量倍率，0 = 完全无声，0.5 = 减半 |
| `throne_bgm_path` / `_volume` | 座天使遭遇战 BGM 路径与音量 |
| `seraph_bgm_path` / `_volume` | 炽天使遭遇战 BGM 路径与音量；座天使与炽天使同时存在时座天使优先 |
| `ufo_bgm_path` / `_volume` | 拾荒者 UFO 遭遇战 BGM 路径与音量 |
| `ufo_hyperspace_bgm_path` / `_volume` | UFO 正式进入超空间航道后的专属 BGM 路径与音量 |

---

> **提示**：中文详细注释版配置文件位于 `config/gameplay_annotated.cfg`，可直接复制替换 `gameplay.cfg` 使用。
