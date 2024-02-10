/**
 * InstaLODOcclusionCullTool.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODOcclusionCullTool.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "InstaLODOcclusionCullTool.generated.h"

UENUM()
enum class EInstaLODOcclusionCullMode : uint8
{
	InstaLOD_AutomaticInterior					UMETA(DisplayName = "Automatic Interior"),
	InstaLOD_CameraBased						UMETA(DisplayName = "Camera Based")
};

UENUM()
enum class EInstaLODOcclusionCullingStrategy : uint8
{
	InstaLOD_Face								UMETA(DisplayName = "Per Face"),
	InstaLOD_SubmeshByFaceSubmeshIndicies		UMETA(DisplayName = "Per Mesh"),
	InstaLOD_SubmeshByFaceAdjacency				UMETA(DisplayName = "Per Face Adjacency")
};

UENUM()
enum class EInstaLODOcclusionCullDataUsage : uint8
{
	InstaLOD_RemoveGeometry						UMETA(DisplayName = "Remove Geometry"),
	InstaLOD_WriteWedgeColors					UMETA(DisplayName = "Write To Vertex Colors")
};

UCLASS(Config=InstaLOD)
class INSTALODUI_API UInstaLODOcclusionCullTool : public UInstaLODBaseTool
{
	GENERATED_BODY()

	/// VARIABLES ///

	/************************************************************************/
	/* Settings																*/
	/************************************************************************/

public:

	/** Automatic Interior: Automatically remove interior surfaces. This mode is designed to operate on individual objects, it is less suited for large scenes. Camera Based: Cameras added to the operation will be used to determine ovvluded surfaces. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Mode"), Category = "Settings")
		EInstaLODOcclusionCullMode OcclusionCullMode = EInstaLODOcclusionCullMode::InstaLOD_AutomaticInterior;

	/** Specifies what kind of geometry will be removed. Per Face: Cull occluded individual faces. Per Mesh: Cull occluded submeshes, submeshes based on individual meshes added to the operation. Per Face Adjacency: Cull occluded submeshes, submeshes automatically derived from vertex adjacency. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Culling Strategy"), Category = "Settings")
		EInstaLODOcclusionCullingStrategy CullingStrategy = EInstaLODOcclusionCullingStrategy::InstaLOD_Face;

	/** Specifies how the occlusion data will be used. Remove Geometry: Remove occluded geometry. Write To Vertex Colors: Write occlusion data into vertex colors set 0. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Data Usage"), Category = "Settings")
		EInstaLODOcclusionCullDataUsage DataUsage = EInstaLODOcclusionCullDataUsage::InstaLOD_RemoveGeometry;

	/** The resolution of the rasterization. Higher resolutions improve quality by reducing the amount of missed faces due to subpixel inaccuracies. Dense geometry benefits from a higher resolution. However, a low resolution can be accommodated by increasing the 'Adjacency Depth' or the 'Precision'. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Resolution", ClampMin = 512, ClampMax = 16384), Category = "Settings")
		int32 Resolution = 1024;

	/** The precision of the automatic interior culling. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Precision"), Category = "Settings")
		EInstaLODImportance Precision = EInstaLODImportance::InstaLOD_Normal;

	/** When 'Culling Strategy' is set to 'Per Face': the recursion depth when protecting neighbors of visible faces. This can be used to prevent holes in the outside geometry that occurred due to a low resolution or a low precision. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Adjacency Depth", ClampMin = 0, ClampMax = 32), Category = "Settings")
		int32 AdjacencyDepth = 5;
	
#if defined(INSTALOD_OCCLUSIONCULL_ALPHAMASK_THRESHOLD)
	// NOTE: alpha mask threshold is not supported, otherwise we would have to flatten all materials
	// which can take an excessive amount of time
	/** Alpha Mask Threshold */
	PROPERTY(Config, EditAnywhere, meta = (DisplayName = "Alpha Mask Threshold", NoSpinbox, UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f), Category = "Settings")
		float AlphaMaskThreshold = 0.5f;
#endif

	/************************************************************************/
	/* Advanced							                                    */
	/************************************************************************/

	/** Makes the algorithm deterministic at the cost of speed. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Deterministic"), Category = "Advanced")
		bool bDeterministic = false;
	
	/************************************************************************/
	/* INTERNAL USE                                                         */
	/************************************************************************/
 
	
	InstaLOD::IOcclusionCullOperation *Operation;
	InstaLOD::OcclusionCullResult OperationResult;

public:
	UInstaLODOcclusionCullTool();

	/** Returns the Mode */
	EInstaLODOcclusionCullMode GetMode() const { return OcclusionCullMode; }

	/** Start - UInstaLODBaseTool Interface */
	virtual void OnMeshOperationExecute(bool bIsAsynchronous) override;
	virtual void DeallocMeshOperation() override;
	virtual bool IsMeshOperationSuccessful() const override;
	virtual bool IsWorldTransformRequired() const override {
		return true;
	}
	
	virtual FText GetFriendlyName() const override;
	virtual FText GetComboBoxItemName() const override;
	virtual FText GetOperationInformation() const override;
	virtual int32 GetOrderId() const override;
	virtual void ResetSettings() override;
	virtual bool ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject) override;
	/** End - UInstaLODBaseTool Interface */

protected:
	
	virtual bool IsMeshOperationExecutable(FText* OutErrorText) const override;


	InstaLOD::OcclusionCullSettings GetOcclusionCullSettings();
};
