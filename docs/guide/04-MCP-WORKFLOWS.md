# 04 — MCP Workflows & Prompt Library

> Copy-paste prompts for Blender MCP and Unreal MCP. Combined pipelines for full asset-to-level workflows.

---

## MCP Setup & Configuration

### claude_desktop_config.json Template

```json
{
  "mcpServers": {
    "blender": {
      "command": "uvx",
      "args": ["blender-mcp"]
    },
    "unreal": {
      "command": "uvx",
      "args": ["unreal-mcp"]
    }
  }
}
```

**Location:** `%APPDATA%\Claude\claude_desktop_config.json` (Windows)

### Connection Verification

Before starting work, verify both servers are connected:

```
Blender: "Get the current scene info from Blender"
Unreal:  "Get the list of actors in the current level"
Both:    "Check connection to Blender MCP and Unreal MCP — report status of each"
```

---

## Blender MCP Prompts

### Environment Blockouts

**Sci-fi Corridor:**
> "Build a modular sci-fi corridor section — 4m wide, 3m tall, 6m long, with recessed lighting panels in the ceiling, conduit pipes along the walls, and a metal grate floor. Use a dark gunmetal material with orange accent lights"

**Outpost Interior:**
> "Create a small outpost room — 6m x 8m, metal walls with panel seams, a workstation desk against one wall, overhead strip lighting, exposed ceiling pipes, and a window frame on one wall looking outward. Dark industrial materials with blue accent lighting"

**Mining Camp:**
> "Build an outdoor mining camp setup — a tarp shelter (3m x 4m, angled), a heat lamp prop, a drill rig base (cylinder + cone), 4 scattered crate props, and a comms mast (thin cylinder, 5m tall with antenna). Materials: weathered metal, dirty tarp fabric, orange hazard markings"

**Europa Surface:**
> "Create a rocky ice terrain section — 10m x 10m base with irregular surface, ice-like blue-white material with subsurface scattering feel, scattered rock formations (3-5 pieces, dark gray), and two small bioluminescent plant props (glowing cyan stems with bulb tops)"

### Props & Objects

**Supply Crate:**
> "Create a sci-fi supply crate — 1m cube, dark metal material with scratches, glowing blue trim along the edges, a handle on top, and hazard stripes on one side"

**Holographic Terminal:**
> "Make a futuristic holographic terminal — a thin rectangular base with a translucent blue emissive screen floating above it at an angle, with a keyboard panel on the base"

**Meditation Node:**
> "Create a meditation marker prop — a smooth stone pedestal (0.5m tall, 0.3m wide) with carved spiral grooves. Top surface has a subtle blue-cyan emissive glow. Base is dark stone material, grooves emit soft light"

**Lore Datapad:**
> "Build a handheld datapad — 20cm x 12cm x 1.5cm rectangular slab, rounded corners, dark frame with a bright emissive screen surface (teal color), small button indentation on the side"

**Warp Scar Formation:**
> "Create an alien mineral formation — organic crystalline cluster, 1m tall, 5-6 pointed crystals growing from a rough base. Material: dark purple with bright cyan emissive veins running through it. Should look geological but unnatural"

### Materials

**Warp Glow Material:**
> "Create a glowing warp material — base color deep blue (#1D7880), strong emissive in cyan (#3BB8C6) with pulsing intensity. Roughness 0.1 for glossy wet look"

**Corporate Metal:**
> "Make a brushed metal material — light gray base, low roughness (0.2), subtle directional scratches, clean and sterile feeling. Add small panel seam details"

**Worn Industrial:**
> "Create a worn industrial metal material — dark gray base, medium roughness (0.5), rust patches at edges, scratches, and oil stain spots. Should feel used and neglected"

### Poly Haven Integration

> "Search Poly Haven for sci-fi or metal materials and show me what's available"

> "Download a concrete wall texture from Poly Haven and apply it to the floor of the current scene"

> "Find an HDRI of a night sky or space environment from Poly Haven and set it as the scene background"

### Export Pipeline

**Single Asset Export:**
> "Export the selected object as FBX to F:\UE Projects\ThoughtSpace\Content\Custom\Meshes\ with the filename SM_[AssetName]. Apply transforms before export"

**Scene Export:**
> "Export all visible objects in the scene as separate FBX files to F:\UE Projects\ThoughtSpace\Content\Custom\Meshes\[SceneName]\ — name each file SM_[ObjectName]"

---

## Unreal MCP Prompts

### Level Layout

**Test Level:**
> "Set up a basic test level — create a large flat wall as a floor (10000x10000), add 4 walls around the edges, place a staircase in one corner leading to a platform, and add basic directional lighting"

**Outpost Layout:**
> "Create a small outpost layout — a central building footprint (2000x3000 units), two smaller structures flanking it (1000x1000 each), a path connecting them, and 4 light post actors along the path. Arrange on flat terrain"

**Interior Room:**
> "Set up an interior room — walls forming a 600x800 unit room, a doorway opening on one wall, ceiling piece, floor piece. Add a point light in the center and a spot light above the doorway"

**Citadel Silhouette:**
> "Place a very tall tower structure (50000 units tall, 5000 units wide) at coordinates (80000, 0, 0) — this represents the distant Citadel on the horizon. It should be visible from the player start but very far away"

### Blueprint Creation & Analysis

**Analyze Existing:**
> "Analyze my BP_MainCharacter Blueprint — tell me what variables it has, what functions are defined, and walk me through the event graph logic"

**Health Pickup:**
> "Create a Blueprint called BP_HealthPickup with a StaticMesh component (sphere), a collision box component, and variables for HealAmount (float, default 25), bIsActive (boolean, default true), and RespawnTime (float, default 10)"

**Interactable Base:**
> "Create a Blueprint called BP_Interactable with a StaticMesh component, a Box Collision component for interaction range, variables for InteractionText (string, default 'Interact'), bCanInteract (boolean, default true), and an event dispatcher OnInteracted"

**Lore Pickup:**
> "Create a Blueprint called BP_LorePickup extending BP_Interactable. Add variables: LoreID (Name), LoreTitle (String), LoreText (Text), AudioClip (Sound Base reference), bIsCollected (Boolean, default false). Add a point light component with blue color for the glow effect"

### Materials in Unreal

**Warp Effect Material:**
> "Create a material called M_WarpGlow — emissive material with base color parameter (default cyan #3BB8C6), emissive strength parameter (default 5.0), and a pulsing effect using a sine node on Time to oscillate emissive intensity"

**Post-Process for Meditation:**
> "Create a post-process material called M_PP_Meditation — adds vignette darkening, slight desaturation, and a subtle blue color tint. Parameters for intensity (0-1) so it can be blended in/out from Blueprint"

### Lighting

**Moody Outpost Lighting:**
> "Set up atmospheric lighting for a sci-fi outpost interior — directional light (low angle, warm), skylight (low intensity, cool blue), 3 point lights (orange/amber, 1000 units radius, low intensity) for practical light sources. Enable Lumen global illumination"

**Europa Exterior:**
> "Set up exterior lighting for Europa's surface — weak directional light (cold blue-white, like distant sun), exponential height fog (dense, blue-gray), skylight (very low, dark blue). Post-process: high contrast, slight blue tint, film grain"

---

## Combined Pipelines

### Full Workflow: "Meditation Chamber"

This demonstrates a complete Blender → UE5 pipeline:

**Step 1 — Blender Blockout:**
> "Create a meditation chamber room — circular room, 5m diameter, 4m ceiling height. Central raised platform (1m diameter, 0.3m high) with carved spiral patterns. 4 pillar-like formations around the platform at cardinal directions. Walls have recessed alcoves. Materials: dark stone base, cyan emissive for the spiral patterns and alcove edges"

**Step 2 — Blender Props:**
> "Add to the meditation chamber: a kneeling cushion on the central platform, 4 floating crystal shards (one near each pillar, hovering 1.5m high), and wall-mounted datapads in 2 of the alcoves. Crystals should have bright cyan emissive material"

**Step 3 — Export:**
> "Export all objects in the meditation chamber as separate FBX files to F:\UE Projects\ThoughtSpace\Content\Custom\Meshes\MeditationChamber\"

**Step 4 — Unreal Layout:**
> "Import the meditation chamber meshes and arrange them to recreate the room layout. Place the room at the origin. Add a point light at the center (cyan, radius 500, intensity 3) and spot lights in each alcove (warm amber, narrow cone)"

**Step 5 — Unreal Polish:**
> "Add a post-process volume to the meditation chamber area — increase bloom, add slight blue color grading, enable ambient occlusion. Add a Niagara particle system at the center for floating dust motes"

### Full Workflow: "Outpost Exterior"

**Step 1 — Blender Environment:**
> "Create an outpost exterior scene — a prefab building (rectangular, 8m x 5m x 3m, flat roof with antenna), a landing pad (circular, 6m diameter, metal grate), 3 cargo containers (2m x 1m x 1m, stacked), and a perimeter fence (4 posts connected by horizontal bars). All in worn industrial metal"

**Step 2 — Blender Detail:**
> "Add weathering details to the outpost — scratch the building walls, add debris at the base (small rock clusters), put warning decal-like stripes on the landing pad edges, and add a small light fixture above the building entrance"

**Step 3 — Export & Import:**
> "Export the complete outpost scene as separate FBX files, then import into UE5 and arrange at coordinates (5000, 0, 0) in the level. Set up Europa-style exterior lighting"

---

## AI Verification Rules

**Golden rules for working with MCP + Claude:**

1. **Claude designs the system, you implement and verify.** Never trust a Blueprint node chain without testing it in-editor.
2. **MCP generates geometry, you judge quality.** Always inspect Blender output before exporting — check scale, normals, material assignments.
3. **Prompts are iterative.** First result is a starting point. Refine with follow-up prompts.
4. **Keep human in the loop.** MCP can't see your game. You decide if it fits the aesthetic.
5. **Document what works.** When a prompt produces good results, save it here or in `docs/guide/MCP-CHEAT-SHEET.md`.

---

## Troubleshooting

### Blender MCP

| Issue | Fix |
|-------|-----|
| "Connection refused" | Ensure Blender is open with the MCP addon enabled |
| Objects at wrong scale | Specify dimensions in meters in the prompt. Blender default is meters. |
| Materials not exporting | Export as FBX with "Embed Textures" or export materials separately |
| Complex geometry fails | Break into simpler sub-prompts. One object per prompt for complex pieces. |

### Unreal MCP

| Issue | Fix |
|-------|-----|
| "No actors found" | Ensure UE5 editor is open with the MCP plugin loaded |
| Blueprint creation fails | Check that the parent class exists. Use simpler structures first. |
| Wrong coordinates | UE5 uses centimeters. 1m Blender = 100 units UE5. Specify units in prompt. |
| Level not updating | Run "Get actors in level" to refresh the connection state |

### General

- If MCP seems unresponsive, restart the MCP server process
- For complex scenes, build incrementally (base → details → polish)
- Always verify scale: Blender meters → UE5 centimeters (x100)
- Name assets consistently: SM_ for static meshes, M_ for materials, T_ for textures

---

## Cross-References

- `docs/guide/MCP-CHEAT-SHEET.md` — Top 20 prompts quick reference
- `docs/guide/03-UE5-ARCHITECTURE.md` — Technical architecture context
- `docs/guide/08-TOOL-STACK.md` — MCP server configuration details
