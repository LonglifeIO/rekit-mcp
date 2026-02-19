"""
Filename: node_manager.py
Description: Python wrapper for PCG node addition and deletion
"""

import logging
from typing import Dict, Any, Optional

logger = logging.getLogger("PCGGraph.NodeManager")


def add_pcg_node(
    unreal_connection,
    graph_path: str,
    node_type: str,
    pos_x: int = 0,
    pos_y: int = 0
) -> Dict[str, Any]:
    """
    Add a node to a PCG graph.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        node_type: Type of node (e.g., "SurfaceSampler", "StaticMeshSpawner", "TransformPoints",
                   "CreatePointsGrid", "DensityFilter", "CopyPoints", "Merge", "Difference",
                   "Subgraph", "GetActorData", etc.)
        pos_x: X position in graph editor (default: 0)
        pos_y: Y position in graph editor (default: 0)

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - node_id (str): FName identifier of the created node
            - settings_class (str): Class name of the node's settings
            - input_pins (list): Available input pins
            - output_pins (list): Available output pins
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("add_pcg_node", {
            "graph_path": graph_path,
            "node_type": node_type,
            "pos_x": pos_x,
            "pos_y": pos_y
        })

        if response.get("success"):
            logger.info(
                f"Successfully added {node_type} node to PCG graph. "
                f"Node ID: {response.get('node_id')}"
            )
        else:
            logger.error(f"Failed to add PCG node: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in add_pcg_node: {e}")
        return {"success": False, "error": str(e)}


def delete_pcg_node(
    unreal_connection,
    graph_path: str,
    node_id: str
) -> Dict[str, Any]:
    """
    Delete a node from a PCG graph.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        node_id: FName identifier of the node to delete

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - deleted_node_id (str): ID of the deleted node
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("delete_pcg_node", {
            "graph_path": graph_path,
            "node_id": node_id
        })

        if response.get("success"):
            logger.info(f"Successfully deleted node '{node_id}' from PCG graph")
        else:
            logger.error(f"Failed to delete PCG node: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in delete_pcg_node: {e}")
        return {"success": False, "error": str(e)}
