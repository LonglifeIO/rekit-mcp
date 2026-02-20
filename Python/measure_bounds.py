"""
Measure bounding boxes for all meshes in the catalog by spawning each
temporarily in UE5, reading bounds, then deleting.

Run with: uv run python measure_bounds.py

WARNING: This spawns and deletes actors. Run in an empty/test level.
Due to the MCP crash-prevention rule (no delete-then-spawn), this script
spawns ALL meshes first with offsets, measures them, then tells you to
delete them manually. It does NOT auto-delete.
"""
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))

from unreal_mcp_server_advanced import UnrealConnection

CATALOG_PATH = os.path.join(os.path.dirname(__file__), "catalogs", "modularscifi_meshes.json")
SPAWN_ORIGIN = [0, 0, 50000]  # High up so they don't interfere with level
GRID_SPACING = 3000  # cm between each spawned mesh
GRID_COLS = 20


def main():
    # Load catalog
    with open(CATALOG_PATH, "r", encoding="utf-8") as f:
        catalog = json.load(f)

    meshes = catalog["meshes"]
    unmeasured = [m for m in meshes if m.get("size_cm") is None]

    if not unmeasured:
        print("All meshes already have bounds measured!")
        return

    print(f"Meshes to measure: {len(unmeasured)} of {len(meshes)}")

    conn = UnrealConnection()
    measured_count = 0
    failed = []

    for i, mesh in enumerate(unmeasured):
        name = mesh["name"]
        path = mesh["path"]

        # Calculate grid position
        col = i % GRID_COLS
        row = i // GRID_COLS
        x = SPAWN_ORIGIN[0] + col * GRID_SPACING
        y = SPAWN_ORIGIN[1] + row * GRID_SPACING
        z = SPAWN_ORIGIN[2]

        actor_name = f"_Measure_{name}"

        # Spawn
        spawn_params = {
            "name": actor_name,
            "type": "StaticMeshActor",
            "location": [x, y, z],
            "rotation": [0, 0, 0],
            "static_mesh": path,
        }

        resp = conn.send_command("spawn_actor", spawn_params)
        if not resp or resp.get("status") == "error":
            err = resp.get("error", "unknown") if resp else "no response"
            failed.append({"name": name, "error": err})
            continue

        # Small delay then query
        time.sleep(0.1)

        details = conn.send_command("get_actor_details", {"name": actor_name})
        if details and details.get("status") != "error":
            result = details.get("result", details)
            ext = result.get("bounds_extent", [0, 0, 0])
            mats = [m.get("name", "") for m in result.get("materials", [])]

            mesh["bounds_extent"] = [round(e, 1) for e in ext]
            mesh["size_cm"] = [round(e * 2, 1) for e in ext]
            mesh["materials"] = mats
            measured_count += 1
        else:
            failed.append({"name": name, "error": "could not get details"})

        if (i + 1) % 25 == 0:
            print(f"  ...{i+1}/{len(unmeasured)} done ({measured_count} measured)")

    # Save updated catalog
    with open(CATALOG_PATH, "w", encoding="utf-8") as f:
        json.dump(catalog, f, indent=2)

    print(f"\nDone! Measured {measured_count} meshes. Failed: {len(failed)}")
    if failed:
        print("\nFailed meshes:")
        for f_item in failed[:20]:
            print(f"  {f_item['name']}: {f_item['error']}")

    print(f"\nCatalog updated: {CATALOG_PATH}")
    print(f"\nIMPORTANT: {measured_count} _Measure_* actors were spawned at Z=50000.")
    print("Select all _Measure_* actors in the Outliner and delete them manually.")


if __name__ == "__main__":
    main()
