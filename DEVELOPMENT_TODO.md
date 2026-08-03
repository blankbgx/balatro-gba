# GBAlatro 自定义开发待办（level-up issues）

> 已完成的修复不在此列。这些是设计讨论中确定的后续工作，按优先级排列。

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

- **gfx0 调色板**：16 色（含透明），非透明 14 色（3F6367 已并入 4F6367）。未来新小丑量化时注意：低频色（#BB4D46/#BC8019/#347EB2/#FD8086）是优先合并候选
- **精灵表索引规则**：`def_joker_gfx_table.h` 条目数必须与 sheet 编号 0..N 连续一一对应（删除占位会致数组前移、贴图错乱）
- **构建流程**：Docker Desktop 需手动启动（~15s）；`rm -rf build` 后必须重跑 `python3 scripts/generate_font.py`
- **ROM 命名**：`GBAlatro_yymmddhhmm[_DEBUG].gba`，上传百度云 `/gbalatro/`（bypy，应用数据目录）
- **地图映射现状**（2026-08-03）：53=0/20, 54=0/21, 55=0/18, 56=0/19, 57=19/0, 58=0/23, 59=0/22, 60=19/1, 61=19/2, 62=20/0, 63=0/24
