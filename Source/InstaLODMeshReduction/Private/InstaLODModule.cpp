/**
 * InstaLODModule.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODModule.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODMeshReductionPCH.h"
#include "InstaLODModule.h"

#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "ComponentReregisterContext.h"
#include "Slate/InstaLODPluginStyle.h"
#include "Misc/ConfigUtilities.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "EditorSupportDelegates.h"
#include "PersonaModule.h"
#include "StaticMeshEditorModule.h"
#include "Widgets/Input/STextComboBox.h"
#include "LevelEditor.h"

#include "Interfaces/IPluginManager.h"

#include "Editor/EditorStyle/Private/SlateEditorStyle.h"

#define GetInstaLODPtr()(GInstaLOD.Get())

TUniquePtr<FInstaLOD> GInstaLOD;

const FString InstaLODShared::TargetedUnrealEngineVersion = TEXT("Compatible UE Version: 5.2");
FString InstaLODShared::Version = TEXT("6.2");
FString InstaLODShared::LicenseInformation = TEXT("Unauthorized");

#define LOCTEXT_NAMESPACE "InstaLOD"

#define UE_INSTALOD_LIBRARY_NAME	"InstaLOD"

// library file name depends on target platform
#if defined (INSTALOD_LIB_DYNAMIC)
#	if PLATFORM_WINDOWS
#		if PLATFORM_64BITS
#			define INSTALOD_LIB_DYNAMIC_PATH	TEXT("DLLs/Win64/") TEXT(UE_INSTALOD_LIBRARY_NAME) TEXT(".dll")
#		endif
#   elif PLATFORM_MAC
#       define INSTALOD_LIB_DYNAMIC_PATH		TEXT("DLLs/Mac/lib" UE_INSTALOD_LIBRARY_NAME ".dylib")
#   elif PLATFORM_LINUX
#		define INSTALOD_LIB_DYNAMIC_PATH		TEXT("DLLs/Linux/lib" UE_INSTALOD_LIBRARY_NAME ".so")
#	endif
#	ifndef INSTALOD_LIB_DYNAMIC_PATH
#		error Platform not supported by InstaLOD
#	endif
#else
#	error InstaLOD supports only dynamically linking the SDK
#endif

IMeshReduction* FInstaLODModule::GetSkeletalMeshReductionInterface() { return GetInstaLODPtr(); }
IMeshReduction* FInstaLODModule::GetStaticMeshReductionInterface() { return GetInstaLODPtr(); }
IMeshMerging* FInstaLODModule::GetMeshMergingInterface() 
{
	return GetInstaLODPtr();
}

IInstaLOD* FInstaLODModule::GetInstaLODInterface() {
	return GetInstaLODPtr();
}

void FInstaLODModule::StartupModule()
{	
	InstaLODAPI = nullptr;

#if defined (INSTALOD_LIB_DYNAMIC)
	const FString PluginBaseDir = IPluginManager::Get().FindPlugin("InstaLODMeshReduction")->GetBaseDir();
	const auto libraryFileName = INSTALOD_LIB_DYNAMIC_PATH;
	bool bLoaded = false;
	
	// try loading from relative plugins path first
	FString LibraryPath = FPaths::Combine(*PluginBaseDir, libraryFileName);
	
	// if relative path is not found, then try to load from engine dir
	if (!FPaths::FileExists(LibraryPath))
		LibraryPath = FPaths::Combine(*FPaths::EnginePluginsDir(), TEXT("InstaLODMeshReduction/"), libraryFileName);
	
	// if engine path is not found, then try to load from project dir
	if (!FPaths::FileExists(LibraryPath))
		LibraryPath = FPaths::Combine(*FPaths::ProjectPluginsDir(), TEXT("InstaLODMeshReduction/"), libraryFileName);
	
	if (FPaths::FileExists(LibraryPath))
	{
		if (void* pLibraryHandle = FPlatformProcess::GetDllHandle(*LibraryPath))
		{
			if (pfnGetInstaLOD pGetInstaLOD = (pfnGetInstaLOD)FPlatformProcess::GetDllExport(pLibraryHandle, TEXT("GetInstaLOD")))
			{
				if (pGetInstaLOD(INSTALOD_API_VERSION, &InstaLODAPI) != 0)
				{
					bLoaded = true;
					UE_LOG(LogInstaLOD, Log, TEXT("%s"), UTF8_TO_TCHAR(InstaLODAPI->GetBuildDate()));
				}
			}
		}
	}
	
	if (!bLoaded)
	{
		UE_LOG(LogInstaLOD, Fatal, TEXT("Unable to load InstaLOD Library. Please ensure that the InstaLOD library is in the library directory. If InstaLOD still fails to load please reinstall InstaLOD."));
	}
#else
	GetInstaLOD(INSTALOD_API_VERSION, &InstaLODAPI);
	UE_LOG(LogInstaLOD, Display, TEXT("%s"), UTF8_TO_TCHAR(InstaLODAPI->GetBuildDate()));
#endif
	
	GInstaLOD.Reset(FInstaLOD::Create(InstaLODAPI));
	
#if PLATFORM_WINDOWS
	// NOTE: InstaLOD.dll is built with MB, this is causing the secureCRT to panic when logging to a unicode stdout during cook
	// therefore it is important to disable all stdout logs
	InstaLODAPI->SetStandardOutputEnabled(false);
#endif

	if (!InstaLODAPI->InitializeAuthorization("InstaLOD", nullptr))
	{
		UE_LOG(LogInstaLOD, Fatal, TEXT("Failed to initialize InstaLOD authorization module with error: %s"), UTF8_TO_TCHAR(InstaLODAPI->GetAuthorizationInformation()));
	}
 
	// setup the toolbar button
	if (GInstaLOD != NULL)
	{
		InstaLODShared::Version = UTF8_TO_TCHAR(InstaLODAPI->GetBuildDate());
		InstaLODShared::LicenseInformation = UTF8_TO_TCHAR(InstaLODAPI->GetAuthorizationInformation());
		
		UE_LOG(LogInstaLOD, Log, TEXT("%s"), *InstaLODShared::LicenseInformation);
		
		FInstaLODPluginStyle::Initialize();
		FInstaLODPluginStyle::ReloadTextures();
		
		InstallHooks();
		
		IModularFeatures::Get().RegisterModularFeature(IMeshReductionModule::GetModularFeatureName(), this);

		const FString BaseInstaLODMeshReductionIni = FConfigCacheIni::NormalizeConfigIniPath(FPaths::ProjectPluginsDir() + "InstaLODMeshReduction/Config/BaseInstaLODMeshReduction.ini");
		UE::ConfigUtilities::ApplyCVarSettingsFromIni(TEXT("Startup"), *BaseInstaLODMeshReductionIni, ECVF_SetByConsoleVariablesIni);
	}
}

void FInstaLODModule::ShutdownModule()
{
	GInstaLOD = NULL;

	FInstaLODPluginStyle::Shutdown();
	
	IModularFeatures::Get().UnregisterModularFeature(IMeshReductionModule::GetModularFeatureName(), this);
}

void FInstaLODModule::InstallHooks()
{
	LateHooksDelegateHandle = FEditorSupportDelegates::UpdateUI.AddLambda([this]() { this->InstallHooksLate(); } );
}

void FInstaLODModule::InstallHooksLate()
{	
	FEditorSupportDelegates::UpdateUI.Remove(LateHooksDelegateHandle);
	
	if (!GInstaLOD->GetInstaLOD()->IsHostAuthorized())
	{
		UE_LOG(LogInstaLOD, Log, TEXT("Machine not authorized: %s"), UTF8_TO_TCHAR(GInstaLOD->GetInstaLOD()->GetAuthorizationInformation()));
		
		const TConsoleVariableData<int32>* ShowAuthorizationWindowCVarIdentifier = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("InstaLOD.ShowAuthorizationWindow"));
		
		check(ShowAuthorizationWindowCVarIdentifier != nullptr);
		
		if (ShowAuthorizationWindowCVarIdentifier->GetValueOnAnyThread() != 0)
		{
			InstaLODShared::OpenAuthorizationWindowModal();
		}
	}
}

class SInstaLODDialogWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SInstaLODDialogWidget ){}
	SLATE_DEFAULT_SLOT( FArguments, Content )
	SLATE_END_ARGS()
	
	void Construct( const FArguments& InArgs )
	{
		TSharedPtr< SScrollBox > ScrollBox;
		
		this->ChildSlot
		[
		 SNew(SVerticalBox)
		 +SVerticalBox::Slot()
		 .AutoHeight()
		 .MaxHeight(300.0f)
		 [
		  SAssignNew(ScrollBox, SScrollBox)
		  ]
		 
		 +SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
			 .Text( NSLOCTEXT("UnrealEd", "OK", "OK") )
			 .OnClicked(this, &SInstaLODDialogWidget::OnOK_Clicked)
			 ]
		 ];
		
		ScrollBox->AddSlot()
		[
		 InArgs._Content.Widget
		 ];
	}
	
	/** Sets the window of this dialog. */
	void SetWindow( TSharedPtr<SWindow> InWindow )
	{
		MyWindow = InWindow;
	}
	
	UNREALED_API static void OpenDialog(const FText& InDialogTitle, const TSharedRef< SWidget >& DisplayContent)
	{
		TSharedPtr< SWindow > Window;
		TSharedPtr< SInstaLODDialogWidget > InstaLODDialogWidget;
		
		Window = SNew(SWindow)
		.Title(InDialogTitle)
		.SizingRule( ESizingRule::Autosized )
		.SupportsMaximize(false) .SupportsMinimize(false)
		[
			SNew( SBorder )
			.Padding( 4.f )
			.BorderImage( FAppStyle::GetBrush( "ToolPanel.GroupBorder" ) )
			[
				SAssignNew(InstaLODDialogWidget, SInstaLODDialogWidget)
				[
					DisplayContent
				 ]
			 ]
		 ];
		
		InstaLODDialogWidget->SetWindow(Window);
		
		FSlateApplication::Get().AddWindow( Window.ToSharedRef() );
	}
	
private:
	FReply OnOK_Clicked(void)
	{
		MyWindow.Pin()->RequestDestroyWindow();
		
		return FReply::Handled();
	}
	
private:
	/** Pointer to the containing window. */
	TWeakPtr< SWindow > MyWindow;
};


void InstaLODShared::OpenAuthorizationWindowModal()
{
	TSharedRef< SEditableText > UsernameEditableText = SNew(SEditableText);
	TSharedRef< SEditableText > PasswordEditableText = SNew(SEditableText).IsPassword(true);
	
	const FText MessageTitle = NSLOCTEXT("InstaLODUI", "InstaLODAuthorization", "InstaLOD: Machine Authorization");
	const FText MessageContent = NSLOCTEXT("InstaLODUI", "InstaLODAuthorizationMessage", "This machine is not authorized for InstaLOD.\nPlease enter your license data to authorize this machine.\n\nPlease obtain a valid license or remove InstaLOD from your project.");
	const FText MessageFooter = NSLOCTEXT("InstaLODUI", "InstaLODAuthorizationFooter", "Visit http://www.InstaLOD.com or contact hello@InstaLOD.com\nfor information on how to obtain a valid InstaLOD license.");
	
	auto AuthorizeMachineClickLambda = [UsernameEditableText, PasswordEditableText]()
	{
		const FString Username = UsernameEditableText->GetText().ToString();
		const FString Password = PasswordEditableText->GetText().ToString();
		
		if (!GInstaLOD->GetInstaLOD()->AuthorizeMachine(TCHAR_TO_UTF8(*Username), TCHAR_TO_UTF8(*Password)))
		{
			// NOTE: the authorization information contains information about the error
			InstaLODShared::LicenseInformation = UTF8_TO_TCHAR(GInstaLOD->GetInstaLOD()->GetAuthorizationInformation());
			
			FText ErrorMessageContents = FText::FromString(InstaLODShared::LicenseInformation);
			FText ErrorMessageTitle = LOCTEXT("InstaLODAcquiredLicenseFailTitle", "InstaLOD: Failed to acquire license");
			FMessageDialog::Open(EAppMsgType::Ok, ErrorMessageContents, &ErrorMessageTitle);
		}
		else
		{
			// NOTE: the authorization information contains information about the error
			InstaLODShared::LicenseInformation = UTF8_TO_TCHAR(GInstaLOD->GetInstaLOD()->GetAuthorizationInformation());
			
			FText SuccessMessageContents = FText::Format(LOCTEXT("InstaLODLicenseAcquired", "{0}\n"), FText::FromString(InstaLODShared::LicenseInformation));
			FText SuccessMessageTitle = LOCTEXT("InstaLODAcquiredLicenseSuccessTitle", "InstaLOD: Acquired license");
			FMessageDialog::Open(EAppMsgType::Ok, SuccessMessageContents, &SuccessMessageTitle);
		}
		
		return FReply::Handled();
	};
	
	const TSharedRef< SWidget > DisplayContent =
	SNew(SVerticalBox)
	+SVerticalBox::Slot()
	.AutoHeight()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	.Padding(10)
	[
		// InstaLOD logo
		SNew(SImage)
		.Image(FInstaLODPluginStyle::Get().GetBrush("InstaLODMeshReduction.LogoSmall"))
	 ]
	
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Justification(ETextJustify::Center)
		.Text(MessageContent)
	 ]
	
	+SVerticalBox::Slot()
	.Padding(0, 20, 0, 0)
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("InstaLODAuthorizationUserEmail", "Username or Email"))
	 ]
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SBorder)
		.Padding( 5.0f )
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			UsernameEditableText
		 ]
	 ]
	
	+SVerticalBox::Slot()
	.Padding(0, 10, 0, 0)
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("InstaLODAuthorizationPassword", "Password or License Key"))
	 ]
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SBorder)
		.Padding( 5.0f )
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			PasswordEditableText
		 ]
	 ]
	
	+SVerticalBox::Slot()
	.HAlign(HAlign_Right)
	.AutoHeight()
	.Padding(0.0f, 10, 0, 0)
	[
	 SNew(SButton)
	 .Text( LOCTEXT("InstaLODAuthorizationAuthorize", "Authorize") )
	 .OnClicked_Lambda(AuthorizeMachineClickLambda)
	 ]
	
	+SVerticalBox::Slot()
	.Padding(0, 20, 0, 20)
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(MessageFooter)
	 ]
	;
	
	TSharedRef< SInstaLODDialogWidget > InstaLODDialogWidget =
	SNew(SInstaLODDialogWidget)
	[
		DisplayContent
	 ];
	
	
	TSharedPtr< SWindow > Window = SNew(SWindow)
	.Title(MessageTitle)
	.SizingRule( ESizingRule::Autosized )
	.SupportsMaximize(false) .SupportsMinimize(false)
	[
		SNew( SBorder )
		.Padding( 10.0f )
		.BorderImage(FAppStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			InstaLODDialogWidget
		 ]
	 ];
	InstaLODDialogWidget->SetWindow(Window);
	
	GEditor->EditorAddModalWindow(Window.ToSharedRef());
	
	if (!GInstaLOD->GetInstaLOD()->IsHostAuthorized())
	{
		if (FMessageDialog::Open(EAppMsgType::OkCancel, MessageContent, &MessageTitle) == EAppReturnType::Ok)
		{
			// we are authorized now!
		}
		else
		{
			// restart the auth loop
			OpenAuthorizationWindowModal();
		}
	}
	else
	{
		const FText AuthorizedMessageContent = NSLOCTEXT("InstaLODUI", "InstaLODAuthorizationCompleted", "A valid InstaLOD license has been acquired for this machine.\n\nInstaLOD will automatically refresh the license if necessary.");
		FMessageDialog::Open(EAppMsgType::Ok, AuthorizedMessageContent, &MessageTitle);
	}
}

void InstaLODShared::OpenDeauthorizationWindowModal()
{
	const FText MessageTitle = NSLOCTEXT("InstaLODUI", "InstaLODDeauthorizeNoLicense", "InstaLOD: Deauthorize");
	
	if (!GInstaLOD->GetInstaLOD()->IsHostAuthorized())
	{
		FText ErrorMessageContents = FText::FromString(InstaLODShared::LicenseInformation);
		FMessageDialog::Open(EAppMsgType::Ok, ErrorMessageContents, &MessageTitle);
		return;
	}
	
	TSharedRef< SEditableText > UsernameEditableText = SNew(SEditableText);
	TSharedRef< SEditableText > PasswordEditableText = SNew(SEditableText).IsPassword(true);
	
	const FText MessageContent = LOCTEXT("InstaLODDeauthorizationMessage", "Please enter your license data to deauthorize this machine.\n");
	const FText MessageFooter = LOCTEXT("InstaLODDeauthorizationFooter", "Visit http://www.InstaLOD.com or contact hello@InstaLOD.com\nfor information on how to obtain a valid InstaLOD license.");
	
	auto AuthorizeMachineClickLambda = [UsernameEditableText, PasswordEditableText, MessageTitle]()
	{
		const FString Username = UsernameEditableText->GetText().ToString();
		const FString Password = PasswordEditableText->GetText().ToString();
		
		if (!GInstaLOD->GetInstaLOD()->DeauthorizeMachine(TCHAR_TO_UTF8(*Username), TCHAR_TO_UTF8(*Password)))
		{
			// NOTE: the authorization information contains information about the error
			InstaLODShared::LicenseInformation = UTF8_TO_TCHAR(GInstaLOD->GetInstaLOD()->GetAuthorizationInformation());
			
			const FText ErrorMessageContents = FText::FromString(InstaLODShared::LicenseInformation);
			const FText ErrorMessageTitle = LOCTEXT("InstaLODDeauthorizationFailedTitle", "InstaLOD: Deauthorization failed");
			FMessageDialog::Open(EAppMsgType::Ok, ErrorMessageContents, &ErrorMessageTitle);
		}
		else
		{
			const FText SuccessMessageContents = FText::Format(LOCTEXT("InstaLODDeauthorized", "InstaLOD license removed and machine deauthorized.\n"), FText::FromString(InstaLODShared::LicenseInformation));
			
			FMessageDialog::Open(EAppMsgType::Ok, SuccessMessageContents, &MessageTitle);
		}
		
		return FReply::Handled();
	};
	
	const TSharedRef< SWidget > DisplayContent =
	SNew(SVerticalBox)
	+SVerticalBox::Slot()
	.AutoHeight()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	.Padding(10)
	[
		// InstaLOD logo
		SNew(SImage)
		.Image(FInstaLODPluginStyle::Get().GetBrush("InstaLODMeshReduction.LogoSmall"))
	 ]
	
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Justification(ETextJustify::Center)
		.Text(MessageContent)
	 ]
	
	+SVerticalBox::Slot()
	.Padding(0, 20, 0, 0)
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("InstaLODAuthorizationUserEmail", "Username or Email"))
	 ]
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SBorder)
		.Padding( 5.0f )
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			UsernameEditableText
		 ]
	 ]
	
	+SVerticalBox::Slot()
	.Padding(0, 10, 0, 0)
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("InstaLODAuthorizationPassword", "Password or License Key"))
	 ]
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SBorder)
		.Padding( 5.0f )
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			PasswordEditableText
		 ]
	 ]
	
	+SVerticalBox::Slot()
	.HAlign(HAlign_Right)
	.AutoHeight()
	.Padding(0.0f, 10, 0, 0)
	[
	 SNew(SButton)
	 .Text( LOCTEXT("InstaLODAuthorizationAuthorize", "Deauthorize") )
	 .OnClicked_Lambda(AuthorizeMachineClickLambda)
	 ]
	
	+SVerticalBox::Slot()
	.Padding(0, 20, 0, 20)
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(MessageFooter)
	 ]
	;
	
	TSharedRef< SInstaLODDialogWidget > InstaLODDialogWidget =
	SNew(SInstaLODDialogWidget)
	[
		DisplayContent
	];
	
	TSharedPtr< SWindow > Window = SNew(SWindow)
	.Title(MessageTitle)
	.SizingRule( ESizingRule::Autosized )
	.SupportsMaximize(false) .SupportsMinimize(false)
	[
		SNew( SBorder )
		.Padding( 10.0f )
		.BorderImage(FAppStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			InstaLODDialogWidget
		]
	];
	InstaLODDialogWidget->SetWindow(Window);
	
	GEditor->EditorAddModalWindow(Window.ToSharedRef());
}

IMPLEMENT_MODULE(FInstaLODModule, InstaLODMeshReduction);

#undef LOCTEXT_NAMESPACE
