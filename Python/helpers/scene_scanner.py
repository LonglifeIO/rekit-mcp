"""
Scene Scanner — tools for analyzing existing UE5 levels to extract grid conventions,
adjacency rules, socket definitions, and room/corridor archetype templates.

Phase B of the kit-bashing pipeline: scan artist-authored scenes to learn patterns
that downstream tools (layout generator, constraint solver) use as examples and rules.
"""

import json
import os
import time
import math
import logging
from typing import Dict, Any, List, Optional, Tuple, Set
from functools import reduce
from collections import defaultdict

logger = logging.getLogger(__name__)

# Import catalog helpers for catalog I/O
from .catalog_enricher import _catalog_path, _load_catalog, _save_catalog, CATALOGS_DIR


# ---------------------------------------------------------------------------
# Piece type classification from mesh names
# ---------------------------------------------------------------------------

# Map mesh name substrings to functional piece types.
# Order matters — first match wins. More specific patterns come first.
PIECE_TYPE_RULES = [
    ("DoorModule", "door"),
    ("WallCorner_2", "corner_diagonal"),
    ("WallCorner", "corner_l"),
    ("Corner", "corner"),
    ("WallWindow", "window"),
    ("Wall_1", "wall"),
    ("Ceiling_Window", "skylight"),
    ("RoofModule", "roof"),
    ("Modular_Merged", "floor"),   # SM_Modular_Merged (the floor tile)
    ("Part_1", "detail"),
    ("Part_2", "detail"),
]


def _classify_piece_type(mesh_name: str) -> str:
    """Classify a mesh into a functional piece type based on its name."""
    for pattern, piece_type in PIECE_TYPE_RULES:
        if pattern in mesh_name:
            return piece_type
    return "unknown"


# ---------------------------------------------------------------------------
# Module-level cache for scene scans (persists across tool calls)
# ---------------------------------------------------------------------------

_scan_cache: Dict[str, Any] = {
    "timestamp": 0.0,
    "actors": None,
    "grid_size": None,
    "grid_positions": None,
    "catalog_name": "",
    "category_filter": "",
    "name_filter": "",
}
_CACHE_TTL = 30  # seconds


def _cache_valid(catalog_name: str, category_filter: str, name_filter: str) -> bool:
    """Check if cached scan data is still fresh and matches current filters."""
    if time.time() - _scan_cache["timestamp"] > _CACHE_TTL:
        return False
    if _scan_cache["actors"] is None:
        return False
    if _scan_cache["catalog_name"] != catalog_name:
        return False
    if _scan_cache["category_filter"] != category_filter:
        return False
    if _scan_cache["name_filter"] != name_filter:
        return False
    return True


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def scan_scene_grid(
    unreal_connection,
    catalog_name: str = "modularscifi_meshes",
    category_filter: str = "modular_building",
    name_filter: str = "",
    max_actors: int = 500,
) -> Dict[str, Any]:
    """
    Scan the current level for structural kit pieces and detect grid alignment.

    Process:
      1. get_actors_in_level → filter by name pattern
      2. get_actor_details for each → mesh_path + transform
      3. Match mesh_path against catalog category
      4. GCD of floor-piece position deltas → grid_size_cm
      5. Snap positions to grid, quantize rotations to nearest 90°

    Args:
        unreal_connection: Active UnrealConnection instance.
        catalog_name: Catalog to match meshes against.
        category_filter: Only include pieces from this catalog category.
        name_filter: Only include actors whose name contains this string (case-insensitive).
        max_actors: Max actors to query details for (performance cap).

    Returns:
        Dict with grid_size_cm, actor_count, structural_count, grid_positions.
    """
    # Check cache
    if _cache_valid(catalog_name, category_filter, name_filter):
        cached = _scan_cache
        return {
            "success": True,
            "cached": True,
            "grid_size_cm": cached["grid_size"],
            "actor_count": len(cached["actors"]),
            "structural_count": len(cached["grid_positions"]),
            "grid_positions": cached["grid_positions"],
        }

    # Load catalog
    catalog = _load_catalog(_catalog_path(catalog_name))
    if catalog is None:
        return {"success": False, "message": f"Catalog not found: {catalog_name}"}

    # Step 1-3: Get structural actors with transforms
    actors = _get_structural_actors(
        unreal_connection, catalog, category_filter, name_filter, max_actors
    )
    if not actors:
        return {
            "success": True,
            "grid_size_cm": 500,
            "actor_count": 0,
            "structural_count": 0,
            "grid_positions": [],
            "message": "No structural actors found matching filters",
        }

    # Step 4: Detect grid size from floor pieces
    floor_positions = [a["location"] for a in actors if a["piece_type"] == "floor"]
    if len(floor_positions) >= 2:
        grid_size = _detect_grid_size(floor_positions)
    else:
        all_positions = [a["location"] for a in actors]
        grid_size = _detect_grid_size(all_positions) if len(all_positions) >= 2 else 500

    # Sanity check: grid should be reasonable for modular kits
    if grid_size < 100 or grid_size > 2000:
        grid_size = 500

    # Step 5: Snap and quantize
    grid_positions = _snap_to_grid(actors, grid_size)

    # Update cache
    _scan_cache.update({
        "timestamp": time.time(),
        "actors": actors,
        "grid_size": grid_size,
        "grid_positions": grid_positions,
        "catalog_name": catalog_name,
        "category_filter": category_filter,
        "name_filter": name_filter,
    })

    return {
        "success": True,
        "cached": False,
        "grid_size_cm": grid_size,
        "actor_count": len(actors),
        "structural_count": len(grid_positions),
        "grid_positions": grid_positions,
    }


def build_adjacency_graph(
    unreal_connection,
    catalog_name: str = "modularscifi_meshes",
    category_filter: str = "modular_building",
    name_filter: str = "",
    max_neighbor_distance: float = 1.5,
) -> Dict[str, Any]:
    """
    Build an adjacency graph from structural pieces in the current level.

    Scans the scene grid (or reuses cache), then for each pair of pieces
    within max_neighbor_distance grid cells, creates an edge with direction
    and connection metadata.

    Args:
        unreal_connection: Active UnrealConnection instance.
        catalog_name: Catalog to match meshes against.
        category_filter: Only include pieces from this catalog category.
        name_filter: Only include actors whose name contains this string.
        max_neighbor_distance: Max distance in grid cells for adjacency.

    Returns:
        Dict with node_count, edge_count, nodes, edges, grid_size_cm.
    """
    scan = scan_scene_grid(
        unreal_connection, catalog_name, category_filter, name_filter
    )
    if not scan.get("success"):
        return scan

    grid_positions = scan["grid_positions"]
    grid_size = scan["grid_size_cm"]

    if len(grid_positions) < 2:
        return {
            "success": True,
            "node_count": len(grid_positions),
            "edge_count": 0,
            "nodes": grid_positions,
            "edges": [],
            "grid_size_cm": grid_size,
        }

    edges = _build_adjacency_edges(grid_positions, grid_size, max_neighbor_distance)

    # Compact node list for output
    nodes = [
        {
            "id": gp["id"],
            "name": gp["name"],
            "mesh_name": gp["mesh_name"],
            "piece_type": gp["piece_type"],
            "grid_x": gp["grid_x"],
            "grid_y": gp["grid_y"],
            "grid_z": gp["grid_z"],
            "rotation_deg": gp["rotation_deg"],
        }
        for gp in grid_positions
    ]

    return {
        "success": True,
        "node_count": len(nodes),
        "edge_count": len(edges),
        "nodes": nodes,
        "edges": edges,
        "grid_size_cm": grid_size,
    }


def extract_archetypes(
    unreal_connection,
    catalog_name: str = "modularscifi_meshes",
    category_filter: str = "modular_building",
    name_filter: str = "",
    method: str = "connected_components",
    min_pieces: int = 3,
    save_to: str = "",
) -> Dict[str, Any]:
    """
    Extract room/corridor archetypes from the current level.

    Builds adjacency graph, clusters via connected components, classifies
    each cluster by aspect ratio and openings, normalizes piece positions
    relative to cluster centroid.

    Args:
        unreal_connection: Active UnrealConnection instance.
        catalog_name: Catalog to match meshes against.
        category_filter: Only include pieces from this catalog category.
        name_filter: Only include actors whose name contains this string.
        method: Clustering method ("connected_components").
        min_pieces: Minimum pieces for a valid cluster.
        save_to: Save archetypes to this filename in catalogs/ (without .json).

    Returns:
        Dict with archetype_count and archetypes list.
    """
    graph = build_adjacency_graph(
        unreal_connection, catalog_name, category_filter, name_filter
    )
    if not graph.get("success"):
        return graph

    nodes = graph["nodes"]
    edges = graph["edges"]
    grid_size = graph["grid_size_cm"]

    if not nodes:
        return {
            "success": True,
            "archetype_count": 0,
            "archetypes": [],
            "grid_size_cm": grid_size,
            "message": "No nodes to cluster",
        }

    # Cluster into connected components
    clusters = _find_connected_components(nodes, edges, min_pieces)

    # Use cached grid_positions for full data (has location, etc.)
    full_grid_positions = _scan_cache.get("grid_positions", [])

    archetypes = []
    for i, cluster_nodes in enumerate(clusters):
        node_ids = {n["id"] for n in cluster_nodes}
        cluster_grid = [gp for gp in full_grid_positions if gp["id"] in node_ids]

        classification = _classify_cluster(cluster_grid, grid_size)
        pieces_relative = _normalize_to_centroid(cluster_grid)

        archetype = {
            "id": f"archetype_{i}",
            "classification": classification["type"],
            "piece_count": len(cluster_grid),
            "grid_bounds": classification["grid_bounds"],
            "floor_count": classification["floor_count"],
            "wall_count": classification["wall_count"],
            "door_count": classification["door_count"],
            "window_count": classification["window_count"],
            "opening_count": classification["opening_count"],
            "aspect_ratio": classification["aspect_ratio"],
            "pieces_relative": pieces_relative,
        }
        archetypes.append(archetype)

    # Save if requested
    if save_to:
        save_path = os.path.join(CATALOGS_DIR, f"{save_to}.json")
        archetype_doc = {
            "version": "1.0",
            "source": "scene_scanner.extract_archetypes",
            "grid_size_cm": grid_size,
            "archetype_count": len(archetypes),
            "archetypes": archetypes,
        }
        _save_catalog(save_path, archetype_doc)

    return {
        "success": True,
        "archetype_count": len(archetypes),
        "archetypes": archetypes,
        "grid_size_cm": grid_size,
    }


def extract_sockets(
    unreal_connection,
    catalog_name: str = "modularscifi_meshes",
    category_filter: str = "modular_building",
    name_filter: str = "",
    save_to_catalog: bool = True,
) -> Dict[str, Any]:
    """
    Infer socket types from observed adjacency patterns in the current scene.

    For each edge in the adjacency graph, records which mesh types connect
    and their relative orientation. Groups recurring patterns into named
    socket types (e.g., wall-wall = 'wall_continuation').

    Args:
        unreal_connection: Active UnrealConnection instance.
        catalog_name: Catalog to match meshes against.
        category_filter: Only include pieces from this catalog category.
        name_filter: Only include actors whose name contains this string.
        save_to_catalog: Write socket definitions back to catalog JSON and
            save a standalone sockets file.

    Returns:
        Dict with mesh_count, total_sockets, socket_types, per_mesh.
    """
    graph = build_adjacency_graph(
        unreal_connection, catalog_name, category_filter, name_filter
    )
    if not graph.get("success"):
        return graph

    nodes = graph["nodes"]
    edges = graph["edges"]

    if not edges:
        return {
            "success": True,
            "mesh_count": 0,
            "total_sockets": 0,
            "socket_types": [],
            "per_mesh": [],
            "message": "No edges to infer sockets from",
        }

    node_map = {n["id"]: n for n in nodes}
    socket_data = _infer_socket_types(edges, node_map)

    if save_to_catalog:
        # Update catalog with sockets per mesh
        catalog_path = _catalog_path(catalog_name)
        catalog = _load_catalog(catalog_path)
        if catalog:
            for mesh_info in socket_data["per_mesh"]:
                mesh_name = mesh_info["mesh_name"]
                for entry in catalog.get("meshes", []):
                    if entry.get("name") == mesh_name:
                        entry["sockets"] = mesh_info["sockets"]
                        break
            _save_catalog(catalog_path, catalog)

        # Also save standalone sockets file
        sockets_path = os.path.join(CATALOGS_DIR, f"{catalog_name}_sockets.json")
        sockets_doc = {
            "version": "1.0",
            "source": "scene_scanner.extract_sockets",
            "socket_types": socket_data["socket_types"],
            "mesh_count": socket_data["mesh_count"],
            "per_mesh": socket_data["per_mesh"],
        }
        _save_catalog(sockets_path, sockets_doc)

    return {
        "success": True,
        "mesh_count": socket_data["mesh_count"],
        "total_sockets": socket_data["total_sockets"],
        "socket_types": socket_data["socket_types"],
        "per_mesh": socket_data["per_mesh"],
    }


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _parse_vector(v) -> List[float]:
    """Parse a vector that might be a dict {x,y,z}, {pitch,yaw,roll}, or list."""
    if isinstance(v, dict):
        if "x" in v:
            return [float(v.get("x", 0)), float(v.get("y", 0)), float(v.get("z", 0))]
        if "pitch" in v:
            return [float(v.get("pitch", 0)), float(v.get("yaw", 0)), float(v.get("roll", 0))]
        vals = list(v.values())
        return [float(x) for x in vals[:3]] if len(vals) >= 3 else [0.0, 0.0, 0.0]
    if isinstance(v, (list, tuple)):
        return [float(x) for x in v[:3]] if len(v) >= 3 else [0.0, 0.0, 0.0]
    return [0.0, 0.0, 0.0]


def _get_structural_actors(
    unreal_connection,
    catalog: Dict[str, Any],
    category_filter: str = "",
    name_filter: str = "",
    max_actors: int = 500,
) -> List[Dict[str, Any]]:
    """
    Get all structural actors in the level with their transforms and mesh paths.

    Queries get_actors_in_level, then get_actor_details for each actor to
    retrieve mesh_path. Matches mesh_path against catalog entries.

    Returns list of dicts with: id, name, mesh_name, mesh_path, category,
    piece_type, location, rotation, size_cm.
    """
    # Build catalog lookups by mesh path and asset name
    path_lookup = {}
    asset_lookup = {}
    for entry in catalog.get("meshes", []):
        path = entry.get("path", "")
        if path:
            path_lookup[path] = entry
            # Index by asset name (last segment before .)
            parts = path.rsplit("/", 1)
            if len(parts) > 1:
                asset_key = parts[-1].split(".")[0]
                asset_lookup[asset_key] = entry

    # Get all actors in level
    try:
        response = unreal_connection.send_command("get_actors_in_level", {})
    except Exception as e:
        logger.error(f"get_actors_in_level failed: {e}")
        return []

    # Unwrap nested response — UE5 MCP returns {"status":"success","result":{"actors":[...]}}
    inner = response.get("result", response) if isinstance(response, dict) else response
    raw_actors = inner.get("actors", []) if isinstance(inner, dict) else []
    if not raw_actors:
        logger.warning(f"No actors in response. Keys: {list(response.keys()) if isinstance(response, dict) else 'N/A'}")
        return []

    # Normalize response — actors can be strings or dicts
    all_actors = []
    for a in raw_actors:
        if isinstance(a, str):
            all_actors.append({"name": a})
        elif isinstance(a, dict):
            all_actors.append(a)

    # Filter by name
    if name_filter:
        name_lower = name_filter.lower()
        all_actors = [
            a for a in all_actors
            if name_lower in (a.get("name", "") or a.get("label", "")).lower()
        ]

    # Limit to max_actors
    if len(all_actors) > max_actors:
        logger.warning(f"Limiting scan to {max_actors} of {len(all_actors)} actors")
        all_actors = all_actors[:max_actors]

    logger.info(f"Querying details for {len(all_actors)} actors...")

    # Get details for each actor and match against catalog
    structural = []
    actor_id = 0

    for actor_info in all_actors:
        actor_name = actor_info.get("name", "") or actor_info.get("label", "")
        if not actor_name:
            continue

        try:
            details = unreal_connection.send_command(
                "get_actor_details", {"name": actor_name}
            )
        except Exception as e:
            logger.debug(f"Skipping {actor_name}: {e}")
            continue

        if not details or details.get("status") == "error":
            continue

        # Unwrap nested response — get_actor_details returns {"status":"success","result":{...}}
        detail_data = details.get("result", details) if isinstance(details, dict) else details

        mesh_path = detail_data.get("static_mesh_path", "")
        if not mesh_path:
            continue

        # Match against catalog
        catalog_entry = path_lookup.get(mesh_path)
        if not catalog_entry:
            asset_name = mesh_path.rsplit("/", 1)[-1].split(".")[0] if "/" in mesh_path else ""
            catalog_entry = asset_lookup.get(asset_name)

        if not catalog_entry:
            continue

        cat = catalog_entry.get("category", "")
        if category_filter and cat != category_filter:
            continue

        location = _parse_vector(detail_data.get("location", [0, 0, 0]))
        rotation = _parse_vector(detail_data.get("rotation", [0, 0, 0]))
        mesh_name = catalog_entry.get("name", "")

        structural.append({
            "id": actor_id,
            "name": actor_name,
            "mesh_name": mesh_name,
            "mesh_path": mesh_path,
            "category": cat,
            "piece_type": _classify_piece_type(mesh_name),
            "location": location,
            "rotation": rotation,
            "size_cm": catalog_entry.get("size_cm"),
        })
        actor_id += 1
        time.sleep(0.02)  # small delay between UE5 queries

    logger.info(f"Found {len(structural)} structural actors")
    return structural


def _detect_grid_size(positions: List[List[float]]) -> int:
    """
    Detect grid cell size from position deltas using GCD.

    Only considers X and Y deltas (Z is typically uniform for one floor level).
    Returns integer grid size in cm.
    """
    deltas = set()
    for i, p1 in enumerate(positions):
        for p2 in positions[i + 1:]:
            dx = abs(round(p1[0] - p2[0]))
            dy = abs(round(p1[1] - p2[1]))
            if dx > 10:
                deltas.add(dx)
            if dy > 10:
                deltas.add(dy)

    if not deltas:
        return 500  # Default ModularSciFi grid

    return reduce(math.gcd, deltas)


def _snap_to_grid(
    actors: List[Dict[str, Any]], grid_size: int
) -> List[Dict[str, Any]]:
    """
    Snap actor positions to grid and quantize rotations.

    Returns list with grid_x/grid_y/grid_z coordinates and quantized yaw.
    """
    results = []
    for actor in actors:
        loc = actor["location"]
        rot = actor["rotation"]

        grid_x = round(loc[0] / grid_size)
        grid_y = round(loc[1] / grid_size)
        grid_z = round(loc[2] / grid_size)

        # Yaw is rotation[1] in [pitch, yaw, roll]
        yaw = rot[1] if len(rot) > 1 else 0.0
        yaw_quantized = _quantize_rotation(yaw)

        results.append({
            "id": actor["id"],
            "name": actor["name"],
            "mesh_name": actor["mesh_name"],
            "piece_type": actor["piece_type"],
            "location": loc,
            "grid_x": grid_x,
            "grid_y": grid_y,
            "grid_z": grid_z,
            "rotation_deg": yaw_quantized,
            "raw_yaw": yaw,
        })

    return results


def _quantize_rotation(yaw: float) -> float:
    """Quantize a yaw angle to the nearest 90-degree step (-180 to 180 range)."""
    yaw = yaw % 360
    if yaw < 0:
        yaw += 360
    quantized = round(yaw / 90) * 90
    if quantized == 360:
        quantized = 0
    # Normalize to [-180, 180) for readability
    if quantized > 180:
        quantized -= 360
    return float(quantized)


def _build_adjacency_edges(
    grid_positions: List[Dict[str, Any]],
    grid_size: int,
    max_dist_factor: float = 1.5,
) -> List[Dict[str, Any]]:
    """
    Build adjacency edges between grid-aligned pieces.

    Two pieces are adjacent if their Chebyshev grid-cell distance
    is within max_dist_factor.
    """
    edges = []
    edge_id = 0

    for i, a in enumerate(grid_positions):
        for j in range(i + 1, len(grid_positions)):
            b = grid_positions[j]

            dx = abs(a["grid_x"] - b["grid_x"])
            dy = abs(a["grid_y"] - b["grid_y"])
            dz = abs(a["grid_z"] - b["grid_z"])

            dist = max(dx, dy, dz)  # Chebyshev distance

            if dist <= max_dist_factor:
                direction = _grid_direction(
                    a["grid_x"], a["grid_y"],
                    b["grid_x"], b["grid_y"],
                )

                edges.append({
                    "id": edge_id,
                    "from_id": a["id"],
                    "to_id": b["id"],
                    "from_mesh": a["mesh_name"],
                    "to_mesh": b["mesh_name"],
                    "from_type": a["piece_type"],
                    "to_type": b["piece_type"],
                    "direction": direction,
                    "grid_distance": dist,
                    "same_cell": (dx == 0 and dy == 0 and dz == 0),
                })
                edge_id += 1

    return edges


def _grid_direction(x1: int, y1: int, x2: int, y2: int) -> str:
    """Determine direction from grid position 1 to grid position 2."""
    dx = x2 - x1
    dy = y2 - y1

    if dx == 0 and dy == 0:
        return "same_cell"

    # Primary axis direction
    parts = []
    if dx > 0:
        parts.append("+x")
    elif dx < 0:
        parts.append("-x")
    if dy > 0:
        parts.append("+y")
    elif dy < 0:
        parts.append("-y")

    return "_".join(parts) if parts else "same_cell"


def _find_connected_components(
    nodes: List[Dict[str, Any]],
    edges: List[Dict[str, Any]],
    min_pieces: int = 3,
) -> List[List[Dict[str, Any]]]:
    """
    Find connected components via BFS.

    Returns list of clusters (each a list of node dicts).
    Only includes clusters with >= min_pieces nodes.
    """
    adj: Dict[int, Set[int]] = defaultdict(set)
    for edge in edges:
        adj[edge["from_id"]].add(edge["to_id"])
        adj[edge["to_id"]].add(edge["from_id"])

    node_map = {n["id"]: n for n in nodes}
    visited: Set[int] = set()
    components = []

    for node in nodes:
        nid = node["id"]
        if nid in visited:
            continue

        component = []
        queue = [nid]
        visited.add(nid)

        while queue:
            current = queue.pop(0)
            if current in node_map:
                component.append(node_map[current])

            for neighbor in adj[current]:
                if neighbor not in visited and neighbor in node_map:
                    visited.add(neighbor)
                    queue.append(neighbor)

        if len(component) >= min_pieces:
            components.append(component)

    return components


def _classify_cluster(
    cluster_grid: List[Dict[str, Any]], grid_size: int
) -> Dict[str, Any]:
    """
    Classify a cluster by shape, size, and openings.

    Returns dict with type, grid_bounds, piece counts, aspect_ratio.
    """
    empty = {
        "type": "empty", "grid_bounds": [0, 0],
        "floor_count": 0, "wall_count": 0, "door_count": 0,
        "window_count": 0, "opening_count": 0, "aspect_ratio": 1.0,
    }
    if not cluster_grid:
        return empty

    # Count piece types
    type_counts: Dict[str, int] = defaultdict(int)
    for p in cluster_grid:
        type_counts[p["piece_type"]] += 1

    floor_count = type_counts.get("floor", 0)
    wall_count = type_counts.get("wall", 0)
    door_count = type_counts.get("door", 0)
    window_count = type_counts.get("window", 0)
    opening_count = door_count + window_count

    # Compute grid bounding box from floor pieces
    floor_pieces = [p for p in cluster_grid if p["piece_type"] == "floor"]
    if not floor_pieces:
        floor_pieces = cluster_grid

    gx_vals = [p["grid_x"] for p in floor_pieces]
    gy_vals = [p["grid_y"] for p in floor_pieces]
    x_span = max(gx_vals) - min(gx_vals) + 1 if gx_vals else 1
    y_span = max(gy_vals) - min(gy_vals) + 1 if gy_vals else 1

    grid_bounds = [x_span, y_span]
    aspect_ratio = max(x_span, y_span) / max(min(x_span, y_span), 1)

    # Classify shape
    if floor_count == 0:
        classification = "fragment"
    elif aspect_ratio > 3 and min(x_span, y_span) <= 2:
        classification = "corridor"
    elif floor_count == 1:
        classification = "cell_1x1"
    else:
        # Check fill ratio to detect L/T shapes
        floor_cells = {(p["grid_x"], p["grid_y"]) for p in floor_pieces}
        total_bb_cells = x_span * y_span
        fill_ratio = len(floor_cells) / total_bb_cells if total_bb_cells > 0 else 1.0

        if fill_ratio < 0.75:
            if len(floor_cells) >= 5:
                classification = "l_shape"
            else:
                classification = "irregular"
        else:
            classification = f"room_{x_span}x{y_span}"

    return {
        "type": classification,
        "grid_bounds": grid_bounds,
        "floor_count": floor_count,
        "wall_count": wall_count,
        "door_count": door_count,
        "window_count": window_count,
        "opening_count": opening_count,
        "aspect_ratio": round(aspect_ratio, 2),
    }


def _normalize_to_centroid(
    cluster_grid: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    """
    Normalize piece positions relative to the cluster's floor centroid.

    Returns pieces with rel_grid_x/y/z coordinates.
    """
    floor_pieces = [p for p in cluster_grid if p["piece_type"] == "floor"]
    if not floor_pieces:
        floor_pieces = cluster_grid
    if not floor_pieces:
        return []

    # Centroid from floor pieces, snapped to nearest grid point
    cx = round(sum(p["grid_x"] for p in floor_pieces) / len(floor_pieces))
    cy = round(sum(p["grid_y"] for p in floor_pieces) / len(floor_pieces))
    cz = min(p["grid_z"] for p in floor_pieces)

    return [
        {
            "mesh_name": p["mesh_name"],
            "piece_type": p["piece_type"],
            "rel_grid_x": p["grid_x"] - cx,
            "rel_grid_y": p["grid_y"] - cy,
            "rel_grid_z": p["grid_z"] - cz,
            "rotation_deg": p["rotation_deg"],
        }
        for p in cluster_grid
    ]


def _infer_socket_types(
    edges: List[Dict[str, Any]],
    node_map: Dict[int, Dict[str, Any]],
) -> Dict[str, Any]:
    """
    Infer socket types from observed adjacency patterns.

    Analyzes edges to determine connection types between piece types
    and groups them into named socket types.
    """
    # Collect connections per mesh
    mesh_connections: Dict[str, Dict[str, set]] = defaultdict(lambda: defaultdict(set))
    socket_types_seen: Set[str] = set()

    for edge in edges:
        from_node = node_map.get(edge["from_id"])
        to_node = node_map.get(edge["to_id"])
        if not from_node or not to_node:
            continue

        from_type = from_node["piece_type"]
        to_type = to_node["piece_type"]
        direction = edge["direction"]
        same_cell = edge.get("same_cell", False)

        socket_type = _infer_single_socket(from_type, to_type, same_cell)
        socket_types_seen.add(socket_type)

        mesh_connections[from_node["mesh_name"]][direction].add(
            (to_type, socket_type)
        )
        reverse_dir = _reverse_direction(direction)
        mesh_connections[to_node["mesh_name"]][reverse_dir].add(
            (from_type, socket_type)
        )

    # Build per-mesh socket definitions
    per_mesh = []
    for mesh_name in sorted(mesh_connections.keys()):
        dir_connections = mesh_connections[mesh_name]
        sockets = []
        for direction in sorted(dir_connections.keys()):
            for neighbor_type, socket_type in sorted(dir_connections[direction]):
                sockets.append({
                    "direction": direction,
                    "socket_type": socket_type,
                    "connects_to": neighbor_type,
                })
        per_mesh.append({
            "mesh_name": mesh_name,
            "socket_count": len(sockets),
            "sockets": sockets,
        })

    return {
        "mesh_count": len(per_mesh),
        "total_sockets": sum(m["socket_count"] for m in per_mesh),
        "socket_types": sorted(socket_types_seen),
        "per_mesh": per_mesh,
    }


# Socket inference rules
_SAME_CELL_SOCKETS = {
    ("corner", "floor"): "floor_corner_base",
    ("corner", "wall"): "wall_corner_endpoint",
    ("corner_diagonal", "floor"): "floor_corner_base",
    ("corner_l", "floor"): "floor_corner_base",
    ("door", "floor"): "floor_door_base",
    ("floor", "roof"): "floor_roof_stack",
    ("floor", "skylight"): "floor_skylight_stack",
    ("floor", "wall"): "floor_wall_base",
    ("floor", "window"): "floor_window_base",
    ("wall", "roof"): "wall_roof_same_cell",
    ("wall", "skylight"): "wall_skylight_same_cell",
    ("door", "roof"): "door_roof_same_cell",
    ("door", "skylight"): "door_skylight_same_cell",
}

_NEIGHBOR_SOCKETS = {
    ("corner", "corner"): "corner_corner_adjacent",
    ("corner", "wall"): "corner_wall_adjacent",
    ("door", "wall"): "door_wall_adjacent",
    ("floor", "door"): "floor_door_edge",
    ("floor", "floor"): "floor_continuation",
    ("floor", "wall"): "floor_wall_edge",
    ("floor", "window"): "floor_window_edge",
    ("roof", "roof"): "roof_continuation",
    ("roof", "skylight"): "roof_skylight_adjacent",
    ("skylight", "skylight"): "skylight_continuation",
    ("wall", "wall"): "wall_continuation",
    ("wall", "window"): "wall_window_adjacent",
}


def _infer_single_socket(from_type: str, to_type: str, same_cell: bool) -> str:
    """Infer socket type from two connected piece types."""
    key = tuple(sorted([from_type, to_type]))

    if same_cell:
        return _SAME_CELL_SOCKETS.get(key, f"{key[0]}_{key[1]}_same_cell")
    else:
        return _NEIGHBOR_SOCKETS.get(key, f"{key[0]}_{key[1]}_adjacent")


def _reverse_direction(direction: str) -> str:
    """Reverse a direction string (+x → -x, etc.)."""
    _rev = {"+x": "-x", "-x": "+x", "+y": "-y", "-y": "+y", "same_cell": "same_cell"}
    if direction in _rev:
        return _rev[direction]
    # Compound directions like "+x_+y"
    return "_".join(_rev.get(p, p) for p in direction.split("_"))
