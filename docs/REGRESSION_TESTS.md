# GBAlatro 回归测试文档（Regression Tests）

> **用途**：记录已修复的 bug 及其复测点，供**定期回归测试**（每次大版本、rebase、架构改动后），确保 bug 没有回归。
>
> **工作流**：每次报告 bug → 修复后，在此文档追加一条（含版本号 + 复测步骤 + 预期）。定期（如每周或每个功能里程碑后）按文档逐项复测。

---

## 📋 复测方法

- ROM：取百度云最新版（`GBAlatro_<YYMMDD><HHMM>_DEBUG.gba`），或本地 `build/workspace.gba`
- 每个用例标注 **P（P0 必测）** / **P1（重要）** / **P2（次要）**
- 复测结果：✅ 通过 / ❌ 回归（回归时立即报告）

---

## 已修复 Bug 清单（按时间倒序）

---

### M33. Swashbuckler 侠盗（79）实装：其他小丑售价总和 → 倍率（P1）— 2026-08-24, commit 7e81a52

**功能**：Swashbuckler 侠盗（orig #110，$4 Common，Act Indep）——**其他所有已持有小丑的售价总和加到倍率**（+m 结算型）。

**实现**：
- INDEPENDENT：`swashbuckler_sell_total()` 遍历 owned 累加除自己外所有 joker 的 `joker_get_sell_value()`（实时 value/2——成长的 Egg 售价也被正确吸收）
- **复制语义**：结算型 → 蓝图/脑暴复制**有效**（复制体把真身算进"其他"→ 双加）
- desc 动态显示当前售价总和（`snprintf %ld`，Flash Card 模式）
- 素材：**gfx18 扩展 32→64px（slot 1）+ 4 新色**（红 #FD5F55 / 橙 #FDB139 / 金棕 #B99C61 / 淡青 #E7FEFE）**追加在 Ancient 色块之后**——Ancient slot 0 像素索引不变（前缀零差异）；palette 11→15（16 上限内）

**复测步骤**：
1. 商店买 Swashbuckler（$4 Common）→ 卡面正常（gfx18 slot 1 海盗主题）；desc 显示当前 +N Mult（其他小丑售价总和）
2. 无其他小丑 → 无加成；带 1 张 $4 卡（售价 $2）→ +2 Mult
3. 多张混合 → 售价总和（$6 卡 = +$3、$8 卡 = +$4…）
4. **蓝图复制 Swashbuckler** → 双倍（复制体把真身算"其他"）
5. **Egg 联动**：Egg 长到 $6 售价后 → Swashbuckler 吸收 +$3（实时值）
6. **Ancient Joker 卡面不受影响**（gfx18 slot 0 像素零差异）
7. desc 花色/数字显示正常（无卡死——snprintf %ld 模式已验证）

**复测结果**：⏳ 待 Delta 实测

---

### M32. Ancient Joker 古老小丑（78）实装：当前花色计分卡逐张 ×1.5 + 花色不重复规则（P1）— 2026-08-23, commit f809c04

**功能**：Ancient Joker 古老小丑（orig #99，$8 Rare，Act On Scored）——**当前花色的计分卡逐张 ×1.5**（Baron 式，N 张匹配 = ×1.5^N）。花色回合结束变化。

**用户特殊规则（2026-08-23）**：
- **连续回合花色必不相同**——回合结束从**其余 3 花色**随机（排除当前，映射 0-2 跳当前）
- **花色与牌组无关**——即使牌组只剩一种花色，仍在 4 花色内随机（不读牌组构成）

**实现**：`persistent_state` = 当前花色（0-3）；ON_JOKER_CREATED 随机首回合花色（无排除）；ON_ROUND_END 真身滚动下一花色（排除当前，`s_is_copying_joker` guard）；ON_CARD_SCORED 匹配花色 → 分数 XMULT 通道 ×1.5（逐卡）。**复制语义**：蓝图/脑暴同步 persistent_state → 复制体同花色逐卡 ×1.5（无 guard），滚动时静默（镜像）。`RNG_SEQ_JOKER_ANCIENT` 独立序列。desc 动态显示当前花色（TTE 花色色约定：Diamond 黄/Club 深绿/Heart 红/Spade 深蓝）。素材 v2：填入空 gfx18 fallback sheet（10 色 < 16 palette，零量化损失、无 5bit 碰撞）。

**复测步骤**：
1. 商店买 Ancient Joker（$8 Rare）→ desc 显示当前花色（随机，带花色色）
2. 出牌：当前花色计分卡逐张 ×1.5（2 张 = ×1.5²）；其他花色不计
3. **回合结束花色必变**：连打多回合，观察 desc 花色每回合变化且**连续两回合不重复**
4. **牌组无关**：DEBUG 改纯单花色牌组（或卖光其他花色）→ 花色仍在 4 种间随机
5. 蓝图/脑暴复制 → 同花色双倍逐卡 ×1.5；复制体不独立滚动（镜像真身）
6. 卡面：gfx18 slot 0（米色古老主题）显示正常
7. **Smeared 联动（0fc501f）**：Ancient 花色=Hearts + Smeared → 红套卡（Heart+Diamond）都 ×1.5；黑套同理（走 `card_effective_suit_mask()`）
8. **描述界面卡死（3100790 修复）**：初版 desc 用 `snprintf` + `%s` 拼带标记短语 → 文本出现连续双标记 `#{cx:0xB000}#{cx:0xB000}` → **tonc tte_write 解析死循环**（现象：打印 "Each played" 后卡死）。修复：4 花色各一条编译期静态 desc（零 snprintf、零连续标记）。**铁律：desc/消息文本禁止连续 TTE 标记**（详见 gbalatro-joker-dev skill `references/desc-color-convention.md`）

**复测结果**：✅ Delta 实测通过（2026-08-23）——回合开始花色消息（白色，deferred 队列串行）正常播放；每次购买时随机初始花色；Smeared 联动（红/黑套互认）；无 Smeared 时精确匹配；蓝图/脑暴复制（同花色双倍逐卡、复制体静默不滚动）。描述界面卡死已修复（静态 desc 无双标记）。

---

### M31. Stuntman 特技演员（77）实装：+250 筹码可复制 / -2 手牌上限被动不可复制（P1）— 2026-08-22, commit e4d9af2

**功能**：Stuntman 特技演员（orig #136，$7 Rare，Act Indep）——**+250 Chips**（INDEPENDENT 阶段提供）+ **-2 hand size**（入手即生效的被动）。

**实现**：
- **+250 Chips = 结算型值**（INDEPENDENT 返回 FLAG_CHIPS）——蓝图/脑暴复制**有效**（每个复制体再报 +250，无 guard）
- **-2 hand size = 静默被动**（effect no-op）——蓝图/脑暴复制**无效**（静默态规则）。`count_stuntman_effects()` 只数真身（Showman 重复持有叠加 -2×n）；round.c 新增 `get_effective_hand_size()` **派生函数**（3DS demake 查询时派生模式：hand_size 保持存储默认值，4 个使用点——发牌 card_draw / 发牌流程 deal / HUD 显示 ×2——读派生值）
- 素材：gfx0 扩展 1056→1088px（slot 33），6 色精确命中 + 最大 ΔE=1.0，前 1056px 零差异

**复测步骤**：
1. 商店买 Stuntman（$7 Rare，DEBUG 免费）→ 卡面显示正常（gfx0 slot 33）；**HUD 手牌上限立即变 6/6**（8-2）
2. 出牌 → INDEPENDENT 阶段 +250 Chips（蓝色，结算型）
3. **蓝图复制 Stuntman** → +500 Chips（复制体再报 +250）；**手牌上限仍 6**（-2 不被复制）
4. Showman 构筑两张真身 → 手牌上限 4（-2×2）
5. 卖出 Stuntman → 手牌上限恢复 8（派生值实时，无残留）
6. 发牌上限同步：手牌上限 6 时只发 6 张
7. **下限保护（5b04976，用户实测修复前行为）**：修复前 4 张真身 → 上限 0 → 进入对局**软卡死不发牌**；5 张 → 上限 -2（int 有符号，非 unsigned 下溢）→ `card_draw` 的 `top >= size-1` 恒 true 提前返回 → **同样静默卡死不发牌**（未触发任何溢出路径，但依赖"不发牌"兜底而非显式保护）。修复后：4/5 张均 clamp 上限 **1**，发 1 张可正常推进

**复测结果**：⏳ 待 Delta 实测（修复前 4/5 张卡死行为已由用户确认；修复后上限 1 待验）

---

### M30. Baseball Card（76）实装：罕见小丑 ×1.5 倍率（P1）— 2026-08-21, commit 99808b9

**功能**：Baseball Card 棒球卡（orig #92，$8 Rare，Act "On Other Jokers"）——**场上每张罕见（Uncommon）小丑计分时 ×1.5**（n 张 Uncommon = ×1.5ⁿ）。参照男爵的分数 XMULT 通道（`(mult*3+1)/2` round-half-up + 溢出饱和）。

**实现**：被动 no-op + joker.c `joker_object_score` 的 INDEPENDENT 钩子——每张 Uncommon 小丑计分时（effect 调用后、NONE-return 前）入队（钩子在 NONE-return 检查**之前**——静默型 Uncommon（Smeared）也触发）。稀有度判定读注册表实际值（`get_joker_registry_entry(id)->rarity`）：蓝图/脑暴均为 **Rare**（注册表 41/52 行，wiki 验证）→ 复制体计分不触发，与 wiki 一致。**动画 follow-up（fdc45b0 → 45bad5d → f145317 → 35fc1ac）**：`baseball_anim_*` 二维串行队列——**按棒球源分轮**（列表顺序），轮内逐个 Uncommon，每对 (源, 目标) 30 帧；**每对播放时应用一个 ×1.5 并刷新 HUD**（数值随动画累乘，原版观感——不再是钩子立即全乘）；源+目标同帧 shake，目标**卡下方 8px** 弹红 "X1.5"（位置修正：原 +16px 在卡中间不可见）；**每个源只在自身轮次 shake**。**状态机门控（f145317）**：独立阶段派发完成后等 `baseball_anim_pending()` 排空才推进——动画播完数值也全部应用，PLAY_ENDING 结算值完整。

**复测步骤**：
1. 商店买 Baseball Card（$8 Rare，DEBUG 免费）+ 场上一张 Uncommon（如 Smeared 模糊小丑）→ 出牌，Smeared 计分时 mult ×1.5（显示 "X1.5"）
2. 3 张 Uncommon → mult ×1.5³（×3.375）
3. 蓝图复制 Baseball Card → 每张 Uncommon ×1.5×2（双倍）
4. 全 Common 场 → 无加成（Baseball Card 自己 Rare 不触发）
5. Common/Rare 卡计分 → 不触发；蓝图/脑暴（均 Rare）计分 → 不触发
6. **动画时序（关键）**：`[蓝图, 棒球, 模糊, 哭与笑, 脑暴]` 出牌 → 独立阶段动画依次：蓝图轮（模糊 shake→哭与笑 shake）→ 棒球轮（同）→ 脑暴轮（同）；每对源+目标同帧 shake，目标下方弹 "X1.5"；**每轮排干后下一源**（30 帧/对）
7. 高倍率（mult ≥ ~14 亿）→ 饱和不溢出不归零
8. 卡面：gfx9 slot 1（米白/玫红棒球构图）显示正常

**复测结果**：✅ Delta 实测通过（2026-08-21）——动画串行（源分轮、数值随动画累乘、HUD 逐步跳）、X1.5 弹窗目标下方可见、+Mult 先派发确认。**待考证（用户留记）**：原版 Baseball Card 的数值应用顺序——本地为「+N 全部先（派发）→ ×1.5 逐个（动画队列源分组）」；若原版是「每张 Uncommon 计分时 +N 后紧跟 ×1.5ⁿ（广播）」，混合 +Mult 构筑的最终值可能不同，留待对照原版视频/源码后定夺。

---

### M29. ON_PLAYED 成长动画同帧重叠：绿色小丑 + 搭乘巴士一起弹 Upgrade!（P1）— 2026-08-20, commit 888bbc0

**背景**：绿色小丑（74）/搭乘巴士（64）/方形小丑（75）都在 `JOKER_EVENT_ON_HAND_PLAYED` 成长并同步弹 "Upgrade!"——两个成长卡同帧弹消息，动画重叠。

**修复**：新增成长消息队列（joker_effects.c `growth_msg_*`）：成长数值**立即生效**（当手收益语义不变），仅消息弹窗串行——按派发顺序（左→右）每 `DEFER_DELAY`（30 帧）弹一条。`joker.c` 新增 `joker_show_message()`（消息 + 触发 shake，UNDEFINED 音效——原 MESSAGE-only 路径传的是未初始化 sfx_id）。死源防护：动画期间 joker 被卖出则整队丢弃；回合初始化清队防跨回合残留。**follow-up（3fc4efb）**：round.c `play_before_scoring_cards_update` 在 ON_HAND_PLAYED 派发完成后、进入计分前等待 `growth_msg_pending()` 排空——原版整个 On Played 阶段（含逐个升级动画）播完才开始计分；迭代器此时已到底（无重复派发），队列自清 + 死源防护，无卡死风险。

**复测步骤**：
1. 绿色小丑 + 搭乘巴士同场：出牌（无人脸）→ 两个 Upgrade! **逐个弹**（30 帧间隔，左→右），不再同帧重叠
2. 数值语义：连续 2 手后巴士 +2、绿 Joker +2（各自独立，当前手收益不变）
3. 绿色小丑弃牌 → Downgrade! 也串行弹出（0 时不弹）
4. 方形小丑打出恰好 4 张 → Upgrade! 串行弹出
5. 单成长卡场景：shake + Upgrade! 正常（无回归）
6. 消息播放中卖出该小丑 → 动画静默丢弃，无崩溃

**复测结果**：⏳ 待 Delta 实测

---

### M28. 窃贼生效动画与手数滚动解耦 + 5 复制时手数停 16（P1）— 2026-08-20, commit ac52b09

**背景**：窃贼触发时小丑 shake（deferred 队列 30 帧 beat）与手数滚动（HUD roll ~98帧/item）并行推进，视觉解耦（shake 先放完、手数后滚）。5 个窃贼复制（蓝图+窃贼+脑暴+窃贼+窃贼）各排队 2 个 HUD roll item = 10 个，超过 `HUD_ROLL_QUEUE_MAX=8`，最后的手数滚动（16→19）被丢弃，HUD 停在 16（实际 `g_game_vars.hands`=19 逻辑正确，纯显示丢失）。

**修复**：DEFER_BURGLAR fire 后不走 settle_wait（窃贼不重排 rack），改为 `s_deferred_wait_hud_roll` —— 等 HUD roll 队列排空（`hud_roll_is_active()==false`）再推进下一个窃贼，实现串行（shake → 手数滚动 → 下一个 joker）。三处入队重置点补 `s_deferred_wait_hud_roll = false` 防跨回合残留。

**复测步骤**：
1. 单窃贼：选盲注 → shake + "+3 hands!" → 手数滚动 +3 → 弃牌滚到 0，**串行**（不再 shake 先放完、手数后滚）
2. 蓝图+窃贼+脑暴+窃贼+窃贼：选盲注 → 5 个小丑逐个 shake，每个 shake 后手数滚动一次，最终手数 **19**（4+3×5），HUD 不卡在 16
3. 窃贼 + Riff-Raff 混搭：Riff-Raff 生成动画与窃贼手数滚动互不干扰

**复测结果**：✅ Delta 实测（2026-08-20）—— 单窃贼、5 复制（蓝图+窃贼+脑暴+窃贼+窃贼，终值 19）、窃贼+Riff-Raff 等各类混搭动画均完全串行，手数滚动与 shake 一一对应，不再解耦或丢滚动。

---

### M27. 幽灵卡牌：大麦克消亡 double-free 越界写破坏 s_deck 的 Card* 指针（P0）— 2026-08-20, commit bb8a8f2

**背景**：幽灵卡（错误筹码/贴图/触发错误小丑）根因。存档取证定位野指针 `0x030000d2`（红桃7 合法指针 `0x030031d2` 第二字节清零），用户 Delta 实测（`PTR X0@30000bee`） + git blame 定位根因链：

1. `expire_all_gros_michel()`（`game.c:907`，用户代码 `9e73e3e4`，"全局灭绝"规则）把每个 Gros Michel push 进 expired 列表
2. `joker_object_score` 的 EXPIRE 通用处理（`joker.c:686`，上游 Geralt）又把当前 joker push 一次 → **同一 JokerObject 在 expired 列表出现两次**
3. `expired_jokers_update_loop`（`game.c:313`）对重复节点执行两次 `remove_owned_joker` + `joker_object_destroy` → **double-free**
4. 第二次 `joker_object_destroy` 读已释放 sprite → `sprite_get_layer` 返回垃圾 → `s_used_layers[layer]=false` 越界写
5. `s_used_layers`（0x030024c0）紧贴 `s_deck`（0x030023f0~0x030024c0），负向越界写清零 `s_deck[k]` 某个 Card* 的 byte1 → 野指针 `0x030000d2`/`0x03000bee` 等
6. 坏指针随"发牌→手牌→弃牌→回收"流转 = 幽灵卡（错误筹码/贴图）

**修复**：
- `list.c`/`list.h` 新增 `list_contains()`
- `joker.c:686` EXPIRE push 前 `if (!list_contains(get_expired_jokers_list(), joker_object))` 去重

**复测步骤**：
1. 持有大麦克（Gros Michel）+ 其他小丑，反复打回合直到大麦克消亡（1/6 概率，ON_ROUND_END）→ EXTINCT! 动画正常、小丑正常移除
2. 大麦克消亡后继续打 5+ 回合 → **不应出现幽灵卡**（错误筹码/杂乱贴图/触发错误小丑）
3. 持有多个大麦克（蓝印刷 Showman 凑多张）→ 一张消亡时全部一起消亡，**无重复销毁/越界写**
4. 消亡后正常出牌/弃牌/洗牌/抽牌，牌面筹码/贴图/小丑触发全部正常

**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测：大麦克消亡后多轮大量出牌/弃牌，**未再刷出幽灵牌**）

---

### M26. 小小丑筹码在结算末尾才提供 + 搭乘公交倍率与升级动画耦合（P2）— 2026-08-20, commit ac40d24

**背景**：两处的 independent 阶段时机偏离原版相位表（docs/balatro_wiki_summary.md:160/344）：
1. 小小丑（Wee）累计筹码在 `ON_HAND_SCORED_END`（所有小丑之后）才加进总分——视觉上在大麦克 +15 之后才弹，打乱槽位从左到右顺序。原版：On-Scored 阶段成长（每个 2 计 +8，弹 Upgrade!），Independent 阶段在**自己槽位**把累计筹码加进总分
2. 搭乘公交（Ride the Bus）的成长（+1 与 Upgrade! 弹窗）和倍率提供捆绑在 Independent 同一帧。原版：成长在 **On Played**（出牌瞬间，打出无计分人头牌即升），Independent 只按槽位顺序弹倍率

**修复**：
- 小小丑：筹码应用从 `ON_HAND_SCORED_END` 移到 `ON_INDEPENDENT`（复制语义不变：蓝图镜像原值）
- 搭乘公交：成长判断移到 `ON_HAND_PLAYED`——直接扫描已计算好的计牌区，无计分人脸牌则 +1 并弹 Upgrade!（此时计分选择已确定，`card_object_is_scoring` 可用）；`ON_INDEPENDENT` 只弹倍率；删除不再需要的 `persistent_state` 标记位

**数值不变性**：当前手牌仍享受新 +1（成长发生在计分前）；修复前后总分一致。

**复测步骤**：
1. 构筑 搭乘巴士/小小丑/奇数托德/蓝图/大麦克：打出含 2 的牌型 → 2 计分时弹 Upgrade!（成长），小丑遍历到小小丑槽位时**就地**弹蓝色筹码（不再等到大麦克之后）
2. 搭乘公交：出牌（无人头）瞬间弹 Upgrade!，小丑遍历到它时只弹红色 +N 倍率
3. 有人头牌计分时：人脸牌计分瞬间弹 Reset!，当手巴士 +0
4. 蓝图复制小小丑/巴士：镜像数值不变

**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测：小小丑与搭乘公交生效时机正确）

### M25. 通关后卡死/崩溃：弃牌音效音高计数器跨回合泄漏致 rate 变负（P0）— 2026-08-19

**背景**（无头复现+全链路日志定位）：8 底注通关后、手牌归入牌组即将结束时卡死（Delta 表现为冻结；mGBA 抓到 `Jumped to invalid address: 01407CB0`）。根因链：
1. `s_cards_discarded` 被复用为收牌飞回音效的音高步进计数器，但其复位代码在 HAND_SHUFFLING 下**永远不可达**（代码里甚至有注释承认）→ 计数器跨回合累积
2. 约 3-4 回合后 `rate = 1024 - 64*N` 变负；`mm_word` 是 u32，负数回绕成 ~43 亿
3. maxmod 收到荒谬 rate → 野跳转崩溃（无头复现固定在第 5 回合开头，构筑无关——与用户构筑 搭乘巴士/奇数托德/抽象/蓝图/卡文迪什 复现一致）

**修复**（两处）：
- round.c 回合初始化处复位 `s_cards_discarded = 0`（与 `s_cards_drawn` 并列）
- audio_utils.c `play_sfx` 防御性钳制：`rate <= 0` 时回退到 `MM_BASE_PITCH_RATE`

**复测步骤**：
1. 正常对局连续打 5+ 回合（每回合留牌不打出，让它收牌飞回）→ 不应崩溃/卡死
2. 通关 8 底注 → 胜利画面正常出现（用户原场景）
3. 听感回归：弃牌/收牌音效音高应每回合从同一基准步进，不再持续走低

**复测结果**：✅ 无头验证（修复前固定在第 5 回合崩溃、rate 出现负值；修复后跑到 ante 4+ 无崩溃、rate 无负值）；✅ Delta 实测（2026-08-20，用户 `GBAlatro_2608192221_DEBUG.gba` 通关 8 底注后**结算画面正常出现、无崩溃/卡死**）——M25 原复现场景验证通过。⏳ **长期观察中**：防计数器类泄漏复发，关注连续多局通关稳定性与弃牌/收牌音高是否随回合数走低

### M24. 窃贼回合开始动画：数字乱跳 + 发牌后数字仍变动 + 蓝图时弃牌卡"1"（P1）— 2026-08-18

**背景**（无头复现+逐帧采样实锤，蓝图+窃贼构筑）三个叠加根因：
1. **数字乱跳**：HUD roll 队列 HOLD 阶段的收尾调用 `display_hands()/display_discards()` 打印的是**实时值**而非该项的目标值——后续触发（蓝图复制）已把实时值推进，导致手数可见序列 4→5→6→**10**→7→8→9→10，非单调跳变
2. **弃牌卡"1"长达 ~230 帧**：roll 队列的 `next_tick_at` 调度在 `g_game_vars.timer` 上，而 round.c 在 ~10 处状态转换点**重置**该 timer（发牌完成等）——时间戳被孤立，队列停摆直到 timer 重新爬上来
3. **发牌后数字仍变动**：发牌（HAND_DRAW）不等盲注特效/滚动队列排空就开始了

**修复**：
- 新增**单调 UI 时基** `s_ui_tick`（game.c，每帧自增、永不重置，导出 `game_get_ui_tick()`）；HUD roll 队列、joker 事件文本清除、deferred 队列三处调度全部改用它
- HOLD 收尾改为打印 `item->to`（该项自己的目标值），串行幻象逐项闭合
- 发牌门禁：`deferred_effects_pending() || hud_roll_is_active()` 时不出牌（round.c HAND_DRAW 分支）

**复测步骤**：
1. 持窃贼进回合：手数 4→7 平滑滚动、弃牌 4→0 平滑滚动，**数字严格单调**，发牌在动画全部结束后才开始
2. 蓝图+窃贼：手数 4→7→10 两段滚动、弃牌仅滚动一次（第二实例 0→0 跳过），无卡"1"滞留
3. 无窃贼/无特效的普通回合：发牌立即开始（门禁空转，零影响）
4. Riff-Raff/仪式匕首的 deferred 动画节奏不变（deferred 队列节拍已换单调时基）

**复测结果**：✅ 无头复现已验证全部 1-3（逐帧采样：数字单调、队列不stall、发牌在 f639 即动画排干后才开始）；待用户在 Delta 上确认观感

**后续修正（同日，commit 紧随 M24）**：+3 / -N 标签覆写原打在数值**上方一行**，恰与面板背景图里烘焙的白色 hands/discards 标签重叠（白上白看不清）。改为**覆写数值自身区域**。复测：回合开始 +3/-4 短暂替换数值本身 → 原地滚动 → 上方白色 caption 始终干净。
**再修正（同日第二版）**：①标签用**白色**——红色 "-4" 在弃牌位看起来像弃牌变负数；②修"+3 不显示、手数原地闪一下 4 再递增"：`hud_enqueue_value_roll` 激活队列时只设字段没走 `hud_roll_start_next_item()`（标签打印在其中），第一项的标签阶段从未执行，直接进 ROLL 擦值→`last_shown=-1` 强制重印旧值→递增，即原地闪烁。现 enqueue 统一经 `start_next_item` 启动。

### M23. 飞溅（Splash）结算后非牌型杂牌保持上浮不归位（P1）— 2026-08-18, commit b37e00f

**背景**：飞溅使所有打出的牌参与记分。出牌结算的**上浮**阶段（`play_starting_played_cards_update`）用 `card_object_is_scoring`（飞溅感知），但**归位**阶段（`play_ending_played_cards_update`）用 `card_object_is_selected`——未被选中的杂牌上浮后永远收不到归位目标，保持漂浮直到被推出屏幕。

**修复**：归位阶段两处判定改用 `card_object_is_scoring`，与上浮阶段对称。出牌途中飞溅不会被卖出，起止集合一致，安全。

**复测步骤**：
1. 持飞溅，打出一对+3 张杂牌 → 结算时 5 张全部上浮参与记分 → 结算结束**全部**下沉归位，再依次飞出
2. 无飞溅打同一牌型 → 只有对子两张上浮/下沉（回归项）
3. 高牌（High Card）+飞溅：只有最大牌是牌型卡，其余 4 张杂牌也须归位

**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M22. 模糊小丑不使特定花色需求小丑生效（P1）— 2026-08-18

**背景**：模糊小丑（Smeared）的花色 bitmask（`card_effective_suit_mask`）只应用于 hand.c 的牌型检测/花色统计；花色小丑（贪婪♦/欲望♥/愤怒♠/暴食♣）共用的 `sinful_joker_effect` 直接比较 `card->suit == 目标花色`，绕开 mask——打出梅花时愤怒小丑（♠）不触发。

**修复**：`sinful_joker_effect` 改走 `card_effective_suit_mask()`（hand.c 导出为全局，hand.h 声明）。原版行为：模糊小丑在场时一张黑牌**同时**触发愤怒和暴食（红牌同理触发欲望+贪婪）。黑板小丑（全黑 X3）走字面值检查不变——mask 与字面值对它结果一致。

**复测步骤**：
1. 持模糊小丑+愤怒小丑，打出含梅花的记分牌型 → 梅花卡触发愤怒 +3 倍率
2. 愤怒+暴食同时在场，打出黑桃/梅花 → **两个都触发**
3. 红牌侧：模糊+欲望（♥）时打方块触发，模糊+贪婪（♦）时打红桃触发
4. 无模糊小丑：花色小丑只认真实花色（回归项）
5. 黑板小丑（全黑手牌 X3）：红桃在手仍不生效（行为不变）

**复测结果**：✅ 全部通过（2026-08-18 用户实测，含黑牌双触发）

### M21. 进入回合时手数/弃牌数闪烁一帧（P1）— 2026-08-17

**背景**：回合 on_init 的 `display_hands()/display_discards()`（1878fe1 为窃贼滚动引入）在**值未变化**时也执行"擦除→重印"。转场帧 update 负载重，擦除→打印间隔（vsnprintf + 字形 ROM 读取）可跨过 y104-112 文本行的扫描输出，扫描线级精确的模拟器（用户实测 **Delta**/手机）上表现为数字消失一帧。mGBA 帧末内存采样不可见（采样时打印已完成）；v0.2.1 无此重印所以无闪烁。用户逐帧观察实锤：最小分数打印的同帧手数消失、下一帧随手牌数 0/8 一起出现。

**修复**：on_init 改用 `display_hands_no_erase()/display_discards_no_erase()`——回合入场时屏幕上的值要么已与待印值相同（第 1 局），要么区域本为空白（商店路径），覆写字形像素即可，无空白窗口。其他调用点（出牌/弃牌等值真变场景）保留擦除。窃贼滚动的 1878fe1 时序不变（仍在 ON_BLIND_SELECTED 派发前绘制）。

**复测步骤**：
1. **Delta 逐帧看**：选盲注→进牌局，手数/弃牌数数字全程不消失（重点：最小分数打印的那一帧）
2. 第 1 局和第 2 局以后（商店路径）都要看
3. 持窃贼进回合：手数 4→7 滚动动画正常（标签 "+3" + 逐帧滚动），无额外闪烁
4. 出牌/弃牌时手数/弃牌数递减显示正常（擦除路径未动）

**复测结果**：✅ 通过（2026-08-18 用户 Delta 实测，进回合数字全程不再消失）

### M20. 信用卡欠款额度不覆盖 reroll（P1）— 2026-08-17

**背景**：信用卡（Credit Card, ID 66）购买路径允许欠款（`money + CREDIT_CARD_DEBT_LIMIT*张数 >= price`，shop.c），但 reroll 按钮的可用判定 `reroll_can_be_pressed()` 只查 `money >= s_reroll_cost`——欠款刷不了商店。原版规则：任何商店消费都可欠款。

**修复**：reroll 判定加上 `CREDIT_CARD_DEBT_LIMIT * count_credit_card_effects()`，与购买路径同式。`money` 为 s32、`display_money` 用 `%ld`，负数显示天然安全。

**复测步骤**：
1. 持有信用卡，把钱花到不足 reroll 费（如 $3 vs reroll $5）→ 按钮**可用**，按下后余额变负（$-2）
2. 欠款抵近上限（余额 - 20×张数 < cost）→ 按钮禁用
3. 无信用卡且余额不足 → 按钮禁用（行为不变）
4. 双信用卡（Showman 重复获取）→ 欠款上限 $40 生效
5. `DEBUG_SHOP_FREE=1` 构建：按钮始终可用、不扣钱（行为不变）

**复测结果**：✅ 主路径通过（2026-08-18 用户实测：单卡额度内欠款 reroll 可用、超出额度按钮正确失效）；4 待 Showman 构筑顺带验证

### M19. 商店按住 B 移动焦点查看描述导致小丑位移累积下沉（P1）— 2026-08-17

**背景**：在 Next Round/Reroll 按钮上**按住 B** 再移动焦点到待售小丑：焦点上升只写入 `ty`（目标值），当帧 `y` 未动且 `vy==0`，desc 进入的"静止守卫"（查速度不查位置）被穿透；desc 捕获**瞬时位置** `y` 作为归位基准，丢弃了排队中的上升量。焦点移开时 unfocus 执行 +RAISE 下沉而无对应上升，每循环净下沉一档，**可叠加**。查看另一张小丑时 desc 流程会把其他商店小丑 `ty` 重置为 `ITEM_SHOP_Y`，漂移被冲掉（表现为"复位"），但 focused 标志仍 true，再次操作立即复现。上游原有 bug，与 M18 无关。

**修复**：归位基准改为捕获**目标位置** `tx/ty`（shop.c `game_shop_process_user_input`）——焦点上升已含在目标值内，任意帧捕获均自洽。

**复测步骤**：
1. 商店焦点在 Next Round（或 Reroll）按钮上，**按住 B** 同时移动焦点到待售小丑 → 描述弹出 → 松开 B → 小丑回到货架位，**不下沉**
2. 上述操作连续重复 5 次 → 位置无累积偏移
3. 再查看另一张小丑 → 两张都归位正常
4. 正常流程（先聚焦小丑、停稳后再按 B）→ 描述显示/归位正常
5. 顶行已持有的小丑按 B 查看 → 归位正常

**复测结果**：✅ 通过（2026-08-17 用户实测，原复现路径不再下沉）

### M18. 仿射矩阵动态借用/归还（affine pooling）（P1）— 2026-08-17, commit 3fc5a4f

**背景**：架构改进。此前每个仿射精灵在 `sprite_new` 时永久占用一个硬件仿射矩阵（共 32 个），空闲精灵 99% 时间持有但不使用。改为动态借用：精灵创建时不分配矩阵，首次需要缩放/旋转时惰性借出（`sprite_checkout_affine`），回到静止状态（scale==1, rotation==0, 无速度）后归还（`sprite_release_affine`）。矩阵池耗尽时降级：位置弹簧正常更新，但缩放/旋转瞬移到目标值。

**复测步骤**：
1. 出牌计分：每张记分卡有缩放+旋转 hit 动画（与改动前一致），小丑触发时抖动
2. 动画结束归位后无残影、无跳闪、无位置偏移
3. 小丑自毁（如大麦克 EXTINCT!）的 shake→移除动画正常
4. 主菜单 A 牌持续旋转动画正常
5. 连续多手牌 + 多小丑连触发（如 Blueprint 链）动画不丢失

**复测结果**：1/2/4 ✅ 通过（2026-08-17 用户实测：计分动画手感与改前一致、归位无跳闪、主菜单 A 牌正常）；3/5 待特定构筑顺带验证

### M17. add_joker 满架防御丢弃泄漏对象池（P2）— 2026-08-15

**背景**：`add_joker()` 的满架防御分支直接 `return`，但调用方（Riff-Raff 延迟生成）已完成 `joker_new` + `joker_object_new` 分配——对象被丢弃且未销毁，内存池槽位泄漏。

**修复**：满架分支改为 `joker_object_destroy(&joker_object)` 后再返回。

**复测步骤**：
1. 持有 5 张小丑（满架）+ Riff-Raff（靠 Showman 或调试手段达成）
2. 选盲注触发 Riff-Raff → 无空位，应**静默跳过**（无动画、无崩溃）
3. 连续多回合重复触发，之后卖出小丑再购买 → 商店/生成流程正常（池未耗尽）
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M16. 大麦克/卡文迪什自毁时机对齐原版（每手→每回合）（P1）— 2026-08-15

**背景**：自毁判定挂在 `ON_HAND_SCORED_END`（每手结束 1/6、1/1000），原版为**每回合结束**一次（fandom #38/#61）。每手触发让大麦克实际存活期望缩短 4-5 倍。游戏内描述也误写为 "end of hand"。

**修复**：两卡自毁移至 `ON_ROUND_END`；描述改 "end of round"；输局（LOSE）不再派发 ON_ROUND_END（结算/自毁对败局无意义）。

**复测步骤**：
1. 持有大麦克打完一个回合（多手牌）→ 手牌结束**不应**出现 EXTINCT!；回合结束结算时才有概率自毁
2. 自毁触发时：EXTINCT! 消息 + 全场大麦克一起灭绝（全局灭绝规则不变）；之后商店不再刷出大麦克、改刷卡文迪什
3. 卡文迪什同理（1/1000，可用调试 RNG 验证）
4. 输局：游戏结束画面不应出现 Egg "+$3" / EXTINCT! 等回合结束消息
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M15. Splash 实装（所有打出牌计分）（P1）— 2026-08-13

**功能**：Splash（ID 72，orig #52，$3 Common，卡面 gfx5 slot 4）。效果：**所有打出的牌都计分**（含通常无牌型匹配的杂牌）。被动实现于 `card_object_is_scoring()`（`|| is_joker_owned(SPLASH_JOKER_ID)`），round.c 记分循环 / 牌型判定 / Ride the Bus 连击检查自动同步。

**复测步骤**：
1. 商店购买 Splash（$3）放入小丑栏
2. 打出一手**混合杂牌**（无对子/顺子/同花等匹配的 5 张散牌）
3. **预期**：5 张牌全部出现计分动画（+n 飘字 + 抖动）并贡献筹码，**逐张节奏一致无多余停顿**（修复 1d89e59：显示闸门 is_selected→is_scoring，此前杂牌白等 0.5s/张无动画）
4. 单独打出 1 张 A 或人头牌（无牌型）：Splash 下仍计分
5. **Ride the Bus 共存**：持有 Splash + Ride the Bus，打出含脸牌的手——脸牌计分 → Ride the Bus 连击重置（与无 Splash 行为一致）；打无脸牌手 → 连击 +1
6. 蓝图/脑暴复制 Splash：效果不变（布尔被动，复制无额外影响，无崩溃）
7. 商店稀有度/价格：Common $3；描述文本 "Every played card counts as scoring"
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M14. 窃贼手数/弃牌 HUD 动画（P2）— 2026-08-07 起，2026-08-10 定型

**⚠️ M14d 定型（2026-08-10，commits f2177bc / 1878fe1 / 3fdb6d5 / b2631f9）——用户连续三轮反馈后的最终形态**：
1. **数字滚动 → 静态渐变**：原"方向滚动"（dy 垂直滑动，增从下/减从上滑入）在 60fps 下帧数不足显得跳变。改为**原地渐变**（`draw_rect->top` 固定，`cur = from + delta*step/24`），`HUD_ROLL_STEPS` 12→24 让递进更细腻。
2. **开局手数"跳闪"（display 时机）**：`display_hands()` 原本在 ON_BLIND_SELECTED 派发**之后**调用——Burglar 已 enqueue（from=旧值），重绘显示真实值后动画又从旧值渐变 → 数字 7→4→7 跳闪。修复：**HUD 显示移到派发之前**（round.c `game_round_on_init`），动画从旧值自然渐变。
3. **同值重绘闪烁**：渐变前段 cur 未变（如 4→7 前 8 步都是 4）仍每步 erase+redraw → 数字原地闪。修复：**`last_shown` 字段，cur 未变跳过重绘**。
4. **label 覆盖闪烁**："+3" 白字原本画在数字位置（先擦数字）→ 4→+3→4 闪变。修复：**label 上移一行**（数字上方 y-8，数字永不消失）。
5. **节奏感**：渐变到目标后**保持 30 帧**（`HUD_ROLL_END_HOLD`）再收尾——原实现渐变结束立即恢复（戛然而止）。完整节奏：label 20f → 渐变 48f → 保持 30f。

**复测步骤（更新）**：
1. 持有窃贼 → 选盲注 → 手数上方浮白色 "+3"（20 帧）→ 数字**原地渐变** 4→5→6→7（无滑动、无闪跳）→ 保持 7 约 30 帧后稳定
2. 弃牌数同样渐变到 0（"-N" 标签）
3. 无窃贼对局：开局手数正常显示，**无任何闪烁**（M14d 修复的独立现象）
4. 蓝图/脑暴复制窃贼（多个实例逐个触发）→ 每个实例触发时都渐变一次
5. 回归：普通打出/弃牌时 HUD 数字正常（无残留动画）

**边界**：动画期间若玩家快速操作，`display_*` 可能覆盖动画——下次触发时重置，无累积。

**背景**：Burglar 触发时 hands/discards HUD 是"白↔本色闪烁"（~1.2s），用户希望像筹码结算那样**数字滚动**（从旧值逐级跳到新值）。

**修复（M14b，commit 5e8fd47 后热修）**：通用队列首次实现用 `item->draw_rect == &HANDS_TEXT_RECT` 指针比较决定滚动后恢复哪个 display 函数——但 `HANDS_TEXT_RECT` 是 layout.h 的 **static const Rect（每个 include 它的 .c 各一份独立副本）**，joker_effects.c 传的 `&HANDS_TEXT_RECT` 地址 ≠ game.c 里的地址 → 比较恒 false → 滚动结束后 `display_hands()/display_discards()` 永不调用 → **手数/弃牌数字被擦除后不恢复（用户："手数和筹码倍率区域完全没有数字了"）**。修复：改为 **enum target**（`HUD_TARGET_HANDS`/`HUD_TARGET_DISCARDS`，定义在 include/game.h），`hud_enqueue_value_roll` 加 target 参数。⚠️ 通用教训：**跨翻译单元共享的 static const 对象绝不能靠地址比较区分——每个 TU 有独立副本**；需要区分时用枚举/ID。⚠️ 另：game.h 枚举的写法是 `enum { ... };`（匿名枚举），joker_effects.c 无需 include layout.h 即可用常量。

**2026-08-08 三次调整（原版动效参考 + 通用化）**：用户观察原版窃贼触发：先在手数上覆盖白色 "+3"，手数**向上**翻动到目标，**然后**弃牌数才翻动（**串行序列**）；若目标值==当前值则不翻动。重构为**通用 HUD 值滚动队列**（`hud_enqueue_value_roll(erase_rect, draw_rect, label_rect, target, color_pb, label, from, to)` + `hud_clear_value_roll_queue()`，game.h 导出）：每项 = 白色 label 覆盖（HUD_ROLL_LABEL_HOLD 20 帧）→ 方向滚动（12 步 × 2 帧，增向上/减向下，本色）→ 下一项；`from == to` 整项跳过。rect 常量（HANDS/DISCARDS_TEXT_RECT + ERASE/ROLL_ERASE）**移入 include/layout.h**（含 `#include "util.h"` 供 UNDEFINED），供 joker_effects.c 的 DEFER_BURGLAR fire 分支调用。后续手数增减/牌型升级 joker 复用同一队列（换 rect+颜色+label 即可）。⚠️ `HUD_ROLL_DY 5` 随 rect 一起在 layout.h 顶部定义（rect 初始化需要）。

**⚠️ M14c 热修（commit 待，2026-08-08 用户：1948 版"选取一张牌后游戏立马卡死"）——两个 bug**：
1. **悬垂 label 指针（卡死根因）**：`HudRollItem.label` 原是 `const char*`，DEFER_BURGLAR fire 分支传的是**栈上 `char discards_label[8]`**（snprintf 的 "-3"）——fire 返回后栈失效，队列在**几帧后**播弃牌项时读野指针 → GBA 无内存保护 → 随机内容被 tte_printf %s 读取 → 可能越界 → **硬卡死**（用户观察到的时间点正好是盲选入队后第一次选牌，队列播到第二项）。修复：`label` 改为 `char label[16]` **深拷贝**（`snprintf(item->label, ...)`），空串表示无 label。
2. **from==to 时队列卡死**：`hud_roll_start_next_item()` 对 `from == to` 设 `phase = HUD_ROLL_PHASE_DONE` 后 return——但 update_loop 没有 DONE 分支 → cur 永不前进、active 永 true → 队列**永久悬挂**（弃牌已为 0 时触发）。修复：`from == to` 时直接 `cur++; hud_roll_start_next_item(); return;`（跳过该项）。
⚠️ 通用教训：**延迟队列/动画队列里绝不能存调用方栈上的指针——必须深拷贝**（队列播放时机晚于调用返回）；**状态机必须覆盖所有 phase 的推进路径**（缺 DONE 分支 = 永久悬挂）。用户规则：**动画队列只用于小丑/星球等特殊效果，不得干扰正常选牌/出牌/结算路径**（本队列 active=false 时空转零开销 ✓）。

**复测步骤**：
1. 持有窃贼 → 选盲注 → 手数数字从旧值**逐级滚动**到 +3 后的值（不是闪烁）
2. 弃牌数从旧值滚到 0
3. 蓝图/脑暴复制窃贼（多个实例逐个触发）→ 每个实例触发时都滚动一次
4. 回归：普通打出/弃牌时 HUD 数字正常（无残留动画）

**边界**：滚动期间若玩家快速操作（打出/弃牌），`display_*` 可能被常规显示覆盖——滚动状态在下次触发时重置，无累积。
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M13. Showman 无法刷出重复卡（reroll 未重建 rollable 池）（P0）— 2026-08-07

**背景**：购买小丑时 `add_joker` 无条件 `joker_set_rollable(id, false)`；但 `game_shop_reroll()` 不调用 `joker_reset_rollable_jokers()`（只在进商店的 `game_shop_reset` 调用）→ 购买过的卡在后续 reroll 中保持 false，**持有 Showman 也刷不出重复卡**。

**修复**：`game_shop_reroll()` 生成新物品前调用 `joker_reset_rollable_jokers()`（与进商店一致；有 Showman 时跳过 owned 排除循环，全部卡可 roll）。

**复测步骤**：
1. 持有 Showman → 买一张任意小丑 → reroll → 该小丑可再次出现在商店
2. 持有 Showman → reroll 多次 → Showman 自己也偶尔出现
3. 无 Showman → 买卡后 reroll → 已持有卡不出现（去重仍生效）
4. 卖出卡后 reroll → 该卡可再刷出

**边界**：Cavendish 初始不可 roll 不受影响（reset 后仍 false）；Gros Michel 销毁后不入池（joker_update_food_pool 在 roll 时处理）。
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M12. Mime 链式复制计数 + "Again!" 位置轮换（P1）— 2026-08-07

**背景**：`count_mime_effects()` 和 Again 定位用单跳解析（只认"直接复制 Mime"），而运行时 `blueprint_brainstorm_joker_effect` 已支持链式复制（脑暴→蓝图→任意非被动小丑）。导致：
1. **计数错误**：`[蓝图, Mime, 脑暴, 男爵]` 脑暴→蓝图→Mime 链未计数 → 只生效 2 次（应 3 次）
2. **Again 位置错误**：每次 pass 都显示在第一个 Mime 复制体上（"被第一个发动mime效果的复制体抢走"），未按 pass 轮换

**修复**：
1. 新增导出函数 `resolve_copy_target(JokerObject*)`（joker_effects.c，复现运行时链式解析：蓝图→右邻、脑暴→最左，`brainstorm_counter < 2` 防环）——声明在 include/joker.h
2. `count_mime_effects()` 改用链式解析 → 场景3 计数 3
3. Again 定位改为收集所有 Mime 效果源（真身+链解析到 Mime 的复制体，按列表序），pass_idx 轮换取源。**PASS_IDX 计算陷阱（2026-08-07 二次修复，commit 后的 follow-up）**：`total_passes` 必须保存**初始计数**（`s_mime_total_passes`，在 `== -1` 分支赋值），不能每次 pass 重新读 `s_mime_passes_left`——重新读会让每轮都算出 `k=0`，Again 永远固定在第一个复制体（用户 1651 版复测确认"again 显示还在第一个复制体上"）。正确：`k = s_mime_total_passes - 1 - s_mime_passes_left`（递减后）。参考 Hanging Chad：它的 "Again!" 走标准 MESSAGE 链自动显示在触发它的 joker 上；Mime 是手动 tte_write 所以必须自己算位置。

**复测步骤**：
1. `[蓝图, Mime, 脑暴, 男爵]` + 手牌 K：应 3 次重触发（蓝图/真身/脑暴各驱动 1 次 pass），"Again!" 依次显示在蓝图→真身→脑暴
2. `[Mime, 脑暴, 蓝图, 男爵]` + 手牌 K："Again!" 依次在真身→脑暴（蓝图复制男爵不参与 Mime）
3. `[男爵, 脑暴, 蓝图, Mime]` + 手牌 K："Again!" 依次在蓝图→真身

**边界**：脑暴复制蓝图、蓝图右邻无卡（蓝图在末尾）→ 链解析返回 NULL，不计数；两个脑暴互指 → brainstorm_counter 防环退出。
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M11. Mime 与 Raised Fist / Reserved Parking 联动缺失（P1）— 2026-08-07

**背景**：`held_hand_has_retrigger_target()` 只检测 K/Q（覆盖 Baron/Shoot the Moon），漏掉 Raised Fist（最低点数牌触发）和 Reserved Parking（任意面牌 J/Q/K 触发）——手里有最低牌或 J 但无 K/Q 时，Mime 不重跑 held walk，这两张卡失去 Mime 联动。

**修复**：泛化目标检测——K/Q（Baron/Shoot the Moon）+ 任意面牌（Reserved Parking）+ 最低点数牌（Raised Fist，Ace 算高值与 `card_get_value` 一致）。

**复测步骤**：
1. 持有 Mime + Raised Fist，手里有 2/3/4（最低牌）无 K/Q → 打出任意牌，最低牌应显示 "Again!" 且 Mult 加 2 次
2. 持有 Mime + Reserved Parking，手里只有 J 无 K/Q → J 应被重触发（2 次 50% 出 $1 机会）
3. 回归：Mime + Baron（K）仍正常 ×2 轮

**边界**：Ace 永远算高值（Raised Fist 不把 Ace 当最低牌）；无任何 held 触发卡时 Mime 不重跑（无空 "Again!"）。
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M10. 新卡实装：Showman + Card Sharp + Burglar v2 卡面（P0）— 2026-08-07

**背景**：新增两张小丑 + 窃贼卡面更新。
- **Showman（马戏团长，ID 69）**：被动——商店/乌合之众可重复获取已持有小丑（`is_showman_joker_active` 状态轮询；`joker_reset_rollable_jokers` 和 Riff-Raff 去重跳过）
- **Card Sharp（老千小丑，ID 70）**：Independent——本回合已打牌型 ×3（用 `g_game_vars.nb_played_hands[hand_type-1] > 1` 判断）
- **大麦克特例（demake 独特设计）**：重复获取需持有 Showman（与普通卡同规则）；销毁后永不入池；**一旦灭绝，所有在场大麦克一起灭绝**（`expire_all_gros_michel`）
- 卡面：Showman/Card Sharp → gfx0 slots 29/30（用户量化后移动）；Burglar v2 → gfx0 slot 28

**复测步骤**：
1. 持有 Showman → 商店/乌合之众出现已持有的小丑（可买第二张）
2. 无 Showman → 已持有小丑正常不出现（回归）
3. 持有多个大麦克（重复获取）→ 其中 1 个灭绝 → **全部一起 EXTINCT**
4. 大麦克销毁前：商店可持续出现；销毁后：永不再出现
5. Card Sharp：先打一对（×1）→ 再打一对 → 第二次 ×3；第一手不触发
6. 窃贼卡面 v2 显示正常（shop + 对局内）

**边界**：Showman 不影响大麦克销毁后不入池规则；Card Sharp 计数每回合重置。
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M9. Mime 重触发死循环 + "Again!" 位置（P0）— 2026-08-07, commit 250f29d

**背景**：
- 死循环：`s_mime_passes_left == 0` 时重新计数 → 第 3 轮遍历后又计到 >0 → 无限重跑无法结算。修复：`-1` 标记未计数（首轮结束只计一次），重跑耗尽归 `-1` 不重计。
- "Again!" 位置：蓝图/脑暴复制 Mime 时，应显示在**复制卡**上（复制体才是重触发效果源），真身 Mime 仅无复制时兜底。

**复测步骤**：
1. 男爵 + Mime + 4K → 留手阶段跑 2 轮（×1.5⁸），**能正常结算不卡死**
2. 男爵 + 蓝图复制 Mime + 4K → 跑 3 轮（×1.5¹²），"Again!" **显示在蓝图上**
3. 男爵 + 脑暴复制 Mime + 4K → 同上，脑暴上显示
4. 仅真身 Mime（无复制）→ "Again!" 显示在 Mime 本体
5. 无 K/Q 手牌 → 不显示 "Again!"（无对象）
6. 多手牌连打：每手牌只重跑正确轮数，**无累积/无卡死**

**边界**：Mime 计数每手重置；蓝图复制非 Mime 的卡时不误显示 "Again!"。
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M1. Mime 重触发留手牌阶段（P0）— 2026-08-07, commit 31a84e1

**背景**：Mime 之前是占位实现（ON_HAND_SCORED_END 空弹 "Again!"，无实际重触发）。现按原版语义实现：留手牌遍历完整再跑 N 轮（N = Mime 效果数，含蓝图/脑暴复制体）。

**复测步骤**：
1. 男爵 + Mime + 手牌 4 张 K → 出任意手牌
2. 观察留手阶段：K 的 ×1.5 应触发 **2 轮**（原始 1 轮 + Mime 重跑 1 轮），总倍率 = 1.5⁸
3. 男爵 + **蓝图复制 Mime** + 4K → 应跑 **3 轮**（1 + 2 个 Mime）
4. Mime + 无 K/Q 手牌 → **不显示 "Again!"**（无对象不触发）
5. 蓝图/脑暴复制 Baron：每张 K ×1.5 两次（**有效**，与原版对齐——Baron/Mime 同属 ON_HELD 阶段，复制有效）

**边界**：Mime 不重触发普通小丑阶段（黑板/搭乘巴士等不受影响）。
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M2. 积分卡 + 脑暴/蓝图：同一手触发 X4（P0）— 2026-08-07, commit 1958724

**背景**：脑暴复制积分卡时，若积分卡在脑暴**左边**，真身先递减到 0、复制体读到 0 提前一回合触发。修复：递减移到 `ON_HAND_SCORED_END`，INDEPENDENT 只做触发判断 → 所有复制体同一手看到相同计数。

**复测步骤**：
1. 积分卡 + 脑暴（脑暴复制积分卡，**积分卡在脑暴左边**）→ 真身和复制体**同一手**同时弹 X4
2. 积分卡 + 蓝图（任意布局）→ 同样同步
3. 积分卡单独 → 每 6 手一次 X4 节奏不变（5→4→3→2→1→0→触发→5）
4. 触发后计数只减一次（不双倍递减）
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M3. 蓝图/脑暴复制闪卡：正确镜像倍率（P0）— 2026-08-07, commit 8053a92

**背景**：复制体 `scoring_state` 恒为 0，`>0` 判断短路导致无倍率。修复：copy 模式读**源卡**的累计值。

**复测步骤**：
1. 闪卡 + 蓝图：商店刷新（闪卡累计 mult）后出牌，蓝图显示**与真身相同的 mult 值**（非 0）
2. 闪卡 + 脑暴：同上
3. 未刷新商店直接出牌：两者均为 0（无累计，正确）
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M4. Baron 倍率（P0）— 2026-08-07, commits 48e71ed / 6ebc855

**背景**：两个根因：
- ① Baron effect 没发布 `*joker_effect`（NULL 解引用 → 显示 X1 / 倍率不生效）
- ② Joker Stencil `int→u32` 负数回绕污染全局 mult（显示 X1.5 但结算极大垃圾值）

**复测步骤**：
1. 男爵 + 手持 K + 任意手牌 → mult 实际 ×1.5（1→2→3→5...），显示红色 "X1.5"
2. 蓝图/脑暴复制男爵 → 每张 K ×2.25（×1.5 两次）
3. **Joker Stencil + 5 张 joker 满槽** → 结算 mult 不出现极大垃圾值（4.29e9）
4. 高倍率场景（mult 接近 ~14 亿）→ 不归零（饱和保护）
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M5. 窃贼 Burglar：逐个触发 + 动画（P0）— 2026-08-07, commits 0371bf8 / 8b5a925

**背景**：
- 之前无动画（返回 NONE 直接跳过 shake）→ 现走 deferred queue
- 真身 + 蓝图 + 脑暴之前同帧全触发 → 现逐个（30 帧间隔）

**复测步骤**：
1. 选盲注后：窃贼卡面震动 + 白字 "+3 hands!"
2. 手数 HUD 蓝白闪烁、弃牌 HUD 红白闪烁（~1.2s）
3. 窃贼 + 蓝图 + 脑暴同时在场：**三张卡依次**触发（不是同时），手数最终 +9
4. 效果播完才发牌（不提前发牌）
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M6. 手数/弃牌 HUD 数字残留（P1）— 2026-08-07, commit 8cd3b6e

**背景**：`tte_printf` 原地覆盖不擦除，10→9 时残留 "0" 显示 "90"。修复：先擦除固定 3 字符宽区域再写。

**复测步骤**：
1. 手数 10 → 打一手 → 显示 **9**（不是 "90"）
2. 9→8→7 正常递减，无残留
3. 手数到 0 → 正常结束回合
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M7. 对局内 L 键误售出（P1）— 2026-08-07, commit e1d4902

**背景**：`jokers_sel_row_on_key_transit` 同时注册于商店和对局内，L 键（SELL_KEY）无状态守卫。修复：仅 `GAME_STATE_SHOP` 响应。

**复测步骤**：
1. 对局内选中 joker 按 L → **不售出**，正常打牌
2. 商店内选中 joker 按 L → 正常售出
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

### M8. 消息自清理（P2）— 2026-08-07, commit 19796bd

**背景**：回合前/后、商店阶段消息无自清理 timer。修复：`set_and_shift_text` 自动挂清除。

**复测步骤**：
1. 商店刷新（闪卡 "Upgrade!"）→ 消息 90 帧后自动消失
2. 选盲注（窃贼 "+3 hands!" / 乌合动画）→ 消息自动清理
3. 对局内结算消息不受影响（不误清）

---
**复测结果**：✅ Delta 实测通过（2026-08-20，用户实测）

## 历史遗留（待确认）

- 蓝图/脑暴复制窃贼：三同触发已修（M5），复测确认无回归
- Baron 显示 X1 旧问题（M4 根因 ① 已修），复测确认

---

## 追加规范

- **新 bug**：修复后在此文档**最上方**（"已修复 Bug 清单"顶部）插入一条，格式：`### M<N>. 标题（P0/P1/P2）— 日期, commit <hash>`
- 每条含：**背景**（根因一句话）、**复测步骤**（具体操作）、**边界**（不回归的关键点）
- 修复提交时**同步更新此文档**（同一 commit 或紧随 commit）
