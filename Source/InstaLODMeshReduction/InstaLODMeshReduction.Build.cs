/**
 * InstaLODMeshReduction.Build.cs (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMeshReduction.Build.cs
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

using UnrealBuildTool;
using System;
using System.Reflection;
using System.Globalization;

public class InstaLODMeshReduction : ModuleRules
{
    public static int EngineVersionMajor = 5;
    public static int EngineVersionMinor = 0;
    public static int EngineVersionPatch = 0;

    public static void LoadEngineDisplayVersion()
    {
        string[] VersionHeader = Utils.ReadAllText("../Source/Runtime/Launch/Resources/Version.h").Replace("\r\n", "\n").Replace("\t", " ").Split('\n');

        foreach (string Line in VersionHeader)
        {
            if (Line.StartsWith("#define ENGINE_MAJOR_VERSION "))
            {
                EngineVersionMajor = int.Parse(Line.Split(' ')[2]);
            }
            else if (Line.StartsWith("#define ENGINE_MINOR_VERSION "))
            {
                EngineVersionMinor = int.Parse(Line.Split(' ')[2]);
            }
            else if (Line.StartsWith("#define ENGINE_PATCH_VERSION "))
            {
                EngineVersionPatch = int.Parse(Line.Split(' ')[2]);
            }
        }
    }

    public InstaLODMeshReduction(ReadOnlyTargetRules Target)
        : base(Target)
    {
        Initialize();
    }

    protected void Initialize()
    {
        LoadEngineDisplayVersion();

		IWYUSupport = IWYUSupport.None;
		PrivatePCHHeaderFile = "InstaLODMeshReductionPCH.h";

        PublicDefinitions.Add("INSTALOD_LIB_DYNAMIC=1");

        PrivateIncludePaths.AddRange(new string[] {
            "InstaLODMeshReduction/Private"
        });

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });
		
        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "Slate",
            "SlateCore",
            "InputCore",
            "UnrealEd",
            "LevelEditor",
            "PropertyEditor",
            "StaticMeshEditor",
			"Persona",
            "EditorStyle",
            "StaticMeshDescription",
            "MeshDescription",
            "MeshDescriptionOperations",
            "MeshBoneReduction",
            "MeshPaint",
            "Projects",
			"AnimationBlueprintLibrary",
            "ClothingSystemRuntimeCommon",
            "SkeletalMeshUtilitiesCommon",
            "RawMesh",
            "MaterialUtilities"
        });

        PrivateIncludePathModuleNames.AddRange(new string[] {
            "Settings",
            "AssetTools",
            "MeshUtilities",
            "MaterialUtilities",
            "StaticMeshDescription",
            "MeshDescription",
            "MeshDescriptionOperations",
            "MeshBoneReduction",
            "ClothingSystemRuntimeCommon",
            "StaticMeshEditor",
			"AnimationBlueprintLibrary",
            "EditorStyle",
            "SkeletalMeshUtilitiesCommon",
            "Persona"
        });
    }
}
