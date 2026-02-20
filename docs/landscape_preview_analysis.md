# LandscapePreview Level Analysis — Learnings for ReKit-MCP

**Date:** 2026-02-20
**Level:** ModularSciFi / LandscapePreview
**Total Actors:** 2,500 (2,376 StaticMeshActors, 102 DecalActors, 22 system actors)

> **Important:** This entire scene — compound structures, terrain, props, infrastructure, everything — was built using only the ModularSciFi asset pack (with the possible exception of landscape base meshes). The same assets are available in the OverviewScene level. This demonstrates what a single well-designed asset pack can achieve when the composition patterns are understood.

---

## 1. Scene Composition Breakdown

Sampled 238 of 2,376 StaticMeshActors (every 10th) with full `get_actor_details` queries.

### Mesh Usage by Category

| Category | Sample Count | Est. Full Level | % of Scene |
|----------|-------------|----------------|-----------|
| **Terrain** (Cliffs, Rocks, DirtPiles) | 79 | ~790 | **33%** |
| **Minerals** (ore deposits, crystals) | 25 | ~250 | **11%** |
| **Panels** (structural detail surfaces) | 27 | ~270 | **11%** |
| **Fences** (perimeter/path definition) | 17 | ~170 | **7%** |
| **Props** (barrels, boxes, terminals, lamps) | 26 | ~260 | **11%** |
| **Compound Modules** (LivingModules, MainStation) | 10 | ~100 | **4%** |
| **Connectors** (RoofConnectors, BridgeConnectors) | 5 | ~50 | **2%** |
| **Modular Building** (walls, floors, doors, roofs) | 8 | ~80 | **3%** |
| **Infrastructure** (pillars, poles, tubes, stairs) | 10 | ~100 | **4%** |
| **Other** (construction, indoor, skybox, water, cube) | ~31 | ~310 | **13%** |

### Key Insight: Buildings are only ~9% of the scene

The actual structures (compound modules + modular building pieces + connectors) make up ~9% of actors. **Terrain and environmental dressing make up 73%+.** A kitbashing tool that only builds structures misses the vast majority of scene composition work.

---

## 2. Top 20 Most-Used Meshes

| Rank | Mesh | Count | Category |
|------|------|-------|----------|
| 1 | SM_Cliff_1 | 45x | Terrain |
| 2 | SM_Panel_1_Merged | 20x | Structural |
| 3 | SM_Rock_9 | 19x | Terrain |
| 4 | SM_Fence_1_Merged | 17x | Infrastructure |
| 5 | SM_Cliff_2 | 15x | Terrain |
| 6 | SM_Mineral_1 | 14x | Environment |
| 7 | SM_Cliff_3 | 10x | Terrain |
| 8 | SM_Mineral_3 | 9x | Environment |
| 9 | SM_Lamp_1 | 8x | Props |
| 10 | SM_Box_1/SM_Box_2 | 5x each | Props |
| 11 | SM_Panel_Merged | 5x | Structural |
| 12 | SM_Barrel_2 | 4x | Props |
| 13 | SM_Pillar_1_Merged | 4x | Infrastructure |
| 14 | SM_DirtPile_1 | 3x | Terrain |
| 15 | SM_Rock_1 | 3x | Terrain |

---

## 3. Placement Patterns

### Scale Distribution
| Pattern | Count | % | Description |
|---------|-------|---|-------------|
| Uniform 1.0 | 161 | 68% | Standard size (buildings, props, panels) |
| Uniform non-1.0 | 63 | 26% | Varied size (terrain, minerals) |
| Non-uniform | 12 | 5% | Stretched (landscape, special shapes) |
| Mirrored (negative) | 2 | 1% | Flipped for variety |

**Terrain pieces use random uniform scales** (0.59 to 5.9x) to create natural-looking variety from a small mesh set. The same SM_Cliff_1 appears at many different sizes.

### Rotation Distribution
| Pattern | Count | % | Description |
|---------|-------|---|-------------|
| Yaw-only | 136 | 57% | Flat placement (buildings, fences, props) |
| With Pitch/Roll | 102 | 43% | Tilted (terrain, rocks, some panels) |

**43% of actors use pitch/roll rotation.** This is primarily terrain pieces conforming to slopes, and panels used as horizontal surfaces or angled detail.

### Material Assignment
The top materials by usage:
1. **MI_DecalsColor2** (72x) — Colored surface detail
2. **MI_Cliffs1** (70x) — Cliff/rock surfaces
3. **MI_PaintedMetal2** (66x) — Metal structural surfaces
4. **MI_GraphicalDecals** (63x) — Graphic overlays
5. **MI_Decals** (56x) — Surface weathering/detail

Most meshes use **3-11 material slots** simultaneously. Panels alone have 7 material slots each, allowing one mesh to show multiple surface types.

---

## 4. Spatial Organization

### Compound Structure (Center of Level)
The main outpost compound at ~(0, 60000) is built from:
- **LivingModules 1-4** — Each as a paired exterior + interior mesh at identical transforms
- **MainStation** — Central hub (also exterior + interior pair)
- **RoofConnectors 1-5** — Bridge the gaps between modules at roof level
- **BridgeConnectors** — Ground-level connections

**Critical Pattern: Paired meshes.** Each habitable space uses TWO actors at the same location — the exterior shell and the interior detail. This is different from our modular approach of assembling from wall/floor/roof pieces.

### Terrain Layering (Everywhere)
Terrain follows clear visual layering:
1. **Landscape actor** — Base terrain mesh
2. **Large cliffs** (scale 2.0-6.0) — Define major terrain features
3. **Medium cliffs/rocks** (scale 1.0-2.0) — Break up silhouettes
4. **Small rocks** (scale 0.5-1.5) — Surface detail
5. **Minerals** (scale 1.5-2.5) — Gameplay/visual interest points
6. **Dirt piles** — Transition between ground types

### Density Clusters
| Region | Actor Count | Primary Content |
|--------|-------------|-----------------|
| (0, 60000) | 190 | Main compound + surrounding detail |
| (-5000, 60000) | 190 | Compound extension + terrain |
| (-15000, 55000) | 156 | Terrain features |
| (-10000, 70000) | 155 | Secondary structures |
| (0, 45000) | 152 | Terrain + infrastructure |

---

## 5. Actionable ReKit-MCP Improvements

### 5A. Scatter/Terrain Tools (Highest Impact)

**Problem:** 53% of scene actors are terrain pieces (cliffs, rocks, minerals) placed with randomized scale and rotation. Currently this requires one `spawn_static_mesh_actor` call per piece.

**Proposed Tool: `scatter_meshes`**
```python
scatter_meshes(
    meshes=["SM_Cliff_1", "SM_Cliff_2", "SM_Rock_9"],  # mesh pool
    region=[x, y, z, radius],          # placement area
    count=20,                           # how many to place
    scale_range=[0.8, 3.0],            # random uniform scale
    rotation_mode="terrain",            # full 3-axis random vs yaw-only
    ground_snap=True,                   # trace down to landscape
    name_prefix="Terrain_Cliff",
    seed=42                             # reproducible randomization
)
```

This single call replaces 20+ individual spawn calls and handles:
- Random mesh selection from pool
- Random scale within range
- Random rotation (full 3-axis for terrain, yaw-only for props)
- Ground snapping via downward trace
- Seed-based reproducibility

### 5B. Prop Cluster Tool (High Impact)

**Problem:** Props (barrels, boxes, lamps, terminals) appear in natural clusters around structures. Currently placed one at a time.

**Proposed Tool: `place_prop_cluster`**
```python
place_prop_cluster(
    props=["SM_Barrel_1", "SM_Barrel_2", "SM_Box_1"],
    center=[x, y, z],
    radius=300,                # cluster spread
    count=5,
    scale_range=[0.9, 1.1],   # slight variation
    rotation_mode="yaw_only",  # props stay upright
    name_prefix="Props_Storage"
)
```

### 5C. Fence/Path Tool (Medium Impact)

**Problem:** Fences follow paths/perimeters. Currently need to manually calculate positions along a line.

**Proposed Tool: `place_along_path`**
```python
place_along_path(
    mesh="SM_Fence_1_Merged",
    points=[[x1,y1,z1], [x2,y2,z2], [x3,y3,z3]],
    spacing=200,              # cm between instances
    align_to_path=True,       # auto-rotate to face along path
    ground_snap=True,
    name_prefix="Fence"
)
```

### 5D. Paired Mesh Spawning (Medium Impact)

**Problem:** LivingModules use exterior+interior mesh pairs at the same transform. No way to spawn paired meshes as a unit.

**Enhancement to `spawn_static_mesh_actor` or new tool:**
```python
spawn_paired_meshes(
    name="Hab_Module_01",
    exterior_mesh="SM_LivingModule_1_Merged",
    interior_mesh="SM_LivingModule_1_Indoor_Merged",
    location=[x, y, z],
    rotation=[0, -180, 0]
)
```

### 5E. Asset Registry / Mesh Catalog (Foundation)

**Problem:** `list_content_browser_meshes` is slow and returns raw paths. No way to quickly find the right mesh for a purpose.

**Proposed Enhancement: Pre-built mesh catalog**
A JSON catalog of all ModularSciFi meshes organized by category, with:
- Short name → full path mapping
- Category tags (terrain, structural, prop, infrastructure)
- Bounding box dimensions (pre-measured)
- Recommended scale ranges
- Material slot names

This lets the AI choose meshes without querying UE5, and provides the metadata needed for scatter/cluster tools.

### 5F. Decal Placement Tool (Lower Priority)

102 decals in this level for weathering and detail. Future tool for surface decal scattering.

---

## 6. Priority Order for Implementation

1. **Mesh Catalog** (5E) — Foundation that all other tools need. Build JSON catalog of ModularSciFi meshes with categories, paths, and dimensions. ~2 hours.

2. **Scatter Tool** (5A) — Highest impact. Covers 53% of scene composition work. Requires ground-snap via line trace. ~4 hours.

3. **Prop Cluster** (5B) — Quick win, simpler version of scatter. ~2 hours.

4. **Fence/Path** (5C) — Enables perimeter definition. ~3 hours.

5. **Paired Spawn** (5D) — Enhancement for compound modules. ~1 hour.

6. **Decals** (5F) — Future polish layer. ~2 hours.

---

## 7. Architecture Note

All new tools should follow the existing pattern:
- Helper modules in `Python/helpers/` (like `hab_spawner.py`)
- Tool registration in `unreal_mcp_server_advanced.py` via `@mcp.tool()`
- Sequential spawning with delays (no parallel UE5 calls)
- Auto-validation screenshots via `_validate_build_internal()`
- JSON variation files for pre-verified layouts

The scatter/cluster tools are conceptually similar to `spawn_hab` but with randomization instead of pre-defined transforms. The hab spawner is actually a special case of a more general "spawn from template" pattern.
