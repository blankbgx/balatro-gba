/**
 * @file joker.h
 *
 * @brief Functions relative to the handling of Jokers
 */
#ifndef JOKER_H
#define JOKER_H

#include "card.h"
#include "game.h"
#include "graphic_utils.h"
#include "item.h"
#include "random.h"

#include <maxmod.h>

// This won't be more than the number of jokers in your current deck
// plus the amount that can fit in the shop, 8 should be fine. For now...
#define MAX_ACTIVE_JOKERS 8

#define MAX_DEFINABLE_JOKERS 150

#define JOKER_SPRITE_OFFSET  16 // Offset for the joker sprites
#define JOKER_STARTING_LAYER 26
// Tile ID for the starting index in the tile memory
#define JOKER_TID     (JOKER_STARTING_LAYER * JOKER_SPRITE_OFFSET)
#define JOKER_BASE_PB 4 // The starting palette index for the jokers, after the boss blind tokens
#define JOKER_LAST_PB (NUM_PALETTES - 1)
// Currently allocating the rest of the palettes for the jokers.
// This number needs to be decreased once we need to allocated palettes for other sprites
// such as planet cards etc.

#define BASE_EDITION     0
#define FOIL_EDITION     1
#define HOLO_EDITION     2
#define POLY_EDITION     3
#define NEGATIVE_EDITION 4

#define MAX_EDITIONS 5

#define COMMON_JOKER    0
#define UNCOMMON_JOKER  1
#define RARE_JOKER      2
#define LEGENDARY_JOKER 3

#define MAX_RARITIES (LEGENDARY_JOKER + 1)

// Percent chance to get a joker of each rarity
// Note that this deviates slightly from the Balatro wiki to allow legendary
// jokers to appear without spectral cards implemented
#define COMMON_JOKER_CHANCE    70
#define UNCOMMON_JOKER_CHANCE  25
#define RARE_JOKER_CHANCE      5
#define LEGENDARY_JOKER_CHANCE 0

// These are the common Joker Events. Special Joker behaviour will be checked on a
// Joker per Joker basis (see if it's there, then do something, e.g. Pareidolia, Baseball Card)
enum JokerEvent
{
    JOKER_EVENT_ON_JOKER_CREATED, // Triggers only once when the Joker is created, mainly used for
                                  // data initialization
    JOKER_EVENT_ON_HAND_PLAYED,   // Triggers only once when the hand is played and before the cards
                                  // are scored
    JOKER_EVENT_ON_CARD_SCORED,   // Triggers when a played card scores (e.g. Walkie Talkie,
                                  // Fibonnacci...)
    JOKER_EVENT_ON_CARD_SCORED_END, // Triggers after the card has finishd scoring (e.g. retrigger
                                    // Jokers)
    JOKER_EVENT_ON_CARD_HELD,       // Triggers when going through held cards
    JOKER_EVENT_INDEPENDENT, // Joker will trigger normally, when Jokers are scored (e.g. base
                             // Joker)
    JOKER_EVENT_ON_HAND_SCORED_END, // Triggers when entire hand has finished scoring (e.g. food
                                    // Jokers)
    JOKER_EVENT_ON_HAND_DISCARDED,  // Triggers when discarding a hand
    JOKER_EVENT_ON_ROUND_END,       // Triggers at the end of the round (e.g. Rocket)
    JOKER_EVENT_ON_BLIND_SELECTED,  // Triggers when selecting a blind (e.g. Dagger, Riff Raff,
                                    // Madness..)
    JOKER_EVENT_ON_SHOP_REROLL,     // Triggers when the shop is rerolled (e.g. Flash Card)
};

// These are flags that can be combined into a single u32 and returned by
// JokerEffect functions to indicate which fields of the output JokerEffect are valid

#define JOKER_EFFECT_FLAG_NONE      0
#define JOKER_EFFECT_FLAG_CHIPS     (1 << 0)
#define JOKER_EFFECT_FLAG_MULT      (1 << 1)
#define JOKER_EFFECT_FLAG_XMULT     (1 << 2)
#define JOKER_EFFECT_FLAG_MONEY     (1 << 3)
#define JOKER_EFFECT_FLAG_RETRIGGER (1 << 4)
#define JOKER_EFFECT_FLAG_EXPIRE    (1 << 5)
#define JOKER_EFFECT_FLAG_MESSAGE   (1 << 6)

#define MAX_JOKER_OBJECTS 32 // The maximum number of joker objects that can be created at once

// Jokers in the game
#define STENCIL_JOKER_ID      15
#define SHORTCUT_JOKER_ID     48
#define BRAINSTORM_JOKER_ID   41
#define PAREIDOLIA_JOKER_ID   46
#define FOUR_FINGERS_JOKER_ID 50
#define BLUEPRINT_JOKER_ID    52
#define SMEARED_JOKER_ID      58
#define MIME_JOKER_ID         56
#define GROS_MICHEL_ID        60
#define CAVENDISH_ID          61
#define LOYALTY_CARD_ID       63
#define RIDING_THE_BUS_ID     64
#define CEREMONIAL_DAGGER_ID  65
#define CREDIT_CARD_ID        66
#define BURGLAR_ID            67
#define FLASH_CARD_ID         68
#define SHOWMAN_ID            69
#define CARD_SHARP_ID         70
#define TO_THE_MOON_ID        71
#define SPLASH_JOKER_ID       72
#define SUPERNOVA_JOKER_ID    73
#define GREEN_JOKER_ID        74
#define SQUARE_JOKER_ID       75
#define BASEBALL_CARD_ID      76

// Serialized "Upgrade!"-style message pop above a joker, used by the
// ON_PLAYED growth animation queue (joker_effects.c growth_msg_*).
void joker_show_message(JokerObject* joker_object, const char* message);
// Growth-message queue scheduler: advances one message per DEFER_DELAY
// beat while the queue is non-empty. Called every frame from game.c.
void growth_msg_process_pending(void);
// Aborts any pending growth messages (round transitions / resets).
void growth_msg_clear(void);
// True while any growth message is queued/playing. round.c gates the
// hand-play -> scoring transition on this.
bool growth_msg_pending(void);
// Baseball Card (76): number of active Baseball Card effects (real cards +
// Blueprint/Brainstorm copies resolving to one via the chain). joker.c's
// INDEPENDENT hook applies X1.5 per effect for every Uncommon joker scored.
int count_baseball_card_effects(void);

// Credit Card: shop purchases may go into debt down to -20$ per REAL
// Credit Card held. Blueprint/Brainstorm cannot copy this passive effect
// (no event trigger - copies invoke a no-op), so only real cards count.
#define CREDIT_CARD_DEBT_LIMIT 20

// Count of REAL Credit Cards held. Called by the shop when checking
// whether a purchase is affordable - re-evaluated per purchase so buying
// (or losing) a card updates the debt limit immediately.
int count_credit_card_effects(void);
// True while at least one REAL To the Moon (冲向月球, ID 71) is held:
// end-of-round interest is doubled. Silent-state joker (no trigger
// action) - Blueprint/Brainstorm copies do NOT count (same rule as
// Credit Card / Showman).
bool is_to_the_moon_active(void);
// Resolve what a Blueprint/Brainstorm copying joker ultimately copies,
// bouncing through other copying jokers (Blueprint -> right neighbor,
// Brainstorm -> leftmost) until a non-copying joker is found, or NULL
// (copy at list edge / infinite Brainstorm loop). Chain copies are valid
// (Brainstorm -> Blueprint -> any non-passive joker, user-ratified
// 2026-08-07). Uses the same walk as blueprint_brainstorm_joker_effect.
JokerObject* resolve_copy_target(JokerObject* copying_joker);
// Per-frame scheduler for deferred blind-selected joker effects (Riff-Raff
// spawns, Ceremonial Dagger sacrifice). Called from game.c's update loop.
void deferred_effects_process_pending(void);
// M24: true while deferred blind-selected effects are still queued.
bool deferred_effects_pending(void);

// True if this round's deferred queue actually produced a visible effect
// (trigger animation / sacrifice). Silent queues (Riff-Raff no free slot,
// Dagger no right neighbor) return false so the round can skip the
// post-effects beat.
bool deferred_effects_ran_animation(void);

// True while deferred blind-selected joker effects are still running
// (Riff-Raff spawn chain / dagger waiting for a victim). The round holds off
// dealing the hand until this returns false.
bool joker_effects_busy(void);

typedef struct
{
    u8 id;       // Unique ID for the joker, used to identify different jokers
    u8 modifier; // base, foil, holo, poly, negative
    u8 value;
    u8 rarity;

    // General purpose values that are interpreted differently for each Joker (scaling, last
    // retriggered card, etc...)
    s32 scoring_state;
    s32 persistent_state;
} Joker;

typedef struct JokerObject
{
    Item; // First member struct inheritance
    Joker* joker;
} JokerObject;

typedef struct // These jokers are triggered after the played hand has finished scoring.
{
    u32 chips;
    u32 mult;
    u32 xmult;     // Multiplicative mult, expressed as a fraction: xmult / xmult_den.
                   // xmult_den == 1 keeps the classic integer path (X2, X3...).
                   // xmult=3, xmult_den=2 -> X1.5 (fraction avoids float entirely).
    u32 xmult_den; // Denominator for the multiplicative mult fraction (default 1).
                   // Only checked when xmult > 0. Rendered as "X1.5" when > 1.
    int money;
    bool retrigger; // Retrigger played hand (e.g. "Dusk" joker, even though on the wiki it says "On
                    // Scored" it makes more sense to have it here)
    bool expire;    // Will make the Joker expire/destry itself if true (i.e. Bananas and fully
                    // consumed Food Jokers)
    char* message;  // Used to send custom messages e.g. "Extinct!" or "Again!"
} JokerEffect;

// ---------------------------------------------------------------------------
// Fractional multiplicative-mult entry points.
//
// TWO independent setters, one call per card, sharing the same instance:
//
//   Integer XMULT (X2, X3...):     joker_effect_set_xmult(e, 3);
//   Fractional XMULT (X1.5...):    joker_effect_set_xmult_den(e, 3, 2);
//
// set_xmult takes a single multiplier (denominator auto-reset to 0, the
// classic integer path). set_xmult_den takes numerator + denominator for
// exact fractions - pure integer math, no float on the GBA. Rendered as
// "X1.5" and applies mult * num / den (multiply-first, overflow-guarded,
// then divide).
//
// NOTE: s_shared_joker_effect is reused across jokers each frame, so the
// denominator is consumed and reset by joker_object_score after use -
// existing integer-XMULT jokers (direct field writes) need NO changes.
// ---------------------------------------------------------------------------
static inline void joker_effect_set_xmult(JokerEffect* joker_effect, u32 mult)
{
    joker_effect->xmult = mult;
    joker_effect->xmult_den = 0; // integer path (X2, X3...)
}

static inline void joker_effect_set_xmult_den(JokerEffect* joker_effect, u32 num, u32 den)
{
    joker_effect->xmult = num;
    joker_effect->xmult_den = den; // fractional path (X1.5...)
}

// JokerEffectFuncs take in a joker that will be scored, a scored_card that is not NULL when related
// to the given joker_event, and output a joker_effect storing the effects of the scored joker They
// return a set of flags indicating what fields of the joker_effect are valid to access
typedef u32 (*JokerEffectFunc)(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

typedef int (*JokerDescFunc)(Joker* joker, Rect dest_rect);

typedef struct
{
    const char* name;
    u8 rarity;
    u8 base_value;
    bool is_desc_dynamic; // Is the little variable description at the bottom dynamic?
                          // Only used by the Misprint joker for now
    JokerDescFunc joker_print_desc;
    JokerEffectFunc joker_effect_func;
} JokerInfo;
const JokerInfo* get_joker_registry_entry(int joker_id);
size_t get_joker_registry_size(void);

void joker_init();

Joker* joker_new(u8 id);
void joker_destroy(Joker** joker);

// Unique effects like "Four Fingers" or "Credit Card" will be hard coded into game.c with a
// conditional check for the joker ID from the players owned jokers game.c should probably be
// restructured so most of the variables in it are moved to some sort of global variable header file
// so they can be easily accessed and modified for the jokers
u32 joker_get_score_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

const char* joker_get_rarity_string(u8 rarity);

/**
 * @brief Get Joker rarity panel color.
 *
 * The colors are organized in the `card_rarity_pal_gfx.png` file which is organized like this:
 *  - 0     -> transparency
 *  - 1,2   -> Common Joker
 *  - 3,4   -> Uncommon Joker
 *  - 5,6   -> Rare Joker
 *  - 7,8   -> Legendary Joker / Tarot Card
 *  - 9,10  -> Planet Card
 *  - 11,12 -> Spectral Card
 *  - 13,14 -> Voucher
 *
 * @param rarity Value of the rarity (Common, Rare...)
 * @param main_color Whether we want the main or shadow color
 * @return u16 value of the color, not a pointer
 */
u16 joker_get_rarity_color(u8 rarity, bool main_color);

int joker_get_sell_value(const Joker* joker);

JokerObject* joker_object_new(Joker* joker);
void joker_object_destroy(JokerObject** joker_object);
// This doesn't actually score anything, it just performs an animation and plays a sound effect
void joker_object_shake(JokerObject* joker_object, mm_word sound_id);

/**
 * @brief Returns the buy price of the joker object.
 *
 * @param joker_object the joker object whose price to return.
 *
 * @return UNDEFINED in case of error, the buy price of the joker otherwise.
 */
int joker_object_get_buy_price(Item* joker_object);

// TODO: Move to an owned_jokers.c/.h file?
/**
 * @brief Add a Joker to the list of owned Jokers and place it in the joker row.
 *
 * @param joker_object The JokerObject to add cast to Item*
 */
void joker_object_add_to_owned(Item* joker_object);

/**
 * @brief Destroy a JokerObject item, free its resources, and make it available to be rolled.
 *
 * @param joker_object Pointer to the JokerObject Item* to destroy; set to NULL.
 */
void joker_object_dispose(Item** joker_object);

/**
 * @brief Set whether a Joker is available to be rolled for the shop, packs, etc.
 *
 * @param joker_id The ID of the joker whose availability to set.
 * @param rollable true to make it rollable, false otherwise.
 */
void joker_set_rollable(int joker_id, bool rollable);

/**
 * @brief Reset rollable jokers to include all jokers in the registry.
 */
void joker_reset_rollable_jokers(void);

/**
 * @brief Roll and create a new JokerObject item.
 *
 * @param key to the RNG sequence that will be used to roll the Joker ID.
 *
 * @return Newly created `Item*` (JokerObject) or NULL if none available.
 */
Item* joker_object_roll_new(enum RngSequence key);

// This scores the joker and returns true if it was scored successfully
// card_object = NULL means the joker_event does not concern a particular Card, i.e. Independend or
// On_Blind_Selected as opposed to events that concern a particular card, i.e. On_Card_Scored or
// On_Card_Held
bool joker_object_score(
    JokerObject* joker_object,
    CardObject* card_object,
    enum JokerEvent joker_event
);

Sprite* joker_object_get_sprite(JokerObject* joker_object);

#endif // JOKER_H
