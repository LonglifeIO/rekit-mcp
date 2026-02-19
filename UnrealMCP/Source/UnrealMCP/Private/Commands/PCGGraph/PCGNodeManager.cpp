#include "Commands/PCGGraph/PCGNodeManager.h"
#include "Commands/PCGGraph/PCGGraphCreator.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGSettings.h"
#include "PCGComponent.h"

#if __has_include("Elements/PCGSurfaceSampler.h")
#include "Elements/PCGSurfaceSampler.h"
#endif
#if __has_include("Elements/PCGStaticMeshSpawner.h")
#include "Elements/PCGStaticMeshSpawner.h"
#endif
#if __has_include("Elements/PCGDensityFilter.h")
#include "Elements/PCGDensityFilter.h"
#endif
#if __has_include("Elements/PCGSpatialNoise.h")
#include "Elements/PCGSpatialNoise.h"
#endif
#if __has_include("Elements/PCGMergeElement.h")
#include "Elements/PCGMergeElement.h"
#endif
#if __has_include("Elements/PCGCopyPoints.h")
#include "Elements/PCGCopyPoints.h"
#endif

#include "UObject/UObjectIterator.h"

// Forward declares for settings classes that may vary by engine version
// We use the registry + fallback approach to handle this gracefully

const TMap<FString, UClass*>& FPCGNodeManager::GetNodeTypeRegistry()
{
    static TMap<FString, UClass*> Registry;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        // Helper lambda to safely register a class by name
        auto TryRegister = [&](const FString& FriendlyName, const FString& ClassName)
        {
            UClass* FoundClass = FindObject<UClass>(nullptr, *ClassName);
            if (!FoundClass)
            {
                // Try with /Script/PCG prefix
                FoundClass = FindObject<UClass>(nullptr, *(TEXT("/Script/PCG.") + ClassName));
            }
            if (FoundClass && FoundClass->IsChildOf(UPCGSettings::StaticClass()))
            {
                Registry.Add(FriendlyName, FoundClass);
                UE_LOG(LogTemp, Verbose, TEXT("PCG Registry: Registered '%s' -> %s"), *FriendlyName, *FoundClass->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Verbose, TEXT("PCG Registry: Could not find class '%s' for '%s'"), *ClassName, *FriendlyName);
            }
        };

        // Core PCG node types - using class names that should work across UE5 versions
        TryRegister(TEXT("SurfaceSampler"), TEXT("PCGSurfaceSamplerSettings"));
        TryRegister(TEXT("StaticMeshSpawner"), TEXT("PCGStaticMeshSpawnerSettings"));
        TryRegister(TEXT("DensityFilter"), TEXT("PCGDensityFilterSettings"));
        TryRegister(TEXT("DensityNoise"), TEXT("PCGSpatialNoiseSettings"));
        TryRegister(TEXT("Union"), TEXT("PCGMergeSettings"));
        TryRegister(TEXT("Merge"), TEXT("PCGMergeSettings"));
        TryRegister(TEXT("CopyPoints"), TEXT("PCGCopyPointsSettings"));
        TryRegister(TEXT("TransformPoints"), TEXT("PCGTransformPointsSettings"));
        TryRegister(TEXT("CreatePointsGrid"), TEXT("PCGCreatePointsGridSettings"));
        TryRegister(TEXT("PointsGrid"), TEXT("PCGCreatePointsGridSettings"));
        TryRegister(TEXT("AttributeFilter"), TEXT("PCGFilterByAttributeSettings"));
        TryRegister(TEXT("FilterByAttribute"), TEXT("PCGFilterByAttributeSettings"));
        TryRegister(TEXT("CreateAttribute"), TEXT("PCGCreateAttributeSettings"));
        TryRegister(TEXT("Difference"), TEXT("PCGDifferenceSettings"));
        TryRegister(TEXT("Subgraph"), TEXT("PCGSubgraphSettings"));
        TryRegister(TEXT("GetActorData"), TEXT("PCGGetActorDataSettings"));
        TryRegister(TEXT("BoundsModifier"), TEXT("PCGBoundsModifierSettings"));
        TryRegister(TEXT("DensityRemapSettings"), TEXT("PCGDensityRemapSettings"));
        TryRegister(TEXT("DensityRemap"), TEXT("PCGDensityRemapSettings"));
        TryRegister(TEXT("Projection"), TEXT("PCGProjectionSettings"));
        TryRegister(TEXT("Intersection"), TEXT("PCGIntersectionSettings"));
        TryRegister(TEXT("SplineSampler"), TEXT("PCGSplineSamplerSettings"));
        TryRegister(TEXT("VolumeSampler"), TEXT("PCGVolumeSamplerSettings"));
        TryRegister(TEXT("PointMatchAndSet"), TEXT("PCGPointMatchAndSetSettings"));
        TryRegister(TEXT("AttributeNoise"), TEXT("PCGAttributeNoiseSettings"));
        TryRegister(TEXT("AttributeOperation"), TEXT("PCGAttributeOperationSettings"));
        // UE5.7 alternative class names
        TryRegister(TEXT("AttributeFilter"), TEXT("PCGAttributeFilterSettings"));
        TryRegister(TEXT("CreateAttribute"), TEXT("PCGMetadataCreateAttributeSettings"));
        TryRegister(TEXT("AttributeOperation"), TEXT("PCGMetadataAttributeOperationSettings"));
        TryRegister(TEXT("Union"), TEXT("PCGUnionSettings"));
        TryRegister(TEXT("MetadataBreakVector"), TEXT("PCGMetadataBreakVectorSettings"));
        TryRegister(TEXT("BreakVector"), TEXT("PCGMetadataBreakVectorSettings"));
        TryRegister(TEXT("MetadataMakeVector"), TEXT("PCGMetadataMakeVectorSettings"));
        TryRegister(TEXT("MakeVector"), TEXT("PCGMetadataMakeVectorSettings"));

        UE_LOG(LogTemp, Display, TEXT("PCG Registry: Registered %d node types"), Registry.Num());

        // Self-discover any remaining UPCGSettings subclasses
        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;
            if (Class->IsChildOf(UPCGSettings::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
            {
                FString ClassName = Class->GetName();
                // Strip "Settings" suffix for a shorter friendly name if not already registered
                FString ShortName = ClassName;
                if (ShortName.EndsWith(TEXT("Settings")))
                {
                    ShortName = ShortName.LeftChop(8); // Remove "Settings"
                }
                // Strip PCG prefix if present
                if (ShortName.StartsWith(TEXT("PCG")))
                {
                    ShortName = ShortName.RightChop(3);
                }

                if (!Registry.Contains(ShortName))
                {
                    Registry.Add(ShortName, Class);
                }
                // Also register the full class name as a key
                if (!Registry.Contains(ClassName))
                {
                    Registry.Add(ClassName, Class);
                }
            }
        }

        UE_LOG(LogTemp, Display, TEXT("PCG Registry: Total entries after auto-discovery: %d"), Registry.Num());
    }

    return Registry;
}

UClass* FPCGNodeManager::ResolveNodeType(const FString& NodeType)
{
    const TMap<FString, UClass*>& Registry = GetNodeTypeRegistry();

    // Exact match in registry
    if (const UClass* const* Found = Registry.Find(NodeType))
    {
        return const_cast<UClass*>(*Found);
    }

    // Case-insensitive search
    for (const auto& Pair : Registry)
    {
        if (Pair.Key.Equals(NodeType, ESearchCase::IgnoreCase))
        {
            return Pair.Value;
        }
    }

    // Direct class lookup as fallback
    UClass* DirectClass = FindObject<UClass>(nullptr, *NodeType);
    if (!DirectClass)
    {
        DirectClass = FindObject<UClass>(nullptr, *(TEXT("/Script/PCG.") + NodeType));
    }
    if (DirectClass && DirectClass->IsChildOf(UPCGSettings::StaticClass()))
    {
        return DirectClass;
    }

    return nullptr;
}

UPCGNode* FPCGNodeManager::FindNodeByName(UPCGGraph* Graph, const FString& NodeName)
{
    if (!Graph) return nullptr;

    // Check regular nodes
    for (UPCGNode* Node : Graph->GetNodes())
    {
        if (Node && Node->GetFName().ToString() == NodeName)
        {
            return Node;
        }
    }

    // Check input/output nodes
    UPCGNode* InputNode = Graph->GetInputNode();
    if (InputNode && InputNode->GetFName().ToString() == NodeName)
    {
        return InputNode;
    }

    UPCGNode* OutputNode = Graph->GetOutputNode();
    if (OutputNode && OutputNode->GetFName().ToString() == NodeName)
    {
        return OutputNode;
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FPCGNodeManager::AddNode(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    FString NodeType;
    if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
    }

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    // Resolve the settings class
    UClass* SettingsClass = ResolveNodeType(NodeType);
    if (!SettingsClass)
    {
        // Build a helpful error with available types
        FString AvailableTypes;
        const TMap<FString, UClass*>& Registry = GetNodeTypeRegistry();
        int32 Count = 0;
        for (const auto& Pair : Registry)
        {
            if (Count > 0) AvailableTypes += TEXT(", ");
            AvailableTypes += Pair.Key;
            Count++;
            if (Count >= 30) { AvailableTypes += TEXT("..."); break; }
        }

        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown PCG node type '%s'. Available types: %s"), *NodeType, *AvailableTypes));
    }

    // Get optional position
    int32 PosX = 0, PosY = 0;
    if (Params->HasField(TEXT("pos_x")))
    {
        PosX = static_cast<int32>(Params->GetNumberField(TEXT("pos_x")));
    }
    if (Params->HasField(TEXT("pos_y")))
    {
        PosY = static_cast<int32>(Params->GetNumberField(TEXT("pos_y")));
    }

    // Create the node
    UPCGSettings* NewSettings = nullptr;
    UPCGNode* NewNode = Graph->AddNodeOfType(SettingsClass, NewSettings);

    if (!NewNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to create PCG node of type '%s'"), *NodeType));
    }

    // Set position
    NewNode->SetNodePosition(PosX, PosY);

    // Notify the graph editor
    Graph->NotifyGraphChanged(EPCGChangeType::Structural);
    Graph->GetPackage()->MarkPackageDirty();

    // Build response
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("node_id"), NewNode->GetFName().ToString());
    Result->SetStringField(TEXT("settings_class"), NewSettings ? NewSettings->GetClass()->GetName() : TEXT("None"));
    Result->SetNumberField(TEXT("pos_x"), PosX);
    Result->SetNumberField(TEXT("pos_y"), PosY);

    // Serialize pins
    TArray<TSharedPtr<FJsonValue>> InputPinsArray;
    for (UPCGPin* Pin : NewNode->GetInputPins())
    {
        if (!Pin) continue;
        TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
        PinObj->SetStringField(TEXT("pin_name"), Pin->Properties.Label.ToString());
        InputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
    }
    Result->SetArrayField(TEXT("input_pins"), InputPinsArray);

    TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
    for (UPCGPin* Pin : NewNode->GetOutputPins())
    {
        if (!Pin) continue;
        TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
        PinObj->SetStringField(TEXT("pin_name"), Pin->Properties.Label.ToString());
        OutputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
    }
    Result->SetArrayField(TEXT("output_pins"), OutputPinsArray);

    return Result;
}

TSharedPtr<FJsonObject> FPCGNodeManager::DeleteNode(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    FString NodeId;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    UPCGNode* Node = FindNodeByName(Graph, NodeId);
    if (!Node)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find node '%s' in PCG graph"), *NodeId));
    }

    // Don't allow deleting the built-in Input/Output nodes
    if (Node == Graph->GetInputNode() || Node == Graph->GetOutputNode())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Cannot delete the built-in Input or Output nodes"));
    }

    Graph->RemoveNode(Node);
    Graph->NotifyGraphChanged(EPCGChangeType::Structural);
    Graph->GetPackage()->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("deleted_node_id"), NodeId);

    return Result;
}
