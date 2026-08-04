#include "game.h"
#include "game/round.h"
#include "game_variables.h"
#include "hand.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "pool.h"
#include "random.h"
#include "soundbank.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

// Score text display row for jokers (mirrors source/joker.c JOKER_SCORE_TEXT_Y)
#define JOKER_SCORE_TEXT_Y 48
#include <string.h>

#define MISPRINT_MAX_MULT 23

#define REGISTER_JOKER_DESC_FUNC(joker_desc_name) \
    static int joker_desc_name(Joker* joker, Rect dest_rect);

#define REGISTER_JOKER_EFFECT_FUNC(joker_effect_name) \
    static u32 joker_effect_name(                     \
        Joker* joker,                                 \
        Card* scored_card,                            \
        enum JokerEvent joker_event,                  \
        JokerEffect** joker_effect                    \
    );

#define SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, restricted_event, checked_event) \
    if (checked_event != restricted_event || scored_card == NULL)                   \
    {                                                                               \
        return JOKER_EFFECT_FLAG_NONE;                                              \
    }
#define SCORE_ON_EVENT_ONLY(restricted_event, checked_event) \
    if (checked_event != restricted_event)                   \
    {                                                        \
        return JOKER_EFFECT_FLAG_NONE;                       \
    }

static JokerEffect s_shared_joker_effect = {0};

// Flag: true when a Copying Joker (Blueprint/Brainstorm) is calling
// another joker's effect function. Used by Wee Joker to distinguish
// normal scoring (accumulate chips) from copy mode (read-only).
bool s_is_copying_joker = false;

// Pointer to the original joker being copied by Blueprint/Brainstorm.
// Used by Wee Joker to read the original's accumulated scoring_state.
Joker* s_copied_joker_source = NULL;

// Joker Descriptions

REGISTER_JOKER_DESC_FUNC(default_joker_desc)
REGISTER_JOKER_DESC_FUNC(greedy_joker_desc)
REGISTER_JOKER_DESC_FUNC(lusty_joker_desc)
REGISTER_JOKER_DESC_FUNC(wrathful_joker_desc)
REGISTER_JOKER_DESC_FUNC(gluttonous_joker_desc)
REGISTER_JOKER_DESC_FUNC(jolly_joker_desc)
REGISTER_JOKER_DESC_FUNC(zany_joker_desc)
REGISTER_JOKER_DESC_FUNC(mad_joker_desc)
REGISTER_JOKER_DESC_FUNC(crazy_joker_desc)
REGISTER_JOKER_DESC_FUNC(droll_joker_desc)
REGISTER_JOKER_DESC_FUNC(sly_joker_desc)
REGISTER_JOKER_DESC_FUNC(wily_joker_desc)
REGISTER_JOKER_DESC_FUNC(clever_joker_desc)
REGISTER_JOKER_DESC_FUNC(devious_joker_desc)
REGISTER_JOKER_DESC_FUNC(crafty_joker_desc)
REGISTER_JOKER_DESC_FUNC(half_joker_desc)
REGISTER_JOKER_DESC_FUNC(stencil_joker_desc)
REGISTER_JOKER_DESC_FUNC(misprint_joker_desc)
REGISTER_JOKER_DESC_FUNC(walkie_talkie_joker_desc)
REGISTER_JOKER_DESC_FUNC(fibonnaci_joker_desc)
REGISTER_JOKER_DESC_FUNC(banner_joker_desc)
REGISTER_JOKER_DESC_FUNC(mystic_summit_joker_desc)
REGISTER_JOKER_DESC_FUNC(blackboard_joker_desc)
REGISTER_JOKER_DESC_FUNC(blue_joker_desc)
REGISTER_JOKER_DESC_FUNC(raised_fist_joker_desc)
REGISTER_JOKER_DESC_FUNC(reserved_parking_joker_desc)
REGISTER_JOKER_DESC_FUNC(business_card_joker_desc)
REGISTER_JOKER_DESC_FUNC(scholar_joker_desc)
REGISTER_JOKER_DESC_FUNC(scary_face_joker_desc)
REGISTER_JOKER_DESC_FUNC(abstract_joker_desc)
REGISTER_JOKER_DESC_FUNC(bull_joker_desc)
REGISTER_JOKER_DESC_FUNC(smiley_face_joker_desc)
REGISTER_JOKER_DESC_FUNC(even_steven_joker_desc)
REGISTER_JOKER_DESC_FUNC(odd_todd_joker_desc)
REGISTER_JOKER_DESC_FUNC(acrobat_joker_desc)
REGISTER_JOKER_DESC_FUNC(hanging_chad_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_duo_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_trio_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_family_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_order_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_tribe_joker_desc)
REGISTER_JOKER_DESC_FUNC(bootstraps_joker_desc)
REGISTER_JOKER_DESC_FUNC(shoot_the_moon_joker_desc)
REGISTER_JOKER_DESC_FUNC(pareidolia_joker_desc)
REGISTER_JOKER_DESC_FUNC(photograph_joker_desc)
REGISTER_JOKER_DESC_FUNC(dusk_joker_desc)
REGISTER_JOKER_DESC_FUNC(shortcut_joker_desc)
REGISTER_JOKER_DESC_FUNC(blueprint_joker_desc)
REGISTER_JOKER_DESC_FUNC(brainstorm_joker_desc)
REGISTER_JOKER_DESC_FUNC(hack_joker_desc)
REGISTER_JOKER_DESC_FUNC(four_fingers_joker_desc)
REGISTER_JOKER_DESC_FUNC(seltzer_joker_desc)
REGISTER_JOKER_DESC_FUNC(sock_and_buskin_joker_desc)

// New joker descriptions
REGISTER_JOKER_DESC_FUNC(wee_joker_desc)
REGISTER_JOKER_DESC_FUNC(riff_raff_joker_desc)
REGISTER_JOKER_DESC_FUNC(baron_joker_desc)
REGISTER_JOKER_DESC_FUNC(mime_joker_desc)
REGISTER_JOKER_DESC_FUNC(egg_joker_desc)
REGISTER_JOKER_DESC_FUNC(smeared_joker_desc)
REGISTER_JOKER_DESC_FUNC(faceless_joker_desc)
REGISTER_JOKER_DESC_FUNC(gros_michel_joker_desc)
REGISTER_JOKER_DESC_FUNC(cavendish_joker_desc)
REGISTER_JOKER_DESC_FUNC(flower_pot_desc)
REGISTER_JOKER_DESC_FUNC(loyalty_card_joker_desc)
REGISTER_JOKER_DESC_FUNC(riding_the_bus_joker_desc)
REGISTER_JOKER_DESC_FUNC(ceremonial_dagger_joker_desc)

// Joker Effect functions

static u32 sinful_joker_effect(
    Card* scored_card,
    u8 sinful_suit,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

REGISTER_JOKER_EFFECT_FUNC(joker_effect_noop)
REGISTER_JOKER_EFFECT_FUNC(default_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(greedy_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(lusty_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(wrathful_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(gluttonous_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(jolly_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(zany_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(mad_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(crazy_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(droll_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(sly_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(wily_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(clever_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(devious_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(crafty_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(half_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(stencil_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(misprint_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(walkie_talkie_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(fibonnaci_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(banner_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(mystic_summit_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(blackboard_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(blue_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(raised_fist_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(reserved_parking_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(business_card_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(scholar_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(scary_face_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(abstract_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(bull_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(smiley_face_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(even_steven_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(odd_todd_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(acrobat_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(hanging_chad_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_duo_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_trio_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_family_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_order_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_tribe_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(bootstraps_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(shoot_the_moon_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(photograph_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(dusk_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(blueprint_brainstorm_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(hack_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(seltzer_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(sock_and_buskin_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(wee_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(riff_raff_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(baron_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(mime_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(egg_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(smeared_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(faceless_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(gros_michel_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(cavendish_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(flower_pot_effect)
REGISTER_JOKER_EFFECT_FUNC(loyalty_card_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(riding_the_bus_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(ceremonial_dagger_joker_effect)

// clang-format off
/* The index of a joker in the registry matches its ID.
 *
 * The joker sprites are matched by ID so the position in the registry
 * determines the joker's sprite.
 *
 * Each consecutive NUM_JOKERS_PER_SPRITESHEET (defined in joker.c) jokers
 * share a spritesheet and thus a color palette.
 *
 * To make better use of color palettes jokers may be rearranged here
 * (and put together in the matching spritesheet) to share a color palette.
 * Otherwise the order is similar to the wiki.
 *
 * TODO: move Name and Description printing out of this when the CardInstance is implemented.
 */
// clang-format off
const JokerInfo joker_registry[] = 
{
    // Spritesheet 0
    { "Joker",            COMMON_JOKER,    2, false, default_joker_desc,          default_joker_effect              }, // DEFAULT_JOKER_ID = 0
    { "Abstract Joker",   COMMON_JOKER,    4, false, abstract_joker_desc,         abstract_joker_effect             }, // 1
    { "Half Joker",       COMMON_JOKER,    5, false, half_joker_desc,             half_joker_effect                 }, // 2
    { "Misprint",         COMMON_JOKER,    4, true,  misprint_joker_desc,         misprint_joker_effect             }, // 3
    { "Scary Face",       COMMON_JOKER,    4, false, scary_face_joker_desc,       scary_face_joker_effect           }, // 4
    { "Sock and Buskin",  UNCOMMON_JOKER,  6, false, sock_and_buskin_joker_desc,  sock_and_buskin_joker_effect      }, // 5
    { "Acrobat",          UNCOMMON_JOKER,  6, false, acrobat_joker_desc,          acrobat_joker_effect              }, // 6
    { "Fibonacci",        UNCOMMON_JOKER,  8, false, fibonnaci_joker_desc,        fibonnaci_joker_effect            }, // 7
    { "Scholar",          COMMON_JOKER,    4, false, scholar_joker_desc,          scholar_joker_effect              }, // 8
    { "Crafty Joker",     COMMON_JOKER,    4, false, crafty_joker_desc,           crafty_joker_effect               }, // 9
    { "Droll Joker",      COMMON_JOKER,    4, false, droll_joker_desc,            droll_joker_effect                }, // 10
    { "Raised Fist",      COMMON_JOKER,    5, false, raised_fist_joker_desc,      raised_fist_joker_effect          }, // 11
    { "Reserved Parking", COMMON_JOKER,    6, false, reserved_parking_joker_desc, reserved_parking_joker_effect     }, // 12
    { "Business Card",    COMMON_JOKER,    4, false, business_card_joker_desc,    business_card_joker_effect        }, // 13
    { "Hanging Chad",     COMMON_JOKER,    4, false, hanging_chad_joker_desc,     hanging_chad_joker_effect         }, // 14
    { "Joker Stencil",    UNCOMMON_JOKER,  8, false, stencil_joker_desc,          stencil_joker_effect              }, // 15
    { "Banner",           COMMON_JOKER,    5, false, banner_joker_desc,           banner_joker_effect               }, // 16
    { "Shoot the Moon",   COMMON_JOKER,    5, false, shoot_the_moon_joker_desc,   shoot_the_moon_joker_effect,      }, // 17
    // Spritesheet 1 
    { "Greedy Joker",     COMMON_JOKER,    5, false, greedy_joker_desc,           greedy_joker_effect               }, // 18
    { "Lusty Joker",      COMMON_JOKER,    5, false, lusty_joker_desc,            lusty_joker_effect                }, // 19
    // Spritesheet 2
    { "Wrathful Joker",   COMMON_JOKER,    5, false, wrathful_joker_desc,         wrathful_joker_effect             }, // 20
    { "Gluttonous Joker", COMMON_JOKER,    5, false, gluttonous_joker_desc,       gluttonous_joker_effect           }, // 21
    // Spritesheet 3
    { "Crazy Joker",      COMMON_JOKER,    4, false, crazy_joker_desc,            crazy_joker_effect                }, // 22
    { "Mad Joker",        COMMON_JOKER,    4, false, mad_joker_desc,              mad_joker_effect                  }, // 23
    { "Clever Joker",     COMMON_JOKER,    4, false, clever_joker_desc,           clever_joker_effect               }, // 24
    { "Devious Joker",    COMMON_JOKER,    4, false, devious_joker_desc,          devious_joker_effect              }, // 25
    { "Even Steven",      COMMON_JOKER,    4, false, even_steven_joker_desc,      even_steven_joker_effect          }, // 26
    // Spritesheet 4
    { "Blackboard",       UNCOMMON_JOKER,  6, false, blackboard_joker_desc,       blackboard_joker_effect           }, // 27
    { "Mystic Summit",    COMMON_JOKER,    5, false, mystic_summit_joker_desc,    mystic_summit_joker_effect        }, // 28
    { "Walkie Talkie",    COMMON_JOKER,    4, false, walkie_talkie_joker_desc,    walkie_talkie_joker_effect        }, // 29
    { "Zany Joker",       COMMON_JOKER,    4, false, zany_joker_desc,             zany_joker_effect                 }, // 30
    { "Wily Joker",       COMMON_JOKER,    4, false, wily_joker_desc,             wily_joker_effect                 }, // 31
    // Spritesheet 5
    { "Sly Joker",        COMMON_JOKER,    3, false, sly_joker_desc,              sly_joker_effect                  }, // 32
    { "Jolly Joker",      COMMON_JOKER,    3, false, jolly_joker_desc,            jolly_joker_effect                }, // 33
    { "Blue Joker",       COMMON_JOKER,    5, false, blue_joker_desc,             blue_joker_effect                 }, // 34
    { "Odd Todd",         COMMON_JOKER,    4, false, odd_todd_joker_desc,         odd_todd_joker_effect             }, // 35
    // Spritesheet 6
    { "The Duo",          RARE_JOKER,      8, false, the_duo_joker_desc,          the_duo_joker_effect              }, // 36
    { "The Trio",         RARE_JOKER,      8, false, the_trio_joker_desc,         the_trio_joker_effect             }, // 37
    { "The Order",        RARE_JOKER,      8, false, the_order_joker_desc,        the_order_joker_effect            }, // 38
    { "The Tribe",        RARE_JOKER,      8, false, the_tribe_joker_desc,        the_tribe_joker_effect            }, // 39
    // Spritesheet 7
    { "The Family",       RARE_JOKER,      8, false, the_family_joker_desc,       the_family_joker_effect           }, // 40
    { "Brainstorm",       RARE_JOKER,     10, false, brainstorm_joker_desc,       blueprint_brainstorm_joker_effect }, // 41 Brainstorm
    // Spritesheet 8
    { "Smiley Face",      COMMON_JOKER,    4, false, smiley_face_joker_desc,      smiley_face_joker_effect          }, // 42
    { "Bull",             UNCOMMON_JOKER,  6, false, bull_joker_desc,             bull_joker_effect                 }, // 43
    // Individual Jokers (for now :3)
    { "Photograph",       COMMON_JOKER,    5, false, photograph_joker_desc,       photograph_joker_effect,          }, // 44
    { "Hack",             UNCOMMON_JOKER,  6, false, hack_joker_desc,             hack_joker_effect                 }, // 45
    { "Pareidolia",       UNCOMMON_JOKER,  5, false, pareidolia_joker_desc,       joker_effect_noop                 }, // 46 Pareidolia
    { "Bootstraps",       UNCOMMON_JOKER,  7, false, bootstraps_joker_desc,       bootstraps_joker_effect           }, // 47
    { "Shortcut",         UNCOMMON_JOKER,  7, false, shortcut_joker_desc,         joker_effect_noop,                }, // 48 Shortcut
    { "Dusk",             UNCOMMON_JOKER,  5, false, dusk_joker_desc,             dusk_joker_effect                 }, // 49
    { "Four Fingers",     UNCOMMON_JOKER,  7, false, four_fingers_joker_desc,     joker_effect_noop,                }, // 50 Four Fingers
    { "Seltzer",          UNCOMMON_JOKER,  6, false, seltzer_joker_desc,          seltzer_joker_effect,             }, // 51
    { "Blueprint",        RARE_JOKER,     10, false, blueprint_joker_desc,        blueprint_brainstorm_joker_effect }, // 52 Blueprint

    // Spritesheet 18 (my_joker)
    { "Wee Joker",     RARE_JOKER,      8, true,  wee_joker_desc, wee_joker_effect              }, // 53 Wee Joker
    { "Riff-Raff",     COMMON_JOKER,    6, false, riff_raff_joker_desc, riff_raff_joker_effect        }, // 54 Riff-Raff
    { "Baron",         RARE_JOKER,      8, false, baron_joker_desc, baron_joker_effect            }, // 55 Baron
    { "Mime",          UNCOMMON_JOKER,  5, false, mime_joker_desc, mime_joker_effect             }, // 56 Mime
    { "Egg",           COMMON_JOKER,    4, false, egg_joker_desc, egg_joker_effect              }, // 57 Egg
    { "Smeared Joker", UNCOMMON_JOKER,  7, false, smeared_joker_desc, smeared_joker_effect        }, // 58 Smeared Joker
    { "Faceless Joker", COMMON_JOKER,    5, false, faceless_joker_desc, faceless_joker_effect     }, // 59 Faceless Joker
    { "Gros Michel",   COMMON_JOKER,    5, false, gros_michel_joker_desc, gros_michel_joker_effect }, // 60 Gros Michel
    { "Cavendish",     COMMON_JOKER,    5, false, cavendish_joker_desc, cavendish_joker_effect     }, // 61 Cavendish
        { "Flower Pot",    UNCOMMON_JOKER,  6, false, flower_pot_desc, flower_pot_effect              }, // 62 Flower Pot
        { "Loyalty Card",  UNCOMMON_JOKER,  5, false, loyalty_card_joker_desc, loyalty_card_joker_effect }, // 63 Loyalty Card
        { "Riding the Bus", COMMON_JOKER,   6, false, riding_the_bus_joker_desc, riding_the_bus_joker_effect }, // 64 Riding the Bus
        { "Ceremonial Dagger", UNCOMMON_JOKER, 6, false, ceremonial_dagger_joker_desc, ceremonial_dagger_joker_effect }, // 65 Ceremonial Dagger

        // The following jokers
    // uncomment them when their sprites are added.
#if 0
#endif
};
// clang-format on

static const size_t joker_registry_size = NUM_ELEM_IN_ARR(joker_registry);

const JokerInfo* get_joker_registry_entry(int joker_id)
{
    if (joker_id < 0 || (size_t)joker_id >= joker_registry_size)
    {
        return NULL;
    }
    return &joker_registry[joker_id];
}

size_t get_joker_registry_size(void)
{
    return joker_registry_size;
}

#pragma region JOKER DESCRIPTIONS

static int default_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG "+4 " TTE_BLACK_TAG "Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int greedy_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_DIAMOND_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int lusty_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_HEART_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int wrathful_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_SPADE_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int gluttonous_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_CLUB_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int jolly_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "+8 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int zany_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "+12 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Three of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int mad_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "+10 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Two Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int crazy_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "+12 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Straight";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int droll_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "+10 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Flush";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int sly_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLUE_TAG "+50 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int wily_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLUE_TAG
        "+100 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Three of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int clever_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLUE_TAG
        "+80 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Two Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int devious_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLUE_TAG
        "+100 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Straight";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int crafty_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLUE_TAG "+80 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Flush";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int half_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "+20 " TTE_BLACK_TAG "Mult if played hand contains " TTE_YELLOW_TAG
                    "3 " TTE_BLACK_TAG "or fewer cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int stencil_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_RED_TAG "X1 " TTE_BLACK_TAG
                    "Mult for each empty Joker slot Joker Stencil included\n\n(Now " TTE_RED_TAG
                    "X%ld " TTE_BLACK_TAG "Mult)";
    const u32 desc_max_size = 130;

    List* jokers = get_jokers_list();
    u32 stencil_bonus = MAX_JOKERS_HELD_SIZE - list_get_len(jokers);

    ListItr itr = list_itr_create(jokers);
    JokerObject* joker_object;
    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object->joker->id == STENCIL_JOKER_ID)
            stencil_bonus++;
    }

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, stencil_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int misprint_joker_desc(Joker* joker, Rect dest_rect)
{
    // TODO: print glitchy desc with occasional next card reveal
    char desc[] = TTE_YELLOW_TAG "Random" TTE_BLACK_TAG " Mult between " TTE_RED_TAG
                                 "+0" TTE_BLACK_TAG " and " TTE_RED_TAG "+23";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int walkie_talkie_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each played " TTE_YELLOW_TAG "10 " TTE_BLACK_TAG "or " TTE_YELLOW_TAG
                      "4 " TTE_BLACK_TAG "gives " TTE_BLUE_TAG "+10 " TTE_BLACK_TAG
                      "Chips and " TTE_RED_TAG "+4 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int fibonnaci_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each played\n" TTE_YELLOW_TAG "Ace" TTE_BLACK_TAG ", " TTE_YELLOW_TAG
                      "2" TTE_BLACK_TAG ", " TTE_YELLOW_TAG "3" TTE_BLACK_TAG ", " TTE_YELLOW_TAG
                      "5" TTE_BLACK_TAG ", " TTE_YELLOW_TAG "8\n" TTE_BLACK_TAG "gives " TTE_RED_TAG
                      "+8 " TTE_BLACK_TAG "Mult when scored";

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int banner_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLUE_TAG "+30 " TTE_BLACK_TAG "Chips for each remaining " TTE_YELLOW_TAG "discard";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int mystic_summit_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG "+15 " TTE_BLACK_TAG "Mult when " TTE_YELLOW_TAG
                                           "0 " TTE_BLACK_TAG "discards remaining ";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int blackboard_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "X3 " TTE_BLACK_TAG "Mult if all cards held in hand are " TTE_SPADE_TAG TTE_BLACK_TAG
        "or " TTE_CLUB_TAG;
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int blue_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_BLUE_TAG "+2 " TTE_BLACK_TAG "Chips for each remaining card in " TTE_YELLOW_TAG
                     "deck" TTE_BLACK_TAG "\n\n(Now " TTE_BLUE_TAG "+%ld" TTE_BLACK_TAG " Chips)";
    const u32 desc_max_size = 139;

    u32 blue_bonus = (get_deck_top() + 1) * 2;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, blue_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int raised_fist_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Adds " TTE_YELLOW_TAG "double" TTE_BLACK_TAG " the rank of " TTE_YELLOW_TAG
                      "lowest" TTE_BLACK_TAG " ranked card held in hand to Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int reserved_parking_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Each " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " card held in hand has a " TTE_GREEN_TAG
        "1 in 2" TTE_BLACK_TAG " chance to give " TTE_YELLOW_TAG "$1";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int business_card_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards have a " TTE_GREEN_TAG
        "1 in 2" TTE_BLACK_TAG " chance to give " TTE_YELLOW_TAG "$2" TTE_BLACK_TAG " when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int scholar_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Played " TTE_YELLOW_TAG "Aces" TTE_BLACK_TAG " give " TTE_BLUE_TAG "+20" TTE_BLACK_TAG
        " Chips and " TTE_RED_TAG "+4" TTE_BLACK_TAG " Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int scary_face_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards give " TTE_BLUE_TAG
                      "+30" TTE_BLACK_TAG " Chips when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int abstract_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_RED_TAG "+3" TTE_BLACK_TAG " Mult for each " TTE_YELLOW_TAG "Joker" TTE_BLACK_TAG
                    " card\n\n(Now " TTE_RED_TAG "+%ld" TTE_BLACK_TAG " Mult)";
    const u32 desc_max_size = 125;

    u32 abstract_bonus = list_get_len(get_jokers_list()) * 3;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, abstract_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int bull_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_BLUE_TAG "+2" TTE_BLACK_TAG " Chips for each " TTE_YELLOW_TAG "$1" TTE_BLACK_TAG
                     " you have\n\n(Now " TTE_BLUE_TAG "+%ld" TTE_BLACK_TAG " Chips)";
    const u32 desc_max_size = 127;

    u32 bull_bonus = (g_game_vars.money > 0) ? g_game_vars.money * 2 : 0;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, bull_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int smiley_face_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards give " TTE_RED_TAG
                      "+5" TTE_BLACK_TAG " Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int even_steven_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_YELLOW_TAG "even rank give " TTE_RED_TAG
                      "+4" TTE_BLACK_TAG " Mult when scored\n(10, 8, 6, 4, 2)";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int odd_todd_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_YELLOW_TAG "odd rank give " TTE_BLUE_TAG
                      "+31" TTE_BLACK_TAG " Chips when scored\n(A, 9, 7, 5, 3)";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int acrobat_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG "X3" TTE_BLACK_TAG " Mult on " TTE_YELLOW_TAG
                                           "final hand" TTE_BLACK_TAG " of round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int hanging_chad_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG "Retrigger " TTE_YELLOW_TAG "first" TTE_BLACK_TAG
                                             " played card used in scoring " TTE_YELLOW_TAG
                                             "2" TTE_BLACK_TAG " additional times ";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_duo_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "X2" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_trio_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "X3" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Three of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_family_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "X4" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Four of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_order_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "X3" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Straight";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_tribe_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "X2" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Flush";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int bootstraps_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_RED_TAG "+2" TTE_BLACK_TAG " Mult for every " TTE_YELLOW_TAG "$5" TTE_BLACK_TAG
                    " you have\n\n(Now " TTE_RED_TAG "+%ld" TTE_BLACK_TAG " Mult)";
    const u32 desc_max_size = 125;

    u32 bootstrap_bonus = (g_game_vars.money > 0) ? (g_game_vars.money / 5) * 2 : 0;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, bootstrap_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int shoot_the_moon_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each " TTE_YELLOW_TAG "Queen" TTE_BLACK_TAG
                      " held in hand gives " TTE_RED_TAG "+13" TTE_BLACK_TAG " Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int photograph_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "First played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " card gives " TTE_RED_TAG
                      "X2" TTE_BLACK_TAG " Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int dusk_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG "Retrigger all played cards in " TTE_YELLOW_TAG
                                             "final hand" TTE_BLACK_TAG " of the round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int brainstorm_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Copies ability of the leftmost " TTE_YELLOW_TAG "Joker";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int blueprint_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Copies ability of " TTE_YELLOW_TAG "Joker" TTE_BLACK_TAG " to the right";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int hack_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Retrigger each played " TTE_YELLOW_TAG "2" TTE_BLACK_TAG ", " TTE_YELLOW_TAG
        "3" TTE_BLACK_TAG ", " TTE_YELLOW_TAG "4" TTE_BLACK_TAG ", or " TTE_YELLOW_TAG "5";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int seltzer_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] = TTE_BLACK_TAG
        "Retrigger all cards played for the next " TTE_YELLOW_TAG "%ld" TTE_BLACK_TAG " hands";
    const u32 desc_max_size = 94;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, joker->persistent_state);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int sock_and_buskin_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Retrigger all played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int pareidolia_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "All cards are considered " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int shortcut_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Allows " TTE_YELLOW_TAG "Straights" TTE_BLACK_TAG
                      " to be made with gaps of " TTE_YELLOW_TAG "1 rank" TTE_BLACK_TAG
                      "\n\n(ex: " TTE_YELLOW_TAG "10 8 6 5 3" TTE_BLACK_TAG ")";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int four_fingers_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "All " TTE_YELLOW_TAG "Flushes" TTE_BLACK_TAG " and " TTE_YELLOW_TAG
                      "Straights" TTE_BLACK_TAG " can be made with 4 cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

#pragma endregion

#pragma region JOKER EFFECTS

static u32 joker_effect_noop(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return JOKER_EFFECT_FLAG_NONE;
}

static u32 default_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)
    *joker_effect = &s_shared_joker_effect;

    (*joker_effect)->mult = 4;

    return JOKER_EFFECT_FLAG_MULT;
}

static u32 sinful_joker_effect(
    Card* scored_card,
    u8 sinful_suit,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->suit == sinful_suit)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }
    return effect_flags_ret;
}

static u32 greedy_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, DIAMONDS, joker_event, joker_effect);
}

static u32 lusty_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, HEARTS, joker_event, joker_effect);
}

static u32 wrathful_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, SPADES, joker_event, joker_effect);
}

static u32 gluttonous_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, CLUBS, joker_event, joker_effect);
}

static u32 jolly_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 8;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 zany_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->THREE_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 12;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 mad_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->TWO_PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 10;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 crazy_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->STRAIGHT)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 12;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 droll_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->FLUSH)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 10;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 sly_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 50;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 wily_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->THREE_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 100;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 clever_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->TWO_PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 80;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 devious_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->STRAIGHT)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 100;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 crafty_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->FLUSH)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 80;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 half_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    int played_size = get_played_size();
    if (played_size <= 3)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 20;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 stencil_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &s_shared_joker_effect;

    List* jokers = get_jokers_list();

    // +1 xmult per empty joker slot...
    int num_jokers = list_get_len(jokers);

    (*joker_effect)->xmult = (MAX_JOKERS_HELD_SIZE)-num_jokers;

    // ...and also each stencil_joker adds +1 xmult
    ListItr itr = list_itr_create(jokers);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object->joker->id == STENCIL_JOKER_ID)
            (*joker_effect)->xmult++;
    }

    return JOKER_EFFECT_FLAG_XMULT;
}

#define MISPRINT_MAX_MULT 23
static u32 misprint_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &s_shared_joker_effect;

    (*joker_effect)->mult = rng_get_u32(RNG_SEQ_JOKER_MISPRINT) % (MISPRINT_MAX_MULT + 1);

    return JOKER_EFFECT_FLAG_MULT;
}

static u32 walkie_talkie_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->rank == TEN || scored_card->rank == FOUR)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 10;
        (*joker_effect)->mult = 4;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS | JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 fibonnaci_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    switch (scored_card->rank)
    {
        case ACE:
        case TWO:
        case THREE:
        case FIVE:
        case EIGHT:
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->mult = 8;
            effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
            break;
        default:
            break;
    }

    return effect_flags_ret;
}

static u32 banner_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_num_discards_remaining() > 0)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 30 * get_num_discards_remaining();
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 mystic_summit_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_num_discards_remaining() == 0)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 15;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 blackboard_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    bool all_cards_are_spades_or_clubs = true;
    CardObject** hand = get_hand_array();
    for (int i = 0; i < g_game_vars.hand_size; i++)
    {
        u8 suit = hand[i]->card->suit;
        if (suit == HEARTS || suit == DIAMONDS)
        {
            all_cards_are_spades_or_clubs = false;
            break;
        }
    }

    if (all_cards_are_spades_or_clubs)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 blue_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &s_shared_joker_effect;

    (*joker_effect)->chips = (get_deck_top() + 1) * 2;

    return JOKER_EFFECT_FLAG_CHIPS;
}

static u32 raised_fist_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    s32* p_lowest_value_index = &(joker->scoring_state);

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    switch (joker_event)
    {
        // Use this event to compute the index of the lowest value card only once.
        // Aces are always considered high value, even in an ace-low straight
        case JOKER_EVENT_ON_HAND_PLAYED:
            // index initialized at 0 but accessed only if
            // hand_size > 0 so we're never out of bounds
            *p_lowest_value_index = 0;
            u8 lowest_value = IMPOSSIBLY_HIGH_CARD_VALUE;
            CardObject** hand = get_hand_array();
            for (int i = 0; i < g_game_vars.hand_size; i++)
            {
                u8 value = card_get_value(hand[i]->card);
                if (lowest_value > value)
                {
                    *p_lowest_value_index = i;
                    lowest_value = value;
                }
            }
            break;

        case JOKER_EVENT_ON_CARD_HELD:
            if (get_scored_card_index() == *p_lowest_value_index)
            {
                *joker_effect = &s_shared_joker_effect;

                (*joker_effect)->mult = 2 * card_get_value(scored_card);
                effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 reserved_parking_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_ON_CARD_HELD, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_is_face(scored_card) && (rng_get_u32(RNG_SEQ_JOKER_RESERVED_PARKING) % 2 == 0))
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->money = 1;
        effect_flags_ret = JOKER_EFFECT_FLAG_MONEY;
    }

    return effect_flags_ret;
};

static u32 business_card_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_is_face(scored_card) && (rng_get_u32(RNG_SEQ_JOKER_BUSINESS_CARD) % 2 == 0))
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->money = 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_MONEY;
    }

    return effect_flags_ret;
}

static u32 scholar_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->rank == ACE)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 20;
        (*joker_effect)->mult = 4;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS | JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 scary_face_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_is_face(scored_card))
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 30;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 abstract_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &s_shared_joker_effect;

    // +1 xmult per occupied joker slot
    int num_jokers = list_get_len(get_jokers_list());

    (*joker_effect)->mult = num_jokers * 3;

    return JOKER_EFFECT_FLAG_MULT;
}

static u32 bull_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // The wiki says it does nothing if money is 0 or below
    // This allows us to avoid scoring negative Chips
    if (g_game_vars.money > 0)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = g_game_vars.money * 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 smiley_face_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_is_face(scored_card))
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 5;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 even_steven_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    switch (scored_card->rank)
    {
        case KING:
        case QUEEN:
        case JACK:
            break;
        default:
            if (card_get_value(scored_card) % 2 == 0)
            {
                *joker_effect = &s_shared_joker_effect;

                (*joker_effect)->mult = 4;
                effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
            }
            break;
    }

    return effect_flags_ret;
}

static u32 odd_todd_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_get_value(scored_card) % 2 == 1) // todo test ace
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = 31;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 acrobat_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // 0 remaining hands mean we're scoring the last hand
    if (get_num_hands_remaining() == 0)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 hanging_chad_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    s32* p_remaining_retriggers = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_remaining_retriggers = 2;
            break;

        // No need to check if this is the first card scored or not
        // p_remaining_retriggers will always reach 0 on the first card, then retrigger
        // will be false and scoring will go onto the next card
        case JOKER_EVENT_ON_CARD_SCORED_END:
            *joker_effect = &s_shared_joker_effect;

            (*joker_effect)->retrigger = (*p_remaining_retriggers > 0);
            if ((*joker_effect)->retrigger)
            {
                *p_remaining_retriggers -= 1;
                (*joker_effect)->message = "Again!";
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 the_duo_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->xmult = 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_trio_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->THREE_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_family_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->FOUR_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->xmult = 4;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_order_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->STRAIGHT)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_tribe_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_contained_hands()->FLUSH)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->xmult = 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 bootstraps_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // Same protection as the Bull Joker
    if (g_game_vars.money > 0)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = (g_game_vars.money / 5) * 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 shoot_the_moon_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_ON_CARD_HELD, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->rank == QUEEN)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = 13;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 photograph_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_first_face_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_first_face_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED:
            // has a face card been encountered already, and if not, is the current scoring card a
            // face card?
            if (*p_first_face_index == UNDEFINED && card_is_face(scored_card))
            {
                *p_first_face_index = get_scored_card_index();
            }
            // if we have a face card index saved, check against it and give mult accordingly
            // Doing this now will trigger the effect the first time we encounter the face card,
            // and we will catch potential retriggers
            if (*p_first_face_index == get_scored_card_index())
            {
                *joker_effect = &s_shared_joker_effect;

                (*joker_effect)->xmult = 2;
                effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
            }
            break;
        default:
            break;
    }

    return effect_flags_ret;
}

static u32 dusk_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            // start at -1 so that a first index of 0 can satisfy the retrigger condition below
            *p_last_retriggered_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            // Only retrigger current card if it's strictly after the last one we retriggered
            if (get_num_hands_remaining() == 0)
            {
                *joker_effect = &s_shared_joker_effect;

                (*joker_effect)->retrigger = (*p_last_retriggered_index < get_scored_card_index());
                if ((*joker_effect)->retrigger)
                {
                    *p_last_retriggered_index = get_scored_card_index();
                    (*joker_effect)->message = "Again!";
                    effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
                }
            }

            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 blueprint_brainstorm_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // No need for this kind of init since these Jokers
    // will have their data copied when needed
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED ||
        joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        return effect_flags_ret;
    }

    // find ourselves in the Jokers list
    List* jokers = get_jokers_list();
    ListItr itr = list_itr_create(jokers);
    JokerObject* copied_joker_object;
    while ((copied_joker_object = list_itr_next(&itr)))
    {
        if (copied_joker_object->joker == joker)
        {
            break;
        }
    }

    // This shouldn't happen since if we are a scoring Joker, we should always
    // be part of the Jokers list, but being extra careful doesn't cost much
    if (copied_joker_object == NULL)
    {
        return effect_flags_ret;
    }

    // find the copied Joker, may need to bounce around Blueprints and a Brainstorm
    // If we encounter NULL, we have a Blueprint at the end of the list that can't copy anything.
    // If we go through a Brainstorms twice, we will be in a loop and need to exit
    u8 brainstorm_counter = 0;
    do
    {
        switch (copied_joker_object->joker->id)
        {
            // get the next Joker for Blueprint
            case BLUEPRINT_JOKER_ID:
                copied_joker_object = list_itr_next(&itr);
                break;

            // Get the first (leftmost) Joker for Brainstorm
            case BRAINSTORM_JOKER_ID:
                brainstorm_counter++;
                itr = list_itr_create(jokers);
                copied_joker_object = list_itr_next(&itr);
                break;

            // We encountered a Joker that isn't a Copying Joker and copy it now
            // but how we copy it depends on this Joker's ID because they don't
            // all handle data the same way.
            default:
                u8 copied_joker_id = copied_joker_object->joker->id;
                const JokerInfo* copied_joker_info = get_joker_registry_entry(copied_joker_id);

                // Copy the persistent data
                joker->persistent_state = copied_joker_object->joker->persistent_state;

                // Then regardless of if we copied the data above, apply the
                // copied JokerEffect function to the local data.
                // Set copying flag and source pointer so the copied effect
                // knows it's being called by a Copying Joker (e.g. Wee Joker
                // reads the original's accumulated scoring_state).
                s_is_copying_joker = true;
                s_copied_joker_source = copied_joker_object->joker;
                effect_flags_ret =
                    copied_joker_info
                        ->joker_effect_func(joker, scored_card, joker_event, joker_effect);
                s_is_copying_joker = false;
                s_copied_joker_source = NULL;

                // make also sure we don't expire
                effect_flags_ret &= ~JOKER_EFFECT_FLAG_EXPIRE;

                // exit the loop
                copied_joker_object = NULL;

                break;
        }
    } while (copied_joker_object != NULL && brainstorm_counter < 2);

    return effect_flags_ret;
}

static u32 hack_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_last_retriggered_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            // Works the same way as Dusk, but check what rank the card is
            switch (scored_card->rank)
            {
                case TWO:
                case THREE:
                case FOUR:
                case FIVE:
                    *joker_effect = &s_shared_joker_effect;

                    (*joker_effect)->retrigger =
                        (*p_last_retriggered_index < get_scored_card_index());
                    if ((*joker_effect)->retrigger)
                    {
                        *p_last_retriggered_index = get_scored_card_index();
                        (*joker_effect)->message = "Again!";
                        effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
                    }
                    break;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 seltzer_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_idx = &(joker->scoring_state);
    s32* p_hands_left_until_exp = &(joker->persistent_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            *p_hands_left_until_exp = 10; // remaining retriggered hands
            break;

        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_last_retriggered_idx = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            // Works the same way as Dusk
            // No need to check for p_hands_left_until_exp because the Joker
            // will be destroyed the moment we hit 0
            *joker_effect = &s_shared_joker_effect;

            (*joker_effect)->retrigger = ((*p_last_retriggered_idx) < get_scored_card_index());
            if ((*joker_effect)->retrigger)
            {
                *p_last_retriggered_idx = get_scored_card_index();
                (*joker_effect)->message = "Again!";
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }
            break;

        case JOKER_EVENT_ON_HAND_SCORED_END:
            // Don't decrement countdown when being copied by Blueprint/Brainstorm
            if (!s_is_copying_joker)
            {
                *joker_effect = &s_shared_joker_effect;
                effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;

                (*p_hands_left_until_exp)--;
                if (*p_hands_left_until_exp > 0)
                {
                    // Need to do this for now because the message's memory can't really be allocated
                    // So we can't use snprintf to craft a message depending on the number of hands left
                    static const char* SELTZER_MESSAGES[] =
                        {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
                    (*joker_effect)->message = (char*)SELTZER_MESSAGES[(*p_hands_left_until_exp) - 1];
                }
                else
                {
                    (*joker_effect)->message = "Drank!";
                    (*joker_effect)->expire = true;
                    effect_flags_ret |= JOKER_EFFECT_FLAG_EXPIRE;
                }
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 sock_and_buskin_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_face_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_last_retriggered_face_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            *joker_effect = &s_shared_joker_effect;

            // Works the same way as Dusk, but for face cards
            (*joker_effect)->retrigger =
                ((*p_last_retriggered_face_index < get_scored_card_index()) &&
                 card_is_face(scored_card));
            if ((*joker_effect)->retrigger)
            {
                *p_last_retriggered_face_index = get_scored_card_index();
                (*joker_effect)->message = "Again!";
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

#pragma endregion

// ============================================================
// New Jokers (my_joker sprites)
// ============================================================

// --- Descriptions ---

static int wee_joker_desc(Joker* joker, Rect dest_rect)
{
    // Dynamic: shows current accumulated chips
    static const char desc_format[] =
        TTE_BLACK_TAG "Each scored " TTE_RED_TAG "2" TTE_BLACK_TAG
        " gives " TTE_RED_TAG "+8 " TTE_BLACK_TAG "Chips "
        "(Now " TTE_RED_TAG "+%ld " TTE_BLACK_TAG "Chips)";
    char desc[256];
    snprintf(desc, sizeof(desc), desc_format, (long)joker->scoring_state);
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int riff_raff_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "When blind starts, create " TTE_RED_TAG "2 " TTE_BLACK_TAG
        "random " TTE_RED_TAG "Common " TTE_BLACK_TAG
        "Jokers (Must have room)";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int baron_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each " TTE_RED_TAG "King " TTE_BLACK_TAG "held in hand "
        "gives " TTE_RED_TAG "X1.5 " TTE_BLACK_TAG "Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int mime_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Retrigger all " TTE_RED_TAG "cards held in hand";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int egg_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Gains " TTE_RED_TAG "$3 " TTE_BLACK_TAG "of sell value "
        "each round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int smeared_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "Clubs " TTE_BLACK_TAG "and " TTE_RED_TAG "Spades "
        TTE_BLACK_TAG "are the same suit, "
        TTE_RED_TAG "Diamonds " TTE_BLACK_TAG "and " TTE_RED_TAG "Hearts "
        TTE_BLACK_TAG "are the same suit";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

// --- Effects ---
// Chips accumulate on the joker itself (stored in scoring_state).
// Starts at +0 chips, each scored 2 adds +8.
// Accumulated chips are added to base chips at ON_HAND_SCORED_END,
// so they participate in the final Chips × Mult calculation.
// Brainstorm/Blueprint only copy the current accumulated value.
static u32 wee_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    switch (joker_event)
    {
        case JOKER_EVENT_ON_CARD_SCORED:
            if (scored_card != NULL && scored_card->rank == TWO)
            {
                if (s_is_copying_joker)
                {
                    // Copy mode: read original's accumulated value (no accumulation)
                    *joker_effect = &s_shared_joker_effect;
                    (*joker_effect)->chips = s_copied_joker_source->scoring_state;
                    return JOKER_EFFECT_FLAG_CHIPS;
                }
                else
                {
                    // Normal mode: accumulate +8 chips and show upgrade animation
                    joker->scoring_state += 8;
                    *joker_effect = &s_shared_joker_effect;
                    (*joker_effect)->message = "Upgrade!";
                    return JOKER_EFFECT_FLAG_MESSAGE;
                }
            }
            break;

        case JOKER_EVENT_ON_HAND_SCORED_END:
            // Add accumulated chips to base chips so they participate
            // in the final Chips × Mult calculation
            if (joker->scoring_state > 0)
            {
                *joker_effect = &s_shared_joker_effect;
                (*joker_effect)->chips = joker->scoring_state;
                return JOKER_EFFECT_FLAG_CHIPS;
            }
            break;

        default:
            break;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Riff-Raff: When blind starts (cards dealt), create 2 random common/uncommon jokers
// --- Riff-Raff serialized spawn chain ----------------------------------------
// ON_BLIND_SELECTED dispatches left-to-right; each Riff-Raff instance (the
// real one and any Blueprint/Brainstorm copies) locks its spawn count at
// DISPATCH time (free slots = MAX - list length at that exact moment, i.e.
// before any deferred spawn happens) and queues itself. The queue is then
// processed serially, one spawn per RIFF_RAFF_SPAWN_DELAY: each queued
// instance shows its "+N Jokers" animation, waits a beat, then actually
// spawns its locked count. Because the count is locked at dispatch:
//  - a Riff-Raff LEFT of a Dagger sees pre-sacrifice slots (locked count),
//    and the slot the Dagger frees goes to whoever dispatches AFTER it
//    (e.g. a Brainstorm copy) - matching original Balatro's left-to-right
//    resolution;
//  - a Riff-Raff RIGHT of a Dagger sees the list minus the (about to be
//    expired) victim, so it gets to use the freed slot.
// The chain stops silently when no requests were queued (no room at dispatch).
#define RIFF_RAFF_SPAWN_DELAY FRAMES(60) // ~1s pause after each trigger anim

static JokerObject* s_riff_raff_queue[MAX_JOKERS_HELD_SIZE];
static int s_riff_raff_queue_anim[MAX_JOKERS_HELD_SIZE];
static int s_riff_raff_queue_count = 0;
static int s_riff_raff_active = -1;
static int s_riff_raff_anim_count = 0;
static u32 s_riff_raff_spawn_at = 0;

// Called every frame from game.c's jokers_update_loop(). Advances the spawn
// chain: activate next queued instance -> show animation -> wait -> spawn.
void riff_raff_process_pending(void)
{
    if (s_riff_raff_queue_count == 0)
        return;

    // Validate the active request's source object is still owned (it may
    // have been sacrificed by a Dagger meanwhile).
    if (s_riff_raff_active >= 0)
    {
        bool still_owned = false;
        ListItr itr = list_itr_create(get_jokers_list());
        JokerObject* cur;
        while ((cur = list_itr_next(&itr)))
        {
            if (cur == s_riff_raff_queue[s_riff_raff_active])
            {
                still_owned = true;
                break;
            }
        }
        if (!still_owned)
        {
            // Source gone - skip this request entirely. Do NOT increment
            // s_riff_raff_active here (the activation branch below owns all
            // advancing); just clear the timer so we come back and advance.
            s_riff_raff_spawn_at = 0;
            return;
        }
    }

    // Activate the next queued instance: use the spawn count that was locked
    // at dispatch time, show its trigger animation, then schedule the spawn.
    // Active index advances ONLY here (first frame starts at 0; afterwards
    // each spawn completion sets spawn_at=0 which brings us back here to
    // advance) - the spawn branch below must NOT increment it, or every
    // queued request after the first would be skipped.
    if (s_riff_raff_spawn_at == 0)
    {
        if (s_riff_raff_active < 0)
            s_riff_raff_active = 0;
        else
            s_riff_raff_active++;
        if (s_riff_raff_active >= s_riff_raff_queue_count)
        {
            s_riff_raff_queue_count = 0;
            s_riff_raff_active = -1;
            return;
        }

        JokerObject* source = s_riff_raff_queue[s_riff_raff_active];
        if (source == NULL || source->joker == NULL)
        {
            s_riff_raff_queue_count = 0;
            s_riff_raff_active = -1;
            return;
        }

        s_riff_raff_anim_count = s_riff_raff_queue_anim[s_riff_raff_active];

        // Show the trigger animation "+N Jokers" over this instance
        char anim_buffer[16];
        snprintf(
            anim_buffer,
            sizeof(anim_buffer),
            "+%d Jokers",
            s_riff_raff_anim_count
        );
        tte_set_pos(fx2int(source->x) + TILE_SIZE, JOKER_SCORE_TEXT_Y);
        tte_set_special(TTE_WHITE_PB * TTE_SPECIAL_PB_MULT_OFFSET);
        tte_write(anim_buffer);
        joker_object_shake(source, UNDEFINED);
        // Keep the event message auto-clear timer from wiping this text early
        schedule_joker_event_text_clear();

        s_riff_raff_spawn_at = g_game_vars.timer + RIFF_RAFF_SPAWN_DELAY;
        return;
    }

    // Time to spawn the queued instance's jokers
    if (g_game_vars.timer >= s_riff_raff_spawn_at)
    {
        int to_spawn = s_riff_raff_anim_count;
        // Same effective-occupancy rule as the activation check above:
        // expired jokers are about to be removed, their slots are usable.
        int current_count = list_get_len(get_jokers_list()) -
                            list_get_len(get_expired_jokers_list());
        if (current_count + to_spawn > MAX_JOKERS_HELD_SIZE)
            to_spawn = MAX_JOKERS_HELD_SIZE - current_count;

        for (int i = 0; i < to_spawn; i++)
        {
            // Riff-Raff only spawns Common Jokers
            u8 rarity = COMMON_JOKER;
            u8 joker_id = 0;
            bool found = false;
            for (int attempt = 0; attempt < 50; attempt++)
            {
                u8 candidate = rng_get_u32() % get_joker_registry_size();
                const JokerInfo* info = get_joker_registry_entry(candidate);
                if (info && info->rarity == rarity && !is_joker_owned(candidate))
                {
                    if (candidate == GROS_MICHEL_ID && is_gros_michel_destroyed())
                        continue;
                    if (candidate == CAVENDISH_ID && !is_gros_michel_destroyed())
                        continue;
                    joker_id = candidate;
                    found = true;
                    break;
                }
            }

            if (!found)
                continue;

            Joker* new_joker = joker_new(joker_id);
            if (new_joker == NULL)
                continue;

            JokerObject* new_joker_object = joker_object_new(new_joker);
            if (new_joker_object == NULL)
            {
                joker_destroy(&new_joker);
                continue;
            }

            // Set Y position to match other held jokers
            new_joker_object->ty = int2fx(HELD_JOKERS_POS.y);
            add_joker(new_joker_object);
            // Mark as non-rollable so shop won't generate the same joker
            joker_set_rollable(joker_id, false);
            current_count++;
        }

        // Advance to the next queued instance: just clear the timer so the
        // activation branch above advances s_riff_raff_active next frame
        // (never increment here - see note at the activation branch).
        s_riff_raff_spawn_at = 0;
        if (s_riff_raff_active + 1 >= s_riff_raff_queue_count)
        {
            s_riff_raff_queue_count = 0;
            s_riff_raff_active = -1;
        }
    }
}

static u32 riff_raff_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_ON_BLIND_SELECTED)
    {
        // Copies (Blueprint/Brainstorm) always trigger: the blueprint copy
        // mechanism syncs persistent_state from the source (already 1 by the
        // time the copy runs), so the flag check must be skipped for copies.
        if (s_is_copying_joker || joker->persistent_state == 0)
        {
            if (!s_is_copying_joker)
            {
                // Only trigger once per round (use persistent_state as flag)
                joker->persistent_state = 1;
            }

            // Queue this instance for the serialized spawn chain (processed
            // left-to-right by riff_raff_process_pending()). The spawn count
            // is LOCKED here at dispatch time: free slots are computed from
            // the list length at this exact moment (before any deferred spawn
            // happens). A Riff-Raff left of a Dagger therefore locks the
            // pre-sacrifice count, while one right of a Dagger sees the list
            // without the sacrificed victim - matching the left-to-right
            // resolution order of original Balatro.
            ListItr itr = list_itr_create(get_jokers_list());
            JokerObject* self_object = NULL;
            JokerObject* cur;
            while ((cur = list_itr_next(&itr)))
            {
                if (cur->joker == joker)
                {
                    self_object = cur;
                    break;
                }
            }

            if (self_object != NULL &&
                s_riff_raff_queue_count < MAX_JOKERS_HELD_SIZE)
            {
                int free_slots =
                    MAX_JOKERS_HELD_SIZE - list_get_len(get_jokers_list());
                if (free_slots > 0)
                {
                    s_riff_raff_queue[s_riff_raff_queue_count] = self_object;
                    s_riff_raff_queue_anim[s_riff_raff_queue_count] =
                        free_slots < 2 ? free_slots : 2;
                    s_riff_raff_queue_count++;
                }
            }
        }
    }
    else if (joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        // Reset flag for next round
        joker->persistent_state = 0;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Baron: Each King held in hand gives x1.5 mult
static u32 baron_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_ON_CARD_HELD, joker_event)

    if (scored_card->rank == KING)
    {
        // x1.5: mult = mult * 3 / 2 (applied directly since xmult only supports integers)
        g_game_vars.mult = u32_protected_mult(g_game_vars.mult, 3) / 2;

        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->message = "X1.5";
        return JOKER_EFFECT_FLAG_MESSAGE;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Mime: Retrigger all cards held in hand
static u32 mime_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_ON_HAND_SCORED_END)
    {
        // Signal that held cards should be retriggered
        // This would need integration with the scoring system
        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->message = "Retrigger!";
        return JOKER_EFFECT_FLAG_MESSAGE;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Egg: Gains $3 of sell value each round
static u32 egg_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        // sell_value = value / 2, so +6 to value = +$3 sell value
        joker->value += 6;

        // Show "+$3" animation with shake
        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->message = "+$3";
        return JOKER_EFFECT_FLAG_MESSAGE;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Smear Joker: Clubs and Spades count as the same suit; Diamonds and Hearts count as the same suit
static u32 smeared_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    // Passive effect - suit merging is handled in hand.c via is_smeared_joker_active()
    return JOKER_EFFECT_FLAG_NONE;
}

// --- Descriptions for new jokers ---

static int faceless_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "If discarded hand contains "
        TTE_RED_TAG "3 or more Face Cards"
        TTE_BLACK_TAG ", earn " TTE_RED_TAG "$5";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int gros_michel_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Provides " TTE_RED_TAG "+15 Mult"
        TTE_BLACK_TAG ". " TTE_RED_TAG "1 in 6"
        TTE_BLACK_TAG " chance this card is destroyed at end of hand";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int cavendish_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Provides " TTE_RED_TAG "X3 Mult"
        TTE_BLACK_TAG ". " TTE_RED_TAG "1 in 1000"
        TTE_BLACK_TAG " chance this card is destroyed at end of hand";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

// --- Effects for new jokers ---

// Faceless Joker: When 3+ face cards are discarded, give $5
static u32 faceless_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_ON_HAND_DISCARDED)
    {
        if (get_discarded_face_card_count() >= 3)
        {
            g_game_vars.money += 5;
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->message = "$5";
            return JOKER_EFFECT_FLAG_MESSAGE;
        }
    }
    return JOKER_EFFECT_FLAG_NONE;
}

// Gros Michel: +15 mult, 1/6 chance to self-destruct after each hand
static u32 gros_michel_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_INDEPENDENT)
    {
        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->mult = 15;
        return JOKER_EFFECT_FLAG_MULT;
    }

    if (joker_event == JOKER_EVENT_ON_HAND_SCORED_END)
    {
        // Don't self-destruct when being copied by Blueprint/Brainstorm
        if (!s_is_copying_joker && (rng_get_u32() % 6) == 0)
        {
            // Self-destruct!
            set_gros_michel_destroyed();
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->message = "EXTINCT!";
            (*joker_effect)->expire = true;
            return JOKER_EFFECT_FLAG_MESSAGE | JOKER_EFFECT_FLAG_EXPIRE;
        }
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Cavendish: x3 mult, 1/1000 chance to self-destruct after each hand
static u32 cavendish_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_INDEPENDENT)
    {
        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->xmult = 3;
        return JOKER_EFFECT_FLAG_XMULT;
    }

    if (joker_event == JOKER_EVENT_ON_HAND_SCORED_END)
    {
        // Don't self-destruct when being copied by Blueprint/Brainstorm
        if (!s_is_copying_joker && (rng_get_u32() % 1000) == 0)
        {
            // Self-destruct!
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->message = "EXTINCT!";
            (*joker_effect)->expire = true;
            return JOKER_EFFECT_FLAG_MESSAGE | JOKER_EFFECT_FLAG_EXPIRE;
        }
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --- Flower Pot (ID 62) ---

// Description: If played hand contains all 4 suits, give x3 mult.
// With Smeared Joker: only need 1 red + 1 black card.
static int flower_pot_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "If played hand has "
        TTE_RED_TAG "all 4 suits"
        TTE_BLACK_TAG ", give "
        TTE_RED_TAG "X3 Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

// Effect: x3 mult if played hand contains all 4 suits.
// Uses suit_counts from hand.c which already applies card_effective_suit_mask().
static u32 flower_pot_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    // Get suit distribution of played cards (already smeared-joker aware)
    extern CardObject** get_played_hand(void);
    extern int get_played_top(void);
    CardObject** played = get_played_hand();
    int top = get_played_top();
    int suit_counts[NUM_SUITS] = {0};
    get_played_suit_counts(played, top, suit_counts);

    // Check if all 4 suits are present
    if (suit_counts[0] > 0 && suit_counts[1] > 0 &&
        suit_counts[2] > 0 && suit_counts[3] > 0)
    {
        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->xmult = 3;
        return JOKER_EFFECT_FLAG_XMULT;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --- Loyalty Card (ID 63) ---

#define LOYALTY_CARD_HANDS_REQUIRED 6

// Description: Every 6 hands played, next hand gets X4 Mult.
// Shows remaining hands until the next X4 (dynamic, updates with the counter).
static int loyalty_card_joker_desc(Joker* joker, Rect dest_rect)
{
    char desc[160];
    int remaining = joker->persistent_state;
    if (remaining < 0)
        remaining = 0;
    if (remaining > LOYALTY_CARD_HANDS_REQUIRED - 1)
        remaining = LOYALTY_CARD_HANDS_REQUIRED - 1;

    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "Every " TTE_RED_TAG "6 hands" TTE_BLACK_TAG " played, "
        TTE_RED_TAG "X4 Mult" TTE_BLACK_TAG ", " TTE_RED_TAG "%d" TTE_BLACK_TAG " remaining",
        remaining
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

// Effect: remaining hands countdown 5 -> 0; the hand that plays at 0 gets X4 Mult,
// then the cycle resets to 5.
// persistent_state holds remaining hands until the next X4 (5-0).
// Each instance counts independently (Blueprint/Brainstorm copies have their own counter).
static u32 loyalty_card_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    s32* p_remaining = &(joker->persistent_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            *p_remaining = LOYALTY_CARD_HANDS_REQUIRED - 1; // 5 hands remaining
            break;

        case JOKER_EVENT_INDEPENDENT:
            // Remaining == 0 means this hand gets the X4, then the cycle resets.
            if (*p_remaining == 0)
            {
                *p_remaining = LOYALTY_CARD_HANDS_REQUIRED - 1;
                *joker_effect = &s_shared_joker_effect;
                (*joker_effect)->xmult = 4;
                effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
            }
            else
            {
                (*p_remaining)--;
            }
            break;

        case JOKER_EVENT_ON_HAND_SCORED_END:
            // Show remaining hands until next X4 (real joker only, not copies)
            if (!s_is_copying_joker)
            {
                *joker_effect = &s_shared_joker_effect;
                effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;

                // Message memory can't be allocated, so use a static string table
                static const char* LOYALTY_MESSAGES[] =
                    {"0", "1", "2", "3", "4", "5"};
                (*joker_effect)->message = (char*)LOYALTY_MESSAGES[*p_remaining];
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

// --- Riding the Bus (ID 64) ---

// Description: gains +1 mult per consecutive hand played without a scoring
// face card. Dynamic: shows current accumulated mult.
static int riding_the_bus_joker_desc(Joker* joker, Rect dest_rect)
{
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "Gains " TTE_RED_TAG "+1 Mult"
        TTE_BLACK_TAG " per consecutive hand played without a scoring "
        TTE_RED_TAG "Face Card" TTE_BLACK_TAG " (currently " TTE_RED_TAG "+%ld "
        TTE_BLACK_TAG "Mult)",
        (long)joker->scoring_state
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

// Effect: a scoring face card resets the accumulated mult *at the moment it
// scores* (ON_CARD_SCORED only fires for cards actually participating in
// scoring, so unselected played face cards don't break the streak, and future
// Splash support is automatic). A hand with no scoring face card extends the
// streak by +1 at INDEPENDENT timing (before scoring completes) so the new
// mult applies to the current hand - same as Wee Joker's immediate chips.
// persistent_state is a per-hand flag marking whether a face card scored.
// Blueprint/Brainstorm copies mirror the original's accumulated mult.
static u32 riding_the_bus_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    s32* p_accumulated_mult = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            *p_accumulated_mult = 0;
            joker->persistent_state = 0;
            break;

        case JOKER_EVENT_INDEPENDENT:
        {
            bool is_copy = s_is_copying_joker;
            s32 mult_to_apply = *p_accumulated_mult;

            if (!is_copy)
            {
                // No scoring face card this hand -> extend the streak NOW so
                // the current hand already benefits from the new mult
                if (joker->persistent_state == 0)
                    (*p_accumulated_mult)++;
                mult_to_apply = *p_accumulated_mult;
            }
            else if (s_copied_joker_source != NULL)
            {
                // Copy mode: mirror the original's accumulated value
                mult_to_apply = s_copied_joker_source->scoring_state;
            }

            if (mult_to_apply > 0)
            {
                *joker_effect = &s_shared_joker_effect;
                (*joker_effect)->mult = mult_to_apply;
                effect_flags_ret = JOKER_EFFECT_FLAG_MULT;

                // Only the real joker pops the message (copies stay silent)
                if (!is_copy)
                {
                    (*joker_effect)->message = "Mult!";
                    effect_flags_ret |= JOKER_EFFECT_FLAG_MESSAGE;
                }
            }
            break;
        }

        case JOKER_EVENT_ON_CARD_SCORED:
            // A face card scoring breaks the streak instantly
            // (card_is_face respects Pareidolia; copies stay silent)
            if (scored_card != NULL && !s_is_copying_joker &&
                card_is_face(scored_card))
            {
                // Always mark the hand (blocks the +1 at INDEPENDENT)
                joker->persistent_state = 1;

                // Only animate/reset if there is a streak to break
                if (*p_accumulated_mult > 0)
                {
                    *p_accumulated_mult = 0;
                    *joker_effect = &s_shared_joker_effect;
                    (*joker_effect)->message = "Reset!";
                    effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;
                }
            }
            break;

        case JOKER_EVENT_ON_HAND_SCORED_END:
            // Copies don't touch the flag (they mirror the original)
            if (s_is_copying_joker)
                break;
            // Clear the per-hand face-card flag for the next hand
            joker->persistent_state = 0;
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

// --- Ceremonial Dagger (ID 65) ---

// Description: when blind is selected, destroy the Joker to the right and add
// double its sell value to this Joker's Mult. Dynamic: shows current mult.
static int ceremonial_dagger_joker_desc(Joker* joker, Rect dest_rect)
{
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "On blind select, destroy " TTE_RED_TAG "Joker to the right"
        TTE_BLACK_TAG " and add double its sell value to " TTE_RED_TAG "Mult"
        TTE_BLACK_TAG " (now " TTE_RED_TAG "+%ld " TTE_BLACK_TAG "Mult)",
        (long)joker->scoring_state
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

// Effect: at ON_BLIND_SELECTED, destroys the Joker immediately to the right
// (only the adjacent one; ones further right are safe; if this Joker is
// rightmost, nothing happens). Adds double the victim's *sell value* to the
// accumulated mult - joker_get_sell_value() reads the live value/2, so it
// correctly absorbs an Egg that has been growing its sell value. The victim
// is shaken and pushed to the expired list (rotating/shrinking animation,
// then auto-removal - safe during list iteration since the owned list isn't
// touched immediately). The mult applies at INDEPENDENT; copies mirror it.
//
// If the right neighbor is still playing its entry animation (e.g. a Joker
// just spawned by Riff-Raff hasn't reached its slot yet), the sacrifice is
// deferred: the victim is stored and consumed by
// ceremonial_dagger_process_pending() once it has arrived at its slot, so
// the entry animation always plays out first.
// If there was no right neighbor at dispatch (e.g. a Riff-Raff to the left
// will spawn new jokers this blind), the dagger waits for one to appear and
// sacrifices it once it settles (s_dagger_waiting_new_victim).
static JokerObject* s_dagger_pending_victim = NULL;
static bool s_dagger_waiting_new_victim = false;

// Sacrifice a victim: add double its sell value to the dagger's mult, shake
// the dagger and the victim, show "+N Mult" over the dagger, and push the
// victim to the expired list for the shrink-and-remove animation.
static void dagger_sacrifice(JokerObject* dagger_object, JokerObject* victim)
{
    if (dagger_object == NULL || dagger_object->joker == NULL ||
        victim == NULL || victim->joker == NULL)
    {
        return;
    }

    s32 gain = 2 * joker_get_sell_value(victim->joker);
    dagger_object->joker->scoring_state += gain;

    // Show "+N Mult" over the dagger (immediate and deferred sacrifices)
    char gain_buffer[24];
    snprintf(gain_buffer, sizeof(gain_buffer), "+%ld Mult", (long)gain);
    tte_set_pos(fx2int(dagger_object->x) + TILE_SIZE, JOKER_SCORE_TEXT_Y);
    tte_set_special(TTE_RED_PB * TTE_SPECIAL_PB_MULT_OFFSET);
    tte_write(gain_buffer);

    joker_object_shake(dagger_object, SFX_MULT);
    joker_object_shake(victim, UNDEFINED);
    list_push_back(get_expired_jokers_list(), victim);
}

// Per-frame check for a deferred dagger sacrifice:
//  - fixed victim: sacrifice it once it has arrived at its slot
//  - waiting-new-victim: a right neighbor will be spawned (Riff-Raff chain);
//    sacrifice it once it appears and settles. Gives up when the Riff-Raff
//    chain is done and still nothing showed up to the right.
// Called every frame from game.c's jokers_update_loop().
void ceremonial_dagger_process_pending(void)
{
    JokerObject* victim = s_dagger_pending_victim;
    if (victim == NULL && !s_dagger_waiting_new_victim)
        return;

    // The victim may have been removed meanwhile (sold, expired...): verify
    // it is still in the owned list and that a real dagger still exists.
    bool victim_found = false;
    JokerObject* dagger_object = NULL;
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (victim != NULL && cur == victim)
            victim_found = true;
        if (cur->joker != NULL && cur->joker->id == CEREMONIAL_DAGGER_ID)
            dagger_object = cur;
    }

    if (dagger_object == NULL)
    {
        // Dagger is gone (sold/expired): nothing to do anymore
        s_dagger_pending_victim = NULL;
        s_dagger_waiting_new_victim = false;
        return;
    }

    if (victim != NULL)
    {
        if (!victim_found)
        {
            s_dagger_pending_victim = NULL;
            return;
        }

        // Wait until the victim has arrived at its slot
        if (victim->x != victim->tx || victim->y != victim->ty)
            return;

        dagger_sacrifice(dagger_object, victim);
        s_dagger_pending_victim = NULL;
        return;
    }

    if (s_dagger_waiting_new_victim)
    {
        // Find the dagger's current right neighbor
        ListItr itr2 = list_itr_create(get_jokers_list());
        JokerObject* cur2;
        JokerObject* neighbor = NULL;
        while ((cur2 = list_itr_next(&itr2)))
        {
            if (cur2 == dagger_object)
            {
                neighbor = list_itr_next(&itr2);
                break;
            }
        }

        if (neighbor != NULL && neighbor->joker != NULL)
        {
            // Wait for it to settle, then sacrifice it
            if (neighbor->x == neighbor->tx && neighbor->y == neighbor->ty)
            {
                dagger_sacrifice(dagger_object, neighbor);
                s_dagger_waiting_new_victim = false;
            }
        }
        else
        {
            // No neighbor yet. If the Riff-Raff spawn chain is done, nothing
            // will ever appear to the right - give up quietly.
            if (s_riff_raff_queue_count == 0)
            {
                s_dagger_waiting_new_victim = false;
            }
        }
    }
}
static u32 ceremonial_dagger_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    s32* p_accumulated_mult = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            *p_accumulated_mult = 0;
            break;

        case JOKER_EVENT_ON_BLIND_SELECTED:
            // Copies don't sacrifice (they mirror the original's mult)
            if (s_is_copying_joker)
                break;

            // Find this Joker in the list; the next entry is the right neighbor
            ListItr itr = list_itr_create(get_jokers_list());
            JokerObject* cur;
            while ((cur = list_itr_next(&itr)))
            {
                if (cur->joker == joker)
                {
                    JokerObject* victim = list_itr_next(&itr);
                    if (victim != NULL && victim->joker != NULL)
                    {
                        // If the victim is still playing its entry animation
                        // (e.g. Riff-Raff just spawned it), defer the
                        // sacrifice until it has reached its slot so its
                        // animation plays out first.
                        if (victim->x != victim->tx || victim->y != victim->ty)
                        {
                            s_dagger_pending_victim = victim;
                        }
                        else
                        {
                            dagger_sacrifice(cur, victim);
                        }
                    }
                    else
                    {
                        // No right neighbor at dispatch: a Riff-Raff to the
                        // left may spawn new jokers this blind - wait for one
                        // to appear to the right and sacrifice it once it
                        // settles (gives up when the spawn chain is done).
                        s_dagger_waiting_new_victim = true;
                    }
                    break;
                }
            }
            break;

        case JOKER_EVENT_INDEPENDENT:
        {
            s32 mult_to_apply;
            if (s_is_copying_joker && s_copied_joker_source != NULL)
            {
                // Copy mode: mirror the original's accumulated mult
                mult_to_apply = s_copied_joker_source->scoring_state;
            }
            else
            {
                mult_to_apply = *p_accumulated_mult;
            }

            if (mult_to_apply > 0)
            {
                *joker_effect = &s_shared_joker_effect;
                (*joker_effect)->mult = mult_to_apply;
                effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
            }
            break;
        }

        default:
            break;
    }

    return effect_flags_ret;
}

// Returns true while deferred blind-selected joker effects are still
// running (Riff-Raff spawn chain / dagger waiting for a victim). The round
// waits for these before dealing the hand, so all joker effects play out
// first and cards are dealt after.
bool joker_effects_busy(void)
{
    return s_riff_raff_queue_count > 0 || s_dagger_pending_victim != NULL ||
           s_dagger_waiting_new_victim;
}
