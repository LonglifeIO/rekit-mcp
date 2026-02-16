#include "Commands/EpicUnrealMCPEditorCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Commands/EpicUnrealMCPBlueprintCommands.h"

FEpicUnrealMCPEditorCommands::FEpicUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor"))
    {
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Kitbashing commands
    else if (CommandType == TEXT("list_content_browser_meshes"))
    {
        return HandleListContentBrowserMeshes(Params);
    }
    else if (CommandType == TEXT("get_actor_details"))
    {
        return HandleGetActorDetails(Params);
    }
    else if (CommandType == TEXT("duplicate_actor"))
    {
        return HandleDuplicateActor(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        AStaticMeshActor* NewMeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
        if (NewMeshActor)
        {
            // Check for an optional static_mesh parameter to assign a mesh
            FString MeshPath;
            if (Params->TryGetStringField(TEXT("static_mesh"), MeshPath))
            {
                UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
                if (Mesh)
                {
                    NewMeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Could not find static mesh at path: %s"), *MeshPath);
                }
            }
        }
        NewActor = NewMeshActor;
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Set the actor label (display name in Outliner) to match the requested name
        NewActor->SetActorLabel(ActorName);

        // Return the created actor's details
        return FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FEpicUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FEpicUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // This function will now correctly call the implementation in BlueprintCommands
    FEpicUnrealMCPBlueprintCommands BlueprintCommands;
    return BlueprintCommands.HandleCommand(TEXT("spawn_blueprint_actor"), Params);
}

// ============================================================================
// Kitbashing Commands
// ============================================================================

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleListContentBrowserMeshes(const TSharedPtr<FJsonObject>& Params)
{
    // Get search parameters
    FString SearchPath;
    if (!Params->TryGetStringField(TEXT("search_path"), SearchPath))
    {
        SearchPath = TEXT("/Game/");
    }

    FString NameFilter;
    Params->TryGetStringField(TEXT("name_filter"), NameFilter);

    int32 MaxResults = 100;
    if (Params->HasField(TEXT("max_results")))
    {
        MaxResults = static_cast<int32>(Params->GetNumberField(TEXT("max_results")));
    }

    // Ensure path formatting
    if (!SearchPath.StartsWith(TEXT("/")))
    {
        SearchPath = TEXT("/") + SearchPath;
    }
    if (!SearchPath.EndsWith(TEXT("/")))
    {
        SearchPath += TEXT("/");
    }

    // Query Asset Registry for StaticMesh assets
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(*SearchPath);
    Filter.bRecursivePaths = true;

    TArray<FAssetData> AssetDataArray;
    AssetRegistry.GetAssets(Filter, AssetDataArray);

    UE_LOG(LogTemp, Log, TEXT("ListContentBrowserMeshes: Found %d meshes in %s"), AssetDataArray.Num(), *SearchPath);

    // Build JSON result, applying name filter and max_results
    TArray<TSharedPtr<FJsonValue>> MeshArray;
    for (const FAssetData& AssetData : AssetDataArray)
    {
        if (MeshArray.Num() >= MaxResults)
        {
            break;
        }

        FString AssetName = AssetData.AssetName.ToString();

        // Apply name filter if provided
        if (!NameFilter.IsEmpty() && !AssetName.Contains(NameFilter))
        {
            continue;
        }

        TSharedPtr<FJsonObject> MeshObj = MakeShared<FJsonObject>();
        MeshObj->SetStringField(TEXT("name"), AssetName);
        MeshObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        MeshObj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());

        MeshArray.Add(MakeShared<FJsonValueObject>(MeshObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("meshes"), MeshArray);
    ResultObj->SetNumberField(TEXT("count"), MeshArray.Num());
    ResultObj->SetNumberField(TEXT("total_found"), AssetDataArray.Num());
    ResultObj->SetStringField(TEXT("search_path"), SearchPath);
    if (!NameFilter.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("name_filter"), NameFilter);
    }

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetActorDetails(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Start with basic actor info
    TSharedPtr<FJsonObject> ResultObj = FEpicUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);

    // Add static mesh path if applicable
    AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(TargetActor);
    if (MeshActor && MeshActor->GetStaticMeshComponent())
    {
        UStaticMesh* Mesh = MeshActor->GetStaticMeshComponent()->GetStaticMesh();
        if (Mesh)
        {
            ResultObj->SetStringField(TEXT("static_mesh_path"), Mesh->GetPathName());
        }

        // Add material info
        UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
        TArray<TSharedPtr<FJsonValue>> MaterialsArray;
        for (int32 i = 0; i < MeshComp->GetNumMaterials(); i++)
        {
            UMaterialInterface* Material = MeshComp->GetMaterial(i);
            if (Material)
            {
                TSharedPtr<FJsonObject> MatObj = MakeShared<FJsonObject>();
                MatObj->SetNumberField(TEXT("slot"), i);
                MatObj->SetStringField(TEXT("name"), Material->GetName());
                MatObj->SetStringField(TEXT("path"), Material->GetPathName());
                MaterialsArray.Add(MakeShared<FJsonValueObject>(MatObj));
            }
        }
        ResultObj->SetArrayField(TEXT("materials"), MaterialsArray);
    }

    // Add bounding box info
    FVector Origin, BoxExtent;
    TargetActor->GetActorBounds(false, Origin, BoxExtent);

    auto VecToJsonArray = [](const FVector& V) -> TArray<TSharedPtr<FJsonValue>>
    {
        TArray<TSharedPtr<FJsonValue>> Arr;
        Arr.Add(MakeShared<FJsonValueNumber>(V.X));
        Arr.Add(MakeShared<FJsonValueNumber>(V.Y));
        Arr.Add(MakeShared<FJsonValueNumber>(V.Z));
        return Arr;
    };

    ResultObj->SetArrayField(TEXT("bounds_origin"), VecToJsonArray(Origin));
    ResultObj->SetArrayField(TEXT("bounds_extent"), VecToJsonArray(BoxExtent));
    ResultObj->SetArrayField(TEXT("bounds_min"), VecToJsonArray(Origin - BoxExtent));
    ResultObj->SetArrayField(TEXT("bounds_max"), VecToJsonArray(Origin + BoxExtent));

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDuplicateActor(const TSharedPtr<FJsonObject>& Params)
{
    FString SourceName;
    if (!Params->TryGetStringField(TEXT("source_name"), SourceName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_name' parameter"));
    }

    FString NewName;
    if (!Params->TryGetStringField(TEXT("new_name"), NewName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Find source actor
    AActor* SourceActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == SourceName)
        {
            SourceActor = Actor;
            break;
        }
    }

    if (!SourceActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source actor not found: %s"), *SourceName));
    }

    // Check for name collision
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == NewName)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *NewName));
        }
    }

    // Determine target location: explicit location, or source + offset
    FVector TargetLocation = SourceActor->GetActorLocation();
    if (Params->HasField(TEXT("location")))
    {
        TargetLocation = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    else if (Params->HasField(TEXT("offset")))
    {
        FVector Offset = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("offset"));
        TargetLocation += Offset;
    }

    // Determine rotation: explicit or copy from source
    FRotator TargetRotation = SourceActor->GetActorRotation();
    if (Params->HasField(TEXT("rotation")))
    {
        TargetRotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }

    // Determine scale: explicit or copy from source
    FVector TargetScale = SourceActor->GetActorScale3D();
    if (Params->HasField(TEXT("scale")))
    {
        TargetScale = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn new actor as StaticMeshActor if source is one, copying mesh + materials
    AStaticMeshActor* SourceMeshActor = Cast<AStaticMeshActor>(SourceActor);
    if (SourceMeshActor && SourceMeshActor->GetStaticMeshComponent())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = *NewName;

        AStaticMeshActor* NewMeshActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), TargetLocation, TargetRotation, SpawnParams);

        if (!NewMeshActor)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn duplicate StaticMeshActor"));
        }

        // Copy mesh
        UStaticMesh* SourceMesh = SourceMeshActor->GetStaticMeshComponent()->GetStaticMesh();
        if (SourceMesh)
        {
            NewMeshActor->GetStaticMeshComponent()->SetStaticMesh(SourceMesh);
        }

        // Copy materials
        UStaticMeshComponent* SourceComp = SourceMeshActor->GetStaticMeshComponent();
        UStaticMeshComponent* NewComp = NewMeshActor->GetStaticMeshComponent();
        for (int32 i = 0; i < SourceComp->GetNumMaterials(); i++)
        {
            UMaterialInterface* Mat = SourceComp->GetMaterial(i);
            if (Mat)
            {
                NewComp->SetMaterial(i, Mat);
            }
        }

        // Set scale
        FTransform NewTransform = NewMeshActor->GetTransform();
        NewTransform.SetScale3D(TargetScale);
        NewMeshActor->SetActorTransform(NewTransform);

        return FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewMeshActor, true);
    }

    // Fallback for non-StaticMeshActors: generic approach
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Source actor '%s' is not a StaticMeshActor. duplicate_actor only supports StaticMeshActors."), *SourceName));
}
