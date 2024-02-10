/**
 * InstaLODMeshToolKitSettings.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMeshToolKitSettings.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once
#include "CoreMinimal.h"

#include "Tools/InstaLODMeshToolKitTool.h"
#include "InstaLODMeshToolKitSettings.generated.h"

UCLASS(Config = InstaLOD, BluePrintable)
class UInstaLODMeshToolKitSettings : public UObject
{
	GENERATED_BODY()

public:

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

	/** If set to a value > 0, automatically welds vertices within the specified distance. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Vertex Welding", UIMin = 0.0f, UIMax = 100.0f, ClampMin = 0.0f, ClampMax = 100.0f, RadioButton), Category = "Settings")
		float VertexWelding = 0.0f;

	/** Prevents welding of normals if the angle between the welded normal and the wedge normal is above the specified value in degrees. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Welding Normal Angle", UIMin = 0.0f, UIMax = 180.0f, ClampMin = 0.0f, ClampMax = 180.0f, RadioButton), Category = "Settings")
		float VertexWeldingNormalAngle = 80.0f;

	/** Enable to weld boundary vertices only. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Weld Boundaries"), Category = "Settings")
		bool bWeldingBoundaries = false;

	/** If set to a value > 0, automatically heals T-Junctions if the vertex-edge distance is below the specified threshold. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "T-Junction Healing", UIMin = 0.0f, UIMax = 10.0f, ClampMin = 0.0f, ClampMax = 10.0f, RadioButton), Category = "Settings")
		float TJunctionHealing = 0.0f;

	/** If set to a value > 0, automatically removes degenerates if two or more corners are shared for a polygon. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Remove Degenerate Faces", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f), Category = "Settings")
		float RemoveDegenerateFaces = 0.0f;

	/** Rebuilds the mesh topology to be manifold. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Fix Non Manifold"), Category = "Settings")
		bool bFixNonManifold = false;

	/** Unifies the direction of the wedge normals. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Conform Normals"), Category = "Settings")
		bool bConformNormals = false;

	/** Unifies the polygon winding order. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Conform Winding Order"), Category = "Settings")
		bool bConformWindingOrder = false;

	/** Flips the normals of the mesh. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Flip Normals"), Category = "Settings")
		bool bFlipNormals = false;

	/** Flips the winding order of the mesh. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Flip Winding Order"), Category = "Settings")
		bool bFlipWindingOrder = false;

	/** Fill all holes in the mesh. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Fill Holes"), Category = "Settings")
		bool bFillHoles = false;

	/** Invalid normals can automatically be fixed by InstaLOD without recomputing all normals. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Normal Healing Mode"), Category = "Settings")
		EInstaLODNormalHealing NormalHealingMode = EInstaLODNormalHealing::InstaLOD_OFF;

	/** Recalculate normals of the mesh */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Recalculate Normals"), Category = "Settings")
		bool bRecalculateNormals = false;

	/** When recalculating normals: smooth faces if the normal angle is below this value (in degrees). */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Hard Angle Threshold", EditCondition = "bRecalculateNormals", UIMin = 0.0f, UIMax = 180.0f, ClampMin = 0.0f, ClampMax = 180.0f, RadioButton), Category = "Settings")
		float HardangleThreshold = 80.0f;

	/** When recalculating normals: smoothed normals are weighted by various geometric properties. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Weighted Normals", EditCondition = "bRecalculateNormals"), Category = "Settings")
		bool bWeightedNormals = true;

	/** If set to a value > 0, automatically remove all submeshes (determined by adjacency) where the bounding sphere radius is belo the threshold. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Minimum Radius", UIMin = 0.0f, ClampMin = 0.0f), Category = "Settings")
		float MinimumRadius = 0.0f;

	/** If set to a value > 0, automatically remove all faces with an area smaller than the specified value. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Minimum Face Area", UIMin = 0.0f, ClampMin = 0.0f), Category = "Settings")
		float MinimumFaceArea = 0.0f;

	InstaLOD::MeshToolKitSettings GetMeshToolKitSettings()
	{
		InstaLOD::MeshToolKitSettings Settings;

		Settings.WeldingThreshold = VertexWelding;
		Settings.WeldingNormalAngleThreshold = VertexWeldingNormalAngle;
		Settings.WeldingBoundaries = bWeldingBoundaries;
		Settings.HealTJunctionThreshold = TJunctionHealing;
		Settings.RemoveDegenerateFacesThreshold = RemoveDegenerateFaces;
		Settings.FixNonManifold = bFixNonManifold;
		Settings.ConformNormals = bConformNormals;
		Settings.ConformWindingOrder = bConformWindingOrder;
		Settings.FlipNormals = bFlipNormals;
		Settings.FlipWindingOrder = bFlipWindingOrder;
		Settings.FillHoles = bFillHoles;
		Settings.NormalHealingMode = (InstaLOD::NormalHealingMode::Type)NormalHealingMode;
		Settings.RecalculateNormals = bRecalculateNormals;
		Settings.HardAngleThreshold = HardangleThreshold;
		Settings.WeightedNormals = bWeightedNormals;
		Settings.MinimumSubMeshBoundingSphereRadius = MinimumRadius;
		Settings.MinimumFaceArea = MinimumFaceArea;

		return Settings;
	}
};

