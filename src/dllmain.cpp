#include "steamworks_lobby_hook.h"

#include <loader/yyc.h>
#include <loader/memory.h>
#include <loader/hook.h>

#include "discord.h"
#include "steam_api.h"
#include "stage_room_name.h"
#include "utils.h"

#include <csignal>
#include <filesystem>
#include <room.h>
#include <d3d11.h>
#include <loader/d3d11_hook.h>

#define DISCORD_SDK "discord_game_sdk.dll"
#define DISCORD_APP_ID 1518032723342659634
#define ROA_STEAM_ID 383980

extern CSteamID* steam_lobby_current;

enum LobbyGameTypeEnum
{
	UNKNOWN = 0,
	EXHIBITION = '1',
	PRIVATE = '2',
	RANKED = '3',
};

discord::Core* g_discord_core;
std::shared_ptr<discord::Activity> g_activity;
std::shared_ptr<discord::Lobby> g_lobby;
std::string g_activitystate;

bool interrupted = false;

bool prepare_discord_sdk()
{
	bool core_loaded = false;
	do 
	{
		if (discord::Core::Create(DISCORD_APP_ID, DiscordCreateFlags_NoRequireDiscord, &g_discord_core) == discord::Result::Ok)
		{
			loader_log_info("welcome to roa-discord-rpc (version {})", VERSION_STR);
			core_loaded = true;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		}
	} while (core_loaded == false);
	
	return true;
}

void update_discord_activity()
{
	g_discord_core->ActivityManager().UpdateActivity(*g_activity, [](discord::Result result) {
		if (result != discord::Result::Ok)
		{
			loader_log_error("error while attempting to update rich pressence (error id: {})", (int)result);
		}
		});
}

void discord_log(discord::LogLevel level, const char* message)
{
	if (message)
		loader_log_debug("discord: {}", message);
}

const char* get_game_type_string(const char* _gametype_lobby_data)
{
	uint8_t gametype_u8 = _gametype_lobby_data[0];

	switch (gametype_u8)
	{
	case LobbyGameTypeEnum::EXHIBITION: return "Casuals";
	case LobbyGameTypeEnum::PRIVATE: return "Friendlies";
	case LobbyGameTypeEnum::RANKED: return "Ranked";
	default: return "Unknown";
	}

	return nullptr;
}

extern char* (__cdecl* Room_Name)(int);
void on_room_start(int room_id, int param2)
{
	const char* current_room_name = Room_Name(room_id);

	//loader_log_debug("room name = {}", current_room_name);

	StageSmallImageName* stage_info = nullptr;
	for (auto& [_key, _value] : s_stage_rooms)
	{
		if (strcmp(current_room_name, _key) == 0)
		{
			stage_info = &_value;
			break;
		}
	}

	if (stage_info)
	{
		g_activity->GetAssets().SetSmallImage(stage_info->api_name);
		g_activity->GetAssets().SetSmallText(stage_info->display_name);
		g_activitystate = std::format("Ingame | {}", stage_info->display_name);
	}
	else
	{
		g_activity->GetAssets().SetSmallImage("");
		g_activity->GetAssets().SetSmallText("");

		if (strcmp("mainMenu_room", current_room_name) == 0)
		{
			g_activitystate = "in the Main Menu";
		}

		if (strcmp("network_char_select", current_room_name) == 0)
		{
			g_activitystate = "selecting a character online...";
		}

		if (strcmp("local_charselect_room", current_room_name) == 0 ||
			strcmp("training_charselect_room", current_room_name) == 0)
		{
			g_activitystate = "selecting a character offline...";
		}

		if (strcmp("local_result_screen", current_room_name) == 0 ||
			strcmp("online_result_screen", current_room_name) == 0)
		{
			g_activitystate = "looking at match results";
		}
	}

	g_activity->SetDetails(g_activitystate.c_str());
	update_discord_activity();
}

uint64_t last_lobby_id;
char lobby_id_str[20];
std::string steam_invite_link;
void present(ID3D11RenderTargetView*, IDXGISwapChain*, ID3D11Device*, ID3D11DeviceContext*)
{
	discord::ActivityParty* _party = &g_activity->GetParty();

	if (steam_lobby_current)
	{
		if (steam_lobby_current->IsValid())
		{
			const int lobby_limit = SteamMatchmaking()->GetLobbyMemberLimit(*steam_lobby_current);
			const int lobby_members = SteamMatchmaking()->GetNumLobbyMembers(*steam_lobby_current);
			const char* lobby_name = SteamMatchmaking()->GetLobbyData(*steam_lobby_current, "title");

			_party->GetSize().SetCurrentSize(lobby_members);
			_party->GetSize().SetMaxSize(lobby_limit);

			uint64_t lobby_id = steam_lobby_current->ConvertToUint64();
			if (last_lobby_id != lobby_id)
			{
				last_lobby_id = lobby_id;

				snprintf(lobby_id_str, 20, "%llu", lobby_id);
				const char* gametype_lobby_data = SteamMatchmaking()->GetLobbyData(*steam_lobby_current, "gametype");
				const char* gametype = get_game_type_string(gametype_lobby_data);

				_party->SetId(lobby_name);

				bool is_friendlies_lobby = (strcmp(gametype_lobby_data, "2") == 0);

				is_friendlies_lobby ?
					_party->SetPrivacy(discord::ActivityPartyPrivacy::Public) :
					_party->SetPrivacy(discord::ActivityPartyPrivacy::Private);

				g_activitystate = std::format("in a {} lobby", gametype);
				g_activity->SetDetails(g_activitystate.c_str());
				g_activity->SetState(lobby_name);


				if (is_friendlies_lobby)
				{
					const CSteamID self_id = SteamUser()->GetSteamID();
					const CSteamID lobby_owner_id = SteamMatchmaking()->GetLobbyOwner(*steam_lobby_current);

					steam_invite_link = std::format(
						"steam://joinlobby/{}/{}/{}",
						ROA_STEAM_ID,
						lobby_id,
						lobby_owner_id.ConvertToUint64()
						);

// 						steam_invite_link = std::format(
// 							"https://steamjoin.com/{}",
// 							lobby_owner_id.ConvertToUint64()
// 						);

					g_discord_core->ActivityManager().RegisterCommand(steam_invite_link.c_str());
					update_discord_activity();

					g_activity->GetSecrets().SetJoin(lobby_id_str);
					g_activity->SetSupportedPlatforms((uint32_t)discord::ActivitySupportedPlatformFlags::Desktop);

					g_discord_core->OverlayManager().SetLocked(false, [](discord::Result result) {
						if (result != discord::Result::Ok)
						{
							loader_log_error("error while attempting to update overlay locked state (error id: {})", (int)result);
						}
						});

// 					g_discord_core->OverlayManager().OpenActivityInvite(discord::ActivityActionType::Join, [](discord::Result result) {
// 						if (result != discord::Result::Ok)
// 						{
// 							loader_log_error("error while attempting to update rich pressence (error id: {})", (int)result);
// 						}
// 						});
				}
				else
				{
					g_discord_core->OverlayManager().SetLocked(true, [](discord::Result result) {
						if (result != discord::Result::Ok)
						{
							loader_log_error("error while attempting to update overlay locked state (error id: {})", (int)result);
						}
						});
				}

				update_discord_activity();
			}
		}
		else
		{
			last_lobby_id = 0;
			g_activity->SetState("");
			_party->GetSize().SetCurrentSize(0);
			_party->GetSize().SetMaxSize(0);
			update_discord_activity();
		}
	}

	g_discord_core->RunCallbacks();
}

PFUNC_YYGMLScript steam_lobby_join_id_orig = nullptr;
PFUNC_YYGMLScript dp_steam_lobby_join_orig = nullptr;

RValue& steam_lobby_join_id_detour(CInstance* self, CInstance* other, RValue* out, int argc, RValue** argv)
{
	return steam_lobby_join_id_orig(self, other, out, argc, argv);
}

RValue& dp_steam_lobby_join_detour(CInstance* self, CInstance* other, RValue* out, int argc, RValue** argv)
{
	return dp_steam_lobby_join_orig(self, other, out, argc, argv);
}

LOADER_MOD_ENTRY initialize(loader_info* loaderinfo)
{
	loader_mod& mod = loader_mod::get();
	mod.name = "roa-discord-rpc";
	mod.description = "implements a discord rich presence activity for Rivals of Aether";
	mod.repository = "https://github.com/raicool/roa-discord-rpc";

	char module_path[128];
	GetModuleFileName(0, module_path, 128);
	const std::string module_folder = std::filesystem::path(module_path).parent_path().string();
	const std::string discord_sdk_path = std::format("{}/mods/discord_game_sdk.dll", module_folder, module_path);

	HMODULE discord_sdk = LoadLibraryA(discord_sdk_path.data());

	if (discord_sdk == nullptr)
	{
		loader_log_error("failed to load discord sdk! is it present in the mods folder?");
		return mod;
	}

	void* steam_lobby_join_id = loader_get_script("gml_Script_steam_lobby_join_id");
	void* dp_steam_lobby_join = loader_get_script("gml_Script_dp_steam_lobby_join");
	loader_hook_create(steam_lobby_join_id, &steam_lobby_join_id_detour, reinterpret_cast<void**>(&steam_lobby_join_id_orig));
	loader_hook_create(dp_steam_lobby_join, &dp_steam_lobby_join_detour, reinterpret_cast<void**>(&dp_steam_lobby_join_orig));
	loader_hook_enable(steam_lobby_join_id);
	loader_hook_enable(dp_steam_lobby_join);

	if (prepare_discord_sdk())
	{
		prepare_room_things();
		add_room_goto_callback(on_room_start);

		if (!prepare_steamworks_hooks())
		{
			loader_log_error("roa-discord-rpc: could not find steam_lobby_current address in Steamworks.gml.dll");
		}

		g_activity = std::make_shared<discord::Activity>();
		g_lobby = std::make_shared<discord::Lobby>();

		g_activity->SetType(discord::ActivityType::Playing);
		g_activity->SetName("Activity");
		g_activity->GetAssets().SetLargeImage("rivals-icon-zetter");

		g_discord_core->SetLogHook(discord::LogLevel::Debug, discord_log);

		g_discord_core->ActivityManager().RegisterSteam(ROA_STEAM_ID);

		int token = g_discord_core->ActivityManager().OnActivityJoin.Connect(
			[](const char* secret) 
			{
//				g_discord_core->ActivityManager().RegisterCommand(secret);
				std::string secret_str = secret;

				if (std::find_if(secret_str.begin(),
					secret_str.end(), [](unsigned char c) { return !std::isdigit(c); }) != secret_str.end())
				{
					loader_log_warn("invalid secret id passed to OnActivityJoin!");
					return;
				}

				loader_log_info("Attempting to join Steam lobby \"{}\"...", secret_str);

				auto lobby_id = std::stoull(secret);

				// no explicit constructor for int64_t :p
				RValue _rv_lobby_id;
				_rv_lobby_id.set_s64(lobby_id);

				auto NetworkManager = loader_get_object_instances("NetworkManager")[0];
				loader_call_script("gml_Script_dp_steam_lobby_join", NetworkManager, NetworkManager, { &_rv_lobby_id });
			}
		);
	}
	else
	{
		loader_log_error("error while trying to creating discord::Core instance", DISCORD_SDK);
	}

	loader_add_present_callback(present);

	return mod;
}

LOADER_MOD_EXIT shutdown()
{
	g_discord_core->ActivityManager().ClearActivity([](discord::Result result)
		{
			if (result == discord::Result::Ok)
			{
				loader_log_debug("roa-discord-rpc: cleared activity!");
			}
			else
			{
				loader_log_error("roa-discord-rpc: error while attempting to cleared activity (error id: {})", (int)result);
			}
		});
}