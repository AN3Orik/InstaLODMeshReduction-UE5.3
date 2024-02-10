/**
 * InstaLODPluginCommands.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODPluginCommands.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODPluginCommands_h
#define InstaLOD_InstaLODPluginCommands_h

#include "Slate/InstaLODPluginStyle.h"

#include "Framework/Commands/Commands.h"

class FLODPluginCommands : public TCommands<FLODPluginCommands>
{
public:
	
	FLODPluginCommands()
	: TCommands<FLODPluginCommands>(TEXT("InstaLODMeshReduction"), NSLOCTEXT("Contexts", "InstaLODMeshReduction", "InstaLOD Plugin"), NAME_None, FInstaLODPluginStyle::GetStyleSetName())
	{
	}
	
	// TCommands<> interface
	virtual void RegisterCommands() override;
	
public:
	TSharedPtr< FUICommandInfo > PluginAction;
};

#endif
