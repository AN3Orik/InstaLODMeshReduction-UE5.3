/**
 * InstaLODOcclusionCullTool.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODOcclusionCullTool.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODOcclusionCullTool.h"
#include "InstaLODUIPCH.h"
#include "Slate/InstaLODWindow.h"


#define LOCTEXT_NAMESPACE "InstaLODUI"


static inline InstaLOD::InstaVec3F FVectorToInstaVec(const FVector& Vector)
{
	InstaLOD::InstaVec3F OutVector;
	OutVector.X = Vector.X;
	OutVector.Y = Vector.Y;
	OutVector.Z = Vector.Z;
	return OutVector;
}

UInstaLODOcclusionCullTool::UInstaLODOcclusionCullTool() :
Operation(nullptr),
OperationResult()
{
}

FText UInstaLODOcclusionCullTool::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "OcclusionCullToolFriendlyName", "OC");
}

FText UInstaLODOcclusionCullTool::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "OcclusionCullToolComboBoxItemName", "Occlusion Cull");
}

FText UInstaLODOcclusionCullTool::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "OcclusionCullToolOperationInformation", "The Occlusion Culling operation is capable of removing  polygons that are not visible to the observer. Occlusion Culling is an excellent tool to reduce polygon count of complex assemblies without impacting the quality of visible polygons. InstaLOD supports automatic detection and culling of interior polygons, alternatively, manually placed cameras can be used.");
}

int32 UInstaLODOcclusionCullTool::GetOrderId() const
{
	return 5;
}

void UInstaLODOcclusionCullTool::ResetSettings()
{
	OcclusionCullMode = EInstaLODOcclusionCullMode::InstaLOD_AutomaticInterior;
	CullingStrategy = EInstaLODOcclusionCullingStrategy::InstaLOD_Face;
	DataUsage = EInstaLODOcclusionCullDataUsage::InstaLOD_RemoveGeometry;
	Resolution = 1024;
	Precision = EInstaLODImportance::InstaLOD_Normal;
	AdjacencyDepth = 5;
#if defined(INSTALOD_OCCLUSIONCULL_ALPHAMASK_THRESHOLD)
	AlphaMaskThreshold = 0.5f;
#endif

	bDeterministic = false;

	// Reset Parent which ultimately ends in a SaveConfig() call to reset everything
	Super::ResetSettings();
}

bool UInstaLODOcclusionCullTool::IsMeshOperationExecutable(FText* OutErrorText) const
{
	if (GetInstaLODWindow() == nullptr)
		return false;
	
	if (!Super::IsMeshOperationExecutable(OutErrorText))
		return false;
	
	if (OcclusionCullMode == EInstaLODOcclusionCullMode::InstaLOD_CameraBased &&
		GetInstaLODWindow()->GetNumCameraComponents() == 0)
	{
		if (OutErrorText != nullptr)
		{
			*OutErrorText = NSLOCTEXT("InstaLODUI", "NoCamerasSelected", "Running a camera based occlusion cull operation requires at least one camera in the actor selection.");
		}
		return false;
	}
	
	return true;
}

void UInstaLODOcclusionCullTool::OnMeshOperationExecute(bool bIsAsynchronous)
{
	// --------------------------
	// can be run on child thread
	// --------------------------
	check(Operation == nullptr);

	static FScopedSlowTask* TaskProgress = nullptr;
	TaskProgress = SlowTaskProgress;
	InstaLOD::pfnOcclusionCullProgressCallback ProgressCallback = [](InstaLOD::IOcclusionCullOperation *, const InstaLOD::IInstaLODMesh* , InstaLOD::IInstaLODMesh* ,
																	const float ProgressInPercent)
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
	Operation = GetInstaLODInterface()->GetInstaLOD()->AllocOcclusionCullOperation();
	Operation->SetProgressCallback(ProgressCallback);
	
	if (OcclusionCullMode == EInstaLODOcclusionCullMode::InstaLOD_CameraBased)
	{
		TArray<TSharedPtr<FInstaLODMeshComponent>> InstaLODMeshComponents = GetInstaLODWindow()->GetEnabledSelectedCameraComponents();
		
		for(const TSharedPtr<FInstaLODMeshComponent>& Component : InstaLODMeshComponents)
		{
			check(Component->CameraComponent.IsValid());
			
			UCameraComponent *const CameraComponent = Component->CameraComponent.Get();
			
			const FTransform ComponentToWorldTransform = CameraComponent->GetComponentTransform();
		
			InstaLOD::OcclusionCullCamera OcclusionCullCamera;
			
			OcclusionCullCamera.Forward = FVectorToInstaVec(CameraComponent->GetForwardVector());
			OcclusionCullCamera.Right = FVectorToInstaVec(CameraComponent->GetRightVector());
			OcclusionCullCamera.Up = FVectorToInstaVec(CameraComponent->GetUpVector());
			OcclusionCullCamera.Position = FVectorToInstaVec(CameraComponent->GetComponentLocation());
			
			OcclusionCullCamera.FieldOfViewInDegrees = CameraComponent->FieldOfView;
			OcclusionCullCamera.IsOrthogonal = CameraComponent->ProjectionMode == ECameraProjectionMode::Orthographic;
			OcclusionCullCamera.OrthogonalScale = CameraComponent->OrthoWidth;
			OcclusionCullCamera.NearPlane = OcclusionCullCamera.IsOrthogonal ? CameraComponent->OrthoNearClipPlane : 0.0f;
			OcclusionCullCamera.FarPlane = OcclusionCullCamera.IsOrthogonal ? CameraComponent->OrthoFarClipPlane : 100000.0f;
			
			OcclusionCullCamera.ResolutionX = GetOcclusionCullSettings().Resolution;
			OcclusionCullCamera.ResolutionY = FMath::RoundHalfToEven(OcclusionCullCamera.ResolutionX / CameraComponent->AspectRatio);
			
			Operation->AddCamera(OcclusionCullCamera, TCHAR_TO_UTF8(*CameraComponent->GetOwner()->GetName()));
		}
	}
	
	// execute
	OperationResult = Operation->Execute(InputMesh, OutputMesh, GetOcclusionCullSettings());
}

bool UInstaLODOcclusionCullTool::IsMeshOperationSuccessful() const
{
	return Operation != nullptr && OperationResult.Success;
}

void UInstaLODOcclusionCullTool::DeallocMeshOperation()
{
	check(Operation);
	GetInstaLODInterface()->GetInstaLOD()->DeallocOcclusionCullOperation(Operation);
	Operation = nullptr;
}

InstaLOD::OcclusionCullSettings UInstaLODOcclusionCullTool::GetOcclusionCullSettings()
{
	InstaLOD::OcclusionCullSettings Settings;
	
	Settings.Mode = (InstaLOD::OcclusionCullMode::Type)OcclusionCullMode;
	Settings.CullingStrategy = (InstaLOD::OcclusionCullingStrategy::Type)CullingStrategy;
	Settings.DataUsage = (InstaLOD::OcclusionCullDataUsage::Type)DataUsage;
	Settings.AutomaticPrecision = (InstaLOD::MeshFeatureImportance::Type)Precision;
	Settings.Resolution = Resolution;
	Settings.AdjacencyDepth = AdjacencyDepth;

	Settings.SubMeshVisbilityFaceThreshold = 1; // FIXME: add UI
	Settings.IgnoreBackface = true; // FIXME: add UI
	
#if defined(INSTALOD_OCCLUSIONCULL_ALPHAMASK_THRESHOLD)
	Settings.AlphaMaskThreshold = AlphaMaskThreshold;
#endif
	
	return Settings;
}

bool UInstaLODOcclusionCullTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!UInstaLODBaseTool::IsValidJSONObject(JsonObject, "OcclusionCull"))
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

	if (SettingsObject->HasField("AdjacencyDepth"))
	{
		AdjacencyDepth = SettingsObject->GetIntegerField("AdjacencyDepth");
	}
	if (SettingsObject->HasField("Resolution"))
	{
		Resolution = SettingsObject->GetIntegerField("Resolution"); 
	}
	if (SettingsObject->HasField("AutomaticPrecision"))
	{
		const FString Importance = SettingsObject->GetStringField("AutomaticPrecision");
		Precision = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("DataUsage"))
	{
		const FString DataUsageValue = SettingsObject->GetStringField("DataUsage");

		if (DataUsageValue.Equals("RemoveGeometry", ESearchCase::IgnoreCase))
		{
			DataUsage = EInstaLODOcclusionCullDataUsage::InstaLOD_RemoveGeometry;
		}
		else if (DataUsageValue.Equals("WriteWedgeColors", ESearchCase::IgnoreCase))
		{
			DataUsage = EInstaLODOcclusionCullDataUsage::InstaLOD_WriteWedgeColors;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *DataUsageValue, TEXT("DataUsage"));
		}
	}
	if (SettingsObject->HasField("CullingStrategy"))
	{
		const FString CullingStrategyValue = SettingsObject->GetStringField("CullingStrategy");

		if (CullingStrategyValue.Equals("SubMeshByFaceAdjacency", ESearchCase::IgnoreCase))
		{
			CullingStrategy = EInstaLODOcclusionCullingStrategy::InstaLOD_SubmeshByFaceAdjacency;
		}
		else if (CullingStrategyValue.Equals("SubMeshByFaceSubMeshIndices", ESearchCase::IgnoreCase))
		{
			CullingStrategy = EInstaLODOcclusionCullingStrategy::InstaLOD_SubmeshByFaceSubmeshIndicies;
		}
		else if (CullingStrategyValue.Equals("Face", ESearchCase::IgnoreCase))
		{
			CullingStrategy = EInstaLODOcclusionCullingStrategy::InstaLOD_Face;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *CullingStrategyValue, TEXT("CullingStrategy"));
		}
	}
	if (SettingsObject->HasField("Mode"))
	{
		const FString CullingModeValue = SettingsObject->GetStringField("Mode");

		if (CullingModeValue.Equals("AutomaticInterior", ESearchCase::IgnoreCase))
		{
			OcclusionCullMode = EInstaLODOcclusionCullMode::InstaLOD_AutomaticInterior;
		}
		else if (CullingModeValue.Equals("CameraBased", ESearchCase::IgnoreCase))
		{
			OcclusionCullMode = EInstaLODOcclusionCullMode::InstaLOD_CameraBased;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *CullingModeValue, TEXT("Mode"));
		}
	}
	if (SettingsObject->HasField("Deterministic"))
	{
		bDeterministic = SettingsObject->GetBoolField("Deterministic");
	}

	return true;
}
#undef LOCTEXT_NAMESPACE
