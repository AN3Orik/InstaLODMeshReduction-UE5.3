/**
 * InstaLODIsotropicRemeshTool.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODIsotropicRemeshTool.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "InstaLODIsotropicRemeshTool.generated.h"

UENUM()
enum class EInstaLODIsotropicRemeshEdgeMode : uint8
{
	InstaLOD_Automatic					UMETA(DisplayName = "Automatic"),
	InstaLOD_Absolute					UMETA(DisplayName = "Absolute"),
	InstaLOD_BoundingSphereRelative		UMETA(DisplayName = "BoundingSphereRelative")
};

UCLASS(Config = InstaLOD)
class INSTALODUI_API UInstaLODIsotropicRemeshTool : public UInstaLODBaseTool
{
	GENERATED_BODY()

	/// VARIABLES ///

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/
public:

	/** The target edge mode. */
	UPROPERTY(Config, EditAnywhere, meta= (DisplayName="Edge Mode"), Category = "Isotropic Remesh Settings")
	EInstaLODIsotropicRemeshEdgeMode EdgeMode = EInstaLODIsotropicRemeshEdgeMode::InstaLOD_Automatic;

	/** Controls the size of the edges of the triangles. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Target Edge Size", ClampMin=0.001), Category = "Isotropic Remesh Settings")
	float TargetEdgeSize = 1.0f;

	/** How many refinement iterations to execute, more iterations lead to a higher degree of isotropy.*/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Precision"), Category = "Isotropic Remesh Settings")
	EInstaLODImportance Precision = EInstaLODImportance::InstaLOD_Normal;

	/** If > 0.0 vertices will be welded as a pre-pass.*/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Welding Threshold", ClampMin=0.0, ClampMax = 100.0), Category = "Isotropic Remesh Settings")
	float WeldingThreshold = 0.0f;

	/** If > 0.0 geometrical features to preserve will be detected based on the dihedral angle.*/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Geometric Feature Angle In Degrees", ClampMin = 0.0, ClampMax = 180.0), Category = "Isotropic Remesh Settings")
	float GeometricFeatureAngleInDegrees= 0.0f;

	/** Edges smaller as the specified threshold will be collapsed. The value is specified of the computed Target Edge Size: 'AbsoluteThreshold = AbsoluteTargetEdgeSize * CollapseThreshold'. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Collapse Threshold", ClampMin = 0.0, ClampMax = 100.0), Category = "Isotropic Remesh Settings")
	float CollapseThreshold = 75.0f;

	/************************************************************************/
	/* Feature Preservation                                                 */
	/************************************************************************/

	/** Determines if color splits will be preserved. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Preserve Color Splits"), Category = "Feature Preservation")
	bool bPreserveColorSplits = false;

	/** Determines if UV splits will be preserved. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Preserve UV Splits"), Category = "Feature Preservation")
	bool bPreserveUVSplits = true;

	/** Determines if normal splits will be preserved. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Preserve Normal Splits"), Category = "Feature Preservation")
	bool bPreserveNormalSplits = false;

	/** Determines if volume preservation is enabled. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Preserve Volume"), Category = "Feature Preservation")
	bool bPreserveVolume = false;

	/************************************************************************/
	/* Normal Recalculation                                                 */
	/************************************************************************/

	/** Recalculate normals of the output mesh. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Recalculate Normals"), Category = "Normal Recalculation")
	bool bRecalculateNormals = false;

	/** When recalculating normals: smooth polygons if the normal angle is below this value (in degrees). */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Hard Angle Threshold", ClampMin=0.0, ClampMax=180), Category = "Normal Recalculation")
	float HardAngleThreshold = 80.0f;

	/** When recalculating normals: smoothed normals are weighted by various geometric properties. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Weighted Normals"), Category = "Normal Recalculation")
	bool bWeightedNormals = true;

	/************************************************************************/
	/* Internal Use                                                         */
	/************************************************************************/

	InstaLOD::IIsotropicRemeshingOperation* Operation;
	InstaLOD::IsotropicRemeshingResult OperationResult;

	/** Constructor */
	UInstaLODIsotropicRemeshTool();

	/** Start - UInstaLODBaseTool Interface */
	virtual void OnMeshOperationExecute(bool bIsAsynchronous) override;
	virtual void DeallocMeshOperation() override;
	virtual bool IsMeshOperationSuccessful() const override;
	virtual bool IsMaterialDataRequired() const override {
		return false;
	}
	virtual bool IsFreezingTransformsForMultiSelection() const override {
		return true;
	}
	virtual bool ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject) override;

	virtual FText GetFriendlyName() const override;
	virtual FText GetComboBoxItemName() const override;
	virtual FText GetOperationInformation() const override;
	virtual int32 GetOrderId() const override;
	virtual void ResetSettings() override;
	/** End - UInstaLODBaseTool Interface */

private:

	InstaLOD::IsotropicRemeshingSettings GetIsotropicRemeshingSettings();
};

