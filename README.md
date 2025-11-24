# NFSMW Slingshot Feature [Beta]

## Disclaimer
This mod is currently in **Beta**. Expect bugs, incomplete features, and behavior that may not reflect the final version.

## Installation
1. Download the latest version of the [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
2. Copy or move `dinput8.dll` to the root folder of the game.
3. Download this mod from the [releases page](https://github.com/Kevin4e/NFSMWSlingshotFeature/releases).
4. Copy or move `NFSMWSlingshotFeature.asi` into the game's `scripts` folder.

## Configuration
You can customize your own slingshot system, by setting **distances**, **boost multipliers**, etc.  
There's also a **leftover** which replaces the speedbreaker bar with the slingshot boost meter.  
  
Configuration file: `NFSMWSlingshotFeatureConfig.ini`.

## Knows Issues
1. The plugin isn't able to recognize vehicles separately from the world objects accurately, which causes two issues:
   - Some active vehicles in the open world may not be recognized.
   - Some world objects are mistakenly identified as vehicles.
  
2. The total distance between vehicles is calculated from the vehicles' center point, not from the rear.  
   - Therefore, the player's vehicle must be very close to vehicles with trailers.

## Credits
- **Kevin4e** - Author of the mod.

## Permissions
- You're **NOT** allowed to re-upload my mod anywhere else without my permission.
- You can use my mod on your modpack **as long as** you ask me privately.
