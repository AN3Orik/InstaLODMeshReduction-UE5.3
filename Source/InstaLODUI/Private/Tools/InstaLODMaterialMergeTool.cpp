/**
 * InstaLODMaterialMergeTool.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMaterialMergeTool.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODMaterialMergeTool.h"
#include "InstaLODUIPCH.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

UInstaLODMaterialMergeTool::UInstaLODMaterialMergeTool() :
Operation(nullptr),
OperationResult()
{
}

FText UInstaLODMaterialMergeTool::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "MaterialMergeToolFriendlyName", "MM");
}

FText UInstaLODMaterialMergeTool::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "MaterialMergeToolComboBoxItemName", "Material Merge");
}

FText UInstaLODMaterialMergeTool::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "MaterialMergeToolOperationInformation", "The Material Merge operation reduces draw calls by combining all material textures in your scene into a single material texture. In the process, the operation recalculates the UV layout for all objects in your scene while efficiently grouping identical meshes.");
}

int32 UInstaLODMaterialMergeTool::GetOrderId() const
{
	return 4;
}


void UInstaLODMaterialMergeTool::OnMeshOperationExecute(bool bIsAsynchronous)
{
	// --------------------------
	// can be run on child thread
	// --------------------------
	check(Operation == nullptr);
	
	InstaLOD::pfnMeshMergeProgressCallback ProgressCallback = [](InstaLOD::IMeshMergeOperation2 *, InstaLOD::IInstaLODMesh* , const float ProgressInPercent)
	{
		if (!IsInGameThread())
		{
			AsyncTask(ENamedThreads::GameThread, [ProgressInPercent]()  { GWarn->UpdateProgress(ProgressInPercent * 100, 100);  });
		}
		else
		{
			GWarn->UpdateProgress(ProgressInPercent * 100, 100);
		}
	};
	
	// alloc mesh operation
	Operation = GetInstaLODInterface()->GetInstaLOD()->AllocMeshMergeOperation();
	Operation->SetProgressCallback(ProgressCallback);
	
	Operation->SetMaterialData(MaterialData);
	Operation->AddMesh(InputMesh);
	
	// execute
	OperationResult = Operation->Execute(OutputMesh, GetMaterialMergeSettings());
}

InstaLOD::IInstaLODMaterial* UInstaLODMaterialMergeTool::GetBakeMaterial()
{
	if (!IsMeshOperationSuccessful())
		return nullptr;
	
	return OperationResult.MergeMaterial;
}

bool UInstaLODMaterialMergeTool::IsMeshOperationSuccessful() const
{
	return Operation != nullptr && OperationResult.Success;
}

void UInstaLODMaterialMergeTool::DeallocMeshOperation()
{
	check(Operation);
	GetInstaLODInterface()->GetInstaLOD()->DeallocMeshMergeOperation(Operation);
	Operation = nullptr;
}

void UInstaLODMaterialMergeTool::ResetSettings()
{
#if 0
	MaterialMergeMode = EInstaLODMaterialMergeMode::InstaLOD_AutoRepack;
#endif
	GutterSizeInPixels = 5;
	SuperSampling = EInstaLODSuperSampling::InstaLOD_X2;
	ShellRotation = EInstaLODShellRotation::InstaLOD_Arbitrary;
	bStackDuplicateShells = true;
	bInsertNormalSplits = true;
	bWorldspaceNormalizeShells = true;
	UVImportance = EInstaLODImportance::InstaLOD_Normal;
	GeometricImportance = EInstaLODImportance::InstaLOD_Normal;
	TextureImportance = EInstaLODImportance::InstaLOD_Normal;
	VisualImportance = EInstaLODImportance::InstaLOD_Normal;
	TextureCoordinateIndexInput = 0;

	bGenerateZeroAreaUV = false;
	ZeroAreaUVThreshold = 0.0f;

	bDeterministic = false;

	MaterialSettings = FInstaLODMaterialSettings();
	
	// Reset Parent which ultimately ends in a SaveConfig() call to reset everything
	Super::ResetSettings();
}

InstaLOD::MeshMergeSettings UInstaLODMaterialMergeTool::GetMaterialMergeSettings()
{
	InstaLOD::MeshMergeSettings Settings;
	
#if 0
	Settings.Mode = (InstaLOD::MeshMergeMode::Type)MaterialMergeMode;
#else
	Settings.Mode = InstaLOD::MeshMergeMode::AutoRepack;
#endif
	
	Settings.TextureFilter = (InstaLOD::TextureFilter::Type)MaterialSettings.TextureFilter; 
	Settings.SolidifyTexturePages = true;
	Settings.ComputeBinormalPerFragment = true;
	Settings.NormalizeTangentSpacePerFragment = false;
	Settings.GutterSizeInPixels = GutterSizeInPixels;
	Settings.ShellRotation = (InstaLOD::UVPackShellRotation::Type)ShellRotation;
	Settings.TexCoordIndexInput = TextureCoordinateIndexInput;

	Settings.StackDuplicateShells = bStackDuplicateShells;
	Settings.InsertNormalSplits = bInsertNormalSplits;
	Settings.WorldspaceNormalizeShells = bWorldspaceNormalizeShells;
	
	Settings.UVImportance = (InstaLOD::MeshFeatureImportance::Type)UVImportance;
	Settings.GeometricImportance = (InstaLOD::MeshFeatureImportance::Type)GeometricImportance;
	Settings.TextureImportance = (InstaLOD::MeshFeatureImportance::Type)TextureImportance;
	Settings.VisualImportance = (InstaLOD::MeshFeatureImportance::Type)VisualImportance;
	
	Settings.GenerateZeroAreaUV = bGenerateZeroAreaUV;
	Settings.ZeroAreaUVThreshold = ZeroAreaUVThreshold;

	Settings.Deterministic = bDeterministic;
	
	return Settings;
}

FMaterialProxySettings UInstaLODMaterialMergeTool::GetMaterialProxySettings() const
{
	FMaterialProxySettings MaterialProxySettings;
	
	MaterialProxySettings.TextureSize = MaterialSettings.TextureSize;
	MaterialProxySettings.TextureSizingType = (ETextureSizingType)(int)MaterialSettings.TextureSizingType;
	MaterialProxySettings.BlendMode = MaterialSettings.BlendMode;
	
	MaterialProxySettings.bNormalMap = MaterialSettings.bNormalMap;
	MaterialProxySettings.MetallicConstant = MaterialSettings.MetallicConstant;
	MaterialProxySettings.bMetallicMap = MaterialSettings.bMetallicMap;
	MaterialProxySettings.RoughnessConstant = MaterialSettings.RoughnessConstant;
	MaterialProxySettings.bRoughnessMap = MaterialSettings.bRoughnessMap;
	MaterialProxySettings.SpecularConstant = MaterialSettings.SpecularConstant;
	MaterialProxySettings.bSpecularMap = MaterialSettings.bSpecularMap;
	MaterialProxySettings.bEmissiveMap = MaterialSettings.bEmissiveMap;
	MaterialProxySettings.bOpacityMap = MaterialSettings.bOpacityMap;
	MaterialProxySettings.bOpacityMaskMap = MaterialSettings.bOpacityMaskMap;
	MaterialProxySettings.bAmbientOcclusionMap = MaterialSettings.bAmbientOcclusionMap;
	MaterialProxySettings.AmbientOcclusionConstant = MaterialSettings.AmbientOcclusionConstant;
	
	MaterialProxySettings.DiffuseTextureSize = MaterialSettings.DiffuseTextureSize;
	MaterialProxySettings.NormalTextureSize = MaterialSettings.NormalTextureSize;
	MaterialProxySettings.MetallicTextureSize = MaterialSettings.MetallicTextureSize;
	MaterialProxySettings.RoughnessTextureSize = MaterialSettings.RoughnessTextureSize;
	MaterialProxySettings.EmissiveTextureSize = MaterialSettings.EmissiveTextureSize;
	MaterialProxySettings.OpacityTextureSize = MaterialSettings.OpacityTextureSize;
	MaterialProxySettings.OpacityMaskTextureSize = MaterialSettings.OpacityMaskTextureSize;
	MaterialProxySettings.AmbientOcclusionTextureSize = MaterialSettings.AmbientOcclusionTextureSize;
	
	return MaterialProxySettings;
}

bool UInstaLODMaterialMergeTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!UInstaLODBaseTool::IsValidJSONObject(JsonObject, "MeshMerge"))
		return false;

	const TSharedPtr<FJsonObject>* SettingsObjectPointer = nullptr;

	if (!JsonObject->TryGetObjectField(FString("Settings"), SettingsObjectPointer) ||
		SettingsObjectPointer == nullptr ||
		!SettingsObjectPointer->IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: Could not retrieve Settings field."));
		return false;
	}

	const TSharedPtr<FJsonObject>& SettingsObject = *SettingsObjectPointer;

	if (SettingsObject->HasField("Mode"))
	{
		// NOTE: Transfer mode not supported in UE
		const FString Mode = SettingsObject->GetStringField("Mode");
		if (Mode.Equals("Transfer", ESearchCase::IgnoreCase))
			return true;
	}
	if (SettingsObject->HasField("GutterSizeInPixels"))
	{
		GutterSizeInPixels = SettingsObject->GetIntegerField("GutterSizeInPixels");
	}
	if (SettingsObject->HasField("GeometricImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("GeometricImportance");
		GeometricImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("TextureImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("TextureImportance");
		TextureImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("VisualImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("VisualImportance");
		VisualImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("UVImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("UVImportance");
		UVImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("GenerateZeroAreaUV"))
	{
		bGenerateZeroAreaUV = SettingsObject->GetBoolField("GenerateZeroAreaUV");
	}
	if (SettingsObject->HasField("ZeroAreaUVThreshold"))
	{
		ZeroAreaUVThreshold = SettingsObject->GetNumberField("ZeroAreaUVThreshold");
	}
	if (SettingsObject->HasField("ShellRotation"))
	{
		const FString ShellRotationValue = SettingsObject->GetStringField("ShellRotation");
		if (ShellRotationValue.Equals("Allow90", ESearchCase::IgnoreCase))
		{
			ShellRotation = EInstaLODShellRotation::InstaLOD_Allow90;
		}
		else if (ShellRotationValue.Equals("Arbitrary", ESearchCase::IgnoreCase))
		{
			ShellRotation = EInstaLODShellRotation::InstaLOD_Arbitrary;
		}
		else if (ShellRotationValue.Equals("None", ESearchCase::IgnoreCase))
		{
			ShellRotation = EInstaLODShellRotation::InstaLOD_None;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *ShellRotationValue, TEXT("ShellRotation"));
		}
	}
	if (SettingsObject->HasField("TexCoordIndexInput"))
	{
		TextureCoordinateIndexInput = SettingsObject->GetIntegerField("TexCoordIndexInput");
	}
	if (SettingsObject->HasField("TextureFilter"))
	{
		const FString Filter = SettingsObject->GetStringField("TextureFilter");
		if (Filter.Equals("Bilinear", ESearchCase::IgnoreCase))
		{
			MaterialSettings.TextureFilter = EInstaLODTextureFilter::InstaLOD_Bilinear;
		}
		else if (Filter.Equals("Bicubic", ESearchCase::IgnoreCase))
		{
			MaterialSettings.TextureFilter = EInstaLODTextureFilter::InstaLOD_Bicubic;
		}
		else if (Filter.Equals("Nearest", ESearchCase::IgnoreCase))
		{
			MaterialSettings.TextureFilter = EInstaLODTextureFilter::InstaLOD_Nearest;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
