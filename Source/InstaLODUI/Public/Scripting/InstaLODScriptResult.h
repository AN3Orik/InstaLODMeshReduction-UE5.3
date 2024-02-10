/**
 * InstaLODScriptResult.h (InstaLOD)
 *
 * Copyright 2016-2022 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODScriptResult.h
 * @copyright 2016-2022 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "InstaLODScriptResult.generated.h"

UCLASS(BluePrintable)
class UInstaLODScriptResult : public UObject
{
	GENERATED_BODY()

public:
	UInstaLODScriptResult() : UObject(),
		bSuccess(false),
		OutResults()
	{
	}
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Success"), Category = "Settings")
		bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "OutResults"), Category = "Settings")
		TArray<UObject*> OutResults = {};
};