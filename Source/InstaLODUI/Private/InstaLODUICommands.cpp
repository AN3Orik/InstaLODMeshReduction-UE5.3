/**
 * InstaLODUICommands.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUICommands.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODUICommands.h"
#include "InstaLODUIPCH.h"
#include "Slate/InstaLODPluginStyle.h"

#define LOCTEXT_NAMESPACE "FInstaLODUIModule"

FInstaLODUICommands::FInstaLODUICommands()
	: TCommands<FInstaLODUICommands>(TEXT("InstaLODUI")
	, NSLOCTEXT("Contexts", "InstaLODUI", "InstaLOD Plugin")
	, NAME_None
	, FInstaLODPluginStyle::GetStyleSetName())
{
}

void FInstaLODUICommands::RegisterCommands()
{
	UI_COMMAND(OpenInstaLODWindow, "InstaLOD", "Opens up a dockable window for the InstaLOD plugin.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenInstaLODWindowFromStaticMeshEditor, "InstaLOD", "Opens up a dockable window for the InstaLOD plugin.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenInstaLODWindowFromSkeletalMeshEditor, "InstaLOD", "Opens up a dockable window for the InstaLOD plugin.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
