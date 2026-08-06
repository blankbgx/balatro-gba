#ifndef GAME_H
#define GAME_H

#include "game/common_ui.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "item.h"

#include <tonc.h>

#define MAX_DECK_SIZE        52
#define MAX_JOKERS_HELD_SIZE 5 // This doesn't account for negatives right now.
#define MAX_SHOP_ITEMS       2 // TODO: Make this dynamic
#define MAX_SELECTION_SIZE   5
#define FRAMES(x)            (((x) + (g_game_vars.game_speed) - 1) / (g_game_vars.game_speed))

// ---------------------------------------------------------------------------
// DEBUG SWITCHES
// Set to 1 to enable, 0 for release builds.
// ---------------------------------------------------------------------------
#define DEBUG_SHOP_FREE 1 // Free shop rerolls + all items priced at $0

// TODO: Can make these dynamic to support interest-related jokers and vouchers
#define MAX_INTEREST   5
#define INTEREST_PER_5 1

// Input bindings
#define SELECT_CARD    KEY_A
#define DESELECT_CARDS KEY_B
#define PEEK_DECK      KEY_L // Not implemented
#define SORT_HAND      KEY_R
#define PAUSE_GAME     KEY_START // Not implemented
#define SELL_KEY       KEY_L
#define TAB_LEFT       KEY_L
#define TAB_RIGHT      KEY_R

// Matching the position of the on-screen buttons
#define PLAY_HAND_KEY KEY_L
// Same value as SELL_KEY - activated on the joker row, while this is activated on the hand row

#define DISCARD_HAND_KEY KEY_R

struct List;
typedef struct List List;

// Utility functions for other files
typedef struct CardObject CardObject;
typedef struct Card Card;
typedef struct JokerObject JokerObject;

// Enum value names in ../include/def_state_info_table.h
enum GameState
{
#define DEF_STATE_INFO(stateEnum, on_init, on_update, on_exit) stateEnum,
#include "def_state_info_table.h"
#undef DEF_STATE_INFO
    GAME_STATE_MAX,
    GAME_STATE_UNDEFINED
};

// Game functions
void game_init(void);

/**
 * @brief Called when exiting the Game Over screen (both win or lose) to reset game variables
 *         and start a fresh new run.
 *
 * WARNING: This function is currently only meant to be called from the "GAME_OVER" state
 * and shouldn't be called from other states, otherwise some data such as shop jokers
 * may not be properly reset.
 */
void game_reset(void);

void game_update(void);
void game_change_state(enum GameState new_game_state);
enum GameState game_get_state(void);

bool is_joker_owned(int joker_id);
bool joker_object_can_acquire(Item* item);
bool card_is_face(Card* card);
void add_joker(JokerObject* joker_object);
void remove_owned_joker(int owned_joker_idx);
List* get_jokers_list(void);
List* get_expired_jokers_list(void);
List* get_discarded_jokers_list(void);

/**
 * @brief Schedule an auto-clear of one-shot joker event messages
 *        (ON_ROUND_END / ON_BLIND_SELECTED) after a short delay.
 */
void schedule_joker_event_text_clear(void);

/**
 * @brief Briefly flash the hands HUD number (white/blue toggle) to draw
 *        the player's eye to a joker-mutated value (e.g. Burglar +3).
 */
void hud_pulse_hands(void);

/**
 * @brief Briefly flash the discards HUD number (white/red toggle).
 */
void hud_pulse_discards(void);

int deck_get_size(void);
int get_deck_top(void);
void deck_push(Card* card);
Card* deck_pop(void);
void deck_shuffle(void);
int get_num_discards_remaining(void);
int get_num_hands_remaining(void);

void display_deck_size_max(void);
void display_chips(void);
void display_mult(void);
void display_money(void);
void display_ante(void);

// joker specific functions
bool is_shortcut_joker_active(void);
bool is_smeared_joker_active(void);
int get_straight_and_flush_size(void);
void joker_update_food_pool(void);
bool is_gros_michel_destroyed(void);
void set_gros_michel_destroyed(void);
void set_discarded_face_card_count(int count);
int get_discarded_face_card_count(void);

void game_start(void);

void display_round(void);
void display_hands(void);
void display_discards(void);
void display_temp_score(u32 value);
void erase_temp_score(void);
void display_score(u32 value);

#endif // GAME_H
