/**
 * InstaLODIsotropicRemeshSettings.h (InstaLOD)
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
#include "Tools/InstaLODIsotropicRemeshTool.h"
#include "InstaLODIsotropicRemeshSettings.generated.h"

UCLASS(Config = InstaLOD, BluePrintable)
class UInstaLODIsotropicRemeshSettings : public UObject
{
	GENERATED_BODY()

public:

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

	/** The target edge mode. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Edge Mode"), Category = "Settings")
		EInstaLODIsotropicRemeshEdgeMode EdgeMode = EInstaLODIsotropicRemeshEdgeMode::InstaLOD_Automatic;

	/** Controls the size of the edges of the triangles. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Target Edge Size", ClampMin = 0.001), Category = "Settings")
		float TargetEdgeSize = 1.0f;

	/** How many refinement iterations to execute, more iterations lead to a higher degree of isotropy.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Precision"), Category = "Settings")
		EInstaLODImportance Precision = EInstaLODImportance::InstaLOD_Normal;

	/** If > 0.0 vertices will be welded as a pre-pass.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Welding Threshold", ClampMin = 0.0, ClampMax = 100.0), Category = "Settings")
		float WeldingThreshold = 0.0f;

	/** If > 0.0 geometrical features to preserve will be detected based on the dihedral angle.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Geometric Feature Angle In Degrees", ClampMin = 0.0, ClampMax = 180.0), Category = "Settings")
		float GeometricFeatureAngleInDegrees = 0.0f;

	/** Edges smaller as the specified threshold will be collapsed. The value is specified of the computed Target Edge Size: 'AbsoluteThreshold = AbsoluteTargetEdgeSize * CollapseThreshold'. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Collapse Threshold", ClampMin = 0.0, ClampMax = 100.0), Category = "Settings")
		float CollapseThreshold = 75.0f;

	/************************************************************************/
	/* Feature Preservation                                                 */
	/************************************************************************/

	/** Determines if color splits will be preserved. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Preserve Color Splits"), Category = "Feature Preservation")
		bool bPreserveColorSplits = false;

	/** Determines if UV splits will be preserved. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Preserve UV Splits"), Category = "Feature Preservation")
		bool bPreserveUVSplits = true;

	/** Determines if normal splits will be preserved. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Preserve Normal Splits"), Category = "Feature Preservation")
		bool bPreserveNormalSplits = false;

	/** Determines if volume preservation is enabled. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Preserve Volume"), Category = "Feature Preservation")
		bool bPreserveVolume = false;

	/************************************************************************/
	/* Normal Recalculation                                                 */
	/************************************************************************/

	/** Recalculate normals of the output mesh. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Recalculate Normals"), Category = "Normal Recalculation")
		bool bRecalculateNormals = false;

	/** When recalculating normals: smooth polygons if the normal angle is below this value (in degrees). */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Hard Angle Threshold", ClampMin = 0.0, ClampMax = 180), Category = "Normal Recalculation")
		float HardAngleThreshold = 80.0f;

	/** When recalculating normals: smoothed normals are weighted by various geometric properties. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Weighted Normals"), Category = "Normal Recalculation")
		bool bWeightedNormals = true;


	InstaLOD::IsotropicRemeshingSettings GetIsotropicRemeshingSettings()
	{
		InstaLOD::IsotropicRemeshingSettings Settings;

		Settings.EdgeMode = (InstaLOD::IsotropicRemeshingEdgeMode::Type)EdgeMode;
		Settings.TargetEdgeSize = TargetEdgeSize;
		Settings.Precision = (InstaLOD::MeshFeatureImportance::Type)Precision;
		Settings.WeldingThreshold = WeldingThreshold;
		Settings.GeometricFeatureAngleInDegrees = GeometricFeatureAngleInDegrees;
		Settings.CollapseThreshold = CollapseThreshold;
		Settings.PreserveColorSplits = bPreserveColorSplits;
		Settings.PreserveTexCoordSplits = true;
		Settings.PreserveNormalSplits = bPreserveNormalSplits;
		Settings.PreserveVolume = bPreserveVolume;
		Settings.RecalculateNormals = bRecalculateNormals;
		Settings.HardAngleThreshold = HardAngleThreshold;
		Settings.WeightedNormals = bWeightedNormals;

		return Settings;
	}
};