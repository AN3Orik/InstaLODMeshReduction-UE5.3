/**
 * InstaLODModule.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODModule.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */
#ifndef InstaLOD_InstaLODModule_h
#define InstaLOD_InstaLODModule_h

#include "MeshUtilities.h"
#include "InstaLOD/InstaLODAPI.h"
#include "InstaLOD/InstaLOD.h"


class FInstaLODModule : public IMeshReductionModule
{
public:
	// IModuleInterface interface.
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual FString GetName() override
	{
		return TEXT("InstaLODMeshReduction");
	}
	virtual class IMeshMerging* GetDistributedMeshMergingInterface() override { return nullptr; }

	virtual IInstaLOD* GetInstaLODInterface();

	virtual class IMeshReduction* GetSkeletalMeshReductionInterface() override;
	virtual class IMeshReduction* GetStaticMeshReductionInterface() override;
	virtual class IMeshMerging* GetMeshMergingInterface() override;

	/** Returns the InstaLODAPI. */
	InstaLOD::IInstaLOD* GetInstaLODAPI() const { return InstaLODAPI; }

private:

	void InstallHooks();

	void InstallHooksLate();

	TArray< TSharedPtr< FString > > LODTypes;
	FDelegateHandle LateHooksDelegateHandle;
	InstaLOD::IInstaLOD* InstaLODAPI;
};

#endif
