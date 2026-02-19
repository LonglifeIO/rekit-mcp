#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "StructUtils/PropertyBag.h"

/**
 * Manages PCG graph user parameters (add, set defaults) and graph assignment to PCGComponents.
 */
class UNREALMCP_API FPCGParameterManager
{
public:
    /**
     * Add a user parameter to a PCG graph
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - param_name (string): Name of the parameter
     *   - param_type (string): Type (Bool, Int32, Int64, Float, Double, String, Name, Vector, Rotator, Transform, SoftObjectPath)
     * @return JSON with param_name, param_type
     */
    static TSharedPtr<FJsonObject> AddGraphParameter(const TSharedPtr<FJsonObject>& Params);

    /**
     * Set a user parameter's default value on a PCG graph
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - param_name (string): Name of the parameter
     *   - default_value (any): Default value to set
     * @return JSON with param_name, new_value
     */
    static TSharedPtr<FJsonObject> SetGraphParameter(const TSharedPtr<FJsonObject>& Params);

    /**
     * Assign a PCG graph to an actor's PCGComponent (or Blueprint's PCGComponent)
     * @param Params JSON parameters:
     *   - graph_path (string): Content path to the PCG graph
     *   - actor_name (string, optional): Name of actor in the level
     *   - blueprint_name (string, optional): Name of Blueprint with PCGComponent
     * @return JSON with graph_path
     */
    static TSharedPtr<FJsonObject> AssignPCGGraph(const TSharedPtr<FJsonObject>& Params);

private:
    /**
     * Convert a param_type string to EPropertyBagPropertyType
     */
    static bool ResolveParamType(
        const FString& TypeString,
        EPropertyBagPropertyType& OutType,
        UScriptStruct*& OutValueStruct);
};
