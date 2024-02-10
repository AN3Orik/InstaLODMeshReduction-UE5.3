/**
 * InstaLODMaterialMergeTool.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMaterialMergeTool.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "Tools/InstaLODBakeBaseTool.h"
#include "InstaLODMaterialMergeTool.generated.h"

UENUM()
enum class EInstaLODMaterialMergeMode : uint8
{
	InstaLOD_AutoRepack		UMETA(DisplayName = "Auto Repack"),
	InstaLOD_Transfer		UMETA(DisplayName = "Transfer")
};

UENUM()
enum class EInstaLODShellRotation : uint8
{
	InstaLOD_None			UMETA(DisplayName = "None"),
	InstaLOD_Allow90		UMETA(DisplayName = "Allow 90"),
	InstaLOD_Arbitrary		UMETA(DisplayName = "Arbitrary")
};

UCLASS(Config=InstaLOD)
class INSTALODUI_API UInstaLODMaterialMergeTool : public UInstaLODBaseTool
{
	GENERATED_BODY()

	/// VARIABLES ///

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

#if 0
	/** Mode */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Mode"), Category = "Settings")
		EInstaLODMaterialMergeMode MaterialMergeMode = EInstaLODMaterialMergeMode::InstaLOD_AutoRepack;
#endif
	/** Places duplicate UV Shells referencing same material ID on top of each other to save UV space*/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Stack Duplicate Shells"), Category = "Settings")
		bool bStackDuplicateShells = true;

	/** Insert UV shell splits for edges that do not share the same vertex normal. This improves the quality when rendering normal maps on normal split heavy geometry  */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Insert Normal Splits"), Category = "Settings")
		bool bInsertNormalSplits = true;

	/** Normalize UV shells according to worldspace scale*/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Worldspace Normalize"), Category = "Settings")
		bool bWorldspaceNormalizeShells = true;

	/** The minimum distance between two UV shells in pixels. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Gutter Size In Pixels", ClampMin = 1, ClampMax = 64), Category = "Settings")
		int32 GutterSizeInPixels = 5;

	/** Enables super sampling of texture pages. NOTE: super sampling causes an exponential increase in memory usage and processing time. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Super Sampling"), Category = "Settings")
		EInstaLODSuperSampling SuperSampling = EInstaLODSuperSampling::InstaLOD_X2;

	/** Enable to allow shells to be rotated for improved packing. Disabling rotations can cause certain scenarios to be unpackable. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Shell Rotation"), Category = "Settings")
	EInstaLODShellRotation ShellRotation = EInstaLODShellRotation::InstaLOD_Arbitrary;

	/** The input UV channel. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Input UV Set Index", ClampMin = 0, ClampMax = 7), Category = "Settings")
	int32 TextureCoordinateIndexInput = 0;

	/************************************************************************/
	/* Feature Importance                                                   */
	/************************************************************************/

	/** Determines importance of weights generated by evaluating occupied UV space. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "UV Importance"), Category = "Feature Importance")
		EInstaLODImportance UVImportance = EInstaLODImportance::InstaLOD_Normal;

	/** Determines importance of weights generated by evaluating world space geometry. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Geometric Importance"), Category = "Feature Importance")
		EInstaLODImportance GeometricImportance = EInstaLODImportance::InstaLOD_Normal;

	/** Determines importance of weights generated by evaluating texture dimensions. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Texture Importance"), Category = "Feature Importance")
		EInstaLODImportance TextureImportance = EInstaLODImportance::InstaLOD_Normal;

	/** Determines importance of weights generated by evaluating the visual importance (raytraced). */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Visual Importance"), Category = "Feature Importance")
	EInstaLODImportance VisualImportance = EInstaLODImportance::InstaLOD_Normal;
	
	/************************************************************************/
	/* Material																*/
	/************************************************************************/
	
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Material Settings"), Category = "Bake Output")
	FInstaLODMaterialSettings MaterialSettings;

	/************************************************************************/
	/* Advanced							                                    */
	/************************************************************************/

	/** Makes the algorithm deterministic at the cost of speed. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Deterministic"), Category = "Advanced")
	bool bDeterministic = false;

	/************************************************************************/
	/* INTERNAL USE                                                         */
	/************************************************************************/
	
	InstaLOD::IMeshMergeOperation2 *Operation;
	InstaLOD::MeshMergeResult OperationResult;

public: 
	/** Creates a basic UV for 0-area triangle to allow copying solid color information. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Generate Zero Area UV"), Category = "Settings")
	bool bGenerateZeroAreaUV = false;
	
	/** UV faces with an area smaller or equal to this value will be considered zero area. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Zero Area UV Threshold", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f), Category = "Settings")
	float ZeroAreaUVThreshold = 0.0f;

public:
	UInstaLODMaterialMergeTool();

	/** Start - UInstaLODBaseTool Interface */
	virtual void OnMeshOperationExecute(bool bIsAsynchronous) override;
	virtual void DeallocMeshOperation() override;
	virtual bool IsMeshOperationSuccessful() const override;
	virtual InstaLOD::IInstaLODMaterial* GetBakeMaterial() override;
	virtual bool IsMaterialDataRequired() const override {
		return true;
	}
	virtual bool IsWorldTransformRequired() const override {
		return true;
	}
	virtual bool ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject) override;
	virtual FMaterialProxySettings GetMaterialProxySettings() const override;
	virtual FText GetFriendlyName() const override;
	virtual FText GetComboBoxItemName() const override;
	virtual FText GetOperationInformation() const override;
	virtual int32 GetOrderId() const override;
	virtual void ResetSettings() override;
	/** End - UInstaLODBaseTool Interface */

private:

	InstaLOD::MeshMergeSettings GetMaterialMergeSettings();
};
