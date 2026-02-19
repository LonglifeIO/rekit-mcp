"""
Outpost compound creation helper functions for Unreal MCP Server.
Builds a sci-fi mining outpost from primitive shapes (cubes, cylinders, cones, spheres).
Designed for ThoughtSpace / Outpost Theta — industrial, arctic, isolated feel.

Visual reference: Low wide rectangular hab modules with glowing orange window panels,
rooftop solar arrays, side-mounted cylindrical tanks, tall antenna masts.
"""

import math
from typing import List, Dict, Any
import logging

logger = logging.getLogger(__name__)

# Import safe spawning functions
try:
    from .actor_name_manager import safe_spawn_actor
except ImportError:
    logger.warning("Could not import actor_name_manager, using fallback spawning")
    def safe_spawn_actor(unreal_connection, params, auto_unique_name=True):
        return unreal_connection.send_command("spawn_actor", params)


CUBE = "/Engine/BasicShapes/Cube.Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
CONE = "/Engine/BasicShapes/Cone.Cone"
SPHERE = "/Engine/BasicShapes/Sphere.Sphere"


def _spawn(unreal, params, all_actors):
    """Spawn a StaticMeshActor and track results."""
    resp = safe_spawn_actor(unreal, params, auto_unique_name=True)
    if resp and resp.get("status") == "success":
        all_actors.append(resp.get("result"))
    return resp


def get_outpost_size_params(compound_size: str) -> Dict[str, float]:
    """Get dimension multipliers for different outpost sizes."""
    sizes = {
        "small":  {"scale": 0.7},
        "medium": {"scale": 1.0},
        "large":  {"scale": 1.4},
    }
    return sizes.get(compound_size, sizes["medium"])


# ── A. Hab Modules (Main + Secondary) ─────────────────────────────────────
# Low, wide rectangular boxes — elongated prefab look matching concept art

def _build_hab_module(unreal, prefix: str, name: str, loc: List[float],
                      ox: float, oy: float, w: float, d: float, h: float,
                      s: float, actors: List) -> None:
    """Build a single hab module with window, solar panel, and side tank."""
    ground = loc[2]
    bx = loc[0] + ox
    by = loc[1] + oy

    # Main body
    _spawn(unreal, {
        "name": f"{prefix}_{name}",
        "type": "StaticMeshActor",
        "location": [bx, by, ground + h / 2],
        "scale": [w / 100, d / 100, h / 100],
        "static_mesh": CUBE
    }, actors)

    # Glowing orange window panel on front face (wide horizontal slit)
    win_w = w * 0.5
    win_h = h * 0.25
    _spawn(unreal, {
        "name": f"{prefix}_{name}_Window",
        "type": "StaticMeshActor",
        "location": [bx, by + d / 2 + 1, ground + h * 0.55],
        "scale": [win_w / 100, 0.04, win_h / 100],
        "static_mesh": CUBE
    }, actors)

    # Rooftop solar panel (flat rectangle, slightly offset)
    _spawn(unreal, {
        "name": f"{prefix}_{name}_Solar",
        "type": "StaticMeshActor",
        "location": [bx + w * 0.15, by - d * 0.1, ground + h + 5 * s],
        "scale": [w * 0.6 / 100, d * 0.4 / 100, 0.04],
        "static_mesh": CUBE
    }, actors)

    # Side-mounted cylindrical tank
    _spawn(unreal, {
        "name": f"{prefix}_{name}_Tank",
        "type": "StaticMeshActor",
        "location": [bx + w / 2 + 25 * s, by, ground + h * 0.35],
        "scale": [0.35 * s, 0.8 * s, 0.35 * s],
        "static_mesh": CYLINDER
    }, actors)


def build_hab_modules(unreal, prefix: str, loc: List[float],
                      size: Dict[str, float], actors: List) -> None:
    """Build all hab modules — 1 main command + 3 secondary."""
    logger.info("Building hab modules...")
    s = size["scale"]

    # Main command hab — largest, center-back
    _build_hab_module(unreal, prefix, "HabCommand", loc,
                      ox=0, oy=-200 * s,
                      w=500 * s, d=250 * s, h=160 * s,
                      s=s, actors=actors)

    # Secondary habs — smaller, spread around
    habs = [
        {"name": "HabStorage",  "ox":  420 * s, "oy": -80 * s,
         "w": 350 * s, "d": 200 * s, "h": 130 * s},
        {"name": "HabMedBay",   "ox":  350 * s, "oy":  280 * s,
         "w": 300 * s, "d": 180 * s, "h": 125 * s},
        {"name": "HabCrew",     "ox": -380 * s, "oy":  200 * s,
         "w": 380 * s, "d": 220 * s, "h": 140 * s},
    ]
    for h in habs:
        _build_hab_module(unreal, prefix, h["name"], loc,
                          ox=h["ox"], oy=h["oy"],
                          w=h["w"], d=h["d"], h=h["h"],
                          s=s, actors=actors)


# ── B. Communications Tower ───────────────────────────────────────────────

def build_comms_tower(unreal, prefix: str, loc: List[float],
                      size: Dict[str, float], actors: List) -> None:
    """Tall comms tower — lattice mast with dish and antenna tip."""
    logger.info("Building communications tower...")
    s = size["scale"]
    tx = loc[0] + 650 * s
    ty = loc[1] - 400 * s
    ground = loc[2]

    # Base pad
    _spawn(unreal, {
        "name": f"{prefix}_CommsTower_Base",
        "type": "StaticMeshActor",
        "location": [tx, ty, ground + 10 * s],
        "scale": [0.6 * s, 0.6 * s, 0.2 * s],
        "static_mesh": CUBE
    }, actors)

    # Main mast
    mast_h = 700 * s
    _spawn(unreal, {
        "name": f"{prefix}_CommsTower_Mast",
        "type": "StaticMeshActor",
        "location": [tx, ty, ground + 20 * s + mast_h / 2],
        "scale": [0.2 * s, 0.2 * s, mast_h / 100],
        "static_mesh": CYLINDER
    }, actors)

    # Cross struts (2 horizontal bars for lattice feel)
    for i, frac in enumerate([0.35, 0.65]):
        strut_z = ground + 20 * s + mast_h * frac
        _spawn(unreal, {
            "name": f"{prefix}_CommsTower_Strut_{i}",
            "type": "StaticMeshActor",
            "location": [tx, ty, strut_z],
            "scale": [0.8 * s, 0.08 * s, 0.08 * s],
            "static_mesh": CUBE
        }, actors)

    # Dish (flattened sphere at top)
    dish_z = ground + 20 * s + mast_h * 0.75
    _spawn(unreal, {
        "name": f"{prefix}_CommsTower_Dish",
        "type": "StaticMeshActor",
        "location": [tx + 40 * s, ty, dish_z],
        "scale": [0.8 * s, 0.8 * s, 0.2 * s],
        "static_mesh": SPHERE
    }, actors)

    # Antenna tip (cone above mast)
    _spawn(unreal, {
        "name": f"{prefix}_CommsTower_Tip",
        "type": "StaticMeshActor",
        "location": [tx, ty, ground + 20 * s + mast_h + 40 * s],
        "scale": [0.15 * s, 0.15 * s, 0.8 * s],
        "static_mesh": CONE
    }, actors)


# ── C. Radio Antenna Array ────────────────────────────────────────────────

def build_antenna_array(unreal, prefix: str, loc: List[float],
                        size: Dict[str, float], actors: List) -> None:
    """Cluster of thin antenna masts at varying heights."""
    logger.info("Building antenna array...")
    s = size["scale"]
    base_x = loc[0] - 550 * s
    base_y = loc[1] - 350 * s
    ground = loc[2]

    masts = [
        {"ox": 0,        "oy": 0,        "h": 500 * s, "r": 0.12},
        {"ox": 80 * s,   "oy": 50 * s,   "h": 650 * s, "r": 0.10},
        {"ox": -60 * s,  "oy": 90 * s,   "h": 400 * s, "r": 0.10},
        {"ox": 40 * s,   "oy": -60 * s,  "h": 550 * s, "r": 0.08},
        {"ox": -30 * s,  "oy": -40 * s,  "h": 350 * s, "r": 0.12},
    ]

    for i, m in enumerate(masts):
        ax = base_x + m["ox"]
        ay = base_y + m["oy"]
        h = m["h"]

        # Pole
        _spawn(unreal, {
            "name": f"{prefix}_Antenna_{i}_Pole",
            "type": "StaticMeshActor",
            "location": [ax, ay, ground + h / 2],
            "scale": [m["r"] * s, m["r"] * s, h / 100],
            "static_mesh": CYLINDER
        }, actors)

        # Tip — cone for taller masts, sphere for shorter
        if h > 450 * s:
            _spawn(unreal, {
                "name": f"{prefix}_Antenna_{i}_Tip",
                "type": "StaticMeshActor",
                "location": [ax, ay, ground + h + 20 * s],
                "scale": [0.1 * s, 0.1 * s, 0.4 * s],
                "static_mesh": CONE
            }, actors)
        else:
            _spawn(unreal, {
                "name": f"{prefix}_Antenna_{i}_Tip",
                "type": "StaticMeshActor",
                "location": [ax, ay, ground + h + 8 * s],
                "scale": [0.15 * s, 0.15 * s, 0.15 * s],
                "static_mesh": SPHERE
            }, actors)


# ── D. Parking / Landing Pad ──────────────────────────────────────────────

def build_parking_pad(unreal, prefix: str, loc: List[float],
                      size: Dict[str, float], actors: List) -> None:
    """Flat vehicle parking slab with corner markers."""
    logger.info("Building parking pad...")
    s = size["scale"]
    px = loc[0] + 50 * s
    py = loc[1] + 550 * s
    ground = loc[2]

    pw, pd = 500 * s, 400 * s
    _spawn(unreal, {
        "name": f"{prefix}_ParkingPad",
        "type": "StaticMeshActor",
        "location": [px, py, ground + 3],
        "scale": [pw / 100, pd / 100, 0.06],
        "static_mesh": CUBE
    }, actors)

    # Corner bollards
    corners = [
        (px - pw / 2, py - pd / 2),
        (px + pw / 2, py - pd / 2),
        (px - pw / 2, py + pd / 2),
        (px + pw / 2, py + pd / 2),
    ]
    for j, (cx, cy) in enumerate(corners):
        _spawn(unreal, {
            "name": f"{prefix}_ParkingBollard_{j}",
            "type": "StaticMeshActor",
            "location": [cx, cy, ground + 30 * s],
            "scale": [0.15 * s, 0.15 * s, 0.6 * s],
            "static_mesh": CYLINDER
        }, actors)


# ── E. Storage Containers ─────────────────────────────────────────────────

def build_containers(unreal, prefix: str, loc: List[float],
                     size: Dict[str, float], actors: List) -> None:
    """Scattered shipping containers near buildings."""
    logger.info("Building storage containers...")
    s = size["scale"]
    ground = loc[2]

    containers = [
        {"ox": 500 * s,   "oy": 120 * s,  "w": 180 * s, "d": 80 * s, "h": 80 * s},
        {"ox": 530 * s,   "oy": 220 * s,  "w": 160 * s, "d": 75 * s, "h": 75 * s},
        {"ox": -300 * s,  "oy": -200 * s, "w": 200 * s, "d": 85 * s, "h": 85 * s},
        {"ox": -250 * s,  "oy": -280 * s, "w": 150 * s, "d": 70 * s, "h": 70 * s},
    ]

    for i, c in enumerate(containers):
        cx = loc[0] + c["ox"]
        cy = loc[1] + c["oy"]
        _spawn(unreal, {
            "name": f"{prefix}_Container_{i}",
            "type": "StaticMeshActor",
            "location": [cx, cy, ground + c["h"] / 2],
            "scale": [c["w"] / 100, c["d"] / 100, c["h"] / 100],
            "static_mesh": CUBE
        }, actors)


# ── F. Perimeter Fence Posts ───────────────────────────────────────────────

def build_perimeter(unreal, prefix: str, loc: List[float],
                    size: Dict[str, float], actors: List) -> None:
    """Thin fence posts around compound edges."""
    logger.info("Building perimeter markers...")
    s = size["scale"]
    ground = loc[2]
    # Rough compound bounding box
    hw, hd = 800 * s, 650 * s

    post_h = 120 * s
    posts = []
    # North + south edges
    for i in range(6):
        x = loc[0] - hw + i * (2 * hw / 5)
        posts.append((x, loc[1] - hd))
        posts.append((x, loc[1] + hd))
    # East + west edges (skip corners)
    for i in range(1, 4):
        y = loc[1] - hd + i * (2 * hd / 4)
        posts.append((loc[0] - hw, y))
        posts.append((loc[0] + hw, y))

    for j, (px, py) in enumerate(posts):
        _spawn(unreal, {
            "name": f"{prefix}_FencePost_{j}",
            "type": "StaticMeshActor",
            "location": [px, py, ground + post_h / 2],
            "scale": [0.1 * s, 0.1 * s, post_h / 100],
            "static_mesh": CYLINDER
        }, actors)


# ── G. Utility Pipes / Conduits ────────────────────────────────────────────

def build_pipes(unreal, prefix: str, loc: List[float],
                size: Dict[str, float], actors: List) -> None:
    """Horizontal pipe conduits connecting hab modules."""
    logger.info("Building utility pipes...")
    s = size["scale"]
    ground = loc[2]

    pipes = [
        # Command hab to storage hab (along X)
        {"pos": [loc[0] + 210 * s, loc[1] - 140 * s, ground + 60 * s],
         "scale": [2.2 * s, 0.15 * s, 0.15 * s]},
        # Command hab to crew hab (along Y then X)
        {"pos": [loc[0] - 190 * s, loc[1] + 0, ground + 70 * s],
         "scale": [0.15 * s, 2.0 * s, 0.15 * s]},
        # Storage to med bay (along Y)
        {"pos": [loc[0] + 390 * s, loc[1] + 100 * s, ground + 55 * s],
         "scale": [0.15 * s, 1.8 * s, 0.15 * s]},
    ]

    for i, p in enumerate(pipes):
        _spawn(unreal, {
            "name": f"{prefix}_Pipe_{i}",
            "type": "StaticMeshActor",
            "location": p["pos"],
            "scale": p["scale"],
            "static_mesh": CYLINDER
        }, actors)


# ── Main entry point ──────────────────────────────────────────────────────

def build_outpost_compound(
    unreal,
    location: List[float],
    name_prefix: str = "Outpost",
    compound_size: str = "medium",
    include_comms_tower: bool = True,
    include_antenna_array: bool = True,
    include_parking_pad: bool = True,
    include_containers: bool = True,
    include_perimeter: bool = True,
) -> Dict[str, Any]:
    """
    Build a complete sci-fi mining outpost compound from primitive shapes.
    Returns dict with success status, actor list, and stats.
    """
    size = get_outpost_size_params(compound_size)
    all_actors: List[Any] = []

    # Core structures (always built)
    build_hab_modules(unreal, name_prefix, location, size, all_actors)
    build_pipes(unreal, name_prefix, location, size, all_actors)

    # Optional modules
    if include_comms_tower:
        build_comms_tower(unreal, name_prefix, location, size, all_actors)
    if include_antenna_array:
        build_antenna_array(unreal, name_prefix, location, size, all_actors)
    if include_parking_pad:
        build_parking_pad(unreal, name_prefix, location, size, all_actors)
    if include_containers:
        build_containers(unreal, name_prefix, location, size, all_actors)
    if include_perimeter:
        build_perimeter(unreal, name_prefix, location, size, all_actors)

    return {
        "actors": all_actors,
        "stats": {
            "size": compound_size,
            "total_actors": len(all_actors),
            "has_comms_tower": include_comms_tower,
            "has_antenna_array": include_antenna_array,
            "has_parking_pad": include_parking_pad,
            "has_containers": include_containers,
            "has_perimeter": include_perimeter,
        },
    }
