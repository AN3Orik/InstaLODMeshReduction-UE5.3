/**
 * InstaLODUIModule.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUIModule.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "IDetailCustomization.h"

class FInstaLODUIModule : public IModuleInterface
{
	/// VARIABLES ///

private:

	/** UI Commands to bind to Slate Actions (like opening a Window on Button Press). */
	TSharedPtr<class FUICommandList> PluginCommands;

	/** These are all Tools that we could find. */
	TArray<class UInstaLODBaseTool*> Tools;

	/// FUNCTIONS ///

private:

	/** 
	*	Adds the Toolbar Button for opening the Window Tab.
	*
	*	@param		Builder			Helper class to add Toolbar extensions
	*/
	void AddToolbarExtension(FToolBarBuilder& Builder, TSharedPtr<FUICommandInfo> UICommand);

	/**
	*	Adds a Menu Button for opening the Winodw Tab.
	*
	*	@param		Builder			Helper class to add Menu extensions
	*/
	void AddMenuExtension(FMenuBuilder& Builder, TSharedPtr<FUICommandInfo> UICommand);

	/**
	*	Creates new DockTab and within it the Window Tab.
	*
	*	@param		SpawnTabArgs	Holds Information about Index and Parent/Owner Window
	*	@return						Reference to the Spawn SDockTab
	*/
	TSharedRef<class SDockTab> OnSpawnInstaLODTab(const class FSpawnTabArgs& SpawnTabArgs);

	/** Inits the Tool Classes.	*/
	void InitToolClasses();

	/** 
	*	Called when the current Module Changes. 
	*
	*	@param		Module			Module Name being loaded
	*	@param		Reason			Reason why the Module got loaded
	*/
	void OnModulesChanged(FName Module, EModuleChangeReason Reason);

	/** Handles the Extensions. */
	void InstallExtensions();
	void RemoveExtensions();

	/** Called when the Tab is closed. Lets us clean up everything. */
	void OnInstaLODTabClosed(TSharedRef<class SDockTab> ClosedTab);

	/** Called to create an Instance of a DetailsCustomization */
	TSharedRef<IDetailCustomization> CreateBaseToolCustomization();

	/** Called to create an Instance of a DetailsCustomization */
	TSharedRef<IDetailCustomization> CreateOptimizeToolCustomization();

	/** Called to create an Instance of a DetailsCustomization */
	TSharedRef<IDetailCustomization> CreateSettingsCustomization();

public:

	void OnAssetEditorRequestedOpen(UObject* OpenedAsset);

	TArray<class UInstaLODBaseTool*> GetAllTools() const { return Tools; }

	/** Start - IModuleInterface Interface */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/** End - IModuleInterface Interface */

	/** Function bound to the OpenInstaLODWindow Command. */
	void OpenInstaLODWindowClicked();
	void OpenInstaLODWindowClickedStaticMeshEditor();
	void OpenInstaLODWindowClickedSkeletalMeshEditor();
};
