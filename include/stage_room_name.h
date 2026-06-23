#pragma once

#include <unordered_map>

struct stage_small_image_name
{
	const char* api_name; // name of image in discord rpc assets
	const char* display_name; // name to be used as small image caption
};

static std::unordered_map<const char*, stage_small_image_name> s_stage_rooms
{
	// 0
	{"stage_fire_clean", {"fire-capital", "Fire Capital"}},
	{"stage_air_clean", {"air-armada", "Air Armada"}},
	{"stage_earth_clean", {"rock-wall", "Rock Wall"}},
	{"stage_water_clean", {"merchant-port", "Merchant Port"}},
	{"stage_tree_clean", {"treetop-lodge", "Treetop Lodge"}},
	{"stage_smoke_clean", {"blazing-hideout", "Blazing Hideout"}},
	{"stage_mount_clean", {"tempest-peak", "Tempest Peak"}},
	{"stage_ice_clean", {"frozen-fortress", "Frozen Fortress"}},
	{"stage_holy_clean", {"tower-of-heaven", "Tower of Heaven"}},
	{"stage_boss_clean", {"aethereal-gates", "Aethereal Gates"}},
	{"stage_abyss_clean", {"the-endless-abyss", "The Endless Abyss"}},
	{"stage_ori_clean", {"spirit-tree", "The Spirit Tree"}},
	{"stage_grass_1v1", {"forest-floor", "The Forest Floor"}},
	{"stage_steam_clean", {"julesvale", "Julesvale"}},
	{"stage_gus_clean", {"troupple-pond", "Troupple Pond"}},

	// 1
	{"stage_earth_ring_a", {"roa-ring", "The RoA Ring"}},
	{"stage_plasma_clean", {"neo-fire-capital", "Neo Fire Capital"}},
	{"stage_poison_clean", {"swampy-estuary", "The Swampy Estuary"}},
	{"stage_grid_alt", {"tutorial-grid", "The Tutorial Grid"}},
	{"stage_sfront_clean", {"aether-high", "Aether High"}},
	{"stage_cavern_clean", {"frozen-gates", "Frozen Gates"}},
	{"stage_gusto_clean", {"pridemoor-keep", "Primedoor Keep"}},
	{"stage_wsfire_clean", {"neo-blazing-rail", "Neo Blazing Rail"}},
	{"stage_wswater_clean", {"highdive-hideaway", "Highdive Hideaway"}},
	{"stage_wsair_clean", {"neo-julesvale", "Neo Julesvale"}},
	{"stage_wsearth_clean", {"crystal-oasis", "Crystal Oasis"}},

	// 2
	{"stage_earth_ring", {"ceo-ring", "The CEO Ring"}},

	// ?
	{"stage_holy_alt", {"tower-of-heaven", "Tower of Heaven X"}},
	{"stage_training_clean", {"tutorial-grid", "Practice Room"}},
};