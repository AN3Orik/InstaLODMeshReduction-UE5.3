/**
 * InstaLODMeshReductionPCH.h (InstaLOD)
 *
 * Copyright 2016-2023 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMeshReductionPCH.h
 * @copyright 2016-2023 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODMeshReductionPCH_h
#define InstaLOD_InstaLODMeshReductionPCH_h

#include "UnrealEd.h"
#include "CoreTypes.h"

class InstaLODShared
{
public:

	/**< Called on authorization. */
	static void OpenAuthorizationWindowModal();

	/**< Called on deauthorization. */
	static void OpenDeauthorizationWindowModal();

	static FString Version;								/**< InstaLOD plugin version. */
	static FString LicenseInformation;					/**< The license information. */
	static const FString TargetedUnrealEngineVersion;	/**< The targted unreal engine version. */
};

DEFINE_LOG_CATEGORY_STATIC(LogInstaLOD, Log, All);

#endif
