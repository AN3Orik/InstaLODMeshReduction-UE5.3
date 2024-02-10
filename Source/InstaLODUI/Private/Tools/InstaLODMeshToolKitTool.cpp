/**
 * InstaLODUVTool.cpp (InstaLOD)
 *
 * Copyright 2016-2020 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUVTool.cpp 
 * @copyright 2016-2020 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODMeshToolKitTool.h"
#include "InstaLODUIPCH.h"

#include "Utilities/InstaLODUtilities.h"
#include "Slate/InstaLODWindow.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

UInstaLODMeshToolKitTool::UInstaLODMeshToolKitTool() : Super(),
Operation(nullptr),
OperationResult()
{
}

InstaLOD::MeshToolKitSettings UInstaLODMeshToolKitTool::GetMeshToolKitSettings()
{
	InstaLOD::MeshToolKitSettings settings;

	auto fnGetNormalHealingMode = [](const EInstaLODNormalHealing mode) -> InstaLOD::NormalHealingMode::Type
	{
		switch (mode)
		{
			case EInstaLODNormalHealing::InstaLOD_OFF :
				return InstaLOD::NormalHealingMode::Off;
			case EInstaLODNormalHealing::InstaLOD_Minimal:
				return InstaLOD::NormalHealingMode::Minimal;
			case EInstaLODNormalHealing::InstaLOD_Default:
				return InstaLOD::NormalHealingMode::Default;
		}
		return InstaLOD::NormalHealingMode::Off;
	};

	settings.WeldingThreshold = VertexWelding;
	settings.WeldingNormalAngleThreshold = VertexWeldingNormalAngle;
	settings.WeldingBoundaries = bWeldingBoundaries;
	settings.HealTJunctionThreshold = TJunctionHealing;
	settings.RemoveDegenerateFacesThreshold = RemoveDegenerateFaces;
	settings.FixNonManifold = bFixNonManifold;
	settings.ConformNormals = bConformNormals;
	settings.ConformWindingOrder = bConformWindingOrder;
	settings.FlipNormals = bFlipNormals;
	settings.FlipWindingOrder = bFlipWindingOrder;
	settings.FillHoles = bFillHoles;
	settings.NormalHealingMode = fnGetNormalHealingMode(NormalHealingMode);
	settings.RecalculateNormals = bRecalculateNormals;
	settings.HardAngleThreshold = HardangleThreshold;
	settings.WeightedNormals = bWeightedNormals;
	settings.MinimumSubMeshBoundingSphereRadius = MinimumRadius;
	settings.MinimumFaceArea = MinimumFaceArea;
	return settings;
}

void UInstaLODMeshToolKitTool::OnMeshOperationExecute(bool bIsAsynchronous)
{
	// --------------------------
	// can be run on child thread
	// --------------------------
	check(Operation == nullptr);

	static FScopedSlowTask* TaskProgress = nullptr;
	TaskProgress = SlowTaskProgress;
	InstaLOD::pfnMeshToolKitProgressCallback ProgressCallback = [](InstaLOD::IMeshToolKitOperation* MeshToolKitOperation, const InstaLOD::IInstaLODMesh* SourceMesh, InstaLOD::IInstaLODMesh* TargetMesh, const float ProgressInPercent)
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
	Operation = GetInstaLODInterface()->GetInstaLOD()->AllocMeshToolKitOperation();
	Operation->SetProgressCallback(ProgressCallback); 

	// execute
	OperationResult = Operation->Execute(InputMesh, OutputMesh, GetMeshToolKitSettings());
}

void UInstaLODMeshToolKitTool::ResetSettings()
{
	VertexWelding = 0.0f;
	VertexWeldingNormalAngle = 80.0f;
	TJunctionHealing = 0.0f;
	RemoveDegenerateFaces = 0.0f;
	bFixNonManifold = false;
	bConformNormals = false;
	bConformWindingOrder = false;
	bFlipNormals = false;
	bFlipWindingOrder = false;
	bFillHoles = false;
	NormalHealingMode = EInstaLODNormalHealing::InstaLOD_OFF;
	bRecalculateNormals = false;
	HardangleThreshold = 80.0f;
	bWeightedNormals = true;
	MinimumRadius = 0.0f;
	MinimumFaceArea = 0.0f;
}

bool UInstaLODMeshToolKitTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!UInstaLODBaseTool::IsValidJSONObject(JsonObject, "MeshToolKit"))
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

	if (SettingsObject->HasField("MinimumFaceArea"))
	{
		MinimumFaceArea = (float)SettingsObject->GetNumberField("MinimumFaceArea");
	}
	if (SettingsObject->HasField("MinimumSubMeshBoundingSphereRadius"))
	{
		MinimumRadius = (float)SettingsObject->GetNumberField("MinimumSubMeshBoundingSphereRadius");
	}
	if (SettingsObject->HasField("WeldingThreshold"))
	{
		VertexWelding = (float)SettingsObject->GetNumberField("WeldingThreshold");
	}
	if (SettingsObject->HasField("WeldingNormalAngleThreshold"))
	{
		VertexWeldingNormalAngle = (float)SettingsObject->GetNumberField("WeldingNormalAngleThreshold");
	}
	if (SettingsObject->HasField("WeldingBoundaries"))
	{
		bWeldingBoundaries = SettingsObject->GetBoolField("WeldingBoundaries");
	}
	if (SettingsObject->HasField("HealTJunctionThreshold"))
	{
		TJunctionHealing = (float)SettingsObject->GetNumberField("HealTJunctionThreshold");
	}
	if (SettingsObject->HasField("RemoveDegenerateFacesThreshold"))
	{
		RemoveDegenerateFaces = (float)SettingsObject->GetNumberField("RemoveDegenerateFacesThreshold");
	}
	if (SettingsObject->HasField("FixNonManifold"))
	{
		bFixNonManifold = SettingsObject->GetBoolField("FixNonManifold");
	}
	if (SettingsObject->HasField("FillHoles"))
	{
		bFillHoles = SettingsObject->GetBoolField("FillHoles");
	}
	if (SettingsObject->HasField("ConformNormals"))
	{
		bConformNormals = SettingsObject->GetBoolField("ConformNormals");
	}
	if (SettingsObject->HasField("FlipNormals"))
	{
		bFlipNormals = SettingsObject->GetBoolField("FlipNormals");
	}
	if (SettingsObject->HasField("FlipWindingOrder"))
	{
		bFlipWindingOrder = SettingsObject->GetBoolField("FlipWindingOrder");
	}
	if (SettingsObject->HasField("RecalculateNormals"))
	{
		bRecalculateNormals = SettingsObject->GetBoolField("RecalculateNormals");
	}
	if (SettingsObject->HasField("WeightedNormals"))
	{
		bWeightedNormals = SettingsObject->GetBoolField("WeightedNormals");
	}
	if (SettingsObject->HasField("HardAngleThreshold"))
	{
		HardangleThreshold = (float)SettingsObject->GetNumberField("HardAngleThreshold");
	}
	if (SettingsObject->HasField("NormalHealingMode"))
	{
		const FString NormalHealingModeValue = SettingsObject->GetStringField("NormalHealingMode");

		if (NormalHealingModeValue.Equals("Off", ESearchCase::IgnoreCase))
		{
			NormalHealingMode = EInstaLODNormalHealing::InstaLOD_OFF;
		}
		else if (NormalHealingModeValue.Equals("Minimal", ESearchCase::IgnoreCase))
		{
			NormalHealingMode = EInstaLODNormalHealing::InstaLOD_Minimal;
		}
		else if (NormalHealingModeValue.Equals("Default", ESearchCase::IgnoreCase))
		{
			NormalHealingMode = EInstaLODNormalHealing::InstaLOD_Default;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *NormalHealingModeValue, TEXT("NormalHealingMode"));
		}
	}

	return true;
}

void UInstaLODMeshToolKitTool::DeallocMeshOperation() 
{
	if (Operation != nullptr)
	{
		GetInstaLODInterface()->GetInstaLOD()->DeallocMeshToolKitOperation(Operation);
		Operation = nullptr;
	}
}

bool UInstaLODMeshToolKitTool::IsMeshOperationSuccessful() const
{
	return Operation != nullptr && OperationResult.Success;
};

FText UInstaLODMeshToolKitTool::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "MTKToolFriendlyName", "MTK");
}

FText UInstaLODMeshToolKitTool::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "MTKToolComboBoxItemName", "Mesh Toolkit");
}

FText UInstaLODMeshToolKitTool::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "MTKToolOperationInformation", "The Mesh Toolkit operation is a swiss-army knife for geometry processing. From welding vertices to conforming normals to point into the same direction.");
}

int32 UInstaLODMeshToolKitTool::GetOrderId() const
{
	return 7;
}
