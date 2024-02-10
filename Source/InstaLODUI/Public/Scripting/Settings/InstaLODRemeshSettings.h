/**
 * InstaLODRemeshSettings.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODRemeshSettings.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once
#include "CoreMinimal.h"
#include "Tools/InstaLODRemeshTool.h"
#include "InstaLODRemeshSettings.generated.h"

UCLASS(Config = InstaLOD, BluePrintable)
class UInstaLODRemeshSettings : public UObject
{
	GENERATED_BODY()

public:

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

	/** Determines how the output surface will be constructed. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Remesh Mode"), Category = "Settings")
		EInstaLODRemeshMode RemeshMode = EInstaLODRemeshMode::InstaLOD_Reconstruct;

	/** The fuzzy face count target of the output geometry. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Fuzzy Face Count Target ", RadioButton), Category = "Settings")
		EInstaLODRemeshFaceCount FuzzyFaceCountTarget = EInstaLODRemeshFaceCount::InstaLOD_Normal;

	/** The maximum amount of polygons for the target mesh. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Maximum Triangles", ClampMin = 4, RadioButton), Category = "Settings")
		int32 MaximumTriangles = 4;

	/** Calculates the amount of polygons to remove based on the display size of the output model. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Screen Size In Pixels", UIMin = 32, UIMax = 1024, ClampMin = 32, ClampMax = 1024, RadioButton), Category = "Settings")
		int32 ScreenSizeInPixels = 300;

	/** When 'Mode' is set to 'Reconstruct' and 'Screen Size in Pixels' is enabled, the pixel distance in screen size that will be merged together. This can be used to avoid the construction of unnecessary geometrical detail. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Pixel Merge Distance", ClampMin = 1, ClampMax = 32), Category = "Settings")
		int32 PixelMergeDistance = 2;

	/** When 'Screen Size in Pixels' is enabled, automatically compute output texture dimensions based on screen size in pixels. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Automatic Texture Size"), Category = "Settings")
		bool bAutomaticTextureSize = false;


	/************************************************************************/
	/* Surface Construction                                                 */
	/************************************************************************/

	/** The resolution of the surface construction. The resolution is ^3 so memory and processing time grows exponentially. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Resolution"), Category = "Surface Construction")
		EInstaLODRemeshResolution RemeshResolution = EInstaLODRemeshResolution::InstaLOD_Normal;

	/** Ignores backfaces during surface construction. NOTE: this can cause holes in the constructed geometry if face normals are not pointing outwards. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Ignore Backfaces"), Category = "Surface Construction")
		bool bIgnoreBackfaces = false;

	/** When 'SurfaceMode' is 'Optimize', lock boundary vertices in place. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Optimize Mode Lock Boundaries"), Category = "Surface Construction")
		bool bSurfaceModeOptimizeLockBoundaries = false;

	/** WWhen 'Mode' is set to 'Reconstruct', InstaLOD constructs a surface that fuses all meshes added to the remesh operation together and culls interior faces. If option is enabled all submeshes will be processed in isolation. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Distinct Construction"), Category = "Settings")
		bool bDistinctContruction = false;

	/** When calculating normals for the output mesh, smooth faces if the normal angle is below this value (in degrees). */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Hard Angle Threshold", UIMin = 0.0f, UIMax = 180.0f, ClampMin = 0.0f, ClampMax = 180.0f), Category = "Surface Construction")
		float HardAngleThreshold = 70.0f;

	/** The welding distance can be used to weld holes in the input geometry. The welded mesh is only used for the surface construction is is not used during baking. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Welding Distance", UIMin = 0.0f, UIMax = 10.0f), Category = "Surface Construction")
		float WeldingDistance = 0.0f;

	/** The minimum distance between two UV shells in pixels. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Gutter Size In Pixels", ClampMin = 1, ClampMax = 64), Category = "Surface Construction")
		int32 GutterSizeInPixels = 2;

	/** Strategy used during unwrap. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Unwrap Strategy"), Category = "Surface Construction")
		EInstaLODUnwrapStrategy UnwrapStrategy = EInstaLODUnwrapStrategy::InstaLOD_Auto;

	/** The importance of UV space stretch during the unwrap. OFF: high levels of stretch allowed, Highest: no stretching allowed. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (DisplayName = "Unwrap Stretch Importance"), Category = "Surface Construction")
		EInstaLODImportance UnwrapStretchImportance = EInstaLODImportance::InstaLOD_Normal;

	InstaLOD::RemeshingSettings GetRemeshingSettings()
	{
		InstaLOD::RemeshingSettings Settings;

		Settings.SurfaceMode = (InstaLOD::RemeshSurfaceMode::Type)RemeshMode;
		Settings.FaceCountTarget = (InstaLOD::RemeshFaceCountTarget::Type)FuzzyFaceCountTarget;
		Settings.MaximumTriangles = MaximumTriangles;
		Settings.ScreenSizeInPixels = ScreenSizeInPixels;
		Settings.ScreenSizePixelMergeDistance = PixelMergeDistance;
		Settings.ScreenSizeInPixelsAutomaticTextureSize = bAutomaticTextureSize;
		
		switch (RemeshResolution)
		{
		case EInstaLODRemeshResolution::InstaLOD_Lowest:
			Settings.Resolution = 100;
			break;
		case EInstaLODRemeshResolution::InstaLOD_Low:
			Settings.Resolution = 150;
			break;
		case EInstaLODRemeshResolution::InstaLOD_Normal:
			Settings.Resolution = 256;
			break;
		case EInstaLODRemeshResolution::InstaLOD_High:
			Settings.Resolution = 512;
			break;
		case EInstaLODRemeshResolution::InstaLOD_VeryHigh:
			Settings.Resolution = 1024;
			break;
		case EInstaLODRemeshResolution::InstaLOD_Extreme:
			Settings.Resolution = 2048;
		}
		Settings.DistinctSurfaceConstructionPerMesh = bDistinctContruction;
		Settings.SurfaceConstructionIgnoreBackface = bIgnoreBackfaces;
		Settings.SurfaceModeOptimizeLockBoundaries = bSurfaceModeOptimizeLockBoundaries;
		Settings.HardAngleThreshold = HardAngleThreshold;
		Settings.WeldDistance = WeldingDistance;
		Settings.GutterSizeInPixels = GutterSizeInPixels;
		Settings.UnwrapStrategy = (InstaLOD::UnwrapStrategy::Type)UnwrapStrategy;
		Settings.StretchImportance = (InstaLOD::MeshFeatureImportance::Type)UnwrapStretchImportance;
		Settings.BakeEngine = InstaLOD::BakeEngine::CPU;

		return Settings;
	}
};

