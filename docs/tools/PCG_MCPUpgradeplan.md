# Plan: Add PCG Graph Support to Unreal MCP Plugin

## Context

The Unreal MCP plugin (`F:\UE Projects\unreal-engine-mcp`) currently supports Blueprint graph manipulation via TCP JSON commands. Jordan's ThoughtSpace project uses PCG (Procedural Content Generation) graphs extensively (e.g., the PCG Hab Generator). Currently, PCG graphs must be built entirely by hand in the editor. Adding PCG support to the MCP plugin enables AI-assisted PCG graph creation, letting Claude build and modify PCG graphs programmatically — the same way it already manipulates Blueprint graphs.

**Working directory for implementation:** `F:\UE Projects\unreal-engine-mcp`

---

## Architecture (How It Fits In)

```
Claude MCP Tool → Python @mcp.tool() → JSON-over-TCP → C++ Handler → UE5 PCG API
```

New components mirror the existing Blueprint graph pattern:
- **C++ Handler:** `FEpicUnrealMCPPCGGraphCommands` (mirrors `FEpicUnrealMCPBlueprintGraphCommands`)
- **C++ Specialists:** `PCGGraph/*.cpp` files (mirrors `BlueprintGraph/*.cpp`)
- **Python Helpers:** `helpers/pcg_graph/*.py` (mirrors `helpers/blueprint_graph/*.py`)
- **Python Tools:** `@mcp.tool()` registrations in `unreal_mcp_server_advanced.py`

---

## Files to Create

### C++ (Plugin Side)

```
Source/UnrealMCP/
  Public/Commands/
    EpicUnrealMCPPCGGraphCommands.h           # Handler class header
    PCGGraph/
      PCGGraphCreator.h                        # create_pcg_graph, read_pcg_graph
      PCGNodeManager.h                         # add_pcg_node, delete_pcg_node
      PCGNodeConnector.h                       # connect_pcg_nodes
      PCGNodePropertyManager.h                 # set_pcg_node_property
      PCGParameterManager.h                    # add/set_pcg_graph_parameter
      PCGComponentAssigner.h                   # assign_pcg_graph_to_component
  Private/Commands/
    EpicUnrealMCPPCGGraphCommands.cpp          # Handler dispatch
    PCGGraph/
      PCGGraphCreator.cpp
      PCGNodeManager.cpp
      PCGNodeConnector.cpp
      PCGNodePropertyManager.cpp
      PCGParameterManager.cpp
      PCGComponentAssigner.cpp
```

### Python (Server Side)

```
Python/helpers/pcg_graph/
  __init__.py
  graph_creator.py                             # create_pcg_graph, read_pcg_graph
  node_manager.py                              # add_pcg_node, delete_pcg_node
  node_connector.py                            # connect_pcg_nodes
  node_properties.py                           # set_pcg_node_property
  parameter_manager.py                         # add/set_pcg_graph_parameter
  component_assigner.py                        # assign_pcg_graph_to_component
```

---

## Files to Modify

| File | Change |
|------|--------|
| `UnrealMCP.Build.cs` | Add `"PCG"` to `PublicDependencyModuleNames`, add PCGGraph include paths |
| `Public/EpicUnrealMCPBridge.h` | Add `#include` and `TSharedPtr<FEpicUnrealMCPPCGGraphCommands>` member |
| `Private/EpicUnrealMCPBridge.cpp` | Instantiate handler in constructor, add routing in `ExecuteCommand()` |
| `Private/Commands/EpicUnrealMCPEditorCommands.cpp` | Add `create_enum_asset` and `move_asset` commands |
| `Public/Commands/EpicUnrealMCPEditorCommands.h` | Add handler method declarations for enum/move |
| `Python/unreal_mcp_server_advanced.py` | Add 11 new `@mcp.tool()` definitions |

---

## Build.cs Changes

```csharp
// Add to PublicDependencyModuleNames:
"PCG"                // UPCGGraph, UPCGNode, UPCGSettings, UPCGComponent

// Add to PublicIncludePaths:
System.IO.Path.Combine(ModuleDirectory, "Public/Commands/PCGGraph")

// Add to PrivateIncludePaths:
System.IO.Path.Combine(ModuleDirectory, "Private/Commands/PCGGraph")
```

Note: `PCGEditor` module may also be needed if `UPCGGraphFactory` is used for asset creation. Test with just `"PCG"` first — direct `NewObject<UPCGGraph>` may suffice.

---

## 11 Commands — Schemas & Key API Calls

### 1. `create_pcg_graph`

**In:** `{ graph_name, path }` **Out:** `{ graph_name, graph_path }`

**UE5 API:**
```cpp
UPackage* Pkg = CreatePackage(*FullPath);
UPCGGraph* Graph = NewObject<UPCGGraph>(Pkg, FName(*Name), RF_Public | RF_Standalone);
FAssetRegistryModule::AssetCreated(Graph);
Pkg->MarkPackageDirty();
```

### 2. `read_pcg_graph`

**In:** `{ graph_path }` **Out:** `{ parameters[], nodes[], connections[] }`

**UE5 API:** Load via `LoadObject<UPCGGraph>`, iterate `GetNodes()`, inspect pins/edges, serialize via reflection.

### 3. `add_pcg_node`

**In:** `{ graph_path, node_type, position?, settings? }` **Out:** `{ node_id, node_type, input_pins[], output_pins[] }`

**UE5 API:**
```cpp
UPCGSettings* Settings = nullptr;
UPCGNode* Node = Graph->AddNodeOfType(SettingsClass, Settings);
Node->SetNodePosition(PosX, PosY);   // #if WITH_EDITOR
```

**Node type → Settings class mapping** (build a `TMap<FString, UClass*>`):

| node_type | Settings Class | Key Properties |
|-----------|---------------|----------------|
| `CreatePointsGrid` | `UPCGCreatePointsGridSettings` | CellSize (FVector), GridExtents (FVector) |
| `SurfaceSampler` | `UPCGSurfaceSamplerSettings` | PointsPerSquaredMeter, PointExtents |
| `TransformPoints` | `UPCGTransformPointsSettings` | OffsetMin/Max, RotationMin/Max, ScaleMin/Max |
| `StaticMeshSpawner` | `UPCGStaticMeshSpawnerSettings` | MeshEntries (struct array — needs special handling) |
| `AttributeFilter` | `UPCGFilterByAttributeSettings` | FilterOperator, Attribute, Threshold |
| `CreateAttribute` | `UPCGCreateAttributeSettings` | OutputAttributeName, AttributeTypes, Value |
| `DensityNoise` | `UPCGSpatialNoiseSettings` | Mode, Iterations, Brightness, Contrast |
| `DensityFilter` | `UPCGDensityFilterSettings` | LowerBound, UpperBound, bInvertFilter |
| `Union` | `UPCGMergeSettings` | bMergeMetadata |
| `CopyPoints` | `UPCGCopyPointsSettings` | RotationBehavior, ScaleBehavior |
| `Difference` | `UPCGDifferenceSettings` | DensityFunction |
| `Subgraph` | `UPCGSubgraphSettings` | SubgraphAsset |
| `GetActorData` | `UPCGGetActorDataSettings` | ActorSelector, Mode |

### 4. `connect_pcg_nodes`

**In:** `{ graph_path, source_node_id, target_node_id, source_pin?, target_pin? }` (pins default to "Out"/"In")
**Out:** `{ source_node_id, target_node_id, connection_type }`

**UE5 API:**
```cpp
Graph->AddEdge(SourceNode, FName("Out"), TargetNode, FName("In"));
```

### 5. `set_pcg_node_property`

**In:** `{ graph_path, node_id, property_name, property_value }` **Out:** `{ node_id, property_name, property_type }`

**UE5 API:** Get `UPCGSettings*` via `Node->GetSettings()`, use `FProperty` reflection to find and set the property. Must handle float, int32, bool, FVector, FRotator, FName, FString, and enum types.

### 6. `delete_pcg_node`

**In:** `{ graph_path, node_id }` **Out:** `{ node_id, node_type, connections_removed }`

**UE5 API:** `Graph->RemoveNode(Node)`

### 7. `add_pcg_graph_parameter`

**In:** `{ graph_path, parameter_name, parameter_type, default_value?, description? }` **Out:** `{ parameter_name, parameter_type, parameter_count }`

**UE5 API:** Parameters use `FInstancedPropertyBag` (StructUtils module):
```cpp
TArray<FPropertyBagPropertyDesc> Descs;
Descs.Add(FPropertyBagPropertyDesc(FName("HabLength"), EPropertyBagPropertyType::Int32));
Graph->AddUserParameters(Descs);
Graph->SetGraphParameter<int32>(FName("HabLength"), 3);
```

Supported types: Bool, Int32, Int64, Float, Double, String, Name, Vector, Rotator, Transform, SoftObjectPath

### 8. `set_pcg_graph_parameter`

**In:** `{ graph_path, parameter_name, default_value }` **Out:** `{ parameter_name, old_value, new_value }`

**UE5 API:** `Graph->SetGraphParameter<T>(FName("ParamName"), Value)`

### 9. `assign_pcg_graph_to_component`

**In:** `{ blueprint_name, component_name, graph_path, generation_trigger? }` **Out:** `{ blueprint_name, component_name, graph_path }`

**UE5 API:** Find BP, find `UPCGComponent*` in SCS, call `PCGComponent->SetGraph(Graph)`.

### 10. `create_enum_asset`

**In:** `{ enum_name, path, values: [{name, description?}] }` **Out:** `{ enum_name, enum_path, values[] }`

**UE5 API:** `NewObject<UUserDefinedEnum>`, `SetEnums()` with `FName`/display name pairs.

### 11. `move_asset`

**In:** `{ source_path, destination_path }` **Out:** `{ source_path, destination_path }`

**UE5 API:** `UEditorAssetLibrary::RenameAsset(Source, Dest)` — handles move + rename + reference fixup.

---

## Bridge Routing (EpicUnrealMCPBridge.cpp)

Add to `ExecuteCommand()`:
```cpp
else if (CommandType == TEXT("create_pcg_graph") ||
         CommandType == TEXT("read_pcg_graph") ||
         CommandType == TEXT("add_pcg_node") ||
         CommandType == TEXT("connect_pcg_nodes") ||
         CommandType == TEXT("set_pcg_node_property") ||
         CommandType == TEXT("delete_pcg_node") ||
         CommandType == TEXT("add_pcg_graph_parameter") ||
         CommandType == TEXT("set_pcg_graph_parameter") ||
         CommandType == TEXT("assign_pcg_graph_to_component"))
{
    ResultJson = PCGGraphCommands->HandleCommand(CommandType, Params);
}
```

Add `create_enum_asset` and `move_asset` to the existing EditorCommands routing.

---

## Phased Build Order

### Phase 1: Foundation (implement first)
1. **Build.cs + Bridge registration** — add PCG module dep, create handler class, wire routing
2. **`create_pcg_graph`** — must exist before any node ops
3. **`read_pcg_graph`** — essential for debugging every subsequent step
4. **`add_pcg_node`** — core operation; test with CreatePointsGrid
5. **`connect_pcg_nodes`** — complete minimal viable graph

**Milestone:** Can create a graph, add nodes, connect them, and inspect the result.

### Phase 2: Configuration
6. **`set_pcg_node_property`** — configure nodes beyond defaults
7. **`add_pcg_graph_parameter`** — parameterized graphs
8. **`set_pcg_graph_parameter`** — set parameter defaults

**Milestone:** Can build fully parameterized PCG graphs (e.g., hab generator).

### Phase 3: Integration
9. **`assign_pcg_graph_to_component`** — links graphs to Blueprints
10. **`delete_pcg_node`** — iterative editing

**Milestone:** Full PCG-to-Blueprint pipeline works.

### Phase 4: Utilities
11. **`create_enum_asset`** — enum parameters (E_HabDoorSide)
12. **`move_asset`** — project organization

---

## Key Technical Risks

| Risk | Mitigation |
|------|------------|
| PCG settings class names may vary between UE versions | Build a self-discovering registry via `TObjectIterator<UClass>` for `UPCGSettings` subclasses at plugin startup |
| `StaticMeshSpawnerSettings` uses complex struct arrays for mesh entries | Start with simple single-mesh support; add `add_mesh_entry` sub-command later |
| `FInstancedPropertyBag` for graph params may need StructUtils module dep | Add `"StructUtils"` to Build.cs if compile errors occur |
| Property reflection for FVector/FRotator on PCG settings | Extend existing `CommonUtils` with struct property handling (check `FStructProperty` and call `ImportTextItem`) |
| Graph regeneration after modification | Call `Graph->ForceNotificationForEditor(EPCGChangeType::Structural)` after all modifications |

---

## Verification Plan

1. **Build test:** Compile plugin with PCG module dependency added
2. **Smoke test per command:** Use Python REPL to send each command over TCP and verify JSON responses
3. **Integration test:** Build a simple PCG graph (SurfaceSampler → TransformPoints → StaticMeshSpawner) via MCP commands, assign to a Blueprint, verify it generates geometry in-editor
4. **Hab Generator test:** Recreate the PCG_Hab_Main graph structure from `docs/PCG_Hab_BuildGuide.md` entirely via MCP commands
5. **Read-back test:** Use `read_pcg_graph` to verify created graphs match expected structure

---

## UE5 PCG Key Headers (for reference)

```cpp
#include "PCGGraph.h"           // UPCGGraph — AddNodeOfType, AddEdge, RemoveNode
#include "PCGNode.h"            // UPCGNode — GetSettings, GetInputPins/OutputPins, SetNodePosition
#include "PCGPin.h"             // UPCGPin — AddEdgeTo, Properties
#include "PCGEdge.h"            // UPCGEdge — InputPin, OutputPin
#include "PCGSettings.h"        // UPCGSettings base class
#include "PCGComponent.h"       // UPCGComponent — SetGraph, GenerateLocal
#include "PCGCommon.h"          // EPCGDataType, EPCGChangeType enums

// Node type settings (in Elements/ subdirectory)
#include "Elements/PCGSurfaceSampler.h"
#include "Elements/PCGTransformPoints.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "Elements/PCGDensityFilter.h"
#include "Elements/PCGSpatialNoise.h"
#include "Elements/PCGMergeElement.h"
#include "Elements/PCGFilterByAttribute.h"
#include "Elements/PCGCreateAttribute.h"
#include "Elements/PCGCopyPoints.h"

// Graph parameters
#include "Helpers/PCGGraphParameterExtension.h"
```
