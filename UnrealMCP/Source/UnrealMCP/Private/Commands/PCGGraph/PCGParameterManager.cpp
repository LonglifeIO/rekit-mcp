#include "Commands/PCGGraph/PCGParameterManager.h"
#include "Commands/PCGGraph/PCGGraphCreator.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "PCGGraph.h"
#include "PCGComponent.h"
#include "StructUtils/PropertyBag.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EditorAssetLibrary.h"

bool FPCGParameterManager::ResolveParamType(
    const FString& TypeString,
    EPropertyBagPropertyType& OutType,
    UScriptStruct*& OutValueStruct)
{
    OutValueStruct = nullptr;

    if (TypeString.Equals(TEXT("Bool"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Bool;
        return true;
    }
    if (TypeString.Equals(TEXT("Int32"), ESearchCase::IgnoreCase) ||
        TypeString.Equals(TEXT("Int"), ESearchCase::IgnoreCase) ||
        TypeString.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Int32;
        return true;
    }
    if (TypeString.Equals(TEXT("Int64"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Int64;
        return true;
    }
    if (TypeString.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Float;
        return true;
    }
    if (TypeString.Equals(TEXT("Double"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Double;
        return true;
    }
    if (TypeString.Equals(TEXT("String"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::String;
        return true;
    }
    if (TypeString.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Name;
        return true;
    }
    if (TypeString.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Struct;
        OutValueStruct = TBaseStructure<FVector>::Get();
        return true;
    }
    if (TypeString.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Struct;
        OutValueStruct = TBaseStructure<FRotator>::Get();
        return true;
    }
    if (TypeString.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Struct;
        OutValueStruct = TBaseStructure<FTransform>::Get();
        return true;
    }
    if (TypeString.Equals(TEXT("SoftObjectPath"), ESearchCase::IgnoreCase))
    {
        OutType = EPropertyBagPropertyType::Struct;
        OutValueStruct = TBaseStructure<FSoftObjectPath>::Get();
        return true;
    }

    return false;
}

TSharedPtr<FJsonObject> FPCGParameterManager::AddGraphParameter(const TSharedPtr<FJsonObject>& Params)
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

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    EPropertyBagPropertyType BagType;
    UScriptStruct* ValueStruct = nullptr;
    if (!ResolveParamType(ParamType, BagType, ValueStruct))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown parameter type '%s'. Supported: Bool, Int32, Int64, Float, Double, String, Name, Vector, Rotator, Transform, SoftObjectPath"), *ParamType));
    }

    // Build the property descriptor
    TArray<FPropertyBagPropertyDesc> Descs;
    FPropertyBagPropertyDesc Desc;
    Desc.Name = FName(*ParamName);
    Desc.ValueType = BagType;
    if (ValueStruct)
    {
        Desc.ValueTypeObject = ValueStruct;
    }
    Descs.Add(Desc);

    // Add the parameter to the graph
    Graph->AddUserParameters(Descs);

    Graph->NotifyGraphChanged(EPCGChangeType::Settings);
    Graph->GetPackage()->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("param_name"), ParamName);
    Result->SetStringField(TEXT("param_type"), ParamType);

    return Result;
}

TSharedPtr<FJsonObject> FPCGParameterManager::SetGraphParameter(const TSharedPtr<FJsonObject>& Params)
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

    if (!Params->HasField(TEXT("default_value")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'default_value' parameter"));
    }

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    // Get the user parameters property bag (const pointer in UE5.7)
    const FInstancedPropertyBag* UserParamsConst = Graph->GetUserParametersStruct();
    if (!UserParamsConst)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Graph has no user parameters struct."));
    }
    const UPropertyBag* BagStruct = UserParamsConst->GetPropertyBagStruct();

    if (!BagStruct)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Graph has no user parameters defined. Use add_pcg_graph_parameter first."));
    }

    // Find the property descriptor for this parameter
    const FPropertyBagPropertyDesc* FoundDesc = nullptr;
    for (const FPropertyBagPropertyDesc& Desc : BagStruct->GetPropertyDescs())
    {
        if (Desc.Name.ToString() == ParamName)
        {
            FoundDesc = &Desc;
            break;
        }
    }

    if (!FoundDesc)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Parameter '%s' not found in graph's user parameters"), *ParamName));
    }

    // Get mutable access to the user parameters
    FInstancedPropertyBag* UserParams = Graph->GetMutableUserParametersStruct();
    if (!UserParams)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Could not get mutable user parameters from graph."));
    }

    // Set the value based on type
    TSharedPtr<FJsonValue> DefaultValue = Params->TryGetField(TEXT("default_value"));
    bool bSuccess = false;
    FString ValueStr;

    switch (FoundDesc->ValueType)
    {
    case EPropertyBagPropertyType::Bool:
    {
        bool bValue = false;
        if (DefaultValue->TryGetBool(bValue))
        {
            bSuccess = (UserParams->SetValueBool(FName(*ParamName), bValue) == EPropertyBagResult::Success);
            ValueStr = bValue ? TEXT("true") : TEXT("false");
        }
        break;
    }
    case EPropertyBagPropertyType::Int32:
    {
        double NumValue = 0;
        if (DefaultValue->TryGetNumber(NumValue))
        {
            bSuccess = (UserParams->SetValueInt32(FName(*ParamName), static_cast<int32>(NumValue)) == EPropertyBagResult::Success);
            ValueStr = FString::FromInt(static_cast<int32>(NumValue));
        }
        break;
    }
    case EPropertyBagPropertyType::Int64:
    {
        double NumValue = 0;
        if (DefaultValue->TryGetNumber(NumValue))
        {
            bSuccess = (UserParams->SetValueInt64(FName(*ParamName), static_cast<int64>(NumValue)) == EPropertyBagResult::Success);
            ValueStr = FString::Printf(TEXT("%lld"), static_cast<int64>(NumValue));
        }
        break;
    }
    case EPropertyBagPropertyType::Float:
    {
        double NumValue = 0;
        if (DefaultValue->TryGetNumber(NumValue))
        {
            bSuccess = (UserParams->SetValueFloat(FName(*ParamName), static_cast<float>(NumValue)) == EPropertyBagResult::Success);
            ValueStr = FString::SanitizeFloat(static_cast<float>(NumValue));
        }
        break;
    }
    case EPropertyBagPropertyType::Double:
    {
        double NumValue = 0;
        if (DefaultValue->TryGetNumber(NumValue))
        {
            bSuccess = (UserParams->SetValueDouble(FName(*ParamName), NumValue) == EPropertyBagResult::Success);
            ValueStr = FString::SanitizeFloat(NumValue);
        }
        break;
    }
    case EPropertyBagPropertyType::String:
    {
        FString StrValue;
        if (DefaultValue->TryGetString(StrValue))
        {
            bSuccess = (UserParams->SetValueString(FName(*ParamName), StrValue) == EPropertyBagResult::Success);
            ValueStr = StrValue;
        }
        break;
    }
    case EPropertyBagPropertyType::Name:
    {
        FString StrValue;
        if (DefaultValue->TryGetString(StrValue))
        {
            bSuccess = (UserParams->SetValueName(FName(*ParamName), FName(*StrValue)) == EPropertyBagResult::Success);
            ValueStr = StrValue;
        }
        break;
    }
    default:
    {
        // For struct types (Vector, Rotator, etc.), we need to use SetValueSerialize
        FString StrValue;
        if (DefaultValue->TryGetString(StrValue))
        {
            bSuccess = (UserParams->SetValueSerialize(FName(*ParamName), StrValue) == EPropertyBagResult::Success);
            ValueStr = StrValue;
        }
        break;
    }
    }

    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to set value for parameter '%s'"), *ParamName));
    }

    Graph->NotifyGraphChanged(EPCGChangeType::Settings);
    Graph->GetPackage()->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("param_name"), ParamName);
    Result->SetStringField(TEXT("new_value"), ValueStr);

    return Result;
}

TSharedPtr<FJsonObject> FPCGParameterManager::AssignPCGGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    // Try actor_name first
    FString ActorName;
    if (Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        // Find actor in the current world
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
        }

        AActor* FoundActor = nullptr;
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetActorLabel() == ActorName || It->GetName() == ActorName)
            {
                FoundActor = *It;
                break;
            }
        }

        if (!FoundActor)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Could not find actor '%s' in the level"), *ActorName));
        }

        UPCGComponent* PCGComp = FoundActor->FindComponentByClass<UPCGComponent>();
        if (!PCGComp)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Actor '%s' does not have a PCGComponent"), *ActorName));
        }

        PCGComp->SetGraph(Graph);

        TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
        Result->SetBoolField(TEXT("success"), true);
        Result->SetStringField(TEXT("graph_path"), GraphPath);
        Result->SetStringField(TEXT("assigned_to_actor"), ActorName);
        return Result;
    }

    // Try blueprint_name
    FString BlueprintName;
    if (Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
        if (!Blueprint)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Could not find Blueprint '%s'"), *BlueprintName));
        }

        // Find PCGComponent in the Blueprint's SimpleConstructionScript
        USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
        if (SCS)
        {
            for (USCS_Node* SCSNode : SCS->GetAllNodes())
            {
                if (SCSNode && SCSNode->ComponentTemplate)
                {
                    UPCGComponent* PCGComp = Cast<UPCGComponent>(SCSNode->ComponentTemplate);
                    if (PCGComp)
                    {
                        PCGComp->SetGraph(Graph);

                        TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
                        Result->SetBoolField(TEXT("success"), true);
                        Result->SetStringField(TEXT("graph_path"), GraphPath);
                        Result->SetStringField(TEXT("assigned_to_blueprint"), BlueprintName);
                        return Result;
                    }
                }
            }
        }

        // Also check CDO
        UObject* CDO = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
        if (CDO)
        {
            AActor* ActorCDO = Cast<AActor>(CDO);
            if (ActorCDO)
            {
                UPCGComponent* PCGComp = ActorCDO->FindComponentByClass<UPCGComponent>();
                if (PCGComp)
                {
                    PCGComp->SetGraph(Graph);

                    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
                    Result->SetBoolField(TEXT("success"), true);
                    Result->SetStringField(TEXT("graph_path"), GraphPath);
                    Result->SetStringField(TEXT("assigned_to_blueprint"), BlueprintName);
                    return Result;
                }
            }
        }

        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Blueprint '%s' does not have a PCGComponent"), *BlueprintName));
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
        TEXT("Must provide either 'actor_name' or 'blueprint_name' parameter"));
}
