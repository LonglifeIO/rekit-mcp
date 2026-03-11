# MCP Outpost Builder Prompt

## Context (paste this after context compaction)

You need to create a new MCP tool called `create_outpost_compound` that builds a sci-fi mining outpost from primitive shapes in Unreal Engine 5.7. This is for the ThoughtSpace project (Outpost Theta).

### Technical Foundation

**Source files to modify:**
- `F:/UE Projects/unreal-engine-mcp/Python/helpers/castle_creation.py` (or create new `outpost_creation.py` alongside it)
- `F:/UE Projects/unreal-engine-mcp/Python/unreal_mcp_server_advanced.py` (register the new tool with `@mcp.tool()` decorator, around line 1684 where `create_castle_fortress` lives)

**How actors are spawned (use this pattern, NOT `spawn_physics_blueprint_actor`):**
```python
from .actor_name_manager import safe_spawn_actor

params = {
    "name": "Outpost_Building_Main",
    "type": "StaticMeshActor",
    "location": [x, y, z],
    "scale": [width/100, depth/100, height/100],  # divide by 100 since base shapes are 100 units
    "static_mesh": "/Engine/BasicShapes/Cube.Cube"
}
result = safe_spawn_actor(unreal, params, auto_unique_name=True)
```

**Available meshes (all 100 units at scale 1):**
- `/Engine/BasicShapes/Cube.Cube` — buildings, walls, doors, platforms, parking slabs
- `/Engine/BasicShapes/Cylinder.Cylinder` — tower masts, pipes, pillars, antenna poles
- `/Engine/BasicShapes/Cone.Cone` — antenna tips, roof peaks
- `/Engine/BasicShapes/Sphere.Sphere` — radar domes, lights, tank caps

### Outpost Compound Layout (~40-50 pieces)

**Design:** Industrial/military arctic mining outpost. Stark, functional, isolated. Rectangular prefab buildings, comms equipment, vehicle parking. NOT a castle — think remote research station.

**Ground Z:** Use `location[2]` as the ground plane. All Z offsets are relative to that.

#### A. Ground Pad (1 piece)
- Large flat cube slab as the compound footprint
- Scale: ~(20, 16, 0.15) — 2000x1600 unit platform
- Color: Dark concrete gray [0.25, 0.24, 0.23]

#### B. Main Building — Command/Hab Module (1 piece)
- Tall rectangular prism, largest structure
- Scale: ~(6, 4, 2.5) — 600x400x250
- Offset from center, toward the back (north)
- Color: Weathered metal gray [0.40, 0.38, 0.36]

#### C. Secondary Buildings (3 pieces)
- Smaller rectangular prisms — storage, med bay, crew quarters
- Scale: ~(3, 3, 1.8) each, varied slightly
- Spread around the compound
- Color: Slightly different grays [0.38, 0.36, 0.34], [0.42, 0.40, 0.37]

#### D. Doors (4-6 pieces)
- Tiny dark cubes inset into building faces
- Scale: ~(0.08, 0.6, 1.2) or (0.6, 0.08, 1.2) depending on wall orientation
- Color: Near-black [0.05, 0.05, 0.08]
- Placed flush against building walls, centered low

#### E. Communications Tower (3-4 pieces per tower, 1-2 towers)
- **Mast:** Tall thin cylinder — scale (0.3, 0.3, 6)
- **Antenna dish:** Flattened sphere — scale (1.2, 1.2, 0.3) at top
- **Antenna tip:** Small cone — scale (0.4, 0.4, 1.0) above dish
- **Base:** Small cube — scale (0.8, 0.8, 0.3)
- Color: Rust/metal [0.50, 0.42, 0.35] for mast, [0.60, 0.58, 0.55] for dish

#### F. Radio Antenna Array (3-5 pieces)
- Multiple thin cylinders of varying heights (scale 0.15, 0.15, 2-4)
- Small spheres on tips (scale 0.2, 0.2, 0.2)
- Grouped together on one side of compound
- Color: Dark metal [0.35, 0.33, 0.30]

#### G. Parking/Landing Pad (2-3 pieces)
- Flat rectangular slab — scale (5, 4, 0.1)
- Slightly lighter color than main ground [0.30, 0.29, 0.28]
- Optional: border markers — thin cubes along edges
- Offset to one side of compound (south or east)

#### H. Storage Containers (3-4 pieces)
- Small rectangular cubes scattered near buildings
- Scale: ~(1.5, 0.8, 0.8) — shipping container proportions
- Colors: Muted industrial — [0.35, 0.25, 0.20], [0.30, 0.35, 0.28], [0.40, 0.35, 0.30]

#### I. Perimeter Markers/Fence Posts (6-8 pieces)
- Thin tall cylinders around compound edges
- Scale: (0.15, 0.15, 1.5)
- Color: Yellow-orange warning [0.70, 0.55, 0.15]

#### J. Utility — Pipes/Conduits (2-3 pieces)
- Horizontal cylinders connecting buildings
- Scale: (0.2, 3.0, 0.2) or similar
- Color: Pipe gray [0.45, 0.43, 0.40]

### Color Palette
| Role | RGB | Description |
|------|-----|-------------|
| Ground pad | [0.25, 0.24, 0.23] | Dark concrete |
| Main building | [0.40, 0.38, 0.36] | Weathered metal |
| Secondary buildings | [0.38, 0.36, 0.34] | Lighter metal |
| Doors | [0.05, 0.05, 0.08] | Near-black |
| Comms tower | [0.50, 0.42, 0.35] | Rusty metal |
| Antenna dish | [0.60, 0.58, 0.55] | Light metal |
| Containers | [0.35, 0.25, 0.20] | Rust/industrial |
| Fence posts | [0.70, 0.55, 0.15] | Warning yellow |
| Pipes | [0.45, 0.43, 0.40] | Pipe gray |
| Parking pad | [0.30, 0.29, 0.28] | Lighter ground |

### Tool Signature
```python
@mcp.tool()
def create_outpost_compound(
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "Outpost",
    compound_size: str = "medium",  # "small", "medium", "large"
    include_comms_tower: bool = True,
    include_antenna_array: bool = True,
    include_parking_pad: bool = True,
    include_containers: bool = True,
    include_perimeter: bool = True
) -> Dict[str, Any]:
```

### Implementation Steps
1. Create `F:/UE Projects/unreal-engine-mcp/Python/helpers/outpost_creation.py`
2. Write all the builder helper functions (one per section A-J above)
3. Import and call from the main tool function
4. Register in `unreal_mcp_server_advanced.py` with `@mcp.tool()` decorator
5. Restart the MCP server (kill and relaunch `uv run unreal_mcp_server_advanced.py`)
6. Test with: `create_outpost_compound(location=[-266390, -143950, -21343.673], name_prefix="OutpostTheta")`

### Important Notes
- All actors should be `StaticMeshActor` type (NOT Blueprint actors)
- Use `safe_spawn_actor` from `actor_name_manager` for unique naming
- Physics should be OFF (these are static environment pieces)
- The `scale` parameter divides by 100 since engine primitives are 100 units base
- Follow existing patterns in `castle_creation.py` for error handling and actor tracking
