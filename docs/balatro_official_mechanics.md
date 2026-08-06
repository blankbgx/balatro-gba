# Balatro 官方机制定义（fandom Jokers 总览页提取）

> 来源：`Jokers _ Balatro Wiki _ Fandom.html`（用户 2026-08-06 扒取，放在项目根目录）
> 用途：新卡实现 / 机制对齐参考

## 📊 全卡数据

- 共 **150 张小丑**（update 1.0.1o-FULL），105 张开局解锁 + 45 张条件解锁
- 品质分布：**Common 61 / Uncommon 64 / Rare 20 / Legendary 5**
- 随机生成概率：Common 70% / Uncommon 25% / Rare 5%
- Legendary 只能从 The Soul 幻灵卡获得

## ✨ Editions（版本卡）

| Edition | 效果 | 商店价格影响 |
|---------|------|-------------|
| Base | 标准 | — |
| Foil | **+50 Chips** | 更高 |
| Holographic | **+10 Mult** | 更高 |
| Polychrome | **×1.5 Mult** | 更高 |
| **Negative** | **+1 Joker slot** | 更高 |

> 高 stake 下还会出现 Eternal / Perishable / Rental 贴纸。
> ⚠️ **与我们的设计对齐**：Negative = +1 Joker slot（用户 2026-08-04 定稿的"不占槽位"机制得到官方确认）

## 🔑 关键术语定义（实现时逐字对齐）

### When Scored
卡被计分时触发。若牌被打出但**未计分**（不属于有效牌型 / 被 boss 削弱），**不触发**。

### Contains
牌型包含关系。例：三条"包含"对子（按三条计分时仍触发对子卡）。四张同花色组成的四条"包含"同花（触发同花卡）。**注意：四条不"包含"两对**。

### Is
"牌型是 X" = 整手牌被归类为 X，不含更高牌型。

### When Blind is Selected（⚠️ 关键）
**选择盲注时触发。跳过盲注则不触发。**
- 对齐：Riff-Raff（54）/ 仪式匕首（65）/ 窃贼（67）/ 疯狂 Madness 都是这个语义
- 项目现状：**尚无跳过盲注功能**，此边界暂不触发；将来实现 skip 时必须同步加判断

### In Deck
只算牌组剩余牌（不含手牌/打出/弃掉）。

### In Full Deck
算完整牌组（含抽过/打出/弃掉/手牌）。

### Add / Destroy / Create
- **Add**：永久加入一张牌到牌组
- **Destroy**：移除目标牌；若摧毁的是小丑，**不获得出售收益**
- **Create**：生成一张新牌

## 🎴 小丑槽位说明

- 玩家通常 **5 个小丑槽位**，某些情况下可增减
- 对应我们已知：MAX_JOKERS_HELD_SIZE=5，Negative 将来 +1

## 📋 页面局限

此页是"Jokers 总览"，**不含每张小丑的具体效果/售价数据**（那是各卡独立页面）。单卡数据仍以 `docs/balatro_jokers_reference.md`（bwiki 145 张）为准，或用户按需扒单卡页。
