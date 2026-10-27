# UnReal World Quest Trainer

A standalone Windows tool for translation researchers and testers of **UnReal World 3.89.1**. It creates native quest offers at a selected NPC and adds base items through the game’s own controls. Quest offers must then be accepted in a normal conversation before appearing in F2.

**[Download the latest ZIP](https://github.com/Librianets/UnRealWorld_QuestTrainer/releases/latest)**. Extract all files into a separate folder and start `URW_QuestTrainer.exe`. Run the game, load a test character and return to the local map with menus closed. Follow the trainer instructions, wait for CHOOSE TARGET, then select an NPC. Keep the game focused while automation runs.

Use a test character: the game persists generated items and quests. Native requirements still apply, including season, elapsed days, NPC/world conditions and duplicate-quest restrictions. An offer is not guaranteed. Version 0.06 adds Russian item labels and a native command to reach played-day 100. The calendar and elapsed time advance too; older quest deadlines may expire. Offers must still be accepted through conversation for F2. Other native quest requirements still apply.

## Day and item tools

Click **Довести стаж до 100 дней** to have the game's own developer command skip the missing days. It stops at day 100 and leaves characters with 100 or more days unchanged. Use a test character: this advances the calendar and elapsed game time, including existing quest deadlines. It does not waive NPC, season, settlement or duplicate requirements. Accept a generated offer in a normal NPC conversation to add it to F2.

The item list shows Russian and English names for all 370 supported prototype records. Names can be entered in either language. Entries with the same name show their native IDs; select the required row to preserve that exact prototype.

## Updates

Click **Проверить обновления тренера** (Check trainer updates). The manifest is downloaded directly from the latest release, without the REST API or its anonymous-request limit. Windows PowerShell 5.1 and Internet access are required. If a newer GitHub release is available, confirm installation. The updater verifies the ZIP and all file SHA256 hashes, closes/restarts only the trainer and keeps one previous copy in `URW_Trainer_backup`. The game may remain open. Keep `URW_TrainerUpdater.ps1` beside the EXE. Manual ZIP updates require closing the trainer first.

## Compatibility and behaviour

Requires the clean 32-bit 3.89.1 game EXE, SHA256 `c6ed73642c1b1cb1f86b818c7586224398c2f0554f6114ced31bc40ff071806d`. English play works without a translation hook. Known hooks are checked separately; unknown binaries are refused. The tool sends foreground keyboard input and reads process state, briefly suspending the game thread to inspect its stack. It does not write process memory, modify the EXE, inject a DLL or directly edit saves. The game itself changes/saves the world. No trainer logs are written during normal use; diagnostic commands are opt-in.

## Build and licence

Visual Studio 2022 C++ x86: run `build.cmd`. Source code is under 0BSD; no game files or translation DLLs are included. See [Russian instructions](README.txt) and [тренер_0.06.txt](тренер_0.06.txt). The trainer is separate from the [translation patch](https://github.com/Librianets/UnRealWorld_MultiTranslate/releases/latest).
