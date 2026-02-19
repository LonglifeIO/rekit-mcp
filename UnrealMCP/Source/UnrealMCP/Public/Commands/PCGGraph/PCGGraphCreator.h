#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UPCGGraph;

/**
 * Handles PCG graph creation and reading/inspection.
 */
class UNREALMCP_API FPCGGraphCreator
{
public:
    /**
     * Create a new PCG graph asset
     * @param Params JSON parameters:
     *   - graph_name (string): Name for the new graph
     *   - path (string, optional): Content path (default: /Game/PCG)
     * @return JSON with graph_path
     */
    static TSharedPtr<FJsonObject> CreatePCGGraph(const TSharedPtr<FJsonObject>& Params);

    /**
     * Read/inspect an existing PCG graph
     * @param Params JSON parameters:
     *   - graph_path (string): Full content path to the graph
     * @return JSON with nodes, connections, parameters arrays
     */
    static TSharedPtr<FJsonObject> ReadPCGGraph(const TSharedPtr<FJsonObject>& Params);

    /**
     * Load a PCG graph by path (tries multiple resolution strategies)
     */
    static UPCGGraph* LoadPCGGraph(const FString& GraphPath);
};
