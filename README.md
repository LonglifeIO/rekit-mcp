# ReKit MCP

**Kitbashing, level design, and viewport tools for Unreal Engine 5.5+ via MCP (Model Context Protocol)**

Place, browse, inspect, duplicate, and snap Content Browser meshes through AI — the core operations for modular kitbashing and level design. Auto-discover asset packs, classify meshes, create materials, and capture viewport screenshots so Claude can see and iterate on placement. Built on top of the [flopperam/unreal-engine-mcp](https://github.com/flopperam/unreal-engine-mcp) foundation.

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5%2B-orange.svg)](https://www.unrealengine.com/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

---

## What ReKit Adds

These tools fill the critical gap between "AI can build from basic shapes" and "AI can kitbash with real game assets":

| **Tool** | **What It Does** |
|----------|-----------------|
| `spawn_static_mesh_actor` | Place any Content Browser mesh into the level at a specific transform |
| `list_content_browser_meshes` | Browse available static meshes by folder path and name filter |
| `get_actor_details` | Get mesh path, materials list, and bounding box for any actor |
| `duplicate_actor` | Copy a StaticMeshActor with offset, new rotation, or new scale |
| `snap_actors` | Align one actor's face flush against another using bounding box math |
| `modular_cluster_snap` | Snap multiple modular pieces together in a single operation |
| `take_screenshot` | Capture the active editor viewport as a PNG — returns the image to Claude for visual verification |
| `validate_build` | Capture a validation screenshot after any build sequence for visual verification |
| `set_auto_validate` | Toggle automatic screenshot validation on/off for compound spawn tools |

### Build Validation Loop

Compound spawn tools automatically capture a viewport screenshot after building so Claude can inspect the result and fix issues before reporting. For manual kitbash sequences (multiple `spawn_static_mesh_actor` calls), call `validate_build` at the end.

```bash
# Manual kitbash -> validate at the end
> spawn_static_mesh_actor(name="Wall_01", ...)
> spawn_static_mesh_actor(name="Wall_02", ...)
> validate_build(label="corridor_walls")

# Disable auto-validation if iterating fast
> set_auto_validate(enabled=False)
```

### Example Kitbash Workflow

```bash
# 1. Discover what meshes are available
> list_content_browser_meshes(search_path="/Game/ModularSciFi/", name_filter="Wall")

# 2. Place a wall segment
> spawn_static_mesh_actor(name="Wall_01", mesh_path="/Game/ModularSciFi/.../SM_Wall", location=[0,0,0])

# 3. Check its dimensions
> get_actor_details(name="Wall_01")

# 4. Duplicate it with an offset
> duplicate_actor(source_name="Wall_01", new_name="Wall_02", offset=[500,0,0])

# 5. Snap pieces together
> snap_actors(moving_actor="Wall_02", target_actor="Wall_01", snap_face="right")
```

### Bug Fixes

- **SetActorLabel**: Spawned actors now display their given name in the Outliner instead of generic `StaticMeshActor_N` labels.
- **FlushRenderingCommands**: Screenshot capture no longer hangs — GPU pipeline is flushed before `ReadPixels()`.
- **Viewport fallback**: `take_screenshot` tries `GetActiveViewport()` first, then iterates all viewport clients to find one with valid dimensions.

---

## Full Tool Arsenal (85 tools)

ReKit includes all tools from the original unreal-engine-mcp, plus kitbashing, mesh catalog, material graph, and scene analysis tools.

| **Category** | **Tools** | **Description** |
|--------------|-----------|-----------------|
| **Kitbashing** | `spawn_static_mesh_actor`, `list_content_browser_meshes`, `get_actor_details`, `duplicate_actor`, `snap_actors`, `modular_cluster_snap` | Place, browse, inspect, copy, and align Content Browser meshes |
| **Mesh Catalog** | `search_mesh_catalog`, `get_mesh_categories`, `query_catalog`, `enrich_catalog_bounds`, `add_catalog_semantics`, `extract_archetypes`, `extract_sockets` | Auto-discover and classify meshes from any asset pack by name patterns, dimensions, and sockets |
| **Scene Analysis** | `scan_scene_grid`, `build_adjacency_graph`, `generate_layout_prompt`, `validate_layout` | Analyze placed actors to detect grid size, adjacency, and spatial patterns |
| **Hab Spawning** | `spawn_hab`, `list_hab_variations` | Spawn pre-defined modular hab layouts from variation data |
| **Material Graph** | `create_material`, `add_material_expression`, `set_material_expression_param`, `connect_material_expressions`, `connect_material_to_output`, `compile_material`, `create_landscape_height_material`, `set_landscape_material` | Full material creation pipeline — nodes, parameters, connections, and landscape materials |
| **Material Assignment** | `get_available_materials`, `apply_material_to_actor`, `apply_material_to_blueprint`, `set_mesh_material_color`, `get_actor_material_info`, `set_texture` | Material discovery, assignment, color, and texture control |
| **Viewport & Validation** | `take_screenshot`, `validate_build`, `set_auto_validate` | Capture editor viewport, validate builds visually, toggle auto-validation |
| **Blueprint Scripting** | `add_node`, `connect_nodes`, `delete_node`, `set_node_property`, `create_variable`, `set_blueprint_variable_properties`, `create_function`, `add_function_input`, `add_function_output`, `delete_function`, `rename_function`, `add_event_node` | Complete Blueprint programming with 23+ node types |
| **Blueprint Analysis** | `read_blueprint_content`, `analyze_blueprint_graph`, `get_blueprint_variable_details`, `get_blueprint_function_details` | Deep inspection of Blueprint structure and execution flow |
| **Blueprint System** | `create_blueprint`, `compile_blueprint`, `add_component_to_blueprint`, `set_static_mesh_properties` | Blueprint creation and component management |
| **World Building** | `create_town`, `construct_house`, `construct_mansion`, `create_outpost_compound`, `create_tower`, `create_arch`, `create_staircase` | Procedural architectural structures from basic shapes |
| **Level Design** | `create_maze`, `create_pyramid`, `create_wall`, `create_castle_fortress`, `create_suspension_bridge`, `create_aqueduct` | Procedural level geometry and structures |
| **Actor Management** | `get_actors_in_level`, `find_actors_by_name`, `delete_actor`, `set_actor_transform` | Scene object control and inspection |
| **Physics** | `spawn_physics_blueprint_actor`, `spawn_existing_blueprint_actor`, `set_physics_properties` | Physics simulation and dynamic objects |
| **PCG** | `create_pcg_graph`, `read_pcg_graph`, `add_pcg_node`, `connect_pcg_nodes`, `set_pcg_node_property`, `get_pcg_node_property`, `delete_pcg_node`, `add_pcg_graph_parameter`, `set_pcg_graph_parameter`, `assign_pcg_graph`, `set_pcg_spawner_entries`, `generate_pcg` | Procedural Content Generation graph creation, editing, and execution |

---

## Mesh Catalog System

The catalog system lets ReKit work with **any asset pack** without hardcoded knowledge. It auto-discovers meshes, classifies them by type (floor, wall, door, corner, etc.), and derives grid sizes from piece dimensions.

```bash
# 1. Scan a pack and build a catalog
> search_mesh_catalog(search_path="/Game/SomeAssetPack/", name_filter="")

# 2. Enrich with bounding box data
> enrich_catalog_bounds(catalog_name="someassetpack")

# 3. Auto-classify piece types
> extract_archetypes(catalog_name="someassetpack")

# 4. Find snap points
> extract_sockets(catalog_name="someassetpack")

# 5. Query by type
> query_catalog(catalog_name="someassetpack", archetype="wall")
```

Catalogs are saved to `Python/catalogs/` as JSON and persist across sessions.

---

## Material Graph

Create materials entirely through MCP — from simple solid colors to complex landscape height-blend materials.

```bash
# Simple material
> create_material(name="M_MyMaterial", base_color=[0.5, 0.1, 0.1])

# Landscape height-blend with two texture layers
> create_landscape_height_material(
    material_name="M_Terrain",
    low_texture_path="/Game/Textures/T_Grass",
    high_texture_path="/Game/Textures/T_Rock",
    blend_height=5000,
    blend_sharpness=0.02
  )
```

---

## Setup

### Prerequisites
- **Unreal Engine 5.5+**
- **Python 3.12+**
- **uv** (Python package manager)
- **MCP Client** (Claude Code, Claude Desktop, Cursor, or Windsurf)

### 1. Clone & Install Plugin

```bash
git clone https://github.com/LonglifeIO/rekit-mcp.git
cd rekit-mcp
```

**Add the plugin to your project:**
```bash
# Copy the plugin folder into your project
cp -r UnrealMCP/ YourProject/Plugins/

# Enable in Unreal Editor
# Edit > Plugins > Search "UnrealMCP" > Enable > Restart Editor
```

### 2. Configure Your MCP Client

Add to your MCP configuration file:

**Claude Code** (`.mcp.json` in project root):
```json
{
  "mcpServers": {
    "rekit-mcp": {
      "command": "uv",
      "args": [
        "--directory",
        "/path/to/rekit-mcp/Python",
        "run",
        "unreal_mcp_server_advanced.py"
      ]
    }
  }
}
```

**Claude Desktop** (`~/.config/claude-desktop/mcp.json`), **Cursor** (`.cursor/mcp.json`), **Windsurf** (`~/.config/windsurf/mcp.json`) use the same format.

> **Note:** On Mac (and sometimes Windows), replace `"uv"` with the full path to the uv executable. Find it with `which uv` (Mac) or `where uv` (Windows).

### 3. Start Building

Open your UE5 project with the plugin enabled, then use your MCP client:

```bash
> "List all wall meshes in the ModularSciFi folder"
> "Place a door module at the entrance"
> "Duplicate Wall_01 three times along the X axis with 500cm spacing"
```

> **Setup issues?** See the [Debugging & Troubleshooting Guide](DEBUGGING.md).
>
> **Blueprint programming?** See the [Blueprint Graph Guide](Guides/blueprint-graph-guide.md).

---

## Architecture

```mermaid
graph TB
    A[AI Client<br/>Claude Code / Cursor / Windsurf] -->|MCP Protocol| B[Python Server<br/>unreal_mcp_server_advanced.py]
    B -->|TCP Socket| C[C++ Plugin<br/>UnrealMCP]
    C -->|Native API| D[Unreal Engine 5.5+]

    B --> E[Kitbashing]
    E --> E1[Spawn / Browse / Duplicate / Snap]

    B --> F[Mesh Catalog]
    F --> F1[Scan / Classify / Query]

    B --> G[Material Graph]
    G --> G1[Create / Connect / Compile]

    B --> H[Scene Analysis]
    H --> H1[Grid Detection / Adjacency / Layout]

    B --> I[Blueprint System]
    I --> I1[Nodes / Variables / Functions / Analysis]

    B --> J[Viewport]
    J --> J1[Screenshot / Validate Build]
```

---

## Docs

Reference documentation is in `docs/`:

| **Folder** | **Contents** |
|------------|-------------|
| `docs/guide/` | MCP workflow prompts, cheat sheet, outpost prompt |
| `docs/hab_variations/` | JSON spatial layouts for modular hab spawning |
| `docs/research/` | Build guides, kitbash pipeline research, concept-to-scene pipeline |
| `docs/tools/` | Asset pipeline reference, MCP upgrade plans |
| `docs/prompts/` | PCG and investigation prompt templates |

---

## Credits

ReKit MCP is a fork of [flopperam/unreal-engine-mcp](https://github.com/flopperam/unreal-engine-mcp) with kitbashing, mesh catalog, material graph, and scene analysis tools added by [LonglifeIO](https://github.com/LonglifeIO).

## License
MIT License
