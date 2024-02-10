/**
 * InstaLODBakeBaseTool.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODBakeBaseTool.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODBakeBaseTool.h"
#include "InstaLODUIPCH.h"

void UInstaLODBakeBaseTool::ResetSettings()
{
#if 0
	SourceMeshUVSetIndex = 0;
#endif
	SuperSampling = EInstaLODSuperSampling::InstaLOD_X2;
	bSolidifyTexturePages = true;
	AlphaMaskThreshold = 0.5f;
	MaterialSettings = FInstaLODMaterialSettings();

	bBakeTexturePagePositionNormalizeAABB = true;
	bBakeTexturePageNormalObjectSpace = false;
	bBakeTexturePageMeshID = false;
	bBakeTexturePageVertexColor = false;
	bBakeTexturePageAmbientOcclusion = false;
	bBakeTexturePageBentNormals = false;
	bBakeTexturePageThickness = false;
	bBakeTexturePageDisplacement = false;
	bBakeTexturePagePosition = false;
	bBakeTexturePageCurvature = false;
	bBakeTexturePageOpacity = false;

	// Reset Parent which ultimately ends in a SaveConfig() call to reset everything
	Super::ResetSettings();
}

InstaLOD::BakeOutputSettings UInstaLODBakeBaseTool::GetBakeOutputSettings() const
{
	InstaLOD::BakeOutputSettings BakeOutput;
	
	// UE specific settings
	BakeOutput.TangentSpaceFormat = InstaLOD::MeshFormat::OpenGL;
	BakeOutput.ComputeBinormalPerFragment = true;
	BakeOutput.NormalizeTangentSpacePerFragment = false;
	
	// convert our settings to InstaLOD
	BakeOutput.SuperSampling = (InstaLOD::SuperSampling::Type)SuperSampling;
	BakeOutput.SolidifyTexturePages = bSolidifyTexturePages;
	BakeOutput.SourceMeshUVChannelIndex = 0;
	BakeOutput.TextureFilter = (InstaLOD::TextureFilter::Type)MaterialSettings.TextureFilter;

	BakeOutput.NormalizePositionByAABB = bBakeTexturePagePositionNormalizeAABB;
	
    BakeOutput.TexturePageNormalTangentSpace = true;
	BakeOutput.TexturePageNormalObjectSpace = bBakeTexturePageNormalObjectSpace;
	BakeOutput.TexturePageMeshID = bBakeTexturePageMeshID;
	BakeOutput.TexturePageVertexColor = bBakeTexturePageVertexColor;
	BakeOutput.TexturePageAmbientOcclusion = bBakeTexturePageAmbientOcclusion;
	BakeOutput.TexturePageBentNormals = bBakeTexturePageBentNormals;
	BakeOutput.TexturePageThickness = bBakeTexturePageThickness;
	BakeOutput.TexturePageDisplacement = bBakeTexturePageDisplacement;
	BakeOutput.TexturePagePosition = bBakeTexturePagePosition;
	BakeOutput.TexturePageCurvature = bBakeTexturePageCurvature;
	BakeOutput.TexturePageTransfer = false;
	BakeOutput.TexturePageOpacity = bBakeTexturePageOpacity;
	BakeOutput.TexturePageReflectance = bBakeTexturePageReflectance;

	return BakeOutput;
}

bool UInstaLODBakeBaseTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!JsonObject.IsValid())
		return false;

	if (!JsonObject->HasField("Settings"))
		return false;
	
	const TSharedPtr<FJsonObject>* SettingsObject = nullptr;

	if (!JsonObject->TryGetObjectField(FString("Settings"), SettingsObject) ||
		SettingsObject == nullptr ||
		!SettingsObject->IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: Could not retrieve Settings field."));
		return false;
	}

	const TSharedPtr<FJsonObject>* BakeOutputSettingsPointer = nullptr;	

	if (!(*SettingsObject)->TryGetObjectField("BakeOutputSettings", BakeOutputSettingsPointer) ||
		BakeOutputSettingsPointer == nullptr ||
		!BakeOutputSettingsPointer->IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: Could not retrieve BakeOutputSettings field."));
		return false;
	}

	const TSharedPtr<FJsonObject>& BakeOutputSettings = *BakeOutputSettingsPointer;

	if (BakeOutputSettings->HasField("TexturePageAmbientOcclusion"))
	{
		bBakeTexturePageAmbientOcclusion = BakeOutputSettings->GetBoolField("TexturePageAmbientOcclusion");
	}
	if (BakeOutputSettings->HasField("TexturePageBentNormals"))
	{
		bBakeTexturePageBentNormals = BakeOutputSettings->GetBoolField("TexturePageBentNormals");
	}
	if (BakeOutputSettings->HasField("TexturePageCurvature"))
	{
		bBakeTexturePageCurvature = BakeOutputSettings->GetBoolField("TexturePageCurvature");
	} 
	if (BakeOutputSettings->HasField("TexturePageDisplacement"))
	{
		bBakeTexturePageDisplacement = BakeOutputSettings->GetBoolField("TexturePageDisplacement");
	}
	if (BakeOutputSettings->HasField("TexturePageMeshID"))
	{
		bBakeTexturePageMeshID = BakeOutputSettings->GetBoolField("TexturePageMeshID");
	}
	if (BakeOutputSettings->HasField("TexturePageNormalObjectSpace"))
	{
		bBakeTexturePageNormalObjectSpace = BakeOutputSettings->GetBoolField("TexturePageNormalObjectSpace");
	}
	if (BakeOutputSettings->HasField("TexturePageOpacity"))
	{
		bBakeTexturePageOpacity = BakeOutputSettings->GetBoolField("TexturePageOpacity");
	}
	if (BakeOutputSettings->HasField("TexturePagePosition"))
	{
		bBakeTexturePagePosition = BakeOutputSettings->GetBoolField("TexturePagePosition");
	}
	if (BakeOutputSettings->HasField("TexturePageReflectance"))
	{
		bBakeTexturePageReflectance = BakeOutputSettings->GetBoolField("TexturePageReflectance");
	}
	if (BakeOutputSettings->HasField("TexturePageThickness"))
	{
		bBakeTexturePageThickness = BakeOutputSettings->GetBoolField("TexturePageThickness");
	}
	if (BakeOutputSettings->HasField("TexturePageVertexColor"))
	{
		bBakeTexturePageVertexColor = BakeOutputSettings->GetBoolField("TexturePageVertexColor");
	}
	if (BakeOutputSettings->HasField("SolidifyTexturePages"))
	{
		bSolidifyTexturePages = BakeOutputSettings->GetBoolField("SolidifyTexturePages");
	}
	if (BakeOutputSettings->HasField("NormalizePositionByAABB"))
	{
		bBakeTexturePagePositionNormalizeAABB = BakeOutputSettings->GetBoolField("NormalizePositionByAABB");
	}
	if (BakeOutputSettings->HasField("SuperSampling"))
	{
		const FString Sampling = BakeOutputSettings->GetStringField("SuperSampling"); 

		SuperSampling = EInstaLODSuperSampling::InstaLOD_None;

		if (Sampling.Equals("x2", ESearchCase::IgnoreCase))
		{
			SuperSampling = EInstaLODSuperSampling::InstaLOD_X2;
		}
		else if (Sampling.Equals("x2", ESearchCase::IgnoreCase) ||
			Sampling.Equals("x4", ESearchCase::IgnoreCase) ||
			Sampling.Equals("x8", ESearchCase::IgnoreCase))
		{
			SuperSampling = EInstaLODSuperSampling::InstaLOD_X4;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *Sampling, TEXT("SuperSampling"));
		}
	}	
	if (BakeOutputSettings->HasField("AlphaMaskThreshold"))
	{
		AlphaMaskThreshold = (float) BakeOutputSettings->GetNumberField("AlphaMaskThreshold");
	}

	return true;
}

FMaterialProxySettings UInstaLODBakeBaseTool::GetMaterialProxySettings() const
{
	FMaterialProxySettings MaterialProxySettings;

	MaterialProxySettings.TextureSize = MaterialSettings.TextureSize;
	MaterialProxySettings.TextureSizingType = (ETextureSizingType)MaterialSettings.TextureSizingType;
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
