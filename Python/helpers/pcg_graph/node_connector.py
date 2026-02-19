"""
Filename: node_connector.py
Description: Python wrapper for PCG node connections
"""

import logging
from typing import Dict, Any, Optional

logger = logging.getLogger("PCGGraph.NodeConnector")


def connect_pcg_nodes(
    unreal_connection,
    graph_path: str,
    from_node_id: str,
    to_node_id: str,
    from_pin: str = "Out",
    to_pin: str = "In"
) -> Dict[str, Any]:
    """
    Connect two nodes in a PCG graph.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        from_node_id: FName of the source node
        to_node_id: FName of the target node
        from_pin: Label of the output pin on source node (default: "Out")
        to_pin: Label of the input pin on target node (default: "In")

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - from_node_id (str): Source node ID
            - from_pin (str): Source pin label
            - to_node_id (str): Target node ID
            - to_pin (str): Target pin label
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("connect_pcg_nodes", {
            "graph_path": graph_path,
            "from_node_id": from_node_id,
            "from_pin": from_pin,
            "to_node_id": to_node_id,
            "to_pin": to_pin
        })

        if response.get("success"):
            logger.info(
                f"Successfully connected {from_node_id}.{from_pin} -> {to_node_id}.{to_pin}"
            )
        else:
            logger.error(f"Failed to connect PCG nodes: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in connect_pcg_nodes: {e}")
        return {"success": False, "error": str(e)}
