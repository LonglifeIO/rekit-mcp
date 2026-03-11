# Rekit-MCP Codebase Audit

**Repo:** `F:/UE Projects/rekit-mcp/` (fork of flopperam's, GitHub: `LonglifeIO/rekit-mcp`)
**Server:** `Python/unreal_mcp_server_advanced.py` — FastMCP framework, 73 tools
**Architecture:** Claude → FastMCP (stdio) → Python server → TCP socket (127.0.0.1:55557) → UE5 C++ plugin

---

## 1. Current Tool Inventory (73 tools)

### Actor Management (4 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `get_actors_in_level` | `random_string=""` | Dict with all actors | Lists every actor in current level |
| `find_actors_by_name` | `pattern: str` | Matching actors | Searches actors by name pattern |
| `delete_actor` | `name: str` | Success status | Deletes actor by exact name |
| `set_actor_transform` | `name, location?, rotation?, scale?` | Success status | Sets position/rotation/scale |

### Kitbashing (7 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `spawn_static_mesh_actor` | `name, mesh_path, location, rotation, scale` | Success + actor info | Spawns single static mesh actor |
| `list_content_browser_meshes` | `search_path, name_filter, max_results` | Mesh list | Scans asset registry (SLOW: 300s timeout) |
| `get_actor_details` | `name: str` | Mesh path, materials, bounding box | Essential for measuring/snapping |
| `duplicate_actor` | `source_name, new_name, location/offset, rotation, scale` | Cloned actor info | Clones actor with new name |
| `snap_actors` | `moving_actor, target_actor, snap_face, offset` | Success | Aligns actor flush against another |
| `take_screenshot` | `filename=""` | Screenshot image | Captures viewport as PNG |
| `search_mesh_catalog` | `query, category, catalog` | Mesh list with paths | Searches pre-built JSON catalog (fast, no UE5 query) |

### Blueprint Creation (6 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `create_blueprint` | `name, parent_class` | Blueprint path | Creates new Blueprint class |
| `add_component_to_blueprint` | `bp_name, comp_type, comp_name, props, loc, rot, scale` | Component details | Adds component to Blueprint |
| `set_static_mesh_properties` | `bp_name, comp_name, static_mesh` | Success | Sets mesh on StaticMeshComponent |
| `set_physics_properties` | `bp_name, comp_name, simulate, gravity, mass, damping...` | Success | Configures physics |
| `compile_blueprint` | `bp_name` | Success + messages | Compiles Blueprint |
| `spawn_physics_blueprint_actor` | `name, mesh, loc, rot, scale, color, physics params` | Actor info | One-call physics actor creation |

### Blueprint Inspection (4 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `read_blueprint_content` | `bp_path, flags for graphs/funcs/vars/comps/interfaces` | Full BP structure | Reads complete Blueprint |
| `analyze_blueprint_graph` | `bp_path, graph_name, detail flags` | Nodes + connections + flow | Detailed graph analysis |
| `get_blueprint_variable_details` | `bp_path, variable_name?` | Variable info | Variable types, defaults, usage |
| `get_blueprint_function_details` | `bp_path, function_name?, include_graph` | Function details | Signature, params, return type |

### Blueprint Node Graph (9 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `add_node` | `bp_name, node_type, pos, message, event_type, var_name...` | Node ID + position | Creates Blueprint nodes (22 types) |
| `connect_nodes` | `bp_name, source_node, source_pin, target_node, target_pin` | Connection details | Links pins between nodes |
| `create_variable` | `bp_name, var_name, var_type, default, is_public, tooltip` | Variable info | Creates BP variable |
| `set_blueprint_variable_properties` | `bp_name, var_name, 25+ property fields` | Updated props | Extensive property modification |
| `add_event_node` | `bp_name, event_name, pos` | Node ID | Creates event nodes |
| `delete_node` | `bp_name, node_id, function_name?` | Success | Removes node from graph |
| `set_node_property` | `bp_name, node_id, property_name/value, action, pin_type...` | Operation details | Semantic node editing (add_pin, set_enum, etc.) |
| `create_function` | `bp_name, function_name, return_type` | Function info | Creates custom function |
| `rename_function` | `bp_name, old_name, new_name` | Success | Renames function |

### Function I/O (3 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `add_function_input` | `bp_name, func_name, param_name, param_type, is_array` | Param details | Adds input parameter |
| `add_function_output` | `bp_name, func_name, param_name, param_type, is_array` | Param details | Adds return parameter |
| `delete_function` | `bp_name, function_name` | Success | Removes function |

### Procedural Generation / Compositions (11 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `create_pyramid` | `base_size, block_size, location, mesh, prefix` | Actor list | Spawns pyramid from cubes |
| `create_wall` | `length, height, block_size, location, orientation, mesh` | Actor list | Creates wall along axis |
| `create_tower` | `height, base_size, block_size, location, style, mesh` | Actor list | Builds tower (4 styles) |
| `create_staircase` | `steps, step_size, location, mesh` | Actor list | Creates staircase |
| `construct_house` | `width, depth, height, location, style, mesh` | Actor list | Builds house structure |
| `construct_mansion` | `scale, location, prefix` | Actor count | Creates mansion (4 scales) |
| `create_arch` | `radius, segments, location, mesh` | Block list | Semicircle arch |
| `create_maze` | `rows, cols, cell_size, wall_height, location` | Maze + solution path | Solvable maze via recursive backtracking |
| `spawn_existing_blueprint_actor` | `bp_name, actor_name, location, rotation` | Actor info | Spawns Blueprint instance |
| `create_suspension_bridge` | `span, width, height, sag, meshes, dry_run` | Actor count + metrics | Realistic bridge with parabolic cables |
| `create_aqueduct` | `arches, radius, tiers, meshes, dry_run` | Actor count + metrics | Multi-tier aqueduct |

### Material Tools (5 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `get_available_materials` | `search_path, include_engine` | Material paths | Lists all materials |
| `apply_material_to_actor` | `actor_name, material_path, slot` | Success | Sets material on actor |
| `apply_material_to_blueprint` | `bp_name, comp_name, material_path, slot` | Success | Sets material on BP component |
| `get_actor_material_info` | `actor_name` | Material slots + materials | Current material info |
| `set_mesh_material_color` | `bp_name, comp_name, color, material, param, slot` | Success | Sets material parameter color |

### Large-Scale Compositions (5 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `create_town` | `size, density, style, infrastructure, location` | Actor count + layout | Complete town (300s timeout) |
| `create_castle_fortress` | `size, location, style, siege, village` | Actor count (300s) | Castle with walls, towers, village |
| `create_outpost_compound` | `location, size, flags for components` | Actor count | Sci-fi outpost |
| `create_suspension_bridge` | (see above) | | |
| `create_aqueduct` | (see above) | | |

### PCG Graph Tools (11 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `create_pcg_graph` | `graph_name, path` | Graph path | Creates new PCG asset |
| `read_pcg_graph` | `graph_path` | Nodes + connections + params | Reads PCG structure |
| `add_pcg_node` | `graph_path, node_type, pos` | Node ID + pins | Adds node (18 types) |
| `connect_pcg_nodes` | `graph_path, from/to node, from/to pin` | Connection info | Links PCG nodes |
| `set_pcg_node_property` | `graph_path, node_id, prop_name, prop_value` | Success | Sets PCG node property |
| `get_pcg_node_property` | `graph_path, node_id, prop_name` | Property value + type | Reads PCG node property |
| `delete_pcg_node` | `graph_path, node_id` | Success | Removes PCG node |
| `add_pcg_graph_parameter` | `graph_path, param_name, param_type` | Param details | Adds user parameter (11 types) |
| `set_pcg_graph_parameter` | `graph_path, param_name, default_value` | Success | Sets parameter default |
| `assign_pcg_graph` | `graph_path, actor_name/bp_name` | Assignment info | Assigns PCG to actor/BP |
| `set_pcg_spawner_entries` | `graph_path, node_id, entries[]` | Entry count | Sets mesh list on spawner |
| `generate_pcg` | `actor_name` | Success | Force-generates PCG |

### Hab Variation Tools (4 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `list_hab_variations` | `variations_dir?` | Variation list with counts | Scans variation_*.json files |
| `spawn_hab` | `variation, location, rotation, prefix, dir, delay, screenshot` | Spawn summary + optional screenshot | Spawns complete hab from JSON |
| `validate_build` | `label` | Screenshot image | Manual validation screenshot |
| `set_auto_validate` | `enabled: bool` | Validate state | Toggles auto-screenshot |

### Mesh Catalog Tools (2 tools)
| Tool | Parameters | Returns | Description |
|------|-----------|---------|-------------|
| `search_mesh_catalog` | `query, category, catalog` | Mesh list with full paths | Fast JSON search (no UE5 query) |
| `get_mesh_categories` | `catalog` | Categories + counts + descriptions | Lists all categories |

---

## 2. Current Schemas & Data Structures

### Mesh Catalog (modularscifi_meshes.json)
```json
{
  "version": "1.0",
  "asset_pack": "ModularSciFi",
  "base_path": "/Game/ModularSciFi/StaticMeshes",
  "total_meshes": 320,
  "category_counts": { "modular_building": 15, ... },
  "category_descriptions": { "modular_building": "Grid-snapping structural pieces (500cm)...", ... },
  "meshes": [
    {
      "name": "SM_Modular_Wall_1_Merged",
      "path": "/Game/.../SM_Modular_Wall_1_Merged.SM_Modular_Wall_1_Merged",
      "base_path": "/Game/.../SM_Modular_Wall_1_Merged",
      "category": "modular_building",
      "tags": ["grid_500cm", "snap", "structure"],
      "folder": "HardSurfaceEnvironment",
      "subfolder": "Modular_Wall_1",
      "is_indoor_variant": false,
      "has_indoor_pair": false,
      "bounds_extent": [250.0, 250.0, 250.0],  // or null
      "size_cm": [500.0, 500.0, 500.0],         // or null
      "materials": ["MI_Metal1", ...]            // some entries have this
    }
  ]
}
```

**Current gaps in catalog:** No sockets, no adjacency rules, no semantic descriptions, no pivot offsets, no spawn weights. Only identity + spatial (partial) + basic tags.

### Hab Variation JSON
```json
{
  "name": "2x1_Basic_V1",
  "description": "Simple 2x1 rectangular hab...",
  "grid_rotation_deg": 0,
  "cell_size_cm": 500,
  "origin_world": {"x": -295000, "y": -133000, "z": -25700},
  "cells": [{"grid": [0, 0], "label": "east"}, ...],
  "pieces": [
    {
      "index": 1,
      "type": "floor",
      "mesh": "SM_Modular_Merged",
      "rel": [0, 0, 0],
      "yaw": 0,
      "label": "East floor"
    }
  ],
  "mesh_paths": {
    "SM_Modular_Merged": "/Game/.../Modular_Floor/SM_Modular_Merged"
  },
  "notes": ["All positions are in LOCAL space relative to the hab origin..."]
}
```

### Actor Spawn Parameters (internal)
```python
params = {
    "name": "ActorName",
    "type": "StaticMeshActor",
    "location": [x, y, z],
    "rotation": [pitch, yaw, roll],
    "static_mesh": "/Game/full/path.AssetName",
}
```

### Actor Details Response (from get_actor_details)
```json
{
  "name": "ActorName",
  "mesh": "/Game/.../SM_Mesh.SM_Mesh",
  "materials": ["MI_Material1", ...],
  "bounding_box": {
    "origin": [x, y, z],
    "extent": [ex, ey, ez],
    "min": [x1, y1, z1],
    "max": [x2, y2, z2]
  }
}
```

---

## 3. Gap Analysis

| Capability (from research) | Have it? | Closest Tool | What's Missing |
|---------------------------|----------|-------------|----------------|
| **Enumerate all level actors** | **Yes** | `get_actors_in_level` | Works, returns name+class+mesh+transform |
| **Get actor transform + bounds** | **Yes** | `get_actor_details` | Returns mesh, materials, bounding box |
| **Spawn static mesh** | **Yes** | `spawn_static_mesh_actor` | Fully functional |
| **Search mesh catalog (fast)** | **Yes** | `search_mesh_catalog` | Searches offline JSON, no UE5 query |
| **Spawn complete hab from JSON** | **Yes** | `spawn_hab` | Handles rotation, sequential spawning |
| **Screenshot for validation** | **Yes** | `take_screenshot` | Returns PNG |
| **Mesh bounds measurement** | **Partial** | `get_actor_details` | Works per-actor; catalog has nulls for many meshes |
| **Grid detection from scene** | **No** | none | Need: GCD of position deltas between structural meshes |
| **Adjacency graph construction** | **No** | none | Need: analyze which pieces neighbor which in a loaded level |
| **Spatial clustering** | **No** | none | Need: connected components or DBSCAN on adjacency graph |
| **Archetype classification** | **No** | none | Need: heuristics (aspect ratio, opening count) |
| **Template storage/retrieval** | **Partial** | hab variation JSONs | Hab-specific format; needs generalization to arbitrary templates |
| **Socket/connectivity metadata** | **No** | none | Catalog has no sockets, no connection rules |
| **WFC adjacency rules** | **No** | none | No WFC integration at all |
| **WFC validation/repair** | **No** | none | No constraint solver |
| **Text/image → layout generation** | **No** | none | Would be LLM prompting + catalog context |
| **Layout manifest → spawn** | **Partial** | `spawn_hab` | Works for hab JSONs; needs generalization |
| **Save scene selection as manifest** | **No** | none | Need: select actors → export as reusable template JSON |
| **Layout diff** | **No** | none | Need: compare two layouts |
| **Semantic annotations per mesh** | **No** | none | Catalog has category+tags but no descriptions/function |
| **Pivot offset per mesh** | **No** | none | Not in catalog; known empirically for hab pieces |
| **Spawn weight / frequency** | **No** | none | Not tracked |
| **Multi-catalog management** | **Partial** | `search_mesh_catalog(catalog=...)` | Supports catalog param but only one catalog exists |
| **PCG parameter control** | **Yes** | PCG tools (11) | Full PCG graph creation/editing/generation |
| **Measure all mesh bounds** | **Partial** | `measure_bounds.py` exists | Script exists but many catalog entries still have null bounds |

---

## 4. Extension Point Map

### Where new tools should live

```
Python/
├── helpers/
│   ├── scene_scanner.py          # NEW: Grid detection, adjacency graph, clustering
│   ├── catalog_enricher.py       # NEW: Add sockets, semantics, pivot data to catalog
│   ├── layout_generator.py       # NEW: Text/image → layout JSON via LLM prompting
│   ├── constraint_solver.py      # NEW: WFC validation/repair layer
│   ├── template_manager.py       # NEW: Save/load/query archetype templates
│   ├── hab_spawner.py            # EXISTING: Generalize to layout_spawner.py
│   ├── actor_name_manager.py     # EXISTING: Reuse safe_spawn_actor
│   └── ...
├── catalogs/
│   ├── modularscifi_meshes.json  # EXISTING: Enrich with sockets/semantics
│   ├── modularscifi_sockets.json # NEW: Socket definitions extracted from scenes
│   ├── modularscifi_archetypes.json # NEW: Room/corridor templates from demo scene
│   └── <other_pack>_meshes.json  # NEW: Per-pack catalogs
├── build_catalog.py              # EXISTING: Extend to include sockets/semantics
└── unreal_mcp_server_advanced.py # EXISTING: Register new tool functions
```

### Patterns to follow

1. **Helper module pattern:** Each domain gets a `helpers/<name>.py` module with pure functions. The main server imports and wraps them as `@mcp.tool()` functions with docstrings.
2. **Safe spawn pattern:** All spawning goes through `safe_spawn_actor` from `actor_name_manager.py` for unique naming and crash prevention.
3. **Sequential MCP calls:** NEVER parallel MCP calls to UE5. All scene queries and spawns must be sequential.
4. **Catalog-first pattern:** `search_mesh_catalog` queries offline JSON without UE5 round-trips. New tools should prefer catalog lookups over live UE5 queries where possible.
5. **Validation screenshot pattern:** Compound spawn operations should optionally capture a screenshot for visual verification.

### Shared utilities to reuse

- `actor_name_manager.safe_spawn_actor()` — All actor creation
- `actor_name_manager.safe_delete_actor()` — Actor removal with tracking
- `actor_name_manager.get_unique_actor_name()` — Name uniqueness
- `hab_spawner._rotate_piece()` — 2D rotation math for grid-aligned placement
- `hab_spawner._resolve_mesh_path()` — Short name → full UE5 path with .AssetName
- `build_catalog.CATEGORY_MAP` — Folder→category mapping for ModularSciFi

---

## 5. Proposed New Tools

### Phase A — Mesh Catalog & Schema

#### `enrich_catalog`
- **Parameters:** `catalog: str = "modularscifi_meshes"`, `measure_bounds: bool = True`
- **Returns:** `{success, enriched_count, catalog_path}`
- **Description:** Iterates catalog meshes, spawns each at origin to measure bounds via `get_actor_details`, then deletes. Fills in `bounds_extent`, `size_cm`, and `pivot_offset_cm` for all entries with null values. Writes enriched catalog back to JSON.
- **Depends on:** `spawn_static_mesh_actor`, `get_actor_details`, `delete_actor`

#### `add_catalog_semantics`
- **Parameters:** `catalog: str`, `mesh_name: str`, `description: str = ""`, `function: str = ""`, `style_tags: list = []`, `spawn_weight: float = 1.0`
- **Returns:** `{success, updated_entry}`
- **Description:** Adds or updates semantic metadata for a single catalog mesh entry. Enables incremental human annotation.
- **Depends on:** Catalog JSON file I/O

#### `query_catalog`
- **Parameters:** `catalog: str`, `query: str = ""`, `category: str = ""`, `tags: list = []`, `has_sockets: bool = False`, `min_size: list = None`, `max_size: list = None`
- **Returns:** `{matches: [{name, path, category, description, size_cm, sockets}]}`
- **Description:** Enhanced catalog search with filtering by tags, size ranges, socket availability. Superset of existing `search_mesh_catalog`.
- **Depends on:** Catalog JSON

### Phase B — Demo Scene Scanner

#### `scan_scene_grid`
- **Parameters:** `mesh_filter: str = ""`, `category_filter: str = "modular_building"`, `catalog: str = "modularscifi_meshes"`
- **Returns:** `{grid_size_cm, actor_count, structural_count, grid_positions: [{name, mesh, grid_pos, rotation_deg}]}`
- **Description:** Enumerates actors in current level, filters to structural kit pieces, detects grid via GCD of position deltas, snaps positions, quantizes rotations. Returns grid-normalized actor data.
- **Depends on:** `get_actors_in_level`, `get_actor_details`, catalog JSON

#### `build_adjacency_graph`
- **Parameters:** `grid_data: dict = None`, `max_neighbor_distance: float = 1.5`
- **Returns:** `{nodes: [{id, mesh, pos, rot}], edges: [{from, to, direction, relative_transform}], stats}`
- **Description:** Takes grid-normalized scene data (or scans live if no data provided), builds adjacency graph where edges connect pieces within max_neighbor_distance × grid_size that share a connection face.
- **Depends on:** `scan_scene_grid` (or accepts pre-computed data)

#### `extract_archetypes`
- **Parameters:** `adjacency_graph: dict = None`, `method: str = "connected_components"`, `min_pieces: int = 3`
- **Returns:** `{archetypes: [{id, classification, piece_count, bounding_box, openings, pieces_relative: [...]}]}`
- **Description:** Clusters adjacency graph into spatial groups. Classifies each by aspect ratio and opening count (corridor, room, junction, dead-end, L-corner). Stores as relative-transform templates.
- **Depends on:** `build_adjacency_graph` (or accepts pre-computed data)

#### `extract_sockets`
- **Parameters:** `adjacency_graph: dict = None`, `catalog: str = "modularscifi_meshes"`
- **Returns:** `{socket_definitions: [{mesh, sockets: [{side, offset, type, compatible_with}]}]}`
- **Description:** Analyzes adjacency graph to infer socket types from observed connections. If wall A's east face always connects to wall B's west face, infers `wall_continuation` socket type. Writes socket data to catalog.
- **Depends on:** `build_adjacency_graph`, catalog JSON

### Phase C — Layout Generation

#### `generate_layout`
- **Parameters:** `description: str`, `catalog: str = "modularscifi_meshes"`, `max_grid: list = [10, 10]`, `style_tags: list = []`, `examples: int = 2`
- **Returns:** `{layout: {grid_size, cells: [{pos, piece_id, rotation}]}, reasoning: str}`
- **Description:** Sends text description + catalog context + few-shot examples to LLM. Returns grid-based layout JSON. Post-processes for grid alignment. NOTE: This tool is a prompt constructor + response parser; the actual LLM call happens through the MCP client (Claude).
- **Depends on:** Catalog JSON, archetype templates for few-shot examples

#### `validate_layout`
- **Parameters:** `layout: dict`, `catalog: str = "modularscifi_meshes"`
- **Returns:** `{valid: bool, violations: [{cell, issue, suggestion}], coverage: float}`
- **Description:** Checks layout against catalog constraints: socket compatibility, grid alignment, no overlaps, floor coverage, required support. Reports violations with repair suggestions.
- **Depends on:** Catalog JSON with sockets

### Phase D — Constraint Solver

#### `repair_layout`
- **Parameters:** `layout: dict`, `catalog: str`, `max_iterations: int = 100`
- **Returns:** `{repaired_layout: dict, changes_made: int, remaining_violations: int}`
- **Description:** Applies WFC-like constraint propagation to fix violations in a proposed layout. Preserves valid placements, re-solves violated cells. Uses adjacency rules from catalog.
- **Depends on:** Catalog with socket/adjacency rules

### Phase E — Spawn & Iterate

#### `spawn_layout`
- **Parameters:** `layout: dict, location: list, rotation: float = 0, name_prefix: str = "Layout", catalog: str`, `spawn_delay: float = 0.1, screenshot: bool = True`
- **Returns:** `{success, spawned_count, failed_count, spawned: [...], screenshot?}`
- **Description:** Generalized version of `spawn_hab`. Spawns any layout manifest (not just hab variations). Handles rotation math, sequential spawning, validation screenshots.
- **Depends on:** `safe_spawn_actor`, catalog JSON, `take_screenshot`

#### `save_scene_as_template`
- **Parameters:** `actor_names: list = []`, `name_filter: str = ""`, `template_name: str`, `catalog: str`
- **Returns:** `{template: {name, pieces_relative, archetype, sockets_at_boundary}}`
- **Description:** Selects actors (by name list or filter), computes relative transforms from centroid, matches meshes to catalog entries, saves as reusable template JSON.
- **Depends on:** `get_actor_details`, catalog JSON

#### `diff_layouts`
- **Parameters:** `layout_a: dict`, `layout_b: dict`
- **Returns:** `{added: [...], removed: [...], moved: [...], unchanged: int}`
- **Description:** Compares two layout manifests. Reports pieces added, removed, or relocated. Useful for tracking human tweaks after AI-proposed layout.
- **Depends on:** Layout JSON comparison

---

## 6. Proposed Schema Additions

### Enhanced Mesh Catalog Entry (v2)
Extends existing catalog entry with socket/semantic/adjacency fields:

```json
{
  "name": "SM_Modular_Wall_1_Merged",
  "path": "/Game/.../SM_Modular_Wall_1_Merged.SM_Modular_Wall_1_Merged",
  "category": "modular_building",
  "tags": ["grid_500cm", "snap", "structure"],

  "spatial": {
    "bounds_extent": [250.0, 250.0, 250.0],
    "size_cm": [500.0, 500.0, 500.0],
    "pivot_offset_cm": [0, 0, 0],
    "grid_footprint_cells": [1, 1, 1],
    "cell_size_cm": 500,
    "rotations_deg": [0, 90, 180, 270]
  },

  "sockets": [
    {
      "side": "south",
      "offset": 0,
      "type": "wall_continuation",
      "compatible_with": ["wall_continuation", "corner_outer", "door_frame"]
    },
    {
      "side": "north",
      "offset": 0,
      "type": "wall_continuation",
      "compatible_with": ["wall_continuation", "corner_outer", "door_frame"]
    }
  ],

  "semantics": {
    "display_name": "Standard Wall",
    "description": "Straight wall panel with ribbed metal surface and conduit runs",
    "function": "Enclosure wall for rooms and corridors",
    "style_tags": ["industrial", "panelled"],
    "spawn_weight": 1.0
  },

  "adjacency_stats": {
    "frequency_in_scenes": 0.21,
    "typical_neighbors": ["SM_Modular_Corner_Merged", "SM_Modular_DoorModule_Merged"],
    "observed_in_archetypes": ["corridor_straight", "room_rectangular"]
  }
}
```

### Layout Manifest
Generalized version of hab variation JSON:

```json
{
  "name": "L_corridor_with_sideroom",
  "description": "L-shaped corridor with one side room",
  "source": "ai_generated",
  "catalog": "modularscifi_meshes",
  "grid": {
    "cell_size_cm": 500,
    "bounds_cells": [6, 4]
  },
  "pieces": [
    {
      "id": "piece_001",
      "mesh": "SM_Modular_Merged",
      "catalog_entry": "SM_Modular_Merged",
      "rel": [0, 0, 0],
      "yaw": 0,
      "type": "floor",
      "label": "Corridor floor 1"
    }
  ],
  "mesh_paths": {
    "SM_Modular_Merged": "/Game/.../Modular_Floor/SM_Modular_Merged"
  },
  "metadata": {
    "generated_by": "generate_layout",
    "validated": true,
    "violations": 0,
    "created": "2026-02-20T12:00:00"
  }
}
```

### Archetype Template
```json
{
  "id": "room_2x3_single_door",
  "classification": "room",
  "grid_size_cells": [2, 3],
  "piece_count": 22,
  "openings": [
    {"side": "south", "offset": 0, "socket_type": "doorway"}
  ],
  "pieces_relative": [
    {"mesh": "SM_Modular_Merged", "rel": [0, 0, 0], "yaw": 0, "type": "floor"},
    {"mesh": "SM_Modular_Wall_1_Merged", "rel": [0, 0, 0], "yaw": -90, "type": "wall"}
  ],
  "source": {
    "demo_level": "ModularSciFi_Demo",
    "centroid_world": [1000, 2000, 0],
    "confidence": 0.95
  },
  "style_tags": ["industrial", "enclosed"],
  "frequency": 3
}
```

### Socket Definition
```json
{
  "mesh": "SM_Modular_Wall_1_Merged",
  "sockets": [
    {
      "id": "wall_1_south",
      "side": "south",
      "offset_cells": 0,
      "type": "wall_continuation",
      "position_local_cm": [0, 0, 0],
      "direction": [0, -1, 0],
      "compatible_with": ["wall_continuation", "corner_outer", "door_frame", "window_frame"],
      "observed_connections": 47,
      "source": "demo_scene_extraction"
    }
  ]
}
```

---

## 7. External Dependencies

| Library | Purpose | Language | License | Package | Required Phase |
|---------|---------|----------|---------|---------|---------------|
| **numpy** | Grid math, GCD, distance computation, rotation matrices | Python | BSD | `numpy` | B (Scene Scanner) |
| **scipy.spatial** | DBSCAN clustering, distance matrices | Python | BSD | `scipy` | B (Scene Scanner) |
| **networkx** | Adjacency graph, connected components, graph algorithms | Python | BSD | `networkx` | B (Scene Scanner) |
| **DeBroglie** | WFC constraint solving (if needed for Phase D) | C#/.NET | MIT | NuGet: `DeBroglie` | D (Constraint Solver) |

**Notes:**
- **numpy** and **scipy** are likely already available in the Python venv. If not, zero-cost BSD licensed.
- **networkx** is the standard Python graph library. Zero-cost BSD licensed.
- **DeBroglie** is C#/.NET — would require either a subprocess call, Python port, or using a Python WFC library instead. Alternatives:
  - **python-wfc** (pure Python, limited features)
  - Custom minimal WFC in Python (adjacency propagation is ~100 lines)
  - Skip WFC entirely for Phase D and use a simpler constraint-propagation validator

**Recommendation:** For Phase D, implement a minimal constraint propagation validator in pure Python (~200 lines) rather than taking on a C# dependency. WFC's full power (entropy-based cell collapse) is overkill for validation/repair — simple iterative constraint checking suffices for our use case (<=10x10 grids).
