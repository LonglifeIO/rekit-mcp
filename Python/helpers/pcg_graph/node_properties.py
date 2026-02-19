"""
Filename: node_properties.py
Description: Python wrapper for PCG node property manipulation
"""

import logging
from typing import Dict, Any, Union, List

logger = logging.getLogger("PCGGraph.NodeProperties")


def set_pcg_node_property(
    unreal_connection,
    graph_path: str,
    node_id: str,
    property_name: str,
    property_value: Any
) -> Dict[str, Any]:
    """
    Set a property on a PCG node's settings.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        node_id: FName of the node
        property_name: Name of the property on the node's UPCGSettings
        property_value: Value to set. Accepted types:
            - bool, int, float for simple properties
            - [x, y, z] list for FVector
            - [pitch, yaw, roll] list for FRotator
            - {"location": [...], "rotation": [...], "scale": [...]} for FTransform
            - str for FString, FName, FSoftObjectPath, enum names
            - str for ImportText fallback on complex structs

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - node_id (str): Node that was modified
            - property_name (str): Property that was set
            - property_type (str): Detected property type
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("set_pcg_node_property", {
            "graph_path": graph_path,
            "node_id": node_id,
            "property_name": property_name,
            "property_value": property_value
        })

        if response.get("success"):
            logger.info(
                f"Successfully set '{property_name}' on node '{node_id}' "
                f"(type: {response.get('property_type')})"
            )
        else:
            logger.error(f"Failed to set PCG node property: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in set_pcg_node_property: {e}")
        return {"success": False, "error": str(e)}


def get_pcg_node_property(
    unreal_connection,
    graph_path: str,
    node_id: str,
    property_name: str
) -> Dict[str, Any]:
    """
    Read a property value from a PCG node's settings.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        node_id: FName of the node
        property_name: Name of the property (supports dot-notation, e.g. "InputSource1.AttributeName")

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - node_id (str): Node that was queried
            - property_name (str): Property that was read
            - property_type (str): Detected property type
            - property_value: The current value
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("get_pcg_node_property", {
            "graph_path": graph_path,
            "node_id": node_id,
            "property_name": property_name
        })

        if response.get("success"):
            logger.info(
                f"Read '{property_name}' from node '{node_id}': "
                f"{response.get('property_value')} (type: {response.get('property_type')})"
            )
        else:
            logger.error(f"Failed to get PCG node property: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in get_pcg_node_property: {e}")
        return {"success": False, "error": str(e)}
