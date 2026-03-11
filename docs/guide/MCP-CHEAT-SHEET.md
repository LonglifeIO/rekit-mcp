# MCP Cheat Sheet — Top 20 Prompts

> Quick reference. For full prompt library, see `docs/guide/04-MCP-WORKFLOWS.md`.

---

## Connection Checks (3)

| # | Prompt | Server |
|---|--------|--------|
| 1 | "Get the current scene info from Blender" | Blender |
| 2 | "Get the list of actors in the current level" | Unreal |
| 3 | "Check connection to Blender MCP and Unreal MCP — report status of each" | Both |

---

## Blender: Blockouts (4)

| # | Prompt |
|---|--------|
| 4 | "Build a modular sci-fi corridor section — 4m wide, 3m tall, 6m long, with recessed lighting panels in the ceiling, conduit pipes along the walls, and a metal grate floor. Dark gunmetal material with orange accent lights" |
| 5 | "Create a small outpost room — 6m x 8m, metal walls with panel seams, workstation desk, overhead strip lighting, exposed ceiling pipes, window frame on one wall. Dark industrial materials with blue accent lighting" |
| 6 | "Create a circular meditation chamber — 5m diameter, 4m ceiling, central raised platform (1m dia, 0.3m high) with spiral patterns, 4 pillars at cardinal directions, alcoves in walls. Dark stone base, cyan emissive accents" |
| 7 | "Create a rocky ice terrain section — 10m x 10m, irregular surface, ice blue-white material, 3-5 dark gray rock formations, 2 bioluminescent plant props (glowing cyan stems with bulb tops)" |

---

## Blender: Props (3)

| # | Prompt |
|---|--------|
| 8 | "Create a sci-fi supply crate — 1m cube, dark metal with scratches, glowing blue trim edges, handle on top, hazard stripes on one side" |
| 9 | "Create a meditation marker — smooth stone pedestal (0.5m tall, 0.3m wide) with carved spiral grooves, subtle blue-cyan emissive glow on top surface" |
| 10 | "Make a futuristic holographic terminal — thin rectangular base with translucent blue emissive screen floating above it at angle, keyboard panel on base" |

---

## Blender: Export (2)

| # | Prompt |
|---|--------|
| 11 | "Export the selected object as FBX to F:\UE Projects\ThoughtSpace\Content\Custom\Meshes\ with filename SM_[Name]. Apply transforms before export" |
| 12 | "Export all visible objects as separate FBX files to F:\UE Projects\ThoughtSpace\Content\Custom\Meshes\[SceneName]\ — name each SM_[ObjectName]" |

---

## Unreal: Layout (4)

| # | Prompt |
|---|--------|
| 13 | "Set up a basic test level — large flat floor (10000x10000), 4 boundary walls, staircase in corner leading to platform, directional lighting" |
| 14 | "Create a small outpost layout — central building (2000x3000 units), two flanking structures (1000x1000), connecting path, 4 light posts along path" |
| 15 | "Set up interior room — 600x800 unit walls, doorway on one wall, ceiling, floor, point light center, spot light above door" |
| 16 | "Place a very tall tower (50000 units tall, 5000 wide) at (80000, 0, 0) — distant Citadel silhouette on horizon" |

---

## Unreal: Blueprints (4)

| # | Prompt |
|---|--------|
| 17 | "Analyze my BP_MainCharacter — list variables, functions, and walk through event graph logic" |
| 18 | "Create BP_Interactable — StaticMesh, Box Collision, InteractionText (string), bCanInteract (bool), OnInteracted event dispatcher" |
| 19 | "Create BP_LorePickup — LoreID (Name), LoreTitle (String), LoreText (Text), AudioClip (Sound Base ref), bIsCollected (Bool), blue point light for glow" |
| 20 | "Create BP_HealthPickup — sphere mesh, collision box, HealAmount (float, 25), bIsActive (bool, true), RespawnTime (float, 10)" |

---

## Combined Pipelines (2 Full Workflows)

### Pipeline A: Meditation Chamber (Blender → UE5)

1. **Blender:** "Create a circular meditation chamber..." (prompt #6)
2. **Blender:** Add props (cushion, crystals, datapads)
3. **Export:** Prompt #12 to `MeditationChamber/`
4. **Unreal:** Import and arrange meshes, add cyan point light, amber spot lights
5. **Unreal:** Post-process volume (bloom, blue grading, AO), Niagara dust motes

### Pipeline B: Outpost Exterior (Blender → UE5)

1. **Blender:** Build prefab building, landing pad, cargo containers, perimeter fence
2. **Blender:** Add weathering, debris, warning stripes, light fixture
3. **Export:** Prompt #12 to `OutpostExterior/`
4. **Unreal:** Import, arrange at (5000, 0, 0), Europa exterior lighting setup

---

## Emergency Fixes

| Problem | Fix |
|---------|-----|
| Blender MCP not responding | Restart Blender, ensure MCP addon is enabled |
| Unreal MCP not responding | Restart UE5 editor, check MCP plugin is loaded |
| Objects at wrong scale | Blender = meters, UE5 = centimeters. Specify units in prompt. |
| Materials missing after export | Re-export FBX with "Embed Textures" option |
| Blueprint creation fails | Check parent class exists. Simplify the request. |

---

## Golden Rules

1. **One object per prompt** for complex pieces — don't try to build everything in one shot
2. **Specify dimensions** in meters (Blender) or units (UE5) — never assume scale
3. **Iterate** — first result is a starting point, refine with follow-ups
4. **Verify in-editor** — MCP can't see your game, you judge quality
5. **Save working prompts** — when something works well, copy it to `04-MCP-WORKFLOWS.md`
