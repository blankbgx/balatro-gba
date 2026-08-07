#include "game.h"

#include "affine_background.h"
#include "affine_background_gfx.h"
#include "audio_utils.h"
#include "background_main_menu_gfx.h"
#include "button.h"
#include "card.h"
#include "game/blind_select.h"
#include "game/common_ui.h"
#include "game/game_over.h"
#include "game/joker_row.h"
#include "game/main_menu.h"
#include "game/options_menu.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/run_setup.h"
#include "game/shop.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "hand.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "random.h"
#include "save.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "splash_screen.h"
#include "sprite.h"
#include "state_machine.h"
#include "timer.h"
#include "tonc_memdef.h"
#include "util.h"

#include <maxmod.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STRAIGHT_AND_FLUSH_SIZE_FOUR_FINGERS 4
#define STRAIGHT_AND_FLUSH_SIZE_DEFAULT      5

// SE sizes

#define STARTING_ROUND 0
#define STARTING_ANTE  1
#define STARTING_MONEY 4
#define STARTING_SCORE 0

#define EXPIRE_ANIMATION_FRAME_COUNT 3

// These functions need to be forward declared
// so they're visible to the state_info array,
// and the sub-state function tables.
// This could be done, and maybe should be done,
// with an X macro, but I'll leave that to the
// reviewer(s).

int deck_get_size(void);
static int deck_get_max_size(void);

// Consts

// clang-format off
// Rects                                       left     top     right   bottom

// Rects for TTE (in pixels)

// Score displayed in the same place as the hand type
static const Rect TEMP_SCORE_RECT           = {8,       64,     64,     72  }; 
static const Rect SCORE_RECT                = {24,      48,     64,     56  };

static const Rect MONEY_TEXT_RECT           = {8,       120,    64,     128 };
static const Rect CHIPS_TEXT_RECT           = {8,       80,     32,     88  };
static const Rect MULT_TEXT_RECT            = {40,      80,     64,     88  };

// Rects with UNDEFINED are only used in tte_printf, they need to be fully defined
// to be used with tte_erase_rect_wrapper()
static const Rect HANDS_TEXT_RECT           = {16,      104,    UNDEFINED, UNDEFINED };
static const Rect DISCARDS_TEXT_RECT        = {48,      104,    UNDEFINED, UNDEFINED };
// Erase area for hands/discards: fixed 3-char width so a value shrinking
// from 2-3 digits to 1 digit (e.g. 10 -> 9) fully clears the old digits
// instead of leaving a stale char ("90" after "10"->"9"). Height = 1 char.
static const Rect HANDS_TEXT_ERASE_RECT     = {16,      104,    16 + 3 * TTE_CHAR_SIZE, 104 + TTE_CHAR_SIZE };
static const Rect DISCARDS_TEXT_ERASE_RECT  = {48,      104,    48 + 3 * TTE_CHAR_SIZE, 104 + TTE_CHAR_SIZE };
static const Rect DECK_SIZE_RECT            = {200,     152,    240,       160       };
static const Rect ROUND_TEXT_RECT           = {48,      144,    UNDEFINED, UNDEFINED };
static const Rect ANTE_TEXT_RECT            = {8,       144,    UNDEFINED, UNDEFINED };
// clang-format on

static StateInfo state_info[] = {
#define DEF_STATE_INFO(stateEnum, init_fn, update_fn, exit_fn) \
    {.on_init = init_fn, .on_update = update_fn, .on_exit = exit_fn},
#include "../include/def_state_info_table.h"
#undef DEF_STATE_INFO
};

static StateMachine game_sm = STATE_MACHINE_DEFINE(state_info, GAME_STATE_MAX);

// Initialization of the global vars
// clang-format off
GameVariables g_game_vars = {
    .timer = 0,
    // Setting the seed to an invalid value so that the Run Setup screen knows we're not reusing a previous Run's seed
    .rng_info = {UNDEFINED, {0}},

    .round = 0, .ante = 0, .money = 0, .hand_size = DEFAULT_HAND_SIZE,
    .deck = DECK_TYPE_RED,

    .best_hand_score = 0, .nb_played_hands = {0},

    .current_blind = BLIND_TYPE_SMALL,
    .next_boss_blind = BLIND_TYPE_BIG,
    .blinds_states =
    {
        BLIND_STATE_CURRENT,
        BLIND_STATE_UPCOMING,
        BLIND_STATE_UPCOMING
    },

    .hands = 0,
    .discards = 0,
    .score = 0,
    .chips = 0,
    .mult = 0,

    .playing_blind_token = NULL,
    .round_end_blind_token = NULL,

    .game_speed = DEFAULT_GAME_SPEED,
    .music_volume = DEFAULT_MUSIC_VOLUME,
    .sound_volume = DEFAULT_SOUND_VOLUME,
};
// clang-format on

static List s_owned_jokers_list;
static List s_discarded_jokers_list;
static List s_expired_jokers_list;

// Stacks
static Card* s_deck[MAX_DECK_SIZE] = {NULL};
static int s_deck_top = -1;

// Joker Special Variables
static int s_shortcut_joker_count = 0;

static int s_four_fingers_joker_count = 0;

static int s_smeared_joker_count = 0;

static int s_showman_joker_count = 0;

// Gros Michel / Cavendish food joker state
static bool s_gros_michel_destroyed = false;

void deck_push(Card* card)
{
    if (s_deck_top >= MAX_DECK_SIZE - 1)
        return;
    s_deck[++s_deck_top] = card;
}

Card* deck_pop(void)
{
    if (s_deck_top < 0)
        return NULL;
    return s_deck[s_deck_top--];
}

void display_ante(void)
{
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%ld#{cx:0x%X000}/%d",
        ANTE_TEXT_RECT.left,
        ANTE_TEXT_RECT.top,
        TTE_YELLOW_PB,
        g_game_vars.ante,
        TTE_WHITE_PB,
        MAX_ANTE
    );
}

void game_init()
{
    state_machine_remove(&game_sm);
    state_machine_register(&game_sm);
    // Initialize all jokers list once
    s_owned_jokers_list = list_init();
    s_discarded_jokers_list = list_init();
    s_expired_jokers_list = list_init();
    // TODO: Move this to an initialization of the play scoring states

    // Reset food joker state for new game
    s_gros_michel_destroyed = false;

    game_shop_reset();

    g_game_vars.hands = MAX_HANDS;
    g_game_vars.discards = MAX_DISCARDS;
    g_game_vars.timer = TM_ZERO;
    g_game_vars.current_blind = BLIND_TYPE_SMALL;
    g_game_vars.blinds_states[0] = BLIND_STATE_CURRENT;
    g_game_vars.blinds_states[1] = BLIND_STATE_UPCOMING;
    g_game_vars.blinds_states[2] = BLIND_STATE_UPCOMING;
    g_game_vars.ante = STARTING_ANTE;
    g_game_vars.money = STARTING_MONEY;
    g_game_vars.score = STARTING_SCORE;
    g_game_vars.round = 0;
    g_game_vars.chips = 0;
    g_game_vars.mult = 0;
    g_game_vars.round = STARTING_ROUND;

    g_game_vars.best_hand_score = 0;
    for (int i = 0; i < HAND_TYPE_MAX; i++)
        g_game_vars.nb_played_hands[i] = 0;
}

void game_reset()
{
    while (list_get_len(&s_owned_jokers_list) > 0)
    {
        JokerObject* joker_object = list_get_at_idx(&s_owned_jokers_list, 0);
        remove_owned_joker(0);
        joker_object_destroy(&joker_object);
    }

    tte_erase_screen();

    // For some reason that I haven't figured out yet,
    // if I don't destroy the blind tokens they won't
    // show up on the next run.
    sprite_destroy(&g_game_vars.playing_blind_token);
    sprite_destroy(&g_game_vars.round_end_blind_token);

    list_clear(&s_owned_jokers_list);
    list_clear(&s_discarded_jokers_list);
    list_clear(&s_expired_jokers_list);

    game_init();
    init_unbeaten_blinds_lists();

    display_round();
    display_score(g_game_vars.score);
    display_chips();
    display_mult();
    display_hands();
    display_discards();
    display_money();
    // Ante
    display_ante();

    affine_background_load_palette(affine_background_gfxPal);
}

static inline void discarded_jokers_update_loop(void)
{
    if (list_is_empty(&s_discarded_jokers_list))
    {
        return;
    }

    ListItr itr = list_itr_create(&s_discarded_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object->x == joker_object->tx && joker_object->y == joker_object->ty)
        {
            list_itr_remove_current_node(&itr);
            // TODO: joker_object_dispose() instead?
            joker_object_destroy(&joker_object);
        }
    }
}

static inline void held_jokers_update_loop(void)
{
    static const int SPACING_LUT[MAX_JOKERS_HELD_SIZE][MAX_JOKERS_HELD_SIZE] = {
        {0,  0,   0,   0,   0  },
        {13, -13, 0,   0,   0  },
        {26, 0,   -26, 0,   0  },
        {39, 13,  -13, -39, 0  },
        {40, 20,  0,   -20, -40}
    };

    FIXED hand_x = int2fx(HELD_JOKERS_POS.x);

    ListItr itr = list_itr_create(&s_owned_jokers_list);
    JokerObject* joker;
    int jokers_top = list_get_len(&s_owned_jokers_list) - 1;
    int i = 0;
    while ((joker = list_itr_next(&itr)))
    {
        // Let the Shop handle the position of this Joker
        if (joker != game_shop_get_description_card())
            joker->tx = hand_x - int2fx(SPACING_LUT[jokers_top][i]);
        i++;
    }
}

bool joker_object_can_acquire(Item* joker_object)
{
    GBAL_RETURN_IF_NULL_RET(joker_object, false);
    return (list_get_len(get_jokers_list()) < MAX_JOKERS_HELD_SIZE);
}

static inline void expired_jokers_update_loop(void)
{
    if (list_is_empty(&s_expired_jokers_list))
    {
        return;
    }

    ListItr itr = list_itr_create(&s_expired_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        // let just enough frames pass that we see it rotating and shrinking
        if (g_game_vars.timer % FRAMES(EXPIRE_ANIMATION_FRAME_COUNT) == 0)
        {
            // get joker idx
            int expired_joker_idx = 0;
            ListItr joker_itr = list_itr_create(&s_owned_jokers_list);
            JokerObject* expired_joker;
            while ((expired_joker = list_itr_next(&joker_itr)) && expired_joker != joker_object)
            {
                expired_joker_idx++;
            }

            // Removing expired Jokers here, instead of immediately like ones we
            // sell or discard allow us to have a small shrink animation without
            // the other owned Jokers rearranging themselves to fill the newly
            // freed space, therefore obscuring the animation
            remove_owned_joker(expired_joker_idx);
            list_itr_remove_current_node(&itr);
            joker_object_destroy(&joker_object);
        }
    }
}

static inline void jokers_update_loop(void)
{
    held_jokers_update_loop();
    discarded_jokers_update_loop();
    expired_jokers_update_loop();
    // Deferred blind-selected joker effects (Riff-Raff spawns, Ceremonial
    // Dagger sacrifice, ...) run through the unified left-to-right queue.
    deferred_effects_process_pending();
}

// ---- One-shot event message auto-clear -------------------------------------
// Messages shown by ON_ROUND_END / ON_BLIND_SELECTED effects (Egg "+$3",
// Riff-Raff "+2 Jokers", ...) are written to the joker score text row but,
// unlike scoring messages, nothing in the state machine erases them later.
// Schedule an erase ~1.5s after the dispatch so they clear themselves.
// Scored row where joker messages are drawn (mirrors PLAYED_CARDS_SCORES_RECT).
#define JOKER_EVENT_TEXT_CLEAR_DELAY FRAMES(90)
static u32 s_joker_event_text_clear_at = 0;

void schedule_joker_event_text_clear(void)
{
    s_joker_event_text_clear_at = g_game_vars.timer + JOKER_EVENT_TEXT_CLEAR_DELAY;
}

static inline void joker_event_text_clear_update_loop(void)
{
    if (s_joker_event_text_clear_at == 0)
        return;
    if (g_game_vars.timer < s_joker_event_text_clear_at)
        return;
    // Scoring is in progress (scoring numbers are being drawn on the same
    // text row): wait for it to finish instead of erasing them early.
    if (get_hand_state() == HAND_PLAYING)
        return;
    tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);
    s_joker_event_text_clear_at = 0;
}

// ---- HUD value roll animation ---------------------------------------------
// Jokers that mutate hands/discards on blind select (Burglar: +3 hands,
// 0 discards) animate the HUD number as a rolling count-up from its
// pre-mutation value to the new value (12 steps x 2 frames ~0.4s), the
// same reading as the chips/mult settlement roll. Text has no scale/affine
// support on GBA, so a digit roll is the cheapest readable "jump" animation.
// Rolling-digit count-up (Burglar +3 hands / discards to 0), like the
// chips/mult settlement roll: the HUD number steps from its pre-trigger
// value to the new value in fixed increments instead of flashing.
#define HUD_ROLL_STEPS 12
#define HUD_ROLL_INTERVAL FRAMES(2)
typedef struct
{
    bool active;
    int from_value;
    int to_value;
    int step;
    u32 next_tick_at;
} HudRollState;
static HudRollState s_hud_roll_hands = {0};
static HudRollState s_hud_roll_discards = {0};

void hud_pulse_hands(int from_value)
{
    // Rolling count-up: from the PRE-mutation value to the new hands count.
    // Caller captures from_value BEFORE mutating g_game_vars.hands.
    s_hud_roll_hands.active = true;
    s_hud_roll_hands.from_value = from_value;
    s_hud_roll_hands.to_value = g_game_vars.hands;
    s_hud_roll_hands.step = 0;
    s_hud_roll_hands.next_tick_at = g_game_vars.timer + HUD_ROLL_INTERVAL;
}

void hud_pulse_discards(int from_value)
{
    s_hud_roll_discards.active = true;
    s_hud_roll_discards.from_value = from_value;
    s_hud_roll_discards.to_value = g_game_vars.discards;
    s_hud_roll_discards.step = 0;
    s_hud_roll_discards.next_tick_at = g_game_vars.timer + HUD_ROLL_INTERVAL;
}

static inline void hud_pulse_update_loop(void)
{
    if (s_hud_roll_hands.active)
    {
        if (g_game_vars.timer >= s_hud_roll_hands.next_tick_at)
        {
            s_hud_roll_hands.step++;
            s_hud_roll_hands.next_tick_at = g_game_vars.timer + HUD_ROLL_INTERVAL;

            if (s_hud_roll_hands.step >= HUD_ROLL_STEPS)
            {
                s_hud_roll_hands.active = false;
                display_hands();
            }
            else
            {
                // Linear interp from_value -> to_value; draw in white so the
                // rolling number reads as an active change (like chips roll).
                int delta = s_hud_roll_hands.to_value - s_hud_roll_hands.from_value;
                int cur = s_hud_roll_hands.from_value +
                          (delta * s_hud_roll_hands.step) / HUD_ROLL_STEPS;
                tte_erase_rect_wrapper(HANDS_TEXT_ERASE_RECT);
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}%d",
                    HANDS_TEXT_RECT.left,
                    HANDS_TEXT_RECT.top,
                    TTE_WHITE_PB,
                    cur
                );
            }
        }
    }
    if (s_hud_roll_discards.active)
    {
        if (g_game_vars.timer >= s_hud_roll_discards.next_tick_at)
        {
            s_hud_roll_discards.step++;
            s_hud_roll_discards.next_tick_at = g_game_vars.timer + HUD_ROLL_INTERVAL;

            if (s_hud_roll_discards.step >= HUD_ROLL_STEPS)
            {
                s_hud_roll_discards.active = false;
                display_discards();
            }
            else
            {
                int delta = s_hud_roll_discards.to_value - s_hud_roll_discards.from_value;
                int cur = s_hud_roll_discards.from_value +
                          (delta * s_hud_roll_discards.step) / HUD_ROLL_STEPS;
                tte_erase_rect_wrapper(DISCARDS_TEXT_ERASE_RECT);
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}%d",
                    DISCARDS_TEXT_RECT.left,
                    DISCARDS_TEXT_RECT.top,
                    TTE_WHITE_PB,
                    cur
                );
            }
        }
    }
}

void game_update()
{
    rng_update();

    g_game_vars.timer++;

    jokers_update_loop();
    joker_event_text_clear_update_loop();
    hud_pulse_update_loop();

    state_machine_update();

    sprite_object_update_all();
}

void game_change_state(enum GameState new_game_state)
{
    g_game_vars.timer = TM_ZERO; // Reset the timer

    state_machine_change_state(&game_sm, new_game_state);
}

enum GameState game_get_state(void)
{
    return game_sm.state;
}

bool is_joker_owned(int joker_id)
{
    ListItr itr = list_itr_create(&s_owned_jokers_list);
    JokerObject* joker;

    while ((joker = list_itr_next(&itr)))
    {
        if (joker->joker->id == joker_id)
        {
            return true;
        }
    }
    return false;
}

List* get_jokers_list(void)
{
    return &s_owned_jokers_list;
}

List* get_expired_jokers_list(void)
{
    return &s_expired_jokers_list;
}

List* get_discarded_jokers_list(void)
{
    return &s_discarded_jokers_list;
}

bool is_shortcut_joker_active(void)
{
    return s_shortcut_joker_count > 0;
}

bool is_smeared_joker_active(void)
{
    return s_smeared_joker_count > 0;
}

bool is_showman_joker_active(void)
{
    return s_showman_joker_count > 0;
}

// Gros Michel / Cavendish food joker pool management
void joker_update_food_pool(void)
{
    if (s_gros_michel_destroyed)
    {
        // Gros Michel was destroyed: remove from pool, add Cavendish
        joker_set_rollable(GROS_MICHEL_ID, false);
        // Only unlock Cavendish if the player doesn't already own it
        // (purchased Cavendish must stay out of the shop pool)
        if (!is_joker_owned(CAVENDISH_ID))
            joker_set_rollable(CAVENDISH_ID, true);
    }
}

bool is_gros_michel_destroyed(void)
{
    return s_gros_michel_destroyed;
}

void set_gros_michel_destroyed(void)
{
    s_gros_michel_destroyed = true;
}

void expire_all_gros_michel(void)
{
    // Demake-specific rule: Gros Michel extinction is GLOBAL. When one
    // Gros Michel self-destructs, every other Gros Michel in play dies
    // with it. Mirror the mechanism used in joker.c (JOKER_EFFECT_FLAG_EXPIRE
    // -> push into the expired list, which triggers the exit animation and
    // eventual removal).
    ListItr itr = list_itr_create(&s_owned_jokers_list);
    JokerObject* joker_object;
    List* expired = get_expired_jokers_list();

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL && joker_object->joker != NULL &&
            joker_object->joker->id == GROS_MICHEL_ID)
        {
            bool already_expired = false;
            ListItr eitr = list_itr_create(expired);
            JokerObject* exp;
            while ((exp = list_itr_next(&eitr)))
            {
                if (exp == joker_object)
                {
                    already_expired = true;
                    break;
                }
            }
            if (!already_expired)
            {
                joker_object_shake(joker_object, UNDEFINED);
                list_push_back(expired, joker_object);
            }
        }
    }
}

// Discarded face card count (used by Jolly Joker)
static int s_discarded_face_card_count = 0;

void set_discarded_face_card_count(int count)
{
    s_discarded_face_card_count = count;
}

int get_discarded_face_card_count(void)
{
    return s_discarded_face_card_count;
}

int get_straight_and_flush_size(void)
{
    return s_four_fingers_joker_count > 0 ? STRAIGHT_AND_FLUSH_SIZE_FOUR_FINGERS
                                          : STRAIGHT_AND_FLUSH_SIZE_DEFAULT;
}

void add_joker(JokerObject* joker_object)
{
    // Defensive: never let the owned rack exceed its capacity. Deferred
    // spawns (Riff-Raff) cap their own count, but other callers exist -
    // an overfull rack would index SPACING_LUT out of bounds.
    if (joker_object == NULL ||
        list_get_len(&s_owned_jokers_list) >= MAX_JOKERS_HELD_SIZE)
    {
        return;
    }

    list_push_back(&s_owned_jokers_list, joker_object);

    // Owned jokers can't be rolled again in the shop
    joker_set_rollable(joker_object->joker->id, false);

    // TODO: Extract to on_joker_added() callback
    // In case the player gets multiple Four Fingers Jokers,
    // only change size when the first one is added
    if (joker_object->joker->id == FOUR_FINGERS_JOKER_ID)
    {
        s_four_fingers_joker_count++;
    }

    if (joker_object->joker->id == SHORTCUT_JOKER_ID)
    {
        s_shortcut_joker_count++;
    }

    if (joker_object->joker->id == SMEARED_JOKER_ID)
    {
        s_smeared_joker_count++;
    }

    if (joker_object->joker->id == SHOWMAN_ID)
    {
        s_showman_joker_count++;
    }
}

void remove_owned_joker(int owned_joker_idx)
{
    // TODO: Extract to on_joker_removed() callback
    JokerObject* joker_object = list_get_at_idx(&s_owned_jokers_list, owned_joker_idx);
    // In case the player gets multiple Four Fingers Jokers,
    // and only reset the size when all of them have been removed
    if (joker_object->joker->id == FOUR_FINGERS_JOKER_ID)
    {
        s_four_fingers_joker_count--;
    }

    if (joker_object->joker->id == SHORTCUT_JOKER_ID)
    {
        s_shortcut_joker_count--;
    }

    if (joker_object->joker->id == SMEARED_JOKER_ID)
    {
        s_smeared_joker_count--;
    }

    if (joker_object->joker->id == SHOWMAN_ID)
    {
        s_showman_joker_count--;
    }

    // TODO: Move to site of joker_destroy()?
    joker_set_rollable(joker_object->joker->id, true);
    list_remove_at_idx(&s_owned_jokers_list, owned_joker_idx);
}

int get_deck_top(void)
{
    return s_deck_top;
}

int get_num_discards_remaining(void)
{
    return g_game_vars.discards;
}

int get_num_hands_remaining(void)
{
    return g_game_vars.hands;
}

void display_money()
{
    Rect money_text_rect = MONEY_TEXT_RECT;
    tte_erase_rect_wrapper(MONEY_TEXT_RECT);

    char money_str_buff[INT_MAX_DIGITS + 2]; // + 2 for null terminator and "$" sign
    snprintf(money_str_buff, sizeof(money_str_buff), "$%ld", g_game_vars.money);

    // Bias left so the number is centered and the "$" sign is on the left
    update_text_rect_to_center_str(&money_text_rect, money_str_buff, SCREEN_LEFT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        money_text_rect.left,
        money_text_rect.top,
        TTE_YELLOW_PB,
        money_str_buff
    );
}

void display_chips(void)
{
    Rect chips_text_rect = CHIPS_TEXT_RECT;

    // In case of overflow, the rect overflow left by 1 char
    Rect chips_text_overflow_rect = chips_text_rect;
    chips_text_overflow_rect.left -= TTE_CHAR_SIZE;
    tte_erase_rect_wrapper(chips_text_overflow_rect);

    char chips_str_buff[UINT_MAX_DIGITS + 1];
    truncate_uint_to_suffixed_str(
        g_game_vars.chips,
        rect_width(&chips_text_rect) / TTE_CHAR_SIZE,
        chips_str_buff
    );

    update_text_rect_to_right_align_str(&chips_text_rect, chips_str_buff, OVERFLOW_LEFT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000;}%s",
        chips_text_rect.left,
        chips_text_rect.top,
        TTE_WHITE_PB,
        chips_str_buff
    );
    toggle_flaming_score();
}

void display_mult(void)
{
    Rect mult_text_overflow_rect = MULT_TEXT_RECT;
    // In case of overflow the rect will overflow right by 1 char
    mult_text_overflow_rect.right += TTE_CHAR_SIZE;
    tte_erase_rect_wrapper(mult_text_overflow_rect);

    char mult_str_buff[UINT_MAX_DIGITS + 1];
    truncate_uint_to_suffixed_str(
        g_game_vars.mult,
        rect_width(&MULT_TEXT_RECT) / TTE_CHAR_SIZE,
        mult_str_buff
    );

    tte_printf(
        "#{P:%d,%d; cx:0x%X000;}%s",
        MULT_TEXT_RECT.left,
        MULT_TEXT_RECT.top,
        TTE_WHITE_PB,
        mult_str_buff
    );

    toggle_flaming_score();
}

void display_deck_size_max(void)
{
    // TODO: the text will overflow if deck max size exceeds 99,
    // we will need a fix at some point for this
    tte_erase_rect_wrapper(DECK_SIZE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d/%d",
        DECK_SIZE_RECT.left,
        DECK_SIZE_RECT.top,
        TTE_WHITE_PB,
        deck_get_size(),
        deck_get_max_size()
    );
}

// Returns true if the card is *considered* a face card
bool card_is_face(Card* card)
{
    // Card is a face card, or Pareidolia is present
    return (
        card->rank == JACK || card->rank == QUEEN || card->rank == KING ||
        is_joker_owned(PAREIDOLIA_JOKER_ID)
    );
}

void display_temp_score(u32 value)
{
    char temp_score_str_buff[UINT_MAX_DIGITS + 1];
    Rect temp_score_rect = TEMP_SCORE_RECT;
    truncate_uint_to_suffixed_str(
        value,
        rect_width(&temp_score_rect) / TTE_CHAR_SIZE,
        temp_score_str_buff
    );
    update_text_rect_to_center_str(&temp_score_rect, temp_score_str_buff, SCREEN_RIGHT);

    tte_erase_rect_wrapper(TEMP_SCORE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        temp_score_rect.left,
        temp_score_rect.top,
        TTE_WHITE_PB,
        temp_score_str_buff
    );
}

void erase_temp_score(void)
{
    tte_erase_rect_wrapper(TEMP_SCORE_RECT);
}

void display_score(u32 value)
{
    Rect score_rect = SCORE_RECT;
    // Clear the existing text before redrawing
    tte_erase_rect_wrapper(SCORE_RECT);

    char score_str_buff[UINT_MAX_DIGITS + 1];

    truncate_uint_to_suffixed_str(value, rect_width(&score_rect) / TTE_CHAR_SIZE, score_str_buff);
    update_text_rect_to_center_str(&score_rect, score_str_buff, SCREEN_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        score_rect.left,
        score_rect.top,
        TTE_WHITE_PB,
        score_str_buff
    );
}

void display_round(void)
{
    // tte_erase_rect_wrapper(ROUND_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%ld",
        ROUND_TEXT_RECT.left,
        ROUND_TEXT_RECT.top,
        TTE_YELLOW_PB,
        g_game_vars.round
    );
}

void display_hands(void)
{
    // Erase first: tte_printf overwrites but does NOT clear stale digits,
    // so 10 -> 9 would render "90" (the '0' lingers). Fixed-width erase
    // area covers up to 3 digits.
    tte_erase_rect_wrapper(HANDS_TEXT_ERASE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%ld",
        HANDS_TEXT_RECT.left,
        HANDS_TEXT_RECT.top,
        TTE_BLUE_PB,
        g_game_vars.hands
    );
}

void display_discards(void)
{
    tte_erase_rect_wrapper(DISCARDS_TEXT_ERASE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%ld",
        DISCARDS_TEXT_RECT.left,
        DISCARDS_TEXT_RECT.top,
        TTE_RED_PB,
        g_game_vars.discards
    );
}

int deck_get_size(void)
{
    return s_deck_top + 1;
}

static int deck_get_max_size(void)
{
    // This is the max amount of cards that the player currently has in their possession
    return get_hand_top() + get_played_top() + s_deck_top + get_discard_top() + 4;
}

void deck_shuffle(void)
{
    for (int i = s_deck_top; i > 0; i--)
    {
        int j = rng_get_u32(RNG_SEQ_CARD_SHUFFLE) % (i + 1);
        Card* temp = s_deck[i];
        s_deck[i] = s_deck[j];
        s_deck[j] = temp;
    }
}

void game_start(void)
{
    affine_background_change_background(AFFINE_BG_GAME);
    tte_colors_setup();

    g_game_vars.hands = MAX_HANDS;
    g_game_vars.discards = MAX_DISCARDS;

    // Fill the deck with all the cards. Later on this can be replaced with a more dynamic system
    // that allows for different decks and card types.
    for (int suit = 0; suit < NUM_SUITS; suit++)
    {
        for (int rank = 0; rank < NUM_RANKS; rank++)
        {
            Card* card = card_new(suit, rank);
            deck_push(card);
        }
    }

    change_background(BG_BLIND_SELECT, false);

    // Deck size/max size
    tte_erase_rect_wrapper(DECK_SIZE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d/%d",
        DECK_SIZE_RECT.left,
        DECK_SIZE_RECT.top,
        TTE_WHITE_PB,
        deck_get_size(),
        deck_get_max_size()
    );

    display_round();                  // Set the round display
    display_score(g_game_vars.score); // Set the score display

    display_chips(); // Set the chips display
    display_mult();  // Set the multiplier display

    display_hands();    // Hand
    display_discards(); // Discard

    display_money(); // Set the money display
    display_ante();

    game_change_state(GAME_STATE_BLIND_SELECT);
}
