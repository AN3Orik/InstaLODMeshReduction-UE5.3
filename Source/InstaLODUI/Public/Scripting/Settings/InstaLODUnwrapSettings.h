/**
 * InstaLODUnwrapSettings.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUnwrapSettings.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODUVTool.h"
#include "InstaLODUnwrapSettings.generated.h"

UCLASS(BluePrintable, Config = InstaLOD)
class UInstaLODUnwrapSettings : public UObject
{
	GENERATED_BODY()

public:

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

	/** The output texture width. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Texture Size Width", UIMin = 32, UIMax = 4096, ClampMin = 32, ClampMax = 4096, RadioButton), Category = "Settings")
		int32 TextureSizeWidth = 1024;

	/** The output texture height. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Texture Size Height", UIMin = 32, UIMax = 4096, ClampMin = 32, ClampMax = 4096, RadioButton), Category = "Settings")
		int32 TextureSizeHeight = 1024;

	/** The minimum distance between two UV shells in pixels. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Gutter Size In Pixels", ClampMin = 1, ClampMax = 64), Category = "Settings")
		int32 GutterSizeInPixels = 5;

	/** The output UV set index. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Output UV Set", UIMin = 0, UIMax = 4, ClampMin = 0, ClampMax = 4), Category = "Settings")
		int32 TexCoordIndexOutput = 0;

	/** Strategy used during unwrapping. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Unwrap Strategy"), Category = "Settings")
		EInstaLODUnwrapStrategy UnwrapStrategy = EInstaLODUnwrapStrategy::InstaLOD_Auto;

	/** The importance of UV space stretch during the unwrap. OFF: high levels of stretch allowed, Highest: no stretching allowed. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Stretch Importance"), Category = "Settings")
		EInstaLODImportance StretchImportance = EInstaLODImportance::InstaLOD_Normal;

	/** When 'Unwrap Strategy' is either 'Hard Surface Axial' or 'Hard Surface Angle', enables stitching to reduce the amount of splits and shells. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Shell Stitching"), Category = "Settings")
		bool bShellStitching = true;

	/** Insert UV shells splits for edges that do not share the same vertex normal. This improves the quality when rendering normal maps on normal split heavy geometry. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Insert Normal Splits"), Category = "Settings")
		bool bInsertNormalSplits = true;

	/** A scale factor to apply to the unwrap. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Scale", UIMin = 0.0f, UIMax = 10.0f, ClampMin = 0.0f, ClampMax = 10.0f), Category = "Settings")
		float UVScale = 1.0f;

	/************************************************************************/
	/* Advanced							                                    */
	/************************************************************************/

	/** Makes the algorithm deterministic at the cost of speed. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Deterministic"), Category = "Advanced")
		bool bDeterministic = false;

	InstaLOD::UnwrapSettings GetUnwrapSettings()
	{
		InstaLOD::UnwrapSettings Settings;

		Settings.UnwrapStrategy = (InstaLOD::UnwrapStrategy::Type)UnwrapStrategy;
		Settings.StretchImportance = (InstaLOD::MeshFeatureImportance::Type)StretchImportance;
		Settings.GutterSizeInPixels = GutterSizeInPixels;
		Settings.TextureHeight = TextureSizeHeight;
		Settings.TextureWidth = TextureSizeWidth;
		Settings.TexCoordIndexOutput = TexCoordIndexOutput;
		Settings.UVScale = UVScale;
		Settings.InsertNormalSplits = bInsertNormalSplits;
		Settings.ShellStitching = bShellStitching;
		Settings.Deterministic = bDeterministic;

		return Settings;
	}
};