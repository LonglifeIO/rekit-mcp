#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles setting properties on PCG node settings via UE5 property reflection.
 * Supports: bool, int32, int64, float, double, FVector, FRotator, FString, FName,
 *           FSoftObjectPath, and enum types. Falls back to ImportText for complex structs.
 */
class UNREALMCP_API FPCGNodePropertyManager
{
public:
    /**
     * Set a property on a PCG node's settings object
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - node_id (string): FName of the node
     *   - property_name (string): Name of the property on UPCGSettings
     *   - property_value (any): Value to set (JSON type depends on property)
     * @return JSON with node_id, property_name, property_type
     */
    static TSharedPtr<FJsonObject> SetNodeProperty(const TSharedPtr<FJsonObject>& Params);

private:
    /**
     * Set a property value on a UObject using reflection
     * @return true if the property was found and set
     */
    static bool SetPropertyValue(
        UObject* Object,
        const FString& PropertyName,
        const TSharedPtr<FJsonValue>& JsonValue,
        FString& OutPropertyType);

    /**
     * Set a property value using dot-notation path traversal for nested structs.
     * E.g., "InputSource1.AttributeName" traverses into FPCGAttributePropertyInputSelector.
     * @return true if the nested property was found and set
     */
    static bool SetPropertyValueByPath(
        UObject* Object,
        const FString& PropertyPath,
        const TSharedPtr<FJsonValue>& JsonValue,
        FString& OutPropertyType);

    /**
     * Set a leaf property value on a raw struct pointer using reflection.
     * Used by SetPropertyValueByPath for the final segment of a dot-notation path.
     * @return true if the property was found and set
     */
    static bool SetStructPropertyValue(
        UScriptStruct* Struct,
        void* StructPtr,
        const FString& PropertyName,
        const TSharedPtr<FJsonValue>& JsonValue,
        FString& OutPropertyType);

public:
    /**
     * Set mesh entries on a PCG Static Mesh Spawner node.
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - node_id (string): FName of the StaticMeshSpawner node
     *   - entries (array): Array of objects with:
     *       - mesh_path (string): Content path to a static mesh asset
     *       - weight (int, optional): Selection weight (default: 1)
     * @return JSON with node_id, entry_count
     */
    static TSharedPtr<FJsonObject> SetSpawnerEntries(const TSharedPtr<FJsonObject>& Params);
};
