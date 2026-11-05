# URW Quest Trainer 0.21

For UnReal World 3.89.1 on Windows, compatible with Hook 0.43 and earlier supported builds.

Download **URW_QuestTrainer_0.21.zip** from [Releases](https://github.com/Librianets/UnRealWorld_QuestTrainer/releases/latest), extract it to its own folder and run **URW_QuestTrainer.exe**. The trainer has a Russian interface. Read the [Russian instructions](README_RU.md) for controls and limits.

Since public 0.15: separate character, skill and condition/wound editors; planned changes with bounds and combined native-value validation; existing world item variants including hides/furs; ordinary waiting for 1–7 days; spawning one of 39 animal types on a checked adjacent ground tile. Existing NPC quest, F5 learning task, item search and skill-point tools remain available.

Changes affect the loaded character and world. Use a test character or a backup. Character editors show the allowed range for each field and reject combinations that violate the game's checks. Advancing the service calendar to day 100 is distinct from ordinary waiting and is not a substitute for processing drying items.

Animal spawning uses the game's generator; sex, behaviour and tameness are not selected. Animals can be hostile. Once the target cell is confirmed the native MC ID prompt cannot be cancelled safely with Escape; follow the trainer's recovery instructions.

The normal ZIP retains the legacy updater layout with bundled headers. The optional **fullsource.zip** contains the same executable and separate source headers. The repository contains separate headers and **build.cmd** for an x86 MSVC build. The game itself is not included.
