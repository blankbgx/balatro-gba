/**
 * @file layout.h
 *
 * @brief Header of shared rects and points needed for ui
 */
#ifndef LAYOUT_H
#define LAYOUT_H

#include "graphic_utils.h"

// clang-format off
// Points                                                 x        y
static const BG_POINT CUR_BLIND_TOKEN_POS              = {8,       18};
static const BG_POINT TOP_LEFT_PANEL_POINT             = {0,       0};
static const BG_POINT ROUND_END_REWARDS_ELLIPSIS_POS   = {10,      13};
static const BG_POINT JOKER_DISCARD_TARGET             = {240,     30};
static const BG_POINT HELD_JOKERS_POS                  = {108,     10};
// Rects                                                  left     top     right   bottom
static const Rect TOP_LEFT_PANEL_ANIM_RECT             = {0,       0,      8,      4};
static const Rect POP_MENU_ANIM_RECT                   = {9,       7,      24,     31};
static const Rect DECK_ANIM_RECT                       = {25,      14,     28,     23}; // Can be used for the Shop and Blind Select screen
static const Rect TOP_LEFT_ITEM_SRC_RECT               = {0,       20,     8,      25};
static const Rect TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT = {0,       28,     8,      28};
static const Rect BLIND_REWARD_RECT                    = {40,      32,     64,     40};
static const Rect BLIND_REQ_TEXT_RECT                  = {32,      24,     64,     32};
static const Rect PLAYING_SCREEN_RECT                  = {72,      0,      240,    160};
static const Rect HAND_SIZE_RECT                       = {128,     128,    152,    160}; // Seems to include both SELECT and PLAYING
// Joker score text rows (y=48: below jokers; y=108: below held cards)
static const Rect PLAYED_CARDS_SCORES_RECT             = {72,      48,     240,    56};
static const Rect HELD_CARDS_SCORES_RECT               = {72,      108,    240,    116};
// Hands/discards HUD numbers (top-left HUD) - shared by game.c
// (display_hands/display_discards) and the generic value-roll queue
// (hud_enqueue_value_roll callers in joker_effects.c). TEXT_RECT only
// feeds tte_printf #{P:x,y} (left/top used); right/bottom are real
// values so no UNDEFINED (util.h) dependency leaks into layout.h.
static const Rect HANDS_TEXT_RECT           = {16,      104,    40,     112 };
static const Rect DISCARDS_TEXT_RECT        = {48,      104,    72,     112 };
// Erase area for hands/discards: fixed 3-char width so a value shrinking
// from 2-3 digits to 1 digit (e.g. 10 -> 9) fully clears the old digits
// instead of leaving a stale char ("90" after "10"->"9"). Height = 1 char.
// Vertical slide distance (px) for the directional roll: increasing
// values slide in from below (up), decreasing from above (down).
#define HUD_ROLL_DY 5
static const Rect HANDS_TEXT_ERASE_RECT     = {16,      104,    16 + 3 * TTE_CHAR_SIZE, 104 + TTE_CHAR_SIZE };
static const Rect DISCARDS_TEXT_ERASE_RECT  = {48,      104,    48 + 3 * TTE_CHAR_SIZE, 104 + TTE_CHAR_SIZE };
// Roll animation erase area: extends HUD_ROLL_DY px above/below the text
// so the sliding digit (directional roll) never leaves a stale trail.
static const Rect HANDS_TEXT_ROLL_ERASE_RECT = {16,      104 - HUD_ROLL_DY, 16 + 3 * TTE_CHAR_SIZE, 104 + TTE_CHAR_SIZE + HUD_ROLL_DY };
static const Rect DISCARDS_TEXT_ROLL_ERASE_RECT = {48,   104 - HUD_ROLL_DY, 48 + 3 * TTE_CHAR_SIZE, 104 + TTE_CHAR_SIZE + HUD_ROLL_DY };
// clang-format on

#endif // LAYOUT_H
