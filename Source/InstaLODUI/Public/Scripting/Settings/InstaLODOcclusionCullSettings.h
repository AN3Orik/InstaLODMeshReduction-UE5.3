/**
 * InstaLODOcclusionCullSettings.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODOcclusionCullSettings.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once
#include "CoreMinimal.h"

#include "Tools/InstaLODOcclusionCullTool.h"
#include "InstaLODOcclusionCullSettings.generated.h"

UCLASS(Config = InstaLOD, BluePrintable)
class UInstaLODOcclusionCullSettings : public UObject
{
	GENERATED_BODY()

public:

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

	/** Automatic Interior: Automatically remove interior surfaces. This mode is designed to operate on individual objects, it is less suited for large scenes. Camera Based: Cameras added to the operation will be used to determine ovvluded surfaces. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Occlusion Cull Mode"), Category = "Settings")
		EInstaLODOcclusionCullMode OcclusionCullMode = EInstaLODOcclusionCullMode::InstaLOD_AutomaticInterior;

	/** Specifies what kind of geometry will be removed. Per Face: Cull occluded individual faces. Per Mesh: Cull occluded submeshes, submeshes based on individual meshes added to the operation. Per Face Adjacency: Cull occluded submeshes, submeshes automatically derived from vertex adjacency. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Culling Strategy"), Category = "Settings")
		EInstaLODOcclusionCullingStrategy CullingStrategy = EInstaLODOcclusionCullingStrategy::InstaLOD_Face;

	/** Specifies how the occlusion data will be used. Remove Geometry: Remove occluded geometry. Write To Vertex Colors: Write occlusion data into vertex colors set 0. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Data Usage"), Category = "Settings")
		EInstaLODOcclusionCullDataUsage DataUsage = EInstaLODOcclusionCullDataUsage::InstaLOD_RemoveGeometry;

	/** When 'Mode' is set to 'AutomaticInterior': the precision of the automatic interior culling. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Automatic Precision"), Category = "Settings")
		EInstaLODImportance AutomaticPrecision = EInstaLODImportance::InstaLOD_Normal;

	/** The resolution of the rasterization. Higher resolutions improve quality by reducing the amount of missed faces due to subpixel inaccuracies. Dense geometry benefits from a higher resolution. However, a low resolution can be accommodated by increasing the 'Adjacency Depth' or the 'Precision'. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Resolution", ClampMin = 512, ClampMax = 16384), Category = "Settings")
		int32 Resolution = 1024;

	/** The precision of the automatic interior culling. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Precision"), Category = "Settings")
		EInstaLODImportance Precision = EInstaLODImportance::InstaLOD_Normal;

	/** When 'Culling Strategy' is set to 'Per Face': the recursion depth when protecting neighbors of visible faces. This can be used to prevent holes in the outside geometry that occurred due to a low resolution or a low precision. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Adjacency Depth", ClampMin = 0, ClampMax = 32), Category = "Settings")
		int32 AdjacencyDepth = 5;

	InstaLOD::OcclusionCullSettings GetOcclusionCullSettings()
	{
		InstaLOD::OcclusionCullSettings Settings;

		Settings.Mode = (InstaLOD::OcclusionCullMode::Type)OcclusionCullMode;
		Settings.AutomaticPrecision = (InstaLOD::MeshFeatureImportance::Type)AutomaticPrecision;
		Settings.CullingStrategy = (InstaLOD::OcclusionCullingStrategy::Type)CullingStrategy;
		Settings.DataUsage = (InstaLOD::OcclusionCullDataUsage::Type)DataUsage;
		Settings.Resolution = Resolution;
		Settings.AutomaticPrecision = (InstaLOD::MeshFeatureImportance::Type)Precision;
		Settings.AdjacencyDepth = AdjacencyDepth;

		return Settings;
	}
};

