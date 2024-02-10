/**
 * UInstaLODUVTool.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file UInstaLODUVTool.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "InstaLODUVTool.generated.h"

UCLASS(Config = InstaLOD)
class INSTALODUI_API UInstaLODUVTool : public UInstaLODBaseTool
{
	GENERATED_BODY()

	/// VARIABLES ///

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

public:
	/** The output texture width. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Texture Size Width", UIMin = 32, UIMax = 4096, ClampMin = 32, ClampMax = 4096, RadioButton), Category = "Settings")
		int32 TextureSizeWidth = 1024;
	
	/** The output texture height. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Texture Size Height", UIMin = 32, UIMax = 4096, ClampMin = 32, ClampMax = 4096, RadioButton), Category = "Settings")
		int32 TextureSizeHeight = 1024;
	
	/** The minimum distance between two UV shells in pixels. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Gutter Size In Pixels", ClampMin = 1, ClampMax = 64), Category = "Settings")
		int32 GutterSizeInPixels = 5;

	/** The output UV set index. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Output UV Set", UIMin = 0, UIMax = 4, ClampMin = 0, ClampMax = 4), Category = "Settings")
		int32 TexCoordIndexOutput = 0;

	/** Strategy used during unwrapping. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Unwrap Strategy"), Category = "Settings")
		EInstaLODUnwrapStrategy UnwrapStrategy = EInstaLODUnwrapStrategy::InstaLOD_Auto;

	/** The importance of UV space stretch during the unwrap. OFF: high levels of stretch allowed, Highest: no stretching allowed. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Stretch Importance"), Category = "Settings")
		EInstaLODImportance StretchImportance = EInstaLODImportance::InstaLOD_Normal;
	
	/** When 'Unwrap Strategy' is either 'Hard Surface Axial' or 'Hard Surface Angle', enables stitching to reduce the amount of splits and shells. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Shell Stitching"), Category = "Settings")
		bool bShellStitching = true;

	/** Insert UV shells splits for edges that do not share the same vertex normal. This improves the quality when rendering normal maps on normal split heavy geometry. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Insert Normal Splits"), Category = "Settings")
		bool bInsertNormalSplits = true;
	
	/** A scale factor to apply to the unwrap. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Scale", UIMin = 0.0f, UIMax = 10.0f, ClampMin = 0.0f, ClampMax = 10.0f), Category = "Settings")
		float UVScale = 1.0f;

	InstaLOD::IUnwrapOperation *Operation;
	InstaLOD::UnwrapResult OperationResult;

	/************************************************************************/
	/* Advanced							                                    */
	/************************************************************************/

	/** Makes the algorithm deterministic at the cost of speed. */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Deterministic"), Category = "Advanced")
		bool bDeterministic = false;

public:

	/** Constructor */
	UInstaLODUVTool();

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

	InstaLOD::UnwrapSettings GetUnwrapSettings();
};

