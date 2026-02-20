"""
Build ModularSciFi mesh catalog from filesystem.
Run with: uv run python build_catalog.py
"""
import json
import os
from collections import Counter

CONTENT_ROOT = r"F:\UE Projects\ThoughtSpace\Content\ModularSciFi\StaticMeshes"
GAME_ROOT = "/Game/ModularSciFi/StaticMeshes"
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "catalogs", "modularscifi_meshes.json")

# Category mapping: folder name -> category
CATEGORY_MAP = {
    # Terrain
    "Cliffs": "terrain", "Rocks": "terrain",
    "DirtPile_1": "terrain", "DirtPile_2": "terrain",
    "Mineral": "terrain", "Grass": "vegetation",
    # Modular building (grid-snapping pieces)
    "Modular_Floor": "modular_building",
    "Modular_Wall_1": "modular_building",
    "Modular_Door_Var1": "modular_building",
    "Modular_Corner": "modular_building",
    "Modular_WallCorner": "modular_building",
    "Modular_WallCorner_2": "modular_building",
    "Modular_Ceiling": "modular_building",
    "Modular_Ceiling_Window1": "modular_building",
    "Modular_Ceiling_Window2": "modular_building",
    "Modular_Ceiling_Window3": "modular_building",
    "Modular_WallWindow_1": "modular_building",
    "Modular_WallWindow_2": "modular_building",
    "Modular_WallWindow_3": "modular_building",
    "Modular_Parts": "modular_building",
    # Compound modules (pre-built rooms)
    "LivingModule_1": "compound_module",
    "LivingModule_2": "compound_module",
    "LivingModule_3": "compound_module",
    "LivingModule_4": "compound_module",
    "MainStation": "compound_module",
    # Connectors (join modules together)
    "Modular_BridgeConnector_1": "connector",
    "Modular_BridgeConnector_2": "connector",
    "Modular_BridgeConnector_3": "connector",
    "Modular_BridgeConnector_4": "connector",
    "Modular_BridgeConnector_5": "connector",
    "RoofConnector_1": "connector",
    "RoofConnector_2": "connector",
    "RoofConnector_3": "connector",
    "RoofConnector_4": "connector",
    "RoofConnector_5": "connector",
    # Interior (indoor detail pieces)
    "Modular_IndoorCorner": "interior",
    "Modular_IndoorWallWindow_1": "interior",
    "Modular_IndoorWallWindow_2": "interior",
    "Modular_IndoorWallWindow_3": "interior",
    "Modular_InteriorColumn": "interior",
    "Modular_InteriorDoorConnector": "interior",
    "Modular_InteriorWall": "interior",
    "Modular_Interior_Door": "interior",
    "Modular_Interior_Part_1": "interior",
    "Modular_Interior_Part_2": "interior",
    "Modular_Interior_Part_3": "interior",
    "Modular_Interior_Part_4": "interior",
    "Modular_Interior_WallPart_1": "interior",
    "Modular_Interior_WallPart_2": "interior",
    "IndoorPartition_1": "interior",
    "IndoorWallFence": "interior",
    "Locker": "interior",
    "Chair": "interior",
    # Infrastructure (fences, supports, platforms)
    "Fence_1": "infrastructure",
    "Fence_2": "infrastructure",
    "Fence_3": "infrastructure",
    "Pillar_1": "infrastructure",
    "Pole": "infrastructure",
    "Modular_WallSupport_1": "infrastructure",
    "Modular_WallSupport_2": "infrastructure",
    "Modular_WallGrid": "infrastructure",
    "WallBracket": "infrastructure",
    "Platform_1": "infrastructure",
    "Platform_2": "infrastructure",
    "CircularPlatform": "infrastructure",
    "Base_1": "infrastructure",
    "Base_2": "infrastructure",
    "Balk_1": "infrastructure",
    # Stairs
    "Stair_Single": "stairs",
    "Stairs_1": "stairs", "Stairs_2": "stairs",
    "Stairs_3": "stairs", "Stairs_4": "stairs",
    "Stairs_5": "stairs", "Stairs_6": "stairs",
    # Props (small placeable objects)
    "Containers": "prop",
    "Lamps": "prop",
    "TechProps": "prop",
    "Modular_Lamp": "prop",
    "LightPanel": "prop",
    "WallPump": "prop",
    "Tank_1": "prop", "Tank_2": "prop",
    "Pump": "prop", "Pump_2": "prop",
    "Vent": "prop",
    "WallPanel_1": "prop",
    # Panels (structural detail surfaces)
    "Panel_1": "panel", "Panel_2": "panel",
    "Panel_3": "panel", "Panel_4": "panel",
    "Panel_5": "panel",
    # Tubes (piping system)
    "Tubes": "tubes",
    # Construction (scaffolding/temporary)
    "Construction_1": "construction", "Construction_2": "construction",
    "Construction_3": "construction", "Construction_4": "construction",
    "Construction_5": "construction", "Construction_6": "construction",
    "Construction_7": "construction", "Construction_8": "construction",
    "Construction_9": "construction",
    # Doors (standalone, not modular)
    "Door1": "door", "Door_2": "door", "DoorFrame_1": "door",
    # Cargo containers
    "CargoContainer_1": "cargo", "CargoContainer_2": "cargo",
    # Decal planes
    "DecalsPlanes": "decal",
    # Shared/utility meshes
    "ShareMeshes": "utility",
    "BPMeshes": "utility",
    "ProceduralStaticMesh": "procedural",
}

# Tags for kitbashing context
TAGS = {
    "modular_building": ["grid_500cm", "snap", "structure"],
    "compound_module": ["pre_built", "paired_interior", "structure"],
    "connector": ["joins_modules", "structure"],
    "interior": ["indoor", "detail"],
    "terrain": ["scatter", "random_scale", "random_rotation"],
    "vegetation": ["scatter", "random_scale", "random_rotation"],
    "infrastructure": ["perimeter", "path_follow"],
    "stairs": ["vertical_connection", "structure"],
    "prop": ["cluster", "detail", "yaw_only"],
    "panel": ["surface_detail", "arbitrary_rotation"],
    "tubes": ["path_follow", "detail"],
    "construction": ["scaffolding", "detail"],
    "door": ["structure", "snap"],
    "cargo": ["prop", "large"],
    "decal": ["surface_detail", "flat"],
    "utility": ["system"],
    "procedural": ["terrain", "path"],
}


def build_catalog():
    meshes = []

    for root, dirs, files in os.walk(CONTENT_ROOT):
        for f in sorted(files):
            if not f.endswith(".uasset"):
                continue

            name = f.replace(".uasset", "")
            rel = os.path.relpath(root, CONTENT_ROOT).replace(os.sep, "/")

            # Build UE5 asset path
            if rel == ".":
                ue_path = f"{GAME_ROOT}/{name}"
            else:
                ue_path = f"{GAME_ROOT}/{rel}/{name}"
            full_path = f"{ue_path}.{name}"

            # Determine folder hierarchy
            parts = rel.split("/")
            top_folder = parts[0] if parts[0] != "." else "Root"
            sub_folder = parts[1] if len(parts) > 1 else ""

            # Resolve category
            lookup_key = sub_folder if sub_folder else top_folder
            category = CATEGORY_MAP.get(lookup_key, "uncategorized")

            # Check for interior pair indicator
            is_indoor = "Indoor" in name or "Interior" in name
            has_pair = False
            if category == "compound_module" and not is_indoor:
                # Check if an indoor variant exists in same folder
                indoor_candidates = [
                    ff for ff in files
                    if ff.endswith(".uasset") and "Indoor" in ff and ff != f
                ]
                has_pair = len(indoor_candidates) > 0

            mesh_entry = {
                "name": name,
                "path": full_path,
                "base_path": ue_path,
                "category": category,
                "tags": TAGS.get(category, []),
                "folder": top_folder,
                "subfolder": sub_folder or top_folder,
            }

            if is_indoor:
                mesh_entry["is_indoor_variant"] = True
            if has_pair:
                mesh_entry["has_indoor_pair"] = True

            # Bounds placeholder — to be filled by measure_bounds.py
            mesh_entry["bounds_extent"] = None
            mesh_entry["size_cm"] = None

            meshes.append(mesh_entry)

    # Sort by category then name
    meshes.sort(key=lambda m: (m["category"], m["name"]))

    # Summary
    cats = Counter(m["category"] for m in meshes)

    catalog = {
        "version": "1.0",
        "asset_pack": "ModularSciFi",
        "description": "Complete mesh catalog for the ModularSciFi asset pack. "
                       "Paths are ready for spawn_static_mesh_actor. "
                       "Bounds are null until measured via measure_bounds.py.",
        "base_path": GAME_ROOT,
        "total_meshes": len(meshes),
        "category_counts": dict(cats.most_common()),
        "category_descriptions": {
            "modular_building": "Grid-snapping structural pieces (500cm). Walls, floors, roofs, corners, windows, doors.",
            "compound_module": "Pre-built room modules (LivingModules, MainStation). Always paired with indoor variant at same transform.",
            "connector": "Bridge and roof connectors that join compound modules together.",
            "interior": "Indoor detail pieces: partitions, columns, indoor windows, furniture.",
            "terrain": "Cliffs, rocks, dirt, minerals. Use random scale (0.5-6x) and full 3-axis rotation.",
            "vegetation": "Grass and plants. Scatter with random scale/rotation.",
            "infrastructure": "Fences, pillars, poles, platforms, supports. Often placed along paths/perimeters.",
            "stairs": "Staircase variants for vertical connections.",
            "prop": "Small objects: barrels, boxes, lamps, terminals, pumps. Cluster near structures.",
            "panel": "Flat structural detail surfaces. Used at arbitrary rotations for wall/floor detail.",
            "tubes": "Piping system with 23 variants for complex routing.",
            "construction": "Scaffolding and temporary construction elements.",
            "door": "Standalone door and frame meshes (not modular grid doors).",
            "cargo": "Large cargo containers.",
            "decal": "Flat planes for projected surface detail.",
            "utility": "System meshes (skybox, water plane, splines).",
            "procedural": "Off-road/terrain meshes for procedural placement.",
        },
        "meshes": meshes,
    }

    # Write
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(catalog, f, indent=2)

    print(f"Catalog written to: {OUTPUT_PATH}")
    print(f"Total meshes: {len(meshes)}")
    print(f"\nCategory breakdown:")
    for c, n in cats.most_common():
        print(f"  {c}: {n}")


if __name__ == "__main__":
    build_catalog()
