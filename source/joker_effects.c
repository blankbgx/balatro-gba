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
static bool s_is_copying_joker = false;

// Pointer to the original joker being copied by Blueprint/Brainstorm.
// Used by Wee Joker to read the original's accumulated scoring_state.
static Joker* s_copied_joker_source = NULL;

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
REGISTER_JOKER_DESC_FUNC(credit_card_joker_desc)
REGISTER_JOKER_DESC_FUNC(burglar_joker_desc)
REGISTER_JOKER_DESC_FUNC(flash_card_joker_desc)
REGISTER_JOKER_DESC_FUNC(showman_joker_desc)
REGISTER_JOKER_DESC_FUNC(card_sharp_joker_desc)
REGISTER_JOKER_DESC_FUNC(to_the_moon_joker_desc)
REGISTER_JOKER_DESC_FUNC(splash_joker_desc)
REGISTER_JOKER_DESC_FUNC(supernova_joker_desc)
REGISTER_JOKER_DESC_FUNC(green_joker_desc)
REGISTER_JOKER_DESC_FUNC(square_joker_desc)
REGISTER_JOKER_DESC_FUNC(baseball_card_desc)
REGISTER_JOKER_DESC_FUNC(stuntman_joker_desc)
REGISTER_JOKER_DESC_FUNC(ancient_joker_desc)
REGISTER_JOKER_DESC_FUNC(swashbuckler_joker_desc)
REGISTER_JOKER_DESC_FUNC(gift_card_joker_desc)
REGISTER_JOKER_EFFECT_FUNC(credit_card_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(burglar_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(flash_card_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(showman_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(card_sharp_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(to_the_moon_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(splash_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(supernova_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(green_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(square_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(baseball_card_effect)
REGISTER_JOKER_EFFECT_FUNC(stuntman_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(ancient_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(swashbuckler_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(gift_card_joker_effect)

// Joker Effect functions

// Plain suit names for the Ancient Joker (78) round-start MESSAGE
// (user 2026-08-23: 特殊颜色看不清——花色播放统一白色). The desc uses
// pre-built static strings with embedded colored suit tags
// (s_ancient_descs), so this array carries only the plain names.
static const char* const s_ancient_suit_names[NUM_SUITS] =
{
    "Diamond ",
    "Club ",
    "Hearts ",
    "Spade ",
};

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
    { "Joker",            COMMON_JOKER,    2, false, default_joker_desc,          default_joker_effect              }, // DEFAULT_JOKER_ID = 0 (orig #1)
    { "Abstract Joker",   COMMON_JOKER,    4, false, abstract_joker_desc,         abstract_joker_effect             }, // 1 (orig #34)
    { "Half Joker",       COMMON_JOKER,    5, false, half_joker_desc,             half_joker_effect                 }, // 2 (orig #16)
    { "Misprint",         COMMON_JOKER,    4, true,  misprint_joker_desc,         misprint_joker_effect             }, // 3 (orig #27)
    { "Scary Face",       COMMON_JOKER,    4, false, scary_face_joker_desc,       scary_face_joker_effect           }, // 4 (orig #33)
    { "Sock and Buskin",  UNCOMMON_JOKER,  6, false, sock_and_buskin_joker_desc,  sock_and_buskin_joker_effect      }, // 5 (orig #109)
    { "Acrobat",          UNCOMMON_JOKER,  6, false, acrobat_joker_desc,          acrobat_joker_effect              }, // 6 (orig #108)
    { "Fibonacci",        UNCOMMON_JOKER,  8, false, fibonnaci_joker_desc,        fibonnaci_joker_effect            }, // 7 (orig #31)
    { "Scholar",          COMMON_JOKER,    4, false, scholar_joker_desc,          scholar_joker_effect              }, // 8 (orig #41)
    { "Crafty Joker",     COMMON_JOKER,    4, false, crafty_joker_desc,           crafty_joker_effect               }, // 9 (orig #15)
    { "Droll Joker",      COMMON_JOKER,    4, false, droll_joker_desc,            droll_joker_effect                }, // 10 (orig #10)
    { "Raised Fist",      COMMON_JOKER,    5, false, raised_fist_joker_desc,      raised_fist_joker_effect          }, // 11 (orig #29)
    { "Reserved Parking", COMMON_JOKER,    6, false, reserved_parking_joker_desc, reserved_parking_joker_effect     }, // 12 (orig #82)
    { "Business Card",    COMMON_JOKER,    4, false, business_card_joker_desc,    business_card_joker_effect        }, // 13 (orig #42)
    { "Hanging Chad",     COMMON_JOKER,    4, false, hanging_chad_joker_desc,     hanging_chad_joker_effect         }, // 14 (orig #115)
    { "Joker Stencil",    UNCOMMON_JOKER,  8, false, stencil_joker_desc,          stencil_joker_effect              }, // 15 (orig #17)
    { "Banner",           COMMON_JOKER,    5, false, banner_joker_desc,           banner_joker_effect               }, // 16 (orig #22)
    { "Shoot the Moon",   COMMON_JOKER,    5, false, shoot_the_moon_joker_desc,   shoot_the_moon_joker_effect,      }, // 17 (orig #140)
    // Spritesheet 1 
    { "Greedy Joker",     COMMON_JOKER,    5, false, greedy_joker_desc,           greedy_joker_effect               }, // 18 (orig #2)
    { "Lusty Joker",      COMMON_JOKER,    5, false, lusty_joker_desc,            lusty_joker_effect                }, // 19 (orig #3)
    // Spritesheet 2
    { "Wrathful Joker",   COMMON_JOKER,    5, false, wrathful_joker_desc,         wrathful_joker_effect             }, // 20 (orig #4)
    { "Gluttonous Joker", COMMON_JOKER,    5, false, gluttonous_joker_desc,       gluttonous_joker_effect           }, // 21 (orig #5)
    // Spritesheet 3
    { "Crazy Joker",      COMMON_JOKER,    4, false, crazy_joker_desc,            crazy_joker_effect                }, // 22 (orig #9)
    { "Mad Joker",        COMMON_JOKER,    4, false, mad_joker_desc,              mad_joker_effect                  }, // 23 (orig #8)
    { "Clever Joker",     COMMON_JOKER,    4, false, clever_joker_desc,           clever_joker_effect               }, // 24 (orig #13)
    { "Devious Joker",    COMMON_JOKER,    4, false, devious_joker_desc,          devious_joker_effect              }, // 25 (orig #14)
    { "Even Steven",      COMMON_JOKER,    4, false, even_steven_joker_desc,      even_steven_joker_effect          }, // 26 (orig #39)
    // Spritesheet 4
    { "Blackboard",       UNCOMMON_JOKER,  6, false, blackboard_joker_desc,       blackboard_joker_effect           }, // 27 (orig #48)
    { "Mystic Summit",    COMMON_JOKER,    5, false, mystic_summit_joker_desc,    mystic_summit_joker_effect        }, // 28 (orig #23)
    { "Walkie Talkie",    COMMON_JOKER,    4, false, walkie_talkie_joker_desc,    walkie_talkie_joker_effect        }, // 29 (orig #101)
    { "Zany Joker",       COMMON_JOKER,    4, false, zany_joker_desc,             zany_joker_effect                 }, // 30 (orig #7)
    { "Wily Joker",       COMMON_JOKER,    4, false, wily_joker_desc,             wily_joker_effect                 }, // 31 (orig #12)
    // Spritesheet 5
    { "Sly Joker",        COMMON_JOKER,    3, false, sly_joker_desc,              sly_joker_effect                  }, // 32 (orig #11)
    { "Jolly Joker",      COMMON_JOKER,    3, false, jolly_joker_desc,            jolly_joker_effect                }, // 33 (orig #6)
    { "Blue Joker",       COMMON_JOKER,    5, false, blue_joker_desc,             blue_joker_effect                 }, // 34 (orig #53)
    { "Odd Todd",         COMMON_JOKER,    4, false, odd_todd_joker_desc,         odd_todd_joker_effect             }, // 35 (orig #40)
    // Spritesheet 6
    { "The Duo",          RARE_JOKER,      8, false, the_duo_joker_desc,          the_duo_joker_effect              }, // 36 (orig #131)
    { "The Trio",         RARE_JOKER,      8, false, the_trio_joker_desc,         the_trio_joker_effect             }, // 37 (orig #132)
    { "The Order",        RARE_JOKER,      8, false, the_order_joker_desc,        the_order_joker_effect            }, // 38 (orig #134)
    { "The Tribe",        RARE_JOKER,      8, false, the_tribe_joker_desc,        the_tribe_joker_effect            }, // 39 (orig #135)
    // Spritesheet 7
    { "The Family",       RARE_JOKER,      8, false, the_family_joker_desc,       the_family_joker_effect           }, // 40 (orig #133)
    { "Brainstorm",       RARE_JOKER,     10, false, brainstorm_joker_desc,       blueprint_brainstorm_joker_effect }, // 41 Brainstorm (orig #138)
    // Spritesheet 8
    { "Smiley Face",      COMMON_JOKER,    4, false, smiley_face_joker_desc,      smiley_face_joker_effect          }, // 42 (orig #104)
    { "Bull",             UNCOMMON_JOKER,  6, false, bull_joker_desc,             bull_joker_effect                 }, // 43 (orig #93)
    // Individual Jokers (for now :3)
    { "Photograph",       COMMON_JOKER,    5, false, photograph_joker_desc,       photograph_joker_effect,          }, // 44 (orig #78)
    { "Hack",             UNCOMMON_JOKER,  6, false, hack_joker_desc,             hack_joker_effect                 }, // 45 (orig #36)
    { "Pareidolia",       UNCOMMON_JOKER,  5, false, pareidolia_joker_desc,       joker_effect_noop                 }, // 46 Pareidolia (orig #37)
    { "Bootstraps",       UNCOMMON_JOKER,  7, false, bootstraps_joker_desc,       bootstraps_joker_effect           }, // 47 (orig #145)
    { "Shortcut",         UNCOMMON_JOKER,  7, false, shortcut_joker_desc,         joker_effect_noop,                }, // 48 Shortcut (orig #69)
    { "Dusk",             UNCOMMON_JOKER,  5, false, dusk_joker_desc,             dusk_joker_effect                 }, // 49 (orig #28)
    { "Four Fingers",     UNCOMMON_JOKER,  7, false, four_fingers_joker_desc,     joker_effect_noop,                }, // 50 Four Fingers (orig #18)
    { "Seltzer",          UNCOMMON_JOKER,  6, false, seltzer_joker_desc,          seltzer_joker_effect,             }, // 51 (orig #102)
    { "Blueprint",        RARE_JOKER,     10, false, blueprint_joker_desc,        blueprint_brainstorm_joker_effect }, // 52 Blueprint (orig #123)

    // Spritesheet 18 (my_joker)
    { "Wee Joker",     RARE_JOKER,      8, true,  wee_joker_desc, wee_joker_effect              }, // 53 Wee Joker (orig #124)
    { "Riff-Raff",     COMMON_JOKER,    6, false, riff_raff_joker_desc, riff_raff_joker_effect        }, // 54 Riff-Raff (orig #67)
    { "Baron",         RARE_JOKER,      8, false, baron_joker_desc, baron_joker_effect            }, // 55 Baron (orig #72)
    { "Mime",          UNCOMMON_JOKER,  5, false, mime_joker_desc, mime_joker_effect             }, // 56 Mime (orig #19)
    { "Egg",           COMMON_JOKER,    4, false, egg_joker_desc, egg_joker_effect              }, // 57 Egg (orig #46)
    { "Smeared Joker", UNCOMMON_JOKER,  7, false, smeared_joker_desc, smeared_joker_effect        }, // 58 Smeared Joker (orig #113)
    { "Faceless Joker", COMMON_JOKER,    5, false, faceless_joker_desc, faceless_joker_effect     }, // 59 Faceless Joker (orig #57)
    { "Gros Michel",   COMMON_JOKER,    5, false, gros_michel_joker_desc, gros_michel_joker_effect }, // 60 Gros Michel (orig #38)
    { "Cavendish",     COMMON_JOKER,    5, false, cavendish_joker_desc, cavendish_joker_effect     }, // 61 Cavendish (orig #61)
        { "Flower Pot",    UNCOMMON_JOKER,  6, false, flower_pot_desc, flower_pot_effect              }, // 62 Flower Pot (orig #122)
        { "Loyalty Card",  UNCOMMON_JOKER,  5, false, loyalty_card_joker_desc, loyalty_card_joker_effect }, // 63 Loyalty Card (orig #25)
        { "Ride the Bus",  COMMON_JOKER,   6, false, riding_the_bus_joker_desc, riding_the_bus_joker_effect }, // 64 Ride the Bus (orig #44)
        { "Ceremonial Dagger", UNCOMMON_JOKER, 6, false, ceremonial_dagger_joker_desc, ceremonial_dagger_joker_effect }, // 65 Ceremonial Dagger (orig #21)
        { "Credit Card",      COMMON_JOKER,    1, false, credit_card_joker_desc,      credit_card_joker_effect      }, // 66 Credit Card (orig #20)
        { "Burglar",          UNCOMMON_JOKER,  6, false, burglar_joker_desc,        burglar_joker_effect            }, // 67 Burglar (orig #47)
        { "Flash Card",       UNCOMMON_JOKER,  5, false, flash_card_joker_desc,     flash_card_joker_effect         }, // 68 Flash Card (orig #96)
        { "Showman",          UNCOMMON_JOKER,  5, false, showman_joker_desc,        showman_joker_effect            }, // 69 Showman (orig #121)
        { "Card Sharp",       UNCOMMON_JOKER,  6, false, card_sharp_joker_desc,     card_sharp_joker_effect         }, // 70 Card Sharp (orig #62)
        { "To the Moon",      UNCOMMON_JOKER,  5, false, to_the_moon_joker_desc,     to_the_moon_joker_effect       }, // 71 To the Moon (orig #84)
        { "Splash",           COMMON_JOKER,    3, false, splash_joker_desc,           splash_joker_effect             }, // 72 Splash (orig #52), art by @MathisMartin31 (Discussion #69, 2026-05-14)
        { "Supernova",        COMMON_JOKER,    5, false, supernova_joker_desc,        supernova_joker_effect          }, // 73 Supernova (orig #43)
        { "Green Joker",      COMMON_JOKER,    4, false, green_joker_desc,            green_joker_effect              }, // 74 Green Joker (orig #58)
        { "Square Joker",     COMMON_JOKER,    4, false, square_joker_desc,           square_joker_effect             }, // 75 Square Joker (orig #65)
        { "Baseball Card",    RARE_JOKER,      8, false, baseball_card_desc,          baseball_card_effect            }, // 76 Baseball Card (orig #92)
        { "Stuntman",         RARE_JOKER,      7, false, stuntman_joker_desc,         stuntman_joker_effect           }, // 77 Stuntman (orig #136)
        { "Ancient Joker",    RARE_JOKER,      8, false, ancient_joker_desc,          ancient_joker_effect            }, // 78 Ancient Joker (orig #99)
        { "Swashbuckler",     COMMON_JOKER,    4, false, swashbuckler_joker_desc,     swashbuckler_joker_effect       }, // 79 Swashbuckler (orig #110)
        { "Gift Card",        UNCOMMON_JOKER,  6, false, gift_card_joker_desc,        gift_card_joker_effect          }, // 80 Gift Card (orig #79)

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
    int empty_slots = MAX_JOKERS_HELD_SIZE - list_get_len(jokers);
    u32 stencil_bonus = empty_slots > 0 ? (u32)empty_slots : 0;

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

    // M22: go through the suit mask so Smeared Joker applies (e.g. a played
    // Club also counts as Spades and must trigger Wrathful Joker).
    if (card_effective_suit_mask(scored_card->suit) & (1 << sinful_suit))
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

    // CLAMP: if the list somehow exceeds MAX_JOKERS_HELD_SIZE (e.g. pending
    // expired jokers still present), the int would go negative and, assigned
    // to a u32, wrap to ~4.29e9 - poisoning mult via protected_mult's
    // overflow (everything after it becomes UINT32_MAX garbage).
    int empty_slots = (MAX_JOKERS_HELD_SIZE)-num_jokers;
    (*joker_effect)->xmult = empty_slots > 0 ? (u32)empty_slots : 0;

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

JokerObject* resolve_copy_target(JokerObject* copying_joker)
{
    if (copying_joker == NULL || copying_joker->joker == NULL)
        return NULL;

    List* jokers = get_jokers_list();
    ListItr itr = list_itr_create(jokers);
    JokerObject* cur = NULL;

    // find ourselves in the Jokers list
    while ((cur = list_itr_next(&itr)))
    {
        if (cur == copying_joker)
        {
            break;
        }
    }
    if (cur != copying_joker)
    {
        return NULL;
    }

    // Bounce through copying jokers like blueprint_brainstorm_joker_effect:
    // Blueprint -> right neighbor, Brainstorm -> leftmost. Chain copies are
    // valid (Brainstorm -> Blueprint -> target). Guard against a Brainstorm
    // loop (two Brainstorms circling each other) with a counter.
    u8 brainstorm_counter = 0;
    do
    {
        switch (cur->joker->id)
        {
            case BLUEPRINT_JOKER_ID:
                cur = list_itr_next(&itr); // next right
                break;

            case BRAINSTORM_JOKER_ID:
                brainstorm_counter++;
                itr = list_itr_create(jokers);
                cur = list_itr_next(&itr); // leftmost
                break;

            default:
                // Non-copying joker: this is what the copy resolves to.
                return cur;
        }
    } while (cur != NULL && brainstorm_counter < 2);

    return cur; // may be NULL (blueprint at edge) or loop-guard exit
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
        TTE_BLACK_TAG "Each scored " TTE_YELLOW_TAG "2" TTE_BLACK_TAG
        " gives " TTE_BLUE_TAG "+8 " TTE_BLACK_TAG "Chips "
        "(Now " TTE_BLUE_TAG "+%ld " TTE_BLACK_TAG "Chips)";
    char desc[256];
    snprintf(desc, sizeof(desc), desc_format, (long)joker->scoring_state);
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int riff_raff_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "When blind starts, create " TTE_YELLOW_TAG "2 " TTE_BLACK_TAG
        "random " TTE_YELLOW_TAG "Common " TTE_BLACK_TAG
        "Jokers (Must have room)";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int baron_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each " TTE_YELLOW_TAG "King " TTE_BLACK_TAG "held in hand "
        "gives " TTE_RED_TAG "X1.5 " TTE_BLACK_TAG "Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int mime_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Retrigger all " TTE_YELLOW_TAG "cards held in hand";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int egg_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Gains " TTE_YELLOW_TAG "$3 " TTE_BLACK_TAG "of sell value "
        "each round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int smeared_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_DARK_GREEN_TAG "Clubs " TTE_BLACK_TAG "and " TTE_DARK_BLUE_TAG "Spades "
        TTE_BLACK_TAG "are the same suit, "
        TTE_YELLOW_TAG "Diamonds " TTE_BLACK_TAG "and " TTE_RED_TAG "Hearts "
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
                // Copies stay silent here: accumulation belongs to the real
                // joker, and the copy mirrors the final accumulated value at
                // ON_HAND_SCORED_END. Returning the running total per scored
                // 2 would double-count it (8+16+24 instead of 24+24).
                if (!s_is_copying_joker)
                {
                    // Normal mode: accumulate +8 chips and show upgrade animation
                    joker->scoring_state += 8;
                    *joker_effect = &s_shared_joker_effect;
                    (*joker_effect)->message = "Upgrade!";
                    return JOKER_EFFECT_FLAG_MESSAGE;
                }
            }
            break;

        case JOKER_EVENT_INDEPENDENT:
            // M26: apply the accumulated chips HERE, at Wee's own slot in the
            // left-to-right independent pass (原版: Wee gains chips at
            // On-Scored, adds them at Independent). Applying them at
            // ON_HAND_SCORED_END made the chips pop after every other joker
            // (e.g. after Gros Michel's +15), breaking slot order.
            {
                // Copies mirror the original's final accumulated value once.
                s32 chips_to_apply;
                if (s_is_copying_joker && s_copied_joker_source != NULL)
                    chips_to_apply = s_copied_joker_source->scoring_state;
                else
                    chips_to_apply = joker->scoring_state;

                if (chips_to_apply > 0)
                {
                    *joker_effect = &s_shared_joker_effect;
                    (*joker_effect)->chips = chips_to_apply;
                    return JOKER_EFFECT_FLAG_CHIPS;
                }
            }
            break;

        default:
            break;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Riff-Raff: When blind starts (cards dealt), create 2 random common jokers
// --- Unified deferred blind-selected action queue ----------------------------
// ON_BLIND_SELECTED dispatches left-to-right; every blind-selected joker
// (Riff-Raff spawns, Ceremonial Dagger sacrifice) enqueues ONE request in
// list order. A single per-frame scheduler then executes the queue strictly
// left-to-right, one action at a time: each action shows its trigger
// animation, waits DEFER_DELAY, then applies its effect. This gives the
// original Balatro resolution order:
//  - a Riff-Raff left of a Dagger spawns into the slots that exist when its
//    turn comes (before the Dagger sacrifices);
//  - the Dagger, when its turn comes, sacrifices whatever is immediately to
//    its right at that moment (which may be a joker a Riff-Raff to its left
//    just spawned - "right neighbor present => higher effective priority");
//  - a Riff-Raff right of a Dagger sees the list after the sacrifice.
#define DEFER_DELAY FRAMES(30) // ~0.5s beat - matches in-round card-move pacing

typedef enum
{
    DEFER_RIFF_RAFF, // activate: lock spawn count; fire: spawn jokers
    DEFER_DAGGER,    // activate: lock right neighbor; fire: sacrifice it
    DEFER_BURGLAR,   // activate: lock nothing; fire: +3 hands, 0 discards + HUD pulse
    DEFER_MSG,       // activate: show queued message text + shake; fire: nothing
} DeferredKind;

static JokerObject* s_deferred_queue[MAX_JOKERS_HELD_SIZE];
static DeferredKind s_deferred_kind[MAX_JOKERS_HELD_SIZE];
// Deep-copied message text for DEFER_MSG (M14c lesson: never store a
// pointer to caller stack/snprintf buffers - the queue fires later).
static char s_deferred_message[MAX_JOKERS_HELD_SIZE][16];
static int s_deferred_count = 0;
static int s_deferred_active = -1;
static int s_deferred_anim_count = 0; // Riff-Raff spawn count (locked at activation)
static JokerObject* s_deferred_victim = NULL; // Dagger victim (locked at activation)
static int s_deferred_wait_frames = 0; // Dagger victim settle wait (with cap)
#define DAGGER_MAX_WAIT FRAMES(90)     // ~1.5s max wait for victim to settle
static u32 s_deferred_fire_at = 0;

// After an effect fires (jokers spawned / victim sacrificed) the rack
// re-lays-out: new jokers slide in and everyone shifts left. Wait for all
// owned jokers to SETTLE (entry animation finished), then hold one more
// DEFER_DELAY beat before activating the next request - so each trigger
// animation plays against a still rack, like the in-round pacing.
static bool s_deferred_settle_wait = false;
static u32 s_deferred_next_beat_at = 0;

// --- ON_PLAYED growth message queue (2026-08-20, user req) ---
// Ride the Bus / Green Joker / Square Joker all grow on
// JOKER_EVENT_ON_HAND_PLAYED and used to pop "Upgrade!" synchronously -
// two growth jokers (e.g. Green Joker + Ride the Bus) popped on the same
// frame, overlapping animations. Growth still applies INSTANTLY inside
// the effect (semantics unchanged - the current hand benefits from the
// new value); ONLY the message pop is serialized: one joker per
// DEFER_DELAY beat, in dispatch order (left-to-right), like 原版's
// staggered per-joker upgrade pops.
static JokerObject* s_growth_msg_queue[MAX_JOKERS_HELD_SIZE];
static const char* s_growth_msg_text[MAX_JOKERS_HELD_SIZE];
static int s_growth_msg_count = 0;
static int s_growth_msg_active = -1;
static u32 s_growth_msg_next_at = 0;

static void growth_msg_enqueue(Joker* joker, const char* message)
{
    if (s_growth_msg_count >= MAX_JOKERS_HELD_SIZE)
        return; // Decorational animation: never block gameplay.

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
    if (self_object == NULL)
        return;

    s_growth_msg_queue[s_growth_msg_count] = self_object;
    s_growth_msg_text[s_growth_msg_count] = message;
    s_growth_msg_count++;
}

// Fires happen FRAMES after enqueue; the joker may have been sold/destroyed
// in between. Skip the whole queue on a dead source (decorational only).
static bool growth_msg_source_is_alive(JokerObject* jo)
{
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (cur == jo)
            return true;
    }
    return false;
}

void growth_msg_process_pending(void)
{
    if (s_growth_msg_count == 0)
        return;

    if (s_growth_msg_active < 0)
    {
        // Fire the first message now, next one after one beat.
        s_growth_msg_active = 0;
        if (!growth_msg_source_is_alive(s_growth_msg_queue[0]))
        {
            growth_msg_clear();
            return;
        }
        joker_show_message(s_growth_msg_queue[0], s_growth_msg_text[0]);
        s_growth_msg_next_at = game_get_ui_tick() + DEFER_DELAY;
        return;
    }

    if (game_get_ui_tick() >= s_growth_msg_next_at)
    {
        s_growth_msg_active++;
        if (s_growth_msg_active >= s_growth_msg_count)
        {
            s_growth_msg_count = 0;
            s_growth_msg_active = -1;
            return;
        }
        if (!growth_msg_source_is_alive(s_growth_msg_queue[s_growth_msg_active]))
        {
            growth_msg_clear();
            return;
        }
        joker_show_message(s_growth_msg_queue[s_growth_msg_active],
                           s_growth_msg_text[s_growth_msg_active]);
        s_growth_msg_next_at = game_get_ui_tick() + DEFER_DELAY;
    }
}

void growth_msg_clear(void)
{
    s_growth_msg_count = 0;
    s_growth_msg_active = -1;
}

// True while any ON_PLAYED growth message is still queued or being shown.
// round.c gates the hand-play -> scoring transition on this: 原版 runs
// the whole On Played phase (incl. per-joker upgrade pops) before the
// cards start scoring.
bool growth_msg_pending(void)
{
    return s_growth_msg_count > 0;
}

// --- Baseball Card trigger animation queue (2026-08-21) ---
// 原版 (user-observed): at INDEPENDENT the animation is TWO-DIMENSIONALLY
// serial. Each Baseball Card effect source (real card + Blueprint/Brainstorm
// copies resolving to one via chain) has its OWN turn, in list order
// (e.g. [Blueprint, Baseball, Smeared, Sock&Buskin, Brainstorm] ->
// Blueprint round -> Baseball round -> Brainstorm round). Within a round
// the Uncommon targets shake one after another (list order), each pair
// being: SOURCE shakes + TARGET shakes (same frame), red "X1.5" pops
// below the target. A source shakes ONLY during its own round - it never
// shakes alongside another source's round.
// The multiplier applies INSTANTLY in joker.c's hook (order-correct);
// ONLY this animation is serialized, one beat per (source, target) pair.
#define BASEBALL_MAX_SOURCES MAX_JOKERS_HELD_SIZE

typedef struct
{
    JokerObject* source;                            // this round's effect instance
    JokerObject* targets[MAX_JOKERS_HELD_SIZE];     // Uncommon targets, list order
    int target_count;
} BaseballAnimSource;

static BaseballAnimSource s_baseball_sources[BASEBALL_MAX_SOURCES];
static int s_baseball_source_count = 0;
static int s_baseball_active_source = -1;
static int s_baseball_active_target = 0;
static u32 s_baseball_anim_next_at = 0;

// True if the joker is a Baseball Card effect source (real or a copy whose
// chain-resolved target is a Baseball Card).
static bool baseball_joker_is_source(JokerObject* jo)
{
    if (jo == NULL || jo->joker == NULL)
        return false;
    if (jo->joker->id == BASEBALL_CARD_ID)
        return true;
    JokerObject* copy_target = resolve_copy_target(jo);
    return (copy_target != NULL && copy_target->joker != NULL &&
            copy_target->joker->id == BASEBALL_CARD_ID);
}

// Called from joker.c's INDEPENDENT hook when an Uncommon joker scored:
// append the target to EVERY baseball source's round (sources built in
// list order on first use).
void baseball_anim_register_trigger(JokerObject* target)
{
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (!baseball_joker_is_source(cur))
            continue;

        // Find or append this source's round.
        int src_idx = -1;
        for (int i = 0; i < s_baseball_source_count; i++)
        {
            if (s_baseball_sources[i].source == cur)
            {
                src_idx = i;
                break;
            }
        }
        if (src_idx < 0)
        {
            if (s_baseball_source_count >= BASEBALL_MAX_SOURCES)
                return; // Decorational: never block gameplay.
            src_idx = s_baseball_source_count++;
            s_baseball_sources[src_idx].source = cur;
            s_baseball_sources[src_idx].target_count = 0;
        }

        BaseballAnimSource* src = &s_baseball_sources[src_idx];
        if (src->target_count < MAX_JOKERS_HELD_SIZE)
            src->targets[src->target_count++] = target;
    }
}

// Fires happen FRAMES after enqueue; source/target may have been
// sold/destroyed in between. Skip the whole queue on a dead pair.
static bool baseball_anim_pair_is_alive(JokerObject* source, JokerObject* target)
{
    bool seen_source = false;
    bool seen_target = false;
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (cur == source)
            seen_source = true;
        if (cur == target)
            seen_target = true;
    }
    return seen_source && seen_target;
}

static void baseball_anim_play_active_pair(void)
{
    BaseballAnimSource* src = &s_baseball_sources[s_baseball_active_source];
    joker_play_baseball_animation(src->source,
                                  src->targets[s_baseball_active_target]);
}

void baseball_anim_process_pending(void)
{
    if (s_baseball_source_count == 0)
        return;

    if (s_baseball_active_source < 0)
    {
        s_baseball_active_source = 0;
        s_baseball_active_target = 0;
        // Skip rounds with no targets (shouldn't happen - register_trigger
        // only creates rounds when it appends a target).
        if (s_baseball_sources[0].target_count == 0)
        {
            baseball_anim_clear();
            return;
        }
        if (!baseball_anim_pair_is_alive(s_baseball_sources[0].source,
                                         s_baseball_sources[0].targets[0]))
        {
            baseball_anim_clear();
            return;
        }
        baseball_anim_play_active_pair();
        s_baseball_anim_next_at = game_get_ui_tick() + DEFER_DELAY;
        return;
    }

    if (game_get_ui_tick() >= s_baseball_anim_next_at)
    {
        // Advance within the current round.
        s_baseball_active_target++;
        while (s_baseball_active_source < s_baseball_source_count)
        {
            BaseballAnimSource* src = &s_baseball_sources[s_baseball_active_source];
            if (s_baseball_active_target < src->target_count)
                break; // Next pair in this round.
            // This round is done - move to the next source's round.
            s_baseball_active_source++;
            s_baseball_active_target = 0;
        }

        if (s_baseball_active_source >= s_baseball_source_count)
        {
            s_baseball_source_count = 0;
            s_baseball_active_source = -1;
            return;
        }

        BaseballAnimSource* src = &s_baseball_sources[s_baseball_active_source];
        if (!baseball_anim_pair_is_alive(src->source,
                                         src->targets[s_baseball_active_target]))
        {
            baseball_anim_clear();
            return;
        }
        baseball_anim_play_active_pair();
        s_baseball_anim_next_at = game_get_ui_tick() + DEFER_DELAY;
    }
}

void baseball_anim_clear(void)
{
    s_baseball_source_count = 0;
    s_baseball_active_source = -1;
    s_baseball_active_target = 0;
}

bool baseball_anim_pending(void)
{
    return s_baseball_source_count > 0;
}

// Burglar-only serial pacing: after the +3 hands/discards fire, wait for
// the HUD value roll to fully drain before the next trigger activates.
// This serializes shake -> number roll (matching 原版); without it the
// 30-frame deferred beat races ahead of the ~98-frame/item roll and 5
// copies overflow the 8-slot HUD queue, dropping the final roll and
// freezing the HUD at 16 instead of reaching 19.
static bool s_deferred_wait_hud_roll = false;

// True once any queued request has actually shown its trigger animation or
// locked a real effect this round (i.e. the queue wasn't entirely silent -
// e.g. Riff-Raff found no free slot, Dagger found no right neighbor). The
// round uses this to skip the post-effects beat when nothing visibly
// happened, so silent rounds deal the hand immediately.
static bool s_deferred_ran_animation = false;

// True if this round's deferred queue actually produced a visible effect.
bool deferred_effects_ran_animation(void)
{
    return s_deferred_ran_animation;
}

// Defined below in the dagger section; used by the deferred scheduler.
static void dagger_sacrifice(JokerObject* dagger_object, JokerObject* victim);

// True if the source object is still an alive owned joker: present in the
// owned list AND not currently in the expired list (a Dagger may have just
// sacrificed it - it must not activate its spawn anymore).
static bool deferred_source_is_alive(JokerObject* source)
{
    if (source == NULL)
        return false;

    bool alive = false;
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (cur == source)
        {
            alive = true;
            break;
        }
    }
    if (alive)
    {
        ListItr eitr = list_itr_create(get_expired_jokers_list());
        while ((cur = list_itr_next(&eitr)))
        {
            if (cur == source)
            {
                alive = false;
                break;
            }
        }
    }
    return alive;
}

// Enqueue a message to show on the given joker through the unified
// deferred queue (serialized, one beat each - 回合结束/盲注阶段消息不再
// 同步重叠). Resolves self_object by joker pointer like Riff-Raff.
// Deep-copies the text (M14c: queue fires later, no dangling pointers).
static void deferred_enqueue_message(Joker* joker, const char* text)
{
    if (joker == NULL || text == NULL)
        return;
    if (s_deferred_count >= MAX_JOKERS_HELD_SIZE)
        return;

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
    if (self_object == NULL)
        return;

    s_deferred_queue[s_deferred_count] = self_object;
    s_deferred_kind[s_deferred_count] = DEFER_MSG;
    s_deferred_message[s_deferred_count][0] = '\0';
    strncat(s_deferred_message[s_deferred_count], text,
            sizeof(s_deferred_message[s_deferred_count]) - 1);
    s_deferred_count++;
}

// M24: true while any deferred blind-selected effect is still queued.
// round.c gates card dealing on this (plus hud_roll_is_active()) so HUD
// numbers don't change after cards are dealt.
bool deferred_effects_pending(void)
{
    return s_deferred_count > 0;
}

// Called every frame from game.c's jokers_update_loop(). Advances the
// deferred queue strictly left-to-right: activate next request (lock its
// action) -> show animation -> wait DEFER_DELAY -> apply effect.
void deferred_effects_process_pending(void)
{
    if (s_deferred_count == 0)
        return;
    // Burglar serial pacing: wait for the HUD value roll to drain before
    // the next trigger activates (set by the Burglar fire branch).
    if (s_deferred_wait_hud_roll)
    {
        if (hud_roll_is_active())
            return;
        s_deferred_wait_hud_roll = false;
        s_deferred_next_beat_at = game_get_ui_tick() + DEFER_DELAY;
        return;
    }
    // Post-fire pacing: after the last effect fired, wait for the rack to
    // settle (entry/re-layout animations finished), then hold one beat
    // before the next request activates.
    if (s_deferred_settle_wait)
    {
        bool all_settled = true;
        ListItr itr = list_itr_create(get_jokers_list());
        JokerObject* cur;
        while ((cur = list_itr_next(&itr)))
        {
            if (cur->x != cur->tx || cur->y != cur->ty)
            {
                all_settled = false;
                break;
            }
        }
        if (all_settled)
        {
            s_deferred_settle_wait = false;
            s_deferred_next_beat_at = game_get_ui_tick() + DEFER_DELAY; // M24
        }
        return;
    }
    if (s_deferred_next_beat_at != 0)
    {
        if (game_get_ui_tick() < s_deferred_next_beat_at) // M24
            return;
        s_deferred_next_beat_at = 0;
    }

    // While an effect is armed (waiting to fire), re-validate its source
    // every frame: if it was sacrificed/removed meanwhile, abandon the fire
    // (clear the timer); the activation branch below then advances past it.
    // NOTE: when the timer is already clear this branch is skipped - the
    // activation branch owns all advancing and skips dead requests itself,
    // so a dead request can never wedge the queue.
    if (s_deferred_active >= 0 && s_deferred_fire_at != 0 &&
        !deferred_source_is_alive(s_deferred_queue[s_deferred_active]))
    {
        s_deferred_fire_at = 0;
        return;
    }

    // Activate the next queued request, skipping any whose source is dead
    // (sacrificed by a Dagger that fired earlier). Active index advances
    // ONLY here (first frame starts at 0; each fire completion clears the
    // timer which brings us back here to advance) - the fire branch below
    // must NOT increment it, or every request after the first is skipped.
    if (s_deferred_fire_at == 0)
    {
        JokerObject* source;
        do
        {
            if (s_deferred_active < 0)
                s_deferred_active = 0;
            else
                s_deferred_active++;
            if (s_deferred_active >= s_deferred_count)
            {
                s_deferred_count = 0;
                s_deferred_active = -1;
                return;
            }
            source = s_deferred_queue[s_deferred_active];
        } while (source == NULL || source->joker == NULL ||
                 !deferred_source_is_alive(source));

        switch (s_deferred_kind[s_deferred_active])
        {
            case DEFER_RIFF_RAFF:
            {
                // Lock the spawn count from the CURRENT effective occupancy
                // (expired jokers are about to be removed, their slots are
                // usable - same rule as the fire branch below). In the strict
                // left-to-right order, slots freed by a Dagger that fired
                // BEFORE this Riff-Raff are already visible here.
                int free_slots = MAX_JOKERS_HELD_SIZE -
                                 (list_get_len(get_jokers_list()) -
                                  list_get_len(get_expired_jokers_list()));
                if (free_slots <= 0)
                {
                    // No room right now - skip silently (no animation)
                    s_deferred_fire_at = 0;
                    return;
                }
                s_deferred_anim_count = free_slots < 2 ? free_slots : 2;

                char anim_buffer[16];
                snprintf(
                    anim_buffer,
                    sizeof(anim_buffer),
                    "+%d Jokers",
                    s_deferred_anim_count
                );
                tte_set_pos(fx2int(source->x) + TILE_SIZE, JOKER_SCORE_TEXT_Y);
                tte_set_special(TTE_WHITE_PB * TTE_SPECIAL_PB_MULT_OFFSET);
                tte_write(anim_buffer);
                joker_object_shake(source, UNDEFINED);
                schedule_joker_event_text_clear();
                s_deferred_ran_animation = true;
                break;
            }

            case DEFER_DAGGER:
            {
                // Lock the victim: whatever is immediately right of the
                // dagger NOW (a Riff-Raff left of it may have just spawned
                // new jokers that landed to the right).
                JokerObject* victim = NULL;
                ListItr itr = list_itr_create(get_jokers_list());
                JokerObject* cur;
                while ((cur = list_itr_next(&itr)))
                {
                    if (cur == source)
                    {
                        victim = list_itr_next(&itr);
                        break;
                    }
                }
                if (victim == NULL || victim->joker == NULL)
                {
                    // Nothing to sacrifice - skip silently
                    s_deferred_fire_at = 0;
                    return;
                }
                s_deferred_victim = victim;
                s_deferred_ran_animation = true;
                break;
            }

            case DEFER_BURGLAR:
            {
                // Activate: show the white "+3 hands!" message and shake
                // NOW (this instance's turn). The actual hands/discards
                // mutation happens in the fire branch one beat later.
                char anim_buffer[16];
                snprintf(anim_buffer, sizeof(anim_buffer), "+3 hands!");
                tte_set_pos(fx2int(source->x) + TILE_SIZE, JOKER_SCORE_TEXT_Y);
                tte_set_special(TTE_WHITE_PB * TTE_SPECIAL_PB_MULT_OFFSET);
                tte_write(anim_buffer);
                joker_object_shake(source, UNDEFINED);
                schedule_joker_event_text_clear();
                s_deferred_ran_animation = true;
                break;
            }

            case DEFER_MSG:
            {
                // Activate: show the queued message text (round-end
                // payouts like Egg "+$3" / Gift Card "+$1", Ancient suit
                // name, ...) - serialized one beat each. No fire effect
                // (the message IS the whole effect).
                tte_set_pos(fx2int(source->x) + TILE_SIZE, JOKER_SCORE_TEXT_Y);
                tte_set_special(TTE_WHITE_PB * TTE_SPECIAL_PB_MULT_OFFSET);
                tte_write(s_deferred_message[s_deferred_active]);
                joker_object_shake(source, UNDEFINED);
                schedule_joker_event_text_clear();
                s_deferred_ran_animation = true;
                break;
            }
        }

        s_deferred_fire_at = game_get_ui_tick() + DEFER_DELAY; // M24
        return;
    }

    // Time to apply the active request's effect
    if (game_get_ui_tick() >= s_deferred_fire_at) // M24
    {
        JokerObject* source = s_deferred_queue[s_deferred_active];
        switch (s_deferred_kind[s_deferred_active])
        {
            case DEFER_RIFF_RAFF:
            {
                int to_spawn = s_deferred_anim_count;
                // NEVER overfill: cap by the ACTUAL list length (including
                // expired-but-not-yet-removed jokers, which still occupy
                // their list slot and would push us past the rack size if
                // counted out - add_joker() has no capacity check).
                int current_count = list_get_len(get_jokers_list());
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
                        u8 candidate = rng_get_u32(RNG_SEQ_JOKER_RIFF_RAFF) %
                                        get_joker_registry_size();
                        const JokerInfo* info = get_joker_registry_entry(candidate);
                        if (info && info->rarity == rarity &&
                            (is_showman_joker_active() || !is_joker_owned(candidate)))
                        {
                            if (candidate == GROS_MICHEL_ID &&
                                is_gros_michel_destroyed())
                                continue;
                            if (candidate == CAVENDISH_ID &&
                                !is_gros_michel_destroyed())
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
                    // Mark as non-rollable so shop won't generate the same
                    joker_set_rollable(joker_id, false);
                    current_count++;
                }
                break;
            }

            case DEFER_DAGGER:
            {
                JokerObject* victim = s_deferred_victim;
                // Verify the victim is still in the owned list AND still
                // immediately right of the dagger (it may have been
                // sold/expired, or the rack reordered meanwhile).
                bool victim_still_right = false;
                ListItr itr = list_itr_create(get_jokers_list());
                JokerObject* cur;
                while ((cur = list_itr_next(&itr)))
                {
                    if (cur == source)
                    {
                        if (list_itr_next(&itr) == victim)
                            victim_still_right = true;
                        break;
                    }
                }
                if (!victim_still_right)
                {
                    // Victim no longer sacrificeable - skip silently
                    s_deferred_victim = NULL;
                    s_deferred_wait_frames = 0;
                    break;
                }

                // Wait for its entry animation to finish so it settles into
                // its slot first. Cap the wait so a stuck animation can't
                // wedge the queue (and hand dealing) forever.
                if (victim->x != victim->tx || victim->y != victim->ty)
                {
                    s_deferred_wait_frames += DEFER_DELAY;
                    if (s_deferred_wait_frames < DAGGER_MAX_WAIT)
                    {
                        s_deferred_fire_at = game_get_ui_tick() + DEFER_DELAY; // M24
                        return;
                    }
                    // Timed out waiting - sacrifice it where it stands
                }
                dagger_sacrifice(source, victim);
                s_deferred_victim = NULL;
                s_deferred_wait_frames = 0;
                break;
            }

            case DEFER_BURGLAR:
            {
                // Fire: apply +3 hands, lose all discards. Queue the HUD
                // roll SEQUENTIALLY (原版 2026-08-08: white "+3" overlays
                // the hands number first, hands roll up to target, THEN
                // discards roll down to 0; if target == current value no
                // roll). Each queued instance (real + Blueprint +
                // Brainstorm copies) fires one beat apart.
                int old_hands = g_game_vars.hands;
                int old_discards = g_game_vars.discards;
                g_game_vars.hands += 3;
                g_game_vars.discards = 0;
                hud_enqueue_value_roll(
                    &HANDS_TEXT_ROLL_ERASE_RECT,
                    &HANDS_TEXT_RECT,
                    &HANDS_TEXT_ERASE_RECT,
                    HUD_TARGET_HANDS,
                    TTE_BLUE_PB,
                    "+3",
                    old_hands,
                    g_game_vars.hands
                );
                // Discards label: "-<old>" (the amount lost, since
                // discards goes to 0). Use a small buffer for the digits.
                char discards_label[8];
                snprintf(discards_label, sizeof(discards_label), "-%d", old_discards);
                hud_enqueue_value_roll(
                    &DISCARDS_TEXT_ROLL_ERASE_RECT,
                    &DISCARDS_TEXT_RECT,
                    &DISCARDS_TEXT_ERASE_RECT,
                    HUD_TARGET_DISCARDS,
                    TTE_RED_PB,
                    discards_label,
                    old_discards,
                    g_game_vars.discards
                );
                break;
            }

            case DEFER_MSG:
            {
                // Fire: nothing - the message shown at activation IS the
                // whole effect.
                break;
            }
        }

        // Advance to the next request: just clear the timer so the
        // activation branch advances s_deferred_active next frame (never
        // increment here - see note at the activation branch). First wait
        // for the rack to settle (spawned jokers' entry / re-layout) plus
        // one beat, so the next trigger animation plays against a still rack.
        s_deferred_fire_at = 0;
        if (s_deferred_active + 1 >= s_deferred_count)
        {
            s_deferred_count = 0;
            s_deferred_active = -1;
        }
        else if (s_deferred_kind[s_deferred_active] == DEFER_BURGLAR)
        {
            // Burglar: serialize the shake with its HUD number roll. Wait
            // for the roll queue to drain before the next trigger activates
            // (the rack doesn't re-layout for Burglar, so settle_wait is
            // meaningless here - what must settle is the HUD roll).
            s_deferred_wait_hud_roll = true;
        }
        else
        {
            s_deferred_settle_wait = true;
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
                s_deferred_count < MAX_JOKERS_HELD_SIZE)
            {
                // Enqueue (no count locking here: the scheduler locks the
                // spawn count when this request activates, from the list
                // state at that moment - strict left-to-right order).
                if (s_deferred_count == 0)
                {
                    s_deferred_ran_animation = false;
                    s_deferred_settle_wait = false;
                    s_deferred_next_beat_at = 0;
                    s_deferred_wait_hud_roll = false;
                }
                s_deferred_queue[s_deferred_count] = self_object;
                s_deferred_kind[s_deferred_count] = DEFER_RIFF_RAFF;
                s_deferred_count++;
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
        // x1.5 as fraction 3/2 via the fractional XMULT channel - no float.
        // Renders red "X1.5" (colored settlement value, not an upgrade).
        // MUST publish the shared instance via *joker_effect first: the
        // caller (joker_object_score) reads the effect through THIS pointer,
        // which starts NULL. Setting the shared struct directly without
        // publishing it leaves the caller dereferencing NULL (reads 0) and
        // the whole XMULT is skipped - mult never changes at all.
        *joker_effect = &s_shared_joker_effect;
        joker_effect_set_xmult_den(&s_shared_joker_effect, 3, 2);
        return JOKER_EFFECT_FLAG_XMULT;
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
    // Mime retriggers the whole "held in hand" scoring pass: when held cards
    // are walked (JOKER_EVENT_ON_CARD_HELD per card), EVERY joker that hooks
    // that pass runs again - Baron (Kings x1.5), Shoot the Moon (Queens +13
    // mult), Steel/Gold/Blue Seals (card abilities). It does NOT retrigger
    // the normal joker scoring order (Blackboard, Ride the Bus etc.).
    //
    // Implementation lives in round.c (play_scoring_held_cards_update):
    // after the walk finishes, it counts Mime effects present (real card +
    // Blueprint/Brainstorm copies) and re-runs the whole walk that many
    // times. Each extra pass re-triggers Baron/Shoot the Moon for every
    // qualifying held card, and shows "Again!" on the Mime (with an actual
    // retrigger target, like Sock and Buskin).
    //
    // This effect function itself does nothing: the retrigger is driven by
    // the round loop, not by a per-card effect return.
    (void)joker;
    (void)scored_card;
    (void)joker_effect;

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
        // sell_value = value / 2, so +6 to value = +$3 sell value.
        // The value change is immediate; the "+$3" pop is serialized
        // through the deferred queue (round-end messages no longer
        // overlap, e.g. Egg + Gift Card - M34 follow-up).
        joker->value += 6;
        deferred_enqueue_message(joker, "+$3");
        return JOKER_EFFECT_FLAG_NONE;
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
        TTE_YELLOW_TAG "3 or more Face Cards"
        TTE_BLACK_TAG ", earn " TTE_YELLOW_TAG "$5";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int gros_michel_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Provides " TTE_RED_TAG "+15 Mult"
        TTE_BLACK_TAG ". " TTE_YELLOW_TAG "1 in 6"
        TTE_BLACK_TAG " chance this card is destroyed at end of round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int cavendish_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Provides " TTE_RED_TAG "X3 Mult"
        TTE_BLACK_TAG ". " TTE_YELLOW_TAG "1 in 1000"
        TTE_BLACK_TAG " chance this card is destroyed at end of round";
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

// Gros Michel: +15 mult, 1/6 chance to self-destruct at end of round
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

    // Official timing: "1 in 6 chance this is destroyed at END OF ROUND"
    // (fandom #38) - one extinction roll per round, not per hand.
    if (joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        // Don't self-destruct when being copied by Blueprint/Brainstorm
        if (!s_is_copying_joker && (rng_get_u32(RNG_SEQ_JOKER_GROS_MICHEL) % 6) == 0)
        {
            // Self-destruct! Demake-specific rule: extinction is GLOBAL -
            // every Gros Michel in play dies together (the player may hold
            // several, since it stays in the pool while alive).
            set_gros_michel_destroyed();
            expire_all_gros_michel();
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->message = "EXTINCT!";
            (*joker_effect)->expire = true;
            return JOKER_EFFECT_FLAG_MESSAGE | JOKER_EFFECT_FLAG_EXPIRE;
        }
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// Cavendish: x3 mult, 1/1000 chance to self-destruct at end of round
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

    // Official timing: "1 in 1000 chance this is destroyed at END OF ROUND"
    // (fandom #61) - same as Gros Michel, one roll per round.
    if (joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        // Don't self-destruct when being copied by Blueprint/Brainstorm
        if (!s_is_copying_joker && (rng_get_u32(RNG_SEQ_JOKER_CAVENDISH) % 1000) == 0)
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
        TTE_YELLOW_TAG "all 4 suits"
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
        TTE_BLACK_TAG "Every " TTE_YELLOW_TAG "6 hands" TTE_BLACK_TAG " played, "
        TTE_RED_TAG "X4 Mult" TTE_BLACK_TAG ", " TTE_YELLOW_TAG "%d" TTE_BLACK_TAG " remaining",
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

    // In copy mode (Blueprint/Brainstorm) the counter MUST be shared with
    // the source joker: the copy reads and decrements the SOURCE's
    // persistent_state, so both trigger on the same hand and stay in sync.
    // (Otherwise the copy counts independently and fires on a different
    // hand than the real card - Brainstorm in particular, which copies the
    // leftmost joker, would desync.)
    s32* p_remaining =
        s_is_copying_joker && s_copied_joker_source != NULL
            ? &(s_copied_joker_source->persistent_state)
            : &(joker->persistent_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            if (!s_is_copying_joker)
                *p_remaining = LOYALTY_CARD_HANDS_REQUIRED - 1; // 5 hands remaining
            break;

        case JOKER_EVENT_INDEPENDENT:
            // Remaining == 0 means this hand gets the X4. Trigger check ONLY
            // here - no reset, no decrement: those happen in
            // ON_HAND_SCORED_END. This guarantees every joker (real card AND
            // copies) sees the SAME counter value during this event regardless
            // of list order. If the real card reset here, a copy to its RIGHT
            // would read 5 instead of 0 and never fire on the trigger hand;
            // if it decremented here, a copy to its right would read the
            // just-decremented 0 and fire one hand early.
            if (*p_remaining == 0)
            {
                *joker_effect = &s_shared_joker_effect;
                (*joker_effect)->xmult = 4;
                effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
                // Copy or real card: both fire X4 on the trigger hand. The
                // real card alone resets the cycle in ON_HAND_SCORED_END.
            }
            break;

        case JOKER_EVENT_ON_HAND_SCORED_END:
            // Show remaining hands until next X4 (real joker only, not copies)
            if (!s_is_copying_joker)
            {
                // End of the trigger hand: reset the cycle. Otherwise just
                // decrement. Runs AFTER the trigger check, so no joker will
                // re-read the counter this hand - ordering can't desync.
                if (*p_remaining == 0)
                    *p_remaining = LOYALTY_CARD_HANDS_REQUIRED - 1; // 5
                else
                    (*p_remaining)--;

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

// --- Ride the Bus (ID 64) ---

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
        TTE_YELLOW_TAG "Face Card" TTE_BLACK_TAG " (currently " TTE_RED_TAG "+%ld "
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

        case JOKER_EVENT_ON_HAND_PLAYED:
        {
            // M26: growth belongs to the On-Played phase (原版 timing per the
            // wiki phase table: "On Played (成长) + Independent") — the +1
            // upgrade pop fires when the hand is played, decoupled from the
            // mult application at Independent (which now pops in slot order
            // like every other joker). The current hand still benefits from
            // the +1, exactly as before. Copies stay silent (Blueprint
            // mirrors only the mult at Independent).
            if (s_is_copying_joker)
                break;

            extern CardObject** get_played_hand(void);
            extern int get_played_top(void);
            CardObject** played = get_played_hand();
            int top = get_played_top();
            bool face_card_will_score = false;
            for (int i = 0; i <= top; i++)
            {
                if (played[i] != NULL && card_object_is_scoring(played[i]) &&
                    card_is_face(played[i]->card))
                {
                    face_card_will_score = true;
                    break;
                }
            }

            if (!face_card_will_score)
            {
                (*p_accumulated_mult)++;
                // Serialized pop: growth applies instantly, the "Upgrade!"
                // animation goes through the growth message queue so
                // multiple growth jokers don't pop on the same frame.
                growth_msg_enqueue(joker, "Upgrade!");
            }
            break;
        }

        case JOKER_EVENT_INDEPENDENT:
        {
            // M26: apply ONLY the accumulated mult here (slot order); the
            // growth/upgrade animation moved to ON_HAND_PLAYED.
            s32 mult_to_apply;
            if (s_is_copying_joker && s_copied_joker_source != NULL)
            {
                // Copy mode: mirror the original's accumulated value
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

        case JOKER_EVENT_ON_CARD_SCORED:
            // A face card scoring breaks the streak instantly
            // (card_is_face respects Pareidolia; copies stay silent)
            if (scored_card != NULL && !s_is_copying_joker &&
                card_is_face(scored_card))
            {
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
        TTE_BLACK_TAG "On blind select, destroy " TTE_YELLOW_TAG "Joker to the right"
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
// The sacrifice is NOT immediate: the dagger enqueues a deferred request at
// its list position and the unified scheduler runs it in strict left-to-right
// order. When its turn comes it locks whatever is immediately to its right
// (which may be a joker a Riff-Raff to its left just spawned) and, after the
// beat, sacrifices it once it has settled.

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

    // Upgrade message only (white, Wee Joker pattern). The red "+N Mult"
    // settlement value is reported at INDEPENDENT during hand scoring -
    // showing it here too would duplicate the settlement display.
    tte_set_pos(fx2int(dagger_object->x) + TILE_SIZE, JOKER_SCORE_TEXT_Y);
    tte_set_special(TTE_WHITE_PB * TTE_SPECIAL_PB_MULT_OFFSET);
    tte_write("Upgrade!");
    schedule_joker_event_text_clear();

    joker_object_shake(dagger_object, SFX_MULT);
    joker_object_shake(victim, UNDEFINED);
    list_push_back(get_expired_jokers_list(), victim);
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

            // Enqueue a deferred sacrifice at this dagger's list position:
            // the unified scheduler runs the queue strictly left-to-right,
            // so jokers LEFT of the dagger fire first (a Riff-Raff there may
            // spawn new jokers that land to the dagger's right - the dagger
            // then eats one when its own turn comes).
            if (s_deferred_count < MAX_JOKERS_HELD_SIZE)
            {
                ListItr itr = list_itr_create(get_jokers_list());
                JokerObject* cur;
                while ((cur = list_itr_next(&itr)))
                {
                    if (cur->joker == joker)
                    {
                        s_deferred_queue[s_deferred_count] = cur;
                        s_deferred_kind[s_deferred_count] = DEFER_DAGGER;
                        if (s_deferred_count == 0)
                        {
                            s_deferred_ran_animation = false;
                            s_deferred_settle_wait = false;
                            s_deferred_next_beat_at = 0;
                            s_deferred_wait_hud_roll = false;
                        }
                        s_deferred_count++;
                        break;
                    }
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
// running (unified deferred action queue non-empty). The round waits for
// these before dealing the hand, so all joker effects play out first and
// cards are dealt after.
bool joker_effects_busy(void)
{
    return s_deferred_count > 0;
}

// --------------------------------------------------------------------------
// Credit Card (66)
// Shop purchases may go into debt down to -20$ per REAL Credit Card held.
// NOTE: Blueprint/Brainstorm CANNOT copy this effect - it is a passive
// state (no event trigger), so a copying Joker invoking its (no-op) effect
// function produces nothing. Only actual Credit Cards count. Passive
// effects like this, Flower Pot, Smeared Joker etc. are not copyable;
// only jokers with active outputs (mult/chips/Riff-Raff spawns) are.
// --------------------------------------------------------------------------

static int credit_card_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    static const char desc[] =
        TTE_YELLOW_TAG "Go up to -20$" TTE_BLACK_TAG " in debt";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

// The card has no per-event effect: its power is the passive debt limit
// queried by count_credit_card_effects(). Kept as a real function so the
// registry entry is uniform.
static u32 credit_card_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    (void)joker;
    (void)scored_card;
    (void)joker_event;
    (void)joker_effect;
    return JOKER_EFFECT_FLAG_NONE;
}

// Count REAL Credit Cards in the held jokers list. Queried live by the
// shop at purchase time; each real card adds 20$ of debt headroom.
int count_credit_card_effects(void)
{
    int count = 0;
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;

    while ((cur = list_itr_next(&itr)))
    {
        if (cur->joker == NULL)
            continue;

        if (cur->joker->id == CREDIT_CARD_ID)
            count++;
    }

    return count;
}

// --------------------------------------------------------------------------
// Flash Card (68)
// Gains +2 Mult every time the shop is rerolled.
// Triggers on JOKER_EVENT_ON_SHOP_REROLL (dispatched from shop.c after
// reroll). Blueprint/Brainstorm copies DO re-execute (event-triggered
// action), each copy grants another +2.
// High-frequency (reroll spam): the "+2" text is rendered immediately on
// top of the previous one (same fixed joker position) via joker_object_score
// -> tte_write, so rapid rerolls overwrite instead of queueing - no
// animation backlog. This matches how shop items shake on reroll.
// --------------------------------------------------------------------------

static int flash_card_joker_desc(Joker* joker, Rect dest_rect)
{
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "Gains " TTE_RED_TAG "+2 Mult" TTE_BLACK_TAG
        " per " TTE_YELLOW_TAG "reroll" TTE_BLACK_TAG
        " in the shop (currently " TTE_RED_TAG "+%ld Mult" TTE_BLACK_TAG ")",
        (long)joker->scoring_state
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 flash_card_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    (void)scored_card;

    if (joker_event == JOKER_EVENT_ON_SHOP_REROLL)
    {
        // Real card: accumulate +2, show a FIXED white "Upgrade!" message
        // (NOT the running total - misleading when spamming rerolls; the
        // running total lives in the description screen).
        // Copies (Blueprint/Brainstorm): stay SILENT - they do not
        // accumulate (mirror-only), so no upgrade message either. This is
        // the general Blueprint rule: copies mirror at settlement, they
        // never participate in accumulation.
        if (s_is_copying_joker)
        {
            return JOKER_EFFECT_FLAG_NONE;
        }
        joker->scoring_state += 2;
        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->message = "Upgrade!";
        return JOKER_EFFECT_FLAG_MESSAGE;
    }
    else if (joker_event == JOKER_EVENT_INDEPENDENT)
    {
        // During hand scoring, report the accumulated mult (copy mode reads
        // the original's accumulated value - no accumulation on the copy).
        // NOTE: the copy's own scoring_state is always 0 (it never
        // accumulates), so the >0 check must look at the SOURCE when copying
        // - otherwise Blueprint/Brainstorm would silently report nothing.
        u32 accumulated =
            s_is_copying_joker ? s_copied_joker_source->scoring_state
                               : joker->scoring_state;
        if (accumulated > 0)
        {
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->mult = accumulated;
            return JOKER_EFFECT_FLAG_MULT;
        }
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --------------------------------------------------------------------------
// Burglar (67)
// When a Blind is selected: gain +3 Hands and lose all discards.
// (Official EN: "When Blind is selected, gain +3 Hands and lose all
// discards" - fandom Nr 47, $6 Uncommon)
// This is an EVENT-TRIGGERED ACTION (explicit trigger point on
// ON_BLIND_SELECTED), so Blueprint/Brainstorm copies DO re-execute it
// (each copy grants another +3 hands). Distinct from passive/quiet cards
// (Credit Card) whose copies produce nothing.
// --------------------------------------------------------------------------

static int burglar_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    static const char desc[] =
        TTE_BLUE_TAG "Gain +3 Hands" TTE_BLACK_TAG " and\n"
        TTE_RED_TAG "lose all discards" TTE_BLACK_TAG;
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 burglar_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    (void)joker;
    (void)scored_card;
    (void)joker_effect;

    if (joker_event == JOKER_EVENT_ON_BLIND_SELECTED)
    {
        // Explicit trigger action: gain +3 hands, lose all discards.
        // Copies (Blueprint/Brainstorm) arrive here through the same event
        // dispatch and each queues its OWN instance, so real card + copies
        // fire sequentially (one beat apart) instead of all at once.
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
            s_deferred_count < MAX_JOKERS_HELD_SIZE)
        {
            if (s_deferred_count == 0)
            {
                s_deferred_ran_animation = false;
                s_deferred_settle_wait = false;
                s_deferred_next_beat_at = 0;
                s_deferred_wait_hud_roll = false;
            }
            s_deferred_queue[s_deferred_count] = self_object;
            s_deferred_kind[s_deferred_count] = DEFER_BURGLAR;
            s_deferred_count++;
        }
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --------------------------------------------------------------------------
// Showman (马戏团长) - ID 69
// Passive: Joker, Tarot, Planet and Spectral cards may appear multiple times.
// In this demake it affects Jokers only: the shop and booster packs (Riff-Raff)
// stop deduplicating owned Jokers, so already-owned ones can appear again.
// --------------------------------------------------------------------------
static int showman_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "Joker, " TTE_YELLOW_TAG "Tarot" TTE_BLACK_TAG ", "
        TTE_YELLOW_TAG "Planet" TTE_BLACK_TAG " and " TTE_YELLOW_TAG "Spectral"
        TTE_BLACK_TAG " cards may appear " TTE_YELLOW_TAG "multiple times"
        TTE_BLACK_TAG " (shop & packs)"
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 showman_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    // Passive: the effect is implemented in joker_reset_rollable_jokers()
    // (shop pool) and Riff-Raff spawn logic via is_showman_joker_active().
    // No event-triggered behaviour - like Smeared Joker, this is a
    // state-polled passive card.
    (void)joker;
    (void)scored_card;
    (void)joker_event;
    (void)joker_effect;

    return JOKER_EFFECT_FLAG_NONE;
}

// --------------------------------------------------------------------------
// Card Sharp (老千小丑) - ID 70
// Independent: X3 Mult if the played poker hand has already been played
// this round (i.e. this hand type occurred 2+ times this round).
// --------------------------------------------------------------------------
static int card_sharp_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "X" TTE_RED_TAG "3 Mult" TTE_BLACK_TAG
        " if played " TTE_YELLOW_TAG "poker hand" TTE_BLACK_TAG
        " has already been played " TTE_YELLOW_TAG "this round"
        TTE_BLACK_TAG
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 card_sharp_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    (void)joker;
    (void)scored_card;

    if (joker_event == JOKER_EVENT_INDEPENDENT)
    {
        // nb_played_hands[hand_type-1] is incremented on play (before
        // scoring), so at Independent time it already includes the current
        // hand. Count > 1 means this hand type was played before this round.
        u32 hand_type = get_hand_type();
        if (hand_type > 0 && hand_type <= HAND_TYPE_MAX &&
            g_game_vars.nb_played_hands[hand_type - 1] > 1)
        {
            *joker_effect = &s_shared_joker_effect;
            joker_effect_set_xmult(&s_shared_joker_effect, 3);
            return JOKER_EFFECT_FLAG_XMULT;
        }
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --------------------------------------------------------------------------
// To the Moon (冲向月球) - ID 71
// Passive: end-of-round interest is doubled while held. Silent-state
// joker (no trigger action) - implemented like Credit Card / Showman:
// no-op effect func + exported query consulted at the interest calc site
// (round_end.c calculate_interest_reward). Blueprint/Brainstorm copies
// do NOT count (copy invokes a no-op, contributes nothing).
// --------------------------------------------------------------------------
static int to_the_moon_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "Earn an extra " TTE_YELLOW_TAG "$1 of interest"
        TTE_BLACK_TAG " for every " TTE_YELLOW_TAG "$5" TTE_BLACK_TAG
        " you have at end of round"
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 to_the_moon_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    // Passive: the effect is implemented in round_end.c's
    // calculate_interest_reward() via is_to_the_moon_active().
    (void)joker;
    (void)scored_card;
    (void)joker_event;
    (void)joker_effect;

    return JOKER_EFFECT_FLAG_NONE;
}

bool is_to_the_moon_active(void)
{
    List* jokers = get_jokers_list();
    ListItr itr = list_itr_create(jokers);
    JokerObject* joker_object;
    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL && joker_object->joker != NULL &&
            joker_object->joker->id == TO_THE_MOON_ID)
        {
            return true;
        }
    }
    return false;
}

// --- Splash (ID 72) ---

// Description: every played card counts in scoring, including cards that
// would normally not score (Ace-less low ranks, face cards when no scoring
// hand type applies, etc.). Passive: implemented in card_object_is_scoring()
// via is_joker_owned(SPLASH_JOKER_ID), so round.c scoring loop, hand-type
// detection and Ride the Bus streak checks all stay in sync automatically.
static int splash_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "Every played card counts " TTE_YELLOW_TAG "as scoring"
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 splash_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    // Passive: the effect is implemented in card_object_is_scoring() via
    // is_joker_owned(SPLASH_JOKER_ID). No event-triggered behaviour.
    (void)joker;
    (void)scored_card;
    (void)joker_event;
    (void)joker_effect;

    return JOKER_EFFECT_FLAG_NONE;
}

// --- Supernova (ID 73) ---
// (Official EN: "Adds the number of times poker hand has been played this
// run to Mult" - fandom Nr 43, $5 Common)
// The counter is run-scoped and stored in g_game_vars.run_played_hands
// (incremented in round.c on every hand play, NEVER reset mid-run).
// Blueprint/Brainstorm copies read the same global array, so they mirror
// the exact same value as the original - identical to the original game.
static int supernova_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    static const char desc[] =
        TTE_BLACK_TAG "Adds the number of times this\npoker hand has been played this\nrun to " TTE_RED_TAG "Mult" TTE_BLACK_TAG;
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 supernova_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    (void)joker;
    (void)scored_card;

    if (joker_event == JOKER_EVENT_INDEPENDENT)
    {
        // Copies mirror the same run counter (no special-casing needed):
        // both report the same +Mult, exactly like the original game where
        // Blueprint duplicates the bonus.
        u32 times_played = g_game_vars.run_played_hands[get_hand_type() - 1];
        if (times_played > 0)
        {
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->mult = times_played;
            return JOKER_EFFECT_FLAG_MULT;
        }
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --- Green Joker (ID 74) ---
// (Official EN: "+1 Mult per hand played -1 Mult per discard" - fandom
// Nr 58, $4 Common. Mult FLOORS at 0: discarding at 0 Mult does nothing.
// Floor-0 is VANILLA behaviour, verified via bwiki 2026-08-20 - NOT a
// local u32-architecture compromise. Do not "fix" this to allow negatives.)
// Real card accumulates in scoring_state; copies stay silent during
// accumulation and mirror the value at INDEPENDENT (Flash Card pattern).
static int green_joker_desc(Joker* joker, Rect dest_rect)
{
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "+1 " TTE_RED_TAG "Mult" TTE_BLACK_TAG " per hand played\n"
        "-1 " TTE_RED_TAG "Mult" TTE_BLACK_TAG " per discard\n"
        "(currently " TTE_RED_TAG "+%ld" TTE_BLACK_TAG ")",
        (long)joker->scoring_state
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 green_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    (void)scored_card;

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            joker->scoring_state = 0;
            break;

        case JOKER_EVENT_ON_HAND_PLAYED:
            // Copies stay silent: accumulation belongs to the real joker,
            // the copy mirrors the value at INDEPENDENT.
            if (!s_is_copying_joker)
            {
                joker->scoring_state += 1;
                growth_msg_enqueue(joker, "Upgrade!");
            }
            break;

        case JOKER_EVENT_ON_HAND_DISCARDED:
            if (!s_is_copying_joker)
            {
                if (joker->scoring_state > 0)
                {
                    joker->scoring_state -= 1;
                    growth_msg_enqueue(joker, "Downgrade!");
                }
            }
            break;

        case JOKER_EVENT_INDEPENDENT:
        {
            u32 accumulated =
                s_is_copying_joker ? s_copied_joker_source->scoring_state
                                   : joker->scoring_state;
            if (accumulated > 0)
            {
                *joker_effect = &s_shared_joker_effect;
                (*joker_effect)->mult = accumulated;
                return JOKER_EFFECT_FLAG_MULT;
            }
            break;
        }

        default:
            break;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --- Square Joker (ID 75) ---
// (Official EN: "This Joker gains +4 Chips if played hand has exactly 4
// cards" - fandom Nr 65, $4 Common.)
// Growth only applies when exactly 4 cards are played (get_played_size()==4).
static int square_joker_desc(Joker* joker, Rect dest_rect)
{
    char desc[200];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "This Joker gains " TTE_BLUE_TAG "+4 Chips" TTE_BLACK_TAG "\n"
        "if played hand has " TTE_YELLOW_TAG "exactly 4 cards" TTE_BLACK_TAG "\n"
        "(currently " TTE_BLUE_TAG "+%ld" TTE_BLACK_TAG ")",
        (long)joker->scoring_state
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 square_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    (void)scored_card;

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            joker->scoring_state = 0;
            break;

        case JOKER_EVENT_ON_HAND_PLAYED:
            // Copies stay silent: accumulation belongs to the real joker.
            if (!s_is_copying_joker)
            {
                if (get_played_size() == 4)
                {
                    joker->scoring_state += 4;
                    growth_msg_enqueue(joker, "Upgrade!");
                }
            }
            break;

        case JOKER_EVENT_INDEPENDENT:
        {
            u32 accumulated =
                s_is_copying_joker ? s_copied_joker_source->scoring_state
                                   : joker->scoring_state;
            if (accumulated > 0)
            {
                *joker_effect = &s_shared_joker_effect;
                (*joker_effect)->chips = accumulated;
                return JOKER_EFFECT_FLAG_CHIPS;
            }
            break;
        }

        default:
            break;
    }

    return JOKER_EFFECT_FLAG_NONE;
}

// --- Baseball Card (ID 76) ---
// (Official EN: "Uncommon Jokers each give X1.5 Mult" - fandom Nr 92,
// $8 Rare. Act = "On Other Jokers": every OTHER joker scored at
// INDEPENDENT that is Uncommon rarity triggers X1.5 per Baseball Card.)
// Implemented as a passive (no-op effect): the multiplier is applied by
// joker.c's joker_object_score INDEPENDENT hook, which queries
// count_baseball_card_effects() for every Uncommon joker scored. The
// hook runs BEFORE the NONE-return check, so silent/static Uncommon
// jokers (e.g. Smeared) grant the bonus too - matching original.
static int baseball_card_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    static const char desc[] =
        TTE_BLACK_TAG "Each " TTE_YELLOW_TAG "Uncommon Joker" TTE_BLACK_TAG
        " gives " TTE_RED_TAG "X1.5" TTE_BLACK_TAG " Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 baseball_card_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    // Passive: the effect is implemented in joker.c's INDEPENDENT hook.
    (void)joker;
    (void)scored_card;
    (void)joker_event;
    (void)joker_effect;

    return JOKER_EFFECT_FLAG_NONE;
}

// Number of active Baseball Card effects: real cards + Blueprint/Brainstorm
// copies whose chain-resolved target is a Baseball Card (Blueprint copies
// its right neighbor, Brainstorm the leftmost - resolve_copy_target handles
// the chain, e.g. Brainstorm -> Blueprint -> Baseball Card). Each effect
// contributes one X1.5 per Uncommon joker scored.
int count_baseball_card_effects(void)
{
    int count = 0;
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (cur == NULL || cur->joker == NULL)
            continue;
        if (cur->joker->id == BASEBALL_CARD_ID)
        {
            count++;
            continue;
        }
        JokerObject* target = resolve_copy_target(cur);
        if (target != NULL && target->joker != NULL &&
            target->joker->id == BASEBALL_CARD_ID)
        {
            count++;
        }
    }
    return count;
}

// --- Stuntman (ID 77) ---
// (Official EN: "+250 Chips, -2 hand size" - fandom Nr 136, $7 Rare.
// Act = Indep: the +250 Chips is a settlement-type value reported at
// INDEPENDENT - Blueprint/Brainstorm copies re-report it (+250 per copy).
// The -2 hand size is a PASSIVE stat applied the moment the card is
// held; it is NOT a triggered action, so copies do NOT apply it -
// count_stuntman_effects() counts real cards only, and round.c derives
// the effective hand size on use.)
static int stuntman_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    static const char desc[] =
        TTE_BLACK_TAG "+" TTE_BLUE_TAG "250" TTE_BLACK_TAG " " TTE_BLUE_TAG "Chips" TTE_BLACK_TAG
        ", " TTE_YELLOW_TAG "-2 hand size" TTE_BLACK_TAG;
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 stuntman_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_INDEPENDENT)
    {
        // Settlement-type value: no copy guard - every Blueprint/Brainstorm
        // copy reports +250 Chips too, matching original.
        *joker_effect = &s_shared_joker_effect;
        (*joker_effect)->chips = 250;
        return JOKER_EFFECT_FLAG_CHIPS;
    }
    (void)joker;
    (void)scored_card;
    return JOKER_EFFECT_FLAG_NONE;
}

// Number of REAL Stuntmen held (Showman duplicates stack: -2 hand size
// each). Blueprint/Brainstorm copies resolve to this check but the -2 is
// a passive stat - copies are NOT counted (silent-state rule).
int count_stuntman_effects(void)
{
    int count = 0;
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (cur != NULL && cur->joker != NULL &&
            cur->joker->id == STUNTMAN_JOKER_ID)
        {
            count++;
        }
    }
    return count;
}

// --- Ancient Joker (ID 78) ---
// (Official EN: "Each played card with [suit] gives X1.5 Mult when
// scored, suit changes at end of round" - fandom Nr 99, $8 Rare.
// Act = On Scored: per-card settlement, Baron-style.)
// USER-MANDATED SPECIAL RULES (2026-08-23):
//  - the suit NEVER repeats across consecutive rounds (each round's suit
//    differs from the previous round's);
//  - the suit is drawn from all 4 suits regardless of deck composition
//    (even a 1-suit deck still rolls any of the 4).
// State: persistent_state = current suit (0-3, card.h NUM_SUITS); the
// copy mechanism syncs it to Blueprint/Brainstorm copies, so copies
// trigger on the same suit automatically (per-card X1.5 each, no guard).

// 4 pre-built static descs (one per suit): avoids snprintf/%s and the
// double-`#{...}#{...}` tag run that the sprintf-joined variant produced
// (描述界面卡死 2026-08-23 - M32 follow-up). Each desc is a single
// well-formed TTE string with ONE suit tag - same shape as every other
// joker desc in the codebase.
static const char* const s_ancient_descs[NUM_SUITS] =
{
    TTE_BLACK_TAG "Each played " TTE_DIAMOND_TAG "card gives " TTE_RED_TAG "X1.5" TTE_BLACK_TAG " Mult when scored",
    TTE_BLACK_TAG "Each played " TTE_CLUB_TAG "card gives " TTE_RED_TAG "X1.5" TTE_BLACK_TAG " Mult when scored",
    TTE_BLACK_TAG "Each played " TTE_HEART_TAG "card gives " TTE_RED_TAG "X1.5" TTE_BLACK_TAG " Mult when scored",
    TTE_BLACK_TAG "Each played " TTE_SPADE_TAG "card gives " TTE_RED_TAG "X1.5" TTE_BLACK_TAG " Mult when scored",
};

static int ancient_joker_desc(Joker* joker, Rect dest_rect)
{
    u8 suit = (joker != NULL) ? (u8)joker->persistent_state : 0;
    if (suit >= NUM_SUITS)
        suit = 0;
    return tte_printf_justified_in_rect(
        s_ancient_descs[suit], dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true
    );
}

static u32 ancient_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED)
    {
        // First-round suit: fully random (no previous-round exclusion).
        if (!s_is_copying_joker)
            joker->persistent_state = rng_get_u32(RNG_SEQ_JOKER_ANCIENT) % NUM_SUITS;
    }
    else if (joker_event == JOKER_EVENT_ON_CARD_SCORED && scored_card != NULL)
    {
        // Per-card settlement: each scored card matching the current suit
        // gives X1.5 (Baron-style - N matching cards = X1.5^N). Smeared
        // Joker merges red/black suits via card_effective_suit_mask()
        // (原版: Hearts counts Diamonds too when Smeared is active - the
        // suit check goes through the merged suit semantics, M32 follow-up).
        // Copies mirror persistent_state (synced) so they trigger on the
        // same suit.
        u8 current_suit = (u8)joker->persistent_state;
        if (current_suit >= NUM_SUITS)
            current_suit = 0;
        if ((card_effective_suit_mask(scored_card->suit) & (1 << current_suit)) != 0)
        {
            *joker_effect = &s_shared_joker_effect;
            joker_effect_set_xmult_den(&s_shared_joker_effect, 3, 2);
            return JOKER_EFFECT_FLAG_XMULT;
        }
    }
    else if (joker_event == JOKER_EVENT_ON_BLIND_SELECTED)
    {
        // In-round there is no desc view, so announce the current suit
        // at each round start (花色在 ON_ROUND_END 已滚动好，这里报的是
        // 新回合的花色). Enqueue into the unified deferred queue (M24
        // blind-selected serialization - plays left-to-right, one beat
        // each, gated with HUD rolls) instead of a synchronous MESSAGE
        // that would overlap other blind-selected effects. Real joker
        // only - copies stay silent (they mirror the same suit anyway).
        if (!s_is_copying_joker)
        {
            // Announce the current suit (white message, serialized via
            // the unified deferred queue - M34 follow-up).
            u8 suit = (u8)joker->persistent_state;
            if (suit >= NUM_SUITS)
                suit = 0;
            deferred_enqueue_message(joker, s_ancient_suit_names[suit]);
        }
    }
    else if (joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        // Suit changes at end of round. NEVER the same as the current
        // round's suit: roll among the other 3 (map 0-2 skipping current),
        // independent of deck composition. Only the real joker rolls -
        // copies keep mirroring persistent_state.
        if (!s_is_copying_joker)
        {
            u8 current = (u8)joker->persistent_state;
            if (current >= NUM_SUITS)
                current = 0;
            u32 r = rng_get_u32(RNG_SEQ_JOKER_ANCIENT) % (NUM_SUITS - 1);
            if (r >= current)
                r++;
            joker->persistent_state = r;
        }
    }
    (void)joker;
    (void)joker_effect;
    return JOKER_EFFECT_FLAG_NONE;
}

// --- Swashbuckler (ID 79) ---
// (Official EN: "Adds the sell value of all other owned Jokers to Mult
// (Currently +1 Mult)" - fandom Nr 110, $4 Common. Act = Indep:
// settlement-type +Mult reported at INDEPENDENT. Blueprint/Brainstorm
// copies re-report it (the copy counts the real card as "other").
// joker_get_sell_value() reads live value/2, so a growing Egg etc. is
// absorbed correctly.)
static int swashbuckler_sell_total(const Joker* self)
{
    int total = 0;
    ListItr itr = list_itr_create(get_jokers_list());
    JokerObject* cur;
    while ((cur = list_itr_next(&itr)))
    {
        if (cur == NULL || cur->joker == NULL)
            continue;
        if (self != NULL && cur->joker == self)
            continue; // own sell value does NOT count (原版 "other owned")
        total += joker_get_sell_value(cur->joker);
    }
    return total;
}

static int swashbuckler_joker_desc(Joker* joker, Rect dest_rect)
{
    char desc[200];
    snprintf(
        desc, sizeof(desc),
        TTE_BLACK_TAG "Adds the sell value of all other owned "
        TTE_YELLOW_TAG "Jokers" TTE_BLACK_TAG " to " TTE_RED_TAG "Mult" TTE_BLACK_TAG
        " (currently " TTE_RED_TAG "+%ld Mult" TTE_BLACK_TAG ")",
        (long)swashbuckler_sell_total(joker)
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 swashbuckler_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_INDEPENDENT)
    {
        int total = swashbuckler_sell_total(joker);
        if (total > 0)
        {
            *joker_effect = &s_shared_joker_effect;
            (*joker_effect)->mult = (u32)total;
            return JOKER_EFFECT_FLAG_MULT;
        }
    }
    (void)scored_card;
    return JOKER_EFFECT_FLAG_NONE;
}

// --- Gift Card (ID 80) ---
// (Official EN: "Add $1 of sell value to every Joker and Consumable card
// at end of round" - fandom Nr 79, $6 Uncommon. Act = N/A (round-end
// event). EVERY owned joker gains sell value - INCLUDING this card
// itself and Blueprint/Brainstorm copies firing their own round-end
// instance (+$1 each, doubled with a copy - matching original).
// sell_value = value/2, so +2 to value = +$1 sell value.)
static int gift_card_joker_desc(Joker* joker, Rect dest_rect)
{
    (void)joker;
    static const char desc[] =
        TTE_BLACK_TAG "Add " TTE_YELLOW_TAG "$1" TTE_BLACK_TAG " of sell value to every "
        TTE_YELLOW_TAG "Joker" TTE_BLACK_TAG " and " TTE_YELLOW_TAG "Consumable" TTE_BLACK_TAG
        " card at end of round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static u32 gift_card_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        // +$1 sell value to EVERY owned joker (incl. self). Consumables
        // don't exist in this port yet - jokers only. The value changes
        // are immediate; the "+$1" pop is serialized via the deferred
        // queue (no longer overlapping Egg's "+$3" - M34 follow-up).
        ListItr itr = list_itr_create(get_jokers_list());
        JokerObject* cur;
        while ((cur = list_itr_next(&itr)))
        {
            if (cur != NULL && cur->joker != NULL)
                cur->joker->value += 2;
        }

        deferred_enqueue_message(joker, "+$1");
        return JOKER_EFFECT_FLAG_NONE;
    }
    (void)joker;
    (void)scored_card;
    return JOKER_EFFECT_FLAG_NONE;
}
