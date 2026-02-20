"""
Hab Spawner — loads pre-verified JSON variation files and spawns complete
habitats in Unreal Engine via sequential safe_spawn_actor calls.

Each JSON file in the variations directory defines a complete hab layout:
mesh types, relative positions, rotations, and full asset paths. This module
handles rotation math, mesh path resolution, and sequential spawning with
configurable inter-spawn delays to keep UE5 stable.
"""

import json
import math
import os
import time
import logging
from typing import Dict, Any, List, Optional

logger = logging.getLogger(__name__)

# Graceful import — helper may be used standalone for testing
try:
    from .actor_name_manager import safe_spawn_actor
except ImportError:
    logger.warning("Could not import safe_spawn_actor; spawning will be unavailable")
    safe_spawn_actor = None

# Default location for variation JSON files
DEFAULT_VARIATIONS_DIR = r"F:\UE Projects\ThoughtSpace\docs\hab_variations"

# Delay between sequential spawn calls (seconds). The MCP server already adds
# 50ms per command; this extra buffer keeps the UE5 game thread comfortable
# when spawning 14-26 pieces in rapid succession.
DEFAULT_SPAWN_DELAY = 0.1


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def list_variations(variations_dir: Optional[str] = None) -> Dict[str, Any]:
    """
    Scan a directory for variation_*.json files and return a summary of each.

    Returns:
        Dict with "success", "count", and "variations" list.
    """
    vdir = _resolve_variations_dir(variations_dir)
    if not os.path.isdir(vdir):
        return {
            "success": False,
            "message": f"Variations directory not found: {vdir}",
        }

    results = []
    pattern = "variation_"
    try:
        for fname in sorted(os.listdir(vdir)):
            if not fname.startswith(pattern) or not fname.endswith(".json"):
                continue
            fpath = os.path.join(vdir, fname)
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    data = json.load(f)
                results.append({
                    "file": fname,
                    "name": data.get("name", fname),
                    "description": data.get("description", ""),
                    "piece_count": len(data.get("pieces", [])),
                    "cell_count": len(data.get("cells", [])),
                })
            except (json.JSONDecodeError, IOError) as e:
                logger.warning(f"Skipping {fname}: {e}")
                results.append({"file": fname, "error": str(e)})
    except OSError as e:
        return {"success": False, "message": f"Cannot read directory: {e}"}

    return {
        "success": True,
        "count": len(results),
        "variations_dir": vdir,
        "variations": results,
    }


def spawn_hab_from_variation(
    unreal_connection,
    variation: str,
    location: List[float],
    rotation: float = 0.0,
    name_prefix: str = "Hab",
    variations_dir: Optional[str] = None,
    spawn_delay: float = DEFAULT_SPAWN_DELAY,
) -> Dict[str, Any]:
    """
    Spawn a complete hab from a variation JSON file.

    Args:
        unreal_connection: Active UnrealConnection instance.
        variation: Variation identifier — filename, partial key, or JSON name.
        location: [x, y, z] world position for the hab origin.
        rotation: Yaw rotation in degrees to apply to the entire hab.
        name_prefix: Prefix for spawned actor names (e.g. "Camp_Hab01").
        variations_dir: Override for the variations directory.
        spawn_delay: Seconds to wait between each spawn call (default 0.1).

    Returns:
        Dict with success, spawned list, failed list, and summary.
    """
    if safe_spawn_actor is None:
        return {"success": False, "message": "safe_spawn_actor not available"}

    if not unreal_connection:
        return {"success": False, "message": "No Unreal connection provided"}

    # --- Load variation data ---
    vdir = _resolve_variations_dir(variations_dir)
    vfile = _find_variation_file(variation, vdir)
    if vfile is None:
        available = _available_variation_names(vdir)
        return {
            "success": False,
            "message": f"Variation '{variation}' not found in {vdir}",
            "available": available,
        }

    try:
        with open(vfile, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        return {"success": False, "message": f"Failed to load {vfile}: {e}"}

    pieces = data.get("pieces", [])
    mesh_paths = data.get("mesh_paths", {})
    if not pieces:
        return {"success": False, "message": "Variation file has no pieces"}

    # --- Spawn each piece sequentially ---
    hab_rotation_rad = math.radians(rotation)
    origin_x, origin_y, origin_z = float(location[0]), float(location[1]), float(location[2])

    spawned: List[Dict[str, Any]] = []
    failed: List[Dict[str, Any]] = []

    for i, piece in enumerate(pieces):
        rel = piece.get("rel", [0, 0, 0])
        piece_yaw = float(piece.get("yaw", 0))
        mesh_short = piece.get("mesh", "")
        label = piece.get("label", f"piece_{piece.get('index', i)}")

        # Resolve full mesh path
        full_mesh = _resolve_mesh_path(mesh_short, mesh_paths)
        if full_mesh is None:
            failed.append({
                "index": piece.get("index", i),
                "label": label,
                "error": f"Unknown mesh: {mesh_short}",
            })
            continue

        # Apply hab rotation to relative position and yaw
        world_x, world_y, world_z, final_yaw = _rotate_piece(
            float(rel[0]), float(rel[1]), float(rel[2]),
            piece_yaw, rotation, hab_rotation_rad,
            origin_x, origin_y, origin_z,
        )

        # Sanitize label for actor name
        safe_label = label.replace(" ", "_").replace("(", "").replace(")", "")
        actor_name = f"{name_prefix}_{safe_label}"

        params = {
            "name": actor_name,
            "type": "StaticMeshActor",
            "location": [world_x, world_y, world_z],
            "rotation": [0.0, final_yaw, 0.0],
            "static_mesh": full_mesh,
        }

        logger.info(
            f"Spawning piece {i+1}/{len(pieces)}: {label} at "
            f"({world_x:.0f}, {world_y:.0f}, {world_z:.0f}) yaw={final_yaw:.1f}"
        )

        resp = safe_spawn_actor(unreal_connection, params)

        if resp and resp.get("status") == "success":
            spawned.append({
                "index": piece.get("index", i),
                "label": label,
                "actor_name": params["name"],
                "location": [world_x, world_y, world_z],
                "yaw": final_yaw,
            })
        else:
            error_msg = "No response"
            if resp:
                error_msg = resp.get("error", resp.get("message", str(resp)))
            failed.append({
                "index": piece.get("index", i),
                "label": label,
                "error": error_msg,
            })

        # Delay between spawns to keep UE5 game thread stable
        if spawn_delay > 0 and i < len(pieces) - 1:
            time.sleep(spawn_delay)

    # --- Summary ---
    total = len(pieces)
    success = len(failed) == 0
    summary = (
        f"Spawned {len(spawned)}/{total} pieces for '{data.get('name', variation)}'"
    )
    if failed:
        summary += f" ({len(failed)} failed)"

    return {
        "success": success,
        "message": summary,
        "variation_name": data.get("name", variation),
        "total_pieces": total,
        "spawned_count": len(spawned),
        "failed_count": len(failed),
        "spawned": spawned,
        "failed": failed,
        "location": location,
        "rotation": rotation,
        "name_prefix": name_prefix,
    }


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _rotate_piece(
    rel_x: float, rel_y: float, rel_z: float,
    piece_yaw: float, hab_rotation_deg: float, hab_rotation_rad: float,
    origin_x: float, origin_y: float, origin_z: float,
) -> tuple:
    """
    Apply hab rotation to a piece's relative position and yaw.

    Uses standard 2D rotation matrix on (rel_x, rel_y), adds origin offset,
    and adds hab rotation to piece yaw.

    Returns:
        (world_x, world_y, world_z, final_yaw)
    """
    cos_r = math.cos(hab_rotation_rad)
    sin_r = math.sin(hab_rotation_rad)

    # Rotate relative position around origin
    rotated_x = rel_x * cos_r - rel_y * sin_r
    rotated_y = rel_x * sin_r + rel_y * cos_r

    world_x = origin_x + rotated_x
    world_y = origin_y + rotated_y
    world_z = origin_z + rel_z

    # Combine yaw: piece yaw + hab rotation
    final_yaw = piece_yaw + hab_rotation_deg

    # Normalize to [-180, 180)
    while final_yaw >= 180:
        final_yaw -= 360
    while final_yaw < -180:
        final_yaw += 360

    return world_x, world_y, world_z, final_yaw


def _resolve_mesh_path(short_name: str, mesh_paths: Dict[str, str]) -> Optional[str]:
    """
    Resolve a short mesh name to a full UE5 asset path with .AssetName suffix.

    The JSON mesh_paths map short names (e.g. "SM_Modular_Merged") to paths
    like "/Game/.../SM_Modular_Merged". UE5 spawn_actor needs the full path
    with the asset name appended after a dot.
    """
    base_path = mesh_paths.get(short_name)
    if base_path is None:
        return None

    # Append .AssetName if not already present
    # e.g. "/Game/.../SM_Modular_Merged" → "/Game/.../SM_Modular_Merged.SM_Modular_Merged"
    asset_name = base_path.rsplit("/", 1)[-1]
    if "." not in base_path.rsplit("/", 1)[-1]:
        return f"{base_path}.{asset_name}"
    return base_path


def _resolve_variations_dir(override: Optional[str] = None) -> str:
    """Get the variations directory from override, env var, or default."""
    if override:
        return override
    return os.environ.get("HAB_VARIATIONS_DIR", DEFAULT_VARIATIONS_DIR)


def _find_variation_file(variation: str, vdir: str) -> Optional[str]:
    """
    Find a variation JSON file using fuzzy matching.

    Tries in order:
    1. Exact filename: variation_<variation>.json
    2. Case-insensitive partial match on filename
    3. Match against the JSON "name" field inside each file
    """
    if not os.path.isdir(vdir):
        return None

    # 1. Exact filename match
    exact = os.path.join(vdir, f"variation_{variation}.json")
    if os.path.isfile(exact):
        return exact

    # Normalize search term for fuzzy matching
    search_lower = variation.lower().replace("-", "_").replace(" ", "_")

    # 2. Case-insensitive partial match on filename
    for fname in os.listdir(vdir):
        if not fname.startswith("variation_") or not fname.endswith(".json"):
            continue
        # Extract the key part: variation_<KEY>.json
        key = fname[len("variation_"):-len(".json")].lower()
        if search_lower in key or key in search_lower:
            return os.path.join(vdir, fname)

    # 3. Match against the "name" field inside JSON files
    for fname in os.listdir(vdir):
        if not fname.startswith("variation_") or not fname.endswith(".json"):
            continue
        fpath = os.path.join(vdir, fname)
        try:
            with open(fpath, "r", encoding="utf-8") as f:
                data = json.load(f)
            name = data.get("name", "").lower().replace("-", "_").replace(" ", "_")
            if search_lower in name or name in search_lower:
                return fpath
        except (json.JSONDecodeError, IOError):
            continue

    return None


def _available_variation_names(vdir: str) -> List[str]:
    """Return a list of available variation keys for error messages."""
    if not os.path.isdir(vdir):
        return []
    names = []
    for fname in sorted(os.listdir(vdir)):
        if fname.startswith("variation_") and fname.endswith(".json"):
            key = fname[len("variation_"):-len(".json")]
            names.append(key)
    return names
