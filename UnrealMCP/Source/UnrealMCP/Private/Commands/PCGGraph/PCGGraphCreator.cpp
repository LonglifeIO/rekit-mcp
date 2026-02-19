#include "Commands/PCGGraph/PCGGraphCreator.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGEdge.h"
#include "PCGSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

UPCGGraph* FPCGGraphCreator::LoadPCGGraph(const FString& GraphPath)
{
    // Strategy 1: Direct load
    UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
    if (Graph)
    {
        return Graph;
    }

    // Strategy 2: Try with asset name appended (e.g., /Game/PCG/MyGraph -> /Game/PCG/MyGraph.MyGraph)
    FString AssetName = FPaths::GetCleanFilename(GraphPath);
    FString FullPath = GraphPath + TEXT(".") + AssetName;
    Graph = LoadObject<UPCGGraph>(nullptr, *FullPath);
    if (Graph)
    {
        return Graph;
    }

    // Strategy 3: Asset registry search
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByClass(UPCGGraph::StaticClass()->GetClassPathName(), AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.GetObjectPathString().Contains(GraphPath) ||
            AssetData.AssetName.ToString() == AssetName)
        {
            return Cast<UPCGGraph>(AssetData.GetAsset());
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FPCGGraphCreator::LoadPCGGraph: Could not find PCG graph at '%s'"), *GraphPath);
    return nullptr;
}

TSharedPtr<FJsonObject> FPCGGraphCreator::CreatePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphName;
    if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
    }

    FString Path = TEXT("/Game/PCG");
    Params->TryGetStringField(TEXT("path"), Path);

    // Build the full package path
    FString FullPath = Path / GraphName;

    // Check if graph already exists
    UPCGGraph* ExistingGraph = LoadObject<UPCGGraph>(nullptr, *(FullPath + TEXT(".") + GraphName));
    if (ExistingGraph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("PCG graph already exists at '%s'"), *FullPath));
    }

    // Create the package
    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to create package at '%s'"), *FullPath));
    }

    // Create the PCG graph object
    UPCGGraph* NewGraph = NewObject<UPCGGraph>(Package, FName(*GraphName), RF_Public | RF_Standalone);
    if (!NewGraph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create PCG graph object"));
    }

    // Register with asset registry
    FAssetRegistryModule::AssetCreated(NewGraph);
    Package->MarkPackageDirty();

    // Save the package
    FString PackageFilename = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewGraph, *PackageFilename, SaveArgs);

    UE_LOG(LogTemp, Display, TEXT("FPCGGraphCreator::CreatePCGGraph: Created PCG graph at '%s'"), *FullPath);

    // Build response
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("graph_path"), FullPath);
    Result->SetStringField(TEXT("graph_name"), GraphName);

    return Result;
}

TSharedPtr<FJsonObject> FPCGGraphCreator::ReadPCGGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString GraphPath;
    if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
    }

    UPCGGraph* Graph = LoadPCGGraph(GraphPath);
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find PCG graph at '%s'"), *GraphPath));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("graph_path"), GraphPath);

    // Serialize nodes
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    const TArray<UPCGNode*>& Nodes = Graph->GetNodes();

    for (UPCGNode* Node : Nodes)
    {
        if (!Node) continue;

        TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
        NodeObj->SetStringField(TEXT("node_id"), Node->GetFName().ToString());
        int32 NodePosX = 0, NodePosY = 0;
        Node->GetNodePosition(NodePosX, NodePosY);
        NodeObj->SetNumberField(TEXT("pos_x"), NodePosX);
        NodeObj->SetNumberField(TEXT("pos_y"), NodePosY);

        // Settings info
        UPCGSettings* Settings = Node->GetSettings();
        if (Settings)
        {
            NodeObj->SetStringField(TEXT("settings_class"), Settings->GetClass()->GetName());
        }
        else
        {
            NodeObj->SetStringField(TEXT("settings_class"), TEXT("None"));
        }

        // Node title — derive from settings class name for portability
        FString NodeTitle;
        if (UPCGSettings* S = Node->GetSettings())
        {
            NodeTitle = S->GetClass()->GetName();
            if (NodeTitle.EndsWith(TEXT("Settings"))) NodeTitle = NodeTitle.LeftChop(8);
            if (NodeTitle.StartsWith(TEXT("PCG"))) NodeTitle = NodeTitle.RightChop(3);
        }
        else
        {
            NodeTitle = Node->GetFName().ToString();
        }
        NodeObj->SetStringField(TEXT("node_title"), NodeTitle);

        // Input pins
        TArray<TSharedPtr<FJsonValue>> InputPinsArray;
        for (UPCGPin* Pin : Node->GetInputPins())
        {
            if (!Pin) continue;
            TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
            PinObj->SetStringField(TEXT("pin_name"), Pin->Properties.Label.ToString());
            PinObj->SetBoolField(TEXT("is_connected"), Pin->IsConnected());
            InputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
        }
        NodeObj->SetArrayField(TEXT("input_pins"), InputPinsArray);

        // Output pins
        TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
        for (UPCGPin* Pin : Node->GetOutputPins())
        {
            if (!Pin) continue;
            TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
            PinObj->SetStringField(TEXT("pin_name"), Pin->Properties.Label.ToString());
            PinObj->SetBoolField(TEXT("is_connected"), Pin->IsConnected());
            OutputPinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
        }
        NodeObj->SetArrayField(TEXT("output_pins"), OutputPinsArray);

        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
    }
    Result->SetArrayField(TEXT("nodes"), NodesArray);

    // Also include the special Input/Output nodes
    TArray<TSharedPtr<FJsonValue>> SpecialNodesArray;

    UPCGNode* InputNode = Graph->GetInputNode();
    if (InputNode)
    {
        TSharedPtr<FJsonObject> InputObj = MakeShareable(new FJsonObject);
        InputObj->SetStringField(TEXT("node_id"), InputNode->GetFName().ToString());
        InputObj->SetStringField(TEXT("role"), TEXT("Input"));

        TArray<TSharedPtr<FJsonValue>> OutputPins;
        for (UPCGPin* Pin : InputNode->GetOutputPins())
        {
            if (!Pin) continue;
            TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
            PinObj->SetStringField(TEXT("pin_name"), Pin->Properties.Label.ToString());
            OutputPins.Add(MakeShareable(new FJsonValueObject(PinObj)));
        }
        InputObj->SetArrayField(TEXT("output_pins"), OutputPins);
        SpecialNodesArray.Add(MakeShareable(new FJsonValueObject(InputObj)));
    }

    UPCGNode* OutputNode = Graph->GetOutputNode();
    if (OutputNode)
    {
        TSharedPtr<FJsonObject> OutputObj = MakeShareable(new FJsonObject);
        OutputObj->SetStringField(TEXT("node_id"), OutputNode->GetFName().ToString());
        OutputObj->SetStringField(TEXT("role"), TEXT("Output"));

        TArray<TSharedPtr<FJsonValue>> InputPins;
        for (UPCGPin* Pin : OutputNode->GetInputPins())
        {
            if (!Pin) continue;
            TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
            PinObj->SetStringField(TEXT("pin_name"), Pin->Properties.Label.ToString());
            InputPins.Add(MakeShareable(new FJsonValueObject(PinObj)));
        }
        OutputObj->SetArrayField(TEXT("input_pins"), InputPins);
        SpecialNodesArray.Add(MakeShareable(new FJsonValueObject(OutputObj)));
    }
    Result->SetArrayField(TEXT("special_nodes"), SpecialNodesArray);

    // Serialize edges/connections
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
    for (UPCGNode* Node : Nodes)
    {
        if (!Node) continue;

        for (UPCGPin* OutputPin : Node->GetOutputPins())
        {
            if (!OutputPin) continue;

            for (const UPCGEdge* Edge : OutputPin->Edges)
            {
                if (!Edge) continue;
                const UPCGPin* OtherPin = Edge->GetOtherPin(OutputPin);
                if (!OtherPin || !OtherPin->GetOwner()) continue;

                TSharedPtr<FJsonObject> ConnObj = MakeShareable(new FJsonObject);
                ConnObj->SetStringField(TEXT("from_node_id"), Node->GetFName().ToString());
                ConnObj->SetStringField(TEXT("from_pin"), OutputPin->Properties.Label.ToString());
                ConnObj->SetStringField(TEXT("to_node_id"), OtherPin->GetOwner()->GetFName().ToString());
                ConnObj->SetStringField(TEXT("to_pin"), OtherPin->Properties.Label.ToString());
                ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnObj)));
            }
        }
    }
    Result->SetArrayField(TEXT("connections"), ConnectionsArray);

    // Serialize user parameters
    TArray<TSharedPtr<FJsonValue>> ParametersArray;
    // User parameters are stored in the graph's UserParameters property bag
    // GetUserParametersStruct returns const FInstancedPropertyBag* in UE5.7
    const FInstancedPropertyBag* UserParams = Graph->GetUserParametersStruct();
    const UPropertyBag* BagStruct = UserParams ? UserParams->GetPropertyBagStruct() : nullptr;
    if (BagStruct)
    {
        for (const FPropertyBagPropertyDesc& Desc : BagStruct->GetPropertyDescs())
        {
            TSharedPtr<FJsonObject> ParamObj = MakeShareable(new FJsonObject);
            ParamObj->SetStringField(TEXT("param_name"), Desc.Name.ToString());

            // Map property bag type to string
            FString TypeStr;
            switch (Desc.ValueType)
            {
            case EPropertyBagPropertyType::Bool:   TypeStr = TEXT("Bool"); break;
            case EPropertyBagPropertyType::Int32:   TypeStr = TEXT("Int32"); break;
            case EPropertyBagPropertyType::Int64:   TypeStr = TEXT("Int64"); break;
            case EPropertyBagPropertyType::Float:   TypeStr = TEXT("Float"); break;
            case EPropertyBagPropertyType::Double:  TypeStr = TEXT("Double"); break;
            case EPropertyBagPropertyType::String:  TypeStr = TEXT("String"); break;
            case EPropertyBagPropertyType::Name:    TypeStr = TEXT("Name"); break;
            case EPropertyBagPropertyType::Struct:  TypeStr = TEXT("Struct"); break;
            default: TypeStr = TEXT("Unknown"); break;
            }
            ParamObj->SetStringField(TEXT("param_type"), TypeStr);

            ParametersArray.Add(MakeShareable(new FJsonValueObject(ParamObj)));
        }
    }
    Result->SetArrayField(TEXT("parameters"), ParametersArray);

    Result->SetNumberField(TEXT("node_count"), Nodes.Num());

    return Result;
}
