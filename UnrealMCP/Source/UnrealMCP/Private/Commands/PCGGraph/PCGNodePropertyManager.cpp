#include "Commands/PCGGraph/PCGNodePropertyManager.h"
#include "Commands/PCGGraph/PCGGraphCreator.h"
#include "Commands/PCGGraph/PCGNodeManager.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGSettings.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPath.h"

#if __has_include("Elements/PCGStaticMeshSpawner.h")
#include "Elements/PCGStaticMeshSpawner.h"
#define HAS_PCG_STATIC_MESH_SPAWNER 1
#else
#define HAS_PCG_STATIC_MESH_SPAWNER 0
#endif

bool FPCGNodePropertyManager::SetPropertyValue(
    UObject* Object,
    const FString& PropertyName,
    const TSharedPtr<FJsonValue>& JsonValue,
    FString& OutPropertyType)
{
    if (!Object || !JsonValue.IsValid())
    {
        return false;
    }

    FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        // Try case-insensitive search
        for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
        {
            if (It->GetName().Equals(PropertyName, ESearchCase::IgnoreCase))
            {
                Property = *It;
                break;
            }
        }
    }

    if (!Property)
    {
        return false;
    }

    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);

    // Bool
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool bValue = false;
        if (JsonValue->TryGetBool(bValue))
        {
            BoolProp->SetPropertyValue(ValuePtr, bValue);
            OutPropertyType = TEXT("Bool");
            return true;
        }
    }

    // Int32
    if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        double NumValue = 0;
        if (JsonValue->TryGetNumber(NumValue))
        {
            IntProp->SetPropertyValue(ValuePtr, static_cast<int32>(NumValue));
            OutPropertyType = TEXT("Int32");
            return true;
        }
    }

    // Int64
    if (FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
    {
        double NumValue = 0;
        if (JsonValue->TryGetNumber(NumValue))
        {
            Int64Prop->SetPropertyValue(ValuePtr, static_cast<int64>(NumValue));
            OutPropertyType = TEXT("Int64");
            return true;
        }
    }

    // Float
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        double NumValue = 0;
        if (JsonValue->TryGetNumber(NumValue))
        {
            FloatProp->SetPropertyValue(ValuePtr, static_cast<float>(NumValue));
            OutPropertyType = TEXT("Float");
            return true;
        }
    }

    // Double
    if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        double NumValue = 0;
        if (JsonValue->TryGetNumber(NumValue))
        {
            DoubleProp->SetPropertyValue(ValuePtr, NumValue);
            OutPropertyType = TEXT("Double");
            return true;
        }
    }

    // FString
    if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        FString StrValue;
        if (JsonValue->TryGetString(StrValue))
        {
            StrProp->SetPropertyValue(ValuePtr, StrValue);
            OutPropertyType = TEXT("String");
            return true;
        }
    }

    // FName
    if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        FString StrValue;
        if (JsonValue->TryGetString(StrValue))
        {
            NameProp->SetPropertyValue(ValuePtr, FName(*StrValue));
            OutPropertyType = TEXT("Name");
            return true;
        }
    }

    // Enum (byte property with enum)
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (ByteProp->Enum)
        {
            FString StrValue;
            if (JsonValue->TryGetString(StrValue))
            {
                int64 EnumValue = ByteProp->Enum->GetValueByNameString(StrValue);
                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(EnumValue));
                    OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *ByteProp->Enum->GetName());
                    return true;
                }
            }
            double NumValue = 0;
            if (JsonValue->TryGetNumber(NumValue))
            {
                ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(NumValue));
                OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *ByteProp->Enum->GetName());
                return true;
            }
        }
        else
        {
            double NumValue = 0;
            if (JsonValue->TryGetNumber(NumValue))
            {
                ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(NumValue));
                OutPropertyType = TEXT("Byte");
                return true;
            }
        }
    }

    // FEnumProperty (UE5 enum style)
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProp->GetEnum();
        if (Enum)
        {
            FString StrValue;
            if (JsonValue->TryGetString(StrValue))
            {
                int64 EnumValue = Enum->GetValueByNameString(StrValue);
                if (EnumValue != INDEX_NONE)
                {
                    FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
                    if (UnderlyingProp)
                    {
                        UnderlyingProp->SetIntPropertyValue(ValuePtr, EnumValue);
                        OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *Enum->GetName());
                        return true;
                    }
                }
            }
            double NumValue = 0;
            if (JsonValue->TryGetNumber(NumValue))
            {
                FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
                if (UnderlyingProp)
                {
                    UnderlyingProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(NumValue));
                    OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *Enum->GetName());
                    return true;
                }
            }
        }
    }

    // FVector (from array [x, y, z])
    if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        UScriptStruct* Struct = StructProp->Struct;

        if (Struct == TBaseStructure<FVector>::Get())
        {
            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
            if (JsonValue->TryGetArray(ArrayValue) && ArrayValue->Num() >= 3)
            {
                FVector& Vec = *reinterpret_cast<FVector*>(ValuePtr);
                Vec.X = (*ArrayValue)[0]->AsNumber();
                Vec.Y = (*ArrayValue)[1]->AsNumber();
                Vec.Z = (*ArrayValue)[2]->AsNumber();
                OutPropertyType = TEXT("Vector");
                return true;
            }
        }

        if (Struct == TBaseStructure<FRotator>::Get())
        {
            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
            if (JsonValue->TryGetArray(ArrayValue) && ArrayValue->Num() >= 3)
            {
                FRotator& Rot = *reinterpret_cast<FRotator*>(ValuePtr);
                Rot.Pitch = (*ArrayValue)[0]->AsNumber();
                Rot.Yaw = (*ArrayValue)[1]->AsNumber();
                Rot.Roll = (*ArrayValue)[2]->AsNumber();
                OutPropertyType = TEXT("Rotator");
                return true;
            }
        }

        if (Struct == TBaseStructure<FTransform>::Get())
        {
            const TSharedPtr<FJsonObject>* ObjValue;
            if (JsonValue->TryGetObject(ObjValue))
            {
                FTransform& Trans = *reinterpret_cast<FTransform*>(ValuePtr);

                if ((*ObjValue)->HasField(TEXT("location")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& Loc = (*ObjValue)->GetArrayField(TEXT("location"));
                    if (Loc.Num() >= 3)
                    {
                        Trans.SetLocation(FVector(Loc[0]->AsNumber(), Loc[1]->AsNumber(), Loc[2]->AsNumber()));
                    }
                }
                if ((*ObjValue)->HasField(TEXT("rotation")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& Rot = (*ObjValue)->GetArrayField(TEXT("rotation"));
                    if (Rot.Num() >= 3)
                    {
                        Trans.SetRotation(FRotator(Rot[0]->AsNumber(), Rot[1]->AsNumber(), Rot[2]->AsNumber()).Quaternion());
                    }
                }
                if ((*ObjValue)->HasField(TEXT("scale")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& Scl = (*ObjValue)->GetArrayField(TEXT("scale"));
                    if (Scl.Num() >= 3)
                    {
                        Trans.SetScale3D(FVector(Scl[0]->AsNumber(), Scl[1]->AsNumber(), Scl[2]->AsNumber()));
                    }
                }

                OutPropertyType = TEXT("Transform");
                return true;
            }
        }

        // FSoftObjectPath
        if (Struct == TBaseStructure<FSoftObjectPath>::Get())
        {
            FString StrValue;
            if (JsonValue->TryGetString(StrValue))
            {
                FSoftObjectPath& SoftPath = *reinterpret_cast<FSoftObjectPath*>(ValuePtr);
                SoftPath.SetPath(StrValue);
                OutPropertyType = TEXT("SoftObjectPath");
                return true;
            }
        }

        // Generic struct fallback: try ImportText_Direct
        FString StrValue;
        if (JsonValue->TryGetString(StrValue))
        {
            const TCHAR* Result = Property->ImportText_Direct(*StrValue, ValuePtr, Object, PPF_None);
            if (Result != nullptr)
            {
                OutPropertyType = FString::Printf(TEXT("Struct(%s)"), *Struct->GetName());
                return true;
            }
        }
    }

    // SoftObjectProperty
    if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
    {
        FString StrValue;
        if (JsonValue->TryGetString(StrValue))
        {
            FSoftObjectPtr& SoftPtr = *reinterpret_cast<FSoftObjectPtr*>(ValuePtr);
            SoftPtr = FSoftObjectPath(StrValue);
            OutPropertyType = TEXT("SoftObjectReference");
            return true;
        }
    }

    // Last resort: ImportText_Direct for any property type
    FString StrValue;
    if (JsonValue->TryGetString(StrValue))
    {
        const TCHAR* Result = Property->ImportText_Direct(*StrValue, ValuePtr, Object, PPF_None);
        if (Result != nullptr)
        {
            OutPropertyType = Property->GetCPPType();
            return true;
        }
    }

    return false;
}

static TSharedPtr<FJsonValue> ReadPropertyToJson(FProperty* Property, const void* ValuePtr, FString& OutPropertyType)
{
    if (!Property || !ValuePtr)
    {
        return nullptr;
    }

    // Bool
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        OutPropertyType = TEXT("Bool");
        return MakeShareable(new FJsonValueBoolean(BoolProp->GetPropertyValue(ValuePtr)));
    }

    // Int32
    if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        OutPropertyType = TEXT("Int32");
        return MakeShareable(new FJsonValueNumber(IntProp->GetPropertyValue(ValuePtr)));
    }

    // Int64
    if (FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
    {
        OutPropertyType = TEXT("Int64");
        return MakeShareable(new FJsonValueNumber(static_cast<double>(Int64Prop->GetPropertyValue(ValuePtr))));
    }

    // Float
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        OutPropertyType = TEXT("Float");
        return MakeShareable(new FJsonValueNumber(FloatProp->GetPropertyValue(ValuePtr)));
    }

    // Double
    if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        OutPropertyType = TEXT("Double");
        return MakeShareable(new FJsonValueNumber(DoubleProp->GetPropertyValue(ValuePtr)));
    }

    // FString
    if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        OutPropertyType = TEXT("String");
        return MakeShareable(new FJsonValueString(StrProp->GetPropertyValue(ValuePtr)));
    }

    // FName
    if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        OutPropertyType = TEXT("Name");
        return MakeShareable(new FJsonValueString(NameProp->GetPropertyValue(ValuePtr).ToString()));
    }

    // Byte/Enum
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (ByteProp->Enum)
        {
            uint8 Val = ByteProp->GetPropertyValue(ValuePtr);
            FString EnumName = ByteProp->Enum->GetNameStringByValue(Val);
            OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *ByteProp->Enum->GetName());
            return MakeShareable(new FJsonValueString(EnumName));
        }
        OutPropertyType = TEXT("Byte");
        return MakeShareable(new FJsonValueNumber(ByteProp->GetPropertyValue(ValuePtr)));
    }

    // FEnumProperty (UE5 style)
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProp->GetEnum();
        if (Enum)
        {
            FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
            if (UnderlyingProp)
            {
                int64 Val = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
                FString EnumName = Enum->GetNameStringByValue(Val);
                OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *Enum->GetName());
                return MakeShareable(new FJsonValueString(EnumName));
            }
        }
    }

    // FVector
    if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        UScriptStruct* Struct = StructProp->Struct;

        if (Struct == TBaseStructure<FVector>::Get())
        {
            const FVector& Vec = *reinterpret_cast<const FVector*>(ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShareable(new FJsonValueNumber(Vec.X)));
            Arr.Add(MakeShareable(new FJsonValueNumber(Vec.Y)));
            Arr.Add(MakeShareable(new FJsonValueNumber(Vec.Z)));
            OutPropertyType = TEXT("Vector");
            return MakeShareable(new FJsonValueArray(Arr));
        }

        if (Struct == TBaseStructure<FRotator>::Get())
        {
            const FRotator& Rot = *reinterpret_cast<const FRotator*>(ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShareable(new FJsonValueNumber(Rot.Pitch)));
            Arr.Add(MakeShareable(new FJsonValueNumber(Rot.Yaw)));
            Arr.Add(MakeShareable(new FJsonValueNumber(Rot.Roll)));
            OutPropertyType = TEXT("Rotator");
            return MakeShareable(new FJsonValueArray(Arr));
        }

        if (Struct == TBaseStructure<FSoftObjectPath>::Get())
        {
            const FSoftObjectPath& SoftPath = *reinterpret_cast<const FSoftObjectPath*>(ValuePtr);
            OutPropertyType = TEXT("SoftObjectPath");
            return MakeShareable(new FJsonValueString(SoftPath.ToString()));
        }

        // Generic struct: export via ExportText
        FString ExportedText;
        StructProp->ExportTextItem_Direct(ExportedText, ValuePtr, nullptr, nullptr, PPF_None);
        OutPropertyType = FString::Printf(TEXT("Struct(%s)"), *Struct->GetName());
        return MakeShareable(new FJsonValueString(ExportedText));
    }

    // SoftObjectProperty
    if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
    {
        const FSoftObjectPtr& SoftPtr = *reinterpret_cast<const FSoftObjectPtr*>(ValuePtr);
        OutPropertyType = TEXT("SoftObjectReference");
        return MakeShareable(new FJsonValueString(SoftPtr.ToSoftObjectPath().ToString()));
    }

    // Fallback: ExportText
    FString ExportedText;
    Property->ExportTextItem_Direct(ExportedText, ValuePtr, nullptr, nullptr, PPF_None);
    OutPropertyType = Property->GetCPPType();
    return MakeShareable(new FJsonValueString(ExportedText));
}

TSharedPtr<FJsonObject> FPCGNodePropertyManager::GetNodeProperty(const TSharedPtr<FJsonObject>& Params)
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

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    UPCGNode* Node = FPCGNodeManager::FindNodeByName(Graph, NodeId);
    if (!Node)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find node '%s' in PCG graph"), *NodeId));
    }

    UPCGSettings* Settings = Node->GetSettings();
    if (!Settings)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Node '%s' has no settings object"), *NodeId));
    }

    // Navigate dot-notation paths (e.g. "InputSource1.AttributeName")
    UObject* TargetObject = Settings;
    void* TargetPtr = nullptr;
    FProperty* TargetProperty = nullptr;
    FString RemainingPath = PropertyName;

    while (RemainingPath.Contains(TEXT(".")))
    {
        FString Head, Tail;
        RemainingPath.Split(TEXT("."), &Head, &Tail);

        FProperty* HeadProp = TargetObject->GetClass()->FindPropertyByName(FName(*Head));
        if (!HeadProp)
        {
            // Case-insensitive fallback
            for (TFieldIterator<FProperty> It(TargetObject->GetClass()); It; ++It)
            {
                if (It->GetName().Equals(Head, ESearchCase::IgnoreCase))
                {
                    HeadProp = *It;
                    break;
                }
            }
        }

        if (!HeadProp)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Could not find property '%s' on node '%s'"), *Head, *NodeId));
        }

        FStructProperty* StructProp = CastField<FStructProperty>(HeadProp);
        if (!StructProp)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Property '%s' is not a struct, cannot traverse into it"), *Head));
        }

        TargetPtr = StructProp->ContainerPtrToValuePtr<void>(TargetObject);
        // For deeper traversal, we need to search within the struct
        FProperty* LeafProp = StructProp->Struct->FindPropertyByName(FName(*Tail));
        if (!LeafProp)
        {
            for (TFieldIterator<FProperty> It(StructProp->Struct); It; ++It)
            {
                if (It->GetName().Equals(Tail, ESearchCase::IgnoreCase))
                {
                    LeafProp = *It;
                    break;
                }
            }
        }

        if (LeafProp && !Tail.Contains(TEXT(".")))
        {
            TargetProperty = LeafProp;
            TargetPtr = LeafProp->ContainerPtrToValuePtr<void>(TargetPtr);
            RemainingPath = TEXT("");
            break;
        }

        // If tail still has dots, we need to continue — but we're in struct territory now
        // For simplicity, fall through to ExportText on the whole struct
        break;
    }

    // Simple (non-dotted) property lookup
    if (!TargetProperty && !RemainingPath.IsEmpty())
    {
        TargetProperty = Settings->GetClass()->FindPropertyByName(FName(*RemainingPath));
        if (!TargetProperty)
        {
            for (TFieldIterator<FProperty> It(Settings->GetClass()); It; ++It)
            {
                if (It->GetName().Equals(RemainingPath, ESearchCase::IgnoreCase))
                {
                    TargetProperty = *It;
                    break;
                }
            }
        }

        if (!TargetProperty)
        {
            // Build available properties list
            FString AvailableProperties;
            int32 Count = 0;
            for (TFieldIterator<FProperty> It(Settings->GetClass()); It; ++It)
            {
                if (It->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
                {
                    if (Count > 0) AvailableProperties += TEXT(", ");
                    AvailableProperties += It->GetName();
                    Count++;
                    if (Count >= 30) { AvailableProperties += TEXT("..."); break; }
                }
            }

            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Could not find property '%s' on node '%s' (class: %s). Available: %s"),
                    *PropertyName, *NodeId, *Settings->GetClass()->GetName(), *AvailableProperties));
        }

        TargetPtr = TargetProperty->ContainerPtrToValuePtr<void>(Settings);
    }

    if (!TargetProperty || !TargetPtr)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not resolve property '%s' on node '%s'"), *PropertyName, *NodeId));
    }

    FString PropertyType;
    TSharedPtr<FJsonValue> JsonValue = ReadPropertyToJson(TargetProperty, TargetPtr, PropertyType);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("node_id"), NodeId);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetStringField(TEXT("property_type"), PropertyType);
    if (JsonValue.IsValid())
    {
        Result->SetField(TEXT("property_value"), JsonValue);
    }

    return Result;
}

TSharedPtr<FJsonObject> FPCGNodePropertyManager::SetNodeProperty(const TSharedPtr<FJsonObject>& Params)
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

    if (!Params->HasField(TEXT("property_value")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    TSharedPtr<FJsonValue> PropertyValue = Params->TryGetField(TEXT("property_value"));

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    UPCGNode* Node = FPCGNodeManager::FindNodeByName(Graph, NodeId);
    if (!Node)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find node '%s' in PCG graph"), *NodeId));
    }

    UPCGSettings* Settings = Node->GetSettings();
    if (!Settings)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Node '%s' has no settings object"), *NodeId));
    }

    // Try to set the property via reflection — use dot-notation path traversal if needed
    FString PropertyType;
    bool bSuccess = PropertyName.Contains(TEXT("."))
        ? SetPropertyValueByPath(Settings, PropertyName, PropertyValue, PropertyType)
        : SetPropertyValue(Settings, PropertyName, PropertyValue, PropertyType);

    if (!bSuccess)
    {
        // Build available properties list for the error message
        FString AvailableProperties;
        int32 Count = 0;
        for (TFieldIterator<FProperty> It(Settings->GetClass()); It; ++It)
        {
            FProperty* Prop = *It;
            if (Prop->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
            {
                if (Count > 0) AvailableProperties += TEXT(", ");
                AvailableProperties += Prop->GetName();
                Count++;
                if (Count >= 30) { AvailableProperties += TEXT("..."); break; }
            }
        }

        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not set property '%s' on node '%s' (class: %s). Available editable properties: %s"),
                *PropertyName, *NodeId, *Settings->GetClass()->GetName(), *AvailableProperties));
    }

    // Mark as modified
    Settings->MarkPackageDirty();
    Graph->ForceNotificationForEditor(EPCGChangeType::Structural);
    Graph->GetPackage()->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("node_id"), NodeId);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetStringField(TEXT("property_type"), PropertyType);

    return Result;
}

bool FPCGNodePropertyManager::SetStructPropertyValue(
    UScriptStruct* Struct,
    void* StructPtr,
    const FString& PropertyName,
    const TSharedPtr<FJsonValue>& JsonValue,
    FString& OutPropertyType)
{
    if (!Struct || !StructPtr || !JsonValue.IsValid())
    {
        return false;
    }

    // Find the property on the struct
    FProperty* Property = Struct->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        // Case-insensitive fallback
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            if (It->GetName().Equals(PropertyName, ESearchCase::IgnoreCase))
            {
                Property = *It;
                break;
            }
        }
    }

    if (!Property)
    {
        return false;
    }

    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(StructPtr);

    // FName (most common for AttributeName fields)
    if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        FString StrValue;
        if (JsonValue->TryGetString(StrValue))
        {
            NameProp->SetPropertyValue(ValuePtr, FName(*StrValue));
            OutPropertyType = TEXT("Name");
            return true;
        }
    }

    // FString
    if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        FString StrValue;
        if (JsonValue->TryGetString(StrValue))
        {
            StrProp->SetPropertyValue(ValuePtr, StrValue);
            OutPropertyType = TEXT("String");
            return true;
        }
    }

    // Bool
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool bValue = false;
        if (JsonValue->TryGetBool(bValue))
        {
            BoolProp->SetPropertyValue(ValuePtr, bValue);
            OutPropertyType = TEXT("Bool");
            return true;
        }
    }

    // Int32
    if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        double NumValue = 0;
        if (JsonValue->TryGetNumber(NumValue))
        {
            IntProp->SetPropertyValue(ValuePtr, static_cast<int32>(NumValue));
            OutPropertyType = TEXT("Int32");
            return true;
        }
    }

    // Float
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        double NumValue = 0;
        if (JsonValue->TryGetNumber(NumValue))
        {
            FloatProp->SetPropertyValue(ValuePtr, static_cast<float>(NumValue));
            OutPropertyType = TEXT("Float");
            return true;
        }
    }

    // Double
    if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        double NumValue = 0;
        if (JsonValue->TryGetNumber(NumValue))
        {
            DoubleProp->SetPropertyValue(ValuePtr, NumValue);
            OutPropertyType = TEXT("Double");
            return true;
        }
    }

    // Byte/Enum
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (ByteProp->Enum)
        {
            FString StrValue;
            if (JsonValue->TryGetString(StrValue))
            {
                int64 EnumValue = ByteProp->Enum->GetValueByNameString(StrValue);
                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(EnumValue));
                    OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *ByteProp->Enum->GetName());
                    return true;
                }
            }
            double NumValue = 0;
            if (JsonValue->TryGetNumber(NumValue))
            {
                ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(NumValue));
                OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *ByteProp->Enum->GetName());
                return true;
            }
        }
        else
        {
            double NumValue = 0;
            if (JsonValue->TryGetNumber(NumValue))
            {
                ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(NumValue));
                OutPropertyType = TEXT("Byte");
                return true;
            }
        }
    }

    // FEnumProperty (UE5 style)
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProp->GetEnum();
        if (Enum)
        {
            FString StrValue;
            if (JsonValue->TryGetString(StrValue))
            {
                int64 EnumValue = Enum->GetValueByNameString(StrValue);
                if (EnumValue != INDEX_NONE)
                {
                    FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
                    if (UnderlyingProp)
                    {
                        UnderlyingProp->SetIntPropertyValue(ValuePtr, EnumValue);
                        OutPropertyType = FString::Printf(TEXT("Enum(%s)"), *Enum->GetName());
                        return true;
                    }
                }
            }
        }
    }

    // Generic fallback: ImportText_Direct
    FString StrValue;
    if (JsonValue->TryGetString(StrValue))
    {
        const TCHAR* Result = Property->ImportText_Direct(*StrValue, ValuePtr, nullptr, PPF_None);
        if (Result != nullptr)
        {
            OutPropertyType = Property->GetCPPType();
            return true;
        }
    }

    return false;
}

bool FPCGNodePropertyManager::SetPropertyValueByPath(
    UObject* Object,
    const FString& PropertyPath,
    const TSharedPtr<FJsonValue>& JsonValue,
    FString& OutPropertyType)
{
    if (!Object || !JsonValue.IsValid())
    {
        return false;
    }

    // Split on first dot: "InputSource1.AttributeName" -> head="InputSource1", tail="AttributeName"
    FString Head, Tail;
    if (!PropertyPath.Split(TEXT("."), &Head, &Tail))
    {
        // No dot — shouldn't happen (caller checks), but handle gracefully
        return SetPropertyValue(Object, PropertyPath, JsonValue, OutPropertyType);
    }

    // Find the head property on the object
    FProperty* HeadProp = Object->GetClass()->FindPropertyByName(FName(*Head));
    if (!HeadProp)
    {
        // Case-insensitive fallback
        for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
        {
            if (It->GetName().Equals(Head, ESearchCase::IgnoreCase))
            {
                HeadProp = *It;
                break;
            }
        }
    }

    if (!HeadProp)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetPropertyValueByPath: Could not find property '%s' on %s"),
            *Head, *Object->GetClass()->GetName());
        return false;
    }

    // Head must be a struct property to traverse into
    FStructProperty* StructProp = CastField<FStructProperty>(HeadProp);
    if (!StructProp)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetPropertyValueByPath: Property '%s' is not a struct (type: %s)"),
            *Head, *HeadProp->GetCPPType());
        return false;
    }

    void* InnerPtr = StructProp->ContainerPtrToValuePtr<void>(Object);
    UScriptStruct* InnerStruct = StructProp->Struct;

    // If tail still contains dots, recurse deeper
    if (Tail.Contains(TEXT(".")))
    {
        FString NextHead, NextTail;
        Tail.Split(TEXT("."), &NextHead, &NextTail);

        FProperty* NextProp = InnerStruct->FindPropertyByName(FName(*NextHead));
        if (!NextProp)
        {
            for (TFieldIterator<FProperty> It(InnerStruct); It; ++It)
            {
                if (It->GetName().Equals(NextHead, ESearchCase::IgnoreCase))
                {
                    NextProp = *It;
                    break;
                }
            }
        }

        if (NextProp)
        {
            FStructProperty* NextStructProp = CastField<FStructProperty>(NextProp);
            if (NextStructProp)
            {
                void* DeepPtr = NextStructProp->ContainerPtrToValuePtr<void>(InnerPtr);
                return SetStructPropertyValue(NextStructProp->Struct, DeepPtr, NextTail, JsonValue, OutPropertyType);
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("SetPropertyValueByPath: Could not traverse deeper into '%s.%s'"), *Head, *NextHead);
        return false;
    }

    // Tail is the leaf property name — set it on the inner struct
    return SetStructPropertyValue(InnerStruct, InnerPtr, Tail, JsonValue, OutPropertyType);
}

TSharedPtr<FJsonObject> FPCGNodePropertyManager::SetSpawnerEntries(const TSharedPtr<FJsonObject>& Params)
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

    const TArray<TSharedPtr<FJsonValue>>* EntriesArray;
    if (!Params->TryGetArrayField(TEXT("entries"), EntriesArray))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'entries' array parameter"));
    }

    UPCGGraph* Graph = FPCGGraphCreator::LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    UPCGNode* Node = FPCGNodeManager::FindNodeByName(Graph, NodeId);
    if (!Node)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find node '%s' in PCG graph"), *NodeId));
    }

    UPCGSettings* Settings = Node->GetSettings();
    if (!Settings)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Node '%s' has no settings object"), *NodeId));
    }

    // Use reflection to find the mesh entries TArray property.
    // In UE5.7, StaticMeshSpawner stores entries on MeshSelectorParameters sub-object.
    FArrayProperty* MeshArrayProp = nullptr;
    FStructProperty* MeshEntryStructProp = nullptr;
    UObject* MeshEntriesOwner = Settings; // The object that owns the array (may be sub-object)

    // Lambda to search an object for mesh entry arrays
    auto FindMeshEntriesOnObject = [&](UObject* Obj) -> bool
    {
        for (TFieldIterator<FArrayProperty> It(Obj->GetClass()); It; ++It)
        {
            FStructProperty* InnerStructProp = CastField<FStructProperty>((*It)->Inner);
            if (InnerStructProp)
            {
                FString StructName = InnerStructProp->Struct->GetName();
                if (StructName.Contains(TEXT("MeshSpawnerEntry")) ||
                    StructName.Contains(TEXT("WeightedMesh")) ||
                    StructName.Contains(TEXT("MeshEntry")) ||
                    StructName.Contains(TEXT("MeshSelectorWeighted")))
                {
                    MeshArrayProp = *It;
                    MeshEntryStructProp = InnerStructProp;
                    MeshEntriesOwner = Obj;
                    return true;
                }
            }
        }
        return false;
    };

    // First search directly on settings
    FindMeshEntriesOnObject(Settings);

    // If not found, check MeshSelectorParameters sub-object (UE5.7 pattern)
    if (!MeshArrayProp)
    {
        FObjectProperty* SelectorProp = FindFProperty<FObjectProperty>(Settings->GetClass(), TEXT("MeshSelectorParameters"));
        if (SelectorProp)
        {
            UObject* SelectorObj = SelectorProp->GetObjectPropertyValue(SelectorProp->ContainerPtrToValuePtr<void>(Settings));
            if (SelectorObj)
            {
                FindMeshEntriesOnObject(SelectorObj);
            }
        }
    }

    if (!MeshArrayProp || !MeshEntryStructProp)
    {
        // List available array properties for debugging
        FString AvailableArrays;
        int32 Count = 0;
        for (TFieldIterator<FArrayProperty> It(Settings->GetClass()); It; ++It)
        {
            if (Count > 0) AvailableArrays += TEXT(", ");
            AvailableArrays += (*It)->GetName();
            FStructProperty* InnerS = CastField<FStructProperty>((*It)->Inner);
            if (InnerS)
            {
                AvailableArrays += TEXT("(") + InnerS->Struct->GetName() + TEXT(")");
            }
            Count++;
        }

        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Node '%s' (class: %s) does not appear to be a StaticMeshSpawner. "
                "Available array properties: %s"),
                *NodeId, *Settings->GetClass()->GetName(), *AvailableArrays));
    }

    UScriptStruct* EntryStruct = MeshEntryStructProp->Struct;

    // Get the array helper to manipulate the TArray (use MeshEntriesOwner, not Settings)
    FScriptArrayHelper ArrayHelper(MeshArrayProp, MeshArrayProp->ContainerPtrToValuePtr<void>(MeshEntriesOwner));

    // Clear existing entries
    ArrayHelper.EmptyValues();

    int32 AddedCount = 0;

    for (const TSharedPtr<FJsonValue>& EntryValue : *EntriesArray)
    {
        const TSharedPtr<FJsonObject>* EntryObj;
        if (!EntryValue->TryGetObject(EntryObj))
        {
            continue;
        }

        FString MeshPath;
        if (!(*EntryObj)->TryGetStringField(TEXT("mesh_path"), MeshPath))
        {
            continue;
        }

        int32 Weight = 1;
        if ((*EntryObj)->HasField(TEXT("weight")))
        {
            Weight = static_cast<int32>((*EntryObj)->GetNumberField(TEXT("weight")));
        }

        // Add a new element to the array
        int32 NewIndex = ArrayHelper.AddValue();
        uint8* EntryPtr = ArrayHelper.GetRawPtr(NewIndex);

        // Lambda to set a mesh on a property within a given struct
        auto TrySetMeshOnProperty = [&MeshPath](FProperty* Prop, void* PropValuePtr) -> bool
        {
            // Try FSoftObjectProperty first (TSoftObjectPtr<UStaticMesh>)
            if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Prop))
            {
                FSoftObjectPtr& SoftPtr = *reinterpret_cast<FSoftObjectPtr*>(PropValuePtr);
                SoftPtr = FSoftObjectPath(MeshPath);
                return true;
            }

            // Try FStructProperty for FSoftObjectPath
            if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
            {
                if (StructProp->Struct == TBaseStructure<FSoftObjectPath>::Get())
                {
                    FSoftObjectPath& SoftPath = *reinterpret_cast<FSoftObjectPath*>(PropValuePtr);
                    SoftPath.SetPath(MeshPath);
                    return true;
                }
            }

            // Try FObjectProperty (direct UStaticMesh pointer)
            if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
            {
                UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
                if (LoadedMesh)
                {
                    ObjProp->SetObjectPropertyValue(PropValuePtr, LoadedMesh);
                    return true;
                }
            }
            return false;
        };

        // Set each field on the new entry using reflection
        bool bMeshSet = false;
        for (TFieldIterator<FProperty> PropIt(EntryStruct); PropIt; ++PropIt)
        {
            FProperty* Prop = *PropIt;
            void* PropValuePtr = Prop->ContainerPtrToValuePtr<void>(EntryPtr);
            FString PropName = Prop->GetName();

            // Set the mesh path — look for soft object path / mesh properties
            if (!bMeshSet && (PropName.Contains(TEXT("Mesh"), ESearchCase::IgnoreCase) ||
                PropName.Contains(TEXT("StaticMesh"), ESearchCase::IgnoreCase)))
            {
                bMeshSet = TrySetMeshOnProperty(Prop, PropValuePtr);
                if (bMeshSet) continue;
            }

            // UE5.7: mesh is inside a Descriptor struct — recurse into it
            if (!bMeshSet && PropName.Equals(TEXT("Descriptor"), ESearchCase::IgnoreCase))
            {
                FStructProperty* DescStructProp = CastField<FStructProperty>(Prop);
                if (DescStructProp)
                {
                    // Search inside the Descriptor for mesh-like properties
                    for (TFieldIterator<FProperty> DescIt(DescStructProp->Struct); DescIt; ++DescIt)
                    {
                        FProperty* DescProp = *DescIt;
                        FString DescPropName = DescProp->GetName();
                        if (DescPropName.Contains(TEXT("Mesh"), ESearchCase::IgnoreCase) ||
                            DescPropName.Contains(TEXT("StaticMesh"), ESearchCase::IgnoreCase))
                        {
                            void* DescPropValuePtr = DescProp->ContainerPtrToValuePtr<void>(PropValuePtr);
                            bMeshSet = TrySetMeshOnProperty(DescProp, DescPropValuePtr);
                            if (bMeshSet) break;
                        }
                    }
                    if (bMeshSet) continue;
                }
            }

            // Set the weight
            if (PropName.Contains(TEXT("Weight"), ESearchCase::IgnoreCase))
            {
                if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
                {
                    IntProp->SetPropertyValue(PropValuePtr, Weight);
                    continue;
                }
                if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
                {
                    FloatProp->SetPropertyValue(PropValuePtr, static_cast<float>(Weight));
                    continue;
                }
            }
        }

        AddedCount++;
    }

    // Mark as modified
    Settings->MarkPackageDirty();
    Graph->ForceNotificationForEditor(EPCGChangeType::Structural);
    Graph->GetPackage()->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("node_id"), NodeId);
    Result->SetNumberField(TEXT("entry_count"), AddedCount);
    Result->SetStringField(TEXT("array_property"), MeshArrayProp->GetName());
    Result->SetStringField(TEXT("entry_struct"), EntryStruct->GetName());

    return Result;
}
