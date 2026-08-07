# Balatro Wiki 要点总结（供 GBA 同人游戏开发参考）

> 来源：Baron / Activation Type / Guide: Activation Sequence / Jokers（Fandom Wiki，1.0.1o-FULL）
> 本文为给 GBA 移植版开发者的精确规则摘要，保留全部具体数值与触发顺序细节。

---

## 一、Baron（男爵）卡牌详解

### 1.1 基本效果

- **稀有度**：Rare（稀有）
- **类型**：Multiplicative Mult（倍率型 Mult）Joker；激活类型 **On Held**（留手触发）
- **效果**：打出牌局后，**手中每有一张 K（King）提供 X1.5 Mult**。
- **不触发的情况**：被弃掉（discarded）的牌、被特定 Boss Blind 削弱（debuffed）的牌不会触发该效果。

### 1.2 协同（Synergies）

**牌组（Decks）**
- **Painted Deck（彩绘牌组）**：手牌上限更大，更容易把 K 留在手中。

**牌型（Poker Hands）**
- 若牌组以 K 为主，应升级**小手牌**（如 High Card 高牌 或 Pair 对子），以便把尽可能多的 K 留在手中不打出。

**Joker 协同**
| Joker | 效果说明 |
|---|---|
| **Mime（默剧演员）** | 使每张 K 的倍率从 1.5x 提升至 2.25x（等效于翻倍以上） |
| **Reserved Parking（预定车位）** | 每张留在手中的 K（或其他面牌）有 50% 概率提供 $1 |
| **Midas Mask（迈达斯面具）** | 为被打出并计分的 K 附加 Gold 强化，此后留在手中也有额外收益 |
| **Shoot the Moon（一箭双雕）** | 与 Baron 极佳搭配：每张 Q 提供的 +13 Mult 会被 Baron 逐个 ×1.5；**注意** Q 必须先于 K 触发，必须把 Q 物理移动到 K 的左侧 |
| **Raised Fist（举起的拳头）** | 效果与 Shoot the Moon 相同（将最低点数的留手牌 Mult 放到最左被 Baron 放大） |
| **Juggler / Troubadour** | 增加手牌上限，使 Baron 触发更多次 |

**Boss Blind 协同**
- **The Serpent（巨蛇）**：最能配合 Baron 的 Boss Blind，因其能力可临时提升手牌上限。

### 1.3 反协同（Anti-Synergies）

- **Abandoned Deck（废弃牌组）**：天然不含任何 K，应避免使用 Baron。
- **The Plant（植物）Boss Blind**：被削弱的 K 使 Baron 完全失效，可能是终结一局的杀手。

### 1.4 策略与公式（全部精确数值）

Baron 强大的根源：**手中每张 K 或 Steel 牌都让分数指数级增长**。

| 情形 | 倍率 |
|---|---|
| 1 张 K | x1.5 |
| 2 张 K | x1.5² = x2.25 |
| 理想的 Baron 牌 = Steel K + Red Seal（红封） | **x1.5⁴ ≈ x5.1** |
| 上述 + Mime | x1.5⁶ ≈ x11.4 |
| + 一个 Blueprint/Brainstorm 复制 Mime | x1.5⁸ ≈ x25.6 |
| + 两个复制件分别复制 Mime 和 Baron | x1.5¹² ≈ x130 |

**满手 Steel 红封 K 时的手牌上限收益**（在 Ante 1 基准分之上）：

| 手牌上限 | 基准 Mult 放大 |
|---|---|
| 8 | ~x619 万亿（6.19×10¹⁴） |
| 9 | ~80 京（8×10¹⁶） |
| 10 | ~10 垓（1.0×10¹⁹） |

配合一定基础 Chips 与 Mult，足以清完前 16 个 Ante；用 Plasma Deck（等离子牌组）可达成传说中的 **Naneinf** 分数。

**单卡 x1.5 总次数公式**：

```
每张牌 x1.5 总次数 = (每次触发的 x1.5 个数) × (总触发次数)
```

对每一张留手的红封 Steel K，由于多个重触发叠加是**加法**关系：

```
每张红封 Steel K 的 x1.5 次数 =
    (1 个 Steel + Baron 等效数量) × (1 基础 + 1 红封 + Mime 等效数量)
```

若用 Blueprint/Brainstorm 复制这些 Joker，**最佳配置是让 Baron 等效数量比 Mime 等效数量多 1**。

**低 Ante 时期**：Shoot the Moon 或 Raised Fist 是最好的一批在 Joker 计分之前提供 Mult 的方式。因为手中卡牌**从左到右触发**，必须把 Q（或最低点数卡）物理移到 K 左边，其 Mult 才会被 Baron 放大。

### 1.5 其他

- 命名来源：Baron 与 Baroness 是贵族头衔，通常低于 King/Count，高于 Knight/Lord。
- 中文名：男爵（简/繁一致）。

---

## 二、Joker 激活类型（Activation Type）

激活（Activation）指 Joker 触发效果的**顺序**。由于激活顺序（Activation Sequence）是整体性的，不同激活类型的 Joker 之间会以非常不同的方式互动。

### 2.1 各类型定义与例子

#### On Played（出牌时）
- **定义**：打出牌时**立即**触发，发生在**任何计分之前**。多为成长型（scaling）能力。
- **例子**：Square Joker 成长、Runner 成长、Vampire 成长、Space Joker、DNA、Midas Mask。
- **注意**：会作用于**未计分**的牌。

#### On Scored（计分时）
- **定义**：对打出手牌中**每一张被计分的牌**触发。
- **例子**：Wrathful Joker、Odd Todd、The Idol、Wee Joker 成长、Dusk。
- **注意**：因为是按卡牌触发，**极度受益于卡牌重触发（retriggers）**（其他重触发 Joker 除外），但会被未计分或被 debuff 的牌阻碍。

#### On Held（留手时）
- **定义**：对手中持有的特定卡牌触发。
- **例子**：Raised Fist、Baron、Reserved Parking、Mime。
- **注意**：受益于 Mime 的重触发（Mime 自身除外），受 debuff 影响。

#### Independent（独立）
- **定义**：在所有手中卡牌计分**之后**触发。
- **例子**：Joker、Sly Joker、Steel Joker。
- **注意**：Joker 上的**版本加成（Editions）**（foil 箔面 / holographic 全息 / polychrome 彩光）也在该阶段触发。绝大多数情况下不受重触发或卡牌 debuff 影响。

#### On Other Jokers（作用于其他 Joker）
- **定义**：作用于其他特定 Joker 的触发。
- **例子**：目前仅有 **Baseball Card（棒球卡）**。

#### On Discard（弃牌时）
- **定义**：因弃掉特定卡牌或特定扑克牌型而触发。
- **分类**：
  - 依赖弃掉**扑克牌型**：Burnt Joker、Faceless Joker。
  - 依赖弃掉**特定卡牌**：Castle 成长、Mail-In Rebate。
- **注意**：受益于更多的弃牌次数，除此之外协同较少。

#### Mixed（混合）
- **定义**：属于多个上述类别。
- **例子**：Runner 既属于 On Played（成长），又属于 Independent（把 Chips 加进总分）。

#### N/A（被动 / Passive）
- **定义**：不属于以上任何类别的不计分 Joker，多为经济型（Economy）Joker（如 Golden Joker）或效果型（Effect）Joker（如 Four Fingers）。
- **注意**：被动 Joker 从不特定地"激活"，因此**不能被 Blueprint 或 Brainstorm 复制**。

### 2.2 策略要点

- On Scored 与 On Held 的 Joker **总是先于** Independent Joker 激活 → 于是 On Scored 的 xMult 与 Independent 的 +Mult 无法按最优顺序触发。
- 例：打出一手所选花色的同花时，**Droll Joker 无法享受到 Ancient Joker 的加成**。

### 2.3 优先级速查（Priority 编号即触发阶段序号）

| 优先级 | 激活类型 | 代表 Joker |
|---|---|---|
| 0 | 复制型（独立于常规激活） | Blueprint、Brainstorm |
| 1 | On Played | To Do List、Space Joker、DNA、Midas Mask |
| 2 | On Scored | 花色 +m（Greedy/Lusty/Wrathful/Gluttonous）、Fibonacci、Even Steven、Smiley Face、Onyx Agate、Scary Face(+c)、Odd Todd(+c)、Hiker(+c)、Arrowhead(+c)、Scholar(++)、Walkie Talkie(++)、Business Card(+$)、Golden Ticket(+$)、Rough Gem(+$)、Photograph(Xm)、Ancient Joker(Xm)、Bloodstone(Xm)、The Idol(Xm)、Triboulet(Xm)、8 Ball(!!)、重触发类（Seltzer、Dusk、Hack、Sock and Buskin、Hanging Chad） |
| 3 | On Held | Raised Fist(+m)、Shoot the Moon(+m)、Reserved Parking(+$)、Baron(Xm)、Mime(...) |
| 4 | Independent | 多数 +m / +c / Xm Joker、Superposition/Séance/Vagabond(!!)、Matador(+$)、Driver's License(Xm, ᛇ)、Canio(Xm) 等 |
| 5 | On Other Jokers | Baseball Card(Xm) |
| 6 | On Discard | Faceless Joker(+$)、Mail-In Rebate(+$)、Trading Card(+$)、Burnt Joker(!!) |
| 7 | N/A | 经济型（Credit Card、Egg、Cloud 9、Rocket、Golden Joker、To the Moon、Satellite…）、效果型（Four Fingers、Marble Joker、Pareidolia、Burglar、Splash、Juggler、Troubadour、Showman、Chicot、Perkeo…） |
| Mixed | 混合 | 见第四章表格 |

（注：此表对应文件 2 的完整优先级表，供定位各 Joker 在触发序列中的阶段使用。）

---

## 三、激活顺序指南（Activation Sequence）

> 这是 Balatro 的核心机制，理解它对构筑与优化至关重要。部分 Joker 的不同组成部分会在不同阶段触发——例如 **Wee Joker** 在 On Scored 阶段**获得** Chips，在 Independent 阶段把 Chips **加进总分**。

### 3.1 完整 Hand Sequence（打出牌后的激活顺序）

1. **Boss Blind 效果**：如 The Flint、The Arm 等 Boss Blind 先激活。
2. **'On Played' Joker**：打出牌时、任何计分之前激活。例：Green Joker 成长、DNA、To Do List。
3. **打出牌计分（Played cards scoring）**：被打出并计分的牌**从左到右**激活。**每张牌**内部的激活顺序为：
   a. **基础效果（Chips）**：卡牌给予对应 Chips 值；Bonus chips 计入此值。
   b. **卡牌修饰**：按顺序激活**强化（enhancements）→ 封（seals，目前只有 gold seal 金封）→ 版本（editions）**。
   c. **'On Scored' Joker**：同一张牌触发多个 Joker 时，**从左到右**激活。例：Wee Joker 成长、Smiley Face、Triboulet。
   d. **重触发（Retriggers）**：每次重触发把上面的激活序列（从基础效果到该牌相关的 On Scored Joker）**整体再执行一次**。多个重触发**加法叠加**。顺序：**红封（Red seal）最先，之后是重触发 Joker 从左到右**。
4. **留手能力（Held in hand abilities）**：手牌从左到右检查是否可激活留手能力，每张牌的序列与计分卡牌类似：
   a. **强化（Steel 钢化）**：目前 Steel 是唯一在每局中于留手阶段激活的卡牌修饰。
   b. **'On Held' Joker**：对仍留在手中的卡牌触发；同一张牌触发多个 Joker 时**从左到右**。例：Raised Fist、Shoot the Moon、Baron。
   c. **重触发**：与计分牌相同，留手牌的重触发**加法叠加**；**红封先激活，然后 Mime 及其复制 Joker 从左到右**。
5. **Joker 版本加成与 'Independent' Joker**：Joker 从左到右结算其版本（foil / holographic / polychrome）并激活 Independent 能力，顺序为：
   - **Foil（+50 Chips）或 holographic（+10 Mult）加成**。
   - **'Independent' Joker**：在所有打出牌计分之后触发的 Joker 基础能力，**不受重触发影响**。例：Fortune Teller、The Duo、Blackboard。
   - **依赖其他 Joker 的 Joker**（目前仅有 Baseball Card）。
   - **Polychrome（X1.5 Mult）加成**。
6. **消耗品（Consumables）**：购买 Observatory 天文台 Voucher 后，**星球牌（Planet cards）提供 X1.5 Mult**，从左到右激活。
7. **Plasma Deck 平衡**：最后，若使用 Plasma Deck，Chips 与 Mult 会被平衡（取平均）。

### 3.2 策略性排列：+Mult 与 xMult 的顺序

几乎所有的排列问题都源于**加法 Mult 与乘法 Mult 的顺序**。根据分配律，**+Mult 先于 xMult 激活得分更高**。

**示例**：一手基础分数 40 × 4；存在一个 +4 Mult Joker 与一个 X2 Mult 的 Ramen。
- Joker 在 Ramen 左侧：`40 × ((4+4) × 2) = 640` 分
- 反向排列：`40 × ((4×2) + 4) = 480` 分

**最佳排列规则**：

| 位置 | 规则 |
|---|---|
| Joker 槽位 | **同一激活类型内**，把加法 Mult Joker 放在乘法 Mult Joker **左侧**。例：Joker 在 Ramen 左；Lusty Joker 在 Bloodstone 左；Raised Fist 在 Baron 左 |
| 即将打出的牌 | 把提供 +Mult 的牌（Mult 卡、配合 Greedy Joker 的方块 Diamond 牌）放在提供 xMult 的牌（Glass 玻璃卡、Polychrome 彩光卡、The Idol 选中的卡）**左侧** |
| 留在手中的牌 | 同样 +Mult 在左、xMult 在右。例：带 Shoot the Moon 的 Q 放在 Steel 卡左侧 |
| 反例（刻意降分） | 若为了喂成长型 Joker 而想要更低的分数，可把 +Mult 放在 xMult **右侧** |

### 3.3 其他排列技巧

- **Midas Mask 在 Vampire 左侧**：给 Vampire 更大的成长加成；反向排列则 Vampire 成长较少，但会**把 Gold 卡留在牌组里**。

---

## 四、小丑激活类型总表（按激活类型分组）

> 数据来源：Jokers 总览表（150 个，1.0.1o-FULL）。类型符号：+c=Chips 型，+m=加法 Mult 型，Xm=乘法 Mult 型，++=Chips 与加法 Mult，!!=效果型，...=重触发型，+$=经济型。
> Blueprint / Brainstorm 在总览表中激活列留空，按 Activation Type 页归为优先级 0（复制/被动，不可被互相复制——实质为 N/A 类）。

### 4.1 On Played（出牌时）— 优先级 1

| 小丑名 | 类型 |
|---|---|
| To Do List | +$ |
| Space Joker | !! |
| DNA | !! |
| Midas Mask | !! |

### 4.2 On Scored（计分时）— 优先级 2

| 小丑名 | 类型 |
|---|---|
| Greedy Joker | +m |
| Lusty Joker | +m |
| Wrathful Joker | +m |
| Gluttonous Joker | +m |
| Fibonacci | +m |
| Even Steven | +m |
| Smiley Face | +m |
| Onyx Agate | +m |
| Scary Face | +c |
| Odd Todd | +c |
| Hiker | +c |
| Arrowhead | +c |
| Scholar | ++ |
| Walkie Talkie | ++ |
| Business Card | +$ |
| Golden Ticket | +$ |
| Rough Gem | +$ |
| 8 Ball | !! |
| Photograph | Xm |
| Ancient Joker | Xm |
| Bloodstone | Xm |
| The Idol | Xm |
| Triboulet | Xm |
| Seltzer | ...（重触发） |
| Dusk | ...（重触发） |
| Hack | ...（重触发） |
| Sock and Buskin | ...（重触发） |
| Hanging Chad | ...（重触发） |

### 4.3 On Held（留手时）— 优先级 3

| 小丑名 | 类型 |
|---|---|
| Raised Fist | +m |
| Shoot the Moon | +m |
| Reserved Parking | +$ |
| Baron | Xm |
| Mime | ...（重触发） |

### 4.4 Independent（独立）— 优先级 4

| 小丑名 | 类型 |
|---|---|
| Joker | +m |
| Jolly Joker | +m |
| Zany Joker | +m |
| Mad Joker | +m |
| Crazy Joker | +m |
| Droll Joker | +m |
| Half Joker | +m |
| Ceremonial Dagger | +m |
| Mystic Summit | +m |
| Misprint | +m |
| Abstract Joker | +m |
| Gros Michel | +m |
| Supernova | +m |
| Red Card | +m |
| Erosion | +m |
| Fortune Teller | +m |
| Flash Card | +m |
| Popcorn | +m |
| Swashbuckler | +m |
| Bootstraps | +m |
| Sly Joker | +c |
| Wily Joker | +c |
| Clever Joker | +c |
| Devious Joker | +c |
| Crafty Joker | +c |
| Banner | +c |
| Ice Cream | +c |
| Blue Joker | +c |
| Stone Joker | +c |
| Bull | +c |
| Stuntman | +c |
| Matador | +$ |
| Superposition | !! |
| Séance | !! |
| Vagabond | !! |
| Joker Stencil | Xm |
| Loyalty Card | Xm |
| Steel Joker | Xm |
| Blackboard | Xm |
| Constellation | Xm |
| Cavendish | Xm |
| Card Sharp | Xm |
| Madness | Xm |
| Hologram | Xm |
| Campfire | Xm |
| Acrobat | Xm |
| Throwback | Xm |
| Glass Joker | Xm |
| Flower Pot | Xm |
| Seeing Double | Xm |
| The Duo | Xm |
| The Trio | Xm |
| The Family | Xm |
| The Order | Xm |
| The Tribe | Xm |
| Driver's License | Xm（有触发条件 ᛇ） |
| Canio | Xm |

### 4.5 On Other Jokers（作用于其他 Joker）— 优先级 5

| 小丑名 | 类型 |
|---|---|
| Baseball Card | Xm |

### 4.6 On Discard（弃牌时）— 优先级 6

| 小丑名 | 类型 |
|---|---|
| Faceless Joker | +$ |
| Mail-In Rebate | +$ |
| Trading Card | +$ |
| Burnt Joker | !! |

### 4.7 Mixed（混合）— 多个阶段

| 小丑名 | 类型 | 阶段分布 |
|---|---|---|
| Ride the Bus | +m | On Played（成长，↑↓ 可重置）+ Independent |
| Green Joker | +m | On Played（成长）+ Independent + On Discard（-1） |
| Spare Trousers | +m | On Played（成长）+ Independent |
| Runner | +c | On Played（成长）+ Independent |
| Square Joker | +c | On Played（成长）+ Independent |
| Castle | +c | On Discard（成长）+ Independent |
| Wee Joker | +c | On Scored（成长）+ Independent |
| Vampire | Xm | On Played（成长）+ Independent |
| Obelisk | Xm | On Played（成长，↑↓）+ Independent |
| Lucky Cat | Xm | On Scored（成长）+ Independent |
| Ramen | Xm | Independent + On Discard（-0.01） |
| Hit the Road | Xm | Independent + On Discard（成长，↑↓） |
| Yorick | Xm | Independent + On Discard（成长，有触发条件 ᛇ） |

### 4.8 N/A（被动 / 经济 / 效果型）— 优先级 7

| 小丑名 | 类型 |
|---|---|
| Credit Card | +$ |
| Delayed Gratification | +$ |
| Egg | +$ |
| Cloud 9 | +$ |
| Rocket | +$ |
| Gift Card | +$ |
| To the Moon | +$ |
| Golden Joker | +$ |
| Satellite | +$ |
| Four Fingers | !! |
| Marble Joker | !! |
| Chaos the Clown | !! |
| Pareidolia | !! |
| Burglar | !! |
| Splash | !! |
| Sixth Sense | !! |
| Riff-Raff | !! |
| Shortcut | !! |
| Luchador | !! |
| Turtle Bean | !!（会自毁 ᛣ） |
| Hallucination | !! |
| Juggler | !! |
| Drunkard | !! |
| Diet Cola | !! |
| Mr. Bones | !!（有触发条件 ᛇ） |
| Troubadour | !! |
| Certificate | !! |
| Smeared Joker | !! |
| Showman | !! |
| Merry Andy | !! |
| Oops! All 6s | !! |
| Invisible Joker | !!（有触发条件 ᛇ） |
| Cartomancer | !! |
| Astronomer | !! |
| Chicot | !! |
| Perkeo | !! |
| Blueprint | !!（复制右侧 Joker；优先级 0，不可复制被动 Joker） |
| Brainstorm | !!（复制最左 Joker；优先级 0，不可复制被动 Joker） |

---

## 附录 A：总览页关键背景数据

- **总数**：150 个 Joker（1.0.1o-FULL）；105 个初始解锁，45 个需条件解锁。
- **稀有度分布**：Common 普通 61、Uncommon 稀有 64、Rare 史诗 20、Legendary 传说 5。
- **随机生成概率**：普通 70%、稀有 25%、史诗 5%；传说仅能通过 The Soul 灵魂光谱卡获得。
- **版本（Editions）**：Foil +50 Chips；Holographic +10 Mult；Polychrome X1.5 Mult；Negative +1 Joker 槽位。
- **关键术语**：
  - **When Scored（计分时）**：卡牌被计分时触发；打出但未计分（非牌型组成卡、或被 Boss Blind 削弱）则不触发。
  - **Contains（包含）**：手牌"包含"某牌型即可触发。例：三张（Three of a Kind）"包含"对子（Pair），五张同花色四条"包含"同花。四条不视为"包含"两对（Two Pair）。
  - **Is（就是）**：整手牌被归类为该牌型，且不构成更高牌型。
  - **In Deck（牌组中）**：指剩余牌组，不含手牌、已打出、已弃掉的牌。
  - **In Full Deck（全牌组）**：统计完整牌组中某属性，含抽到、打出、弃掉及在手中的全部牌。
  - **Retrigger（重触发）**：使卡牌再次计分，会激活所有计分时效果（其他 Joker、强化、版本、除红封外的封、卡牌自身 Chips）。
  - **Debuffed（被削弱）**：该 Joker 及其版本无法触发；保留完整售价（仍可被 Swashbuckler/Temperance 等利用）；仍可被随机效果（如 Madness、Wheel of Fortune）选中，但不受 Crimson Heart Boss Blind 指定。

## 附录 B：Baron 实现要点（针对 GBA 引擎）

1. Baron 属于 **On Held（优先级 3）**：必须在手牌留手阶段、按手牌从左到右、每张 K 触发一次 X1.5。
2. Steel 强化（红封 Steel K）在留手阶段触发顺序为：**Steel 强化 → On Held Joker → 重触发**；单张红封 Steel K 的基础触发 = 1（Steel）+ 1（Baron 触发）×（1 基础 + 1 红封）次触发，即 x1.5⁴。
3. Mime 及 Blueprint/Brainstorm 复制的重触发，按"红封最先，重触发 Joker 从左到右"的顺序叠加（加法叠加）。
4. 触发总数公式：`(1 Steel + Baron 等效数) × (1 基础 + 1 红封 + Mime 等效数)`；最优配比 = Baron 等效比 Mime 等效多 1。
5. 被弃掉的 K、被 Boss Blind debuff 的 K 不计入；Q（Shoot the Moon）+13 Mult 需位于 K 左侧才能被放大。
