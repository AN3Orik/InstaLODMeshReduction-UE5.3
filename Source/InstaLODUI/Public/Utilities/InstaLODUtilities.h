/**
 * InstaLODUtilities.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUtilities.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "InstaLODTypes.h"
#include "InstaLOD/InstaLODAPI.h"
#include "InstaLOD/InstaLOD.h"
#include "Scripting/Settings/InstaLODResultSettings.h"
#include "InstaLOD/InstaLODMeshExtended.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

struct InstaLODMergeData
{
	FInstaLODMeshComponent* Component;
	InstaLOD::IInstaLODMesh* InstaLODMesh;
};


class UInstaLODUtilities : public UObject
{
public:

	/************************************************************************/
	/* Utilities                                                            */
	/************************************************************************/

	/**
	 * Writes all InstaLOD messages to the log.
	 *
	 * @return true if messages have been written to the log.
	 */
	static bool WriteInstaLODMessagesToLog(class IInstaLOD* InstaLOD);

	/**
	 * FIXME
	 */
	static void CreateMaterialData(IInstaLOD* InstaLOD, TArray<InstaLODMergeData>& InOutComponentsToMerge, class InstaLOD::IInstaLODMaterialData *OutMaterialData, const struct FMaterialProxySettings& InMaterialProxySettings, TArray<UMaterialInterface*>& OutMaterials);

	/**
	*	Appends a Static-/SkeletalMeshComponent to an InstanLODExtendedMesh.
	*
	*	@param		MeshComponent		The MeshComponent holding either Static- or SkeletalMeshComponent
	*	@param		OutInstaLODMesh		InstaLODMesh that we appended to
	*/
	static void AppendMeshComponentToInstaLODMesh(class IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, class InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex = 0);

	/**
	*	Converts the passed Static-/SkeletalMeshComponent into an InstaLODMesh.
	*
	*	@param		MeshComponent		The MeshComponent holding either Static- or SkeletalMeshComponent
	*	@param		OutInstaLODMesh		Converted InstaLODMesh
	*/
	static void GetInstaLODMeshFromMeshComponent(class IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, class InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex = 0, bool IsStaticMeshInWorldSpaceRequired = false);

	static void GetInstaLODMeshFromStaticMeshComponent(class IInstaLOD* InstaLOD, class UStaticMeshComponent* StaticMeshComponent, class InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex, bool bWorldSpace);
	static void GetInstaLODMeshFromSkeletalMeshComponent(class IInstaLOD* InstaLOD, class USkeletalMeshComponent* SkeletalMeshComponent, class InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex, class UAnimSequence *const BakePose = nullptr);
	
	/**
	*	Saves the InstaLODMesh Data into the passed Static-/SkeletalMeshComponent.
	*
	*	@param		MeshComponent		The MeshComponent holding either Static- or SkeletalMeshComponent
	*	@param		InstaLODMesh		InstaLODMesh holding the (new) Mesh Data
	*/
	static void InsertLODToMeshComponent(class IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, class InstaLOD::IInstaLODMesh* InstaLODMesh, int32 TargetLODIndex, UMaterialInterface* NewMaterial);

	static void InsertLODToStaticMesh(class IInstaLOD* InstaLOD, class UStaticMesh* StaticMesh, class InstaLOD::IInstaLODMesh* InstaLODMesh, int32 TargetLODIndex, UMaterialInterface* NewMaterial);
	static void InsertLODToSkeletalMesh(class IInstaLOD* InstaLOD, class USkeletalMesh* SkeletalMesh, class InstaLOD::IInstaLODMesh* InstaLODMesh, int32 TargetLODIndex, UMaterialInterface* NewMaterial);


	/**
	 * Updates the Mesh inside of the MeshComponent's Static- or SkeletalMeshComponent.
	 *
	 * @param MeshComponent The MeshComponent holding either Static- or SkeletalMeshComponent
	 * @param NewMesh New Static-/SkeletalMesh
	 */
	static void UpdateMeshOfMeshComponent(TSharedPtr<FInstaLODMeshComponent> MeshComponent, UObject* NewMesh, bool bRemoveOverrideMaterials);
	
	/**
	 * Transforms the specified mesh instance by the specified transform.
	 *
	 * @param InstaLODMesh
	 * @param Transform
	 */
	static void TransformInstaLODMesh(InstaLOD::IInstaLODMesh* InstaLODMesh, const FTransform& Transform, bool LocalToWorld);
	
	/**
	 * FIXME
	 */
	static UMaterialInstanceConstant* CreateFlattenMaterialInstance(const FFlattenMaterial& FlattenMaterial, const FMaterialProxySettings& InMaterialProxySettings, const FString& SaveObjectPath, TArray<UObject*>& OutAssetsToSync, bool bIsFlipbookMaterial = false);
	
	/**
	 * FIXME
	 */
	static UTexture* ConvertInstaLODTexturePageToTexture(InstaLOD::IInstaLODTexturePage* InstaLODTexturePage, const FString& SaveObjectPath);

	/**
	*	Saves an InstanLOD Mesh into a duplicate of an existing Static-/SkeletalMesh Asset.
	*
	*	@param		MeshComponent		The MeshComponent holding either Static- or SkeletalMeshComponent
	*	@param		InstaLODMesh		InstaLODMesh holding the (new) Mesh Data
	*	@param		SaveObjectPath		Previously determined save path for the new object
	*/
	static UObject* SaveInstaLODMeshToDuplicateAsset(class IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, class InstaLOD::IInstaLODMesh* InstaLODMesh, const FString& SaveObjectPath, bool clearMaterialAndSectionInfo);
	
	/**
	 *	Creates a plane at Zero with with the specified extents and face normal (0,0,1).
	 */
	static InstaLOD::IInstaLODMesh* CreatePlane(class IInstaLOD* InstaLOD, const FVector2d Extents);
	
	/**
	 *	Saves an InstanLOD Mesh to a new StaticMesh Asset.
	 *
	 * @param InstaLOD InstaLOD API object.
	 * @param InstaLODMesh InstaLODMesh holding the (new) Mesh Data.
	 * @param SaveObjectPath Previously determined save path for the new object.
	 * @return UObject.
	 */
	static UObject* SaveInstaLODMeshToStaticMeshAsset(class IInstaLOD* InstaLOD, class InstaLOD::IInstaLODMesh* InstaLODMesh, const FString& SaveObjectPath);

	/**
	 *	Saves an InstanLOD Mesh to a new SkeletalMesh Asset.
	 *
	 * @param InstaLOD InstaLOD API object.
	 * @param InstaLODMesh InstaLODMesh holding the (new) Mesh Data.
	 * @param SaveObjectPath Previously determined save path for the new object.
	 * @return UObject.
	 */
	static UObject* SaveInstaLODMeshToSkeletalMeshAsset(class IInstaLOD* InstaLOD, class InstaLOD::IInstaLODMesh* InstaLODMesh, const FString& SaveObjectPath);

	static void RetrieveMesh(UStaticMeshComponent *const StaticMeshComponent, int32 LODIndex, FMeshDescription& RawMesh, bool bPropagateVertexColours, bool bInWorldSpace);

	static void RemoveAllLODsFromStaticMesh(class UStaticMesh* StaticMesh);
	static void RemoveAllLODsFromSkeletalMesh(class USkeletalMesh* SkeletalMesh);

	/** Opens the Save Dialog and returns the selected path. */
	static FString OpenSaveDialog(const FText& DialogTitle, const FString& DefaultPackageName, const FString &AssetNamePrefix = FString(), bool bMultiSave = false);

	/**
	 * Determines whether an asset already exists at the specified path.
	 *
	 * @param SaveObjectPath The path.
	 * @return True if asset already exists.
	 */
	static bool CheckIfAssetExists(const FString& SaveObjectPath);

	/**
	 * Ensures the save path is unique. With the PathNamesSet additional
	 * paths can beprovided to check against. 
	 * @note The unique path will be added to the set.
	 * 
	 * @param SavePath The save path.
	 * @param Entry The entry.
	 * @param PathNamesSet The set of paths.
	 * @param Default The default save path.
	 * @return The unique save path.
	 * 
	 */
	static FString EnsureSavePathIsUniqueForEntry(const FString& SavePath, UObject* const Entry, TSet<FString>& PathNamesSet, const FString& Default);

	/**
	 * Returns a String containing invalid materials.
	 *
	 * @return String with invalid characters.
	 */
	static FString GetInvalidCharactersForSavePath();

	/**
	 * Creates a valid save path.
	 *
	 * @param Path the path.
	 * @return the validated path.
	 */
	static FString GetValidSavePath(const FString& Path);

	/**
	 * Determines whether the save path contains illegal characters.
	 *
	 * @param savePath The save path.
	 * @return True if contains illegal characters.
	 */
	static bool DoesSavePathContainIllegalCharacters(const FString& SavePath);

	/**
	 * Creates merge data based on the specified parameters.
	 *
	 * @param MeshComponents the components to create merge data from.
	 * @param InstaLODInterface the UE interface.
	 * @param BaseLODIndex the base lod index.
	 * @return array of merge data objects.
	 */
	static TArray<InstaLODMergeData> CreateMergeData(TArray<TSharedPtr<FInstaLODMeshComponent>>& MeshComponents, IInstaLOD* const InstaLODInterface, const int BaseLODIndex);

	/**
	 * Finalizes the script result based on the provided settings.
	 *
	 * @param Entry The entry.
	 * @param InstaLODInterface The InstaLOD API interface.
	 * @param MeshComponent The mesh component.
	 * @param OutputMesh The mesh result.
	 * @param ResultSettings The settings for the result.
	 * @param Material The material.
	 * @return True upon success.
	 */
	static bool FinalizeScriptProcessResult(UObject* const Entry, IInstaLOD* const InstaLODInterface, TSharedPtr<FInstaLODMeshComponent>& MeshComponent, InstaLOD::IInstaLODMesh* const OutputMesh, UInstaLODResultSettings* const ResultSettings, TArray<UObject*>& OutputArray, UMaterialInstanceConstant* const Material, const bool bIsFreezingTransformsForMultiSelection = false);

	/**
	 * Creates a FlattenMaterial from the provided InstaLOD Material.
	 * 
	 * @param Material The material.
	 * @param InMaterialProxySettings The proxy material settings.
	 * @param SaveObjectPath Where the material is saved.
	 * @param OutAssetsToSync Array receiving the material.
	 * @param bIsFlipbookMaterial Whether it is a material for a flipbook imposter.
	 * @param bIsFreezingTransformsForMultiSelection Whether the operation freezes the transform for multi-selection.
	 * @return The material.
	 */
	static UMaterialInstanceConstant* CreateFlattenMaterialInstanceFromInstaMaterial(InstaLOD::IInstaLODMaterial* const Material, const FMaterialProxySettings& InMaterialProxySettings, const FString& SaveObjectPath, TArray<UObject*>& OutAssetsToSync, bool bIsFlipbookMaterial = false);

	/**
	* Customizes the pivot Position of the output mesh.
	*
	* @param Mesh The output mesh.
	* @param PivotPosition The new position of the pivot.
	* @param bLimitedRange Whether the custom pivot position is restricted to be inside the bounding box.
	* @return The new coordination of the pivot of the mesh in world space.
	*/
	static FVector CustomizePivotPosition(UStaticMesh* const StaticMesh, const FVector& PivotPosition, const bool bLimitedRange);

	/**
	* Performs a translation to the origin then transforms the mesh using the @PostTransform Transformation and finally transforms the mesh back.
	*
	* @param Mesh The output mesh.
	* @param PostTransform The desired transformation.
	*/
	static void ApplyPostTransformForVista(UStaticMesh* const StaticMesh, const FTransform& PostTransform);
};
