"""
Filename: spawner_entries.py
Description: Python wrapper for PCG Static Mesh Spawner entry manipulation
"""

import logging
from typing import Dict, Any, List

logger = logging.getLogger("PCGGraph.SpawnerEntries")


def set_pcg_spawner_entries(
    unreal_connection,
    graph_path: str,
    node_id: str,
    entries: List[Dict[str, Any]]
) -> Dict[str, Any]:
    """
    Set mesh entries on a PCG Static Mesh Spawner node.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        node_id: FName of the StaticMeshSpawner node
        entries: List of dictionaries, each with:
            - mesh_path (str): Content path to a static mesh asset
            - weight (int, optional): Selection weight (default: 1)

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - node_id (str): Node that was modified
            - entry_count (int): Number of entries set
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("set_pcg_spawner_entries", {
            "graph_path": graph_path,
            "node_id": node_id,
            "entries": entries
        })

        if response.get("success"):
            logger.info(
                f"Successfully set {response.get('entry_count', 0)} mesh entries "
                f"on spawner node '{node_id}'"
            )
        else:
            logger.error(f"Failed to set spawner entries: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in set_pcg_spawner_entries: {e}")
        return {"success": False, "error": str(e)}
