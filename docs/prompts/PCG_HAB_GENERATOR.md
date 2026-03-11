# Prompt: PCG Hab & Settlement Generator

Use this prompt in a fresh Claude Code session to plan and build a PCG-based procedural hab generator.

---

## The Prompt

I want to build a **Procedural Content Generation (PCG) system in UE5.7** that generates modular sci-fi hab units and eventually full settlements. Blueprints only, no C++. This is a learning project — I want to understand PCG deeply, not just get a result.

### Phase 1: Single Hab Generator
Build a PCG Graph that assembles a rectangular hab unit from modular pieces. The user places a PCG Actor, adjusts parameters in the Details panel, and the hab generates/regenerates live.

**Parameters to expose:**
- `HabLength` (int, 1-6) — number of 500cm modules along the long axis
- `HabWidth` (int, 1-3) — number of 500cm modules along the short axis
- `WindowDensity` (float, 0-1) — probability of wall-with-window vs solid wall
- `DoorCount` (int, 1-4) — number of door modules
- `DoorSide` (enum: Front/Back/Left/Right) — which face gets doors
- `HasRoof` (bool) — whether to generate roof tiles
- `Seed` (int) — randomization seed for reproducible results
- `Style` (enum: Clean/Weathered/Damaged) — future use for material variants

**Modular grid:** 500cm (5m) per module. All pieces snap to this grid.

**Available modular pieces** (all in `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/`):

| Category | Pieces |
|----------|--------|
| **Floors** | `Modular_Floor/SM_Modular_Merged` |
| **Walls (solid)** | `Modular_Wall_1/SM_Modular_Wall_1_Merged` |
| **Walls (windows)** | `Modular_WallWindow_1/SM_WallWindow_1_Merged`, `Modular_WallWindow_2/SM_Modular_WallWindow_2_Merged`, `Modular_WallWindow_3/SM_Modular_WallWindow_3_Merged` |
| **Doors** | `Modular_Door_Var1/SM_Modular_DoorModule_Merged` |
| **Corners** | `Modular_Corner/SM_Modular_Corner_Merged`, `Modular_WallCorner/SM_Modular_WallCorner_Merged` |
| **Roof** | `Modular_Ceiling/SM_Modular_RoofModule_Merged`, `Modular_Ceiling_Window1-3` |
| **Interior** | `Modular_InteriorWall`, `Modular_IndoorCorner`, `Modular_Interior_Door`, `Modular_IndoorWallWindow_1-3` |
| **Details** | `Vent/SM_VentModule_Merged`, `WallBracket/SM_WallBracket_Merged`, `Panel_1-5`, `WallPump`, `Modular_Lamp`, `Pole`, `Pillar_1` |
| **Roof details** | `RoofConnector_1-5` |
| **Structural** | `Construction_1-9`, `Platform_1-2`, `Balk_1`, `Base_1-2` |
| **Connectors** | `Modular_BridgeConnector_1-5` |
| **Pre-built modules** | `LivingModule_1-4` (each with indoor variant), `MainStation`, `CargoContainer_1-2` |

**Key pivot/rotation info we discovered:**
- Floor pivot is at the (+X, -Y, -Z) corner. At 0° yaw, floor extends 500cm in -X and +Y.
- Wall at 0° yaw: faces +X, extends 500cm in -Y from pivot, ~115cm thin in X.
- Yaw 90° = faces +Y, extends +X. Yaw 180° = faces -X, extends +Y. Yaw -90° = faces -Y, extends -X.
- Walls are 500cm tall (Z). Roof sits at floor Z + 500.
- Corners are small (128x128x505cm) and sit at grid junctions.

### Phase 2: Settlement Layout (future)
Once single habs work, expand to:
- Multiple hab types (barracks, command, medical, storage)
- Pathways/walkways between habs using BridgeConnector pieces
- Fences and perimeter walls
- Scatter props (barrels, containers, rocks from the ModularSciFi pack)
- Lighting (lamp posts, light panels)
- PCG-driven settlement layout from a spline boundary

### What I need from you:
1. **Research** UE5.7 PCG Framework — specifically the nodes needed for modular grid-based assembly (Create Points Grid, Static Mesh Spawner, Filter, Transform Points, Subgraphs)
2. **Plan** the PCG Graph structure — what subgraphs, what data flows, what attributes on points
3. **Provide step-by-step Blueprint instructions** for building the PCG Graph in the editor (I can't edit .uasset files, so give me node-by-node instructions)
4. **Start with Phase 1** — get a single parametric hab working before anything else

### Project context:
- **Engine:** UE5.7, Blueprints only
- **Game:** ThoughtSpace — atmospheric sci-fi thriller (NOT horror)
- **Style:** Moody, industrial outpost aesthetic (think Alien, The Expanse)
- **Studio:** Long Life Games (solo dev)
- **Guide files:** See `docs/guide/` for full project context
- **CLAUDE.md** in project root has architecture and conventions
