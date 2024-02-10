/**
 * InstaLODLoadModule.cpp (InstaLOD)
 *
 * Copyright 2023 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODLoadModule.cpp
 * @copyright 2023 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODLoadModule.h"
#include "InstaLODLoadPCH.h"
#include "Misc/ConfigUtilities.h"

DEFINE_LOG_CATEGORY(LogInstaLOD);

#define LOCTEXT_NAMESPACE "InstaLODLoad"

/**< CVar to enable auto loading behavior enforcement. */
static TAutoConsoleVariable<int32> CVarEnableModuleLoad(TEXT("InstaLOD.EnableModuleLoad"), 1, TEXT("Due to an issue in the Unreal Engine settings, Project settings for Meshreduction are not loaded correctly upon startup. The current fix is to enforce the settings. This behavior can be disabled.\n"));

void FInstaLODLoadModule::StartupModule()
{
	// Load CVar setting from configuration file
	const FString BaseInstaLODMeshReductionIni = FConfigCacheIni::NormalizeConfigIniPath(FPaths::ProjectPluginsDir() + "InstaLODMeshReduction/Config/BaseInstaLODMeshReduction.ini");
	UE::ConfigUtilities::ApplyCVarSettingsFromIni(TEXT("Startup"), *BaseInstaLODMeshReductionIni, ECVF_SetByConsoleVariablesIni);

	if (CVarEnableModuleLoad.GetValueOnAnyThread() == 0)
		return;

	const FString InstaLODPluginName = TEXT("InstaLODMeshReduction");
	UE_LOG(LogInstaLOD, Log, TEXT("InstaLOD enforce %s plugin."), *InstaLODPluginName);

	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// NOTE: Due to a bug in UE 5.2, where the settings are loaded from config, but are overwritten by ConsoleVariable reinitialization, the settings are never correctly applied.
	// In order to still be able to load InstaLOD we enforce the values before the MeshReductionManagerModule is configured.
	IConsoleVariable* const MeshReductionCVar = ConsoleManager.FindConsoleVariable(TEXT("r.MeshReductionModule"));
	MeshReductionCVar->Set(*InstaLODPluginName, EConsoleVariableFlags::ECVF_SetByCode);

	IConsoleVariable* const SkeletalMeshReductionCVar = ConsoleManager.FindConsoleVariable(TEXT("r.SkeletalMeshReductionModule"));
	SkeletalMeshReductionCVar->Set(*InstaLODPluginName, EConsoleVariableFlags::ECVF_SetByCode);

	IConsoleVariable* const ProxyMeshReductionCVar = ConsoleManager.FindConsoleVariable(TEXT("r.ProxyLODMeshReductionModule"));
	ProxyMeshReductionCVar->Set(*InstaLODPluginName, EConsoleVariableFlags::ECVF_SetByCode);
}

void FInstaLODLoadModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FInstaLODLoadModule, InstaLODLoad);

#undef LOCTEXT_NAMESPACE

