/**
 * InstaLODResultSettings.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODResultSettings.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once
#include "CoreMinimal.h"
#include "InstaLODResultSettings.generated.h"

UENUM()
enum class EInstaLODSavingOption : uint8
{
	InsertAsLOD				UMETA(DisplayName = "Insert as LOD"),
	OnlyReturnInOutArray	UMETA(DisplayName = "Return in Out Array"),
};

UCLASS(Config = InstaLOD, BluePrintable)
class UInstaLODResultSettings : public UObject
{
	GENERATED_BODY()

public:

	/************************************************************************/
	/* Settings                                                             */
	/************************************************************************/

	/** How the resulting meshes should be handled. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (ExposeOnSpawn, DisplayName = "Saving Option"), Category = "Settings")
	EInstaLODSavingOption SavingOption = EInstaLODSavingOption::OnlyReturnInOutArray;

	/** The target LOD index. */
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (ExposeOnSpawn, EditCondition = "SavingOption == EInstaLODSavingOption::InsertAsLOD", EditConditionhides, DisplayName = "Target LOD Index"), Category = "Settings")
	int32 TargetLODIndex = 1;

	/** Customizing the pivot position is only accessible if OnlyReturnInOutArray is selected and number of static meshes is more than one when using remesh operation.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (ExposeOnSpawn, EditCondition = "SavingOption == EInstaLODSavingOption::OnlyReturnInOutArray", EditConditionhides, DisplayName = "Pivot Position"), Category = "Settings")
		EInstaLODPivotPosition PivotPosition = EInstaLODPivotPosition::InstaLOD_Default;

	/** Custom pivot position. This is for positioning the pivot inside the bounding box. The vector (0.5, 0.5, 0.5) places the pivot in the center.*/
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, meta = (ExposeOnSpawn, EditCondition = "(SavingOption == EInstaLODPivotPosition::OnlyReturnInOutArray && (PivotPosition == EInstaLODPivotPosition::InstaLOD_Custom || PivotPosition == EInstaLODPivotPosition::InstaLOD_CustomLimited))", EditConditionhides, DisplayName = "Position"), Category = "Settings")
		FVector Position = FVector::ZeroVector;
};