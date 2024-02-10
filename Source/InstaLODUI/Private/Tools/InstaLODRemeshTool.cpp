/**
 * InstaLODRemeshTool.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODRemeshTool.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODRemeshTool.h"
#include "InstaLODUIPCH.h"

#include "RawMesh.h"
#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "RawMesh.h"

#include "Utilities/InstaLODUtilities.h"
#include "Slate/InstaLODWindow.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

UInstaLODRemeshTool::UInstaLODRemeshTool() : Super(),
Operation(nullptr),
OperationResult()
{
	// initialize state
	SetActiveSettingsIndex(SettingsCheckBoxIndex, false);
}

void UInstaLODRemeshTool::SetActiveSettingsIndex(int32 NewIndex, bool bForceSave)
{
	SettingsCheckBoxIndex = NewIndex;

	// updated the used method based on the index
	switch (SettingsCheckBoxIndex)
	{
	case 0:
		bUseFaceCountTarget = true;
		bUseMaximumTriangles = false;
		bUseScreenSizeInPixels = false;
		break;
	case 1:
		bUseFaceCountTarget = false;
		bUseMaximumTriangles = true;
		bUseScreenSizeInPixels = false;
		break;
	case 2:
		bUseFaceCountTarget = false;
		bUseMaximumTriangles = false;
		bUseScreenSizeInPixels = true;
		break;
	default:
		bUseFaceCountTarget = true;
		bUseMaximumTriangles = false;
		bUseScreenSizeInPixels = false;
		break;
	}

	if (bForceSave)
	{
		SaveConfig();
	}
}

void UInstaLODRemeshTool::OnMeshOperationExecute(bool bIsAsynchronous)
{
	// --------------------------
	// can be run on child thread
	// --------------------------
	check(Operation == nullptr);

	static FScopedSlowTask* TaskProgress = nullptr;
	TaskProgress = SlowTaskProgress;
	InstaLOD::pfnRemeshProgressCallback ProgressCallback  = [](class InstaLOD::IRemeshingOperation *, InstaLOD::IInstaLODMesh*, const float ProgressInPercent)
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

	// alloc mesh operation
	Operation = GetInstaLODInterface()->GetInstaLOD()->AllocRemeshingOperation();
	Operation->SetProgressCallback(ProgressCallback);
	Operation->SetMaterialData(MaterialData);
	Operation->AddMesh(InputMesh);

	// update reduction selection
	SetActiveSettingsIndex(GetActiveSettingsIndex(), false);

	// execute
	OperationResult = Operation->Execute(OutputMesh, GetRemeshingSettings());
}

bool UInstaLODRemeshTool::IsMeshOperationSuccessful() const
{
	return Operation != nullptr && OperationResult.Success;
}

void UInstaLODRemeshTool::DeallocMeshOperation()
{
	check(Operation);
	GetInstaLODInterface()->GetInstaLOD()->DeallocRemeshingOperation(Operation);
	Operation = nullptr;
}

InstaLOD::IInstaLODMaterial* UInstaLODRemeshTool::GetBakeMaterial()
{
	if (!IsMeshOperationSuccessful())
		return nullptr;

	return OperationResult.BakeMaterial;
}

FText UInstaLODRemeshTool::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "RemeshToolFriendlyName", "RE");
}

FText UInstaLODRemeshTool::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "RemeshToolComboBoxItemName", "Remesh");
}

FText UInstaLODRemeshTool::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "RemeshToolOperationInformation", "The Remesh operation reconstructs the mesh from scratch. In the process, it reduces the topological complexity of the mesh while preserving the visual outcome.\n\nUV coordinates are automatically created and the original mesh appearance is transferred onto the reconstructed mesh using InstaLOD's texture baker.");
}

int32 UInstaLODRemeshTool::GetOrderId() const
{
	return 2;
}

void UInstaLODRemeshTool::ResetSettings()
{
	RemeshMode = EInstaLODRemeshMode::InstaLOD_Reconstruct;
	FuzzyFaceCountTarget = EInstaLODRemeshFaceCount::InstaLOD_Normal;
	MaximumTriangles = 4;
	ScreenSizeInPixels = 300;
	PixelMergeDistance = 2;
	bAutomaticTextureSize = false;
	
	RemeshResolution = EInstaLODRemeshResolution::InstaLOD_Normal;
	bIgnoreBackfaces = false;
	HardAngleThreshold = 70.0f;
	WeldingDistance = 0.0f;
	GutterSizeInPixels = 5;
	UnwrapStrategy = EInstaLODUnwrapStrategy::InstaLOD_Auto;
	UnwrapStretchImportance = EInstaLODImportance::InstaLOD_Normal;
	bPostProcessLayout = false;
	bShellStitching = true;
	bInsertNormalSplits = true;
	bLockBoundaries = false;
	bDeterministic = false;
	SetActiveSettingsIndex(0, false);

	// Reset Parent which ultimately ends in a SaveConfig() call to reset everything
	Super::ResetSettings();
}

InstaLOD::RemeshingSettings UInstaLODRemeshTool::GetRemeshingSettings()
{
	InstaLOD::RemeshingSettings Settings;

	Settings.SurfaceMode = (InstaLOD::RemeshSurfaceMode::Type)RemeshMode;

	if (bUseFaceCountTarget)
	{
		Settings.FaceCountTarget = (InstaLOD::RemeshFaceCountTarget::Type)FuzzyFaceCountTarget;
	}
	else if (bUseMaximumTriangles)
	{
		Settings.MaximumTriangles = MaximumTriangles;
	}
	else if (bUseScreenSizeInPixels)
	{
		Settings.ScreenSizeInPixels = ScreenSizeInPixels;
		Settings.ScreenSizePixelMergeDistance = PixelMergeDistance;
		Settings.ScreenSizeInPixelsAutomaticTextureSize = bAutomaticTextureSize;
	}
	Settings.BakeEngine = InstaLOD::BakeEngine::CPU;

	// Surface Construction
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
	Settings.SurfaceConstructionIgnoreBackface = bIgnoreBackfaces;
	Settings.HardAngleThreshold = HardAngleThreshold;
	Settings.WeldDistance = WeldingDistance;
	Settings.GutterSizeInPixels = GutterSizeInPixels;
	Settings.UnwrapStrategy = (InstaLOD::UnwrapStrategy::Type)UnwrapStrategy;
	Settings.StretchImportance = (InstaLOD::MeshFeatureImportance::Type)UnwrapStretchImportance;
	Settings.SurfaceModeOptimizeLockBoundaries = bLockBoundaries;
	
	Settings.PostProcessLayout = bPostProcessLayout;
	Settings.ShellStitching = bShellStitching;
	Settings.InsertNormalSplits = bInsertNormalSplits;
	Settings.AlphaMaskThreshold = AlphaMaskThreshold;
	Settings.Deterministic = bDeterministic;
	Settings.BakeOutput = GetBakeOutputSettings();

	return Settings;
}

bool UInstaLODRemeshTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!UInstaLODBaseTool::IsValidJSONObject(JsonObject, "Remesh"))
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

	if (SettingsObject->HasField("SurfaceMode"))
	{
		const FString SurfaceModeValue = SettingsObject->GetStringField("SurfaceMode");

		if (SurfaceModeValue.Equals("Reconstruct", ESearchCase::IgnoreCase))
		{
			RemeshMode = EInstaLODRemeshMode::InstaLOD_Reconstruct;
		}
		else if (SurfaceModeValue.Equals("Optimize", ESearchCase::IgnoreCase))
		{
			RemeshMode = EInstaLODRemeshMode::InstaLOD_Optimize;
		}
		else if (SurfaceModeValue.Equals("ConvexHull", ESearchCase::IgnoreCase))
		{
			RemeshMode = EInstaLODRemeshMode::InstaLOD_ConvexHull;
		}
		else if (SurfaceModeValue.Equals("UV", ESearchCase::IgnoreCase))
		{
			RemeshMode = EInstaLODRemeshMode::InstaLOD_UV;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *SurfaceModeValue, TEXT("SurfaceMode"));
		}
	}
	if (SettingsObject->HasField("MaximumTriangles"))
	{
		MaximumTriangles = SettingsObject->GetIntegerField("MaximumTriangles");
	}
	if (SettingsObject->HasField("ScreenSizeInPixels"))
	{
		ScreenSizeInPixels = SettingsObject->GetIntegerField("ScreenSizeInPixels");
	}
	if (SettingsObject->HasField("ScreenSizePixelMergeDistance"))
	{
		PixelMergeDistance = SettingsObject->GetIntegerField("ScreenSizePixelMergeDistance");
	}
	if (SettingsObject->HasField("GutterSizeInPixels"))
	{
		GutterSizeInPixels = SettingsObject->GetIntegerField("GutterSizeInPixels");
	}
	if (SettingsObject->HasField("FuzzyFaceCountTarget"))
	{
		const FString FuzzyFaceCountTargetValue = SettingsObject->GetStringField("FuzzyFaceCountTarget");
		
		if (FuzzyFaceCountTargetValue.Equals("Lowest", ESearchCase::IgnoreCase))
		{
			FuzzyFaceCountTarget = EInstaLODRemeshFaceCount::InstaLOD_Lowest;
		}
		else if (FuzzyFaceCountTargetValue.Equals("Low", ESearchCase::IgnoreCase))
		{
			FuzzyFaceCountTarget = EInstaLODRemeshFaceCount::InstaLOD_Low;
		}
		else if (FuzzyFaceCountTargetValue.Equals("Normal", ESearchCase::IgnoreCase))
		{
			FuzzyFaceCountTarget = EInstaLODRemeshFaceCount::InstaLOD_Normal;
		}
		else if (FuzzyFaceCountTargetValue.Equals("High", ESearchCase::IgnoreCase))
		{
			FuzzyFaceCountTarget = EInstaLODRemeshFaceCount::InstaLOD_High;
		}
		else if (FuzzyFaceCountTargetValue.Equals("Highest", ESearchCase::IgnoreCase))
		{
			FuzzyFaceCountTarget = EInstaLODRemeshFaceCount::InstaLOD_Highest;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *FuzzyFaceCountTargetValue, TEXT("FuzzyFaceCountTarget"));
		}
	}
	if (SettingsObject->HasField("IgnoreBackface"))
	{
		bIgnoreBackfaces = SettingsObject->GetBoolField("IgnoreBackface");
	}
	if (SettingsObject->HasField("StretchImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("StretchImportance"); 
		UnwrapStretchImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("UnwrapStrategy"))
	{
		const FString UnwrapStrategyValue = SettingsObject->GetStringField("UnwrapStrategy");
		UnwrapStrategy = UInstaLODBaseTool::GetUnwrapStrategyValueForString(UnwrapStrategyValue);
	}
	if (SettingsObject->HasField("AlphaMaskThreshold"))
	{
		AlphaMaskThreshold = SettingsObject->GetNumberField("AlphaMaskThreshold");
	}
	if (SettingsObject->HasField("ShellStitching"))
	{
		bShellStitching = SettingsObject->GetBoolField("ShellStitching");
	}
	if (SettingsObject->HasField("InsertNormalSplits"))
	{
		bInsertNormalSplits = SettingsObject->GetBoolField("InsertNormalSplits");
	}
	if (SettingsObject->HasField("PostProcessLayout"))
	{
		bPostProcessLayout = SettingsObject->GetBoolField("PostProcessLayout");
	}
	if (SettingsObject->HasField("SurfaceModeOptimizeLockBoundaries"))
	{
		bLockBoundaries = SettingsObject->GetBoolField("SurfaceModeOptimizeLockBoundaries");
	}
	if (SettingsObject->HasField("Deterministic"))
	{
		bDeterministic = SettingsObject->GetBoolField("Deterministic");
	}

	UInstaLODBakeBaseTool::ReadSettingsFromJSONObject(SettingsObject);

	return true;
}
#undef LOCTEXT_NAMESPACE
