# roa-discord-rpc

implements a discord rich presence activity for Rivals of Aether

###### requires roa-mod-loader to be installed in rivals' directory (https://github.com/raicool/roa-mod-loader/releases)

<img src="res/discord-example.png">

# building & installing
> If you dont want to compile the project yourself, precompiled versions are available in the [Releases](https://github.com/raicool/roa-discord-rpc/releases) page

###### requires Visual Studio 2022 in order to be built

1. clone the repository as well as submodules:
```sh
	git clone https://github.com/raicool/roa-discord-rpc --recurse-submodules
	cd roa-discord-rpc
```
2. using Visual Studio's x86 Dev command prompt *vcvars32.bat* run
```sh
	cmake -G "Ninja" -DCMAKE_SYSTEM_PROCESSOR=i386 -B bin/
	ninja -C bin/
```
3. copy the dll `bin/roa-discord-rpc.dll` as well as `bin/discord_game_sdk.dll` into the mods directory of rivals of aether, e.g `C:/Program Files (x86)/Steam/steamapps/common/Rivals of Aether/mods/`
	- if the mods directory doesnt already exists, you will have to create one
