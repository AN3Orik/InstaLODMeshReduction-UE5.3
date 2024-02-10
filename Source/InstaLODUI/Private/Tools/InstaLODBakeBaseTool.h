/**
 * InstaLODBakeBaseTool.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODBakeBaseTool.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "InstaLODBakeBaseTool.generated.h"


UENUM()
enum class EInstaLODTextureSizingType : uint8
{
	InstaLOD_UseSingleTextureSize UMETA(DisplayName = "Use TextureSize for all material properties"),
	InstaLOD_UseAutomaticBiasedSizes UMETA(DisplayName = "Use automatically biased texture sizes based on TextureSize"),
	InstaLOD_UseManualOverrideTextureSize UMETA(DisplayName = "Use per property manually overriden texture sizes")
};

USTRUCT()
struct FInstaLODMaterialSettings
{
	GENERATED_USTRUCT_BODY()

	/** The output texture size. */
	UPROPERTY(Category = Material, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint TextureSize;

	/** Determines how the texture size is specified. */
	UPROPERTY(Category = Material, EditAnywhere)
	EInstaLODTextureSizingType TextureSizingType;

	/** Determines the blend mode for the output material. */
	UPROPERTY(Category = Material, EditAnywhere)
	TEnumAsByte<EBlendMode> BlendMode;
	
	/** The texture filtering used when sampling textures. */
	UPROPERTY(Category = Material, EditAnywhere)
	EInstaLODTextureFilter TextureFilter;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bNormalMap;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bMetallicMap;

	UPROPERTY(Category = Material, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bMetallicMap"))
	float MetallicConstant;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bRoughnessMap;

	UPROPERTY(Category = Material, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bRoughnessMap"))
	float RoughnessConstant;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bSpecularMap;

	UPROPERTY(Category = Material, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bSpecularMap"))
	float SpecularConstant;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bEmissiveMap;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bOpacityMap;

	UPROPERTY(Category = Material, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bOpacityMap"))
	float OpacityConstant;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bOpacityMaskMap;

	UPROPERTY(Category = Material, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bOpacityMaskMap"))
	float OpacityMaskConstant;

	UPROPERTY(Category = Material, EditAnywhere)
	bool bAmbientOcclusionMap;

	UPROPERTY(Category = Material, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", editcondition = "!bAmbientOcclusionMap"))
	float AmbientOcclusionConstant;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint DiffuseTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint NormalTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint MetallicTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint RoughnessTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint SpecularTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint EmissiveTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint OpacityTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint OpacityMaskTextureSize;

	UPROPERTY(Category = Material, AdvancedDisplay, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint AmbientOcclusionTextureSize;

	FInstaLODMaterialSettings() :
	TextureSize(2048, 2048),
	TextureSizingType(EInstaLODTextureSizingType::InstaLOD_UseSingleTextureSize),
	BlendMode(BLEND_Opaque),
	TextureFilter(EInstaLODTextureFilter::InstaLOD_Bilinear),
	bNormalMap(true),
	bMetallicMap(false),
	MetallicConstant(0.0f),
	bRoughnessMap(false),
	RoughnessConstant(0.5f),
	bSpecularMap(false),
	SpecularConstant(0.5f),
	bEmissiveMap(false),
	bOpacityMap(false),
	OpacityConstant(1.0f),
	bOpacityMaskMap(false),
	OpacityMaskConstant(1.0f),
	bAmbientOcclusionMap(false),
	AmbientOcclusionConstant(1.0f),
	DiffuseTextureSize(2048, 2048),
	NormalTextureSize(2048, 2048),
	MetallicTextureSize(1024, 1024),
	RoughnessTextureSize(1024, 1024),
	SpecularTextureSize(1024, 1024),
	EmissiveTextureSize(1024, 1024),
	OpacityTextureSize(1024, 1024),
	OpacityMaskTextureSize(1024, 1024),
	AmbientOcclusionTextureSize(1024, 1024)
	{
	}
};


/**
 * Additional BaseTool Class for Tools that feature Baking options.
 */
UCLASS(Config=InstaLOD, Abstract)
class INSTALODUI_API UInstaLODBakeBaseTool : public UInstaLODBaseTool
{
	GENERATED_BODY()
	
public:

	/************************************************************************/
	/* Bake Output                                                          */
	/************************************************************************/	
	
#if 0
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Source Mesh UV Set Index", ClampMin = 0, ClampMax = 7), Category = "Bake Output")
	int32 SourceMeshUVSetIndex = 0;
#endif

	/** Enables super sampling of texture pages. NOTE: super sampling causes an exponential increase in memory usage and processing time. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Super Sampling"), Category = "Bake Output")
	EInstaLODSuperSampling SuperSampling = EInstaLODSuperSampling::InstaLOD_X2;

	/** Enables solidification of texture pages to avoid pixel bleed. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Solidify Texture Pages"), Category = "Bake Output")
	bool bSolidifyTexturePages = true;
	
	/** Alpha mask values equal or below this threshold will be considered transparent. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Alpha Mask Threshold", NoSpinbox, UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f), Category = "Bake Output")
	float AlphaMaskThreshold = 0.5f;

	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Material Settings"), Category = "Bake Output")
	FInstaLODMaterialSettings MaterialSettings;
	
	/** Output Normal Map object space. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Object-Space Normals"), Category = "Bake Texture Pages")
	bool bBakeTexturePageNormalObjectSpace = false;
	/** Output Mesh-ID Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Mesh ID"), Category = "Bake Texture Pages")
	bool bBakeTexturePageMeshID = false;
	/** Output Vertex-Colors Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Vertex Colors"), Category = "Bake Texture Pages")
	bool bBakeTexturePageVertexColor = false;
	/** Output Ambient-Occlusion Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Ambient Occlusion"), Category = "Bake Texture Pages")
	bool bBakeTexturePageAmbientOcclusion = false;
	/** Output Bent-Normals Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Bent Normals"), Category = "Bake Texture Pages")
	bool bBakeTexturePageBentNormals = false;
	/** Output Thickness Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Thickness"), Category = "Bake Texture Pages")
	bool bBakeTexturePageThickness = false;
	/** Output Displacement Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Displacement"), Category = "Bake Texture Pages")
	bool bBakeTexturePageDisplacement = false;
	/** Output Position Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Position"), Category = "Bake Texture Pages")
	bool bBakeTexturePagePosition = false;
	/** Normalizes the Position Map by AABB */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Normalize Position Map by AABB", EditCondition="bBakeTexturePagePosition", EditConditionHides), Category = "Bake Texture Pages")
	bool bBakeTexturePagePositionNormalizeAABB = true;
	/** Output Curvature Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Curvature"), Category = "Bake Texture Pages")
	bool bBakeTexturePageCurvature = false;
	/** Output Opacity Map. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Opacity"), Category = "Bake Texture Pages")
	bool bBakeTexturePageOpacity = false;
	/** Output Reflectance Map */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Reflectance"), Category = "Bake Texture Pages")
	bool bBakeTexturePageReflectance = false;
	
	/// FUNCTIONS ///
	virtual InstaLOD::BakeOutputSettings GetBakeOutputSettings() const;

	/** Start - UInstaLODBaseTool Interface */
	virtual void ResetSettings() override;
	virtual FMaterialProxySettings GetMaterialProxySettings() const override;
	virtual bool ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject) override;
	/** End - UInstaLODBaseTool Interface */
};

