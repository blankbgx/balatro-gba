# GBAlatro 自定义开发待办（level-up issues）

> 已完成的修复不在此列。这些是设计讨论中确定的后续工作，按优先级排列。

## 🎯 主线目标：完成全部小丑实装（当务之急）

当前自定义小丑已实装 **13 张（53-65）**。注册表已含全部已实现条目（0-65，无注释残留）；原版其余 joker（66+）尚未加入，需完整走：效果实现 → 精灵量化 → 注册 → 映射 → 测试。每个交付照常走：编译 → 命名时间戳 ROM → upload_rom.sh 上传。

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

- 星球牌/塔罗牌：共用 1-2 套调色板，预留 Sheet 21（用户暂缓）
- 调试开关扩展：开局金币、无限出牌等（`DEBUG_SHOP_FREE` 同款模式，`include/game.h`）

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
