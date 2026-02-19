#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UPCGSettings;

/**
 * Manages adding and deleting nodes in PCG graphs.
 * Includes a node type registry for friendly name -> UPCGSettings subclass mapping.
 */
class UNREALMCP_API FPCGNodeManager
{
public:
    /**
     * Add a node to a PCG graph
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - node_type (string): Friendly name or exact class name of the settings
     *   - pos_x (int, optional): X position in graph editor (default: 0)
     *   - pos_y (int, optional): Y position in graph editor (default: 0)
     * @return JSON with node_id, settings_class, input_pins, output_pins
     */
    static TSharedPtr<FJsonObject> AddNode(const TSharedPtr<FJsonObject>& Params);

    /**
     * Delete a node from a PCG graph
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - node_id (string): FName of the node to delete
     * @return JSON with deleted_node_id
     */
    static TSharedPtr<FJsonObject> DeleteNode(const TSharedPtr<FJsonObject>& Params);

    /**
     * Find a node in a graph by its FName string (used by connector + property manager)
     */
    static UPCGNode* FindNodeByName(class UPCGGraph* Graph, const FString& NodeName);

private:
    /**
     * Resolve a friendly node type name to a UClass* for UPCGSettings subclass.
     * Falls back to FindObject<UClass> for exact class names.
     */
    static UClass* ResolveNodeType(const FString& NodeType);

    /**
     * Build the friendly name -> UClass* registry on first use.
     */
    static const TMap<FString, UClass*>& GetNodeTypeRegistry();
};
