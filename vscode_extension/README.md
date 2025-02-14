# KCD2 Lua Extension for VS Code

This extension allows you to write and execute Lua code directly in Kingdom Come: Deliverance 2 from VS Code. Write your scripts in a comfortable editor environment and see the results instantly in-game.

## Features

- Execute code directly in KCD2 with a single command or keybind
- Automatically find startup scripts and run them and all their dependencies
- Automatically find and run all script dependencies added by Script.ReloadScript in the workspace
- Optionally run code automatically when saving your Lua file
- View execution results and errors in VS Code's output panel
- Works on both Windows and Linux (via Wine)

## Requirements

This extension requires the companion KCD2 Lua ASI mod to be installed in your game:

1. Download and install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
   - Place `dinput8.dll` in your game directory next to KingdomCome.exe: `KingdomComeDeliverance2/Bin/Win64MasterMasterSteamPGO/`

2. Get the KCD2 Lua ASI mod from [kcd2lua releases](https://github.com/yobson1/kcd2lua/releases)
   - Place `vscodelua.asi` in the same directory as above

Note that the asi and extension's major version must match to be compatible. 1.X.X will work with 1.X.X but not 2.X.X+\
If the extension stops working check your versions are compatible, there may be an update available for the asi

## Getting Started

- Use `Ctrl+Shift+P` and search for "KCD2: Run Lua Code" to run the currently open file
- You can also use Ctrl+Shift+R or enable the `Run On Save` setting to run code when you save the file
- Use `Ctrl+Shift+P` and search for "KCD2: Run Workspace Scripts" to find and run all startup scripts
- You can also use Ctrl+Alt+R to find and run all startup scripts
- Startup scripts are any Lua files found in directories named Mods or Startup, as you would have in your mod's .pak file. These can be nested in any number of other directories. For example `MyProject/Scripts/Startup/MyMod.lua`, `Mods/MyMod.lua`, `Example/Project/Mods/MyMod.lua`
- Any scripts added by `Script.ReloadScript` that can be found in the workspace will also be run, recursively
- The code will execute in-game and any output will appear in VS Code's output panel
- Keep mind if you re-run a script you already ran you may overwrite globals you previously defined, if they contain data collected at runtime it will be lost. To avoid this it's good practice to define your globals like this: `MyModTable = MyModTable or {}`

## Contributing

Found a bug or want to contribute? Visit our [GitHub repository](https://github.com/yobson1/kcd2lua)

## License

MIT - see LICENSE file for details

## Credits

[Oren/ecaii](https://github.com/ecaii) - Original code and concept
