/**
 * InstaLODPluginCommands.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODPluginCommands.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODMeshReductionPCH.h"
#include "InstaLODPluginCommands.h"

#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "FLODPluginModule"

void FLODPluginCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "InstaLOD", "Displays help and instructions for the InstaLOD plugin", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
