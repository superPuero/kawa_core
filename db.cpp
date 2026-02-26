#include "kawa/core/core.h"

using namespace kawa;

struct user 
{
    string username;
    string email;
};

struct genre
{
    string name;
    dyn_array<entity_id> children;
};

struct game_genres
{
    dyn_array<entity_id> values;
};

struct developer 
{
    string name;
};

struct publisher
{
    string name;
};

struct platform 
{
    string name;
};


struct game_platforms
{
    dyn_array<entity_id> values;
};

struct game_details 
{
};

struct game_entry
{
    string title;
    string release_date;

    struct {
        string description;
        string min_requirements;
        string max_requirements;
    } details;

    dyn_array<entity_id> platforms;
    dyn_array<entity_id> genres;

    entity_id developer_id;
    entity_id publisher_id;
    entity_id detail_id;
    dyn_array<entity_id> reviews;
};

struct review
{
    entity_id user_id;
    f32 rating;
    string comment;
    string review_date;
};

struct game_reviews
{
    dyn_array<entity_id> values;
};

void populate_db(registry& db)
{
    auto dev_nintendo = db.entity(developer{ "Nintendo" });
    auto dev_valve = db.entity(developer{ "Valve" });
    auto dev_rockstar = db.entity(developer{ "Rockstar Games" });
    auto dev_cdpr = db.entity(developer{ "CD Projekt Red" });
    auto dev_bethesda = db.entity(developer{ "Bethesda Game Studios" });
    auto dev_fromsoft = db.entity(developer{ "FromSoftware" });
    auto dev_ubisoft = db.entity(developer{ "Ubisoft Montreal" });
    auto dev_naughty = db.entity(developer{ "Naughty Dog" });
    auto dev_santa = db.entity(developer{ "Santa Monica Studio" });
    auto dev_capcom = db.entity(developer{ "Capcom" });
    auto dev_square = db.entity(developer{ "Square Enix" });
    auto dev_blizzard = db.entity(developer{ "Blizzard Entertainment" });
    auto dev_ea = db.entity(developer{ "Electronic Arts" });
    auto dev_bioware = db.entity(developer{ "BioWare" });
    auto dev_obsidian = db.entity(developer{ "Obsidian Entertainment" });
    auto dev_insomniac = db.entity(developer{ "Insomniac Games" });
    auto dev_id = db.entity(developer{ "Id Software" });
    auto dev_bungie = db.entity(developer{ "Bungie" });
    auto dev_respawn = db.entity(developer{ "Respawn Entertainment" });
    auto dev_konami = db.entity(developer{ "Konami" });
    auto dev_larian = db.entity(developer{ "Larian Studios" });
    auto dev_supergiant = db.entity(developer{ "Supergiant Games" });
    auto dev_cherry = db.entity(developer{ "Team Cherry" });
    auto dev_concernedape = db.entity(developer{ "ConcernedApe" });
    auto dev_kojima = db.entity(developer{ "Kojima Productions" });

    auto pub_larian = db.entity(publisher{ "Larian Studios" });
    auto pub_supergiant = db.entity(publisher{ "Supergiant Games" });
    auto pub_cherry = db.entity(publisher{ "Team Cherry" });
    auto pub_concernedape = db.entity(publisher{ "ConcernedApe" });
    auto pub_505 = db.entity(publisher{ "505 Games" });
    auto pub_nintendo = db.entity(publisher{ "Nintendo" });
    auto pub_valve = db.entity(publisher{ "Valve" });
    auto pub_taketwo = db.entity(publisher{ "Take-Two Interactive" });
    auto pub_cdpr = db.entity(publisher{ "CD Projekt" });
    auto pub_bethesda = db.entity(publisher{ "Bethesda Softworks" });
    auto pub_bandai = db.entity(publisher{ "Bandai Namco" });
    auto pub_ubisoft = db.entity(publisher{ "Ubisoft" });
    auto pub_sony = db.entity(publisher{ "Sony Interactive Entertainment" });
    auto pub_capcom = db.entity(publisher{ "Capcom" });
    auto pub_square = db.entity(publisher{ "Square Enix" });
    auto pub_activision = db.entity(publisher{ "Activision Blizzard" });
    auto pub_ea = db.entity(publisher{ "Electronic Arts" });
    auto pub_microsoft = db.entity(publisher{ "Microsoft Game Studios" });
    auto pub_sega = db.entity(publisher{ "Sega" });
    auto pub_warner = db.entity(publisher{ "Warner Bros. Games" });
    auto pub_2k = db.entity(publisher{ "2K Games" });
    auto pub_deepsilver = db.entity(publisher{ "Deep Silver" });
    auto pub_devolver = db.entity(publisher{ "Devolver Digital" });
    auto pub_paradox = db.entity(publisher{ "Paradox Interactive" });
    auto pub_thq = db.entity(publisher{ "THQ Nordic" });

    // --- Platforms ---
    auto plat_pc = db.entity(platform{ "PC" });
    auto plat_ps5 = db.entity(platform{ "PlayStation 5" });
    auto plat_ps4 = db.entity(platform{ "PlayStation 4" });
    auto plat_xsx = db.entity(platform{ "Xbox Series X/S" });
    auto plat_xone = db.entity(platform{ "Xbox One" });
    auto plat_switch = db.entity(platform{ "Nintendo Switch" });
    auto plat_3ds = db.entity(platform{ "Nintendo 3DS" });
    auto plat_ios = db.entity(platform{ "iOS" });
    auto plat_android = db.entity(platform{ "Android" });
    auto plat_macos = db.entity(platform{ "macOS" });

    // --- Genres ---
    auto gen_action = db.entity(genre{ "Action" });
    auto gen_rpg = db.entity(genre{ "RPG" });
    auto gen_strategy = db.entity(genre{ "Strategy" });
    auto gen_adventure = db.entity(genre{ "Adventure" });
    auto gen_simulation = db.entity(genre{ "Simulation" });
    auto gen_sports = db.entity(genre{ "Sports" });
    auto gen_puzzle = db.entity(genre{ "Puzzle" });
    auto gen_shooter = db.entity(genre{ "Shooter" });
    auto gen_platformer = db.entity(genre{ "Platformer" });
    auto gen_fighting = db.entity(genre{ "Fighting" });
    auto gen_action_rpg = db.entity(genre{ "Action RPG" });
    auto gen_jrpg = db.entity(genre{ "JRPG" });
    auto gen_mmorpg = db.entity(genre{ "MMORPG" });
    auto gen_tbs = db.entity(genre{ "Turn-Based Strategy" });
    auto gen_rts = db.entity(genre{ "RTS" });
    auto gen_survival = db.entity(genre{ "Survival Horror" });
    auto gen_vn = db.entity(genre{ "Visual Novel" });
    auto gen_racing = db.entity(genre{ "Racing" });
    auto gen_br = db.entity(genre{ "Battle Royale" });
    auto gen_metroidvania = db.entity(genre{ "Metroidvania" });

    // --- user ---
    auto usr_gamer1 = db.entity(user{ "GamerOne", "gamer1@example.com" });
    auto usr_pro99 = db.entity(user{ "ProPlayer99", "pro99@test.com" });
    auto usr_noob69 = db.entity(user{ "NoobMaster69", "thor@asgard.com" });
    auto usr_zeldafan = db.entity(user{ "ZeldaFan", "link@hyrule.com" });
    auto usr_fpsking = db.entity(user{ "FPS_King", "aimbot@sus.com" });
    auto usr_stratbuff = db.entity(user{ "StrategyBuff", "civ@history.com" });
    auto usr_indie = db.entity(user{ "IndieLover", "hollow@nest.com" });
    auto usr_retro = db.entity(user{ "RetroGamer", "mario@nes.com" });
    auto usr_speed = db.entity(user{ "SpeedRunner", "sonic@fast.com" });
    auto usr_coop = db.entity(user{ "CoOpBuddy", "p2@portal.com" });
    auto usr_lore = db.entity(user{ "LoreMaster", "vaati@souls.com" });
    auto usr_casual = db.entity(user{ "CasualJoe", "joe@mobile.com" });
    auto usr_streamer = db.entity(user{ "StreamerX", "ttv@live.com" });
    auto usr_reviewer = db.entity(user{ "ReviewerGuy", "ign@rating.com" });
    auto usr_trophy = db.entity(user{ "TrophyHunter", "plat@psn.com" });
    auto usr_pcmr = db.entity(user{ "PCMasterRace", "rgb@steam.com" });
    auto usr_console = db.entity(user{ "ConsolePeasant", "couch@tv.com" });
    auto usr_mobile = db.entity(user{ "MobileWhale", "gacha@money.com" });
    auto usr_glitch = db.entity(user{ "GlitchHunter", "bug@fix.com" });
    auto usr_modder = db.entity(user{ "Modder101", "nexus@mods.com" });

    // --- Updated Previous Entries (with Genres) ---

    db.entity(
        game_entry{
            .title = "Elden Ring",
            .release_date = "2022-02-25",
            .details = {
                .description = "A vast dark fantasy action-RPG created by Hidetaka Miyazaki and George R. R. Martin.",
                .min_requirements = "GTX 1060 3GB",
                .max_requirements = "RTX 3060 Ti"
            },
            .platforms = { { plat_pc, plat_ps4, plat_ps5, plat_xone, plat_xsx } },
            .genres = { { gen_action_rpg, gen_rpg, gen_adventure } },
            .developer_id = dev_fromsoft,
            .publisher_id = pub_bandai,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_lore, .rating = 10.0f, .comment = "The lore runs deeper than the Erdtree roots. Masterpiece.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_noob69, .rating = 3.0f, .comment = "Tree sentinel kept killing me. Too hard.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_pro99, .rating = 9.5f, .comment = "Incredible build variety and challenging bosses.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Baldur's Gate 3",
            .release_date = "2023-08-03",
            .details = {
                .description = "A story-rich, party-based RPG set in the universe of Dungeons & Dragons.",
                .min_requirements = "GTX 970",
                .max_requirements = "RTX 2060 Super"
            },
            .platforms = { { plat_pc, plat_ps5, plat_xsx, plat_macos } },
            .genres = { { gen_rpg, gen_tbs, gen_adventure } },
            .developer_id = dev_larian,
            .publisher_id = pub_larian,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_stratbuff, .rating = 10.0f, .comment = "Turn-based perfection. The reactivity of the world is unmatched.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_coop, .rating = 9.8f, .comment = "Best co-op campaign I've ever played with my friends.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Red Dead Redemption 2",
            .release_date = "2018-10-26",
            .details = {
                .description = "An epic tale of life in America's unforgiving heartland.",
                .min_requirements = "GTX 770",
                .max_requirements = "RTX 2060"
            },
            .platforms = { { plat_pc, plat_ps4, plat_xone } },
            .genres = { { gen_action, gen_adventure } },
            .developer_id = dev_rockstar,
            .publisher_id = pub_taketwo,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_casual, .rating = 9.0f, .comment = "A bit slow at times, but an absolutely beautiful story.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_pcmr, .rating = 9.5f, .comment = "Looks unbelievable on max settings. Outstanding physics.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "The Legend of Zelda: Breath of the Wild",
            .release_date = "2017-03-03",
            .details = {
                .description = "Step into a world of discovery, exploration, and adventure.",
                .min_requirements = "N/A",
                .max_requirements = "N/A"
            },
            .platforms = { { plat_switch } },
            .genres = { { gen_action, gen_adventure, gen_rpg } },
            .developer_id = dev_nintendo,
            .publisher_id = pub_nintendo,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_zeldafan, .rating = 10.0f, .comment = "Redefined open-world games forever. Pure magic.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_speed, .rating = 9.0f, .comment = "The physics engine makes for some insane sequence breaking. Love it.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Cyberpunk 2077",
            .release_date = "2020-12-10",
            .details = {
                .description = "An open-world, action-adventure RPG set in the dark future of Night City.",
                .min_requirements = "GTX 1060",
                .max_requirements = "RTX 3080"
            },
            .platforms = { { plat_pc, plat_ps4, plat_ps5, plat_xone, plat_xsx } },
            .genres = { { gen_action_rpg, gen_rpg, gen_shooter } },
            .developer_id = dev_cdpr,
            .publisher_id = pub_cdpr,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_glitch, .rating = 7.0f, .comment = "Was a goldmine for bugs at launch, but it's much better now.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_pcmr, .rating = 9.0f, .comment = "Path tracing makes this the best looking game on PC right now.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "The Elder Scrolls V: Skyrim",
            .release_date = "2011-11-11",
            .details = {
                .description = "Epic fantasy RPG that lets you be anyone and do anything.",
                .min_requirements = "GTX 260",
                .max_requirements = "GTX 780"
            },
            .platforms = { { plat_pc, plat_ps4, plat_ps5, plat_xone, plat_xsx, plat_switch } },
            .genres = { { gen_action_rpg, gen_rpg, gen_adventure } },
            .developer_id = dev_bethesda,
            .publisher_id = pub_bethesda,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_modder, .rating = 10.0f, .comment = "I have 1,500 mods installed. It occasionally boots.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_gamer1, .rating = 9.5f, .comment = "A timeless classic that I end up replaying every year.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "God of War",
            .release_date = "2018-04-20",
            .details = {
                .description = "Kratos lives as a man in the lands of Norse Gods and monsters.",
                .min_requirements = "GTX 960",
                .max_requirements = "RTX 2060"
            },
            .platforms = { { plat_pc, plat_ps4 } },
            .genres = { { gen_action, gen_adventure } },
            .developer_id = dev_santa,
            .publisher_id = pub_sony,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_trophy, .rating = 9.8f, .comment = "Incredible combat. The Valkyries gave me a real challenge for the Platinum.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_console, .rating = 10.0f, .comment = "Boy.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Portal 2",
            .release_date = "2011-04-18",
            .details = {
                .description = "A hilariously mind-bending puzzle game with physics-based mechanics.",
                .min_requirements = "8600m GT",
                .max_requirements = "GTX 460"
            },
            .platforms = { { plat_pc, plat_macos } },
            .genres = { { gen_puzzle, gen_platformer, gen_adventure } },
            .developer_id = dev_valve,
            .publisher_id = pub_valve,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_coop, .rating = 10.0f, .comment = "Testing friendship limits in the co-op campaign.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_reviewer, .rating = 10.0f, .comment = "Flawless pacing, brilliant writing, perfect game.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Hollow Knight",
            .release_date = "2017-02-24",
            .details = {
                .description = "Forge your own path in this award-winning action-adventure through a ruined kingdom of insects.",
                .min_requirements = "GeForce 9800GTX",
                .max_requirements = "GTX 560"
            },
            .platforms = { { plat_pc, plat_ps4, plat_xone, plat_switch, plat_macos } },
            .genres = { { gen_metroidvania, gen_action, gen_platformer, gen_adventure } },
            .developer_id = dev_cherry,
            .publisher_id = pub_cherry,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_indie, .rating = 10.0f, .comment = "The atmosphere, the music, the tight controls... an absolute masterpiece of the genre.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_lore, .rating = 9.0f, .comment = "Piecing together the fall of Hallownest was a joy.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Apex Legends",
            .release_date = "2019-02-04",
            .details = {
                .description = "A free-to-play hero shooter where legendary characters battle for glory.",
                .min_requirements = "GT 640",
                .max_requirements = "GTX 970"
            },
            .platforms = { { plat_pc, plat_ps4, plat_ps5, plat_xone, plat_xsx, plat_switch } },
            .genres = { { gen_br, gen_shooter, gen_action } },
            .developer_id = dev_respawn,
            .publisher_id = pub_ea,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_fpsking, .rating = 8.5f, .comment = "Best movement in any BR by far. Matchmaking can be rough though.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_streamer, .rating = 9.0f, .comment = "Great for content, high skill ceiling.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    // --- Brand New Game Entries ---

    db.entity(
        game_entry{
            .title = "Hades",
            .release_date = "2020-09-17",
            .details = {
                .description = "Defy the god of the dead as you hack and slash out of the Underworld in this rogue-like dungeon crawler.",
                .min_requirements = "Dual Core 2.4 GHz",
                .max_requirements = "Quad Core 3.0 GHz"
            },
            .platforms = { { plat_pc, plat_switch, plat_ps4, plat_ps5, plat_xone, plat_xsx, plat_macos } },
            .genres = { { gen_action_rpg, gen_action, gen_rpg } },
            .developer_id = dev_supergiant,
            .publisher_id = pub_supergiant,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_indie, .rating = 9.8f, .comment = "Incredible art, music, and voice acting. The gameplay loop is addictive.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_speed, .rating = 9.5f, .comment = "Excellent for speedrunning. Weapons feel incredibly distinct.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Stardew Valley",
            .release_date = "2016-02-26",
            .details = {
                .description = "You've inherited your grandfather's old farm plot in Stardew Valley. Armed with hand-me-down tools and a few coins, you set out to begin your new life.",
                .min_requirements = "2 GHz",
                .max_requirements = "N/A"
            },
            .platforms = { { plat_pc, plat_macos, plat_switch, plat_ps4, plat_xone, plat_ios, plat_android } },
            .genres = { { gen_simulation, gen_rpg } },
            .developer_id = dev_concernedape,
            .publisher_id = pub_concernedape,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_casual, .rating = 10.0f, .comment = "The most relaxing game I've ever played. So much content for the price.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_mobile, .rating = 9.0f, .comment = "Plays great on mobile too. A perfect port.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "DOOM Eternal",
            .release_date = "2020-03-20",
            .details = {
                .description = "Hell’s armies have invaded Earth. Become the Slayer in an epic single-player campaign to conquer demons across dimensions.",
                .min_requirements = "GTX 1050 Ti",
                .max_requirements = "RTX 2060"
            },
            .platforms = { { plat_pc, plat_ps4, plat_ps5, plat_xone, plat_xsx, plat_switch } },
            .genres = { { gen_shooter, gen_action } },
            .developer_id = dev_id,
            .publisher_id = pub_bethesda,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_fpsking, .rating = 9.5f, .comment = "Rip and tear until it is done. The combat loop is absolute adrenaline.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_pro99, .rating = 9.0f, .comment = "Forces you to play aggressively. The resource management is brilliant.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "The Last of Us Part I",
            .release_date = "2013-06-14",
            .details = {
                .description = "In a ravaged civilization, Joel is hired to smuggle a 14-year-old girl, Ellie, out of a military quarantine zone.",
                .min_requirements = "N/A",
                .max_requirements = "N/A"
            },
            .platforms = { { plat_ps4, plat_ps5, plat_pc } },
            .genres = { { gen_action, gen_adventure, gen_survival } },
            .developer_id = dev_naughty,
            .publisher_id = pub_sony,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_reviewer, .rating = 10.0f, .comment = "One of the greatest video game narratives of all time.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_console, .rating = 9.5f, .comment = "The stealth and combat feel so brutally realistic.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Super Mario Odyssey",
            .release_date = "2017-10-27",
            .details = {
                .description = "Explore incredible places far from the Mushroom Kingdom as you join Mario and his new ally Cappy.",
                .min_requirements = "N/A",
                .max_requirements = "N/A"
            },
            .platforms = { { plat_switch } },
            .genres = { { gen_platformer, gen_adventure, gen_action } },
            .developer_id = dev_nintendo,
            .publisher_id = pub_nintendo,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_retro, .rating = 9.8f, .comment = "Captures the magic of Mario 64 while adding so many fresh mechanics.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_speed, .rating = 9.0f, .comment = "Movement tech is insane. Cappy jumps allow for crazy shortcuts.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Mass Effect Legendary Edition",
            .release_date = "2021-05-14",
            .details = {
                .description = "Relive the cinematic saga. Includes single-player base content and over 40 DLC from the highly acclaimed trilogy.",
                .min_requirements = "GTX 760",
                .max_requirements = "GTX 1070"
            },
            .platforms = { { plat_pc, plat_ps4, plat_xone } },
            .genres = { { gen_action_rpg, gen_rpg, gen_shooter, gen_adventure } },
            .developer_id = dev_bioware,
            .publisher_id = pub_ea,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_lore, .rating = 9.5f, .comment = "The world-building of the Mass Effect universe remains unparalleled.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_gamer1, .rating = 9.0f, .comment = "Commander Shepard is an icon. The remasters smoothed out the clunky ME1 combat.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Fallout: New Vegas",
            .release_date = "2010-10-19",
            .details = {
                .description = "Welcome to Vegas. New Vegas. It’s the kind of town where you dig your own grave prior to being shot in the head.",
                .min_requirements = "GeForce 6 series",
                .max_requirements = "GeForce 8 series"
            },
            .platforms = { { plat_pc, plat_xone } },
            .genres = { { gen_rpg, gen_action_rpg, gen_shooter } },
            .developer_id = dev_obsidian,
            .publisher_id = pub_bethesda,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_stratbuff, .rating = 9.5f, .comment = "The faction mechanics and player agency in this game put modern RPGs to shame.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_glitch, .rating = 8.0f, .comment = "A masterpiece held together by duct tape and fan patches.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Death Stranding",
            .release_date = "2019-11-08",
            .details = {
                .description = "Sam Bridges must brave a world utterly transformed by the Death Stranding to reconnect the shattered world.",
                .min_requirements = "GTX 1050 4 GB",
                .max_requirements = "GTX 1060 6 GB"
            },
            .platforms = { { plat_ps4, plat_ps5, plat_pc } },
            .genres = { { gen_adventure, gen_action } },
            .developer_id = dev_kojima,
            .publisher_id = pub_505,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_reviewer, .rating = 9.0f, .comment = "A deeply unique, polarizing experience. It's a gorgeous walking simulator with terrifying stealth elements.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_casual, .rating = 7.5f, .comment = "Very weird, very cinematic. Delivering packages gets oddly soothing.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Resident Evil 4",
            .release_date = "2023-03-24",
            .details = {
                .description = "Survival is just the beginning. Six years have passed since the biological disaster in Raccoon City.",
                .min_requirements = "GTX 1050 Ti",
                .max_requirements = "RTX 2070"
            },
            .platforms = { { plat_pc, plat_ps4, plat_ps5, plat_xsx } },
            .genres = { { gen_survival, gen_action, gen_shooter } },
            .developer_id = dev_capcom,
            .publisher_id = pub_capcom,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_retro, .rating = 10.0f, .comment = "A perfect remake. It honors the original while modernizing the mechanics flawlessly.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_pro99, .rating = 9.0f, .comment = "The knife parry system adds an incredible new layer of skill to the combat.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Final Fantasy VII Remake",
            .release_date = "2020-04-10",
            .details = {
                .description = "A spectacular reimagining of one of the most visionary games ever, exploring the city of Midgar.",
                .min_requirements = "GTX 780",
                .max_requirements = "GTX 1080"
            },
            .platforms = { { plat_ps4, plat_ps5, plat_pc } },
            .genres = { { gen_action_rpg, gen_jrpg, gen_rpg } },
            .developer_id = dev_square,
            .publisher_id = pub_square,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_pcmr, .rating = 9.5f, .comment = "The combat system is the best hybrid of action and turn-based I've ever experienced.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_lore, .rating = 9.0f, .comment = "Love how they expanded the lore of Midgar, even if some parts felt like padding.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "World of Warcraft",
            .release_date = "2004-11-23",
            .details = {
                .description = "Descend into the World of Warcraft and join thousands of mighty heroes in an online world of myth, magic, and limitless adventure.",
                .min_requirements = "GTX 760",
                .max_requirements = "GTX 1080"
            },
            .platforms = { { plat_pc, plat_macos } },
            .genres = { { gen_mmorpg, gen_rpg, gen_adventure } },
            .developer_id = dev_blizzard,
            .publisher_id = pub_activision,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_pcmr, .rating = 8.5f, .comment = "Have been playing for 15 years. It's a second job, but I love my guild.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_coop, .rating = 9.0f, .comment = "Nothing beats the feeling of a world-first raid clear.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Marvel's Spider-Man Remastered",
            .release_date = "2018-09-07",
            .details = {
                .description = "Play as an experienced Peter Parker, fighting big crime and iconic villains in Marvel's New York.",
                .min_requirements = "GTX 950",
                .max_requirements = "GTX 1060"
            },
            .platforms = { { plat_ps4, plat_ps5, plat_pc } },
            .genres = { { gen_action, gen_adventure } },
            .developer_id = dev_insomniac,
            .publisher_id = pub_sony,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_casual, .rating = 9.5f, .comment = "The web-swinging is so fluid and satisfying. You really feel like Spider-Man.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_trophy, .rating = 9.0f, .comment = "A really fun and accessible Platinum trophy. Crime grinding got a bit repetitive though.", .review_date = "CURR_DATE"})
                }
            }
        }
    );

    db.entity(
        game_entry{
            .title = "Sekiro: Shadows Die Twice",
            .release_date = "2019-03-22",
            .details = {
                .description = "Carve your own clever path to vengeance in an all-new adventure from developer FromSoftware.",
                .min_requirements = "GTX 760",
                .max_requirements = "GTX 970"
            },
            .platforms = { { plat_pc, plat_ps4, plat_xone } },
            .genres = { { gen_action, gen_adventure } },
            .developer_id = dev_fromsoft,
            .publisher_id = pub_activision,
            .reviews = {
                {
                    db.entity(review{.user_id = usr_pro99, .rating = 10.0f, .comment = "The tightest, most rewarding combat system ever designed. Hesitation is defeat.", .review_date = "CURR_DATE"}),
                    db.entity(review{.user_id = usr_noob69, .rating = 2.0f, .comment = "I can't even get past the Chained Ogre. Returning it.", .review_date = "CURR_DATE"})
                }
            }
        }
    );
}

#include "httplib.h"

int main()
{
	registry db({.name = "kawa_db", .max_entity_count = 1'000'000, .max_component_count = 32});

    populate_db(db);

    httplib::Server svr;

    svr.Get("/search.css",
        [](const httplib::Request&, httplib::Response& res)
        {
            res.set_file_content("search.css");
        }
    );

    svr.Get("/search", 
        [](const httplib::Request&, httplib::Response& res) 
        {
            res.set_file_content("search.html");
        }
    );

    svr.Get("/get-platforms",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            thread_local string out;
            out.clear();

            out += "<form id=\"platform-select\">";

            db.query(
                [&](platform& p)
                {
                    out += std::format(
                        "<input type = \"checkbox\" id = \"{0}\" name = \"genre\" value = \"{0}\">"
                        "<label for = \"{0}\">{0}</label><br>",
                        p.name);
                }
            );

            out += "</form>";


            res.set_content(out, "text/html");
        }
    );

    svr.Get("/get-genres",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            thread_local string out;
            out.clear();

            out += "<form id=\"genre-select\">";

            string htmx = "hx-on:change = \"document.body.dispatchEvent(new Event('refresh_search'))\"";

            db.query(
                [&](entity_id id, genre& g)
                {
                    out += std::format(
                        "<input {2} type = \"checkbox\" id = \"{0}\" name = \"genre\" value = \"{1}\">"
                        "<label {2} for = \"{0}\">{0}</label><br>",
                        g.name, (u64)id, htmx);
                }
            );

            out += "</form>";

            res.set_content(out, "text/html");
        }
    );

    svr.Get("/search-query",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            thread_local string out;
            thread_local string search_req;
            thread_local dyn_array<entity_id> selected_genres;

            selected_genres.clear();
            search_req.clear();
            out.clear();

            auto range = req.params.equal_range("genre");
            for (auto it = range.first; it != range.second; ++it)
            {
                selected_genres.push_back((entity_id)std::stoull(it->second));
            }


            search_req = req.get_param_value("query");

            db.query(
                [&](game_entry& game)
                {
                    if (game.title.find(search_req) == string::npos) return;

                    if (!selected_genres.empty()) 
                    {
                        bool has_all_selected = 
                            std::ranges::all_of(selected_genres, 
                                [&](entity_id selected_id) 
                                {
                                    return std::ranges::any_of(game.genres,
                                        [&](entity_id game_genre_id) 
                                        {
                                            return selected_id == game_genre_id;
                                        }
                                    );
                                }
                            );

                        if (!has_all_selected) return; 
                    }

                    out += "<li>" + game.title + "</li>";
                }
            );

            res.set_content(out, "text/html");
        }
    );

    svr.listen("0.0.0.0", 8085);
}