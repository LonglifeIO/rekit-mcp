# Prompt: Investigate Adding PCG Graph Support to Unreal MCP Plugin

Use this prompt in a new Claude Code session with the MCP plugin repo as the working directory (`F:\UE Projects\unreal-engine-mcp`).

---

## Context

I have an Unreal Engine MCP (Model Context Protocol) plugin that lets an AI assistant manipulate UE5 Blueprints via a TCP JSON bridge. The plugin currently supports:
- Creating/modifying Blueprint actors, variables, components
- Adding/connecting/deleting Blueprint graph nodes (K2Nodes)
- Setting node properties, creating functions, managing events

I want to **extend it to support PCG (Procedural Content Generation) graphs** — creating PCG Graph assets, adding PCG nodes, connecting them, setting their properties, and managing PCG graph parameters.

## Architecture Summary

The plugin follows this pattern:
1. **TCP Server** (`MCPServerRunnable`) listens on `127.0.0.1:55557`, receives JSON commands
2. **Bridge** (`EpicUnrealMCPBridge`) routes commands by type string to handler classes
3. **Command Handlers** (e.g., `FEpicUnrealMCPBlueprintGraphCommands`) dispatch to specialist classes
4. **Specialists** (e.g., `FBlueprintNodeManager`, `FBPVariables`, `FBPConnector`) execute UE5 API calls
5. Results return as JSON over TCP

To add PCG support, I need to follow this same pattern with new handler + specialist files.

## What I Need You To Investigate

### 1. UE5.7 PCG C++ API Surface

Research the UE5 C++ API for PCG graph manipulation. Key classes to investigate:

- `UPCGGraph` — the graph asset itself
- `UPCGSettings` / `UPCGNode` — individual nodes in the graph
- `UPCGComponent` — the component that drives generation
- `FPCGGraphParameter` or equivalent — graph parameters (Int32, Float, Bool, etc.)
- `UPCGBlueprintSettings` / PCG node types — how different PCG node types are created programmatically

Specific questions:
- How do you programmatically create a `UPCGGraph` asset (equivalent to right-click → PCG → PCG Graph)?
- How do you add nodes to a PCG graph? Is there a `UPCGNode::CreateNode()` or factory pattern?
- How do you connect PCG node pins? What's the pin/edge API?
- How do you set node settings/properties (e.g., CellSize on a Create Points Grid node)?
- How do you add/modify graph parameters (the equivalent of Blueprint variables but for PCG)?
- How do you create a UEnum asset programmatically?

### 2. Required MCP Commands

Based on the PCG API, design these MCP commands (JSON in/out format matching existing patterns):

```
create_pcg_graph          — Create a new PCG Graph asset at a specified content path
add_pcg_node              — Add a node to a PCG graph by type (CreatePointsGrid, TransformPoints, AttributeFilter, StaticMeshSpawner, Union, etc.)
connect_pcg_nodes         — Connect output pin of one node to input pin of another
set_pcg_node_property     — Set a property on a PCG node (e.g., CellSize, GridExtents, Operator, AttributeName)
add_pcg_graph_parameter   — Add a typed parameter to the PCG graph
set_pcg_graph_parameter   — Set default value of a graph parameter
read_pcg_graph            — Return the full graph structure (nodes, connections, parameters) as JSON
delete_pcg_node           — Remove a node from the graph
assign_pcg_graph_to_component — Set which PCG graph a PCGComponent uses
create_enum_asset         — Create a UEnum asset (needed for door side enum, etc.)
move_asset                — Move/rename a content browser asset to a different folder
```

### 3. Implementation Plan

For each command, outline:
- The UE5 C++ API calls needed
- The JSON input schema (required/optional params)
- The JSON output schema
- Any gotchas or limitations

### 4. File Structure

Following the existing pattern, propose:
- New header/cpp files needed (e.g., `EpicUnrealMCPPCGCommands.h/.cpp`, specialist files)
- Changes to `EpicUnrealMCPBridge.cpp` for command routing
- Changes to `UnrealMCP.Build.cs` for PCG module dependencies

### 5. PCG Node Type Registry

List the most important PCG node types and their key properties that the MCP should support. Priority order:

**Must have (covers 90% of PCG work):**
- Create Points Grid (CellSize, GridExtents)
- Transform Points (Offset, Rotation, Scale, Mode)
- Attribute Operation (Operation, Input1, Input2, OutputTarget)
- Create Attribute (AttributeName, Source, Value)
- Attribute Filter (Operator, TargetAttribute, ThresholdSource, ThresholdValue)
- Static Mesh Spawner (mesh entries with paths and weights)
- Union (merge multiple point streams)
- Density Noise (seed, mode)
- Density Filter (bounds)

**Nice to have:**
- Copy Points
- Surface Sampler
- Spline Sampler
- Bounds Modifier
- Point Filter (by tag, by index)
- Subgraph

### 6. Python-Side Tool Definitions

Show what the corresponding Python MCP tool definitions would look like (the `@tool` decorated functions that map to JSON commands), matching the existing pattern in the Python bridge layer.

## Key Files to Read

Read these existing files to understand the patterns before designing PCG support:

**Architecture (start here):**
- `UnrealMCP/Source/UnrealMCP/Private/EpicUnrealMCPBridge.cpp` — command routing
- `UnrealMCP/Source/UnrealMCP/Public/EpicUnrealMCPBridge.h` — handler registration

**Blueprint graph manipulation (closest analogue to PCG):**
- `UnrealMCP/Source/UnrealMCP/Public/Commands/EpicUnrealMCPBlueprintGraphCommands.h`
- `UnrealMCP/Source/UnrealMCP/Private/Commands/EpicUnrealMCPBlueprintGraphCommands.cpp`
- `UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintGraph/NodeManager.cpp` — how nodes are created
- `UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintGraph/BPConnector.cpp` — how nodes are connected
- `UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintGraph/BPVariables.cpp` — how variables are created

**Utilities:**
- `UnrealMCP/Source/UnrealMCP/Private/Commands/EpicUnrealMCPCommonUtils.cpp` — shared helpers

**Build config:**
- `UnrealMCP/UnrealMCP.Build.cs` — module dependencies

## Constraints

- UE5.7 (latest, PCG is production-ready)
- Plugin is Editor-only (no runtime)
- All operations must execute on GameThread (use AsyncTask pattern from Bridge)
- JSON communication via TCP (no HTTP, no WebSocket)
- Must handle PCG graph regeneration after modifications
- PCG graphs are NOT Blueprint graphs — they use a completely different node system (UPCGNode, not UK2Node)

## Expected Output

A detailed implementation document with:
1. API research findings (with specific class/method names)
2. Command schemas (JSON in/out for each command)
3. C++ implementation outlines for each command
4. File structure and build.cs changes
5. A recommended build order (which commands to implement first for maximum value)
6. Any blockers or unknowns that need empirical testing in-editor
