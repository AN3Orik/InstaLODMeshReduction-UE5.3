/**
 * InstaLODScriptWrapper.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODScriptWrapper.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "InstaLODScriptResult.h"
#include "InstaLODScriptWrapper.generated.h"

UCLASS(BluePrintable)
class UInstaLODScriptWrapper : public UObject
{
	GENERATED_BODY()

public:
	
	/** Creates an imposter for each of the provided entries. */
	UFUNCTION(BlueprintCallable, Category = "setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* ImposterizeAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODImposterizeSettings* const ImposterizeSettings, class UInstaLODResultSettings* const ResultSettings, class UInstaLODBakeOutputSettings* const MaterialSettings);

	/** Material Merges all of the provided assets. */
	UFUNCTION(BlueprintCallable, Category = "setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* MaterialMergeAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODMaterialMergeSettings* const MaterialMergeSettings, class UInstaLODResultSettings* const ResultSettings, class UInstaLODBakeOutputSettings* const MaterialSettings);

	/** Occlusion Culls the provided assets. */
	UFUNCTION(BlueprintCallable, Category = "setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* OcclusionCullAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODOcclusionCullSettings* const OcclusionCullSettings, class UInstaLODResultSettings* const ResultSettings);

	/** UV Unwraps each of the provided assets. */
	UFUNCTION(BlueprintCallable, Category = "setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* UVUnwrapAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODUnwrapSettings* const UVUnwrapSettings, class UInstaLODResultSettings* const ResultSettings);

	/** Remeshes the provided assets. */
	UFUNCTION(BlueprintCallable, Category = "Setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* RemeshAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODRemeshSettings* const RemeshSettings, class UInstaLODResultSettings* const ResultSettings, class UInstaLODBakeOutputSettings* const MaterialSettings);

	/** Optimizes the provided assets. */
	UFUNCTION(BlueprintCallable, Category = "Setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* OptimizeAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODOptimizeSettings* const OptimizeSettings, class UInstaLODResultSettings* const ResultSettings);

	/** Isotropic remeshes the provided assets. */
	UFUNCTION(BlueprintCallable, Category = "Setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* IsotropicRemeshAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODIsotropicRemeshSettings* const IsotropicRemeshSettings, class UInstaLODResultSettings* const ResultSettings);

	/** Applies the Mesh Toolkit on each of the assets. */
	UFUNCTION(BlueprintCallable, Category = "Setting")
		static UPARAM(DisplayName = "Script Result") UInstaLODScriptResult* MeshToolKitAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, class UInstaLODMeshToolKitSettings* const MeshToolKitSettingsObject, UInstaLODResultSettings* const ResultSettingsObject);

	/** Creates the default imposterize material settings. */
	UFUNCTION(BlueprintCallable, Category = "Setting")
		static UPARAM(DisplayName = "Material Settings") UInstaLODBakeOutputSettings* CreateDefaultImposterizeMaterialSettings();

	/** Creates the default material settings. */
	UFUNCTION(BlueprintCallable, Category = "Setting")
		static UPARAM(DisplayName = "Material Settings") UInstaLODBakeOutputSettings* CreateDefaultMaterialSettings();
};