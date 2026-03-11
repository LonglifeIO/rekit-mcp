"""
Filename: material_graph.py
Description: Python wrappers for material graph creation and editing commands.
Includes 7 thin wrappers for individual C++ commands and 1 compound helper
that builds a complete height-blended landscape material from texture sets.
"""

import logging
import time
from typing import Dict, Any, Optional, List

logger = logging.getLogger("MaterialGraph")


# ============================================================================
# Thin wrappers for individual C++ commands
# ============================================================================

def create_material(
    unreal_connection,
    material_name: str,
    path: str = "/Game/Materials",
    two_sided: bool = False
) -> Dict[str, Any]:
    """
    Create a new material asset.

    Args:
        unreal_connection: Connection to Unreal Engine
        material_name: Name for the new material
        path: Content path where the material will be created
        two_sided: Whether the material should be two-sided

    Returns:
        Dictionary with material_path, material_name, already_existed, success
    """
    try:
        response = unreal_connection.send_command("create_material", {
            "material_name": material_name,
            "path": path,
            "two_sided": two_sided
        })

        if response.get("success"):
            logger.info(f"Created material '{material_name}' at {path}")
        else:
            logger.error(f"Failed to create material: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in create_material: {e}")
        return {"success": False, "error": str(e)}


def add_material_expression(
    unreal_connection,
    material_path: str,
    expression_class: str,
    node_name: str = "",
    pos_x: int = 0,
    pos_y: int = 0
) -> Dict[str, Any]:
    """
    Add a material expression node to a material graph.

    Args:
        unreal_connection: Connection to Unreal Engine
        material_path: Content path to the material
        expression_class: Type of expression (e.g. "TextureSample", "WorldPosition",
            "Lerp", "ComponentMask", "Divide", "Subtract", "Clamp", "Constant",
            "TextureCoordinate", "Multiply", "Add", "ScalarParameter", "VectorParameter")
        node_name: Friendly name for later reference
        pos_x: X position in graph
        pos_y: Y position in graph

    Returns:
        Dictionary with expression_name, expression_class, expression_index, success
    """
    try:
        params = {
            "material_path": material_path,
            "expression_class": expression_class,
            "pos_x": pos_x,
            "pos_y": pos_y
        }
        if node_name:
            params["node_name"] = node_name

        response = unreal_connection.send_command("add_material_expression", params)

        if response.get("success"):
            logger.info(f"Added {expression_class} expression '{node_name}' to {material_path}")
        else:
            logger.error(f"Failed to add expression: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in add_material_expression: {e}")
        return {"success": False, "error": str(e)}


def set_material_expression_param(
    unreal_connection,
    material_path: str,
    expression_name: str,
    param_name: str,
    param_value: Any
) -> Dict[str, Any]:
    """
    Set a parameter on a material expression.

    Args:
        unreal_connection: Connection to Unreal Engine
        material_path: Content path to the material
        expression_name: Name/identifier of the expression
        param_name: Name of the parameter to set
        param_value: Value to set (type depends on parameter)

    Returns:
        Dictionary with success status
    """
    try:
        response = unreal_connection.send_command("set_material_expression_param", {
            "material_path": material_path,
            "expression_name": expression_name,
            "param_name": param_name,
            "param_value": param_value
        })

        if response.get("success"):
            logger.info(f"Set {param_name} on '{expression_name}'")
        else:
            logger.error(f"Failed to set param: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in set_material_expression_param: {e}")
        return {"success": False, "error": str(e)}


def connect_material_expressions(
    unreal_connection,
    material_path: str,
    from_expression: str,
    to_expression: str,
    from_output_index: int = 0,
    to_input_index: int = 0
) -> Dict[str, Any]:
    """
    Connect two material expressions.

    Args:
        unreal_connection: Connection to Unreal Engine
        material_path: Content path to the material
        from_expression: Source expression name
        to_expression: Target expression name
        from_output_index: Output pin index on source
        to_input_index: Input pin index on target

    Returns:
        Dictionary with success status
    """
    try:
        response = unreal_connection.send_command("connect_material_expressions", {
            "material_path": material_path,
            "from_expression": from_expression,
            "to_expression": to_expression,
            "from_output_index": from_output_index,
            "to_input_index": to_input_index
        })

        if response.get("success"):
            logger.info(f"Connected '{from_expression}' -> '{to_expression}'")
        else:
            logger.error(f"Failed to connect: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in connect_material_expressions: {e}")
        return {"success": False, "error": str(e)}


def connect_material_to_output(
    unreal_connection,
    material_path: str,
    expression_name: str,
    material_property: str,
    output_index: int = 0
) -> Dict[str, Any]:
    """
    Connect a material expression to a material output property.

    Args:
        unreal_connection: Connection to Unreal Engine
        material_path: Content path to the material
        expression_name: Name of the expression to connect
        material_property: Target property (BaseColor, Normal, Roughness,
            Metallic, AmbientOcclusion, EmissiveColor, Specular, Opacity)
        output_index: Output pin index on the expression

    Returns:
        Dictionary with success status
    """
    try:
        response = unreal_connection.send_command("connect_material_to_output", {
            "material_path": material_path,
            "expression_name": expression_name,
            "material_property": material_property,
            "output_index": output_index
        })

        if response.get("success"):
            logger.info(f"Connected '{expression_name}' to {material_property}")
        else:
            logger.error(f"Failed to connect to output: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in connect_material_to_output: {e}")
        return {"success": False, "error": str(e)}


def set_landscape_material(
    unreal_connection,
    actor_name: str,
    material_path: str
) -> Dict[str, Any]:
    """
    Assign a material to a landscape actor.

    Args:
        unreal_connection: Connection to Unreal Engine
        actor_name: Name of the landscape actor
        material_path: Content path to the material

    Returns:
        Dictionary with success status
    """
    try:
        response = unreal_connection.send_command("set_landscape_material", {
            "actor_name": actor_name,
            "material_path": material_path
        })

        if response.get("success"):
            logger.info(f"Assigned material '{material_path}' to landscape '{actor_name}'")
        else:
            logger.error(f"Failed to set landscape material: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in set_landscape_material: {e}")
        return {"success": False, "error": str(e)}


def compile_material(
    unreal_connection,
    material_path: str
) -> Dict[str, Any]:
    """
    Recompile and save a material.

    Args:
        unreal_connection: Connection to Unreal Engine
        material_path: Content path to the material

    Returns:
        Dictionary with success status and message
    """
    try:
        response = unreal_connection.send_command("compile_material", {
            "material_path": material_path
        })

        if response.get("success"):
            logger.info(f"Compiled material at '{material_path}'")
        else:
            logger.error(f"Failed to compile material: {response.get('error', 'Unknown error')}")

        return response

    except Exception as e:
        logger.error(f"Exception in compile_material: {e}")
        return {"success": False, "error": str(e)}


# ============================================================================
# Compound helper: Build height-blended landscape material
# ============================================================================

def create_landscape_height_material(
    unreal_connection,
    material_name: str,
    material_path: str = "/Game/Materials",
    low_diffuse: str = "",
    low_normal: str = "",
    low_orm: str = "",
    high_diffuse: str = "",
    high_normal: str = "",
    high_orm: str = "",
    blend_start: float = 5000.0,
    blend_range: float = 2000.0,
    tiling: float = 4.0,
    landscape_actor: str = ""
) -> Dict[str, Any]:
    """
    Build a complete WorldPosition.Z height-blended landscape material.

    Creates a material with two texture layers (low/high elevation) that blend
    based on world Z height. Snow at high elevations, rock/ice at low. Fully
    automatic, no manual landscape painting needed.

    Material graph structure:
        TexCoord -> 6 TextureSamples (low+high diffuse/normal/orm)
        WorldPosition -> ComponentMask(Z) -> Subtract(blend_start) -> Divide(blend_range) -> Clamp(0,1)
        Lerp_diffuse(low, high, alpha) -> BaseColor
        Lerp_normal(low, high, alpha) -> Normal
        Lerp_orm R channel -> Roughness, G channel -> Metallic, B channel -> AO

    Args:
        unreal_connection: Connection to Unreal Engine
        material_name: Name for the new material
        material_path: Content path (default /Game/Materials)
        low_diffuse: Texture path for low-elevation diffuse/albedo
        low_normal: Texture path for low-elevation normal map
        low_orm: Texture path for low-elevation ORM (Occlusion/Roughness/Metallic)
        high_diffuse: Texture path for high-elevation diffuse/albedo
        high_normal: Texture path for high-elevation normal map
        high_orm: Texture path for high-elevation ORM
        blend_start: World Z (cm) where blend begins (default 5000)
        blend_range: Transition width in cm (default 2000)
        tiling: UV tiling scale (default 4)
        landscape_actor: If provided, auto-assigns the material to this landscape

    Returns:
        Dictionary with material_path, operations_completed count, success
    """
    DELAY = 0.15  # seconds between sequential commands
    errors = []
    operations_completed = 0
    mat_full_path = f"{material_path}/{material_name}"

    def _send(func, *args, step_name="", **kwargs):
        nonlocal operations_completed
        time.sleep(DELAY)
        result = func(unreal_connection, *args, **kwargs)
        if not result.get("success"):
            errors.append(f"{step_name}: {result.get('error', 'Unknown error')}")
            logger.error(f"Step '{step_name}' failed: {result.get('error')}")
        else:
            operations_completed += 1
        return result

    try:
        # Step 1: Create material
        logger.info(f"Creating height-blend material '{material_name}' at '{material_path}'")
        _send(create_material, material_name, material_path, step_name="create_material")
        if errors:
            return {"success": False, "error": f"Failed at material creation: {errors[-1]}"}

        # Step 2: Add TexCoord node
        _send(add_material_expression, mat_full_path, "TextureCoordinate", "TexCoord", -1200, -400)

        # Step 3: Set tiling
        _send(set_material_expression_param, mat_full_path, "TexCoord", "UTiling", tiling, step_name="set_utiling")
        _send(set_material_expression_param, mat_full_path, "TexCoord", "VTiling", tiling, step_name="set_vtiling")

        # Step 4: Add 6 texture sample nodes
        tex_nodes = [
            ("low_diffuse",  "TextureSample", "LowDiffuse",  -800, -600),
            ("low_normal",   "TextureSample", "LowNormal",   -800, -300),
            ("low_orm",      "TextureSample", "LowORM",      -800, 0),
            ("high_diffuse", "TextureSample", "HighDiffuse",  -800, 300),
            ("high_normal",  "TextureSample", "HighNormal",   -800, 600),
            ("high_orm",     "TextureSample", "HighORM",      -800, 900),
        ]

        texture_paths = {
            "low_diffuse": low_diffuse,
            "low_normal": low_normal,
            "low_orm": low_orm,
            "high_diffuse": high_diffuse,
            "high_normal": high_normal,
            "high_orm": high_orm,
        }

        for key, expr_class, name, px, py in tex_nodes:
            _send(add_material_expression, mat_full_path, expr_class, name, px, py, step_name=f"add_{name}")

            # Set texture path if provided
            tex_path = texture_paths.get(key, "")
            if tex_path:
                _send(set_material_expression_param, mat_full_path, name, "Texture", tex_path, step_name=f"set_tex_{name}")

            # Set normal sampler type for normal maps
            if "normal" in key:
                _send(set_material_expression_param, mat_full_path, name, "SamplerType", "Normal", step_name=f"sampler_{name}")

            # Set linear sampler for ORM
            if "orm" in key:
                _send(set_material_expression_param, mat_full_path, name, "SamplerType", "LinearColor", step_name=f"sampler_{name}")

            # Connect TexCoord to each texture sample UVs (input index 0)
            _send(connect_material_expressions, mat_full_path, "TexCoord", name, 0, 0, step_name=f"connect_uv_{name}")

        # Step 5: Build the height blend alpha chain
        # WorldPosition
        _send(add_material_expression, mat_full_path, "WorldPosition", "WorldPos", -1200, 1200)

        # ComponentMask (Z only)
        _send(add_material_expression, mat_full_path, "ComponentMask", "MaskZ", -1000, 1200)
        _send(set_material_expression_param, mat_full_path, "MaskZ", "R_mask", False, step_name="mask_r")
        _send(set_material_expression_param, mat_full_path, "MaskZ", "G_mask", False, step_name="mask_g")
        _send(set_material_expression_param, mat_full_path, "MaskZ", "B_mask", True, step_name="mask_b")
        _send(set_material_expression_param, mat_full_path, "MaskZ", "A_mask", False, step_name="mask_a")

        # Connect WorldPos -> MaskZ
        _send(connect_material_expressions, mat_full_path, "WorldPos", "MaskZ", 0, 0, step_name="connect_worldpos_mask")

        # Subtract (- blend_start)
        _send(add_material_expression, mat_full_path, "Subtract", "SubBlendStart", -800, 1200)
        _send(set_material_expression_param, mat_full_path, "SubBlendStart", "ConstB", blend_start, step_name="set_blend_start")

        # Connect MaskZ -> Subtract.A
        _send(connect_material_expressions, mat_full_path, "MaskZ", "SubBlendStart", 0, 0, step_name="connect_mask_sub")

        # Divide (/ blend_range)
        _send(add_material_expression, mat_full_path, "Divide", "DivBlendRange", -600, 1200)
        _send(set_material_expression_param, mat_full_path, "DivBlendRange", "ConstB", blend_range, step_name="set_blend_range")

        # Connect Subtract -> Divide.A
        _send(connect_material_expressions, mat_full_path, "SubBlendStart", "DivBlendRange", 0, 0, step_name="connect_sub_div")

        # Clamp (0-1)
        _send(add_material_expression, mat_full_path, "Clamp", "ClampAlpha", -400, 1200)
        _send(set_material_expression_param, mat_full_path, "ClampAlpha", "MinDefault", 0.0, step_name="clamp_min")
        _send(set_material_expression_param, mat_full_path, "ClampAlpha", "MaxDefault", 1.0, step_name="clamp_max")

        # Connect Divide -> Clamp
        _send(connect_material_expressions, mat_full_path, "DivBlendRange", "ClampAlpha", 0, 0, step_name="connect_div_clamp")

        # Step 6: Add 3 Lerp nodes (diffuse, normal, ORM)
        _send(add_material_expression, mat_full_path, "Lerp", "LerpDiffuse", -200, -400)
        _send(add_material_expression, mat_full_path, "Lerp", "LerpNormal", -200, 0)
        _send(add_material_expression, mat_full_path, "Lerp", "LerpORM", -200, 400)

        # Connect texture samples to lerps
        # Lerp inputs: 0=A, 1=B, 2=Alpha
        # Diffuse: A=Low, B=High
        _send(connect_material_expressions, mat_full_path, "LowDiffuse", "LerpDiffuse", 0, 0, step_name="connect_lowdiff_lerp")
        _send(connect_material_expressions, mat_full_path, "HighDiffuse", "LerpDiffuse", 0, 1, step_name="connect_highdiff_lerp")
        _send(connect_material_expressions, mat_full_path, "ClampAlpha", "LerpDiffuse", 0, 2, step_name="connect_alpha_lerpdiff")

        # Normal: A=Low, B=High
        _send(connect_material_expressions, mat_full_path, "LowNormal", "LerpNormal", 0, 0, step_name="connect_lownorm_lerp")
        _send(connect_material_expressions, mat_full_path, "HighNormal", "LerpNormal", 0, 1, step_name="connect_highnorm_lerp")
        _send(connect_material_expressions, mat_full_path, "ClampAlpha", "LerpNormal", 0, 2, step_name="connect_alpha_lerpnorm")

        # ORM: A=Low, B=High
        _send(connect_material_expressions, mat_full_path, "LowORM", "LerpORM", 0, 0, step_name="connect_loworm_lerp")
        _send(connect_material_expressions, mat_full_path, "HighORM", "LerpORM", 0, 1, step_name="connect_highorm_lerp")
        _send(connect_material_expressions, mat_full_path, "ClampAlpha", "LerpORM", 0, 2, step_name="connect_alpha_lerporm")

        # Step 7: Connect lerps to material outputs
        _send(connect_material_to_output, mat_full_path, "LerpDiffuse", "BaseColor", 0, step_name="output_basecolor")
        _send(connect_material_to_output, mat_full_path, "LerpNormal", "Normal", 0, step_name="output_normal")

        # ORM outputs: R=AO, G=Roughness, B=Metallic (standard Megascans ORM packing)
        # LerpORM output index 1=R (AO), 2=G (Roughness), 3=B (Metallic)
        _send(connect_material_to_output, mat_full_path, "LerpORM", "AmbientOcclusion", 1, step_name="output_ao")
        _send(connect_material_to_output, mat_full_path, "LerpORM", "Roughness", 2, step_name="output_roughness")
        _send(connect_material_to_output, mat_full_path, "LerpORM", "Metallic", 3, step_name="output_metallic")

        # Step 8: Compile
        _send(compile_material, mat_full_path, step_name="compile")

        # Step 9: Assign to landscape if requested
        if landscape_actor:
            _send(set_landscape_material, landscape_actor, mat_full_path, step_name="assign_landscape")

        # Build result
        result = {
            "success": len(errors) == 0,
            "material_path": mat_full_path,
            "operations_completed": operations_completed,
        }

        if errors:
            result["warnings"] = errors
            result["message"] = f"Material created with {len(errors)} warning(s). Some steps may have failed."
        else:
            result["message"] = f"Height-blended landscape material created successfully with {operations_completed} operations."

        if landscape_actor:
            result["landscape_actor"] = landscape_actor

        return result

    except Exception as e:
        logger.error(f"Exception in create_landscape_height_material: {e}")
        return {"success": False, "error": str(e), "partial_errors": errors}
