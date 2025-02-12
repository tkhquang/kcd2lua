# KCD2 Lua Extension for VS Code

This extension allows you to write and execute Lua code directly in Kingdom Come: Deliverance 2 from VS Code. Write your scripts in a comfortable editor environment and see the results instantly in-game.

## Features

- Execute code directly in KCD2 with a single command or keybind
- Optionally run code automatically when saving your Lua file
- View execution results and errors in VS Code's output panel
- Works on both Windows and Linux (via Wine)

## Requirements

This extension requires the companion KCD2 Lua ASI mod to be installed in your game:

1. Download and install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
   - Place `dinput8.dll` in your game directory next to KingdomCome.exe: `KingdomComeDeliverance2/Bin/Win64MasterMasterSteamPGO/`

2. Get the KCD2 Lua ASI mod from [kcd2lua releases](https://github.com/yobson1/kcd2lua/releases)
   - Place `vscodelua.asi` in the same directory as above

## Getting Started

- Use `Ctrl+Shift+P` and search for "KCD2: Run Lua Code"
- You can also use Ctrl+Shift+R or enable the `Run On Save` setting to run code when you save the file
- The code will execute in-game and any output will appear in VS Code's output panel

## Contributing

Found a bug or want to contribute? Visit our [GitHub repository](https://github.com/yobson1/kcd2lua)

## License

MIT - see LICENSE file for details

## Credits

[Oren/ecaii](https://github.com/ecaii) - Original code and concept
