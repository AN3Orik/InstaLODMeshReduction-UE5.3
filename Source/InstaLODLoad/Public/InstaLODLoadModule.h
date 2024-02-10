/**
 * InstaLODLoadModule.h (InstaLOD)
 *
 * Copyright 2023 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODLoadModule.h
 * @copyright 2023 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "IDetailCustomization.h"

class FInstaLODLoadModule : public IModuleInterface
{
	/// VARIABLES ///

public:
	/** Start - IModuleInterface Interface */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/** End - IModuleInterface Interface */
};
