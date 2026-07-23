# Signal Triangulation

A Satisfactory mod that upgrades your radar network: research **Signal Triangulation** in the
MAM (Quartz tree, after Radar Technology) to reveal the exact map locations of every
uncollected **Somersloop**, **Mercer Sphere**, and **crash site** on the planet.

## Features

- New MAM research node grafted into the vanilla Quartz tree below Radar Technology
  (cost: 5× Crystal Oscillator, 2× Heavy Modular Frame; 3-second analysis).
- Map markers with proper artifact icons for every uncollected artifact, sourced from the
  game's own world-scannable data (works for unexplored areas — that's the fantasy: your
  radar towers triangulated them).
- Markers disappear automatically within ~10 seconds of collecting an artifact or looting
  a crash site.
- Each artifact type is its own map filter category (Somersloops / Mercer Spheres /
  Hard Drives) with independent show/hide toggles in the map sidebar.

## Console variables

| CVar | Default | Effect |
| --- | --- | --- |
| `ST.ShowSomersloops` | 1 | Show/hide Somersloop markers |
| `ST.ShowMercerSpheres` | 1 | Show/hide Mercer Sphere markers |
| `ST.ShowHardDrives` | 1 | Show/hide crash site markers |
| `ST.ForceUnlock` | 0 | Treat the research as unlocked (testing) |

Changes apply within ~10 seconds.

## Implementation notes

- Artifact positions come from `AFGScannableSubsystem`'s cook-time cache (the same data
  radar towers use for their "N in this area" counts), filtered by liveness
  (`DoesPickupExist` / `HasDropPodBeenLooted`).
- The MAM node is injected into the vanilla Quartz research tree at world load via
  reflection over the tree's node data (`STTreeInjector`).
- Per-artifact map filter categories are custom `ERepresentationType` values registered at
  runtime; category labels come from an SML blueprint-script hook on the map filter
  widget's `GetCategoryName` (`STMapHooks`).
- Map icons are material instances of the game's `MM_UI_CompassIcon` master with the
  artifact icon and a white background baked in (authored headlessly by
  `DevTools/author_materials.py`).

## Building

Standard SML mod: place in the starter project's `Mods/` folder and package with Alpakit.
The content assets (schematic, icon materials) are authored by the scripts in `DevTools/`
via the headless editor:

```
UnrealEditor-Cmd FactoryGame.uproject -run=pythonscript -script=Mods/SignalTriangulation/DevTools/author_assets.py -nullrhi -unattended
UnrealEditor-Cmd FactoryGame.uproject -run=pythonscript -script=Mods/SignalTriangulation/DevTools/author_materials.py -nullrhi -unattended
```

## Compatibility

- Game 1.2 (SML ^3.12.0). Singleplayer tested; multiplayer and dedicated servers untested
  (markers are server-authoritative and replicate via the representation manager, so MP
  should work — reports welcome).
- The category-label hook targets the vanilla map filter widget; a future game update could
  change that widget, in which case labels fall back to blank rows until the mod updates
  (everything else keeps working).
