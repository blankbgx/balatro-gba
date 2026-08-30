#include "joker.h"

#include "card.h"
#include "game/round.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "layout.h"
#include "mgba_logger.h"
#include "pool.h"
#include "random.h"
#include "soundbank.h"
#include "util.h"

// Tiles and palettes
#include "card_rarity_pal_gfx.h"
#include "item.h"
#include "joker_gfx.h"

#include <maxmod.h>
#include <stdlib.h>
#include <string.h>
#include <tonc.h>

// JOKER_SCORE_TEXT_Y (48) lives in layout.h, shared with round.c/joker_effects.c
#define HELD_CARD_SCORE_TEXT_Y 108
#define MAX_CARD_SCORE_STR_LEN 2
// what it was before (MAX_DEFINEABLE_JOKERS / JOKERS_PER_SPRITESHEET)
#define MAX_NUM_JOKERS_SPRITESHEETS 75

static const unsigned int* joker_gfxTiles[] = {
#define DEF_JOKER_GFX(idx) joker_gfx##idx##Tiles,
#include "../include/def_joker_gfx_table.h"
#undef DEF_JOKER_GFX
};
static const unsigned short* joker_gfxPal[] = {
#define DEF_JOKER_GFX(idx) joker_gfx##idx##Pal,
#include "def_joker_gfx_table.h"
#undef DEF_JOKER_GFX
};

// TODO: Removed unplanned editions...
const static u8 EDITION_PRICE_LUT[MAX_EDITIONS] = {
    0, // BASE_EDITION
    2, // FOIL_EDITION
    3, // HOLO_EDITION
    5, // POLY_EDITION
    5, // NEGATIVE_EDITION
};

/* So for the card objects, I needed them to be properly sorted
   which is why they let you specify the layer index when creating a new card object.
   Since the cards would overlap a lot in your hand, If they weren't sorted properly, it would look
   like a mess. The joker objects are functionally identical to card objects, so they use the same
   logic. But I'm going to use a simpler approach for the joker objects since I'm lazy and sorting
   them wouldn't look good enough to warrant the effort.
*/
static bool s_used_layers[MAX_JOKER_OBJECTS] = {false}; // Track used layers for joker sprites
// TODO: Refactor sorting into SpriteObject?

// Maps the spritesheet index to the palette bank index allocated to it.
// Spritesheets that were not allocated are
static int s_joker_spritesheet_pb_map[MAX_NUM_JOKERS_SPRITESHEETS];
static int s_joker_pb_num_sprite_users[JOKER_LAST_PB - JOKER_BASE_PB + 1] = {0};

BITSET_DEFINE(s_rollable_jokers_bitset, MAX_DEFINABLE_JOKERS)

// See linked issue for context of maps
// https://github.com/GBALATRO/balatro-gba/issues/274#issue-3685075538

// clang-format off
// Map of Joker ID -> Spritesheet idx
static const int JOKER_ID_TO_SPRITE_MAP[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1,
    2, 2,
    3, 3, 3, 3, 3,
    4, 4, 4, 4, 4,
    5, 5, 5, 5,
    6, 6, 6, 6,
    7, 7,
    8, 8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    // Custom jokers (ID 53-72): sheet -> slot comments are on the
    // JOKER_ID_TO_SPRITE_IDX_IN_SHEET table below; this table is sheet only.
    // 53-56,58-59,62-65,67-71 -> sheet 0 (merged); 57,60-61 -> sheet 19;
    // 66 -> sheet 15; 64 -> sheet 8; 72 -> sheet 5.
    0, 0, 0, 0, 19, 0, 0, 19, 19, // 53-61
    0,  // 62 Flower Pot
    0,  // 63 Loyalty Card
    8,  // 64 Ride the Bus
    0,  // 65 Ceremonial Dagger
    15, // 66 Credit Card
    0,  // 67 Burglar
    0,  // 68 Flash Card
    0,  // 69 Showman
    0,  // 70 Card Sharp
    0,  // 71 To the Moon
    5,  // 72 Splash
    // 73-75: all have real art now (Supernova gfx6/4, Green Joker gfx2/2, Square gfx0/32).
    6,  // 73 Supernova -> sheet 6, slot 4 (art: user-drawn 2026-08-21)
    2,  // 74 Green Joker -> sheet 2, slot 2 (art: user-drawn 2026-08-20)
    0,  // 75 Square Joker -> sheet 0, slot 32 (art: user-drawn 2026-08-20)
    9,  // 76 Baseball Card -> sheet 9, slot 1 (art: user-drawn 2026-08-21)
    0,  // 77 Stuntman -> sheet 0, slot 33 (art: user-drawn 2026-08-22)
    18, // 78 Ancient Joker -> sheet 18, slot 0 (empty fallback sheet, art: user-drawn 2026-08-23)
    18, // 79 Swashbuckler -> sheet 18, slot 1 (art: user-drawn 2026-08-24)
    1,  // 80 Gift Card -> sheet 1, slot 2 (art: user-drawn 2026-08-24)
    18, // 81 Mr. Bones -> sheet 18, slot 2 (art: user-drawn 2026-08-24)
    0,  // 82 Runner -> sheet 0, slot 34 (art: user-drawn 2026-08-28)
    7,  // 83 To Do List -> sheet 7, slot 2 (art: user-drawn 2026-08-29)
    0,  // 84 Merry Andy -> sheet 0, slot 35 (art: user-drawn 2026-08-30)
};

// Map of Joker ID -> sprite index within its spritesheet
// This handles non-sequential IDs across spritesheets
static const int JOKER_ID_TO_SPRITE_IDX_IN_SHEET[] = {
    // IDs 0-17: spritesheet 0 (sequential, use joker_id - 0)
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    // IDs 18-19: spritesheet 1 (sequential, use joker_id - 18)
    0, 1,
    // IDs 20-21: spritesheet 2 (sequential)
    0, 1,
    // IDs 22-26: spritesheet 3 (sequential)
    0, 1, 2, 3, 4,
    // IDs 27-31: spritesheet 4 (sequential)
    0, 1, 2, 3, 4,
    // IDs 32-35: spritesheet 5 (sequential)
    0, 1, 2, 3,
    // IDs 36-39: spritesheet 6 (sequential)
    0, 1, 2, 3,
    // IDs 40-41: spritesheet 7 (sequential)
    0, 1,
    // IDs 42-43: spritesheet 8 (sequential)
    0, 1,
    // IDs 44-52: individual spritesheets (all index 0)
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    // Custom jokers (ID 53-72): slot index within the sheet named above.
    // Most custom art was merged into sheet 0 (slots 18-31); sheet 19
    // holds the remaining food jokers; 66 on sheet 15, 64 on sheet 8,
    // 72 on sheet 5.
    20, // 53 Wee          -> sheet 0, slot 20
    21, // 54 Riff-Raff    -> sheet 0, slot 21
    18, // 55 Baron        -> sheet 0, slot 18
    19, // 56 Mime         -> sheet 0, slot 19
    0,  // 57 Egg          -> sheet 19, slot 0
    23, // 58 Smeared      -> sheet 0, slot 23
    22, // 59 Faceless     -> sheet 0, slot 22
    1,  // 60 Gros Michel  -> sheet 19, slot 1
    2,  // 61 Cavendish    -> sheet 19, slot 2
    25, // 62 Flower Pot   -> sheet 0, slot 25
    24, // 63 Loyalty Card -> sheet 0, slot 24
    2,  // 64 Ride the Bus -> sheet 8, slot 2
    26, // 65 Ceremonial Dagger -> sheet 0, slot 26
    1,  // 66 Credit Card  -> sheet 15, slot 1
    28, // 67 Burglar      -> sheet 0, slot 28
    27, // 68 Flash Card   -> sheet 0, slot 27
    29, // 69 Showman      -> sheet 0, slot 29
    30, // 70 Card Sharp   -> sheet 0, slot 30
    31, // 71 To the Moon  -> sheet 0, slot 31
    4,  // 72 Splash       -> sheet 5, slot 4
    4,  // 73 Supernova    -> sheet 6, slot 4 (art: user-drawn 2026-08-21)
    2,  // 74 Green Joker  -> sheet 2, slot 2 (art: user-drawn 2026-08-20)
    32, // 75 Square Joker -> sheet 0, slot 32 (art: user-drawn 2026-08-20)
    1,  // 76 Baseball Card -> sheet 9, slot 1 (art: user-drawn 2026-08-21)
    33, // 77 Stuntman      -> sheet 0, slot 33 (art: user-drawn 2026-08-22)
    0,  // 78 Ancient Joker -> sheet 18, slot 0 (art: user-drawn 2026-08-23)
    1,  // 79 Swashbuckler  -> sheet 18, slot 1 (art: user-drawn 2026-08-24)
    2,  // 80 Gift Card     -> sheet 1, slot 2 (art: user-drawn 2026-08-24)
    2,  // 81 Mr. Bones     -> sheet 18, slot 2 (art: user-drawn 2026-08-24)
    34, // 82 Runner        -> sheet 0, slot 34 (art: user-drawn 2026-08-28)
    2,  // 83 To Do List    -> sheet 7, slot 2 (art: user-drawn 2026-08-29)
    35, // 84 Merry Andy    -> sheet 0, slot 35 (art: user-drawn 2026-08-30)
    };

// Lookup table of Joker Rarity strings. Used to display at the bottom of the description screen.
static const char* JOKER_RARITY_STRINGS_LUT[MAX_RARITIES] = {
    "Common", "Uncommon", "Rare", "Legendary"
};
// clang-format on

static int s_get_num_spritesheets(void);
static int s_joker_get_spritesheet_idx(u8 joker_id);
static int s_joker_get_sprite_idx_in_sheet(u8 joker_id, int spritesheet_idx);
static void s_joker_pb_add_sprite_user(int pb);
static void s_joker_pb_remove_sprite_user(int pb);
static int s_joker_pb_get_num_sprite_users(int joker_pb);
static int s_get_unused_joker_pb(void);
static int s_allocate_pb_if_needed(u8 joker_id);
static int joker_get_random_rarity(enum RngSequence key);

void joker_init()
{
    // This should init once only so no need to free
    int num_spritesheets = s_get_num_spritesheets();

    for (int i = 0; i < num_spritesheets; i++)
    {
        s_joker_spritesheet_pb_map[i] = UNDEFINED;
    }
}

Joker* joker_new(u8 id)
{
    if (id >= get_joker_registry_size())
        return NULL;

    Joker* joker = POOL_GET(Joker);
    const JokerInfo* jinfo = get_joker_registry_entry(id);

    joker->id = id;
    joker->modifier = BASE_EDITION; // TODO: Make this a parameter
    joker->value = jinfo->base_value + EDITION_PRICE_LUT[joker->modifier];
    joker->rarity = jinfo->rarity;
    joker->scoring_state = 0;
    joker->persistent_state = 0;

    // initialize persistent Joker data if needed
    JokerEffect* joker_effect = NULL;
    jinfo->joker_effect_func(joker, NULL, JOKER_EVENT_ON_JOKER_CREATED, &joker_effect);

    return joker;
}

void joker_destroy(Joker** joker)
{
    POOL_FREE(Joker, *joker);
    *joker = NULL;
}

u32 joker_get_score_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    const JokerInfo* jinfo = get_joker_registry_entry(joker->id);
    if (!jinfo)
        return JOKER_EFFECT_FLAG_NONE;

    return jinfo->joker_effect_func(joker, scored_card, joker_event, joker_effect);
}

const char* joker_get_rarity_string(u8 rarity)
{
    if (rarity >= MAX_RARITIES)
        return NULL;

    return JOKER_RARITY_STRINGS_LUT[rarity];
}

u16 joker_get_rarity_color(u8 rarity, bool main_color)
{
    if (rarity >= MAX_RARITIES)
        return 0x0;

    // +1 to account for the transparency
    // odd indices are the main colors, even ones are the shadows
    return card_rarity_pal_gfxPal[1 + 2 * rarity + (main_color ? 0 : 1)];
}

int joker_get_buy_price(const Joker* joker)
{
    GBAL_RETURN_IF_NULL_RET(joker, UNDEFINED);

    return joker->value;
}

int joker_get_sell_value(const Joker* joker)
{
    GBAL_RETURN_IF_NULL_RET(joker, UNDEFINED);

    return joker->value / 2;
}

// JokerObject methods
JokerObject* joker_object_new(Joker* joker)
{
    JokerObject* joker_object = POOL_GET(JokerObject);

    sprite_object_init((SpriteObject*)joker_object);

    int layer = 0;
    for (int i = 0; i < MAX_JOKER_OBJECTS; i++)
    {
        if (!s_used_layers[i])
        {
            layer = i;
            s_used_layers[i] = true; // Mark this layer as used
            break;
        }
    }

    joker_object->joker = joker;

    joker_object->type = ITEM_TYPE_JOKER;

    int tile_index = JOKER_TID + layer * JOKER_SPRITE_OFFSET;

    int joker_spritesheet_idx = s_joker_get_spritesheet_idx(joker->id);
    int joker_idx = s_joker_get_sprite_idx_in_sheet(joker->id, joker_spritesheet_idx);
    int joker_pb = s_allocate_pb_if_needed(joker->id);
    s_joker_pb_add_sprite_user(joker_pb);

    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        &joker_gfxTiles[joker_spritesheet_idx][joker_idx * TILE_SIZE * JOKER_SPRITE_OFFSET],
        TILE_SIZE * JOKER_SPRITE_OFFSET
    );

    sprite_object_set_sprite(
        (SpriteObject*)joker_object,
        sprite_new(
            ATTR0_SQUARE | ATTR0_4BPP,
            ATTR1_SIZE_32,
            tile_index,
            joker_pb,
            JOKER_STARTING_LAYER + layer
        )
    );

    return joker_object;
}

void joker_object_destroy(JokerObject** joker_object)
{
    if (joker_object == NULL || *joker_object == NULL)
        return;

    s16 layer = sprite_get_layer(joker_object_get_sprite(*joker_object)) - JOKER_STARTING_LAYER;
    s_used_layers[layer] = false;
    s_joker_pb_remove_sprite_user(sprite_get_pb(joker_object_get_sprite(*joker_object)));
    if (s_joker_pb_get_num_sprite_users((sprite_get_pb(joker_object_get_sprite(*joker_object)))) ==
        0)
    {
        s_joker_spritesheet_pb_map[s_joker_get_spritesheet_idx((*joker_object)->joker->id)] =
            UNDEFINED;
    }

    sprite_object_destroy((SpriteObject*)(*joker_object));
    joker_destroy(&(*joker_object)->joker);
    POOL_FREE(JokerObject, *joker_object);
    *joker_object = NULL;
}

void joker_object_dispose(Item** joker_object_item)
{
    GBAL_RETURN_IF_NULL_VOID(joker_object_item);
    GBAL_RETURN_IF_NULL_VOID(*joker_object_item);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(*joker_object_item, ITEM_TYPE_JOKER);

    JokerObject* joker_object = (JokerObject*)(*joker_object_item);
    GBAL_RETURN_IF_NULL_VOID(joker_object->joker);

    joker_set_rollable(joker_object->joker->id, true);

    joker_object_destroy(&joker_object);
    *joker_object_item = NULL;
}

void joker_object_shake(JokerObject* joker_object, mm_word sound_id)
{
    sprite_object_shake((SpriteObject*)joker_object, sound_id);
}

int joker_object_get_buy_price(Item* joker_object)
{
    GBAL_RETURN_IF_NULL_RET(joker_object, UNDEFINED);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_RET(joker_object, ITEM_TYPE_JOKER, UNDEFINED);

    return ((JokerObject*)joker_object)->joker->value;
}

void joker_object_add_to_owned(Item* joker_object)
{
    GBAL_RETURN_IF_NULL_VOID(joker_object);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(joker_object, ITEM_TYPE_JOKER);

    joker_object->ty = int2fx(HELD_JOKERS_POS.y);
    add_joker((JokerObject*)joker_object);
}

void joker_set_rollable(int joker_id, bool rollable)
{
    bitset_set_idx(&s_rollable_jokers_bitset, joker_id, rollable);
}

/**
 * @brief Computes the number of Jokers we can currently roll in the Shop.
 *         The Jokers we own is taken into account and can't be rolled again.
 */
static inline int get_num_rollable_jokers(void)
{
    return bitset_num_set_bits(&s_rollable_jokers_bitset);
}

/**
 * @brief Returns true if we can't roll any Joker
 */
static inline bool no_rollable_jokers(void)
{
    return bitset_is_empty(&s_rollable_jokers_bitset);
}

GBAL_UNUSED
static inline bool joker_is_rollable(int joker_id)
{
    return bitset_get_idx(&s_rollable_jokers_bitset, joker_id);
}

void joker_reset_rollable_jokers(void)
{
    int num_jokers = get_joker_registry_size();

    bitset_clear(&s_rollable_jokers_bitset);
    for (int i = 0; i < num_jokers; i++)
    {
        bitset_set_idx(&s_rollable_jokers_bitset, i, true);
    }

    // Food joker initial pool state: Cavendish starts non-rollable
    // (only becomes rollable after Gros Michel is destroyed)
    bitset_set_idx(&s_rollable_jokers_bitset, CAVENDISH_ID, false);

    // Owned jokers can't be rolled again in the shop... unless Showman
    // (马戏团长) is in play: it allows already-owned Jokers to appear
    // multiple times in the shop/booster packs. Gros Michel follows the
    // same rule - it can only be re-obtained while Showman is held.
    // Once destroyed it leaves the pool forever (joker_update_food_pool).
    if (!is_showman_joker_active())
    {
        List* owned_jokers = get_jokers_list();
        ListItr itr = list_itr_create(owned_jokers);
        JokerObject* joker_object;
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != NULL && joker_object->joker != NULL)
            {
                bitset_set_idx(&s_rollable_jokers_bitset, joker_object->joker->id, false);
            }
        }
    }
}

/**
 * @brief Rolls a random Joker among the available ones
 */
static int joker_roll_id(enum RngSequence key)
{
    // Update Gros Michel / Cavendish pool before rolling
    joker_update_food_pool();

    // Now determine how many jokers are available based on the rarity
    int jokers_avail_size = get_num_rollable_jokers();

    if (jokers_avail_size == 0)
        return UNDEFINED;

    // Roll for what rarity the joker will be
    int joker_rarity = joker_get_random_rarity(key);

    int matching_joker_ids[jokers_avail_size];
    int fallback_random_idx = rng_get_u32(key) % jokers_avail_size;
    int fallback_random_joker_id = UNDEFINED;
    int match_count = 0;

    BitsetItr itr = bitset_itr_create(&s_rollable_jokers_bitset);

    int i = 0;
    int joker_id = UNDEFINED;
    while ((joker_id = bitset_itr_next(&itr)) != UNDEFINED)
    {
        if (i++ == fallback_random_idx)
            fallback_random_joker_id = joker_id;
        const JokerInfo* info = get_joker_registry_entry(joker_id);
        if (info->rarity == joker_rarity)
        {
            matching_joker_ids[match_count++] = joker_id;
        }
    }

    int selected_joker_id = (match_count > 0) ? matching_joker_ids[rng_get_u32(key) % match_count]
                                              : fallback_random_joker_id;

    return selected_joker_id;
}

Item* joker_object_roll_new(enum RngSequence key)
{
    if (no_rollable_jokers())
        return NULL;

    int joker_id = 0;
#ifdef TEST_JOKER_ID0 // Allow defining an ID for a joker to always appear in shop and be tested
    if (joker_is_rollable(TEST_JOKER_ID0))
    {
        joker_id = TEST_JOKER_ID0;
    }
    else
#endif
#ifdef TEST_JOKER_ID1
        if (joker_is_rollable(TEST_JOKER_ID1))
    {
        joker_id = TEST_JOKER_ID1;
    }
    else
#endif
    {
        joker_id = joker_roll_id(key);
    }

    // If for some reason only no joker is left, don't make another
    if (joker_id == UNDEFINED)
        return NULL;

    joker_set_rollable(joker_id, false);

    return (Item*)joker_object_new(joker_new(joker_id));
}

/**
 * @brief Get a random rarity a Joker will be rolled from
 *
 * @param key to the RNG sequence used
 * @return a random Joker rarity
 */
static inline int joker_get_random_rarity(enum RngSequence key)
{
    int joker_rarity = 0;
    int rarity_roll = rng_get_u32(key) % 100;
    if (rarity_roll < COMMON_JOKER_CHANCE)
    {
        joker_rarity = COMMON_JOKER;
    }
    else if (rarity_roll < COMMON_JOKER_CHANCE + UNCOMMON_JOKER_CHANCE)
    {
        joker_rarity = UNCOMMON_JOKER;
    }
    else if (rarity_roll < COMMON_JOKER_CHANCE + UNCOMMON_JOKER_CHANCE + RARE_JOKER_CHANCE)
    {
        joker_rarity = RARE_JOKER;
    }
    else if (rarity_roll < COMMON_JOKER_CHANCE + UNCOMMON_JOKER_CHANCE + RARE_JOKER_CHANCE +
                               LEGENDARY_JOKER_CHANCE)
    {
        joker_rarity = LEGENDARY_JOKER;
    }

    return joker_rarity;
}

static void set_and_shift_text(char* str, int* cursor_pos_x, int* cursor_pos_y, int color_pb)
{
    tte_set_pos(*cursor_pos_x, *cursor_pos_y);
    tte_set_special(color_pb * TTE_SPECIAL_PB_MULT_OFFSET);
    tte_write(str);

    // + 1 For space
    const int joker_score_display_offset_px = (MAX_CARD_SCORE_STR_LEN + 1) * TTE_CHAR_SIZE;
    *cursor_pos_x += joker_score_display_offset_px;

    // Self-clearing: any joker message written through here schedules its
    // own auto-erase, so callers never need to remember (works in every
    // phase - round, blind select, shop). Safe during scoring: the clear
    // timer skips while HAND_PLAYING and waits 90 frames anyway.
    schedule_joker_event_text_clear();
}

bool joker_object_score(
    JokerObject* joker_object,
    CardObject* card_object,
    enum JokerEvent joker_event
)
{
    if (joker_object == NULL)
    {
        return false;
    }

    JokerEffect* joker_effect = NULL;
    Card* card = card_object ? card_object->card : NULL;
    u32 effect_flags_ret =
        joker_get_score_effect(joker_object->joker, card, joker_event, &joker_effect);

    // Baseball Card (76): every Uncommon joker scored at INDEPENDENT gives
    // X1.5 Mult per Baseball Card effect (real + Blueprint/Brainstorm copies
    // resolving to one via the chain). 原版 Act = "On Other Jokers". Applied
    // BEFORE the NONE-return check so silent/static Uncommon jokers (e.g.
    // Smeared) grant the bonus too - matching original Balatro where rarity
    // is checked for every scored joker.
    if (joker_event == JOKER_EVENT_INDEPENDENT &&
        joker_object->joker->id != BASEBALL_CARD_ID)
    {
        const JokerInfo* info = get_joker_registry_entry(joker_object->joker->id);
        if (info != NULL && info->rarity == UNCOMMON_JOKER)
        {
            // The X1.5 values are NOT applied here - they accumulate one
            // per (source, target) animation pair as the baseball_anim_*
            // queue plays (HUD follows the animation, 原版观感). The queue
            // drains before the phase advances and PLAY_ENDING settles, so
            // the final mult is always complete and order-correct.
            baseball_anim_register_trigger(joker_object);
        }
    }

    if (effect_flags_ret == JOKER_EFFECT_FLAG_NONE)
    {
        return false;
    }

    if (effect_flags_ret & JOKER_EFFECT_FLAG_RETRIGGER)
    {
        set_retrigger(joker_effect->retrigger);
    }

    // joker_effect.message will have been set if the Joker had anything custom to say

    int cursorPosX = TILE_SIZE; // Offset of one tile to better center the text on the card
    int cursorPosY = 0;
    if (joker_event == JOKER_EVENT_ON_CARD_HELD)
    {
        // display the text on top of the card instead of below the Joker for Held Cards effects
        // scored_card cannot be NULL here because of the joker event
        cursorPosX += fx2int(card_object->x);
        cursorPosY = HELD_CARD_SCORE_TEXT_Y;
    }
    else
    {
        cursorPosX += fx2int(joker_object->x);
        cursorPosY = JOKER_SCORE_TEXT_Y;
    }

    mm_word sfx_id;
    if (effect_flags_ret & JOKER_EFFECT_FLAG_CHIPS)
    {
        g_game_vars.chips = u32_protected_add(g_game_vars.chips, joker_effect->chips);
        char score_buffer[INT_MAX_DIGITS + 2]; // For '+' and null terminator
        snprintf(score_buffer, sizeof(score_buffer), "+%lu", joker_effect->chips);
        set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_BLUE_PB);
        sfx_id = SFX_CHIPS_GENERIC; // The joker chips effect is "generic"
    }
    if (effect_flags_ret & JOKER_EFFECT_FLAG_MULT)
    {
        g_game_vars.mult = u32_protected_add(g_game_vars.mult, joker_effect->mult);
        char score_buffer[INT_MAX_DIGITS + 2];
        snprintf(score_buffer, sizeof(score_buffer), "+%lu", joker_effect->mult);
        set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_RED_PB);
        sfx_id = SFX_MULT;
    }
    // if xmult is zero, DO NOT multiply by it
    if (effect_flags_ret & JOKER_EFFECT_FLAG_XMULT && joker_effect->xmult > 0)
    {
        if (joker_effect->xmult_den > 1)
        {
            // Fractional path: mult *= xmult / xmult_den (e.g. 3/2 = X1.5).
            // Multiply first, then divide - keeps precision; u32_protected_mult
            // guards overflow. ROUND to nearest integer (add den/2 before
            // dividing) so small mults don't truncate to nothing:
            //   mult=1, X1.5 -> (1*3 + 1)/2 = 2  (was: 1*3/2 = 1, no-op!)
            // Rendered as "X1.5" (red).
            u32 mult_times_num = u32_protected_mult(g_game_vars.mult, joker_effect->xmult);
            if (mult_times_num > UINT32_MAX - joker_effect->xmult_den / 2)
            {
                // Saturation: (mult*num + den/2) would overflow u32 and
                // wrap to ~0, ZEROING the multiplier. Clamp to UINT32_MAX
                // instead of wrapping.
                g_game_vars.mult = UINT32_MAX;
            }
            else
            {
                g_game_vars.mult =
                    (mult_times_num + joker_effect->xmult_den / 2) /
                    joker_effect->xmult_den;
            }

            char score_buffer[INT_MAX_DIGITS + 2];
            u32 whole = joker_effect->xmult / joker_effect->xmult_den;
            u32 frac  = (joker_effect->xmult % joker_effect->xmult_den) * 10 /
                        joker_effect->xmult_den;
            if (frac == 0)
            {
                snprintf(score_buffer, sizeof(score_buffer), "X%lu", (unsigned long)whole);
            }
            else
            {
                snprintf(score_buffer, sizeof(score_buffer), "X%lu.%lu", (unsigned long)whole, (unsigned long)frac);
            }
            set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_RED_PB);
        }
        else
        {
            // Classic integer path (X2, X3...)
            g_game_vars.mult = u32_protected_mult(g_game_vars.mult, joker_effect->xmult);
            char score_buffer[INT_MAX_DIGITS + 2];
            snprintf(score_buffer, sizeof(score_buffer), "X%lu", (unsigned long)joker_effect->xmult);
            set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_RED_PB);
        }
        sfx_id = SFX_XMULT;

        // Consume the denominator immediately: s_shared_joker_effect is a
        // shared instance reused by every joker each frame, so a fractional
        // card (Baron: den=2) must not leak into the next joker's integer
        // XMULT. Resetting here means new jokers never need to remember.
        joker_effect->xmult_den = 0;
    }
    if (effect_flags_ret & JOKER_EFFECT_FLAG_MONEY)
    {
        g_game_vars.money += joker_effect->money;
        char score_buffer[INT_MAX_DIGITS + 2];
        snprintf(score_buffer, sizeof(score_buffer), "%d$", joker_effect->money);
        set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_YELLOW_PB);
        // TODO: Money sound effect
    }
    // custom message for Jokers (including retriggers where Jokers will say "Again!")
    // joker_effect->message will have been set if the Joker had anything custom to say
    if (effect_flags_ret & JOKER_EFFECT_FLAG_MESSAGE)
    {
        set_and_shift_text(joker_effect->message, &cursorPosX, &cursorPosY, TTE_WHITE_PB);
    }
    // this will start the Joker expire animation
    if (effect_flags_ret & JOKER_EFFECT_FLAG_EXPIRE && joker_effect->expire)
    {
        joker_object_shake(joker_object, UNDEFINED);
        // M27 dedup: an effect may already have pushed this joker into the
        // expired list itself (Gros Michel's global extinction pushes EVERY
        // Gros Michel, including this one). Pushing again double-schedules
        // joker_object_destroy -> use-after-free (sprite_get_layer reads freed
        // memory, s_used_layers[layer]=false goes OOB and corrupts the s_deck
        // Card* slots -> ghost cards).
        if (!list_contains(get_expired_jokers_list(), joker_object))
            list_push_back(get_expired_jokers_list(), joker_object);
    }

    // Update displays
    display_chips();
    display_mult();
    display_money();

    joker_object_shake(joker_object, sfx_id);

    return true;
}

Sprite* joker_object_get_sprite(JokerObject* joker_object)
{
    if (joker_object == NULL)
        return NULL;
    return sprite_object_get_sprite((SpriteObject*)joker_object);
}

// Serialized message display for the ON_PLAYED growth animation queue
// (joker_effects.c growth_msg_*): draws "Upgrade!"-style text above a
// joker OUTSIDE the synchronous joker_object_score path. Mirrors the
// message layout used there (joker x + score offset, row 48, white).
// The message is self-clearing via the same clear-timer machinery
// (safe during scoring: the clear timer skips while HAND_PLAYING and
// waits 90 frames anyway).
void joker_show_message(JokerObject* joker_object, const char* message)
{
    if (joker_object == NULL || message == NULL)
        return;

    int cursorPosX = fx2int(joker_object->x);
    int cursorPosY = JOKER_SCORE_TEXT_Y;
    const int joker_score_display_offset_px = (MAX_CARD_SCORE_STR_LEN + 1) * TTE_CHAR_SIZE;
    cursorPosX += joker_score_display_offset_px;

    schedule_joker_event_text_clear();
    set_and_shift_text((char*)message, &cursorPosX, &cursorPosY, TTE_WHITE_PB);
    // Keep the trigger shake the joker would have had when returning a
    // synchronous MESSAGE flag (UNDEFINED = visual shake, no sound - the
    // old MESSAGE-only path passed an uninitialized sfx_id anyway).
    joker_object_shake(joker_object, UNDEFINED);
}

// Plays one Baseball Card trigger animation pair (原版: 每个棒球效果源
// 只在自身轮次触发时与目标同帧 shake，目标下方弹红色 "X1.5"，HUD 倍率
// 随每对累乘一步):
//  - apply ONE X1.5 to g_game_vars.mult (round-half-up, saturate) and
//    refresh the HUD - the multiplier accumulates with the animation;
//  - shake the SOURCE (the Baseball Card effect instance whose round it
//    is) and the TARGET Uncommon joker on the SAME frame - a source never
//    shakes alongside another source's round;
//  - pop "X1.5" in red just BELOW the target joker.
// Called by the baseball_anim_* queue scheduler (one pair per beat). The
// queue drains before PLAY_ENDING settles, so the final mult is complete.
void joker_play_baseball_animation(JokerObject* source, JokerObject* target)
{
    if (source == NULL || target == NULL)
        return;

    // 1. Accumulate one X1.5 (HUD follows the animation).
    u32 mult_times_num = u32_protected_mult(g_game_vars.mult, 3);
    g_game_vars.mult =
        (mult_times_num > UINT32_MAX - 1) ? UINT32_MAX
                                          : (mult_times_num + 1) / 2;
    display_mult();

    // 2. Shake source + target together (same frame, 原版).
    joker_object_shake(source, UNDEFINED);
    joker_object_shake(target, UNDEFINED);

    // 3. Red "X1.5" just below the target joker card (card is 32px tall).
    int bx = TILE_SIZE + fx2int(target->x);
    int by = fx2int(target->y) + TILE_SIZE * 5; // 8px below the card bottom
    set_and_shift_text("X1.5", &bx, &by, TTE_RED_PB);
    schedule_joker_event_text_clear();
}

static int s_get_num_spritesheets()
{
    return MAX_NUM_JOKERS_SPRITESHEETS;
}

static int s_joker_get_spritesheet_idx(u8 joker_id)
{
    return JOKER_ID_TO_SPRITE_MAP[joker_id];
}

static int s_joker_get_sprite_idx_in_sheet(u8 joker_id, int spritesheet_idx)
{
    return JOKER_ID_TO_SPRITE_IDX_IN_SHEET[joker_id];
}

static void s_joker_pb_add_sprite_user(int pb)
{
    s_joker_pb_num_sprite_users[pb - JOKER_BASE_PB]++;
}

static void s_joker_pb_remove_sprite_user(int pb)
{
    int num_sprite_users = s_joker_pb_num_sprite_users[pb - JOKER_BASE_PB];
    s_joker_pb_num_sprite_users[pb - JOKER_BASE_PB] = max(0, num_sprite_users - 1);
}

static int s_joker_pb_get_num_sprite_users(int joker_pb)
{
    return s_joker_pb_num_sprite_users[joker_pb - JOKER_BASE_PB];
}

static int s_get_unused_joker_pb()
{
    for (int i = 0; i < NUM_ELEM_IN_ARR(s_joker_pb_num_sprite_users); i++)
    {
        if (s_joker_pb_num_sprite_users[i] == 0)
        {
            return (i + JOKER_BASE_PB);
        }
    }

    return UNDEFINED;
}

static int s_allocate_pb_if_needed(u8 joker_id)
{
    int joker_spritesheet_idx = s_joker_get_spritesheet_idx(joker_id);
    int joker_pb = s_joker_spritesheet_pb_map[joker_spritesheet_idx];
    if (joker_pb != UNDEFINED)
    {
        // Already allocated
        return joker_pb;
    }

    // Allocate a new palette
    joker_pb = s_get_unused_joker_pb();

    if (joker_pb == UNDEFINED)
    {
        // Ran out of palettes, default to base and pray
        joker_pb = JOKER_BASE_PB;
    }
    else
    {
        s_joker_spritesheet_pb_map[joker_spritesheet_idx] = joker_pb;
        memcpy16(
            &pal_obj_mem[PAL_ROW_LEN * joker_pb],
            joker_gfxPal[joker_spritesheet_idx],
            NUM_ELEM_IN_ARR(joker_gfx0Pal)
        );
    }

    return joker_pb;
}
