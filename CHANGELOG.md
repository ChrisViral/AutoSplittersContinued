# Changelog

## 0.5.1 - Input Rate Hotfix

- Fixed an issue with input rate being force set to zero

## 0.5.0 - U5 Compatibility Update

- Fixed compilation errors
- Updated mod reference to AutoSplittersContinued
- Update recipe and unlock costs:
  - Recipe went from 2 Iron Plates to 1 Modular Frame, 1 Stator, 1 Computer
  - Unlock cost went from 10 Iron Plates to 50 Modular Frames, 50 Computers
  - Unlock location went from a Tier 1 Milestone to a Tier 5 Milestone
- **WARNING** Previous saves made with this mod will *NOT* be compatible with this version. Due to mod reference changes, all existing auto splitters will be gone from your world.

## 0.4.0 - Alpha

Add missing bits for multiplayer support

### Changes
- Move all replicated properties into a nested struct to simplify change tracking (only requires a single onRep function). This bumps the serialization version.
- Also cleanup naming of some properties allowed by the version bump.
- Change all fixed size per-output `TArray`s into C arrays for lower overhead.
- Enable and disable dormancy as needed when clients interact with splitter.
- Add new `OnStateChanged` event that gets fired on client and server whenever state updates.
- That's a lie, as on the server we cannot fire the event from factory tick, so we have to use
  a timer in the widget.
- Update the widget to work correctly in both cient and server settings.
- Clean up the widget by removing lots of unused leftover functions, macros and variables.
- Add tracking functionality that warns users if they are playing with an alpha version.
- Add logging to RPCs
- Make sure all players are using 0.4.0 when playing in multiplayer.
- Remove earlier attempt at using a `ReplicationDetailsActor`. That was more finicky than it's worth, because the `FGBuildableConveyorAttachment` messes with the setup.

## 0.3.9

### Changes
- Rename game instance and game world module and add C++ base class for game world module.
- Add subsystem that gets stored in the savegame and allows for carrying global state across saves.
- Use subsystem to add proper version tracking of both the mod version and the serialization version
  that defines the layout of the properties stored in the save.
- Make upgrade process much more robust using those features and clean up a lot of the mess added when
  trying to debug the 0.2.0 -> 0.3.x crashes.
- Add horrible workaround for making the close button on the widget work. We have to derive from the
  original dark window widget, which somehow breaks data bindings, so now we have to manually update all state every frame. :\
- Lots of small cleanups and fixes.

## 0.3.8

### Changes

- Preload mod configuration during initialization to avoid spurious crashes due to a race condition in the configuration loader of SML.

## 0.3.7

### Changes

- Re-enable hook that keeps Satisfactory from crashing when upgrading splitters to auto splitters
- Completely rework savegame upgrade from 0.2.0 and older: Just dismantle old autosplitters and re-build them from scratch with
  the hologram.
- Fix a crash when attaching conveyors to an autosplitter that had all belts removed. Someday I'll remember the order of arguments
  in TArray::Init()...
- Salvage cases where the fixed precision arithmetic cannot perfectly assign output rates by distributing the remainder
  proportionally. Shouldn't be a big deal in terms of fairness, but might lead to fairly long cycle times.
- Reduce the risk of multiple threads stepping on each other's toes when the factory tick and grab output run simultaneously. I don't
  know whether that can actually happen, but let's build in some basic safeguards. I'd really like to avoid a full-blown lock on each
  splitter, though.
## 0.3.5

### Changes

- Make upgrade process more robust by comparing world coordinates when matching components.
- Delay all modifying operations until all `BeginPlay()` calls during the initial savegame initialization have
  finished.
- Add configuration system based on SML's config support.
- Add a fall-back upgrade method that simply removes all splitters when upgrading from 0.2.0 or older. Can be
  enabled through configuration.
- Make input rate configurable and fix some problems with propagation of auto state to upstream splitter.
- Take into account overclocking settings of downstream factories when calculating distribution shares. Limitations:
  - The calculation is based on fixed precision arithmetic, for now limited to a precision of 0.01%.
  - The allocation is rather finicky, as it will bail out if the shares cannot be precisely allocated without any remainder. 
  - The allocation is only updated when interacting with the network by building or removing components or showing the splitter
    UI. It will not auto-detect changes to the overclock rate.
- First internal changes to pave the way for multiplayer support.
## 0.3.0

### Changes
- Regular Splitters can now be upgraded to Auto Splitters with the build gun. Yay!
- The rate calculations are now based on fixed point arithmetic using the items/min metric.
  As a result, cycle times should no longer explode when playing with different rates. The number of fractional digits is limited to 3.
- Added awesome icons by Deantendo (Deantendo#4265 on the Modding discord) for the mod, the milestone
  and the recipe. Thank you so much!
- The UI now shows (and lets you enter) the items/min for each output. For now, the first splitter
  always assumes a completely full belt.
- The UI shows the assumed input rate that is used for the calculations.
- After changing a number, you have to hit Enter to make it effective. If the splitter network cannot
  calculate an item distribution based on your number, it will just revert to the old number.

### Caveats
- There is a known bug, where the boxes around splitters and merge don't disappear after upgrading to
  an Auto Splitter. As a workaround, start building something else like a belt and exit the build mode again.

## 0.2.0

- Fixed positioning of splitters in relation to vanilla splitters and mergers.

## 0.1.0

- Initial release
