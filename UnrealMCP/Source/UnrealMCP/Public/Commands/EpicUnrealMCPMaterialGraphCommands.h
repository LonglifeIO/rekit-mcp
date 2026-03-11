// Material Graph Commands Handler

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FEpicUnrealMCPMaterialGraphCommands
{
public:
    FEpicUnrealMCPMaterialGraphCommands();
    ~FEpicUnrealMCPMaterialGraphCommands();

    /**
     * Main command handler for Material Graph operations
     * @param CommandType The type of command to execute
     * @param Params JSON parameters for the command
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Create a new material asset
    TSharedPtr<FJsonObject> HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params);

    // Add a material expression node to a material graph
    TSharedPtr<FJsonObject> HandleAddMaterialExpression(const TSharedPtr<FJsonObject>& Params);

    // Set a parameter on a material expression
    TSharedPtr<FJsonObject> HandleSetMaterialExpressionParam(const TSharedPtr<FJsonObject>& Params);

    // Connect two material expressions
    TSharedPtr<FJsonObject> HandleConnectMaterialExpressions(const TSharedPtr<FJsonObject>& Params);

    // Connect a material expression to a material output property
    TSharedPtr<FJsonObject> HandleConnectMaterialToOutput(const TSharedPtr<FJsonObject>& Params);

    // Assign a material to a landscape actor
    TSharedPtr<FJsonObject> HandleSetLandscapeMaterial(const TSharedPtr<FJsonObject>& Params);

    // Recompile a material
    TSharedPtr<FJsonObject> HandleCompileMaterial(const TSharedPtr<FJsonObject>& Params);
};
