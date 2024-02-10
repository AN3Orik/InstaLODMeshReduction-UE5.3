/**
 * UInstaLODUVTool.h (InstaLOD)
 *
 * Copyright 2016-2020 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file UInstaLODUVTool.h
 * @copyright 2016-2020 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "InstaLODMeshToolKitTool.generated.h"

UENUM()
enum class EInstaLODNormalHealing : uint8
{
	InstaLOD_OFF				UMETA(DisplayName = "Off"),
	InstaLOD_Minimal			UMETA(DisplayName = "Minimal"),
	InstaLOD_Default			UMETA(DisplayName = "Default"),
};


UCLASS(Config = InstaLOD)
class INSTALODUI_API UInstaLODMeshToolKitTool : public UInstaLODBaseTool
{
	GENERATED_BODY()

	/// VARIABLES ///

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

public:
	/** If set to a value > 0, automatically welds vertices within the specified distance. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Vertex Welding", UIMin = 0.0f, UIMax = 100.0f, ClampMin = 0.0f, ClampMax = 100.0f), Category = "Settings")
		float VertexWelding = 0.0f;

	/** Prevents welding of normals if the angle between the welded normal and the wedge normal is above the specified value in degrees. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Welding Normal Angle", EditCondition = "VertexWelding > 0.0", EditConditionHides, UIMin = 0.0f, UIMax = 180.0f, ClampMin = 0.0f, ClampMax = 180.0f), Category = "Settings")
		float VertexWeldingNormalAngle = 80.0f;

	/** Enable to weld boundary vertices only. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Weld Boundaries", EditCondition = "VertexWelding > 0.0", EditConditionHides), Category = "Settings")
		bool bWeldingBoundaries = false;

	/** If set to a value > 0, automatically heals T-Junctions if the vertex-edge distance is below the specified threshold. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "T-Junction Healing", UIMin = 0.0f, UIMax = 10.0f, ClampMin = 0.0f, ClampMax = 10.0f), Category = "Settings")
		float TJunctionHealing = 0.0f;
	
	/** If set to a value > 0, automatically removes degenerates if two or more corners are shared for a polygon. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Remove Degenerate Faces", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f), Category = "Settings")
		float RemoveDegenerateFaces = 0.0f;

	/** Rebuilds the mesh topology to be manifold. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Fix Non Manifold"), Category = "Settings")
		bool bFixNonManifold = false;

	/** Unifies the direction of the wedge normals. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Conform Normals"), Category = "Settings")
		bool bConformNormals = false;

	/** Unifies the polygon winding order. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Conform Winding Order"), Category = "Settings")
		bool bConformWindingOrder = false;

	/** Flips the normals of the mesh. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Flip Normals"), Category = "Settings")
		bool bFlipNormals = false;

	/** Flips the winding order of the mesh. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Flip Winding Order"), Category = "Settings")
		bool bFlipWindingOrder = false;

	/** Fill all holes in the mesh. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Fill Holes"), Category = "Settings")
		bool bFillHoles = false;

	/** Invalid normals can automatically be fixed by InstaLOD without recomputing all normals. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Normal Healing Mode"), Category = "Settings")
		EInstaLODNormalHealing NormalHealingMode = EInstaLODNormalHealing::InstaLOD_OFF;
	
	/** Recalculate normals of the mesh */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Recalculate Normals"), Category = "Settings")
		bool bRecalculateNormals = false;

	/** When recalculating normals: smooth faces if the normal angle is below this value (in degrees). */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Hard Angle Threshold", EditCondition="bRecalculateNormals", EditConditionHides, UIMin = 0.0f, UIMax = 180.0f, ClampMin = 0.0f, ClampMax = 180.0f), Category = "Settings")
		float HardangleThreshold = 80.0f;

	/** When recalculating normals: smoothed normals are weighted by various geometric properties. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Weighted Normals", EditCondition="bRecalculateNormals", EditConditionHides), Category = "Settings")
		bool bWeightedNormals = true;
	
	/** If set to a value > 0, automatically remove all submeshes (determined by adjacency) where the bounding sphere radius is belo the threshold. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Minimum Radius", UIMin = 0.0f, ClampMin = 0.0f), Category = "Settings")
		float MinimumRadius = 0.0f;

	/** If set to a value > 0, automatically remove all faces with an area smaller than the specified value. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Minimum Face Area", UIMin = 0.0f, ClampMin = 0.0f), Category = "Settings")
		float MinimumFaceArea = 0.0f;

	InstaLOD::IMeshToolKitOperation *Operation;
	InstaLOD::MeshToolKitResult OperationResult;

public:

	/** Constructor */
	UInstaLODMeshToolKitTool();

	/** Start - UInstaLODBaseTool Interface */
	virtual void OnMeshOperationExecute(bool bIsAsynchronous) override;
	virtual void DeallocMeshOperation() override;
	virtual bool IsMeshOperationSuccessful() const override;
	virtual void ResetSettings() override;
	virtual FText GetComboBoxItemName() const override;
	virtual FText GetOperationInformation() const override;
	virtual int32 GetOrderId() const override;
	virtual FText GetFriendlyName() const override;
	
	virtual bool IsMaterialDataRequired() const override {
		return false;
	}
	virtual bool IsFreezingTransformsForMultiSelection() const override {
		return false;
	}
	virtual bool IsWorldTransformRequired() const override {
		return false;
	}
	virtual bool ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject) override;
	/** End - UInstaLODBaseTool Interface */
private:

	InstaLOD::MeshToolKitSettings GetMeshToolKitSettings();
};

