"""
Catalog Enricher — tools for measuring mesh bounds, adding semantic metadata,
and querying the mesh catalog with advanced filters.

Phase A of the kit-bashing pipeline: enrich the existing ModularSciFi catalog
with measured bounds, pivot offsets, and semantic annotations so that downstream
tools (scene scanner, layout generator, constraint solver) have the data they need.
"""

import json
import os
import time
import logging
from typing import Dict, Any, List, Optional

logger = logging.getLogger(__name__)

# Graceful import — helper may be used standalone for testing
try:
    from .actor_name_manager import safe_spawn_actor, safe_delete_actor
except ImportError:
    logger.warning("Could not import actor helpers; measurement will be unavailable")
    safe_spawn_actor = None
    safe_delete_actor = None

# Catalogs directory (sibling of helpers/)
CATALOGS_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "catalogs")


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def measure_catalog_bounds(
    unreal_connection,
    catalog_name: str = "modularscifi_meshes",
    category: str = "",
    dry_run: bool = False,
    delay: float = 0.3,
) -> Dict[str, Any]:
    """
    Measure bounds for catalog meshes that have null size_cm/bounds_extent.

    For each unmeasured mesh: spawns at origin, queries get_actor_details
    for bounding box, deletes the actor, writes measurements back to catalog.

    Args:
        unreal_connection: Active UnrealConnection instance.
        catalog_name: Catalog JSON filename (without .json).
        category: Only measure meshes in this category (empty = all).
        dry_run: If True, report what would be measured without spawning.
        delay: Seconds between spawn-measure-delete cycles.

    Returns:
        Dict with success, measured_count, skipped_count, errors.
    """
    if not dry_run and (safe_spawn_actor is None or safe_delete_actor is None):
        return {"success": False, "message": "Actor helpers not available"}

    catalog_path = _catalog_path(catalog_name)
    catalog = _load_catalog(catalog_path)
    if catalog is None:
        return {"success": False, "message": f"Catalog not found: {catalog_path}"}

    meshes = catalog.get("meshes", [])
    measured = 0
    skipped = 0
    errors = []

    for entry in meshes:
        # Filter by category
        if category and entry.get("category", "") != category:
            continue

        # Skip if already measured (check both flat and nested formats)
        size = entry.get("size_cm") or (entry.get("spatial", {}) or {}).get("size_cm")
        if size is not None:
            skipped += 1
            continue

        mesh_path = entry.get("path", "")
        if not mesh_path:
            errors.append({"mesh": entry.get("name", "?"), "error": "No path in catalog"})
            continue

        if dry_run:
            measured += 1
            continue

        # Spawn → measure → delete
        result = _measure_single_mesh(unreal_connection, mesh_path, entry.get("name", "Measure"))
        if result["success"]:
            # Write bounds into entry (flat format for v1 compat, spatial for v2)
            entry["bounds_extent"] = result["bounds_extent"]
            entry["size_cm"] = result["size_cm"]
            if "spatial" not in entry:
                entry["spatial"] = {}
            entry["spatial"]["bounds_extent"] = result["bounds_extent"]
            entry["spatial"]["size_cm"] = result["size_cm"]
            entry["spatial"]["pivot_offset_cm"] = result.get("pivot_offset_cm", [0, 0, 0])
            measured += 1
        else:
            errors.append({"mesh": entry.get("name", "?"), "error": result.get("error", "Unknown")})

        if delay > 0:
            time.sleep(delay)

    if not dry_run and measured > 0:
        _save_catalog(catalog_path, catalog)

    return {
        "success": True,
        "dry_run": dry_run,
        "measured_count": measured,
        "skipped_count": skipped,
        "error_count": len(errors),
        "errors": errors[:10],  # Cap error list
        "catalog": catalog_name,
    }


def add_semantics(
    catalog_name: str,
    mesh_name: str,
    display_name: str = "",
    description: str = "",
    function: str = "",
    style_tags: Optional[List[str]] = None,
    spawn_weight: float = -1.0,
) -> Dict[str, Any]:
    """
    Add or update semantic metadata for a single mesh in the catalog.

    Only updates fields that are provided (non-empty / non-default).

    Args:
        catalog_name: Catalog JSON filename (without .json).
        mesh_name: Mesh name to update (fuzzy matched).
        display_name: Human-readable short name.
        description: What the piece looks like.
        function: What role it plays in a layout.
        style_tags: Visual style tags (e.g. ["industrial", "panelled"]).
        spawn_weight: How commonly it should appear (0-1, -1 = don't change).

    Returns:
        Dict with success and updated fields.
    """
    catalog_path = _catalog_path(catalog_name)
    catalog = _load_catalog(catalog_path)
    if catalog is None:
        return {"success": False, "message": f"Catalog not found: {catalog_path}"}

    entry = _find_mesh_entry(catalog, mesh_name)
    if entry is None:
        return {
            "success": False,
            "message": f"Mesh '{mesh_name}' not found in catalog",
        }

    # Create semantics sub-object if needed
    if "semantics" not in entry:
        entry["semantics"] = {}

    updated = []
    if display_name:
        entry["semantics"]["display_name"] = display_name
        updated.append("display_name")
    if description:
        entry["semantics"]["description"] = description
        updated.append("description")
    if function:
        entry["semantics"]["function"] = function
        updated.append("function")
    if style_tags is not None and len(style_tags) > 0:
        entry["semantics"]["style_tags"] = style_tags
        updated.append("style_tags")
    if spawn_weight >= 0:
        entry["semantics"]["spawn_weight"] = spawn_weight
        updated.append("spawn_weight")

    if not updated:
        return {"success": True, "message": "No fields to update", "mesh": entry["name"]}

    _save_catalog(catalog_path, catalog)

    return {
        "success": True,
        "mesh": entry["name"],
        "updated_fields": updated,
        "semantics": entry["semantics"],
    }


def query_meshes(
    catalog_name: str = "modularscifi_meshes",
    query: str = "",
    category: str = "",
    tags: Optional[List[str]] = None,
    min_size_cm: Optional[List[float]] = None,
    max_size_cm: Optional[List[float]] = None,
    has_bounds: bool = False,
    limit: int = 20,
) -> Dict[str, Any]:
    """
    Enhanced catalog search with multi-criteria filtering.

    Text query matches against name, description, function, and display_name.

    Args:
        catalog_name: Catalog JSON filename.
        query: Text search (matches name, description, function).
        category: Filter by category.
        tags: Require all of these tags.
        min_size_cm: [x, y, z] minimum size filter.
        max_size_cm: [x, y, z] maximum size filter.
        has_bounds: Only return meshes with measured bounds.
        limit: Max results to return.

    Returns:
        Dict with count and matching mesh entries.
    """
    catalog_path = _catalog_path(catalog_name)
    catalog = _load_catalog(catalog_path)
    if catalog is None:
        return {"success": False, "message": f"Catalog not found: {catalog_path}"}

    meshes = catalog.get("meshes", [])
    results = []
    query_lower = query.lower()
    category_lower = category.lower()

    for m in meshes:
        # Category filter
        if category_lower and m.get("category", "").lower() != category_lower:
            continue

        # Text query — match against name, semantics fields
        if query_lower:
            searchable = m.get("name", "").lower()
            sem = m.get("semantics", {})
            searchable += " " + sem.get("display_name", "").lower()
            searchable += " " + sem.get("description", "").lower()
            searchable += " " + sem.get("function", "").lower()
            if query_lower not in searchable:
                continue

        # Tag filter — require all specified tags
        if tags:
            entry_tags = set(m.get("tags", []))
            sem_tags = set((m.get("semantics", {}) or {}).get("style_tags", []))
            all_tags = entry_tags | sem_tags
            if not all(t.lower() in {x.lower() for x in all_tags} for t in tags):
                continue

        # Size filter — check against size_cm (flat or nested)
        size = m.get("size_cm") or (m.get("spatial", {}) or {}).get("size_cm")

        if has_bounds and size is None:
            continue

        if min_size_cm and size:
            if any(s < mn for s, mn in zip(size, min_size_cm) if mn is not None):
                continue

        if max_size_cm and size:
            if any(s > mx for s, mx in zip(size, max_size_cm) if mx is not None):
                continue

        # Build result entry
        sem = m.get("semantics", {})
        results.append({
            "name": m["name"],
            "path": m["path"],
            "category": m.get("category", ""),
            "tags": m.get("tags", []),
            "size_cm": size,
            "display_name": sem.get("display_name", ""),
            "description": sem.get("description", ""),
            "function": sem.get("function", ""),
            "style_tags": sem.get("style_tags", []),
        })

        if len(results) >= limit:
            break

    return {
        "success": True,
        "query": query,
        "category_filter": category,
        "result_count": len(results),
        "meshes": results,
    }


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _catalog_path(catalog_name: str) -> str:
    """Get full path to a catalog JSON file."""
    return os.path.join(CATALOGS_DIR, f"{catalog_name}.json")


def _load_catalog(path: str) -> Optional[Dict[str, Any]]:
    """Load catalog JSON. Returns None if not found."""
    if not os.path.isfile(path):
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def _save_catalog(path: str, data: Dict[str, Any]) -> None:
    """Write catalog JSON back to disk."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    logger.info(f"Catalog saved: {path}")


def _find_mesh_entry(catalog: Dict[str, Any], mesh_name: str) -> Optional[Dict[str, Any]]:
    """
    Find a mesh entry by name (fuzzy match).

    Tries exact match first, then case-insensitive contains.
    """
    meshes = catalog.get("meshes", [])
    name_lower = mesh_name.lower()

    # Exact match
    for m in meshes:
        if m.get("name", "") == mesh_name:
            return m

    # Case-insensitive contains
    for m in meshes:
        if name_lower in m.get("name", "").lower():
            return m

    return None


def _measure_single_mesh(
    unreal_connection,
    mesh_path: str,
    mesh_name: str,
) -> Dict[str, Any]:
    """
    Spawn a mesh at origin, measure its bounds, then delete it.

    Returns dict with success, bounds_extent, size_cm, pivot_offset_cm.
    """
    actor_name = f"_MeasureBounds_{mesh_name}"

    # Spawn at origin
    spawn_params = {
        "name": actor_name,
        "type": "StaticMeshActor",
        "location": [0.0, 0.0, 0.0],
        "rotation": [0.0, 0.0, 0.0],
        "static_mesh": mesh_path,
    }

    spawn_resp = safe_spawn_actor(unreal_connection, spawn_params)
    if not spawn_resp or spawn_resp.get("status") != "success":
        error = "Spawn failed"
        if spawn_resp:
            error = spawn_resp.get("error", spawn_resp.get("message", str(spawn_resp)))
        return {"success": False, "error": error}

    # Small delay for UE5 to register the actor
    time.sleep(0.1)

    # Query bounds
    try:
        detail_resp = unreal_connection.send_command("get_actor_details", {"name": actor_name})
    except Exception as e:
        logger.error(f"get_actor_details failed for {actor_name}: {e}")
        detail_resp = None

    # Delete immediately
    time.sleep(0.05)
    try:
        safe_delete_actor(unreal_connection, actor_name)
    except Exception as e:
        logger.warning(f"Failed to delete measurement actor {actor_name}: {e}")

    if not detail_resp:
        return {"success": False, "error": "No response from get_actor_details"}

    bbox = detail_resp.get("bounding_box", {})
    extent = bbox.get("extent")
    origin = bbox.get("origin")

    if not extent:
        return {"success": False, "error": "No bounding box in response"}

    # extent is half-size, size_cm is full dimensions
    bounds_extent = [round(float(e), 1) for e in extent]
    size_cm = [round(float(e) * 2, 1) for e in extent]

    # Pivot offset: difference between bbox origin and spawn location (0,0,0)
    pivot_offset = [0.0, 0.0, 0.0]
    if origin:
        pivot_offset = [round(float(o), 1) for o in origin]

    return {
        "success": True,
        "bounds_extent": bounds_extent,
        "size_cm": size_cm,
        "pivot_offset_cm": pivot_offset,
    }
