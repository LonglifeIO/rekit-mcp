// PCG Graph Commands Handler

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FEpicUnrealMCPPCGGraphCommands
{
public:
    FEpicUnrealMCPPCGGraphCommands();
    ~FEpicUnrealMCPPCGGraphCommands();

    /**
     * Main command handler for PCG Graph operations
     * @param CommandType The type of command to execute
     * @param Params JSON parameters for the command
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Create a new PCG graph asset
    TSharedPtr<FJsonObject> HandleCreatePCGGraph(const TSharedPtr<FJsonObject>& Params);

    // Read/inspect an existing PCG graph
    TSharedPtr<FJsonObject> HandleReadPCGGraph(const TSharedPtr<FJsonObject>& Params);

    // Add a node to a PCG graph
    TSharedPtr<FJsonObject> HandleAddPCGNode(const TSharedPtr<FJsonObject>& Params);

    // Connect two nodes in a PCG graph
    TSharedPtr<FJsonObject> HandleConnectPCGNodes(const TSharedPtr<FJsonObject>& Params);

    // Set a property on a PCG node's settings
    TSharedPtr<FJsonObject> HandleSetPCGNodeProperty(const TSharedPtr<FJsonObject>& Params);

    // Delete a node from a PCG graph
    TSharedPtr<FJsonObject> HandleDeletePCGNode(const TSharedPtr<FJsonObject>& Params);

    // Add a user parameter to a PCG graph
    TSharedPtr<FJsonObject> HandleAddPCGGraphParameter(const TSharedPtr<FJsonObject>& Params);

    // Set a user parameter default value on a PCG graph
    TSharedPtr<FJsonObject> HandleSetPCGGraphParameter(const TSharedPtr<FJsonObject>& Params);

    // Assign a PCG graph to an actor's PCGComponent
    TSharedPtr<FJsonObject> HandleAssignPCGGraph(const TSharedPtr<FJsonObject>& Params);

    // Set mesh entries on a Static Mesh Spawner node
    TSharedPtr<FJsonObject> HandleSetPCGSpawnerEntries(const TSharedPtr<FJsonObject>& Params);
};
