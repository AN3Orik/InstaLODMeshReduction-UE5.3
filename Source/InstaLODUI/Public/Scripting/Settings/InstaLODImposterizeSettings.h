/**
 * InstaLODImposterizeSettings.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODImposterizeSettings.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODImposterizeTool.h"
#include "InstaLODImposterizeSettings.generated.h"

UCLASS(BluePrintable, Config = InstaLOD)
class UInstaLODImposterizeSettings : public UObject
{
	GENERATED_BODY()

public:

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

	/** Determines the type of the imposter. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Type"), Category = "Settings")
		EInstaLODImposterizeType ImposterizeType = EInstaLODImposterizeType::InstaLOD_Billboard;
	/** The minimum distance between two UV shells in pixels. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Gutter Size In Pixels", ClampMin = 1, ClampMax = 64), Category = "Settings")
		int32 GutterSizeInPixels = 2;

	/************************************************************************/
	/* Billboard Settings                                                   */
	/************************************************************************/

	/** The amount of quads to spawn for the XY axis of the input geometry AABB. Additional quads will be rotated by 180 deg/count along the Y axis. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Billboard", EditConditionhides, DisplayName = "XY Axis Quads", ClampMin = 0, ClampMax = 16), Category = "Billboard Settings")
		int32 XYAxisQuads_Billboard = 0;

	/** The amount of quads to spawn for the XZ axis of the input geometry AABB. Additional quads will be rotated by 180 deg/count along the Z axis. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Billboard", EditConditionhides, DisplayName = "XZ Axis Quads", ClampMin = 0, ClampMax = 16), Category = "Billboard Settings")
		int32 XZAxisQuads_Billboard = 1;

	/** The amount of quads to spawn for the YZ axis of the input geometry AABB. Additional quads will be rotated by 180 deg/count along the Z axis. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Billboard", EditConditionhides, DisplayName = "YZ Axis Quads", ClampMin = 0, ClampMax = 16), Category = "Billboard Settings")
		int32 YZAxisQuads_Billboard = 1;

	/** If enabled, quads will generate polygons for each side of a quad. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Billboard", EditConditionhides, DisplayName = "Two Sided Quads"), Category = "Billboard Settings")
		bool bTwoSidedQuads_Billboard = true;

	/** Subdivisions along the U axis for billboard imposters. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Billboard", EditConditionhides, DisplayName = "Subdivisions U", ClampMin = 1, ClampMax = 16), Category = "Billboard Settings")
		int32 SubdivisionsU = 1;

	/** Subdivisions along the V axis for billboard imposters. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Billboard", EditConditionhides, DisplayName = "Subdivisions V", ClampMin = 1, ClampMax = 16), Category = "Billboard Settings")
		int32 SubdivisionsV = 1;

	/************************************************************************/
	/* AABB Settings                                                        */
	/************************************************************************/

	/** AABB imposter faces will be displaced along the face normal by the specified value in world space units. This is useful to create hay or headges with overhanging faces. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_AABB", EditConditionhides, DisplayName = "Displacement", ClampMin = -50, ClampMax = 50), Category = "AABB Settings")
		int32 AABB_Displacement = 0;

	/************************************************************************/
	/* Vista Settings                                                        */
	/************************************************************************/

	/** The distance of the imposter plane from the camera.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Vista", EditConditionhides, DisplayName = "Plane Distance", ClampMin = 0.0, ClampMax = 1.0), Category = "Vista Settings")
		float VistaDistance = 1.0;

	/** If enabled, the imposter plane will paralled with the camera. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Vista", EditConditionhides, DisplayName = "Parallels Imposter Plane With Camera"), Category = "Vista Settings")
		bool bParallelImposterPlane = true;

	/************************************************************************/
	/* Hybrid Billboard Cloud Settings                                      */
	/************************************************************************/

	/** The maximum amount of faces to spawn for cloud imposter. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "Maximum Face Count", ClampMin = 4, ClampMax = 10000), Category = "Hybrid Billboard Cloud Settings")
		int32 MaximumFaceCount = 700;

	/** The amount of faces in percentage of the 'Maximum Face Count' to allocate for the polygon part of hybrid billboard cloud imposters. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "Hybrid Mesh Face Factor", UIMin = 0.0f, UIMax = 50.0f, ClampMin = 0.0f, ClampMax = 50.0f), Category = "Hybrid Billboard Cloud Settings")
		float HybridMeshFaceFactor = 50.0f;

	/** Spawn XY axis quads. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "XY Axis Quads"), Category = "Hybrid Billboard Cloud Settings")
		bool bXYAxisQuads = false;

	/** Spawn XZ axis quads. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "XZ Axis Quads"), Category = "Hybrid Billboard Cloud Settings")
		bool bXZAxisQuads = false;

	/** Spawn YZ axis quads. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "YZ Axis Quads"), Category = "Hybrid Billboard Cloud Settings")
		bool bYZAxisQuads = true;

	/** If enabled, quads will generate polgons for each side of a quad. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "Two Sided Quads"), Category = "Hybrid Billboard Cloud Settings")
		bool bTwoSidedQuads_Hybrid = true;

	/** Material names matching the specified suffix will be used for polygonal mesh output when creating a hybrid billboard cloud imposter. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "Hybrid Cloud Poly Material Suffix"), Category = "Hybrid Billboard Cloud Settings")
		FString HybridCloudPolyMaterialSuffix = "_cloudpoly";

	/** Defines the target normal for the hybrid billboard cloud imposter sprites. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "Cloud Normal", UIMin = -1.0f, UIMax = 1.0f, ClampMin = -1.0f, ClampMax = 1.0f), Category = "Hybrid Billboard Cloud Settings")
		FVector CloudNormal = FVector(0.35f, 0.4f, 0.8f);

	/** Defines how much the normals of the hybrid billboard cloud sprites align to the target normal. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud", EditConditionhides, DisplayName = "Cloud Normal Conformity", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f), Category = "Hybrid Billboard Cloud Settings")
		float CloudNormalConformity = 0.5f;

	/************************************************************************/
	/* Flipbook	Settings							                        */
	/************************************************************************/

	/** The amount of flipbook frames to generate for each rotation axis. NOTE: by default flipbook's top left frame is projected along the negative Z axis. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (EditCondition = "ImposterizeType == EInstaLODImposterizeType::InstaLOD_Flipbook", EditConditionhides, DisplayName = "Flipbook Frames Per Axis", ClampMin = 1, ClampMax = 64), Category = "Flipbook Settings")
		int32 FlipbookFramesPerAxis = 8;

	/************************************************************************/
	/* Alpha Cutout Settings												*/
	/************************************************************************/

	/** Enable AlphaCutOut */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Enable"), Category = "Alpha Cutout (Preview)")
		bool bIsAlphaCutOutEnabled = false;

	/** Enable Subdivision */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Subdivide"), Category = "Alpha Cutout (Preview)")
		bool bIsAlphaCutOutSubdivideEnabled = false;

	/** Resolution for Subdivision */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Resolution", ClampMin = 1, ClampMax = 64), Category = "Alpha Cutout (Preview)")
		int32 AlphaCutOutResolution = 16;

	/************************************************************************/
	/* Advanced							                                    */
	/************************************************************************/

	/** Makes the algorithm deterministic at the cost of speed. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Deterministic"), Category = "Advanced")
		bool bDeterministic = false;


	InstaLOD::ImposterizeSettings GetImposterizeSettings()
	{
		InstaLOD::ImposterizeSettings Settings;

		Settings.Type = (InstaLOD::ImposterType::Type)ImposterizeType;
		Settings.FlipbookFramesPerAxis = FlipbookFramesPerAxis;
		Settings.CustomGeometry = nullptr;
		Settings.AABBDisplacement = AABB_Displacement;
		Settings.QuadXYCount = XYAxisQuads_Billboard;
		Settings.QuadXZCount = XZAxisQuads_Billboard;
		Settings.QuadYZCount = YZAxisQuads_Billboard;
		Settings.EnableQuadTwoSided = bTwoSidedQuads_Billboard;
		Settings.QuadSubdivisionsU = SubdivisionsU;
		Settings.QuadSubdivisionsV = SubdivisionsV;

		Settings.CloudFaceCount = MaximumFaceCount;
		Settings.CloudPolyFaceFactor = HybridMeshFaceFactor / 100.0f;
		Settings.EnableCloudQuadXY = bXYAxisQuads;
		Settings.EnableCloudQuadXZ = bXZAxisQuads;
		Settings.EnableCloudQuadYZ = bYZAxisQuads;
		Settings.EnableCloudQuadTwoSided = bTwoSidedQuads_Hybrid;
		Settings.CloudNormal = InstaLOD::InstaVec3F(CloudNormal.X, CloudNormal.Y, CloudNormal.Z);
		Settings.CloudNormalConformity = CloudNormalConformity;

		Settings.AlphaCutOut = bIsAlphaCutOutEnabled;
		Settings.AlphaCutOutSubdivide = bIsAlphaCutOutSubdivideEnabled;
		Settings.AlphaCutOutResolution = AlphaCutOutResolution;
		Settings.GutterSizeInPixels = GutterSizeInPixels;
		Settings.BakeEngine = InstaLOD::BakeEngine::CPU;
		Settings.Deterministic = bDeterministic;

		return Settings;
	}
};