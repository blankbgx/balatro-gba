# GBAlatro 自定义开发待办（level-up issues）

> 已完成的修复不在此列。这些是设计讨论中确定的后续工作，按优先级排列。

## ✅ 已修复：8 底注通关前卡死（2026-08-19 立案，同日修复）

根因是 M25（见 docs/REGRESSION_TESTS.md）：`s_cards_discarded` 音高计数器在 HAND_SHUFFLING 下复位不可达 → 跨回合泄漏 → 音效 rate 变负 → maxmod 野跳转。与构筑无关。WINPROBE 标记已随修复移除（commit 42b2830 的探针已撤）。

## 🔍 调查中：幽灵卡面（2026-08-18 立案）

**现象**：一张数据完全正常的方片A（能被小丑正确识别、能组 3A 葫芦）卡面显示为方片2，**持续数回合**，新开游戏消失（偶发）。
**已排除**：两张素材表图块均正确无重复；LUT 索引与素材布局一致；OBJ tile 静态分区无重叠（手牌 0-255 / 出牌 256-335 / 盲注 336-415 / 小丑 416+）；4 个 VRAM 写入点（card.c×2、blind.c、joker.c）均按 layer 分区。
**嫌疑**：动态 layer/slot 碰撞——卡面每次抽牌都会重拷，持续错面说明覆写在拷贝之后反复发生。
**手段**：DEBUG 构建内置看门狗（`gfx_face_watchdog`，game.c，ROUND 状态每 15 帧校验手牌 VRAM 槽，不匹配时识别槽内实际是**哪张牌面**=覆写者身份），mgba 日志 + 屏幕顶部红色 `GFXBUG slot:card>shown` 文本（Delta 无日志控制台，靠屏显）。
**下一步**：用户持 DEBUG ROM 复现 → 上报 GFXBUG 行的数字（槽位:真实花色点数>显示花色点数）→ 定位覆写路径。
**坑**：看门狗只在 `DEBUG_SHOP_FREE=1` 构建编译；`-1 -1` 表示槽内容不是任何已知卡面（→ 覆写者是非卡面图形或野指针）。

## 🧪 开发期交付默认 DEBUG 构建（2026-08-18 定）

**规则**：开发阶段交付到百度云的测试 ROM **默认带 `DEBUG_SHOP_FREE=1`**（免费商店+免费 reroll → 任何目标构筑都能靠刷 reroll 凑出），命名带 `_DEBUG` 后缀。**源码默认保持关**——`make` 裸构建永远是正式版（2026-08-15 drift 事故教训：提交默认开 = 所有 main 构建带作弊）。`Makefile` 新增 `make debug` 目标（clean + 带旗标重建）；注意 clean 会删字体 `.s`，docker 流程需先在宿主机重新生成字体再构建。

**为什么**：M20/M22 这类需要特定构筑的复测靠自然对局太难凑（信用卡/模糊+花色小丑），用户明确"开发阶段默认 debug=1"。

## 📌 回归测试工作流（2026-08-07 定）

**规则**：每次用户报 bug → 修复后，**同一 commit 里同步更新 `docs/REGRESSION_TESTS.md`**——在文档顶部插入新条目（M 编号 / 日期 / commit / 复测步骤 / 预期结果 / 边界），供将来定期复测（rebase、大版本、架构改动后逐项核对）。

**为什么**：防止 bug 回归。文档含全部已修复 bug（M1-M8 起），定期跑一遍确认旧问题不复发。

## ⛔ 开发纪律：GBA 禁止 FILE 输出（2026-08-09 归档，dde6361）

**规则**：项目代码**禁止调用任何 newlib FILE 层函数**——`printf`/`fprintf`/`puts`/`fputs`/`iprintf`/`fopen`/`fwrite`/`fclose`/`fflush`/`assert`（`__assert_func` 内部走 `fprintf(stderr)`）。文本渲染一律 `vsnprintf` + `tte_write`（见 `include/tte_printf_override.h`）。

**为什么**：tonc 的 `tte_printf` 被宏定义为 `iprintf`（tonc_tte.h:472），走 stdout FILE 链（`__sbprintf → _fflush_r → __swrite → _write_r → devoptab` 间接调用）。本仓库运行时 devoptab/句柄区（0x030072e8-0x0300731c）会被破坏（根因未完全定位，见 Temp/gbalatro-devoptab-handoff-2026-08-09.md），任何 FILE 输出 → 跳飞 0x5EC002E4 → 卡死（deck 选择/Options 界面，已修 dde6361）。存档/成就/快照走 SRAM 内存映射（save.c），与 FILE 层零关系——**新增存储功能继续走 SRAM 段，不要引入文件系统**。

## 🎯 主线目标：完成全部小丑实装（当务之急）

当前自定义小丑已实装 **19 张（53-71）**：53 Wee、54 Riff-Raff、55 Baron、56 Mime、57 Egg、58 Smeared、59 Faceless、60 Gros Michel、61 Cavendish、62 Flower Pot、63 Loyalty Card、64 Riding the Bus、65 Ceremonial Dagger、66 Credit Card、67 Burglar、68 Flash Card、69 Showman、70 Card Sharp、**71 To the Moon（冲向月球，利息翻倍，被动不可复制，gfx0 slot 31）**。注册表已含全部已实现条目（0-71）；原版其余 joker（72+）尚未加入，需完整走：效果实现 → 精灵量化 → 注册 → 映射 → 测试。每个交付照常走：编译 → 命名时间戳 ROM → upload_rom.sh 上传。

## 🌐 上游命名规范化跟踪（2026-08-10 记，待定案）

**背景**：上游 GBALATRO/balatro-gba 正在系统性规范化代码命名。

**已合并**：
- #591：静态变量改 `s_` 前缀 + 常量改大写（大规模重命名）
- #563/#597/#599：Sprite mode 字段 / obj_aff_copy 移除 / 独立 RNG（本地已含，无冲突）

**进行中**：
- #609：静态函数前缀风格政策（issue 讨论中）
- #611/#604/#605：移除函数 `game_` 前缀（blind_select 等）
- #606/#607：对局中按住 B 查看小丑描述（上游也在做描述系统）

**对本地的影响**：
- 本地大量代码用 `game_` 前缀（`game_round_*`、`game_start` 等）——**上游若继续移除 game_ 前缀，将来 rebase 会产生大量冲突**
- 描述系统：本地已有描述面板（动态描述/长描述/clamp≥0），上游实现可对比取舍后吸收

**行动**：⏸ 等上游命名政策定案（#609）后再评估是否对齐；**rebase 上游前先查 #609 状态**。不急。

## 🎵 HUD roll 动画结束节奏课题（2026-08-10 定，低优先，纯视觉）

**背景**：用户希望 Burglar 的 +N/-N 数字动画结束时有"停顿节奏感"（参照通用延迟队列的 30帧/效果 节奏，DEVELOPMENT_TODO.md §通用延迟动作队列：`DEFER_DELAY=FRAMES(30)`）。

**尝试与回滚**：
- 提交 `b2631f9`：加 `HUD_ROLL_END_HOLD=FRAMES(30)`（渐变到目标后保持 30 帧再收尾）→ 另一 agent 测试报**动画跳乱** → 曾 `git revert`（`3e00268`）
- 当前稳定版 = `3fdb6d5`（label 画数字上方 + 渐变同值跳过——修好了开局手数闪烁）

**✅ 2026-08-10 已恢复并优化（cherry-pick ab63d31 + 9c2dd4d + 后续 commit）**：
- 30帧保持已恢复（`HUD_ROLL_END_HOLD=FRAMES(30)`）
- **修复"跳乱"嫌疑点**：HOLD 恢复时**去掉 `erase_rect` 擦除**——`display_hands()/display_discards()` 自己会擦 3 字符宽区域再重绘；原实现擦 ROLL 扩展区（±HUD_ROLL_DY）会误擦数字上方内容（label 区/背景）→ 可能是"跳乱"根源
- **待用户验证**：渐变到目标后保持 30 帧再收尾，无跳乱/错位/重影

**状态**：⏳ 待用户验证（2026-08-10）。

## 🔖 注册表加"原版 ID"属性（2026-08-09 定）

**进度**：✅ **注释版已完成**（提交，2026-08-09）——`source/joker_effects.c` 全部 72 条注册条目已加 `(orig #XX)` 注释（如 `// 53 Wee Joker (orig #124)`），数据源 `docs/balatro_jokers_merged.md`（fandom Nr 1-150），**纯注释 gba 产物不变**。特例：Riding the Bus → orig #44（merged.md 写作 "Ride the Bus"）。

**未来结构化字段**（图鉴系统做时再实施，会改变 gba 产物）：`JokerInfo` 结构（include/joker.h）加 `u8 original_id` 字段，存原版 fandom 编号。

**用途**：
- 快速定位文档属性（当前从项目 ID 查 merged.md 要靠手工换算——**项目 ID ≠ 原版 ID**，例：Blue Joker 项目 ID 34 = 原版 Nr 53）
- 未来图鉴（codex/dex）系统直接读取展示

**实施要点**（结构化时）：
- 字段放 `name` 后；全部 72 个注册条目（0-71）填充原版 Nr，数据源 = `docs/balatro_jokers_merged.md` 的 Nr 列（按名字对齐，注意同名换算）
- **无法游戏内显式验证**（当前无消费方）——验证方式 = 静态脚本：逐条比对注册表 original_id ↔ merged.md Nr ↔ 名字，保证 0-71 全填且无重复错位
- 后续新增 joker（72+）注册时同步填 original_id（作为注册流程一步）

## ✅ 通用延迟动作队列（已完成 2026-08-04，7a2ce13 起）

**状态**：✅ **已实装并全场景实测通过**（Riff-Raff 真身/复制 + 匕首 + 蓝图/脑暴复制，含卡死修复与节奏调优）。新开局触发 joker 直接 `schedule` 入队即可，无需复制队列。

**最终设计**（与初版差异已修正）：
- 统一队列：`{source JokerObject*, kind (RIFF_RAFF/DAGGER)}` 平行数组 + 单一 per-frame 调度器（`deferred_effects_process_pending`），严格左→右逐请求执行：激活（锁定参数 + 弹动画）→ 等 `DEFER_DELAY=FRAMES(30)` → 生效（spawn / 献祭）→ **等全员 settle（入场/重排动画播完）+ 30帧** → 下一个
- **参数在激活时锁定**（非分发时，2026-08-04 实证修正）：Riff-Raff 生成数 = 激活瞬间 free（expired 排除）；匕首 victim = 激活瞬间右侧紧邻
- **source 有效性**：`deferred_source_is_alive()` = 在 owned 且不在 expired；激活分支 do-while 统一跳过死请求（验证分支只管蓄力期）——**防卡死的关键**
- **不超员**：fire 时按实际列表长度封顶；`add_joker()` 加容量保护（SPACING_LUT 越界防护）
- 发牌门控：`joker_effects_busy()` = 队列非空；忙→闲转换后**有效果才** +30帧 缓冲再发牌（静默回合立即发牌）
- **架构红线**（写死到注释）：ON_BLIND_SELECTED 效果函数**不得直接修改 jokers 列表**，只能入队/延迟；生效时刻检查状态，禁止分发时检查
- 节奏对齐对局内：30帧/效果 ≈ 卡牌移动间隔；FRAMES() 随 game_speed 缩放

## 🎨 Negative/版本小丑调色板约束（2026-08-04 评审定稿）

**来源**：2026-08-04 讨论 + MoA 评审（桌面端跑通）。GBA OBJ 4BPP 共 16 个调色板 bank（`pal_obj_mem`）。当前分配：卡牌 0-2、boss 盲注 3（运行时换写）、小丑 `JOKER_BASE_PB=4` 起按需分配（`s_joker_spritesheet_pb_map` + 引用计数，sheet 内共享）。

**评审结论：运行时 XOR 生成负片 bank 可行，但只占"负片系统"的 1/3**：

- ⭐ **核心玩法定位**：Negative edition 的效果 = **不占小丑槽位**（有效槽位 +1），实现更多 joker 同时在场——这是 Balatro 核心玩法，**将来必定要做**。负片调色板只是该系统的视觉子集；**动态槽位（里程碑②）是主线需求**，负片 pb 是其中一环
- ✅ **tile 零成本**：负片只改 sprite ATTR2 的 pb 字段，复用原 tiles；不占 gfx 资产配额（gfx0 已满 15/16）
- 🎨 **负片颜色算法（非纯反相）**：原版 Negative = **灰度化（低对比度）+ 橙红底片基底，再反相**（胶片负片效果）。GBA 实现 = 静态查表一次生成（16 色，运行时零开销）：
  ```
  // BGR15 近似（每通道 5bit）：
  gray   = 0.299R + 0.587G + 0.114B        // 灰度化
  lowc   = gray * 0.6 + 中灰偏移            // 低对比度压暗
  mixed  = 混合橙红基底(约 0x7C00 系)        // 底片色调
  neg[i] = 0x7FFF ^ mixed                   // 反相；i==0 仍透明
  ```
  **不做色彩浮动**（金属光泽/霓虹脉动）——GBA 显示尺寸小 + 性能限制，静态反相足够，动画效果留给 Foil/Holo 后续再说
- ⚠️ **透明索引保护**：`neg[0] = 0x0000`——index 0 是透明色，参与计算会变白底
- ⚠️ **bank 预算最坏会爆**：持有 5 joker + 商店预览 + 描述卡 ≈ 8 sheet 同屏，全负片 = 8+8=16 无余量（还没算牌面/盲注）——**必须定义溢出降级**（掷 edition 前查负片 bank 可用性，不足降级普通版）
- ⚠️ **不是孤立改动**：依赖 `Joker.edition` 字段（用 enum：FOIL/HOLO/POLY/NEGATIVE 将来都要）、+1 槽位动态化（`MAX_JOKERS_HELD_SIZE=5` 牵连 SPACING_LUT 手写 5×5 间距表、MAX_JOKER_OBJECTS 层池、存档数组）、描述卡 edge case（换显示对象时 pb 随 edition 刷新）

**里程碑（排期）**：① edition 字段 + 存档（enum）→ ② 动态槽位（含间距表，UI 活，最大风险）→ ③ 负片 pb（本方案）→ ④ 商店/pack 掷率

## 💳 Credit Card（ID 66）已实装（2026-08-05）

**状态**：✅ 逻辑 + 精灵已实装（提交 51a7ade，gfx15 第二格，15/15 色满）。**商店金钱系统接入后待实测**（当前 `DEBUG_SHOP_FREE` 全部免费，欠款逻辑空转）。

- 效果：购买可欠款至 -20$/张；**仅真身计费**（蓝图/脑暴不能复制被动效果——复制调用 no-op 无产出，与原版一致）；多卡 -20×n；上限实时统计（`count_credit_card_effects()`，每次购买现算，shop.c 检查点）
- **验证点**：① 欠款到 -20 ② 多卡叠加（-40）③ 蓝图/脑暴复制信用卡**无效**（保持 -20）④ 卡面显示正常（紫粉配色）
- 依赖：商店金钱系统（解锁 DEBUG_SHOP_FREE 后）

## 🧠 蓝图/脑暴复制规则（2026-08-05 定稿）

**判断标准：效果是否有"明确的触发动作"**（不是看被动/主动标签）：

| 类型 | 例子 | 蓝图/脑暴复制 |
|------|------|--------------|
| **事件触发动作**（有明确触发点，执行改状态/产出） | 窃贼（选盲注：弃牌归零+手数+3）、Riff-Raff（选盲注 spawn）、倍率/筹码卡 | ✅ 有效——复制体在同一事件再执行一次动作（窃贼+蓝图=+6手数） |
| **静默/被动状态**（无触发点，效果函数 no-op，靠外部查询） | 信用卡（商店欠款额度查询）、Troubadour 类（+手牌数常驻） | ❌ 无效——复制调用 no-op 无产出 |

**实操**：新 joker 实现时——事件触发型（ON_BLIND_SELECTED 等挂动作）→ 蓝图复制自然生效，无需特判；静默型 → 效果函数保持 no-op + 全局查询，蓝图复制自动空转。

## 💾 SaveMeta：成就系统 + joker 解锁进度（为将来预留）

**来源**：2026-08-03 设计讨论。GBAlatro 已有三段式 SRAM 存档（Header@0x00 / Options@0x10 / Game@0x30，magic+hash 校验），缺元进度段。

- 新增 `SaveMeta` 段（`SAVE_SECTION_FLAG_META (1<<2)`，GAME 之后）
  - joker 解锁位图（bit per joker ID）
  - 成就位图
  - 版本号（结构变更兼容）
- API：`unlock_joker(id) / is_joker_unlocked(id)`、成就同款
- 商店池：生成时用 rollable 标志 + 解锁位过滤（游戏初期仅基础 joker 可用）
- 元进度 = 设备级（跨 run 持久），与 SaveGame（run 级）隔离
- SRAM 32KB 空间充足（现有三段仅几百字节）

## 🎮 START 键 Run Info 面板（信息面板入口）

**来源**：2026-08-03 设计讨论。回合制游戏"暂停"无意义，START 键的最佳归宿是信息面板。

- 对局中按 START → 弹出信息面板（背景冻结，模式参考 `GAME_STATE_OPTIONS_MENU` 的弹出面板）
- 面板内容：
  - 盲注序列：当前盲注 + 下一个（Small/Big/Boss）
  - 牌型等级表：各牌型基础 chips/mult（星球牌功能的前置）
  - 放弃本局：确认后回主菜单（不用再打输才能重开）
  - 设置入口：游戏速度/音量/对比度（复用现有 OPTIONS_MENU）
- 实现：状态机加 `GAME_STATE_RUN_INFO`，复用弹出面板模式
- 技术要点：`source/game/options_menu.c` 的弹出模式可参考；`PAUSE_GAME = KEY_START` 宏已存在于 `include/game.h`（标记 Not implemented）

## 🎴 对局内描述框 + hold SELECT 查看描述（含 UI 退避）

**来源**：2026-08-03 设计讨论，用户提出、已确认方案、暂缓实现。

- 语义统一：SELECT 全局 = 查看信息，B 全局 = 返回/取消
- 对局内 hold SELECT → 查看当前选中目标描述（行 0 = 小丑、行 1 = 手牌）
- 描述框放屏幕中央，**只动 OBJ 区域**（已调研确认）：
  - 手牌区（OBJ）下移、小丑区（OBJ）上移、消耗牌区（OBJ）上移
  - 被选中小丑 tx/ty 动画弹向中央，松开弹回
  - **按钮区、槽位阴影是 BG tilemap，不能移动**——被描述框覆盖即可（商店的逐行拷贝 `main_bg_se_move_rect_1_tile_vert` 方案不适用于对局内零散布局）
- 参考：商店 `GAME_SHOP_SHOW_CARD_DESC` 状态机（shop.c ~606）+ NinePatchRect 描述框

## 🐛 JOLLY_JOKER_ID 与 Faceless Joker 冲突核查

**来源**：2026-08-03 提交 Loyalty Card 时发现。

- `include/joker.h:100` 定义 `JOLLY_JOKER_ID 59`，但注册表里 ID 59 是 **Faceless Joker**（原版 Jolly Joker 实际是 ID 33）
- round.c 用 `JOLLY_JOKER_ID` 判断弃牌相关逻辑，可能指向错误的 joker
- 需核查 round.c / hand.c 中引用点，确认逻辑是否受影响

## 🔲 其它候选（未细化）

- 星球牌/塔罗牌：共用 1-2 套调色板，预留 Sheet 21（用户暂缓）。**购买判定必须走 `shop_can_afford()`**（shop.c，M20 后已收敛购买+reroll 入口）——消耗品/礼包实装时不要自己写 money 检查，否则信用卡欠款额度会漏（M20 的教训）
- 调试开关扩展：开局金币、无限出牌等（`DEBUG_SHOP_FREE` 同款模式，`include/game.h`）
- **科学计数法/分数系统扩展（2026-08-17 探索定稿，低优先）**：科学计数法需求 = **无尽模式需求的充要条件**——正常模式（24 轮/底注 8，最终 boss 100k）下 `u32`（上限 4.29e9）+ 后缀截断（`truncate_uint_to_suffixed_str()`，util.c:116，K/M/B 式显示）完全够，**零改造**。若将来上无尽模式：u64（1.8e19）撑不到底注 16（需求 8.6e20，差 ~47 倍，彩注叠加更不够）→ **直接上 double（原版同款架构）**，GBA 软浮点（加减 ~50 周期/乘 ~100 周期）在结算/显示低频路径无性能压力，`%e` 走已 override 的 vsnprintf 链（newlib 软浮点支持）；**跳过 u64 过渡**（省一轮全量类型贯穿回归）。显示层预留后门：`truncate_uint_to_suffixed_str()` 的 u32 签名将来需放宽为 double/u64。无尽模式同时附带：无限底注表 + 彩注系统 + Boss 轮换 + 续档，属大特性，当前主线（小丑实装）优先。

## 🖥️ 同屏卡数上限 + 滚动列表方案（2026-08-07 用户定稿）

**目标**：控制同屏精灵/调色板数量，腾出资源给更多显示效果（贴纸/封蜡/膜）。

**方案**：
- 同屏最多显示**当前上限 + 2~3 张**，再多进入**滚动列表**显示
- 富余的 OBJ 精灵（硬上限 128）和调色板（16 bank）用于新效果
- 贴纸/封蜡/膜 = 每卡一个状态覆盖层精灵 + 专属调色板，现资源超载做不了，滚动方案腾出后可做

**实现要点**（开工时）：
- 滚动列表用**箭头指示器** + 切换键（L 已占用为售出键，需规划按键方案）
- 滚动时**精灵池化复用**（滑动中只更新位置/层级，不重建）
- 需兼容现有 `SelectionGrid` 点选/详情交互机制
- 触发时机：手牌/小丑数超过同屏上限时自动启用

---

## 已归档决策（防遗忘）

- **效果函数架构红线**：`s_shared_joker_effect` 是单缓冲——效果函数必须**同步**消费 `joker_effect` 并立即返回，禁止保存指针供后续延迟使用（否则同帧多 joker 触发会串数据）
- **描述文本长度纪律**：描述框硬限制 ≈7 行 × 16 字符/行。wiki 原文超长时（>80 字符）自主精简句式（保留触发时机/目标/数值语义），动态数字部分用 `(now +N)` 结尾。参考：巴士/匕首已按此风格精简
- **计数器生命周期**：所有自增/自减计数器（Wee chips、巴士 mult、积分卡剩余手数等）= **对局临时状态**——每局从初始值开始、卖掉重买归初始、不视为跨局数据（存档字段即使写入也不具跨局语义；若未来做严格语义，可在 load_game 时按 joker ID 重置计数器）
- **gfx0 调色板**：16 色（含透明），非透明 14 色（3F6367 已并入 4F6367）。未来新小丑量化时注意：低频色（#BB4D46/#BC8019/#347EB2/#FD8086）是优先合并候选
- **精灵表索引规则**：`def_joker_gfx_table.h` 条目数必须与 sheet 编号 0..N 连续一一对应（删除占位会致数组前移、贴图错乱）
- **构建流程**：Docker Desktop 需手动启动（~15s）；`rm -rf build` 后必须重跑 `python3 scripts/generate_font.py`
- **ROM 命名**：`GBAlatro_yymmddhhmm[_DEBUG].gba`，上传百度云 `/gbalatro/`（bypy，应用数据目录）
- **地图映射现状**（2026-08-03，已交付 12 张自定义）：53=0/20, 54=0/21, 55=0/18, 56=0/19, 57=19/0, 58=0/23, 59=0/22, 60=19/1, 61=19/2, **62=0/25**（花盆已从 sheet 20 迁入 gfx0）, 63=0/24, **64=8/2**（巴士入 gfx8）。gfx20 保留为孤儿表（索引对齐）
