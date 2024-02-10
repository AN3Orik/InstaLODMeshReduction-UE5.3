/**
 * InstaLODScriptWrapper.cpp (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODScriptWrapper.cpp
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "Scripting/InstaLODScriptWrapper.h"
#include "InstaLOD/InstaLODMeshExtended.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "InstaLODModule.h"
#include "Utilities/InstaLODUtilities.h"
#include "Tools/InstaLODBaseTool.h"
#include "Misc/ScopeExit.h"

#include "Scripting/Settings/InstaLODImposterizeSettings.h"
#include "Scripting/Settings/InstaLODMaterialMergeSettings.h"
#include "Scripting/Settings/InstaLODOcclusionCullSettings.h"
#include "Scripting/Settings/InstaLODUnwrapSettings.h"
#include "Scripting/Settings/InstaLODRemeshSettings.h"
#include "Scripting/Settings/InstaLODIsotropicRemeshSettings.h"
#include "Scripting/Settings/InstaLODOptimizeSettings.h"
#include "Scripting/Settings/InstaLODMeshToolKitSettings.h"
#include "Scripting/Settings/InstaLODResultSettings.h"
#include "Scripting/Settings/InstaLODMaterialSettings.h"

namespace InstaLODScriptUtilities
{
	/**
	 * Generates the Cloud polygonal mesh.
	 *
	 * @param Mesh The InstaLODMesh.
	 * @param CloudPolyMesh The cloud poly mesh.
	 * @param UniqueMaterials The unique materials.
	 * @param HybridBillboardCloudPolySuffix The cloud poly suffix.
	 * @param InstaLODInterface The InstaLOD interface.
	 * @return True upon success.
	 */
	static bool GenerateCloudPolygonalMesh(InstaLOD::IInstaLODMesh* const Mesh, InstaLOD::IInstaLODMeshExtended* const CloudPolyMesh, TArray<UMaterialInterface*>& UniqueMaterials, const FString& HybridBillboardCloudPolySuffix, IInstaLOD* const InstaLODInterface)
	{
		check(Mesh != nullptr);
		check(InstaLODInterface != nullptr);
		check(CloudPolyMesh != nullptr);

		if (Mesh == nullptr || InstaLODInterface == nullptr || CloudPolyMesh == nullptr)
			return false;

		// NOTE: results in an empty cloud poly mesh
		if (HybridBillboardCloudPolySuffix.IsEmpty())
			return true;

		uint64 FaceCount;
		int32* const FaceMaterials = Mesh->GetFaceMaterialIndices(&FaceCount);

		TArray<int32> CloudPolySections;
		TArray<int32> BillboardSections;

		for (uint64 FaceIndex = 0lu; FaceIndex < FaceCount; FaceIndex++)
		{
			const int32 MaterialIndex = FaceMaterials[FaceIndex];
			check(UniqueMaterials.IsValidIndex(MaterialIndex));
			UMaterialInterface* const Material = UniqueMaterials[MaterialIndex];

			if (Material->GetName().EndsWith(HybridBillboardCloudPolySuffix))
			{
				CloudPolySections.AddUnique(MaterialIndex);
			}
			else
			{
				BillboardSections.AddUnique(MaterialIndex);
			}
		}

		if (CloudPolySections.Num() > 0)
		{
			InstaLOD::IInstaLODMeshExtended* const TempMesh = static_cast<InstaLOD::IInstaLODMeshExtended*>(InstaLODInterface->AllocInstaLODMesh());
			InstaLOD::IInstaLODMeshExtended* const SubMesh = static_cast<InstaLOD::IInstaLODMeshExtended*>(InstaLODInterface->AllocInstaLODMesh());

			// copy the material IDs to the submesh IDs so we can extract by material ID
			TempMesh->ResizeFaceSubMeshIndices(FaceCount);
			TempMesh->AppendMesh(Mesh);

			int32* const TempFaceMaterials = TempMesh->GetFaceMaterialIndices(&FaceCount);
			uint32* const TempFaceSubmeshIndices = TempMesh->GetFaceSubMeshIndices(&FaceCount);
			for (uint64 FaceIndex = 0; FaceIndex < FaceCount; FaceIndex++)
			{
				TempFaceSubmeshIndices[FaceIndex] = TempFaceMaterials[FaceIndex];
			}

			for (const int32 MaterialID : CloudPolySections)
			{
				TempMesh->ExtractSubMesh(MaterialID, SubMesh);
				CloudPolyMesh->AppendMesh(SubMesh);
			}

			Mesh->Clear();
			for (const int32 MaterialID : BillboardSections)
			{
				TempMesh->ExtractSubMesh(MaterialID, SubMesh);
				Mesh->AppendMesh(SubMesh);
			}

			InstaLODInterface->GetInstaLOD()->DeallocMesh(TempMesh);
			InstaLODInterface->GetInstaLOD()->DeallocMesh(SubMesh);
		}

		return true;
	}

	/**
	 * Evaluates the result base and target LOD index and eventually fixes invalid values.
	 * 
	 * @param Entry The optimize target.
	 * @param BaseLODIndex The base LOD index.
	 * @param TargetLODIndex The target LOD index.
	 * @param MeshComponent The target mesh component.
	 * @return True if the indices are valid.
	 */
	static bool EvaluateResultBaseAndTargetLODIndex (UObject* const Entry, int32& BaseLODIndex, int32& TargetLODIndex, TSharedPtr<FInstaLODMeshComponent>& MeshComponent)
	{
		check(Entry);

		bool bHasWrongSpecification = false;
		if (Entry->IsA<UStaticMesh>())
		{
			UStaticMeshComponent* const StaticMeshComponent = NewObject<UStaticMeshComponent>();
			StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(Entry));
			MeshComponent = TSharedPtr<FInstaLODMeshComponent>(new FInstaLODMeshComponent(StaticMeshComponent));
			UStaticMesh* const StaticMesh = StaticMeshComponent->GetStaticMesh();

			if (BaseLODIndex < 0 || BaseLODIndex >= StaticMesh->GetNumSourceModels())
			{
				UE_LOG(LogTemp, Warning, TEXT("BaseLOD index out of bounds. Setting Base LOD Index to 0"));
				BaseLODIndex = 0;
				bHasWrongSpecification = true;
			}

			// NOTE: we don't allow adding to LOD index 0
			if (TargetLODIndex < 1 || TargetLODIndex > StaticMesh->GetNumSourceModels())
			{
				UE_LOG(LogTemp, Warning, TEXT("Wrong TargetLOD Index. Target LOD Index will be set to last Index in LOD Chain."));
				TargetLODIndex = StaticMesh->GetNumSourceModels();
				bHasWrongSpecification = true;
			}
		}
		else if (Entry->IsA<USkeletalMesh>())
		{
			USkeletalMeshComponent* const SkeletalMeshComponent = NewObject<USkeletalMeshComponent>();
			SkeletalMeshComponent->SetSkeletalMesh(Cast<USkeletalMesh>(Entry));
			MeshComponent = TSharedPtr<FInstaLODMeshComponent>(new FInstaLODMeshComponent(SkeletalMeshComponent));
			USkeletalMesh* const SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();

			if (BaseLODIndex < 0 || BaseLODIndex > SkeletalMesh->GetLODInfoArray().Num() - 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("BaseLOD index out of bounds. Setting Base LOD Index to 0"));
				BaseLODIndex = 0;
				bHasWrongSpecification = true;
			}

			if (TargetLODIndex < SkeletalMesh->GetLODInfoArray().Num())
			{
				UE_LOG(LogTemp, Warning, TEXT("Wrong TargetLOD index. Appending LOD at the end of the LOD chain."));
				TargetLODIndex = SkeletalMesh->GetLODInfoArray().Num();
				bHasWrongSpecification = true;
			}
		}
		else
		{
			check(false);
			return false;
		}
		return !bHasWrongSpecification;
	}

	/**
	 * Determines whether the UOBject is valid.
	 * 
	 * @param Entry The entry.
	 * @return True if valid.
	 */
	static bool IsEntryValid(UObject* const Entry)
	{
		if (Entry == nullptr)
		{
			UE_LOG(LogInstaLOD, Error, TEXT("Object is null."));
			return false;
		}

		if (!Entry->IsA<UStaticMesh>() && !Entry->IsA<USkeletalMesh>())
		{
			UE_LOG(LogInstaLOD, Error, TEXT("Unsupported UObject type, item must be of type: 'UStaticMesh' or 'USkeletalMesh'."));
			return false;
		}

		return true;
	}
}

/**
 * The InstaLODScriptOperation encapsulates an InstaLOD
 * Operation.
 */
class InstaLODScriptOperation
{
public:
	InstaLODScriptOperation(const TArray<UObject*>& EntriesArray, const int32 BaseLODIndexValue, UInstaLODResultSettings* const ResultSettingsObject) :
	Entries(EntriesArray),
	BaseLODIndex(BaseLODIndexValue),
	ResultSettings(ResultSettingsObject)
	{
		check(ResultSettingsObject)
	}

	/**
	 * Sets the execute operation callback.
	 *
	 * @param ExecuteOperation The execute operation.
	 */
	void SetExecuteOperation(TFunction<void(InstaLODScriptOperation&, UObject*, IInstaLOD*, UInstaLODScriptResult*, TSharedPtr<FInstaLODMeshComponent>, const int32, const int32)>& InExecuteOperation)
	{
		ExecuteOperation = InExecuteOperation;
	}

	/**
	 * Executes the operation.
	 *
	 * @param ScriptResult The script result object.
	 * @return True upon success.
	 */
	bool Execute(UInstaLODScriptResult* const ScriptResult)
	{ 
		FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
		IInstaLOD* const InstaLODAPI = InstaLODModule.GetInstaLODInterface();
		IInstaLOD* const InstaLODInterface = InstaLODModule.GetInstaLODInterface();
		TArray<UObject*> OutputArray;

		FScopedSlowTask Task(Entries.Num(), NSLOCTEXT("InstaLODUI", "ScriptStart", "InstaLOD Script Operation in progress"));

		for (UObject* const Entry : Entries)
		{
			Task.EnterProgressFrame(1.0f, NSLOCTEXT("InstaLODUI", "ScriptProgress", "InstaLOD Script Operation in progress"));

			if (!InstaLODScriptUtilities::IsEntryValid(Entry))
				continue;

			TSharedPtr<FInstaLODMeshComponent> MeshComponent;
			InstaLODScriptUtilities::EvaluateResultBaseAndTargetLODIndex(Entry, BaseLODIndex, ResultSettings->TargetLODIndex, MeshComponent);
			ExecuteOperation(*this, Entry, InstaLODAPI, ScriptResult, MeshComponent, BaseLODIndex, ResultSettings->TargetLODIndex);
		}

		ScriptResult->OutResults = OutputArray;
		return true;
	}

	TFunction<void (InstaLODScriptOperation&, UObject*, IInstaLOD*, UInstaLODScriptResult*, TSharedPtr<FInstaLODMeshComponent>, const int32, const int32)> ExecuteOperation;	/**< The Execution callback. */
	TArray<UObject*> Entries;	/**< The Entries to optimize. */
	int32 BaseLODIndex;			/**< The base LOD index. */
	UInstaLODResultSettings* ResultSettings;	/**< The result settings object. */
};

UInstaLODScriptResult* UInstaLODScriptWrapper::ImposterizeAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODImposterizeSettings* const ImposterizeSettings, UInstaLODResultSettings* const ResultSettings, UInstaLODBakeOutputSettings* const MaterialSettings)
{
	if (ImposterizeSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();

	UInstaLODScriptResult* const ScriptResult = NewObject<UInstaLODScriptResult>();
	FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
	InstaLOD::IInstaLOD* const InstaLODAPI = InstaLODModule.GetInstaLODAPI();
	IInstaLOD* const InstaLODInterface = InstaLODModule.GetInstaLODInterface();

	InstaLOD::ImposterizeSettings Settings = ImposterizeSettings->GetImposterizeSettings();
	Settings.BakeOutput = MaterialSettings->GetBakeOutputSettings();

	InstaLOD::IImposterizeOperation* const Imposterize = InstaLODAPI->AllocImposterizeOperation();
	FBoxSphereBounds BoundingBox = FBoxSphereBounds(FVector::ZeroVector, FVector::ZeroVector, 0.0f);
	const bool bIsHybridBillboardCloud = ImposterizeSettings->ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud;
	InstaLOD::IInstaLODMeshExtended* CloudPolygonalMesh = nullptr;
	TArray<TSharedPtr<FInstaLODMeshComponent>> MeshComponents;
	TArray<UObject*> ValidEntries;

	if (bIsHybridBillboardCloud)
	{
		CloudPolygonalMesh = static_cast<InstaLOD::IInstaLODMeshExtended*>(InstaLODAPI->AllocMesh());
	}

	for (UObject* const Entry : Entries)
	{
		if (!InstaLODScriptUtilities::IsEntryValid(Entry))
			continue;

		TSharedPtr<FInstaLODMeshComponent> MeshComponent;
		ValidEntries.Add(Entry); 
		InstaLODScriptUtilities::EvaluateResultBaseAndTargetLODIndex(Entry, BaseLODIndex, ResultSettings->TargetLODIndex, MeshComponent);

		if (BoundingBox.SphereRadius == 0)
		{
			BoundingBox = MeshComponent->GetComponent()->CalcBounds(MeshComponent->GetComponent()->GetComponentTransform());
		}
		else
		{
			BoundingBox = BoundingBox + MeshComponent->GetComponent()->CalcBounds(MeshComponent->GetComponent()->GetComponentTransform());
		}

		MeshComponents.Add(MeshComponent);
	}

	const float ComponentsBoundingSphereRadius = BoundingBox.SphereRadius;
	TArray<UMaterialInterface*> UniqueMaterials;
	TArray<InstaLODMergeData> MergeData = UInstaLODUtilities::CreateMergeData(MeshComponents, InstaLODInterface, BaseLODIndex);
	InstaLOD::IInstaLODMaterialData* const MaterialData = InstaLODAPI->AllocMaterialData();
	UInstaLODUtilities::CreateMaterialData(InstaLODInterface, MergeData, MaterialData, MaterialSettings->GetFlattenMaterialSettings()->GetMaterialProxySettings(), UniqueMaterials);

	if (bIsHybridBillboardCloud)
	{
		for (InstaLODMergeData& MergeItem : MergeData)
		{
			InstaLODScriptUtilities::GenerateCloudPolygonalMesh(MergeItem.InstaLODMesh, CloudPolygonalMesh, UniqueMaterials, ImposterizeSettings->HybridCloudPolyMaterialSuffix, InstaLODInterface);
		}
	}

	for (InstaLODMergeData& MergeItem : MergeData)
	{
		Imposterize->AddMesh(MergeItem.InstaLODMesh);
	}

	InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODAPI->AllocMesh();

	if (CloudPolygonalMesh != nullptr)
	{
		Imposterize->AddCloudPolygonalMesh(CloudPolygonalMesh);
	}

	ON_SCOPE_EXIT
	{
		InstaLODAPI->DeallocImposterizeOperation(Imposterize);
		InstaLODAPI->DeallocMesh(OutputInstaLODMesh);
		InstaLODAPI->DeallocMaterialData(MaterialData);

		if (CloudPolygonalMesh != nullptr)
		{
			InstaLODInterface->GetInstaLOD()->DeallocMesh(CloudPolygonalMesh);
		}

		for (InstaLODMergeData& MergeItem : MergeData)
		{
			InstaLODAPI->DeallocMesh(MergeItem.InstaLODMesh);
		}
	};

	Imposterize->SetMaterialData(MaterialData);
	const InstaLOD::ImposterizeResult Result = Imposterize->Execute(OutputInstaLODMesh, Settings);
	TArray<UObject*> OutAssetsToSync;

	if (Result.Success == true)
	{
		const FString Path = TEXT("/Game/") + MaterialSettings->SavePath.Path + TEXT("/") + FGuid::NewGuid().ToString();
		UMaterialInstanceConstant* const BakeMaterial = UInstaLODUtilities::CreateFlattenMaterialInstanceFromInstaMaterial(Result.BakeMaterial, MaterialSettings->GetFlattenMaterialSettings()->GetMaterialProxySettings(), Path, OutAssetsToSync, Settings.Type == (InstaLOD::ImposterType::Type)EInstaLODImposterizeType::InstaLOD_Flipbook);

		if (Settings.Type == (InstaLOD::ImposterType::Type)EInstaLODImposterizeType::InstaLOD_Flipbook)
		{
			BakeMaterial->SetScalarParameterValueEditorOnly(TEXT("FramesPerAxis"), Settings.FlipbookFramesPerAxis);
			BakeMaterial->SetScalarParameterValueEditorOnly(TEXT("SpriteSize"), ComponentsBoundingSphereRadius);
			BakeMaterial->PostEditChange();
		}
		else if (bIsHybridBillboardCloud)
		{
			// we always enable two sided rendering and foliage shading model for billboard cloud imposters
			BakeMaterial->BasePropertyOverrides.TwoSided = true;
			BakeMaterial->BasePropertyOverrides.bOverride_TwoSided = true;
			BakeMaterial->BasePropertyOverrides.ShadingModel = MSM_TwoSidedFoliage;
			BakeMaterial->BasePropertyOverrides.bOverride_ShadingModel = true;
			BakeMaterial->PostEditChange();
		}

		bool bIsSuccessful = UInstaLODUtilities::FinalizeScriptProcessResult(ValidEntries[0], InstaLODInterface, MeshComponents[0], OutputInstaLODMesh, ResultSettings, ScriptResult->OutResults, BakeMaterial, /*bIsFreezingTransformsForMultiSelection:*/true);

		if (bIsSuccessful)
		{
			ScriptResult->bSuccess = true;
		}
		else
		{
			ScriptResult->OutResults.Add(nullptr);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"))
	}

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (UObject* const Item : OutAssetsToSync)
	{
		AssetRegistry.AssetCreated(Item);
	}
	return ScriptResult;
}

UInstaLODScriptResult* UInstaLODScriptWrapper::MaterialMergeAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODMaterialMergeSettings* const MaterialMergeSettings, UInstaLODResultSettings* const ResultSettings, UInstaLODBakeOutputSettings* const MaterialSettings)
{
	if (MaterialMergeSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();

	UInstaLODScriptResult* const ScriptResult = NewObject<UInstaLODScriptResult>();
	FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
	InstaLOD::IInstaLOD* const InstaLODAPI = InstaLODModule.GetInstaLODAPI();
	IInstaLOD* const InstaLODInterface = InstaLODModule.GetInstaLODInterface();

	InstaLOD::MeshMergeSettings Settings = MaterialMergeSettings->GetMaterialMergeSettings();
	InstaLOD::IMeshMergeOperation2* const MaterialMergeOperation = InstaLODAPI->AllocMeshMergeOperation();
	TArray<TSharedPtr<FInstaLODMeshComponent>> MeshComponents;
	TArray<UObject*> ValidEntries;

	for (UObject* const Entry : Entries)
	{
		if (!InstaLODScriptUtilities::IsEntryValid(Entry))
			continue;

		ValidEntries.Add(Entry);
		TSharedPtr<FInstaLODMeshComponent> MeshComponent;
		InstaLODScriptUtilities::EvaluateResultBaseAndTargetLODIndex(Entry, BaseLODIndex, ResultSettings->TargetLODIndex, MeshComponent);

		MeshComponents.Add(MeshComponent);
	}

	TArray<UMaterialInterface*> UniqueMaterials;
	TArray<InstaLODMergeData> MergeData = UInstaLODUtilities::CreateMergeData(MeshComponents, InstaLODInterface, BaseLODIndex);
	InstaLOD::IInstaLODMaterialData* const MaterialData = InstaLODAPI->AllocMaterialData();
	UInstaLODUtilities::CreateMaterialData(InstaLODInterface, MergeData, MaterialData, MaterialSettings->GetFlattenMaterialSettings()->GetMaterialProxySettings(), UniqueMaterials);

	for (InstaLODMergeData& MergeItem : MergeData)
	{
		MaterialMergeOperation->AddMesh(MergeItem.InstaLODMesh);
	}

	InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODAPI->AllocMesh();

	ON_SCOPE_EXIT
	{
		InstaLODAPI->DeallocMeshMergeOperation(MaterialMergeOperation);
		InstaLODAPI->DeallocMesh(OutputInstaLODMesh);
		InstaLODInterface->GetInstaLOD()->DeallocMaterialData(MaterialData);

		for (InstaLODMergeData& MergeItem : MergeData)
		{
			InstaLODAPI->DeallocMesh(MergeItem.InstaLODMesh);
		}
	};

	MaterialMergeOperation->SetMaterialData(MaterialData);
	const InstaLOD::MeshMergeResult Result = MaterialMergeOperation->Execute(OutputInstaLODMesh, Settings);
	TArray<UObject*> OutAssetsToSync;

	if (Result.Success)
	{
		const FString Path = TEXT("/Game/") + MaterialSettings->SavePath.Path + TEXT("/") + FGuid::NewGuid().ToString();
		UMaterialInstanceConstant* BakeMaterial = UInstaLODUtilities::CreateFlattenMaterialInstanceFromInstaMaterial(Result.MergeMaterial, MaterialSettings->GetFlattenMaterialSettings()->GetMaterialProxySettings(), Path, OutAssetsToSync, /*bIsFlipbookMaterial:*/false);

		ScriptResult->bSuccess = UInstaLODUtilities::FinalizeScriptProcessResult(ValidEntries[0], InstaLODInterface, MeshComponents[0], OutputInstaLODMesh, ResultSettings, ScriptResult->OutResults, BakeMaterial);

		if (!ScriptResult->bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"));
			ScriptResult->OutResults.Add(nullptr);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"))
	}

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (UObject* const Item : OutAssetsToSync)
	{
		AssetRegistry.AssetCreated(Item);
	}

	return ScriptResult;
}

UInstaLODScriptResult* UInstaLODScriptWrapper::OcclusionCullAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODOcclusionCullSettings* const OcclusionCullSettings, UInstaLODResultSettings* const ResultSettings)
{
	if (OcclusionCullSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();

	UInstaLODScriptResult* ScriptResult = NewObject<UInstaLODScriptResult>();
	InstaLODScriptOperation ScriptOperation(Entries, BaseLODIndex, ResultSettings);
	InstaLOD::OcclusionCullSettings Settings = OcclusionCullSettings->GetOcclusionCullSettings();

	TFunction <void(InstaLODScriptOperation&, UObject*, IInstaLOD*, UInstaLODScriptResult*, TSharedPtr<FInstaLODMeshComponent>, int32, int32)> Execution = [/*Copy:*/ Settings](InstaLODScriptOperation& Operation, UObject* const Entry, IInstaLOD* const InstaLODAPI, UInstaLODScriptResult* const ScriptResult, TSharedPtr<FInstaLODMeshComponent> MeshComponent, int32 BaseLODIndex, int32 TargetLODIndex)
	{
		// Process occlussion call operation and handle result
		InstaLOD::IInstaLOD* const InstaLODInterface = InstaLODAPI->GetInstaLOD();
		InstaLOD::IOcclusionCullOperation* const OcclusionCull = InstaLODInterface->AllocOcclusionCullOperation();
		InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODInterface->AllocMesh();
		UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(InstaLODAPI, MeshComponent, OutputInstaLODMesh);

		ON_SCOPE_EXIT
		{
			InstaLODInterface->DeallocOcclusionCullOperation(OcclusionCull);
			InstaLODInterface->DeallocMesh(OutputInstaLODMesh);
		};

		const InstaLOD::OcclusionCullResult Result = OcclusionCull->Execute(OutputInstaLODMesh, OutputInstaLODMesh, Settings);

		if (Result.Success == true)
		{
			ScriptResult->bSuccess = UInstaLODUtilities::FinalizeScriptProcessResult(Entry, InstaLODAPI, MeshComponent, OutputInstaLODMesh, Operation.ResultSettings, ScriptResult->OutResults, nullptr);
		}

		if (!ScriptResult->bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"));
			ScriptResult->OutResults.Add(nullptr);
		}
	};
	
	ScriptOperation.SetExecuteOperation(Execution);
	ScriptOperation.Execute(ScriptResult);

	return ScriptResult;
}

UInstaLODScriptResult* UInstaLODScriptWrapper::UVUnwrapAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODUnwrapSettings* const UVUnwrapSettings, UInstaLODResultSettings* const ResultSettings)
{
	if (UVUnwrapSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();

	UInstaLODScriptResult* ScriptResult = NewObject<UInstaLODScriptResult>();
	InstaLODScriptOperation ScriptOperation(Entries, BaseLODIndex, ResultSettings);
	InstaLOD::UnwrapSettings Settings = UVUnwrapSettings->GetUnwrapSettings();

	TFunction <void(InstaLODScriptOperation&, UObject*, IInstaLOD*, UInstaLODScriptResult*, TSharedPtr<FInstaLODMeshComponent>, int32, int32)> Execution = [/*Copy:*/ Settings](InstaLODScriptOperation& Operation, UObject* const Entry, IInstaLOD* const InstaLODAPI, UInstaLODScriptResult* const ScriptResult, TSharedPtr<FInstaLODMeshComponent> MeshComponent, int32 BaseLODIndex, int32 TargetLODIndex)
	{
		// Process occlussion call operation and handle result
		InstaLOD::IInstaLOD* const InstaLODInterface = InstaLODAPI->GetInstaLOD();
		InstaLOD::IUnwrapOperation* const Unwrap = InstaLODInterface->AllocUnwrapOperation();
		InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODInterface->AllocMesh();
		UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(InstaLODAPI, MeshComponent, OutputInstaLODMesh);

		ON_SCOPE_EXIT
		{
			InstaLODInterface->DeallocUnwrapOperation(Unwrap);
			InstaLODInterface->DeallocMesh(OutputInstaLODMesh);
		};

		const InstaLOD::UnwrapResult Result = Unwrap->Execute(OutputInstaLODMesh, OutputInstaLODMesh, Settings);

		if (Result.Success == true)
		{
			ScriptResult->bSuccess = UInstaLODUtilities::FinalizeScriptProcessResult(Entry, InstaLODAPI, MeshComponent, OutputInstaLODMesh, Operation.ResultSettings, ScriptResult->OutResults, nullptr);
		}

		if (!ScriptResult->bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"));
			ScriptResult->OutResults.Add(nullptr);
		}
	};

	ScriptOperation.SetExecuteOperation(Execution);
	ScriptOperation.Execute(ScriptResult);
	return ScriptResult;
}

UInstaLODScriptResult* UInstaLODScriptWrapper::RemeshAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODRemeshSettings* const RemeshSettings, UInstaLODResultSettings* const ResultSettings, UInstaLODBakeOutputSettings* const MaterialSettings)
{
	if (RemeshSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();
	
	FScopedSlowTask Task(Entries.Num(), NSLOCTEXT("InstaLODUI", "ScriptStart", "InstaLOD Script Operation in progress"));

	UInstaLODScriptResult* ScriptResult = NewObject<UInstaLODScriptResult>();
	FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
	InstaLOD::IInstaLOD* const InstaLODAPI = InstaLODModule.GetInstaLODAPI();
	IInstaLOD* const InstaLODInterface = InstaLODModule.GetInstaLODInterface();

	InstaLOD::RemeshingSettings Settings = RemeshSettings->GetRemeshingSettings();
	Settings.BakeOutput = MaterialSettings->GetBakeOutputSettings();
	InstaLOD::IRemeshingOperation* const Remesh = InstaLODAPI->AllocRemeshingOperation();
	TArray<TSharedPtr<FInstaLODMeshComponent>> MeshComponents;
	TArray<UObject*> OutAssetsToSync;
	TArray<UObject*> ValidEntries;

	for (UObject* const Entry : Entries)
	{
		if (!InstaLODScriptUtilities::IsEntryValid(Entry))
			continue;

		ValidEntries.Add(Entry);

		TSharedPtr<FInstaLODMeshComponent> MeshComponent;
		InstaLODScriptUtilities::EvaluateResultBaseAndTargetLODIndex(Entry, BaseLODIndex, ResultSettings->TargetLODIndex, MeshComponent);

		MeshComponents.Add(MeshComponent);
	}

	TArray<UMaterialInterface*> UniqueMaterials;
	TArray<InstaLODMergeData> MergeData = UInstaLODUtilities::CreateMergeData(MeshComponents, InstaLODInterface, BaseLODIndex);
	InstaLOD::IInstaLODMaterialData* const MaterialData = InstaLODAPI->AllocMaterialData();
	UInstaLODUtilities::CreateMaterialData(InstaLODInterface, MergeData, MaterialData, MaterialSettings->GetFlattenMaterialSettings()->GetMaterialProxySettings(), UniqueMaterials);

	for (InstaLODMergeData& MergeItem : MergeData)
	{
		Remesh->AddMesh(MergeItem.InstaLODMesh);
	}

	InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODAPI->AllocMesh();

	Task.EnterProgressFrame(1, NSLOCTEXT("InstaLODUI", "ScriptProgress", "InstaLOD Script Operation in progress"));

	ON_SCOPE_EXIT
	{
		InstaLODAPI->DeallocRemeshingOperation(Remesh);
		InstaLODAPI->DeallocMesh(OutputInstaLODMesh);
		InstaLODAPI->DeallocMaterialData(MaterialData);

		for (InstaLODMergeData& MergeItem : MergeData)
		{
			InstaLODAPI->DeallocMesh(MergeItem.InstaLODMesh);
		}
	};

	Remesh->SetMaterialData(MaterialData);
	const InstaLOD::RemeshingResult Result = Remesh->Execute(OutputInstaLODMesh, Settings);
	
	if (Result.Success == true)
	{
		// Generate unique asset string we'll use as temporary save path
		const FString Path = TEXT("/Game/") + MaterialSettings->SavePath.Path + TEXT("/") + FGuid::NewGuid().ToString();
		UMaterialInstanceConstant* const BakeMaterial = UInstaLODUtilities::CreateFlattenMaterialInstanceFromInstaMaterial(Result.BakeMaterial, MaterialSettings->GetFlattenMaterialSettings()->GetMaterialProxySettings(), Path, OutAssetsToSync, /*bIsFlipbookMaterial:*/false);
		bool bIsSuccessful = UInstaLODUtilities::FinalizeScriptProcessResult(ValidEntries[0], InstaLODInterface, MeshComponents[0], OutputInstaLODMesh, ResultSettings, ScriptResult->OutResults, BakeMaterial, /*bIsFreezingTransformsForMultiSelection:*/true);

		if (bIsSuccessful)
		{
			ScriptResult->bSuccess = true;
		}
		else
		{
			ScriptResult->OutResults.Add(nullptr);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"))
	}

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (UObject* const Item : OutAssetsToSync)
	{
		AssetRegistry.AssetCreated(Item);
	}

	return ScriptResult;
}

UInstaLODScriptResult* UInstaLODScriptWrapper::OptimizeAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODOptimizeSettings* const OptimizeSettings, UInstaLODResultSettings* const ResultSettings)
{
	if (OptimizeSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();

	UInstaLODScriptResult* const ScriptResult = NewObject<UInstaLODScriptResult>();
	InstaLODScriptOperation ScriptOperation(Entries, BaseLODIndex, ResultSettings);
	InstaLOD::OptimizeSettings Settings = OptimizeSettings->GetOptimizeSettings();

	TFunction <void(InstaLODScriptOperation&, UObject*, IInstaLOD*, UInstaLODScriptResult*, TSharedPtr<FInstaLODMeshComponent>, int32, int32)> Execution = [/*Copy:*/ Settings](InstaLODScriptOperation& Operation, UObject* const Entry, IInstaLOD* const InstaLODAPI, UInstaLODScriptResult* const ScriptResult, TSharedPtr<FInstaLODMeshComponent> MeshComponent, int32 BaseLODIndex, int32 TargetLODIndex)
	{
		// Process occlussion call operation and handle result
		InstaLOD::IInstaLOD* const InstaLODInterface = InstaLODAPI->GetInstaLOD(); 
		InstaLOD::IOptimizeOperation* const Optimize = InstaLODInterface->AllocOptimizeOperation();
		InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODInterface->AllocMesh(); 
		UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(InstaLODAPI, MeshComponent, OutputInstaLODMesh);

		ON_SCOPE_EXIT
		{
			InstaLODInterface->DeallocOptimizeOperation(Optimize);
			InstaLODInterface->DeallocMesh(OutputInstaLODMesh);
		};

		const InstaLOD::OptimizeResult Result = Optimize->Execute(OutputInstaLODMesh, OutputInstaLODMesh, Settings);

		if (Result.Success == true)
		{
			ScriptResult->bSuccess = UInstaLODUtilities::FinalizeScriptProcessResult(Entry, InstaLODAPI, MeshComponent, OutputInstaLODMesh, Operation.ResultSettings, ScriptResult->OutResults, nullptr);
		}

		if (!ScriptResult->bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"));
			ScriptResult->OutResults.Add(nullptr);
		}
	};

	ScriptOperation.SetExecuteOperation(Execution);
	ScriptOperation.Execute(ScriptResult);
	return ScriptResult;
}

UInstaLODScriptResult* UInstaLODScriptWrapper::IsotropicRemeshAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODIsotropicRemeshSettings* const IsotropicRemeshSettings, UInstaLODResultSettings* const ResultSettings)
{
	if (IsotropicRemeshSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();

	UInstaLODScriptResult* const ScriptResult = NewObject<UInstaLODScriptResult>();
	InstaLODScriptOperation ScriptOperation(Entries, BaseLODIndex, ResultSettings);
	InstaLOD::IsotropicRemeshingSettings Settings = IsotropicRemeshSettings->GetIsotropicRemeshingSettings();

	TFunction <void(InstaLODScriptOperation&, UObject*, IInstaLOD*, UInstaLODScriptResult*, TSharedPtr<FInstaLODMeshComponent>, int32, int32)> Execution = [/*Copy:*/ Settings](InstaLODScriptOperation& Operation, UObject* const Entry, IInstaLOD* const InstaLODAPI, UInstaLODScriptResult* const ScriptResult, TSharedPtr<FInstaLODMeshComponent> MeshComponent, int32 BaseLODIndex, int32 TargetLODIndex)
	{
		InstaLOD::IInstaLOD* const InstaLODInterface = InstaLODAPI->GetInstaLOD();
		InstaLOD::IIsotropicRemeshingOperation* const IsotropicRemesh = InstaLODInterface->AllocIsotropicRemeshingOperation();
		InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODInterface->AllocMesh();
		UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(InstaLODAPI, MeshComponent, OutputInstaLODMesh);

		ON_SCOPE_EXIT
		{
			InstaLODInterface->DeallocIsotropicRemeshingOperation(IsotropicRemesh);
			InstaLODInterface->DeallocMesh(OutputInstaLODMesh);
		};

		const InstaLOD::IsotropicRemeshingResult Result = IsotropicRemesh->Execute(OutputInstaLODMesh, OutputInstaLODMesh, Settings);

		if (Result.Success == true)
		{
			ScriptResult->bSuccess = UInstaLODUtilities::FinalizeScriptProcessResult(Entry, InstaLODAPI, MeshComponent, OutputInstaLODMesh, Operation.ResultSettings, ScriptResult->OutResults, nullptr);
		}

		if (!ScriptResult->bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"));
			ScriptResult->OutResults.Add(nullptr);
		}
	};

	ScriptOperation.SetExecuteOperation(Execution);
	ScriptOperation.Execute(ScriptResult);
	return ScriptResult;
}

UInstaLODScriptResult* UInstaLODScriptWrapper::MeshToolKitAssets(const TArray<UObject*> Entries, int32 BaseLODIndex, UInstaLODMeshToolKitSettings* const MeshToolKitSettings, UInstaLODResultSettings* const ResultSettings)
{
	if (MeshToolKitSettings == nullptr || ResultSettings == nullptr)
		return NewObject<UInstaLODScriptResult>();

	UInstaLODScriptResult* const ScriptResult = NewObject<UInstaLODScriptResult>();
	InstaLODScriptOperation ScriptOperation(Entries, BaseLODIndex, ResultSettings);
	InstaLOD::MeshToolKitSettings Settings = MeshToolKitSettings->GetMeshToolKitSettings();

	TFunction <void(InstaLODScriptOperation&, UObject*, IInstaLOD*, UInstaLODScriptResult*, TSharedPtr<FInstaLODMeshComponent>, int32, int32)> Execution = [/*Copy:*/ Settings](InstaLODScriptOperation& Operation, UObject* const Entry, IInstaLOD* const InstaLODAPI, UInstaLODScriptResult* const ScriptResult, TSharedPtr<FInstaLODMeshComponent> MeshComponent, int32 BaseLODIndex, int32 TargetLODIndex)
	{
		// Process occlussion call operation and handle result
		InstaLOD::IInstaLOD* const InstaLODInterface = InstaLODAPI->GetInstaLOD();
		InstaLOD::IMeshToolKitOperation* const MTK = InstaLODInterface->AllocMeshToolKitOperation();
		InstaLOD::IInstaLODMesh* const OutputInstaLODMesh = InstaLODInterface->AllocMesh();
		UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(InstaLODAPI, MeshComponent, OutputInstaLODMesh);

		ON_SCOPE_EXIT
		{
			InstaLODInterface->DeallocMeshToolKitOperation(MTK);
			InstaLODInterface->DeallocMesh(OutputInstaLODMesh);
		};

		const InstaLOD::MeshToolKitResult Result = MTK->Execute(OutputInstaLODMesh, OutputInstaLODMesh, Settings);

		if (Result.Success == true)
		{
			ScriptResult->bSuccess = UInstaLODUtilities::FinalizeScriptProcessResult(Entry, InstaLODAPI, MeshComponent, OutputInstaLODMesh, Operation.ResultSettings, ScriptResult->OutResults, nullptr);
		}

		if (!ScriptResult->bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("InstaLOD operation failed!"));
			ScriptResult->OutResults.Add(nullptr);
		}
	};

	ScriptOperation.SetExecuteOperation(Execution);
	ScriptOperation.Execute(ScriptResult);
	return ScriptResult;
}

UInstaLODBakeOutputSettings* UInstaLODScriptWrapper::CreateDefaultImposterizeMaterialSettings()
{
	UInstaLODBakeOutputSettings* ImposterizeMaterialDefaultSettings = NewObject<UInstaLODBakeOutputSettings>();
	ImposterizeMaterialDefaultSettings->FlattenMaterialSettings->BlendMode = BLEND_Masked;
	ImposterizeMaterialDefaultSettings->bBakeTexturePageOpacity = true;
	return ImposterizeMaterialDefaultSettings;
}

UInstaLODBakeOutputSettings* UInstaLODScriptWrapper::CreateDefaultMaterialSettings()
{
	UInstaLODBakeOutputSettings* MaterialDefaultSettings = NewObject<UInstaLODBakeOutputSettings>();
	return MaterialDefaultSettings;
}