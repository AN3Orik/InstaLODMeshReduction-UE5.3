/**
 * InstaLODMaterialSettings.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMaterialSettings.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "Tools/InstaLODBakeBaseTool.h"
#include "InstaLODMaterialSettings.generated.h"

UCLASS(BluePrintable, Config = InstaLOD)
class UFlattenMaterialSettings : public UObject
{
	GENERATED_BODY()

public:
	/** The output texture size. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint TextureSize = FIntPoint(2048, 2048);

	/** Determines how the texture size is specified. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Texture Sizing Type"), Category = "Material")
		EInstaLODTextureSizingType TextureSizingType = EInstaLODTextureSizingType::InstaLOD_UseSingleTextureSize;

	/** Determines the blend mode for the output material. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Blend Mode"), Category = "Material")
		TEnumAsByte<EBlendMode> BlendMode = BLEND_Opaque;

	/** The texture filtering used when sampling textures. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Texture Filter"), Category = "Material")
		EInstaLODTextureFilter TextureFilter = EInstaLODTextureFilter::InstaLOD_Bilinear;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Normal Map"), Category = "Material")
		bool bNormalMap = true;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Metallic Map"), Category = "Material")
		bool bMetallicMap = false;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Metallic Constant", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bMetallicMap"), Category = "Material")
		float MetallicConstant = 0.0f;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Roughness Map"), Category = "Material")
		bool bRoughnessMap = false;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Roughness Constant", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bRoughnessMap"), Category = "Material")
		float RoughnessConstant = 0.5f;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Specular Map"), Category = "Material")
		bool bSpecularMap = false;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Specular Constant", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bSpecularMap"), Category = "Material")
		float SpecularConstant = 0.5f;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Emissive Map"), Category = "Material")
		bool bEmissiveMap = false;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Opacity Map"), Category = "Material")
		bool bOpacityMap = false;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Opacity Constant", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bOpacityMap"), Category = "Material")
		float OpacityConstant = 1.0f;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Opacity MaskMap"), Category = "Material")
		bool bOpacityMaskMap = false;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Opacity Mask Constant", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bOpacityMaskMap"), Category = "Material")
		float OpacityMaskConstant = 1.0f;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Ambient Occlusion Map"), Category = "Material")
		bool bAmbientOcclusionMap = false;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Ambient Occlusion Constant", ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bAmbientOcclusionMap"), Category = "Material")
		float AmbientOcclusionConstant = 1.0f;

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Diffuse Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint DiffuseTextureSize = FIntPoint(2048, 2048);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Normal Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint NormalTextureSize = FIntPoint(2048, 2048);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Metallic Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint MetallicTextureSize = FIntPoint(1024, 1024);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Roughness Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint RoughnessTextureSize = FIntPoint(1024, 1024);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Specular Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint SpecularTextureSize = FIntPoint(1024, 1024);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Emissive Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint EmissiveTextureSize = FIntPoint(1024, 1024);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Opacity Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint OpacityTextureSize = FIntPoint(1024, 1024);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Opacity Mask Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint OpacityMaskTextureSize = FIntPoint(1024, 1024);

	UPROPERTY(BlueprintReadWrite, Config, AdvancedDisplay, EditAnywhere, meta = (DisplayName = "Ambient Occlusion Texture Size", ClampMin = "1", UIMin = "1"), Category = "Material")
		FIntPoint AmbientOcclusionTextureSize = FIntPoint(1024, 1024);

	FMaterialProxySettings GetMaterialProxySettings()
	{
		FMaterialProxySettings MaterialProxySettings;

		MaterialProxySettings.TextureSize = TextureSize;
		MaterialProxySettings.TextureSizingType = (ETextureSizingType)TextureSizingType;
		MaterialProxySettings.BlendMode = BlendMode;

		MaterialProxySettings.bNormalMap = bNormalMap;
		MaterialProxySettings.MetallicConstant = MetallicConstant;
		MaterialProxySettings.bMetallicMap = bMetallicMap;
		MaterialProxySettings.RoughnessConstant = RoughnessConstant;
		MaterialProxySettings.bRoughnessMap = bRoughnessMap;
		MaterialProxySettings.SpecularConstant = SpecularConstant;
		MaterialProxySettings.bSpecularMap = bSpecularMap;
		MaterialProxySettings.bEmissiveMap = bEmissiveMap;
		MaterialProxySettings.bOpacityMap = bOpacityMap;
		MaterialProxySettings.bOpacityMaskMap = bOpacityMaskMap;
		MaterialProxySettings.bAmbientOcclusionMap = bAmbientOcclusionMap;
		MaterialProxySettings.AmbientOcclusionConstant = AmbientOcclusionConstant;

		MaterialProxySettings.DiffuseTextureSize = DiffuseTextureSize;
		MaterialProxySettings.NormalTextureSize = NormalTextureSize;
		MaterialProxySettings.MetallicTextureSize = MetallicTextureSize;
		MaterialProxySettings.RoughnessTextureSize = RoughnessTextureSize;
		MaterialProxySettings.EmissiveTextureSize = EmissiveTextureSize;
		MaterialProxySettings.OpacityTextureSize = OpacityTextureSize;
		MaterialProxySettings.OpacityMaskTextureSize = OpacityMaskTextureSize;
		MaterialProxySettings.AmbientOcclusionTextureSize = AmbientOcclusionTextureSize;

		return MaterialProxySettings;
	}
};

UCLASS(BluePrintable, Config = InstaLOD)
class UInstaLODBakeOutputSettings : public UObject
{
	GENERATED_BODY()

public:

	UInstaLODBakeOutputSettings() : UObject()
	{
		FlattenMaterialSettings = NewObject<UFlattenMaterialSettings>();
		SavePath.Path = TEXT("InstaLOD_Script_Output");
	}

	/************************************************************************/
	/* Bake Output                                                          */
	/************************************************************************/
	
	/** Enables super sampling of texture pages. NOTE: super sampling causes an exponential increase in memory usage and processing time. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Super Sampling"), Category = "Bake Output")
		EInstaLODSuperSampling SuperSampling = EInstaLODSuperSampling::InstaLOD_X2;

	/** Enables solidification of texture pages to avoid pixel bleed. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Solidify Texture Pages"), Category = "Bake Output")
		bool bSolidifyTexturePages = true;

	/** Alpha mask values equal or below this threshold will be considered transparent. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Alpha Mask Threshold", NoSpinbox, UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f), Category = "Bake Output")
		float AlphaMaskThreshold = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Material Settings"), Category = "Bake Output")
		UFlattenMaterialSettings* FlattenMaterialSettings;

	/** Output Normal Map object space. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Object-Space Normals"), Category = "Bake Texture Pages")
		bool bBakeTexturePageNormalObjectSpace = false;

	/** Output Mesh-ID Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Mesh ID"), Category = "Bake Texture Pages")
		bool bBakeTexturePageMeshID = false;

	/** Output Vertex-Colors Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Vertex Colors"), Category = "Bake Texture Pages")
		bool bBakeTexturePageVertexColor = false;

	/** Output Ambient-Occlusion Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Ambient Occlusion"), Category = "Bake Texture Pages")
		bool bBakeTexturePageAmbientOcclusion = false;

	/** Output Bent-Normals Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Bent Normals"), Category = "Bake Texture Pages")
		bool bBakeTexturePageBentNormals = false;

	/** Output Thickness Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Thickness"), Category = "Bake Texture Pages")
		bool bBakeTexturePageThickness = false;

	/** Output Displacement Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Displacement"), Category = "Bake Texture Pages")
		bool bBakeTexturePageDisplacement = false;

	/** Output Position Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Position"), Category = "Bake Texture Pages")
		bool bBakeTexturePagePosition = false;

	/** Normalizes the Position Map by AABB. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Normalize Position Map by AABB", EditCondition = "bBakeTexturePagePosition"), Category = "Bake Texture Pages")
		bool bBakeTexturePagePositionNormalizeAABB = true;

	/** Output Curvature Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Curvature"), Category = "Bake Texture Pages")
		bool bBakeTexturePageCurvature = false;

	/** Output Opacity Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Opacity"), Category = "Bake Texture Pages")
		bool bBakeTexturePageOpacity = false;

	/** Output Reflectance Map. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Reflectance"), Category = "Bake Texture Pages")
		bool bBakeTexturePageReflectance = false;

	/** Output path for material and textures.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Bake Output Save Path", RelativeToGameContentDir), Category = "Settings")
		FDirectoryPath SavePath;

	/** The amount of sub rays to cast for each pixel. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Ambient Occlusion Sample Count", ClampMin = 16, ClampMax = 128), Category = "Settings")
		int32 AmbientOcclusionSampleCount = 32;

	/** The amount of sub rays to cast for each pixel. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Thickness Sample Count", ClampMin = 16, ClampMax = 128), Category = "Settings")
		int32 ThicknessSampleCount = 32;

	/** Enable to automatically delight all output color textures. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Auto Delight Color Output"), Category = "Settings")
		bool bEnableAutoDelight = false;

	UFUNCTION(BlueprintCallable, Category = "Setting")
	UFlattenMaterialSettings* GetFlattenMaterialSettings()
	{
		check(FlattenMaterialSettings != nullptr);

		return FlattenMaterialSettings;
	}

	InstaLOD::BakeOutputSettings GetBakeOutputSettings()
	{
		check(FlattenMaterialSettings != nullptr);

		InstaLOD::BakeOutputSettings BakeOutput;

		// UE specific settings
		BakeOutput.TangentSpaceFormat = InstaLOD::MeshFormat::OpenGL;
		BakeOutput.ComputeBinormalPerFragment = true;
		BakeOutput.NormalizeTangentSpacePerFragment = false;

		// convert our settings to InstaLOD
		BakeOutput.SuperSampling = (InstaLOD::SuperSampling::Type)SuperSampling;
		BakeOutput.SolidifyTexturePages = bSolidifyTexturePages;

		BakeOutput.SourceMeshUVChannelIndex = 0;

		BakeOutput.TextureFilter = (InstaLOD::TextureFilter::Type)FlattenMaterialSettings->TextureFilter;

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
		BakeOutput.AutoDelightColorOutput = bEnableAutoDelight;

		BakeOutput.AmbientOcclusion.SampleCount = ThicknessSampleCount;
		BakeOutput.Thickness.SampleCount = ThicknessSampleCount;

		return BakeOutput;
	}
};