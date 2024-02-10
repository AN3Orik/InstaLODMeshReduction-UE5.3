/**
 * InstaLODLoad.Build.cs (InstaLOD)
 *
 * Copyright 2023 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODLoad.Build.cs
 * @copyright 2023 InstaLOD GmbH. All rights reserved.
 * @section License
 */

using UnrealBuildTool;
using System;
using System.Reflection;
using System.Globalization;

public class InstaLODLoad : ModuleRules
{
	public InstaLODLoad(ReadOnlyTargetRules Target)
		: base(Target)
	{

		PrivatePCHHeaderFile = "Private/InstaLODLoadPCH.h";

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"UnrealEd",
			"CoreUObject",
			"Engine"
		});
	}
}
