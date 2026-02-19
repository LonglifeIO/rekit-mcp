"""
Filename: parameter_manager.py
Description: Python wrapper for PCG graph parameter management and graph assignment
"""

import logging
from typing import Dict, Any, Optional, Union

logger = logging.getLogger("PCGGraph.ParameterManager")


def add_pcg_graph_parameter(
    unreal_connection,
    graph_path: str,
    param_name: str,
    param_type: str
) -> Dict[str, Any]:
    """
    Add a user parameter to a PCG graph.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        param_name: Name for the parameter
        param_type: Type of the parameter. Supported:
            Bool, Int32, Int64, Float, Double, String, Name,
            Vector, Rotator, Transform, SoftObjectPath

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - param_name (str): Name of the added parameter
            - param_type (str): Type of the added parameter
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("add_pcg_graph_parameter", {
            "graph_path": graph_path,
            "param_name": param_name,
            "param_type": param_type
        })

        if response.get("success"):
            logger.info(f"Successfully added parameter '{param_name}' ({param_type}) to PCG graph")
        else:
            logger.error(f"Failed to add PCG graph parameter: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in add_pcg_graph_parameter: {e}")
        return {"success": False, "error": str(e)}


def set_pcg_graph_parameter(
    unreal_connection,
    graph_path: str,
    param_name: str,
    default_value: Any
) -> Dict[str, Any]:
    """
    Set a user parameter's default value on a PCG graph.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        param_name: Name of the parameter to set
        default_value: Default value (type must match the parameter's declared type)

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - param_name (str): Name of the parameter
            - new_value (str): String representation of the set value
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("set_pcg_graph_parameter", {
            "graph_path": graph_path,
            "param_name": param_name,
            "default_value": default_value
        })

        if response.get("success"):
            logger.info(f"Successfully set parameter '{param_name}' default value")
        else:
            logger.error(f"Failed to set PCG graph parameter: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in set_pcg_graph_parameter: {e}")
        return {"success": False, "error": str(e)}


def assign_pcg_graph(
    unreal_connection,
    graph_path: str,
    actor_name: Optional[str] = None,
    blueprint_name: Optional[str] = None
) -> Dict[str, Any]:
    """
    Assign a PCG graph to an actor's or Blueprint's PCGComponent.

    Args:
        unreal_connection: Connection to Unreal Engine
        graph_path: Content path to the PCG graph
        actor_name: Name of an actor in the level (provide this OR blueprint_name)
        blueprint_name: Name of a Blueprint asset (provide this OR actor_name)

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - graph_path (str): Path of the assigned graph
            - assigned_to_actor or assigned_to_blueprint (str): Target name
            - error (str): Error message if failed
    """
    try:
        params = {"graph_path": graph_path}
        if actor_name:
            params["actor_name"] = actor_name
        elif blueprint_name:
            params["blueprint_name"] = blueprint_name
        else:
            return {"success": False, "error": "Must provide either actor_name or blueprint_name"}

        response = unreal_connection.send_command("assign_pcg_graph", params)

        if response.get("success"):
            target = actor_name or blueprint_name
            logger.info(f"Successfully assigned PCG graph to '{target}'")
        else:
            logger.error(f"Failed to assign PCG graph: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in assign_pcg_graph: {e}")
        return {"success": False, "error": str(e)}


def generate_pcg(
    unreal_connection,
    actor_name: str
) -> Dict[str, Any]:
    """
    Force-generate a PCG graph on an actor's PCGComponent.

    Args:
        unreal_connection: Connection to Unreal Engine
        actor_name: Name of actor in the level with a PCGComponent

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - actor_name (str): Actor that was generated
            - error (str): Error message if failed
    """
    try:
        response = unreal_connection.send_command("generate_pcg", {
            "actor_name": actor_name
        })

        if response.get("success"):
            logger.info(f"Successfully triggered PCG generation on '{actor_name}'")
        else:
            logger.error(f"Failed to generate PCG: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in generate_pcg: {e}")
        return {"success": False, "error": str(e)}
