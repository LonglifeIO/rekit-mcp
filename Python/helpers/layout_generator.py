"""
Layout Generator — tools for constructing LLM prompts that generate modular
level layouts, and validating the resulting layout JSON.

Phase C of the kit-bashing pipeline: accept text description + catalog →
output grid-based layout JSON ready for spawning via spawn_hab.
"""

import json
import os
import logging
from typing import Dict, Any, List, Optional
from collections import defaultdict

logger = logging.getLogger(__name__)

from .catalog_enricher import _catalog_path, _load_catalog, CATALOGS_DIR


# ---------------------------------------------------------------------------
# Piece definitions for prompt generation
# ---------------------------------------------------------------------------

# Map piece_type to canonical mesh name + placement notes
PIECE_DEFS = {
    "floor": {
        "mesh": "SM_Modular_Merged",
        "desc": "500x500cm floor tile",
        "placement": "Place at cell origin. yaw=0 always.",
    },
    "wall": {
        "mesh": "SM_Modular_Wall_1_Merged",
        "desc": "500cm solid wall segment",
        "placement": (
            "Relative to floor pivot: "
            "South=same pos yaw=-90, "
            "East=(X, Y+500) yaw=0, "
            "North=(X-500, Y+500) yaw=90, "
            "West=(X-500, Y) yaw=180"
        ),
    },
    "door": {
        "mesh": "SM_Modular_DoorModule_Merged",
        "desc": "500cm wall with door opening (same placement rules as wall)",
        "placement": "Same position/rotation rules as wall.",
    },
    "window": {
        "mesh": "SM_Modular_WallWindow_3_Merged",
        "desc": "500cm wall with window (same placement rules as wall)",
        "placement": "Same position/rotation rules as wall. Alt: SM_WallWindow_1_Merged, SM_Modular_WallWindow_2_Merged.",
    },
    "corner": {
        "mesh": "SM_Modular_Corner_Merged",
        "desc": "Corner trim column (convex corners only)",
        "placement": "SE=yaw -90, NE=yaw 0, NW=yaw 90, SW=yaw 180. Position at the corner vertex of the cell.",
    },
    "roof": {
        "mesh": "SM_Modular_RoofModule_Merged",
        "desc": "500x500cm roof panel (pivot at floor level, geometry offsets up)",
        "placement": "Same position as floor tile. yaw=0 always.",
    },
    "skylight": {
        "mesh": "SM_Modular_Ceiling_Window1_Merged",
        "desc": "500x500cm glass roof panel",
        "placement": "Same as roof. Alt: SM_Modular_Ceiling_Window2_Merged.",
    },
}

# Wall-type pieces (these share wall placement rules)
_WALL_TYPES = {"wall", "door", "window"}

# Valid yaw angles
_VALID_YAWS = {0, 90, -90, 180}

# Output JSON format template
_OUTPUT_FORMAT = """{
  "name": "<layout_name>",
  "description": "<brief description>",
  "grid_rotation_deg": 0,
  "cell_size_cm": 500,
  "cells": [
    {"grid": [0, 0], "label": "<cell label>"}
  ],
  "pieces": [
    {"index": 1, "type": "<piece_type>", "mesh": "<mesh_name>", "rel": [X, Y, Z], "yaw": <degrees>, "label": "<description>"}
  ],
  "mesh_paths": {
    "<mesh_name>": "<full_asset_path>"
  }
}"""


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def generate_layout_prompt(
    description: str,
    catalog_name: str = "modularscifi_meshes",
    max_grid: List[int] = None,
    style_tags: List[str] = None,
    num_examples: int = 2,
    archetype_catalog: str = "modularscifi_archetypes",
) -> Dict[str, Any]:
    """
    Construct an optimized prompt for Claude to generate a modular layout.

    The prompt includes:
      - Condensed piece catalog with placement rules
      - Few-shot examples from the archetype catalog
      - The user's layout description
      - Chain-of-Symbol output format instructions

    This tool does NOT call an external LLM. It returns the prompt string
    for Claude (the MCP client) to process.

    Args:
        description: Natural language layout description.
        catalog_name: Catalog file to pull mesh paths from.
        max_grid: Maximum grid size [width, height] in cells.
        style_tags: Tags to match when selecting example archetypes.
        num_examples: Number of few-shot examples to include.
        archetype_catalog: Archetype file to pull examples from.

    Returns:
        Dict with prompt string, pieces_included count, examples_included count.
    """
    if max_grid is None:
        max_grid = [10, 10]
    if style_tags is None:
        style_tags = []

    # Load catalog for mesh paths
    catalog = _load_catalog(_catalog_path(catalog_name))
    mesh_paths = {}
    if catalog:
        for entry in catalog.get("meshes", []):
            name = entry.get("name", "")
            path = entry.get("path", "")
            if name and path:
                mesh_paths[name] = path

    # Build pieces section
    pieces_section = _build_pieces_section(mesh_paths)

    # Load examples from archetype catalog
    examples_section, examples_count = _build_examples_section(
        archetype_catalog, mesh_paths, num_examples, style_tags
    )

    # Build wall placement rules
    wall_rules = _build_wall_rules()

    # Build corner placement rules
    corner_rules = _build_corner_rules()

    # Assemble prompt
    prompt = f"""You are a modular level designer. Place pieces on a 500cm grid to build the requested layout.

## AVAILABLE PIECES

{pieces_section}

## PLACEMENT RULES

### Wall/Door/Window Placement (relative to floor cell pivot)
{wall_rules}

### Corner Trim Placement (convex corners only)
{corner_rules}

### Roof/Skylight Placement
- Same position as the floor tile in that cell. yaw=0 always.
- Roof pivot is at FLOOR level — the mesh geometry internally offsets up to ceiling height.
- Use skylight (glass roof) for variety. Adjacent glass panels create visual continuity.

### General Rules
- Every wall/door/window MUST have a floor tile in the same cell or the adjacent cell it borders.
- Every floor cell should have a roof or skylight above it.
- Corners go at convex vertices where two walls meet at 90 degrees.
- All positions are RELATIVE to the layout origin (first cell's floor pivot).
- Z=0 for all single-story pieces.

{examples_section}

## TASK

{description}

Max grid size: {max_grid[0]}x{max_grid[1]} cells.

## OUTPUT FORMAT

Return ONLY valid JSON in this exact format:
{_OUTPUT_FORMAT}

### Required mesh_paths entries
Include a mesh_paths entry for every unique mesh name used in pieces. Use these paths:
{_format_mesh_paths(mesh_paths)}

## STRATEGY

Think step by step:
1. First, sketch the floor plan (which cells exist and their grid coordinates)
2. Place floor tiles at each cell
3. Place walls around the perimeter (skip edges shared between adjacent cells)
4. Replace some walls with doors (for entry/exit) and windows (for variety)
5. Place corner trims at every convex corner vertex
6. Place roof/skylight over every floor cell
7. Verify: every wall has a floor, every floor has a roof, corners at all convex vertices
"""

    return {
        "success": True,
        "prompt": prompt,
        "pieces_included": len(PIECE_DEFS),
        "examples_included": examples_count,
        "max_grid": max_grid,
    }


def validate_layout(
    layout: Dict[str, Any],
    catalog_name: str = "modularscifi_meshes",
) -> Dict[str, Any]:
    """
    Validate a layout JSON against placement rules.

    Checks:
      - All mesh names resolve to catalog entries
      - All positions on grid (multiples of cell_size)
      - All rotations in allowed set (0, 90, -90, 180)
      - No duplicate pieces (same position + type)
      - Floor coverage (every wall/door/window has a floor in its cell)
      - Roof coverage (every floor has a roof or skylight)

    Args:
        layout: Layout JSON dict (same format as hab variation JSONs).
        catalog_name: Catalog to validate mesh names against.

    Returns:
        Dict with valid bool, violation_count, violations list, and stats.
    """
    violations = []
    pieces = layout.get("pieces", [])
    cell_size = layout.get("cell_size_cm", 500)
    mesh_paths = layout.get("mesh_paths", {})

    # Load catalog for mesh validation
    catalog = _load_catalog(_catalog_path(catalog_name))
    catalog_names = set()
    catalog_paths = {}
    if catalog:
        for entry in catalog.get("meshes", []):
            name = entry.get("name", "")
            path = entry.get("path", "")
            if name:
                catalog_names.add(name)
            if name and path:
                catalog_paths[name] = path

    # Stats counters
    stats = defaultdict(int)

    # Track floor and roof positions for coverage checks
    floor_cells = set()  # (gx, gy)
    roof_cells = set()   # (gx, gy)
    wall_positions = []  # list of (gx, gy, type) for floor-coverage check
    seen_pieces = set()  # (rel_tuple, type) for duplicate check

    # Normalize type aliases
    _TYPE_ALIASES = {"ceiling": "roof"}  # older variations use "ceiling"

    for piece in pieces:
        ptype = piece.get("type", "unknown")
        ptype = _TYPE_ALIASES.get(ptype, ptype)  # normalize aliases
        mesh = piece.get("mesh", "")
        rel = piece.get("rel", [0, 0, 0])
        yaw = piece.get("yaw", 0)
        idx = piece.get("index", "?")
        stats[ptype] += 1

        # Check 1: Mesh name exists in catalog or mesh_paths
        if mesh and mesh not in catalog_names and mesh not in mesh_paths:
            violations.append({
                "index": idx,
                "piece": f"{ptype} ({mesh})",
                "issue": f"Mesh '{mesh}' not found in catalog",
                "suggestion": "Check mesh name spelling against catalog entries",
            })

        # Check 2: Position on grid
        if len(rel) >= 2:
            x, y = rel[0], rel[1]
            if cell_size > 0:
                if abs(x) % cell_size != 0 or abs(y) % cell_size != 0:
                    # Allow wall offsets which are multiples of cell_size
                    # Wall placement formulas use offsets like (X-500, Y+500)
                    # so positions should still be multiples of cell_size
                    violations.append({
                        "index": idx,
                        "piece": f"{ptype} ({mesh})",
                        "issue": f"Position ({x}, {y}) not on {cell_size}cm grid",
                        "suggestion": f"All coordinates must be multiples of {cell_size}",
                    })

        # Check 3: Rotation validity
        yaw_norm = yaw % 360
        if yaw_norm > 180:
            yaw_norm -= 360
        if yaw_norm not in _VALID_YAWS:
            violations.append({
                "index": idx,
                "piece": f"{ptype} ({mesh})",
                "issue": f"Rotation {yaw} not in valid set {_VALID_YAWS}",
                "suggestion": "Use 0, 90, -90, or 180 degrees only",
            })

        # Check 4: Duplicate detection
        rel_key = (tuple(rel[:3]) if len(rel) >= 3 else tuple(rel), ptype)
        if rel_key in seen_pieces:
            violations.append({
                "index": idx,
                "piece": f"{ptype} ({mesh})",
                "issue": f"Duplicate {ptype} at position {rel}",
                "suggestion": "Remove duplicate piece",
            })
        seen_pieces.add(rel_key)

        # Track positions for coverage checks
        if len(rel) >= 2:
            gx = int(round(rel[0] / cell_size)) if cell_size > 0 else 0
            gy = int(round(rel[1] / cell_size)) if cell_size > 0 else 0

            if ptype == "floor":
                floor_cells.add((gx, gy))
            elif ptype in ("roof", "skylight"):
                roof_cells.add((gx, gy))
                # 2x1 ceiling panels (Ceiling_Window2) cover an adjacent cell too
                if "Ceiling_Window2" in mesh or "CeilingWindow2" in mesh:
                    # Panel spans 2 cells — register neighbor based on yaw
                    yaw_n = yaw % 360
                    if yaw_n > 180:
                        yaw_n -= 360
                    if yaw_n == 0 or yaw_n == 180:
                        roof_cells.add((gx + 1, gy))
                        roof_cells.add((gx - 1, gy))
                    else:
                        roof_cells.add((gx, gy + 1))
                        roof_cells.add((gx, gy - 1))
            elif ptype in _WALL_TYPES:
                # Determine which cell this wall belongs to based on yaw
                wall_cell = _wall_cell_from_position(rel, yaw, cell_size)
                wall_positions.append((wall_cell, ptype, idx, mesh))

    # Check 5: Floor coverage — every wall/door/window should have a floor nearby
    for wall_cell, wtype, widx, wmesh in wall_positions:
        if wall_cell not in floor_cells:
            violations.append({
                "index": widx,
                "piece": f"{wtype} ({wmesh})",
                "issue": f"{wtype.title()} at cell {wall_cell} has no floor underneath",
                "suggestion": "Add a floor tile at that cell or adjust wall position",
            })

    # Check 6: Roof coverage — every floor should have a roof/skylight
    for fc in floor_cells:
        if fc not in roof_cells:
            violations.append({
                "index": "-",
                "piece": f"floor at {fc}",
                "issue": f"Floor at cell {fc} has no roof or skylight",
                "suggestion": "Add a roof or skylight at that cell position",
            })

    # Check 7: mesh_paths completeness
    meshes_used = {p.get("mesh", "") for p in pieces if p.get("mesh")}
    for mesh in meshes_used:
        if mesh not in mesh_paths:
            violations.append({
                "index": "-",
                "piece": mesh,
                "issue": f"Mesh '{mesh}' used in pieces but missing from mesh_paths",
                "suggestion": "Add the full asset path to mesh_paths",
            })

    return {
        "success": True,
        "valid": len(violations) == 0,
        "violation_count": len(violations),
        "violations": violations,
        "stats": dict(stats),
    }


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _build_pieces_section(mesh_paths: Dict[str, str]) -> str:
    """Build the pieces section of the prompt."""
    lines = []
    for ptype, pdef in PIECE_DEFS.items():
        mesh = pdef["mesh"]
        desc = pdef["desc"]
        placement = pdef["placement"]
        lines.append(f"**{ptype}**: `{mesh}` — {desc}")
        lines.append(f"  Placement: {placement}")
        lines.append("")
    return "\n".join(lines)


def _build_wall_rules() -> str:
    """Build detailed wall placement rules."""
    return """For a floor tile at position (X, Y, 0):
- **South wall**: rel=(X, Y, 0), yaw=-90
- **East wall**: rel=(X, Y+500, 0), yaw=0
- **North wall**: rel=(X-500, Y+500, 0), yaw=90
- **West wall**: rel=(X-500, Y, 0), yaw=180

IMPORTANT: Only place walls on the PERIMETER of the layout. Walls between two adjacent floor cells create an interior wall that blocks passage. Shared edges between cells should be left open (no wall) unless you specifically want an interior partition.

When two cells share an edge, the wall that would separate them is omitted for both cells."""


def _build_corner_rules() -> str:
    """Build corner placement rules."""
    return """Corners go at convex vertices where two perpendicular walls meet.
For a floor tile at position (X, Y, 0):
- **SE corner**: rel=(X, Y, 0), yaw=-90
- **NE corner**: rel=(X, Y+500, 0), yaw=0
- **NW corner**: rel=(X-500, Y+500, 0), yaw=90
- **SW corner**: rel=(X-500, Y, 0), yaw=180

Only place corners at CONVEX (outward-pointing) corners of the layout perimeter. Do NOT place corners at concave (inward) corners or interior positions."""


def _build_examples_section(
    archetype_catalog: str,
    mesh_paths: Dict[str, str],
    num_examples: int,
    style_tags: List[str],
) -> tuple:
    """Build few-shot examples from archetype catalog. Returns (section_text, count)."""
    arch_path = os.path.join(CATALOGS_DIR, f"{archetype_catalog}.json")
    arch_data = _load_catalog(arch_path)

    if not arch_data or not arch_data.get("archetypes"):
        return "", 0

    archetypes = arch_data["archetypes"]
    cell_size = arch_data.get("grid_size_cm", 500)

    # Select archetypes (prefer matching style tags, or just take first N)
    selected = archetypes[:num_examples]

    if not selected:
        return "", 0

    examples = ["## EXAMPLES\n"]
    for arch in selected:
        ex = _archetype_to_example(arch, cell_size, mesh_paths)
        if ex:
            examples.append(ex)

    return "\n".join(examples), len(selected)


def _archetype_to_example(
    archetype: Dict[str, Any],
    cell_size: int,
    mesh_paths: Dict[str, str],
) -> str:
    """Convert an archetype to a compact JSON example for few-shot prompting."""
    classification = archetype.get("classification", "unknown")
    piece_count = archetype.get("piece_count", 0)
    pieces_rel = archetype.get("pieces_relative", [])

    if not pieces_rel:
        return ""

    # Build cells list from floor pieces
    cells = []
    for p in pieces_rel:
        if p["piece_type"] == "floor":
            cells.append({
                "grid": [p["rel_grid_x"], p["rel_grid_y"]],
                "label": f"cell ({p['rel_grid_x']},{p['rel_grid_y']})"
            })

    # Build pieces list with world-relative positions
    pieces = []
    used_meshes = set()
    for i, p in enumerate(pieces_rel):
        mesh = p["mesh_name"]
        used_meshes.add(mesh)
        pieces.append({
            "index": i + 1,
            "type": p["piece_type"],
            "mesh": mesh,
            "rel": [
                p["rel_grid_x"] * cell_size,
                p["rel_grid_y"] * cell_size,
                p["rel_grid_z"] * cell_size,
            ],
            "yaw": p["rotation_deg"],
            "label": f"{p['piece_type']} at grid ({p['rel_grid_x']},{p['rel_grid_y']})",
        })

    # Build mesh_paths for used meshes
    ex_paths = {}
    for m in used_meshes:
        if m in mesh_paths:
            ex_paths[m] = mesh_paths[m]

    example_json = {
        "name": f"example_{classification}",
        "description": f"Extracted {classification} with {piece_count} pieces",
        "grid_rotation_deg": 0,
        "cell_size_cm": cell_size,
        "cells": cells,
        "pieces": pieces,
        "mesh_paths": ex_paths,
    }

    return f"### Example: {classification} ({piece_count} pieces)\n```json\n{json.dumps(example_json, indent=2)}\n```\n"


def _format_mesh_paths(mesh_paths: Dict[str, str]) -> str:
    """Format mesh paths for the prompt."""
    # Only include modular building meshes
    relevant = {k: v for k, v in mesh_paths.items()
                if any(p["mesh"] in k or k in p.get("mesh", "")
                       for p in PIECE_DEFS.values())}

    # Also add known window/door variants
    for k, v in mesh_paths.items():
        if any(sub in k for sub in ["WallWindow", "DoorModule", "Ceiling_Window"]):
            relevant[k] = v

    if not relevant:
        # Fall back to hardcoded paths from memory
        relevant = {
            "SM_Modular_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Floor/SM_Modular_Merged",
            "SM_Modular_Wall_1_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Wall_1/SM_Modular_Wall_1_Merged",
            "SM_Modular_DoorModule_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Door_Var1/SM_Modular_DoorModule_Merged",
            "SM_Modular_WallWindow_3_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_3/SM_Modular_WallWindow_3_Merged",
            "SM_WallWindow_1_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_1/SM_WallWindow_1_Merged",
            "SM_Modular_WallWindow_2_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_2/SM_Modular_WallWindow_2_Merged",
            "SM_Modular_Corner_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Corner/SM_Modular_Corner_Merged",
            "SM_Modular_RoofModule_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Ceiling/SM_Modular_RoofModule_Merged",
            "SM_Modular_Ceiling_Window1_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_CeilingWindow_1/SM_Modular_Ceiling_Window1_Merged",
            "SM_Modular_Ceiling_Window2_Merged": "/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_CeilingWindow_2/SM_Modular_Ceiling_Window2_Merged",
        }

    lines = []
    for name, path in sorted(relevant.items()):
        lines.append(f'  "{name}": "{path}"')
    return "{\n" + ",\n".join(lines) + "\n}"


def _wall_cell_from_position(
    rel: List, yaw: float, cell_size: int
) -> tuple:
    """
    Determine which floor cell a wall piece belongs to based on its
    position and rotation.

    Wall placement formulas (relative to floor at X,Y):
      South: same pos, yaw=-90 → cell is (X/cs, Y/cs)
      East:  (X, Y+500), yaw=0 → cell is (X/cs, (Y-cs)/cs) = (X/cs, Y/cs - 1)
      North: (X-500, Y+500), yaw=90 → cell is ((X+cs)/cs, (Y-cs)/cs)
      West:  (X-500, Y), yaw=180 → cell is ((X+cs)/cs, Y/cs)

    We reverse-engineer the floor cell from the wall's position and yaw.
    """
    if cell_size <= 0:
        return (0, 0)

    x = rel[0] if len(rel) > 0 else 0
    y = rel[1] if len(rel) > 1 else 0

    yaw_norm = yaw % 360
    if yaw_norm > 180:
        yaw_norm -= 360

    if yaw_norm == -90:  # South wall
        gx = int(round(x / cell_size))
        gy = int(round(y / cell_size))
    elif yaw_norm == 0:  # East wall
        gx = int(round(x / cell_size))
        gy = int(round((y - cell_size) / cell_size))
    elif yaw_norm == 90:  # North wall
        gx = int(round((x + cell_size) / cell_size))
        gy = int(round((y - cell_size) / cell_size))
    elif yaw_norm == 180 or yaw_norm == -180:  # West wall
        gx = int(round((x + cell_size) / cell_size))
        gy = int(round(y / cell_size))
    else:
        # Fallback: closest grid cell
        gx = int(round(x / cell_size))
        gy = int(round(y / cell_size))

    return (gx, gy)
