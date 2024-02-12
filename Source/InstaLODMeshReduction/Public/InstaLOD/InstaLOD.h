/**
 * InstaLOD.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLOD.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLOD_h
#define InstaLOD_InstaLOD_h

#include "Runtime/Launch/Resources/Version.h"
#include "MeshUtilities.h"
#include "SkeletalMeshReductionSettings.h"
#include "Engine/SkinnedAssetCommon.h"

namespace InstaLOD
{
	class IInstaLOD;
	class IInstaLODMaterialData;
	class IInstaLODMaterial;
	class IInstaLODMesh;
	class IInstaLODSkeleton;
};

struct UE_SkeletalBakePoseData
{
	UE_SkeletalBakePoseData() : BakePoseAnimation(nullptr), ReferenceSkeleton(nullptr), SkeletalMesh(nullptr)
	{}
	const UAnimSequence* BakePoseAnimation;
	FReferenceSkeleton* ReferenceSkeleton;
	USkeletalMesh* SkeletalMesh;
};

class IInstaLOD
{
public:

	using UE_MaterialProxySettings = ::FMaterialProxySettings; 
	
	using UE_SkeletalMeshResource = class ::FSkeletalMeshModel;
	using UE_StaticLODModel = class ::FSkeletalMeshLODModel;
	
	using UE_MeshBuildOptions = IMeshUtilities::MeshBuildOptions;
	
	virtual ~IInstaLOD() {}
	
	virtual InstaLOD::IInstaLODMesh* AllocInstaLODMesh() = 0;
	virtual InstaLOD::IInstaLODSkeleton* AllocInstaLODSkeleton() = 0;
	
	virtual bool ConvertInstaLODMeshToRawMesh(InstaLOD::IInstaLODMesh* InMesh, struct FRawMesh &OutMesh) = 0;
	virtual bool ConvertMeshDescriptionToInstaLODMesh(const struct FMeshDescription &InMesh, InstaLOD::IInstaLODMesh* OutMesh) = 0;
	virtual bool ConvertInstaLODMeshToMeshDescription(InstaLOD::IInstaLODMesh* InMesh, const TMap<int32, FName> &MaterialMapOut, struct FMeshDescription &OutMesh) = 0;
	virtual bool ConvertSkeletalLODModelToInstaLODMesh(const UE_StaticLODModel& InMesh, InstaLOD::IInstaLODMesh *const OutMesh, UE_SkeletalBakePoseData *const BakePoseData = nullptr) = 0;
	virtual bool ConvertInstaLODMeshToSkeletalLODModel(InstaLOD::IInstaLODMesh *const InMesh, class USkeletalMesh* SourceSkeletalMesh, class FSkeletalMeshImportData& ImportData, UE_StaticLODModel& OutMesh) = 0;
	
	virtual bool ConvertFlattenMaterialsToInstaLODMaterialData(const TArray<FFlattenMaterial>& InputMaterials, InstaLOD::IInstaLODMaterialData* OutMaterialData, const UE_MaterialProxySettings& MaterialSettings) = 0;
	virtual bool ConvertInstaLODMaterialToFlattenMaterial(InstaLOD::IInstaLODMaterial* InMaterial, FFlattenMaterial &OutMaterial, const UE_MaterialProxySettings& MaterialSettings) = 0;
	
	virtual bool ConvertReferenceSkeletonToInstaLODSkeleton(const FReferenceSkeleton& ReferenceSkeleton, InstaLOD::IInstaLODSkeleton *const InstaSkeleton, TMap<int32, TPair<uint32, FString>>& OutUEBoneIndexToInstaLODBoneIndex) = 0;
	virtual void UnbindClothAtLODIndex(USkeletalMesh* SkeletalMesh, const int32 LODIndex) = 0;

	virtual InstaLOD::IInstaLOD* GetInstaLOD() = 0;
};

class FInstaLOD : public IMeshReduction, public IMeshMerging, public IInstaLOD
{
public:
	virtual ~FInstaLOD() { }
	
	virtual const FString& GetVersionString() const override
	{
		return VersionString;
	}

	virtual bool IsReductionActive(const struct FMeshReductionSettings &ReductionSettings) const override
	{
		return ReductionSettings.PercentTriangles != 1.0f;
	}

	virtual bool IsReductionActive(const struct FMeshReductionSettings& ReductionSettings, uint32 NumVertices, uint32 NumTriangles) const override
	{
		return ReductionSettings.PercentTriangles != 1.0f;
	}

	virtual bool IsReductionActive(const struct FSkeletalMeshOptimizationSettings &ReductionSettings) const override
	{		
		return ReductionSettings.NumOfTrianglesPercentage != 1.0f;
	}
	
	virtual bool IsReductionActive(const struct FSkeletalMeshOptimizationSettings &ReductionSettings, uint32 NumVertices, uint32 NumTriangles) const override
	{
		return ReductionSettings.NumOfTrianglesPercentage != 1.0f;
	}
	
	virtual FString GetName() override
	{
		return TEXT("InstaLOD");
	}

	virtual void ReduceMeshDescription(FMeshDescription& OutReducedMesh, float& OutMaxDeviation, const FMeshDescription& InMesh,
									   const FOverlappingCorners& InOverlappingCorners, const struct FMeshReductionSettings& ReductionSettings);

	virtual bool ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex);

	virtual bool ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex, const struct FSkeletalMeshOptimizationSettings& Settings,
									bool bCalcLODDistance, bool bReregisterComponent = true, const class ITargetPlatform* TargetPlatform = nullptr);

	virtual bool ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex,
									const struct FSkeletalMeshOptimizationSettings& Settings, bool bCalcLODDistance, const class ITargetPlatform* TargetPlatform);

	virtual bool ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex, const class ITargetPlatform* TargetPlatform);

	virtual bool IsSupported() const override
	{
		return true;
	}
	
	virtual InstaLOD::IInstaLODMesh* AllocInstaLODMesh();
	virtual InstaLOD::IInstaLODSkeleton* AllocInstaLODSkeleton();
	
	virtual bool ConvertInstaLODMeshToRawMesh(InstaLOD::IInstaLODMesh* InMesh, struct FRawMesh &OutMesh);
	virtual bool ConvertInstaLODMeshToMeshDescription(InstaLOD::IInstaLODMesh* InMesh, const TMap<int32, FName> &MaterialMapOut, struct FMeshDescription &OutMesh);
	virtual bool ConvertMeshDescriptionToInstaLODMesh(const struct FMeshDescription &InMesh, InstaLOD::IInstaLODMesh* OutMesh);
	
	virtual bool ConvertSkeletalLODModelToInstaLODMesh(const UE_StaticLODModel& InMesh, InstaLOD::IInstaLODMesh *const OutMesh, UE_SkeletalBakePoseData *const BakePoseData = nullptr);
	virtual bool ConvertInstaLODMeshToSkeletalLODModel(InstaLOD::IInstaLODMesh *const InMesh, class USkeletalMesh* SourceSkeletalMesh, class FSkeletalMeshImportData& ImportData, UE_StaticLODModel& OutMesh);
	
	virtual bool ConvertFlattenMaterialsToInstaLODMaterialData(const TArray<FFlattenMaterial>& InputMaterials, InstaLOD::IInstaLODMaterialData* OutMaterialData, const UE_MaterialProxySettings& MaterialSettings);
	virtual bool ConvertInstaLODMaterialToFlattenMaterial(InstaLOD::IInstaLODMaterial* InMaterial, FFlattenMaterial &OutMaterial, const UE_MaterialProxySettings& MaterialSettings);
	
	virtual bool ConvertReferenceSkeletonToInstaLODSkeleton(const FReferenceSkeleton& ReferenceSkeleton, InstaLOD::IInstaLODSkeleton *const InstaSkeleton, TMap<int32, TPair<uint32, FString>>& OutUEBoneIndexToInstaLODBoneIndex);

	virtual void ProxyLOD(const TArray<struct FMeshMergeData>& InData, const struct FMeshProxySettings& InProxySettings,
						  const TArray<struct FFlattenMaterial>& InputMaterials, const FGuid InJobGUID) override;
		
	virtual void AggregateLOD();

	virtual void UnbindClothAtLODIndex(USkeletalMesh* SkeletalMesh, const int32 LODIndex) override;
	
	static FInstaLOD* Create(InstaLOD::IInstaLOD *InstaLODAPI)
	{
		return new FInstaLOD(InstaLODAPI);
	}
		
	virtual InstaLOD::IInstaLOD* GetInstaLOD() 
	{
		return InstaLOD;
	}
	
private:
	explicit FInstaLOD(InstaLOD::IInstaLOD *InstaLODAPI);
	
	void ReduceSkeletalMeshAtLODIndex(class USkeletalMesh* SkeletalMesh, int32 LODIndex,
									  const FSkeletalMeshOptimizationSettings& Settings, bool bCalcLODDistance, const class ITargetPlatform* TargetPlatform);
	
	void PostProcessReducedSkeletalMesh(class USkeletalMesh* SkeletalMesh, UE_StaticLODModel& OutputLODModel, float& OutMaxDeviation,
										const FSkeletalMeshOptimizationSettings& Settings, const float ElapsedTime);
	
	void PostProcessMergedRawMesh(FMeshDescription& OutProxyMesh, FFlattenMaterial& OutMaterial, const FMeshProxySettings& InProxySettings, const float ElapsedTime);
	
	void DispatchNotification(const FText& InText, /*SNotificationItem::ECompletionState*/int32 type);
	
	FString VersionString;
	InstaLOD::IInstaLOD *InstaLOD;
};

#endif
