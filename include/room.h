#pragma once

#include "yoyo.h"

using Command_GotoRoomCallback = void(*)(int, int);

void add_room_goto_callback(Command_GotoRoomCallback new_callback);
void prepare_room_things();
CRoom* get_running_room();