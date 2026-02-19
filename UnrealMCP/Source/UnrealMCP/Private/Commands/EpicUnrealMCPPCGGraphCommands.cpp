#include "Commands/EpicUnrealMCPPCGGraphCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Commands/PCGGraph/PCGGraphCreator.h"
#include "Commands/PCGGraph/PCGNodeManager.h"
#include "Commands/PCGGraph/PCGNodeConnector.h"
#include "Commands/PCGGraph/PCGNodePropertyManager.h"
#include "Commands/PCGGraph/PCGParameterManager.h"

FEpicUnrealMCPPCGGraphCommands::FEpicUnrealMCPPCGGraphCommands()
{
}

FEpicUnrealMCPPCGGraphCommands::~FEpicUnrealMCPPCGGraphCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_pcg_graph"))
    {
        return HandleCreatePCGGraph(Params);
    }
    else if (CommandType == TEXT("read_pcg_graph"))
    {
        return HandleReadPCGGraph(Params);
    }
    else if (CommandType == TEXT("add_pcg_node"))
    {
        return HandleAddPCGNode(Params);
    }
    else if (CommandType == TEXT("connect_pcg_nodes"))
    {
        return HandleConnectPCGNodes(Params);
    }
    else if (CommandType == TEXT("set_pcg_node_property"))
    {
        return HandleSetPCGNodeProperty(Params);
    }
    else if (CommandType == TEXT("delete_pcg_node"))
    {
        return HandleDeletePCGNode(Params);
    }
    else if (CommandType == TEXT("add_pcg_graph_parameter"))
    {
        return HandleAddPCGGraphParameter(Params);
    }
    else if (CommandType == TEXT("set_pcg_graph_parameter"))
    {
        return HandleSetPCGGraphParameter(Params);
    }
    else if (CommandType == TEXT("assign_pcg_graph"))
    {
        return HandleAssignPCGGraph(Params);
    }
    else if (CommandType == TEXT("set_pcg_spawner_entries"))
    {
        return HandleSetPCGSpawnerEntries(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown PCG graph command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleCreatePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphName;
    if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleCreatePCGGraph: Creating PCG graph '%s'"), *GraphName);

    return FPCGGraphCreator::CreatePCGGraph(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleReadPCGGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleReadPCGGraph: Reading PCG graph '%s'"), *GraphPath);

    return FPCGGraphCreator::ReadPCGGraph(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleAddPCGNode(const TSharedPtr<FJsonObject>& Params)
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

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleAddPCGNode: Adding '%s' node to PCG graph '%s'"), *NodeType, *GraphPath);

    return FPCGNodeManager::AddNode(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleConnectPCGNodes(const TSharedPtr<FJsonObject>& Params)
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

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleConnectPCGNodes: Connecting '%s' to '%s' in PCG graph '%s'"),
        *FromNodeId, *ToNodeId, *GraphPath);

    return FPCGNodeConnector::ConnectNodes(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleSetPCGNodeProperty(const TSharedPtr<FJsonObject>& Params)
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

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleSetPCGNodeProperty: Setting '%s' on node '%s' in PCG graph '%s'"),
        *PropertyName, *NodeId, *GraphPath);

    return FPCGNodePropertyManager::SetNodeProperty(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleDeletePCGNode(const TSharedPtr<FJsonObject>& Params)
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

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleDeletePCGNode: Deleting node '%s' from PCG graph '%s'"),
        *NodeId, *GraphPath);

    return FPCGNodeManager::DeleteNode(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleAddPCGGraphParameter(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    FString ParamType;
    if (!Params->TryGetStringField(TEXT("param_type"), ParamType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_type' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleAddPCGGraphParameter: Adding parameter '%s' (%s) to PCG graph '%s'"),
        *ParamName, *ParamType, *GraphPath);

    return FPCGParameterManager::AddGraphParameter(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleSetPCGGraphParameter(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleSetPCGGraphParameter: Setting parameter '%s' in PCG graph '%s'"),
        *ParamName, *GraphPath);

    return FPCGParameterManager::SetGraphParameter(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleAssignPCGGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleAssignPCGGraph: Assigning PCG graph '%s'"), *GraphPath);

    return FPCGParameterManager::AssignPCGGraph(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPPCGGraphCommands::HandleSetPCGSpawnerEntries(const TSharedPtr<FJsonObject>& Params)
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

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPPCGGraphCommands::HandleSetPCGSpawnerEntries: Setting entries on node '%s' in PCG graph '%s'"),
        *NodeId, *GraphPath);

    return FPCGNodePropertyManager::SetSpawnerEntries(Params);
}
