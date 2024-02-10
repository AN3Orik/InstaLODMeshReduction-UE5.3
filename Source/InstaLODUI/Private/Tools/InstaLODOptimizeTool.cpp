/**
 * InstaLODOptimizeTool.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODOptimizeTool.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODOptimizeTool.h"
#include "InstaLODUIPCH.h"

#include "RawMesh.h"
#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"

#include "Utilities/InstaLODUtilities.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

UInstaLODOptimizeTool::UInstaLODOptimizeTool() : Super(),
BakePose(nullptr),
Operation(nullptr),
OperationResult()
{
	// initialize state
	SetActiveSettingsIndex(SettingsCheckBoxIndex, false);
}

void UInstaLODOptimizeTool::OnNewSelection()
{
	Super::OnNewSelection();
	BakePose = nullptr;
}

void UInstaLODOptimizeTool::SetActiveSettingsIndex(int32 NewIndex, bool bForceSave)
{
	SettingsCheckBoxIndex = NewIndex;

	// update used method based on setting
	switch (NewIndex)
	{
	case 0:
		bUsePercentTriangles = true;
		bUseAbsoluteTriangles = false;
		bUseScreenSizeInPixels = false;
		break;
	case 1:
		bUsePercentTriangles = false;
		bUseAbsoluteTriangles = true;
		bUseScreenSizeInPixels = false;
		break;
	case 2:
		bUsePercentTriangles = false;
		bUseAbsoluteTriangles = false;
		bUseScreenSizeInPixels = true;
		break;
	default:
		bUsePercentTriangles = true;
		bUseAbsoluteTriangles = false;
		bUseScreenSizeInPixels = false;
		break;
	}

	if (bForceSave)
	{
		SaveConfig();
	}
}

FText UInstaLODOptimizeTool::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "OptimizeToolFriendlyName", "OP");
}

FText UInstaLODOptimizeTool::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "OptimizeToolComboBoxItemName", "Optimize");
}

FText UInstaLODOptimizeTool::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "OptimizeToolOperationInformation", "The Optimize operation carefully removes polygons and relocates vertices until your polygon target has been met. Mesh attributes, such as normals or UV coordinates, are adjusted to preserve the appearence of the mesh.");
}

int32 UInstaLODOptimizeTool::GetOrderId() const
{
	return 1;
}

FSkeletalMeshOptimizationSettings UInstaLODOptimizeTool::GetSkeletalMeshOptimizationSettings(const int32 BaseLOD)
{
	FSkeletalMeshOptimizationSettings Settings;

	Settings.SkinningImportance = (SkeletalMeshOptimizationImportance)SkinningImportance;
	Settings.ShadingImportance = (SkeletalMeshOptimizationImportance)ShadingImportance;
	Settings.TextureImportance = (SkeletalMeshOptimizationImportance)TextureImportance;
	Settings.SilhouetteImportance = (SkeletalMeshOptimizationImportance)SilhouetteImportance;
	
#if defined(INSTALOD_SKELETON_OPTIMIZE)
	Settings.MaxBonesPerVertex = MaxInfluencePerVertex;
#endif
	
	Settings.NumOfTrianglesPercentage = PercentTriangles;
	Settings.NormalsThreshold = HardAngleThreshold;
	Settings.MaxDeviationPercentage = MaximumDeviation;
	Settings.WeldingThreshold = WeldingDistance;
	Settings.bRecalcNormals = bRecalculateNormals;
	Settings.BaseLOD = BaseLOD;

	return Settings;
}

void UInstaLODOptimizeTool::ResetSettings()
{
	SetActiveSettingsIndex(0, false);
	
	AutomaticQuality = EInstaLODImportance::InstaLOD_OFF;

	PercentTriangles = 50.0f;
	AbsoluteTriangles = 4;
	ScreenSizeInPixels = 300;
	MaximumDeviation = 0.0f;
	
	BoundaryImportance = EInstaLODImportance::InstaLOD_Normal;
	TextureImportance = EInstaLODImportance::InstaLOD_Normal;
	ShadingImportance = EInstaLODImportance::InstaLOD_Normal;
	SilhouetteImportance = EInstaLODImportance::InstaLOD_Normal;
	SkinningImportance = EInstaLODImportance::InstaLOD_Normal;
	
	bRecalculateNormals = false;
	HardAngleThreshold = 80.f;

	bProtectSplits = true;
	bProtectBoundaries = true;
	bOptimalPlacement = true;
	bNormalizeMeshScale = false;
	bDeterministic = false;
	
#if defined(INSTALOD_SKELETON_OPTIMIZE)
	BakePose = nullptr;
	LeafBoneWeldDistance = 0.0f;
	MaximumBoneDepth = 0;
	MaxInfluencePerVertex = 0;
	MinInfluenceThreshold = 0.0f;
	IgnoreJointRegEx = "";
#endif
	WeldingDistance = 0.0f;
	TJunctionHealingDistance = 0.0f;
	bLockSplits = false;
	bLockBoundaries = false;
	bVertexColorsAsOptimizerWeights = false;

	// Reset Parent which ultimately ends in a SaveConfig() call to reset everything
	Super::ResetSettings();
}

void UInstaLODOptimizeTool::OnMeshOperationExecute(bool bIsAsynchronous)
{
	// --------------------------
	// can be run on child thread
	// --------------------------
	check(Operation == nullptr);

	static FScopedSlowTask* TaskProgress = nullptr;
	TaskProgress = SlowTaskProgress;
	InstaLOD::pfnOptimizationProgressCallback ProgressCallback = [](InstaLOD::IOptimizeOperation *, const InstaLOD::IInstaLODMesh* , InstaLOD::IInstaLODMesh* ,
																	const float ProgressInPercent, const uint32, const uint32 )
		{
			static float LastProgress = 0.0f;

			if (FMath::IsNearlyEqual(LastProgress, ProgressInPercent, KINDA_SMALL_NUMBER) || ProgressInPercent < 0.0f)
				return;

			if (LastProgress >= 1.0f)
			{
				LastProgress = 0.0f;
				return;
			}

			const float DeltaProgress = (ProgressInPercent - LastProgress) * 100.0f;
			LastProgress = ProgressInPercent;

			if (!IsInGameThread())
			{
				AsyncTask(ENamedThreads::GameThread, [DeltaProgress]()
						  {
								TaskProgress->EnterProgressFrame(DeltaProgress);
						  });
			}
			else
			{
				TaskProgress->EnterProgressFrame(DeltaProgress);
			}
		};

	if (bVertexColorsAsOptimizerWeights)
	{
		const bool bDidCreateOptimizerWeights = InputMesh->ConvertColorDataToOptimizerWeights(0);
	
		if (!bDidCreateOptimizerWeights)
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Failed to convert vertex colors to optimizer weights."));
		}
	}
	
	// alloc mesh operation
	Operation = GetInstaLODInterface()->GetInstaLOD()->AllocOptimizeOperation();
	Operation->SetProgressCallback(ProgressCallback);

	InstaLOD::OptimizeSettings OptimizeSettings = GetOptimizeSettings();
	
	bool bIsSkeletalOptimizationEnabled = bSingleSkeletalMeshSelected && Skeleton != nullptr &&
	(OptimizeSettings.SkeletonOptimize.LeafBoneWeldDistance != 0 ||
	OptimizeSettings.SkeletonOptimize.MaximumBoneDepth != 0 ||
	OptimizeSettings.SkeletonOptimize.MinimumBoneInfluenceThreshold != 0 ||
	OptimizeSettings.SkeletonOptimize.MaximumBoneInfluencesPerVertex != 0);

	TArray<uint32> IgnoreJointIndices;

	if(bIsSkeletalOptimizationEnabled)
	{
		OptimizeSettings.Skeleton = Skeleton;

		if(!IgnoreJointRegEx.IsEmpty())
		{
			// find matching strings
			FRegexPattern RegexPattern(IgnoreJointRegEx);

			for (TPair<int32, TPair<uint32, FString>> Values : UEBoneIndexToInstaLODBoneIndexAndName)
			{
				FRegexMatcher Match(RegexPattern, Values.Value.Value);

				if(Match.FindNext())
					IgnoreJointIndices.Push(Values.Value.Key);
			}

			OptimizeSettings.SkeletonOptimize.IgnoreJointIndices = IgnoreJointIndices.GetData();
			OptimizeSettings.SkeletonOptimize.IgnoreJointIndicesCount = IgnoreJointIndices.Num();
		}
	}

	// update reduction selection
	SetActiveSettingsIndex(GetActiveSettingsIndex(), false);

	// execute
	OperationResult = Operation->Execute(InputMesh, OutputMesh, OptimizeSettings);
}

bool UInstaLODOptimizeTool::IsMeshOperationSuccessful() const
{
	return Operation != nullptr && OperationResult.Success;
}

void UInstaLODOptimizeTool::DeallocMeshOperation()
{
	check(Operation);
	GetInstaLODInterface()->GetInstaLOD()->DeallocOptimizeOperation(Operation);
	Operation = nullptr;
}

InstaLOD::OptimizeSettings UInstaLODOptimizeTool::GetOptimizeSettings()
{
	InstaLOD::OptimizeSettings ReturnSettings;

	ReturnSettings.AutomaticQuality = (InstaLOD::MeshFeatureImportance::Type)AutomaticQuality;

	if (bUsePercentTriangles)
	{
		ReturnSettings.PercentTriangles = PercentTriangles / 100;
		UE_LOG(LogTemp, Log, TEXT("## InstaLOD Optimize Settings | Used PercentTriangles ##"));
	}
	else if (bUseAbsoluteTriangles)
	{
		ReturnSettings.AbsoluteTriangles = AbsoluteTriangles;
		UE_LOG(LogTemp, Log, TEXT("## InstaLOD Optimize Settings | Used AbsoluteTriangles ##"));
	}
	else if (bUseScreenSizeInPixels)
	{
		ReturnSettings.ScreenSizeInPixels = ScreenSizeInPixels;
		UE_LOG(LogTemp, Log, TEXT("## InstaLOD Optimize Settings | Used ScreenSizeInPixels ##"));
	}
	ReturnSettings.MaxDeviation = MaximumDeviation;
	ReturnSettings.AlgorithmStrategy = InstaLOD::AlgorithmStrategy::Smart_v2;

	// TODO: Wrap with function to ignore order problems
	ReturnSettings.TextureImportance = (InstaLOD::MeshFeatureImportance::Type)TextureImportance;
	ReturnSettings.ShadingImportance = (InstaLOD::MeshFeatureImportance::Type)ShadingImportance;
	ReturnSettings.SkinningImportance = (InstaLOD::MeshFeatureImportance::Type)SkinningImportance;
	ReturnSettings.SilhouetteImportance = (InstaLOD::MeshFeatureImportance::Type)SilhouetteImportance;

	ReturnSettings.RecalculateNormals = bRecalculateNormals;
	ReturnSettings.HardAngleThreshold = HardAngleThreshold;

#if defined(INSTALOD_SKELETON_OPTIMIZE)
	ReturnSettings.SkeletonOptimize.LeafBoneWeldDistance = LeafBoneWeldDistance;
	ReturnSettings.SkeletonOptimize.MaximumBoneDepth = MaximumBoneDepth;
	ReturnSettings.SkeletonOptimize.MaximumBoneInfluencesPerVertex = MaxInfluencePerVertex;
	ReturnSettings.SkeletonOptimize.MinimumBoneInfluenceThreshold = MinInfluenceThreshold;

	// 
	//ReturnSettings.SkeletonOptimize.IgnoreJointIndices = 
#endif
	
	ReturnSettings.WeldingThreshold = WeldingDistance;
	ReturnSettings.HealTJunctionThreshold = TJunctionHealingDistance;

	ReturnSettings.LockSplits = bLockSplits;
	ReturnSettings.LockBoundaries = bLockBoundaries;
	ReturnSettings.ProtectSplits = bProtectSplits;
	ReturnSettings.ProtectBoundaries = bProtectBoundaries;
	ReturnSettings.OptimalPlacement = bOptimalPlacement;
	ReturnSettings.NormalizeMeshScale = bNormalizeMeshScale;
	ReturnSettings.OptimizerVertexWeights = bVertexColorsAsOptimizerWeights;
	ReturnSettings.Deterministic = bDeterministic;

	return ReturnSettings;
}

bool UInstaLODOptimizeTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!UInstaLODBaseTool::IsValidJSONObject(JsonObject, "Optimize"))
		return false;

	const TSharedPtr<FJsonObject>* SettingsObjectPointer = nullptr;

	if (!JsonObject->TryGetObjectField(FString("Settings"), SettingsObjectPointer) ||
		SettingsObjectPointer == nullptr ||
		!SettingsObjectPointer->IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: Could not retrieve Settings field."));
		return false;
	}

	const TSharedPtr<FJsonObject>& SettingsObject = *SettingsObjectPointer;

	if (SettingsObject->HasField("AutomaticQuality"))
	{
		const FString quality = SettingsObject->GetStringField("AutomaticQuality");
		AutomaticQuality = UInstaLODBaseTool::GetImportanceValueForString(quality);
	}
	if (SettingsObject->HasField("AbsoluteTriangles"))
	{
		AbsoluteTriangles = SettingsObject->GetIntegerField("AbsoluteTriangles");
	}
	if (SettingsObject->HasField("MaxDeviation"))
	{
		MaximumDeviation = SettingsObject->GetNumberField("MaxDeviation");
	}
	if (SettingsObject->HasField("HardAngleThreshold"))
	{
		HardAngleThreshold = SettingsObject->GetNumberField("HardAngleThreshold"); 
	}
	if (SettingsObject->HasField("PercentTriangles"))
	{
		PercentTriangles = (float)SettingsObject->GetNumberField("PercentTriangles") * 100.0f;
	}
	if (SettingsObject->HasField("ScreenSizeInPixels"))
	{
		ScreenSizeInPixels = SettingsObject->GetIntegerField("ScreenSizeInPixels");
	}
	if (SettingsObject->HasField("RecalculateNormals"))
	{
		bRecalculateNormals = SettingsObject->GetBoolField("RecalculateNormals");
	}
	if (SettingsObject->HasField("LockSplits"))
	{
		bLockSplits = SettingsObject->GetBoolField("LockSplits");
	}
	if (SettingsObject->HasField("LockBoundaries"))
	{
		bLockBoundaries = SettingsObject->GetBoolField("LockBoundaries");
	}
	if (SettingsObject->HasField("ProtectSplits"))
	{
		bProtectSplits = SettingsObject->GetBoolField("ProtectSplits");
	}
	if (SettingsObject->HasField("ProtectBoundaries"))
	{
		bProtectBoundaries = SettingsObject->GetBoolField("ProtectBoundaries");
	}
	if (SettingsObject->HasField("OptimalPlacement"))
	{
		bOptimalPlacement = SettingsObject->GetBoolField("OptimalPlacement");
	}
	if (SettingsObject->HasField("NormalizeMeshScale"))
	{
		bNormalizeMeshScale = SettingsObject->GetBoolField("NormalizeMeshScale");
	}
	if (SettingsObject->HasField("HealTJunctionThreshold"))
	{
		TJunctionHealingDistance = SettingsObject->GetNumberField("HealTJunctionThreshold");
	}
	if (SettingsObject->HasField("BoundaryImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("BoundaryImportance");
		BoundaryImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("ShadingImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("ShadingImportance");
		ShadingImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("SilhouetteImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("SilhouetteImportance");
		SilhouetteImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("SkinningImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("SkinningImportance");
		SkinningImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("TextureImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("TextureImportance");
		TextureImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("Deterministic"))
	{
		bDeterministic = SettingsObject->GetBoolField("Deterministic");
	}
	if (SettingsObject->HasField("SkeletonOptimize"))
	{
		const TSharedPtr<FJsonObject>& SkeletonOptimize = SettingsObject->GetObjectField("SkeletonOptimize");

		if (SkeletonOptimize->HasField("LeafBoneWeldDistance"))
		{
			LeafBoneWeldDistance = (float) SkeletonOptimize->GetNumberField("LeafBoneWeldDistance");
		}
		if (SkeletonOptimize->HasField("MaximumBoneDepth"))
		{
			MaximumBoneDepth = SkeletonOptimize->GetIntegerField("MaximumBoneDepth");
		}
		if (SkeletonOptimize->HasField("MaximumBoneInfluencesPerVertex"))
		{
			MaxInfluencePerVertex = SkeletonOptimize->GetIntegerField("MaximumBoneInfluencesPerVertex");
		}
		if (SkeletonOptimize->HasField("MinimumBoneInfluenceThreshold"))
		{
			MinInfluenceThreshold = (float) SkeletonOptimize->GetNumberField("MinimumBoneInfluenceThreshold");
		}
		if (SkeletonOptimize->HasField("IgnoreJointRegEx"))
		{
			IgnoreJointRegEx = SkeletonOptimize->GetStringField("IgnoreJointRegEx");
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
