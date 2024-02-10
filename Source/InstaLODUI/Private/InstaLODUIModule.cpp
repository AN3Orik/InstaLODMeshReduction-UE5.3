/**
 * InstaLODUIModule.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUIModule.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODUIModule.h"
#include "InstaLODUIPCH.h"

#include "InstaLODModule.h"
#include "InstaLODUICommands.h"
#include "Slate/InstaLODPluginStyle.h"

#include "Slate/InstaLODWindow.h"
#include "Tools/InstaLODBaseTool.h"
#include "Tools/InstaLODSettings.h"

#include "Customizations/InstaLODBaseToolCustomization.h"
#include "Customizations/InstaLODOptimizeToolCustomization.h"
#include "Customizations/InstaLODSettingsCustomization.h"
#include "Customizations/InstaLODMaterialSettingsCustomizations.h"


#include "Widgets/Docking/SDockTab.h"

#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ToolMenus.h"
#include "ToolMenuSection.h"
#include "Interfaces/IPluginManager.h"
#include "LevelEditor.h"
#include "PropertyEditorModule.h"
#include "ISkeletalMeshEditorModule.h"
#include "StaticMeshEditorModule.h"
#include "EditorSupportDelegates.h"
#include "Runtime/Launch/Resources/Version.h"
#include "IMeshReductionManagerModule.h"

static const FName InstaLODWindowTabName("InstaLODWindow");

DEFINE_LOG_CATEGORY(LogInstaLOD);

#define LOCTEXT_NAMESPACE "InstaLODUI"

void FInstaLODUIModule::StartupModule()
{
	// register the UI commands class and create a new UICommandList
	FInstaLODUICommands::Register();
	PluginCommands = MakeShareable(new FUICommandList);

	// bind the open window UI command to a function so we can utilize it
 	PluginCommands->MapAction(FInstaLODUICommands::Get().OpenInstaLODWindow,
 		FExecuteAction::CreateRaw(this, &FInstaLODUIModule::OpenInstaLODWindowClicked),
 		FCanExecuteAction());
	PluginCommands->MapAction(FInstaLODUICommands::Get().OpenInstaLODWindowFromStaticMeshEditor,
		FExecuteAction::CreateRaw(this, &FInstaLODUIModule::OpenInstaLODWindowClickedStaticMeshEditor),
		FCanExecuteAction());
	PluginCommands->MapAction(FInstaLODUICommands::Get().OpenInstaLODWindowFromSkeletalMeshEditor,
		FExecuteAction::CreateRaw(this, &FInstaLODUIModule::OpenInstaLODWindowClickedSkeletalMeshEditor),
		FCanExecuteAction());

	const FSlateIcon InstaLODIcon(FInstaLODPluginStyle::GetStyleSetName(), "InstaLODUI.TabIcon");
	// register the InstaMAT tab spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(InstaLODWindowTabName);
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(InstaLODWindowTabName, FOnSpawnTab::CreateRaw(this, &FInstaLODUIModule::OnSpawnInstaLODTab))
		.SetIcon(InstaLODIcon)
		.SetDisplayName(LOCTEXT("InstaLODWindowTabTitle", "InstaLOD"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// register menu toolbar
	UToolMenu* const ToolBar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.AssetsToolBar");

	if (ToolBar != nullptr)
	{
		FToolMenuSection& Content = ToolBar->FindOrAddSection("InstaLOD");

		/// The fnCreateMenu lambda generates the menu entries.
		const auto fnCreateMenu = [this](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection("InstaLOD", LOCTEXT("InstaLODToolbarMenu", "InstaLOD Menu"));

			MenuBuilder.AddMenuEntry(
				TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([]()
					{
						return LOCTEXT("InstaLOD", "InstaLOD");
					})),
				TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([]()
					{
						return LOCTEXT("InstaLODButtonTooltip", "Opens the InstaLOD window.");
					})),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateRaw(this, &FInstaLODUIModule::OpenInstaLODWindowClicked),
							FCanExecuteAction()
						),
						NAME_None,
						EUserInterfaceActionType::Button
						);

			MenuBuilder.EndSection();
		};

		FToolMenuEntry InstaLODButton = FToolMenuEntry::InitComboButton(
			"InstaLODMenu",
			FUIAction(),
			FNewToolMenuChoice(FNewMenuDelegate::CreateLambda(fnCreateMenu)),
			NSLOCTEXT("InstaLOD", "InstaLOD", "InstaLOD"),
			NSLOCTEXT("InstaLODToolTip", "InstaLODToolTip", "InstaLOD Mesh Optimization"),
			FSlateIcon(FInstaLODPluginStyle::GetStyleSetName(), "InstaLODUI.TabIcon")
		);
		InstaLODButton.SetCommandList(PluginCommands);

		Content.AddEntry(InstaLODButton);
	}

	// register details customizations
	PropertyModule.RegisterCustomClassLayout("InstaLODBaseTool", FOnGetDetailCustomizationInstance::CreateRaw(this, &FInstaLODUIModule::CreateBaseToolCustomization));
	PropertyModule.RegisterCustomClassLayout("InstaLODSettings", FOnGetDetailCustomizationInstance::CreateRaw(this, &FInstaLODUIModule::CreateSettingsCustomization));
	PropertyModule.RegisterCustomClassLayout("InstaLODOptimizeTool", FOnGetDetailCustomizationInstance::CreateRaw(this, &FInstaLODUIModule::CreateOptimizeToolCustomization));
	PropertyModule.RegisterCustomPropertyTypeLayout("InstaLODMaterialSettings", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FInstaLODMaterialSettingsCustomizations::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged();
	
	InstallExtensions();
}

void FInstaLODUIModule::ShutdownModule()
{
	RemoveExtensions();

	FInstaLODUICommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(InstaLODWindowTabName);
}

void FInstaLODUIModule::OpenInstaLODWindowClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(InstaLODWindowTabName);
}

void FInstaLODUIModule::OpenInstaLODWindowClickedStaticMeshEditor()
{
	FGlobalTabmanager::Get()->TryInvokeTab(InstaLODWindowTabName);
}

void FInstaLODUIModule::OpenInstaLODWindowClickedSkeletalMeshEditor()
{
	FGlobalTabmanager::Get()->TryInvokeTab(InstaLODWindowTabName);
}

void FInstaLODUIModule::OnAssetEditorRequestedOpen(UObject* OpenedAsset)
{
}

TSharedRef<class SDockTab> FInstaLODUIModule::OnSpawnInstaLODTab(const class FSpawnTabArgs& SpawnTabArgs)
{
	// ensure that the user setup InstaLOD as the mesh reduction module
	FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
	IMeshReductionModule& ReductionModule = FModuleManager::Get().LoadModuleChecked<IMeshReductionModule>("MeshReductionInterface");
	
	if (ReductionModule.GetSkeletalMeshReductionInterface() != InstaLODModule.GetSkeletalMeshReductionInterface() ||
		ReductionModule.GetStaticMeshReductionInterface() != InstaLODModule.GetStaticMeshReductionInterface() ||
		ReductionModule.GetMeshMergingInterface() != InstaLODModule.GetMeshMergingInterface())
	{
		TSharedRef<SDockTab> DockTab =
		SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
		 
		 SNew(SVerticalBox)
		 +SVerticalBox::Slot()
		 .AutoHeight()
		 .HAlign(HAlign_Left)
		 .MaxHeight(32.f)
		 [
		  // InstaLOD Logo
		  SNew(SImage)
		  .Image(FInstaLODPluginStyle::Get().GetBrush("InstaLODMeshReduction.LogoTiny"))
		  ]
		 
		 +SVerticalBox::Slot()
		 .AutoHeight()
		 [
		  SNew(STextBlock)
		  .Text(NSLOCTEXT("InstaLODUI", "BadSetup_Title", "InstaLOD is not setup properly!"))
		  .AutoWrapText(true)
		  .Justification(ETextJustify::Center)
		  .ColorAndOpacity(FSlateColor(FLinearColor(FColor::Red)))
		  .TextStyle( &FInstaLODPluginStyle::Get().GetWidgetStyle<FTextBlockStyle>( "InstaLODMeshReduction.H2" ) )
		]
		 
		 +SVerticalBox::Slot()
		 .AutoHeight()
		 [
		  SNew(STextBlock)
		  .Text(NSLOCTEXT("InstaLODUI", "BadSetup_Message",
						  "\n\n1. Please open the 'Project Settings' and select the 'Hierarchical LOD Mesh Simplification' category.\nChoose 'InstaLODMeshReduction' as 'Hierarchical LOD Mesh Reduction' plugin."
						  "\n\n2. Select the 'Mesh Simplification' category.\nChoose 'InstaLODMeshReduction' as 'Mesh Reduction' plugin."
						  "\n\n3. Select the 'Skeletal Mesh Simplification' category.\nChoose 'InstaLODMeshReduction' as 'Skeletal Mesh Reduction' plugin."
			 ))

		  .AutoWrapText(true)
		  .Justification(ETextJustify::Center)
		  .ColorAndOpacity(FSlateColor(FLinearColor(FColor::Red)))
		  .TextStyle( &FInstaLODPluginStyle::Get().GetWidgetStyle<FTextBlockStyle>( "InstaLODMeshReduction.H5" ) )
		  ]
		 
		];
		
		return DockTab;
	}
	
	// init our tool classes, cleaned up when the tab is closed.
	InitToolClasses();

	// construct toolbar widget
	TSharedRef<SInstaLODWindow> InstaLODWindow = 
		SNew(SInstaLODWindow)
		.ToolsToRegister(Tools);

	TSharedRef<SDockTab> DockTab =
		SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			InstaLODWindow
		];

	DockTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FInstaLODUIModule::OnInstaLODTabClosed));

	return DockTab;
}


void FInstaLODUIModule::InitToolClasses()
{
	Tools.Empty();

	// iterate over all classes and filter the children of our UInstaLODBaseTool
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UInstaLODBaseTool::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
		{
			UClass* Class = *It;
			UInstaLODBaseTool* NewTool = NewObject<UInstaLODBaseTool>(GetTransientPackage(), Class);
			NewTool->AddToRoot();
			Tools.Add(NewTool);
		}
	}

	Tools.Sort(UInstaLODBaseTool::OrderById);
}

void FInstaLODUIModule::OnModulesChanged(FName Module, EModuleChangeReason Reason)
{
	//UE_LOG(LogTemp, Warning, TEXT("## MODULE LOADED: %s ##"), *Module.ToString());

	if (Module == TEXT("LevelEditor") && Reason == EModuleChangeReason::ModuleLoaded)
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

		// replace default images
		auto Style = const_cast<FSlateStyleSet*>(static_cast<const FSlateStyleSet*>(&FInstaLODPluginStyle::Get()));
		FSlateImageBrush InstaLODBrush(Style->RootToContentDir("InstaLOD_Logo_302x64", TEXT(".png")), FVector2D(302, 64));
		*const_cast<FSlateBrush*>(FAppStyle::GetBrush("MeshProxy.SimplygonLogo")) = InstaLODBrush;

		// add toolbar extension
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender());
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FInstaLODUIModule::AddToolbarExtension, FInstaLODUICommands::Get().OpenInstaLODWindow));
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

		// add menu extension
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("General", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FInstaLODUIModule::AddMenuExtension, FInstaLODUICommands::Get().OpenInstaLODWindow));
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	else if (Module == TEXT("StaticMeshEditor") && Reason == EModuleChangeReason::ModuleLoaded)
	{
		IStaticMeshEditorModule& StaticMeshEditorModule = FModuleManager::LoadModuleChecked<IStaticMeshEditorModule>("StaticMeshEditor");
		
		// add toolbar extension
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender());
		ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FInstaLODUIModule::AddToolbarExtension, FInstaLODUICommands::Get().OpenInstaLODWindowFromStaticMeshEditor));
		StaticMeshEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

		// add menu extension
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("MeshChange", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FInstaLODUIModule::AddMenuExtension, FInstaLODUICommands::Get().OpenInstaLODWindowFromStaticMeshEditor));
		StaticMeshEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

		UToolMenu *const WindowMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window");
		if (FToolMenuSection* const LevelEditorSection = WindowMenu->FindSection("LevelEditor"))
		{
				LevelEditorSection->AddMenuEntry("OpenInstaLOD",
				LOCTEXT("InstaLODOpenWindow", "InstaLOD"),
				LOCTEXT("OpenBridgeTab_Desc", "Op."),
				FSlateIcon(FInstaLODPluginStyle::GetStyleSetName(), "InstaLODUI.TabIcon"),
				FUIAction(FExecuteAction::CreateRaw(this, &FInstaLODUIModule::OpenInstaLODWindowClicked), FCanExecuteAction()));
		}
	}
	else if (Module == TEXT("SkeletalMeshEditor") && Reason == EModuleChangeReason::ModuleLoaded)
	{
		ISkeletalMeshEditorModule& SkeletalMeshEditorModule = FModuleManager::LoadModuleChecked<ISkeletalMeshEditorModule>("SkeletalMeshEditor");

		// add toolbar extension
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

		ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FInstaLODUIModule::AddToolbarExtension, FInstaLODUICommands::Get().OpenInstaLODWindowFromSkeletalMeshEditor));
		SkeletalMeshEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

		// add menu extension
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("AssetEditorActions", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FInstaLODUIModule::AddMenuExtension, FInstaLODUICommands::Get().OpenInstaLODWindowFromSkeletalMeshEditor));
		SkeletalMeshEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FInstaLODUIModule::InstallExtensions()
{
	FModuleManager::Get().OnModulesChanged().AddRaw(this, &FInstaLODUIModule::OnModulesChanged);

	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		OnModulesChanged("LevelEditor", EModuleChangeReason::ModuleLoaded);
	}
	if (FModuleManager::Get().IsModuleLoaded("StaticMeshEditor"))
	{
		OnModulesChanged("StaticMeshEditor", EModuleChangeReason::ModuleLoaded);
	}
	if (FModuleManager::Get().IsModuleLoaded("SkeletalMeshEditor"))
	{
		OnModulesChanged("SkeletalMeshEditor", EModuleChangeReason::ModuleLoaded);
	}
}

void FInstaLODUIModule::RemoveExtensions()
{
	FModuleManager::Get().OnModulesChanged().RemoveAll(this);
}

void FInstaLODUIModule::OnInstaLODTabClosed(TSharedRef<class SDockTab> ClosedTab)
{
	// make sure to remove the tools from root
	for (UObject* Tool : Tools)
	{
		if (Tool)
		{
			Tool->SaveConfig();
			Tool->RemoveFromRoot();
		}
	}
}

TSharedRef<IDetailCustomization> FInstaLODUIModule::CreateBaseToolCustomization()
{
	return MakeShareable(new FInstaLODBaseToolCustomization);
}

TSharedRef<IDetailCustomization> FInstaLODUIModule::CreateOptimizeToolCustomization()
{
	return MakeShareable(new FInstaLODOptimizeToolCustomization);
}

TSharedRef<IDetailCustomization> FInstaLODUIModule::CreateSettingsCustomization()
{
	return MakeShareable(new FInstaLODSettingsCustomization);
}

void FInstaLODUIModule::AddToolbarExtension(FToolBarBuilder& Builder, TSharedPtr<FUICommandInfo> UICommand)
{
	Builder.AddToolBarButton(UICommand);
}

void FInstaLODUIModule::AddMenuExtension(FMenuBuilder& Builder, TSharedPtr<FUICommandInfo> UICommand)
{
	Builder.AddMenuEntry(UICommand);
}

IMPLEMENT_MODULE(FInstaLODUIModule, InstaLODUI);

#undef LOCTEXT_NAMESPACE

