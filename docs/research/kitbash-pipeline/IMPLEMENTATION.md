# Implementation Plan: AI-Driven Kit-Bashing Pipeline

Each phase produces **usable tools on its own**. Phase A alone is valuable without later phases.

---

## Phase A — Mesh Catalog & Schema

**Goal:** Enrich the existing ModularSciFi catalog with measured bounds, pivot offsets, and semantic annotations. Build tools to generate, query, and maintain catalogs.

### New/Modified Files

| File | Action | Description |
|------|--------|-------------|
| `Python/helpers/catalog_enricher.py` | **CREATE** | Functions to measure bounds, add semantics, enrich catalog |
| `Python/build_catalog.py` | **MODIFY** | Add v2 schema fields (spatial, semantics, sockets stubs) |
| `Python/catalogs/modularscifi_meshes.json` | **REGENERATE** | Rebuild with v2 schema, measured bounds |
| `Python/unreal_mcp_server_advanced.py` | **MODIFY** | Register 3 new tool functions |

### New Tools

#### 1. `enrich_catalog_bounds`
```python
@mcp.tool()
def enrich_catalog_bounds(
    catalog: str = "modularscifi_meshes",
    category: str = "",
    dry_run: bool = False
) -> Dict[str, Any]:
```
- Iterates catalog meshes (optionally filtered by category)
- For each mesh with null bounds: spawns at origin, queries `get_actor_details` for bounding box, deletes actor
- Writes measured `bounds_extent`, `size_cm`, `pivot_offset_cm` back to catalog JSON
- `dry_run` mode reports what would be measured without spawning
- **Sequential spawning** — one at a time with 0.2s delay between spawn-measure-delete cycles
- Returns: `{success, measured_count, skipped_count, errors}`

#### 2. `add_catalog_semantics`
```python
@mcp.tool()
def add_catalog_semantics(
    mesh_name: str,
    catalog: str = "modularscifi_meshes",
    display_name: str = "",
    description: str = "",
    function: str = "",
    style_tags: List[str] = [],
    spawn_weight: float = -1
) -> Dict[str, Any]:
```
- Finds mesh entry in catalog by name (fuzzy match)
- Creates or updates `semantics` sub-object with provided fields
- Only updates fields that are non-empty / non-default (-1 for spawn_weight means "don't change")
- Returns: `{success, mesh_name, updated_fields}`

#### 3. `query_catalog`
```python
@mcp.tool()
def query_catalog(
    query: str = "",
    category: str = "",
    tags: List[str] = [],
    min_size_cm: List[float] = None,
    max_size_cm: List[float] = None,
    has_bounds: bool = False,
    catalog: str = "modularscifi_meshes",
    limit: int = 20
) -> Dict[str, Any]:
```
- Enhanced search: text query matches name/description/function, category filter, tag intersection, size range filter
- `has_bounds=True` filters to meshes with measured bounds only
- Returns: `{count, matches: [{name, path, category, size_cm, description, tags}]}`

### Implementation Details

**`catalog_enricher.py` internal functions:**

```python
def measure_mesh_bounds(unreal_connection, mesh_path: str, catalog_entry: dict) -> dict:
    """Spawn mesh at origin, measure bounds, delete. Returns enriched entry."""

def add_semantic_fields(catalog_path: str, mesh_name: str, fields: dict) -> dict:
    """Update semantic fields for one mesh in catalog JSON."""

def query_meshes(catalog_path: str, filters: dict) -> list:
    """Multi-criteria search across catalog entries."""

def upgrade_catalog_schema(catalog_path: str) -> dict:
    """Migrate v1 catalog to v2 schema (add spatial/semantics/sockets stubs)."""
```

**Schema migration (v1 → v2):**
- Existing `bounds_extent` and `size_cm` move under `spatial` sub-object
- Add `spatial.pivot_offset_cm` (default `[0,0,0]`, filled by measurement)
- Add `spatial.grid_footprint_cells` (computed from `size_cm / cell_size`)
- Add `spatial.rotations_deg` (default `[0, 90, 180, 270]` for modular pieces)
- Add empty `sockets: []` array (filled by Phase B)
- Add `semantics: {}` object (filled by `add_catalog_semantics`)
- Add `adjacency_stats: {}` object (filled by Phase B)
- Preserve all existing fields for backward compatibility

### `build_catalog.py` modifications:

- Add `"schema_version": "2.0"` to catalog root
- Output `spatial`, `semantics`, `sockets` sub-objects per mesh
- Pre-populate `spatial.grid_footprint_cells` for modular_building category meshes (all 1x1x1 on 500cm grid)
- Add `rotations_deg: [0, 90, 180, 270]` for all grid-snapping pieces

### Dependencies

- None beyond standard library (json, os, math, logging, time)
- Uses existing `safe_spawn_actor` and `safe_delete_actor` from `actor_name_manager.py`

### Test Cases

1. **Measure bounds test:**
   ```
   enrich_catalog_bounds(category="modular_building")
   ```
   Expected: All 15 modular_building meshes get measured bounds. `SM_Modular_Wall_1_Merged` should have `size_cm` approximately `[500, 500, 500]`.

2. **Add semantics test:**
   ```
   add_catalog_semantics(
       mesh_name="SM_Modular_Wall_1_Merged",
       display_name="Standard Wall",
       description="Straight wall panel with ribbed metal surface and conduit runs",
       function="Enclosure wall for rooms and corridors",
       style_tags=["industrial", "panelled"]
   )
   ```
   Expected: Catalog entry updated, retrievable via `query_catalog(query="industrial wall")`.

3. **Query test:**
   ```
   query_catalog(category="modular_building", has_bounds=True, min_size_cm=[400, 400, 0])
   ```
   Expected: Returns only modular pieces with measured bounds >= 400cm in X and Y.

### Exit Criteria

- [ ] All 15 modular_building meshes have measured `bounds_extent` and `size_cm` (non-null)
- [ ] `query_catalog` returns correct results for category, tag, size, and text filters
- [ ] `add_catalog_semantics` persists to JSON and is queryable
- [ ] Catalog JSON validates against v2 schema
- [ ] Existing `search_mesh_catalog` and `spawn_hab` still work (backward compatible)

---

## Phase B — Demo Scene Scanner

**Goal:** Scan artist-authored demo levels to extract grid conventions, adjacency rules, socket definitions, and room/corridor archetype templates.

### New/Modified Files

| File | Action | Description |
|------|--------|-------------|
| `Python/helpers/scene_scanner.py` | **CREATE** | Grid detection, adjacency graph, clustering, archetype extraction |
| `Python/catalogs/modularscifi_sockets.json` | **CREATE** | Extracted socket definitions |
| `Python/catalogs/modularscifi_archetypes.json` | **CREATE** | Room/corridor templates |
| `Python/unreal_mcp_server_advanced.py` | **MODIFY** | Register 4 new tools |

### New Tools

#### 1. `scan_scene_grid`
```python
@mcp.tool()
def scan_scene_grid(
    mesh_filter: str = "",
    category_filter: str = "modular_building",
    catalog: str = "modularscifi_meshes"
) -> Dict[str, Any]:
```
- Calls `get_actors_in_level`, filters to structural kit pieces via catalog category
- Computes GCD of position deltas between same-category pieces → grid_size_cm
- Snaps all positions to detected grid, quantizes rotations to nearest 90deg
- Returns: `{grid_size_cm, actor_count, structural_count, grid_positions: [{name, mesh, grid_x, grid_y, grid_z, rotation_deg}]}`

#### 2. `build_adjacency_graph`
```python
@mcp.tool()
def build_adjacency_graph(
    category_filter: str = "modular_building",
    max_neighbor_distance: float = 1.5,
    catalog: str = "modularscifi_meshes"
) -> Dict[str, Any]:
```
- Scans scene grid (or reuses cached data)
- For each pair of pieces within max_neighbor_distance × grid_size, checks if they share a face
- Builds graph: nodes = piece instances, edges = adjacency with direction and relative transform
- Returns: `{node_count, edge_count, nodes: [{id, mesh, pos, rot}], edges: [{from_id, to_id, direction, rel_transform}]}`

#### 3. `extract_archetypes`
```python
@mcp.tool()
def extract_archetypes(
    method: str = "connected_components",
    min_pieces: int = 3,
    catalog: str = "modularscifi_meshes",
    save_to: str = ""
) -> Dict[str, Any]:
```
- Builds adjacency graph from current scene
- Clusters via connected components (default) or flood-fill-from-doors
- Classifies each cluster: corridor (>4:1 aspect, 2 openings), room (<2:1, larger area), T-junction (3 openings), etc.
- Normalizes piece positions relative to cluster centroid
- Optionally saves to JSON file
- Returns: `{archetype_count, archetypes: [{id, classification, piece_count, grid_size, openings, pieces_relative}]}`

#### 4. `extract_sockets`
```python
@mcp.tool()
def extract_sockets(
    catalog: str = "modularscifi_meshes",
    save_to_catalog: bool = True
) -> Dict[str, Any]:
```
- Builds adjacency graph from current scene
- For each edge, records which mesh faces connect and their relative orientation
- Infers socket types from recurring connection patterns (wall↔wall = `wall_continuation`, wall↔door = `door_frame`, etc.)
- Writes socket definitions back to catalog if `save_to_catalog=True`
- Returns: `{mesh_count, total_sockets, socket_types: ["wall_continuation", ...], per_mesh: [{mesh, sockets}]}`

### Implementation Details

**`scene_scanner.py` internal functions:**

```python
def scan_structural_actors(unreal_connection, catalog_data, category_filter) -> list:
    """Get all actors, filter to kit meshes, return with transforms."""

def detect_grid_size(positions: list) -> float:
    """GCD of position deltas between structural meshes."""

def snap_to_grid(positions: list, grid_size: float) -> list:
    """Snap positions, quantize rotations to 90deg."""

def build_graph(grid_positions: list, grid_size: float, max_dist_factor: float) -> tuple:
    """Build adjacency graph. Returns (nodes, edges)."""

def cluster_connected_components(nodes, edges, min_pieces) -> list:
    """Group into connected components."""

def classify_archetype(cluster) -> str:
    """Classify cluster by aspect ratio and openings."""

def infer_sockets(edges, catalog_data) -> dict:
    """Infer socket types from observed adjacencies."""

def normalize_to_centroid(cluster) -> list:
    """Convert absolute positions to relative-from-centroid."""
```

**Grid detection algorithm:**
```python
import math
from functools import reduce

def detect_grid_size(positions):
    deltas = set()
    for i, p1 in enumerate(positions):
        for p2 in positions[i+1:]:
            dx = abs(p1[0] - p2[0])
            dy = abs(p1[1] - p2[1])
            if dx > 10:  # ignore sub-cm noise
                deltas.add(round(dx))
            if dy > 10:
                deltas.add(round(dy))
    if not deltas:
        return 500  # default
    return reduce(math.gcd, deltas)
```

### Dependencies

- **numpy** — Distance computation, array operations
- **Standard library** — `math.gcd`, `functools.reduce`, `collections.defaultdict`
- No networkx needed for Phase B — connected components can be done with simple BFS/DFS on adjacency list

### Test Cases

1. **Grid scan of ModularSciFi demo:**
   ```
   scan_scene_grid(category_filter="modular_building")
   ```
   Expected: `grid_size_cm: 500`, structural pieces identified with grid-aligned positions.

2. **Adjacency graph from existing habs:**
   ```
   build_adjacency_graph()
   ```
   Expected: Each hab's pieces connected; pieces at (0,0) and (-500,0) share an edge.

3. **Archetype extraction:**
   ```
   extract_archetypes(save_to="modularscifi_archetypes.json")
   ```
   Expected: Existing 2x1 habs classified as "room_2x1" archetype. Corridor segments as "corridor". Saved to JSON.

4. **Socket extraction from habs:**
   ```
   extract_sockets(save_to_catalog=True)
   ```
   Expected: Wall↔Wall connections labeled `wall_continuation`, Wall↔Door as `door_frame`, Floor↔Floor edges detected. Catalog updated.

### Exit Criteria

- [ ] `scan_scene_grid` correctly detects 500cm grid from a level with ModularSciFi pieces
- [ ] `build_adjacency_graph` produces valid graph where connected pieces share edges
- [ ] `extract_archetypes` classifies at least one cluster as "room" and stores template JSON
- [ ] `extract_sockets` identifies at least 3 socket types and writes them to catalog
- [ ] Archetype templates can be fed back to `spawn_layout` (Phase E) and reproduce the original scene

---

## Phase C — Layout Generation

**Goal:** Accept text description + catalog → output grid-based layout JSON ready for spawning.

### New/Modified Files

| File | Action | Description |
|------|--------|-------------|
| `Python/helpers/layout_generator.py` | **CREATE** | Prompt construction, response parsing, grid validation |
| `Python/unreal_mcp_server_advanced.py` | **MODIFY** | Register 2 new tools |

### New Tools

#### 1. `generate_layout_prompt`
```python
@mcp.tool()
def generate_layout_prompt(
    description: str,
    catalog: str = "modularscifi_meshes",
    max_grid: List[int] = [10, 10],
    style_tags: List[str] = [],
    num_examples: int = 2,
    archetype_catalog: str = "modularscifi_archetypes"
) -> Dict[str, Any]:
```
- Constructs an optimized prompt for the LLM containing:
  - Condensed catalog (modular_building pieces only, with display names and descriptions)
  - Grid rules (500cm cells, placement formulas for walls/doors/corners/roofs)
  - Few-shot examples from archetype catalog (best-matching archetypes by style tags)
  - The user's layout description
  - Chain-of-Symbol format instructions for output
- Returns the prompt as a string — the LLM (Claude) executes it and returns layout JSON
- Returns: `{prompt: str, catalog_pieces_included: int, examples_included: int}`

**Important:** This tool doesn't call an external LLM API. It constructs a prompt that Claude (the MCP client) will process. The response will be a layout JSON that gets fed to `validate_layout`.

#### 2. `validate_layout`
```python
@mcp.tool()
def validate_layout(
    layout: Dict[str, Any],
    catalog: str = "modularscifi_meshes"
) -> Dict[str, Any]:
```
- Checks layout JSON against rules:
  - All mesh references resolve to catalog entries
  - All positions on grid (multiples of cell_size)
  - All rotations in allowed set (0, 90, 180, 270)
  - No two pieces occupy same cell with same type
  - Floor coverage check (every wall/door has a floor underneath)
  - Socket compatibility (if sockets available in catalog)
- Returns: `{valid, violation_count, violations: [{cell, piece, issue, suggestion}], stats: {floors, walls, doors, corners, roofs}}`

### Implementation Details

**Prompt template structure (Chain-of-Symbol):**
```
You are a modular level designer. Place pieces on a {cell_size}cm grid.

AVAILABLE PIECES (place by grid coordinate + rotation):
- floor: SM_Modular_Merged (500x500cm, flat)
- wall: SM_Modular_Wall_1_Merged (500cm, vertical)
  Placement: South=same pos yaw=-90, East=(X,Y+500) yaw=0, North=(X-500,Y+500) yaw=90, West=(X-500,Y) yaw=180
- door: SM_Modular_DoorModule_Merged (same rules as wall)
- window: SM_Modular_WallWindow_3_Merged (same rules as wall)
- corner: SM_Modular_Corner_Merged
  Placement: SE=yaw -90, NE=yaw 0, NW=yaw 90, SW=yaw 180
- roof: SM_Modular_RoofModule_Merged (same pos as floor, yaw=0)
- skylight: SM_Modular_Ceiling_Window2_Merged (special rotation rules)

EXAMPLES:
{few_shot_examples}

TASK: {user_description}
Max grid size: {max_grid[0]}x{max_grid[1]} cells.

Output JSON in this format:
{output_format}

Think step by step: first place floors, then walls around the perimeter, then doors/windows, then corners, then roof.
```

### Dependencies

- None beyond standard library
- Uses catalog JSON for piece data and archetype JSON for examples

### Test Cases

1. **Generate prompt:**
   ```
   generate_layout_prompt(
       description="A straight corridor, 4 cells long, with a door on the west end and windows on the north side",
       max_grid=[4, 1]
   )
   ```
   Expected: Returns prompt string with catalog pieces, wall placement rules, and 1-2 corridor examples.

2. **Validate known-good layout:**
   ```
   validate_layout(layout=<2x1_Basic_V1_layout>)
   ```
   Expected: `{valid: true, violation_count: 0}`

3. **Validate broken layout (missing floor):**
   ```
   validate_layout(layout=<layout_with_wall_but_no_floor>)
   ```
   Expected: `{valid: false, violations: [{issue: "Wall at (0,0) has no floor underneath"}]}`

### Exit Criteria

- [ ] `generate_layout_prompt` produces a well-structured prompt with catalog context and examples
- [ ] `validate_layout` correctly identifies violations in intentionally broken layouts
- [ ] `validate_layout` passes all existing hab variation JSONs as valid
- [ ] End-to-end: prompt generated → Claude produces layout → validate_layout approves → spawn_layout works

---

## Phase D — Constraint Solver

**Goal:** Automatically repair layout violations using constraint propagation.

### New/Modified Files

| File | Action | Description |
|------|--------|-------------|
| `Python/helpers/constraint_solver.py` | **CREATE** | Constraint propagation, violation repair |
| `Python/unreal_mcp_server_advanced.py` | **MODIFY** | Register 1 new tool |

### New Tools

#### 1. `repair_layout`
```python
@mcp.tool()
def repair_layout(
    layout: Dict[str, Any],
    catalog: str = "modularscifi_meshes",
    max_iterations: int = 50,
    preserve_doors: bool = True,
    preserve_windows: bool = True
) -> Dict[str, Any]:
```
- Takes a layout with violations
- Iteratively resolves:
  - Missing floors → add floor pieces
  - Missing walls → add wall pieces to exposed perimeter edges
  - Missing corners → add corners at perimeter vertices
  - Missing roof → add roof pieces over all floor cells
  - Socket mismatches → swap piece variants to match neighbors
- Preserves door and window placements (user intent) by default
- Returns: `{repaired_layout, changes_made: int, changes: [{action, cell, piece, reason}], remaining_violations: int}`

### Implementation Details

**Constraint propagation algorithm (pure Python, no WFC library):**

```python
def repair_layout(layout, catalog, max_iter=50):
    """
    Simple iterative constraint repair:
    1. Find all floor cells
    2. For each floor cell, check perimeter edges
    3. If edge is exposed (no neighbor floor cell), ensure wall/door/window exists
    4. For each perimeter vertex, ensure corner exists
    5. For each floor cell, ensure roof/skylight exists
    6. Repeat until no violations remain or max_iter reached
    """
```

This is simpler than full WFC but sufficient for:
- Completing partial layouts (LLM forgot some walls)
- Fixing missing corners (common LLM mistake)
- Ensuring floor coverage and roof coverage
- NOT suitable for: generating entire layouts from scratch (that's Phase C's job)

### Dependencies

- None beyond standard library

### Test Cases

1. **Repair missing walls:**
   Layout with 4 floor cells in a row but only 2 of 6 required walls.
   Expected: Adds the 4 missing walls at correct positions and rotations.

2. **Repair missing corners:**
   Layout with all floors, walls, roof but no corners.
   Expected: Adds corners at all perimeter vertices with correct rotations.

3. **Preserve user doors:**
   Layout with intentional door placement but missing adjacent wall.
   Expected: Adds wall on other side, preserves door.

### Exit Criteria

- [ ] Repairing a floor-only layout produces a complete enclosed room
- [ ] Repairing a layout with deliberate doors preserves them and adds walls elsewhere
- [ ] All existing hab variations pass through repair unchanged (already valid)
- [ ] Repair + validate = valid for any repairable layout

---

## Phase E — Spawn & Iterate

**Goal:** Generalized spawning from any layout manifest, plus save-back and diff tools.

### New/Modified Files

| File | Action | Description |
|------|--------|-------------|
| `Python/helpers/layout_spawner.py` | **CREATE** | Generalized layout spawning (extends hab_spawner pattern) |
| `Python/helpers/template_manager.py` | **CREATE** | Save scene selections as templates, diff layouts |
| `Python/unreal_mcp_server_advanced.py` | **MODIFY** | Register 3 new tools |

### New Tools

#### 1. `spawn_layout`
```python
@mcp.tool()
def spawn_layout(
    layout: Dict[str, Any],
    location: List[float] = [0, 0, 0],
    rotation: float = 0,
    name_prefix: str = "Layout",
    catalog: str = "modularscifi_meshes",
    spawn_delay: float = 0.1,
    screenshot: bool = True
) -> Dict[str, Any]:
```
- Accepts any layout manifest JSON (not just hab variations)
- Resolves mesh paths from catalog
- Applies rotation math (reuses `_rotate_piece` from hab_spawner)
- Sequential spawning with delays
- Optional validation screenshot
- Returns: `{success, spawned_count, failed_count, spawned: [...], screenshot?}`

#### 2. `save_scene_as_template`
```python
@mcp.tool()
def save_scene_as_template(
    actor_filter: str = "",
    template_name: str = "unnamed",
    catalog: str = "modularscifi_meshes",
    save_path: str = ""
) -> Dict[str, Any]:
```
- Finds actors matching filter pattern
- Gets details for each (mesh, transform, bounds)
- Computes centroid, normalizes all positions to relative
- Matches meshes to catalog entries for short names
- Saves as template/layout JSON
- Returns: `{success, piece_count, template_path, template}`

#### 3. `diff_layouts`
```python
@mcp.tool()
def diff_layouts(
    layout_a: Dict[str, Any],
    layout_b: Dict[str, Any]
) -> Dict[str, Any]:
```
- Compares two layout manifests by cell position + piece type
- Reports: added pieces, removed pieces, moved pieces, changed rotations, unchanged
- Returns: `{added: [...], removed: [...], moved: [...], rotation_changed: [...], unchanged_count: int}`

### Dependencies

- None beyond standard library
- Reuses rotation math from `hab_spawner.py`

### Test Cases

1. **Spawn generated layout:**
   Generate layout via Phase C prompt → validate → spawn.
   Expected: All pieces appear in UE5 at correct positions. Screenshot confirms.

2. **Save and replay:**
   Spawn a hab → human tweaks in editor → `save_scene_as_template` → spawn copy elsewhere.
   Expected: Copy matches tweaked version, not original.

3. **Diff after human edit:**
   Spawn layout → human moves one wall → save → diff with original.
   Expected: Diff shows 1 moved piece, rest unchanged.

### Exit Criteria

- [ ] `spawn_layout` works with both hab variation JSONs and generated layout JSONs
- [ ] `save_scene_as_template` produces valid JSON that can be re-spawned
- [ ] `diff_layouts` correctly identifies added, removed, and moved pieces
- [ ] Full round-trip: describe → generate → validate → repair → spawn → tweak → save → respawn elsewhere

---

## Build Sequence Summary

| Phase | New Tools | Files Created | Files Modified | Dependencies | Standalone Value |
|-------|-----------|--------------|----------------|-------------|-----------------|
| **A** | 3 | 1 helper | build_catalog.py, server | stdlib only | Measured catalog, semantic search |
| **B** | 4 | 1 helper, 2 catalogs | server | numpy (optional) | Auto-extracted sockets + archetypes |
| **C** | 2 | 1 helper | server | stdlib only | Text → layout generation |
| **D** | 1 | 1 helper | server | stdlib only | Auto-repair broken layouts |
| **E** | 3 | 2 helpers | server | stdlib only | General spawn + save + diff |

**Total: 13 new tools, 6 new helper files, 2 new catalog files.**

Each phase depends on the catalog built in Phase A. Phases B-E can be developed in parallel after A, though B's sockets/archetypes improve C's layout generation quality.
