# CubeWorld ExtendedModLoader Mod
This mod allows you to list all your mods ingame, their status and some basic info. 

## Requirements
Download and setup the [Cube-World-Mod-Launcher](https://github.com/ChrisMiuchiz/Cube-World-Mod-Launcher).

## Installation
1. Download the lastest version from [here](https://github.com/Tandashi/Cube-World-ExtendedModLauncher-Mod/releases)
2. Unzip
3. Place the `libstdc++-6.dll` directly into the CubeWorld folder. Mostly located (C:\Program Files (x86)\Cube World)
4. Place the `ExtendedModLoader.dll` into the `Mods` folder.
5. Now you successfully installed the ExtendedModLoader

## For Modders
Provide your mods as follows:
- ModFolder (Can be named as you would like)
  - mod (See [modinfo](#modinfo) for further details)
  - mod.dll (can be named as you would like but has to be .dll)

## ModInfo
```json
{
  "name": "[MODNAME]",
  "version": "[MODVERSION]",
  "author": "[AUTHORNAME]"
}
```
