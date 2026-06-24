#include "utils.h"

#include "steam_api.h"
#include "loader/memory.h"

#include <vector>
#include <Psapi.h>

CSteamID* steam_lobby_current = nullptr;

// for some reason the build of Steamworks.gml rivals uses is bugged
// steam_lobby_get_id_high and steam_lobby_get_id_low both return the same thing lmao
bool prepare_steamworks_hooks()
{
	std::vector<uint8_t> pattern_lobby_get_id_high
	{
		0xa1, DYNAMIC_PTR_FIELD,
		0x66, 0x0f, 0x6e, 0xc0, 
		0xf3, 0x0f, 0xe6, 0xc0, 
		0xc1, 0xe8, 0x1f,
		0xf2, 0x0f, 0x58, 0x04, 0xc5, 0x10, 0xdf, 0x01, 0x10
	};

	HMODULE steamworks = GetModuleHandle("Steamworks.gml.dll");
	if (!steamworks)
	{
		loader_log_error("roa-discord-rpc: could not get Steamworks.gml.dll handle");
		return false;
	}

	MODULEINFO minfo{};
	::GetModuleInformation(GetCurrentProcess(), steamworks, std::addressof(minfo), sizeof(minfo));

	// this is a offset from steamwork's dll, 
	// should be safe to hardcode this as they hardly update it anyway (this version is from 2019 LOL)
	uint32_t address = (uint32_t)minfo.lpBaseOfDll + 0x2137;

	if (address)
	{
		steam_lobby_current = *(CSteamID**)address;
		return true;
	}
	else
	{
		return false;
	}
}