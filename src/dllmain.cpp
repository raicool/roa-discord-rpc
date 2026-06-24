#include "steamworks_lobby_hook.h"

#include <loader/yyc.h>
#include <loader/memory.h>
#include <loader/hook.h>

#include "discord.h"
#include "steam_api.h"

#include "callback.h"
#include "stage_room_name.h"
#include "utils.h"

#include <csignal>
#include <room.h>

#define DISCORD_SDK "discord_game_sdk.dll"
#define DISCORD_APP_ID 1518032723342659634
#define ROA_STEAM_ID 383980

extern CSteamID* steam_lobby_current;

enum lobby_game_type
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

void exit_proc(HMODULE hModule)
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
	FreeLibraryAndExitThread((HMODULE)hModule, 0);
}

void update_discord_activity()
{
	g_discord_core->ActivityManager().UpdateActivity(*g_activity, [](discord::Result result) {
		if (result != discord::Result::Ok)
		{
			loader_log_error("roa-discord-rpc: error while attempting to update rich pressence (error id: {})", (int)result);
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
	case lobby_game_type::EXHIBITION: return "Casuals";
	case lobby_game_type::PRIVATE: return "Friendlies";
	case lobby_game_type::RANKED: return "Ranked";
	default: return "Unknown";
	}

	return nullptr;
}

extern char* (__cdecl* Room_Name)(int);
void on_room_start(int room_id, int param2)
{
	const char* current_room_name = Room_Name(room_id);

	loader_log_debug("room name = {}", current_room_name);

	stage_small_image_name* stage_info = nullptr;
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
DWORD WINAPI entry(LPVOID hModule)
{
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

// 		int token = g_discord_core->ActivityManager().OnActivityJoin.Connect(
// 			[](const char* secret) 
// 			{
// 				g_discord_core->ActivityManager().RegisterCommand(secret);
// 			}
// 		);
	}
	else
	{
		loader_log_error("roa-discord-rpc: error while trying to creating discord::Core instance", DISCORD_SDK);
	}

	std::signal(SIGINT, [](int) { interrupted = true; });

	do 
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

					snprintf(lobby_id_str, 20, "%lu", lobby_id);
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


					// commented all this out since it doesnt rly work
					
// 					if (is_friendlies_lobby)
// 					{
// 						const CSteamID self_id = SteamUser()->GetSteamID();
// 						const CSteamID lobby_owner_id = SteamMatchmaking()->GetLobbyOwner(*steam_lobby_current);
// 
// // 						steam_invite_link = std::format(
// // 							"steam://joinlobby/{}/{}/{}",
// // 							ROA_STEAM_ID,
// // 							lobby_id,
// // 							lobby_owner_id.ConvertToUint64()
// // 							);
// 
// // 						steam_invite_link = std::format(
// // 							"https://steamjoin.com/{}",
// // 							lobby_owner_id.ConvertToUint64()
// // 						);
// 
// //						g_discord_core->ActivityManager().RegisterCommand(steam_invite_link.c_str());
// 						update_discord_activity();
// 
// 						g_activity->GetSecrets().SetJoin(steam_invite_link.c_str());
// 						g_activity->SetSupportedPlatforms((uint32_t)discord::ActivitySupportedPlatformFlags::Desktop);
// 
// // 						g_discord_core->OverlayManager().SetLocked(false, [](discord::Result result) {
// // 							if (result != discord::Result::Ok)
// // 							{
// // 								loader_log_error("roa-discord-rpc: error while attempting to update overlay locked state (error id: {})", (int)result);
// // 							}
// // 							});
// // 						g_discord_core->OverlayManager().OpenActivityInvite(discord::ActivityActionType::Join, [](discord::Result result) {
// // 							if (result != discord::Result::Ok)
// // 							{
// // 								loader_log_error("roa-discord-rpc: error while attempting to update rich pressence (error id: {})", (int)result);
// // 							}
// // 							});
// 					}
// 					else
// 					{
// // 						g_discord_core->OverlayManager().SetLocked(true, [](discord::Result result) {
// // 							if (result != discord::Result::Ok)
// // 							{
// // 								loader_log_error("roa-discord-rpc: error while attempting to update overlay locked state (error id: {})", (int)result);
// // 							}
// // 							});
// 					}

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

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	} while (!interrupted);

	return TRUE;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);

		HANDLE hHandle = CreateThread(NULL, 0, entry, hModule, 0, NULL);
		if (hHandle != NULL)
		{
			CloseHandle(hHandle);
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH && !lpReserved)
	{
		FreeLibraryAndExitThread(hModule, 0);
	}

	return TRUE;
}