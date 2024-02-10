/**
 * InstaLODMeshReductionPCH.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMeshReductionPCH.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODMeshReductionPCH_h
#define InstaLOD_InstaLODMeshReductionPCH_h

#include "UnrealEd.h"
#include "CoreTypes.h"

class InstaLODShared
{
public:
	static FString Version;
	static FString LicenseInformation;
	static const FString TargetedUnrealEngineVersion;
	
	
	static void OpenAuthorizationWindowModal();
	static void OpenDeauthorizationWindowModal();
};

DEFINE_LOG_CATEGORY_STATIC(LogInstaLOD, Log, All);

#endif
