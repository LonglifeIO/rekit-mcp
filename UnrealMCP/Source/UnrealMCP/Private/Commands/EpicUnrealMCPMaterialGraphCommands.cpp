#include "Commands/EpicUnrealMCPMaterialGraphCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"

// Material editing
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

// Asset creation
#include "Factories/MaterialFactoryNew.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "EditorAssetLibrary.h"

// Landscape
#include "EngineUtils.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "LandscapeInfo.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Kismet/GameplayStatics.h"

// Texture loading
#include "Engine/Texture2D.h"

FEpicUnrealMCPMaterialGraphCommands::FEpicUnrealMCPMaterialGraphCommands()
{
}

FEpicUnrealMCPMaterialGraphCommands::~FEpicUnrealMCPMaterialGraphCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_material"))
    {
        return HandleCreateMaterial(Params);
    }
    else if (CommandType == TEXT("add_material_expression"))
    {
        return HandleAddMaterialExpression(Params);
    }
    else if (CommandType == TEXT("set_material_expression_param"))
    {
        return HandleSetMaterialExpressionParam(Params);
    }
    else if (CommandType == TEXT("connect_material_expressions"))
    {
        return HandleConnectMaterialExpressions(Params);
    }
    else if (CommandType == TEXT("connect_material_to_output"))
    {
        return HandleConnectMaterialToOutput(Params);
    }
    else if (CommandType == TEXT("set_landscape_material"))
    {
        return HandleSetLandscapeMaterial(Params);
    }
    else if (CommandType == TEXT("compile_material"))
    {
        return HandleCompileMaterial(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown material graph command: %s"), *CommandType));
}

// ============================================================================
// Helper: Load material by path
// ============================================================================
static UMaterial* LoadMaterialByPath(const FString& MaterialPath)
{
    // Strategy 1: Direct load
    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (Material) return Material;

    // Strategy 2: Try with asset name appended
    FString AssetName = FPaths::GetCleanFilename(MaterialPath);
    FString FullPath = MaterialPath + TEXT(".") + AssetName;
    Material = LoadObject<UMaterial>(nullptr, *FullPath);
    if (Material) return Material;

    // Strategy 3: Asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByClass(UMaterial::StaticClass()->GetClassPathName(), AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.GetObjectPathString().Contains(MaterialPath) ||
            AssetData.AssetName.ToString() == AssetName)
        {
            return Cast<UMaterial>(AssetData.GetAsset());
        }
    }

    return nullptr;
}

// ============================================================================
// Helper: Find expression by name in material
// ============================================================================
static UMaterialExpression* FindExpressionByName(UMaterial* Material, const FString& ExpressionName)
{
    if (!Material) return nullptr;

    for (UMaterialExpression* Expr : Material->GetExpressions())
    {
        if (!Expr) continue;

        // Check the description (user-set name)
        if (!Expr->Desc.IsEmpty() && Expr->Desc == ExpressionName)
        {
            return Expr;
        }

        // Check parameter name for parameter expressions
        if (UMaterialExpressionParameter* ParamExpr = Cast<UMaterialExpressionParameter>(Expr))
        {
            if (ParamExpr->ParameterName.ToString() == ExpressionName)
            {
                return Expr;
            }
        }

        // Check texture parameter name
        if (UMaterialExpressionTextureSampleParameter* TexParam = Cast<UMaterialExpressionTextureSampleParameter>(Expr))
        {
            if (TexParam->ParameterName.ToString() == ExpressionName)
            {
                return Expr;
            }
        }

        // Check FName
        if (Expr->GetFName().ToString() == ExpressionName)
        {
            return Expr;
        }
    }

    return nullptr;
}

// ============================================================================
// 1. create_material
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialName;
    if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
    }

    FString Path = TEXT("/Game/Materials");
    Params->TryGetStringField(TEXT("path"), Path);

    bool bTwoSided = false;
    Params->TryGetBoolField(TEXT("two_sided"), bTwoSided);

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleCreateMaterial: Creating material '%s' at '%s'"), *MaterialName, *Path);

    // Build full path
    FString FullPath = Path / MaterialName;

    // Check if material already exists
    UMaterial* ExistingMaterial = LoadObject<UMaterial>(nullptr, *(FullPath + TEXT(".") + MaterialName));
    if (ExistingMaterial)
    {
        // Return existing material info instead of error
        TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
        Result->SetBoolField(TEXT("success"), true);
        Result->SetStringField(TEXT("material_path"), FullPath);
        Result->SetStringField(TEXT("material_name"), MaterialName);
        Result->SetBoolField(TEXT("already_existed"), true);
        return Result;
    }

    // Create the package
    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to create package at '%s'"), *FullPath));
    }

    // Use MaterialFactoryNew to create the material
    UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
    UMaterial* NewMaterial = Cast<UMaterial>(Factory->FactoryCreateNew(
        UMaterial::StaticClass(), Package, FName(*MaterialName),
        RF_Public | RF_Standalone, nullptr, GWarn));

    if (!NewMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material object"));
    }

    // Set two-sided if requested
    if (bTwoSided)
    {
        NewMaterial->TwoSided = true;
    }

    // Register with asset registry
    FAssetRegistryModule::AssetCreated(NewMaterial);
    Package->MarkPackageDirty();

    // Save the package
    FString PackageFilename = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewMaterial, *PackageFilename, SaveArgs);

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleCreateMaterial: Created material at '%s'"), *FullPath);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("material_path"), FullPath);
    Result->SetStringField(TEXT("material_name"), MaterialName);
    Result->SetBoolField(TEXT("already_existed"), false);

    return Result;
}

// ============================================================================
// 2. add_material_expression
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleAddMaterialExpression(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionClass;
    if (!Params->TryGetStringField(TEXT("expression_class"), ExpressionClass))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_class' parameter"));
    }

    FString NodeName;
    Params->TryGetStringField(TEXT("node_name"), NodeName);

    double PosXD = 0.0, PosYD = 0.0;
    Params->TryGetNumberField(TEXT("pos_x"), PosXD);
    Params->TryGetNumberField(TEXT("pos_y"), PosYD);
    int32 PosX = static_cast<int32>(PosXD);
    int32 PosY = static_cast<int32>(PosYD);

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleAddMaterialExpression: Adding '%s' to material '%s'"),
        *ExpressionClass, *MaterialPath);

    UMaterial* Material = LoadMaterialByPath(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find material at '%s'"), *MaterialPath));
    }

    // Map friendly class names to UClass
    static TMap<FString, UClass*> ExpressionClassMap;
    if (ExpressionClassMap.Num() == 0)
    {
        ExpressionClassMap.Add(TEXT("TextureSample"), UMaterialExpressionTextureSample::StaticClass());
        ExpressionClassMap.Add(TEXT("TextureSampleParameter2D"), UMaterialExpressionTextureSampleParameter2D::StaticClass());
        ExpressionClassMap.Add(TEXT("WorldPosition"), UMaterialExpressionWorldPosition::StaticClass());
        ExpressionClassMap.Add(TEXT("LinearInterpolate"), UMaterialExpressionLinearInterpolate::StaticClass());
        ExpressionClassMap.Add(TEXT("Lerp"), UMaterialExpressionLinearInterpolate::StaticClass());
        ExpressionClassMap.Add(TEXT("ComponentMask"), UMaterialExpressionComponentMask::StaticClass());
        ExpressionClassMap.Add(TEXT("Divide"), UMaterialExpressionDivide::StaticClass());
        ExpressionClassMap.Add(TEXT("Subtract"), UMaterialExpressionSubtract::StaticClass());
        ExpressionClassMap.Add(TEXT("Clamp"), UMaterialExpressionClamp::StaticClass());
        ExpressionClassMap.Add(TEXT("Constant"), UMaterialExpressionConstant::StaticClass());
        ExpressionClassMap.Add(TEXT("TextureCoordinate"), UMaterialExpressionTextureCoordinate::StaticClass());
        ExpressionClassMap.Add(TEXT("TexCoord"), UMaterialExpressionTextureCoordinate::StaticClass());
        ExpressionClassMap.Add(TEXT("Multiply"), UMaterialExpressionMultiply::StaticClass());
        ExpressionClassMap.Add(TEXT("Add"), UMaterialExpressionAdd::StaticClass());
        ExpressionClassMap.Add(TEXT("AppendVector"), UMaterialExpressionAppendVector::StaticClass());
        ExpressionClassMap.Add(TEXT("Constant2Vector"), UMaterialExpressionConstant2Vector::StaticClass());
        ExpressionClassMap.Add(TEXT("Constant3Vector"), UMaterialExpressionConstant3Vector::StaticClass());
        ExpressionClassMap.Add(TEXT("Constant4Vector"), UMaterialExpressionConstant4Vector::StaticClass());
        ExpressionClassMap.Add(TEXT("ScalarParameter"), UMaterialExpressionScalarParameter::StaticClass());
        ExpressionClassMap.Add(TEXT("VectorParameter"), UMaterialExpressionVectorParameter::StaticClass());
        ExpressionClassMap.Add(TEXT("TextureObjectParameter"), UMaterialExpressionTextureObjectParameter::StaticClass());
    }

    UClass* TargetClass = nullptr;

    // Check friendly name map first
    if (UClass** Found = ExpressionClassMap.Find(ExpressionClass))
    {
        TargetClass = *Found;
    }
    else
    {
        // Try to find by full/partial class name via reflection
        FString FullClassName = TEXT("MaterialExpression") + ExpressionClass;
        for (TObjectIterator<UClass> It; It; ++It)
        {
            if (It->IsChildOf(UMaterialExpression::StaticClass()) &&
                (It->GetName() == ExpressionClass || It->GetName() == FullClassName))
            {
                TargetClass = *It;
                break;
            }
        }
    }

    if (!TargetClass)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown expression class: '%s'. Use friendly names like TextureSample, WorldPosition, Lerp, ComponentMask, Divide, Subtract, Clamp, Constant, TextureCoordinate, Multiply, Add, ScalarParameter, VectorParameter, etc."), *ExpressionClass));
    }

    // Create the expression using UMaterialEditingLibrary
    UMaterialExpression* NewExpression = UMaterialEditingLibrary::CreateMaterialExpression(
        Material, TargetClass, PosX, PosY);

    if (!NewExpression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to create material expression '%s'"), *ExpressionClass));
    }

    // Set the description as the user-friendly name for later lookup
    if (!NodeName.IsEmpty())
    {
        NewExpression->Desc = NodeName;
    }

    // Mark material as dirty
    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    Material->MarkPackageDirty();

    // Build response
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("expression_name"), NewExpression->GetFName().ToString());
    Result->SetStringField(TEXT("expression_class"), TargetClass->GetName());
    if (!NodeName.IsEmpty())
    {
        Result->SetStringField(TEXT("node_name"), NodeName);
    }

    // Find the index of this expression
    auto Expressions = Material->GetExpressions();
    int32 ExprIndex = INDEX_NONE;
    for (int32 i = 0; i < Expressions.Num(); i++)
    {
        if (Expressions[i] == NewExpression) { ExprIndex = i; break; }
    }
    Result->SetNumberField(TEXT("expression_index"), ExprIndex);

    return Result;
}

// ============================================================================
// 3. set_material_expression_param
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleSetMaterialExpressionParam(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionName;
    if (!Params->TryGetStringField(TEXT("expression_name"), ExpressionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_name' parameter"));
    }

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleSetMaterialExpressionParam: Setting '%s' on '%s' in material '%s'"),
        *ParamName, *ExpressionName, *MaterialPath);

    UMaterial* Material = LoadMaterialByPath(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find material at '%s'"), *MaterialPath));
    }

    UMaterialExpression* Expression = FindExpressionByName(Material, ExpressionName);
    if (!Expression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find expression '%s' in material"), *ExpressionName));
    }

    // Handle specific parameter types based on expression class and param name
    bool bHandled = false;

    // Texture path for TextureSample/TextureSampleParameter2D
    if (ParamName == TEXT("Texture") || ParamName == TEXT("texture") || ParamName == TEXT("texture_path"))
    {
        FString TexturePath;
        if (Params->TryGetStringField(TEXT("param_value"), TexturePath))
        {
            UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
            if (!Texture)
            {
                // Try appending asset name
                FString AssetName = FPaths::GetCleanFilename(TexturePath);
                Texture = LoadObject<UTexture>(nullptr, *(TexturePath + TEXT(".") + AssetName));
            }

            if (Texture)
            {
                if (UMaterialExpressionTextureSample* TexSample = Cast<UMaterialExpressionTextureSample>(Expression))
                {
                    TexSample->Texture = Texture;
                    bHandled = true;
                }
                else if (UMaterialExpressionTextureObjectParameter* TexObjParam = Cast<UMaterialExpressionTextureObjectParameter>(Expression))
                {
                    TexObjParam->Texture = Texture;
                    bHandled = true;
                }
            }
            else
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                    FString::Printf(TEXT("Could not load texture at '%s'"), *TexturePath));
            }
        }
    }
    // SamplerType for TextureSample
    else if (ParamName == TEXT("SamplerType") || ParamName == TEXT("sampler_type"))
    {
        FString SamplerTypeStr;
        if (Params->TryGetStringField(TEXT("param_value"), SamplerTypeStr))
        {
            if (UMaterialExpressionTextureSample* TexSample = Cast<UMaterialExpressionTextureSample>(Expression))
            {
                if (SamplerTypeStr == TEXT("LinearColor") || SamplerTypeStr == TEXT("Color"))
                {
                    TexSample->SamplerType = SAMPLERTYPE_Color;
                }
                else if (SamplerTypeStr == TEXT("Normal"))
                {
                    TexSample->SamplerType = SAMPLERTYPE_Normal;
                }
                else if (SamplerTypeStr == TEXT("LinearGrayscale") || SamplerTypeStr == TEXT("Grayscale"))
                {
                    TexSample->SamplerType = SAMPLERTYPE_Grayscale;
                }
                else if (SamplerTypeStr == TEXT("Masks") || SamplerTypeStr == TEXT("LinearMasks"))
                {
                    TexSample->SamplerType = SAMPLERTYPE_Masks;
                }
                bHandled = true;
            }
        }
    }
    // ParameterName for parameter expressions
    else if (ParamName == TEXT("ParameterName") || ParamName == TEXT("parameter_name"))
    {
        FString ParamValue;
        if (Params->TryGetStringField(TEXT("param_value"), ParamValue))
        {
            if (UMaterialExpressionParameter* ParamExpr = Cast<UMaterialExpressionParameter>(Expression))
            {
                ParamExpr->ParameterName = FName(*ParamValue);
                bHandled = true;
            }
            else if (UMaterialExpressionTextureSampleParameter* TexParam = Cast<UMaterialExpressionTextureSampleParameter>(Expression))
            {
                TexParam->ParameterName = FName(*ParamValue);
                bHandled = true;
            }
        }
    }
    // Constant value
    else if (ParamName == TEXT("R") || ParamName == TEXT("Value") || ParamName == TEXT("value"))
    {
        double Value = 0.0;
        if (Params->TryGetNumberField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionConstant* ConstExpr = Cast<UMaterialExpressionConstant>(Expression))
            {
                ConstExpr->R = static_cast<float>(Value);
                bHandled = true;
            }
            else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expression))
            {
                ScalarParam->DefaultValue = static_cast<float>(Value);
                bHandled = true;
            }
        }
    }
    // TextureCoordinate UTiling/VTiling
    else if (ParamName == TEXT("UTiling") || ParamName == TEXT("u_tiling"))
    {
        double Value = 1.0;
        if (Params->TryGetNumberField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionTextureCoordinate* TexCoord = Cast<UMaterialExpressionTextureCoordinate>(Expression))
            {
                TexCoord->UTiling = static_cast<float>(Value);
                bHandled = true;
            }
        }
    }
    else if (ParamName == TEXT("VTiling") || ParamName == TEXT("v_tiling"))
    {
        double Value = 1.0;
        if (Params->TryGetNumberField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionTextureCoordinate* TexCoord = Cast<UMaterialExpressionTextureCoordinate>(Expression))
            {
                TexCoord->VTiling = static_cast<float>(Value);
                bHandled = true;
            }
        }
    }
    // ComponentMask channels (R, G, B, A)
    else if (ParamName == TEXT("R_mask") || ParamName == TEXT("r"))
    {
        bool Value = false;
        if (Params->TryGetBoolField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionComponentMask* MaskExpr = Cast<UMaterialExpressionComponentMask>(Expression))
            {
                MaskExpr->R = Value;
                bHandled = true;
            }
        }
    }
    else if (ParamName == TEXT("G_mask") || ParamName == TEXT("g"))
    {
        bool Value = false;
        if (Params->TryGetBoolField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionComponentMask* MaskExpr = Cast<UMaterialExpressionComponentMask>(Expression))
            {
                MaskExpr->G = Value;
                bHandled = true;
            }
        }
    }
    else if (ParamName == TEXT("B_mask") || ParamName == TEXT("b"))
    {
        bool Value = false;
        if (Params->TryGetBoolField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionComponentMask* MaskExpr = Cast<UMaterialExpressionComponentMask>(Expression))
            {
                MaskExpr->B = Value;
                bHandled = true;
            }
        }
    }
    else if (ParamName == TEXT("A_mask") || ParamName == TEXT("a"))
    {
        bool Value = false;
        if (Params->TryGetBoolField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionComponentMask* MaskExpr = Cast<UMaterialExpressionComponentMask>(Expression))
            {
                MaskExpr->A = Value;
                bHandled = true;
            }
        }
    }
    // Clamp Min/Max default values
    else if (ParamName == TEXT("MinDefault") || ParamName == TEXT("min_default"))
    {
        double Value = 0.0;
        if (Params->TryGetNumberField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionClamp* ClampExpr = Cast<UMaterialExpressionClamp>(Expression))
            {
                ClampExpr->MinDefault = static_cast<float>(Value);
                bHandled = true;
            }
        }
    }
    else if (ParamName == TEXT("MaxDefault") || ParamName == TEXT("max_default"))
    {
        double Value = 1.0;
        if (Params->TryGetNumberField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionClamp* ClampExpr = Cast<UMaterialExpressionClamp>(Expression))
            {
                ClampExpr->MaxDefault = static_cast<float>(Value);
                bHandled = true;
            }
        }
    }
    // ConstB for Divide/Subtract/Multiply/Add (the inline constant value)
    else if (ParamName == TEXT("ConstB") || ParamName == TEXT("const_b"))
    {
        double Value = 1.0;
        if (Params->TryGetNumberField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionDivide* DivExpr = Cast<UMaterialExpressionDivide>(Expression))
            {
                DivExpr->ConstB = static_cast<float>(Value);
                bHandled = true;
            }
            else if (UMaterialExpressionSubtract* SubExpr = Cast<UMaterialExpressionSubtract>(Expression))
            {
                SubExpr->ConstB = static_cast<float>(Value);
                bHandled = true;
            }
            else if (UMaterialExpressionMultiply* MulExpr = Cast<UMaterialExpressionMultiply>(Expression))
            {
                MulExpr->ConstB = static_cast<float>(Value);
                bHandled = true;
            }
            else if (UMaterialExpressionAdd* AddExpr = Cast<UMaterialExpressionAdd>(Expression))
            {
                AddExpr->ConstB = static_cast<float>(Value);
                bHandled = true;
            }
        }
    }
    else if (ParamName == TEXT("ConstA") || ParamName == TEXT("const_a"))
    {
        double Value = 0.0;
        if (Params->TryGetNumberField(TEXT("param_value"), Value))
        {
            if (UMaterialExpressionDivide* DivExpr = Cast<UMaterialExpressionDivide>(Expression))
            {
                DivExpr->ConstA = static_cast<float>(Value);
                bHandled = true;
            }
            else if (UMaterialExpressionSubtract* SubExpr = Cast<UMaterialExpressionSubtract>(Expression))
            {
                SubExpr->ConstA = static_cast<float>(Value);
                bHandled = true;
            }
            else if (UMaterialExpressionMultiply* MulExpr = Cast<UMaterialExpressionMultiply>(Expression))
            {
                MulExpr->ConstA = static_cast<float>(Value);
                bHandled = true;
            }
            else if (UMaterialExpressionAdd* AddExpr = Cast<UMaterialExpressionAdd>(Expression))
            {
                AddExpr->ConstA = static_cast<float>(Value);
                bHandled = true;
            }
        }
    }

    // Fallback: Try generic FProperty approach
    if (!bHandled)
    {
        FProperty* Property = Expression->GetClass()->FindPropertyByName(FName(*ParamName));
        if (Property)
        {
            // Try to set via JSON value
            const TSharedPtr<FJsonValue>* ParamValue;
            if (Params->Values.Contains(TEXT("param_value")))
            {
                ParamValue = &Params->Values[TEXT("param_value")];
                FString ErrorMessage;
                if (FEpicUnrealMCPCommonUtils::SetObjectProperty(Expression, ParamName, *ParamValue, ErrorMessage))
                {
                    bHandled = true;
                }
                else
                {
                    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                        FString::Printf(TEXT("Failed to set property '%s': %s"), *ParamName, *ErrorMessage));
                }
            }
        }
    }

    if (!bHandled)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not set parameter '%s' on expression '%s'. Supported params depend on expression type."), *ParamName, *ExpressionName));
    }

    // Mark material dirty
    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("expression_name"), ExpressionName);
    Result->SetStringField(TEXT("param_name"), ParamName);

    return Result;
}

// ============================================================================
// 4. connect_material_expressions
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleConnectMaterialExpressions(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString FromExpression;
    if (!Params->TryGetStringField(TEXT("from_expression"), FromExpression))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_expression' parameter"));
    }

    FString ToExpression;
    if (!Params->TryGetStringField(TEXT("to_expression"), ToExpression))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_expression' parameter"));
    }

    double FromOutputIndexD = 0.0;
    Params->TryGetNumberField(TEXT("from_output_index"), FromOutputIndexD);
    int32 FromOutputIndex = static_cast<int32>(FromOutputIndexD);

    double ToInputIndexD = 0.0;
    Params->TryGetNumberField(TEXT("to_input_index"), ToInputIndexD);
    int32 ToInputIndex = static_cast<int32>(ToInputIndexD);

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleConnectMaterialExpressions: Connecting '%s'[%d] -> '%s'[%d] in '%s'"),
        *FromExpression, FromOutputIndex, *ToExpression, ToInputIndex, *MaterialPath);

    UMaterial* Material = LoadMaterialByPath(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find material at '%s'"), *MaterialPath));
    }

    UMaterialExpression* FromExpr = FindExpressionByName(Material, FromExpression);
    if (!FromExpr)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find source expression '%s'"), *FromExpression));
    }

    UMaterialExpression* ToExpr = FindExpressionByName(Material, ToExpression);
    if (!ToExpr)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find target expression '%s'"), *ToExpression));
    }

    // Resolve output name with bounds check (NAME_None must become "" for UMaterialEditingLibrary)
    FString FromOutputName = TEXT("");
    const TArray<FExpressionOutput>& Outputs = FromExpr->GetOutputs();
    if (Outputs.IsValidIndex(FromOutputIndex))
    {
        FName OutName = Outputs[FromOutputIndex].OutputName;
        FromOutputName = OutName.IsNone() ? TEXT("") : OutName.ToString();
    }

    // Resolve input name (NAME_None must become "" for UMaterialEditingLibrary)
    FName InName = ToExpr->GetInputName(ToInputIndex);
    FString ToInputName = InName.IsNone() ? TEXT("") : InName.ToString();

    UE_LOG(LogTemp, Display, TEXT("ConnectMaterialExpressions: FromOutputName='%s', ToInputName='%s'"), *FromOutputName, *ToInputName);

    // Use UMaterialEditingLibrary to connect — try multiple name combinations
    // because NAME_None.ToString() returns "None" but the API expects ""
    bool bConnected = UMaterialEditingLibrary::ConnectMaterialExpressions(
        FromExpr, FromOutputName,
        ToExpr, ToInputName);

    if (!bConnected)
    {
        // Fallback 1: empty output name, keep input name
        bConnected = UMaterialEditingLibrary::ConnectMaterialExpressions(
            FromExpr, TEXT(""),
            ToExpr, ToInputName);
    }

    if (!bConnected)
    {
        // Fallback 2: keep output name, empty input name
        bConnected = UMaterialEditingLibrary::ConnectMaterialExpressions(
            FromExpr, FromOutputName,
            ToExpr, TEXT(""));
    }

    if (!bConnected)
    {
        // Fallback 3: both empty (default first output to first input)
        bConnected = UMaterialEditingLibrary::ConnectMaterialExpressions(
            FromExpr, TEXT(""),
            ToExpr, TEXT(""));
    }

    if (!bConnected)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to connect '%s'[%d] (out='%s') to '%s'[%d] (in='%s')"),
                *FromExpression, FromOutputIndex, *FromOutputName, *ToExpression, ToInputIndex, *ToInputName));
    }

    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("from_expression"), FromExpression);
    Result->SetStringField(TEXT("to_expression"), ToExpression);
    Result->SetNumberField(TEXT("from_output_index"), FromOutputIndex);
    Result->SetNumberField(TEXT("to_input_index"), ToInputIndex);

    return Result;
}

// ============================================================================
// 5. connect_material_to_output
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleConnectMaterialToOutput(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionName;
    if (!Params->TryGetStringField(TEXT("expression_name"), ExpressionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_name' parameter"));
    }

    FString MaterialProperty;
    if (!Params->TryGetStringField(TEXT("material_property"), MaterialProperty))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_property' parameter"));
    }

    double OutputIndexD = 0.0;
    Params->TryGetNumberField(TEXT("output_index"), OutputIndexD);
    int32 OutputIndex = static_cast<int32>(OutputIndexD);

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleConnectMaterialToOutput: Connecting '%s' to %s in '%s'"),
        *ExpressionName, *MaterialProperty, *MaterialPath);

    UMaterial* Material = LoadMaterialByPath(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find material at '%s'"), *MaterialPath));
    }

    UMaterialExpression* Expression = FindExpressionByName(Material, ExpressionName);
    if (!Expression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find expression '%s' in material"), *ExpressionName));
    }

    // Map string property names to EMaterialProperty
    EMaterialProperty MatProp;
    if (MaterialProperty == TEXT("BaseColor") || MaterialProperty == TEXT("Diffuse"))
    {
        MatProp = MP_BaseColor;
    }
    else if (MaterialProperty == TEXT("Normal"))
    {
        MatProp = MP_Normal;
    }
    else if (MaterialProperty == TEXT("Roughness"))
    {
        MatProp = MP_Roughness;
    }
    else if (MaterialProperty == TEXT("Metallic"))
    {
        MatProp = MP_Metallic;
    }
    else if (MaterialProperty == TEXT("AmbientOcclusion") || MaterialProperty == TEXT("AO"))
    {
        MatProp = MP_AmbientOcclusion;
    }
    else if (MaterialProperty == TEXT("EmissiveColor") || MaterialProperty == TEXT("Emissive"))
    {
        MatProp = MP_EmissiveColor;
    }
    else if (MaterialProperty == TEXT("Specular"))
    {
        MatProp = MP_Specular;
    }
    else if (MaterialProperty == TEXT("Opacity"))
    {
        MatProp = MP_Opacity;
    }
    else if (MaterialProperty == TEXT("OpacityMask"))
    {
        MatProp = MP_OpacityMask;
    }
    else if (MaterialProperty == TEXT("WorldPositionOffset"))
    {
        MatProp = MP_WorldPositionOffset;
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown material property: '%s'. Supported: BaseColor, Normal, Roughness, Metallic, AmbientOcclusion, EmissiveColor, Specular, Opacity, OpacityMask, WorldPositionOffset"), *MaterialProperty));
    }

    // Connect using UMaterialEditingLibrary
    FString OutputName;
    if (OutputIndex < Expression->GetOutputs().Num())
    {
        OutputName = Expression->GetOutputs()[OutputIndex].OutputName.ToString();
    }

    bool bConnected = UMaterialEditingLibrary::ConnectMaterialProperty(Expression, OutputName, MatProp);

    if (!bConnected)
    {
        // Try with empty output name
        bConnected = UMaterialEditingLibrary::ConnectMaterialProperty(Expression, TEXT(""), MatProp);
    }

    if (!bConnected)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to connect '%s' to material property '%s'"),
                *ExpressionName, *MaterialProperty));
    }

    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("expression_name"), ExpressionName);
    Result->SetStringField(TEXT("material_property"), MaterialProperty);

    return Result;
}

// ============================================================================
// 6. set_landscape_material
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleSetLandscapeMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleSetLandscapeMaterial: Assigning material '%s' to landscape '%s'"),
        *MaterialPath, *ActorName);

    // Find the landscape actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    ALandscapeProxy* LandscapeProxy = nullptr;

    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
    {
        if (It->GetActorLabel() == ActorName || It->GetName() == ActorName || It->GetFName().ToString() == ActorName)
        {
            LandscapeProxy = *It;
            break;
        }
    }

    if (!LandscapeProxy)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find landscape actor '%s'"), *ActorName));
    }

    // Load the material interface
    UMaterialInterface* MaterialInterface = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!MaterialInterface)
    {
        // Try with asset name appended
        FString AssetName = FPaths::GetCleanFilename(MaterialPath);
        MaterialInterface = LoadObject<UMaterialInterface>(nullptr, *(MaterialPath + TEXT(".") + AssetName));
    }

    if (!MaterialInterface)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not load material at '%s'"), *MaterialPath));
    }

    // Set the landscape material
    LandscapeProxy->LandscapeMaterial = MaterialInterface;
    LandscapeProxy->UpdateAllComponentMaterialInstances();
    LandscapeProxy->MarkPackageDirty();

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleSetLandscapeMaterial: Assigned material '%s' to landscape '%s'"),
        *MaterialPath, *ActorName);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("actor_name"), ActorName);
    Result->SetStringField(TEXT("material_path"), MaterialPath);

    return Result;
}

// ============================================================================
// 7. compile_material
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleCompileMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleCompileMaterial: Compiling material '%s'"), *MaterialPath);

    UMaterial* Material = LoadMaterialByPath(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not find material at '%s'"), *MaterialPath));
    }

    // Recompile the material
    UMaterialEditingLibrary::RecompileMaterial(Material);

    // Save the material package
    UPackage* Package = Material->GetOutermost();
    if (Package)
    {
        FString PackagePath = Package->GetPathName();
        FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, Material, *PackageFilename, SaveArgs);
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPMaterialGraphCommands::HandleCompileMaterial: Material compiled successfully"));

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("material_path"), MaterialPath);
    Result->SetStringField(TEXT("message"), TEXT("Material compiled and saved successfully"));

    return Result;
}
