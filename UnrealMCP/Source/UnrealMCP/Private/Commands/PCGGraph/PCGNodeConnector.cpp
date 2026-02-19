#include "Commands/PCGGraph/PCGNodeConnector.h"
#include "Commands/PCGGraph/PCGGraphCreator.h"
#include "Commands/PCGGraph/PCGNodeManager.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"

// Helper to find a pin by label on a node (case-insensitive)
static UPCGPin* FindPinByLabel(UPCGNode* Node, const FString& PinLabel, bool bIsInput)
{
    const TArray<UPCGPin*>& Pins = bIsInput ? Node->GetInputPins() : Node->GetOutputPins();

    // Exact match first
    for (UPCGPin* Pin : Pins)
    {
        if (Pin && Pin->Properties.Label.ToString() == PinLabel)
        {
            return Pin;
        }
    }

    // Case-insensitive match
    for (UPCGPin* Pin : Pins)
    {
        if (Pin && Pin->Properties.Label.ToString().Equals(PinLabel, ESearchCase::IgnoreCase))
        {
            return Pin;
        }
    }

    // If only one pin exists, use it as default
    if (Pins.Num() == 1 && Pins[0])
    {
        return Pins[0];
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FPCGNodeConnector::ConnectNodes(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    FString FromNodeId;
    if (!Params->TryGetStringField(TEXT("from_node_id"), FromNodeId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_node_id' parameter"));
    }

    FString ToNodeId;
    if (!Params->TryGetStringField(TEXT("to_node_id"), ToNodeId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_node_id' parameter"));
    }

    // Optional pin names with defaults
    FString FromPin = TEXT("Out");
    Params->TryGetStringField(TEXT("from_pin"), FromPin);

    FString ToPin = TEXT("In");
    Params->TryGetStringField(TEXT("to_pin"), ToPin);

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    // Find source and target nodes (FindNodeByName checks regular + input/output nodes)
    UPCGNode* FromNode = FPCGNodeManager::FindNodeByName(Graph, FromNodeId);
    if (!FromNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find source node '%s' in PCG graph"), *FromNodeId));
    }

    UPCGNode* ToNode = FPCGNodeManager::FindNodeByName(Graph, ToNodeId);
    if (!ToNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find target node '%s' in PCG graph"), *ToNodeId));
    }

    // Find the output pin on source node
    UPCGPin* OutputPin = FindPinByLabel(FromNode, FromPin, false);
    if (!OutputPin)
    {
        // Build available pins for error message
        FString AvailablePins;
        for (UPCGPin* Pin : FromNode->GetOutputPins())
        {
            if (Pin)
            {
                if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
                AvailablePins += Pin->Properties.Label.ToString();
            }
        }
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find output pin '%s' on node '%s'. Available: %s"),
                *FromPin, *FromNodeId, *AvailablePins));
    }

    // Find the input pin on target node
    UPCGPin* InputPin = FindPinByLabel(ToNode, ToPin, true);
    if (!InputPin)
    {
        FString AvailablePins;
        for (UPCGPin* Pin : ToNode->GetInputPins())
        {
            if (Pin)
            {
                if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
                AvailablePins += Pin->Properties.Label.ToString();
            }
        }
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find input pin '%s' on node '%s'. Available: %s"),
                *ToPin, *ToNodeId, *AvailablePins));
    }

    // Create the edge — try pin-based first (UE5.7), fall back to graph-based
    bool bSuccess = OutputPin->AddEdgeTo(InputPin);
    if (!bSuccess)
    {
        // Fallback: Graph->AddEdge returns UPCGNode* (the To node for chaining), not bool
        bSuccess = (Graph->AddEdge(FromNode, OutputPin->Properties.Label, ToNode, InputPin->Properties.Label) != nullptr);
    }
    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to create edge from '%s.%s' to '%s.%s'"),
                *FromNodeId, *FromPin, *ToNodeId, *ToPin));
    }

    Graph->NotifyGraphChanged(EPCGChangeType::Structural);
    Graph->GetPackage()->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("from_node_id"), FromNodeId);
    Result->SetStringField(TEXT("from_pin"), OutputPin->Properties.Label.ToString());
    Result->SetStringField(TEXT("to_node_id"), ToNodeId);
    Result->SetStringField(TEXT("to_pin"), InputPin->Properties.Label.ToString());

    return Result;
}
