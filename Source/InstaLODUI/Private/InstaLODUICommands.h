/**
 * InstaLODUICommands.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUICommands.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "Framework/Commands/Commands.h"

class FInstaLODUICommands : public TCommands<FInstaLODUICommands>
{
public:
	FInstaLODUICommands();
	
	TSharedPtr<FUICommandInfo> OpenInstaLODWindow; /**< command used to open InstaLOD Window. */
	TSharedPtr<FUICommandInfo> OpenInstaLODWindowFromStaticMeshEditor; /**< command use to open the window from StaticMeshEditor. */
	TSharedPtr<FUICommandInfo> OpenInstaLODWindowFromSkeletalMeshEditor; /**< command use to open the window from SkeletalMeshEditor. */

	/** Start - TCommands<> Interface */
	virtual void RegisterCommands() override;
	/** End - TCommands<> Interface */
};
