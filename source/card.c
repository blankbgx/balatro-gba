#include "card.h"
#include "game.h"
#include "joker.h"

#include "graphic_utils.h"
#include "item.h"
#include "util.h"

#include <maxmod.h>
#include <stdlib.h>
#include <string.h>

// Audio
#include "pool.h"
#include "soundbank.h"

// Card Sprites and Palettes
#include "deck_big_gfx.h"
#include "deck_gfx.h"
#include "decks_face_down_gfx.h"
#include "high_contrast_deck_pal_gfx.h"

// Card sprites lookup table. First index is the suit, second index is the rank. The value is the
// tile index.
const static u16 CARD_SPRITE_LUT[NUM_SUITS][NUM_RANKS] = {
    {0,   16,  32,  48,  64,  80,  96,  112, 128, 144, 160, 176, 192},
    {208, 224, 240, 256, 272, 288, 304, 320, 336, 352, 368, 384, 400},
    {416, 432, 448, 464, 480, 496, 512, 528, 544, 560, 576, 592, 608},
    {624, 640, 656, 672, 688, 704, 720, 736, 752, 768, 784, 800, 816}
};
// Deck sprites lookup table. Index is the deck Id. The value is the tile index.
const static u16 DECK_SPRITE_LUT[DECK_TYPE_MAX] = {0, 16, 32, 48, 64, 80};

static bool s_high_contrast = DEFAULT_HIGH_CONTRAST;
static bool s_more_readable = DEFAULT_MORE_READABLE;

#ifdef DEBUG_SHOP_FREE
// Ghost-face hunt (2026-08-18): ring buffer of recent VRAM slot writes.
// When the watchdog finds a slot holding the wrong face, this names the
// last writer to that slot (layer, face copied, tick). face_down writes
// are recorded with suit/rank = -2 (deck back art).
#define CARD_WRITER_LOG_SIZE 16
static struct
{
    int layer;
    int suit;
    int rank;
    u32 tick;
} s_card_writer_log[CARD_WRITER_LOG_SIZE];
static int s_card_writer_log_pos = 0;

static void card_debug_log_write(int layer, int suit, int rank)
{
    s_card_writer_log[s_card_writer_log_pos].layer = layer;
    s_card_writer_log[s_card_writer_log_pos].suit = suit;
    s_card_writer_log[s_card_writer_log_pos].rank = rank;
    s_card_writer_log[s_card_writer_log_pos].tick = game_get_ui_tick();
    s_card_writer_log_pos = (s_card_writer_log_pos + 1) % CARD_WRITER_LOG_SIZE;
}

/**
 * @brief Most recent recorded write to `layer`, walking the ring backwards.
 * @return false if no write to that layer is in the buffer.
 */
bool card_debug_last_writer(int layer, int* suit, int* rank, u32* tick)
{
    for (int n = 1; n <= CARD_WRITER_LOG_SIZE; n++)
    {
        int idx = (s_card_writer_log_pos - n + CARD_WRITER_LOG_SIZE) % CARD_WRITER_LOG_SIZE;
        if (s_card_writer_log[idx].layer == layer)
        {
            *suit = s_card_writer_log[idx].suit;
            *rank = s_card_writer_log[idx].rank;
            *tick = s_card_writer_log[idx].tick;
            return true;
        }
    }
    return false;
}
#endif // DEBUG_SHOP_FREE

void card_init()
{
    GRIT_CPY(&pal_obj_mem[DECK_SPRITES_PB * PAL_ROW_LEN], decks_face_down_gfxPal);
}

void set_cards_high_contrast(bool enable)
{
    s_high_contrast = enable;
    if (s_high_contrast)
    {
        GRIT_CPY(&pal_obj_mem[CARD_PB * PAL_ROW_LEN], high_contrast_deck_pal_gfxPal);
    }
    else
    {
        GRIT_CPY(&pal_obj_mem[CARD_PB * PAL_ROW_LEN], deck_gfxPal);
    }
}

void set_cards_more_readable(bool enable)
{
    s_more_readable = enable;
}

bool get_cards_high_contrast(void)
{
    return s_high_contrast;
}

bool get_cards_more_readable(void)
{
    return s_more_readable;
}

// Card methods
Card* card_new(u8 suit, u8 rank)
{
    Card* card = POOL_GET(Card);

    card->suit = suit;
    card->rank = rank;

    return card;
}

void card_destroy(Card** card)
{
    POOL_FREE(Card, *card);
    *card = NULL;
}

u8 card_get_value(Card* card)
{
    if (card->rank == JACK || card->rank == QUEEN || card->rank == KING)
    {
        return 10; // Face cards are worth 10
    }
    else if (card->rank == ACE)
    {
        return 11; // Ace is worth 11
    }
    else
    {
        return card->rank + RANK_OFFSET; // 2-10 are worth their rank + RANK_OFFSET
    }

    return 0; // Should never reach here, but just in case
}

// CardObject methods
CardObject* card_object_new(Card* card)
{
    CardObject* card_object = POOL_GET(CardObject);
    sprite_object_init((SpriteObject*)card_object);
    card_object->card = card;

    card_object->type = ITEM_TYPE_PLAYING_CARD;
    card_object->selected = false;

    return card_object;
}

void card_object_destroy(CardObject** card_object)
{
    if (*card_object == NULL)
        return;
    sprite_object_destroy((SpriteObject*)(*card_object));
    POOL_FREE(CardObject, *card_object);
    *card_object = NULL;
}

void card_object_set_sprite(CardObject* card_object, s16 layer)
{
    int tile_index = CARD_TID + (layer * CARD_SPRITE_OFFSET);
    const unsigned int* card_tiles = s_more_readable ? deck_big_gfxTiles : deck_gfxTiles;
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        &card_tiles[CARD_SPRITE_LUT[card_object->card->suit][card_object->card->rank] * TILE_SIZE],
        TILE_SIZE * CARD_SPRITE_OFFSET
    );
#ifdef DEBUG_SHOP_FREE
    card_debug_log_write(layer, card_object->card->suit, card_object->card->rank);
#endif
    Sprite* sprite = sprite_new(
        ATTR0_SQUARE | ATTR0_4BPP,
        ATTR1_SIZE_32,
        tile_index,
        CARD_PB,
        layer + CARD_STARTING_LAYER
    );
    sprite_object_set_sprite((SpriteObject*)card_object, sprite);
}

void card_object_set_sprite_face_down(CardObject* card_object, enum DeckType deck, s16 layer)
{
    int tile_index = CARD_TID + (layer * CARD_SPRITE_OFFSET);
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        &decks_face_down_gfxTiles[DECK_SPRITE_LUT[deck] * TILE_SIZE],
        TILE_SIZE * CARD_SPRITE_OFFSET
    );
#ifdef DEBUG_SHOP_FREE
    card_debug_log_write(layer, -2, -2); // deck back art
#endif
    Sprite* sprite = sprite_new(
        ATTR0_SQUARE | ATTR0_4BPP,
        ATTR1_SIZE_32,
        tile_index,
        DECK_SPRITES_PB,
        layer + CARD_STARTING_LAYER
    );
    sprite_object_set_sprite((SpriteObject*)card_object, sprite);
}

void card_object_shake(CardObject* card_object, mm_word sound_id)
{
    sprite_object_shake((SpriteObject*)card_object, sound_id);
}

#ifdef DEBUG_SHOP_FREE
/**
 * @brief DEBUG watchdog helper: verify a card's VRAM slot holds ITS face.
 *
 * Compares the 16 tiles at the card's slot (CARD_TID + layer*offset) with
 * the expected face from the active sheet. On mismatch, searches BOTH deck
 * sheets (all 52 faces) to identify what the slot actually contains -
 * identifying the displayed face names the overwriter (see the "A♦ shown
 * as 2♦" ghost-face hunt, 2026-08-18).
 *
 * @return true if the slot holds the correct face. On false, the
 *         shown_suit / shown_rank out-params identify the displayed face
 *         (-1/-1 = content matches no known card face).
 */
bool card_debug_slot_matches(const Card* card, int layer, int* shown_suit, int* shown_rank)
{
    const int tile_index = CARD_TID + (layer * CARD_SPRITE_OFFSET);
    const u32* vram = (const u32*)&tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index];
    const u32 copy_words = TILE_SIZE * CARD_SPRITE_OFFSET;

    const unsigned int* card_tiles = s_more_readable ? deck_big_gfxTiles : deck_gfxTiles;
    const u32* expected = (const u32*)&card_tiles[CARD_SPRITE_LUT[card->suit][card->rank] * TILE_SIZE];
    if (memcmp(expected, vram, copy_words * sizeof(u32)) == 0)
        return true;

    for (int sheet = 0; sheet < 2; sheet++)
    {
        const unsigned int* tiles = sheet ? deck_big_gfxTiles : deck_gfxTiles;
        for (int s = 0; s < NUM_SUITS; s++)
        {
            for (int r = 0; r < NUM_RANKS; r++)
            {
                if (memcmp(&tiles[CARD_SPRITE_LUT[s][r] * TILE_SIZE], vram,
                           copy_words * sizeof(u32)) == 0)
                {
                    *shown_suit = s;
                    *shown_rank = r;
                    return false;
                }
            }
        }
    }
    *shown_suit = -1;
    *shown_rank = -1;
    return false;
}
#endif // DEBUG_SHOP_FREE

void card_object_set_selected(CardObject* card_object, bool selected)
{
    if (card_object == NULL)
        return;
    card_object->selected = selected;
}

bool card_object_is_selected(CardObject* card_object)
{
    if (card_object == NULL)
        return false;
    return card_object->selected;
}

// A card participates in scoring if it is selected - or, while Splash
// (ID 72) is owned, EVERY played card scores.
// Consumers: round.c scoring loop and joker streak checks (Ride the Bus)
// use THIS function so they stay in sync with Splash behavior.
bool card_object_is_scoring(CardObject* card_object)
{
    return card_object_is_selected(card_object) || is_joker_owned(SPLASH_JOKER_ID);
}

Sprite* card_object_get_sprite(CardObject* card_object)
{
    if (card_object == NULL)
        return NULL;
    return sprite_object_get_sprite((SpriteObject*)card_object);
}

int card_object_get_buy_price(Item* card_object)
{
    GBAL_RETURN_IF_NULL_RET(card_object, UNDEFINED);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_RET(card_object, ITEM_TYPE_PLAYING_CARD, UNDEFINED);

    return 1;
}

void card_object_dispose(Item** card_object_item)
{
    GBAL_RETURN_IF_NULL_VOID(card_object_item);
    GBAL_RETURN_IF_NULL_VOID(*card_object_item);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(*card_object_item, ITEM_TYPE_PLAYING_CARD);

    CardObject* card_object = (CardObject*)(*card_object_item);
    card_object_destroy(&card_object);
    *card_object_item = NULL;
}
