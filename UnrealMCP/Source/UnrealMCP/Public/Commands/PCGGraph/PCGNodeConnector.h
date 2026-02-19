#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles connecting nodes in PCG graphs via pin edges.
 */
class UNREALMCP_API FPCGNodeConnector
{
public:
    /**
     * Connect two nodes in a PCG graph
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - from_node_id (string): FName of the source node
     *   - from_pin (string): Label of the output pin (default: "Out")
     *   - to_node_id (string): FName of the target node
     *   - to_pin (string): Label of the input pin (default: "In")
     * @return JSON with from_node_id, to_node_id
     */
    static TSharedPtr<FJsonObject> ConnectNodes(const TSharedPtr<FJsonObject>& Params);
};
