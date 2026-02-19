"""
Filename: graph_creator.py
Description: Python wrapper for PCG graph creation and reading
"""

import logging
from typing import Dict, Any, Optional

logger = logging.getLogger("PCGGraph.GraphCreator")


def create_pcg_graph(
    unreal_connection,
    graph_name: str,
    path: str = "/Game/PCG"
) -> Dict[str, Any]:
    """
    Create a new PCG graph asset.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_name: Name for the new PCG graph
        path: Content path where the graph will be created (default: /Game/PCG)

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - graph_path (str): Full content path to the created graph
            - graph_name (str): Name of the created graph
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("create_pcg_graph", {
            "graph_name": graph_name,
            "path": path
        })

        if response.get("success"):
            logger.info(f"Successfully created PCG graph '{graph_name}' at {path}")
        else:
            logger.error(f"Failed to create PCG graph: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in create_pcg_graph: {e}")
        return {"success": False, "error": str(e)}


def read_pcg_graph(
    unreal_connection,
    graph_path: str
) -> Dict[str, Any]:
    """
    Read/inspect an existing PCG graph structure.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Full content path to the PCG graph

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - nodes (list): Array of node objects with id, settings_class, pins
            - special_nodes (list): Input/Output nodes
            - connections (list): Array of edge objects
            - parameters (list): Array of user parameter descriptors
            - node_count (int): Total number of user nodes
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("read_pcg_graph", {
            "graph_path": graph_path
        })

        if response.get("success"):
            node_count = response.get("node_count", 0)
            logger.info(f"Successfully read PCG graph at '{graph_path}' ({node_count} nodes)")
        else:
            logger.error(f"Failed to read PCG graph: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in read_pcg_graph: {e}")
        return {"success": False, "error": str(e)}
