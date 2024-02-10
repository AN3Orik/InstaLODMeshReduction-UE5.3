/**
 * InstaLODUVTool.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUVTool.cpp 
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODUVTool.h"
#include "InstaLODUIPCH.h"

#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"

#include "Utilities/InstaLODUtilities.h"
#include "Slate/InstaLODWindow.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

UInstaLODUVTool::UInstaLODUVTool() : Super(),
Operation(nullptr),
OperationResult()
{
}

InstaLOD::UnwrapSettings UInstaLODUVTool::GetUnwrapSettings()
{
	InstaLOD::UnwrapSettings settings;

	switch (UnwrapStrategy)
	{
	case EInstaLODUnwrapStrategy::InstaLOD_Auto:
		settings.UnwrapStrategy = InstaLOD::UnwrapStrategy::Type::Auto;
		break;
	case EInstaLODUnwrapStrategy::InstaLOD_Organic:
		settings.UnwrapStrategy = InstaLOD::UnwrapStrategy::Type::Organic;
		break;
	case EInstaLODUnwrapStrategy::InstaLOD_HardSurfaceAngle:
		settings.UnwrapStrategy = InstaLOD::UnwrapStrategy::Type::HardSurfaceAngle;
		break;
	case EInstaLODUnwrapStrategy::InstaLOD_HardSurfaceAxial:
		settings.UnwrapStrategy = InstaLOD::UnwrapStrategy::Type::HardSurfaceAxial;
		break;
	};

	switch (StretchImportance)
	{
	case EInstaLODImportance::InstaLOD_OFF:
		settings.StretchImportance = InstaLOD::MeshFeatureImportance::Off;
		break;
	case EInstaLODImportance::InstaLOD_Lowest:
		settings.StretchImportance = InstaLOD::MeshFeatureImportance::Lowest;
		break;
	case EInstaLODImportance::InstaLOD_Low:
		settings.StretchImportance = InstaLOD::MeshFeatureImportance::Low;
		break;
	case EInstaLODImportance::InstaLOD_Normal:
		settings.StretchImportance = InstaLOD::MeshFeatureImportance::Normal;
		break;
	case EInstaLODImportance::InstaLOD_High:
		settings.StretchImportance = InstaLOD::MeshFeatureImportance::High;
		break;
	case EInstaLODImportance::InstaLOD_Highest:
		settings.StretchImportance = InstaLOD::MeshFeatureImportance::Highest;
		break;
	};

	settings.GutterSizeInPixels = GutterSizeInPixels;
	settings.TextureHeight = TextureSizeHeight;
	settings.TextureWidth = TextureSizeWidth;
	settings.TexCoordIndexOutput = TexCoordIndexOutput;
	settings.UVScale = UVScale;
	settings.InsertNormalSplits = bInsertNormalSplits;
	settings.ShellStitching = bShellStitching;
	settings.Deterministic = bDeterministic;

	return settings;
}

void UInstaLODUVTool::OnMeshOperationExecute(bool bIsAsynchronous)
{
	// --------------------------
	// can be run on child thread
	// --------------------------
	check(Operation == nullptr);

	static FScopedSlowTask* TaskProgress = nullptr;
	TaskProgress = SlowTaskProgress;
	InstaLOD::pfnUnwrapProgressCallback ProgressCallback = [](class InstaLOD::IUnwrapOperation *, const InstaLOD::IInstaLODMesh*, InstaLOD::IInstaLODMesh* outputMesh, const float ProgressInPercent)
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
	Operation = GetInstaLODInterface()->GetInstaLOD()->AllocUnwrapOperation();
	Operation->SetProgressCallback(ProgressCallback);
	
	// NOTE: we clamp the texcoordindexoutput to the max channel
	if (TexCoordIndexOutput > 0)
	{
		for (int32 TexCoordChannelIndex=0; TexCoordChannelIndex<=TexCoordIndexOutput; TexCoordChannelIndex++)
		{
			uint64 CurrentTextureCoordinatesCount = 0;
			InputMesh->GetWedgeTexCoords(TexCoordChannelIndex, &CurrentTextureCoordinatesCount);
			
			// found the maximum channel, now clamping to highest valid channel
			if (CurrentTextureCoordinatesCount != 0)
				continue;

			TexCoordIndexOutput = --TexCoordChannelIndex < 0 ? 0 : TexCoordChannelIndex;
			break;
		}
	}

	// execute
	OperationResult = Operation->Execute(InputMesh, OutputMesh, GetUnwrapSettings());
}

void UInstaLODUVTool::ResetSettings()
{
	UnwrapStrategy = EInstaLODUnwrapStrategy::InstaLOD_Auto;
	StretchImportance = EInstaLODImportance::InstaLOD_Normal;
	UVScale = 1.0f;
	TexCoordIndexOutput = 0;
	TextureSizeHeight = 1024;
	TextureSizeWidth = 1024;
	GutterSizeInPixels = 5;
	bShellStitching = true;
	bInsertNormalSplits = true;
}

bool UInstaLODUVTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!UInstaLODBaseTool::IsValidJSONObject(JsonObject, "UVUnwrap"))
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

	if (SettingsObject->HasField("GutterSizeInPixels"))
	{
		GutterSizeInPixels = SettingsObject->GetIntegerField("GutterSizeInPixels");
	}
	if (SettingsObject->HasField("InsertNormalSplits"))
	{
		bInsertNormalSplits = SettingsObject->GetBoolField("InsertNormalSplits");
	}
	if (SettingsObject->HasField("ShellStitching"))
	{
		bShellStitching = SettingsObject->GetBoolField("ShellStitching");
	}
	if (SettingsObject->HasField("UVScale"))
	{
		UVScale = (float)SettingsObject->GetNumberField("UVScale");
	}
	if (SettingsObject->HasField("TextureHeight"))
	{
		TextureSizeHeight = SettingsObject->GetIntegerField("TextureHeight");
	}
	if (SettingsObject->HasField("TextureWidth"))
	{
		TextureSizeWidth = SettingsObject->GetIntegerField("TextureWidth");
	}
	if (SettingsObject->HasField("UnwrapStrategy"))
	{
		const FString UnwrapStrategyValue = SettingsObject->GetStringField("UnwrapStrategy");
		UnwrapStrategy = UInstaLODBaseTool::GetUnwrapStrategyValueForString(UnwrapStrategyValue);
	}
	if (SettingsObject->HasField("StretchImportance"))
	{
		const FString Importance = SettingsObject->GetStringField("StretchImportance");
		StretchImportance = UInstaLODBaseTool::GetImportanceValueForString(Importance);
	}
	if (SettingsObject->HasField("Deterministic"))
	{
		bDeterministic = SettingsObject->GetBoolField("Deterministic");
	}

	return true;
}

void UInstaLODUVTool::DeallocMeshOperation() 
{
	if (Operation != nullptr)
	{
		GetInstaLODInterface()->GetInstaLOD()->DeallocUnwrapOperation(Operation);
		Operation = nullptr;
	}
}

bool UInstaLODUVTool::IsMeshOperationSuccessful() const
{
	return Operation != nullptr && OperationResult.Success;
};

FText UInstaLODUVTool::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "UnwrapToolFriendlyName", "UV");
}

FText UInstaLODUVTool::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "UnwrapToolComboBoxItemName", "UV Unwrap");
}

FText UInstaLODUVTool::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "UnwrapToolOperationInformation", "The UV Unwrapping operation automatically generates a UV parameterization and layout for the input mesh. The resulting UV layout is guaranteed to be free of overlaps. Multiple modes that provide the targeted results depending on the input mesh are available.");
}

int32 UInstaLODUVTool::GetOrderId() const
{
	return 6;
}
