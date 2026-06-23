#include "room.h"

#include "utils.h"
#include "yoyo.h"

#include <loader/memory.h>

CRoom* running_room = nullptr;
std::vector<Command_GotoRoomCallback> callbacks;

char*(__cdecl* Room_Name)(int) = nullptr;
void(__cdecl* Command_GotoRoom)(int, int) = nullptr;

void __cdecl Command_GotoRoom_detour(int param1, int param2)
{
	for (Command_GotoRoomCallback& _func : callbacks)
	{
		_func(param1, param2);
	}
	return Command_GotoRoom(param1, param2);
}

void add_room_goto_callback(Command_GotoRoomCallback new_callback)
{
	if (std::find(callbacks.begin(), callbacks.end(), new_callback) >= callbacks.end())
	{
		callbacks.emplace_back(new_callback);
	}
}

void _get_room_ptr()
{
	std::vector<uint8_t> pattern =
	{
		0xa1, DYNAMIC_PTR_FIELD,
		0x56,
		0x57,
		0x8b, 0x7c, 0x24, 0x0c,
		0x8b, 0xb0, 0x80, 0x00, 0x00, 0x00,
		0x85, 0xf6,
		0x74, 0x2a
	};

	uint32_t loc = (uint32_t)loader_search_memory(pattern);

	if (loc)
	{
		running_room = *(CRoom**)INCR_PTR(loc, 1);
	}
}

void _get_room_name_funcptr()
{
	std::vector<uint8_t> pattern =
	{
		0x8b, 0x44, 0x24, 0x04,
		0x85, DYNAMIC_OFFSET_8,
		0x78, 0x24,
		0x3b, 0x05, DYNAMIC_PTR_FIELD,
		0x7d, 0x1c,
		0x8b, 0x0d, DYNAMIC_PTR_FIELD,
	};

	void* loc = loader_search_memory(pattern);

	if (loc)
	{
		Room_Name = reinterpret_cast<char*(*)(int)>(loc);
	}
}

void _set_room_goto_hook()
{
	std::vector<uint8_t> pattern =
	{
		0x8b, 0x44, 0x24, 0x04,
		0x50,
		0xa3, DYNAMIC_PTR_FIELD,
		0xc7, 0x05, DYNAMIC_PTR_FIELD, 0x00, 0x00, 0x00, 0x00,
		0xe8, DYNAMIC_PTR_FIELD
	};

	void* loc = loader_search_memory(pattern);

	if (loc)
	{
		loader_hook_create(loc, &Command_GotoRoom_detour, reinterpret_cast<void**>(&Command_GotoRoom));
		loader_hook_enable(loc);
	}
}

void prepare_room_things()
{
	_get_room_ptr();
	_get_room_name_funcptr();

	_set_room_goto_hook();
}

CRoom* get_running_room()
{
	return running_room;
}