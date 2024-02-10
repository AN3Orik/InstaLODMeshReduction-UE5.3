/**
 * InstaLODSettings.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODSettings.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODSettings.h"
#include "InstaLODUIPCH.h"

#include "Interfaces/IPluginManager.h"

#include "InstaLODModule.h"
#include "InstaLODUIModule.h"
#include "Utilities/InstaLODUtilities.h"
#include "Slate/InstaLODWindow.h"

#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

UInstaLODSettings::UInstaLODSettings()
{
	FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");

	InstaLOD::IInstaLOD* InstaLODAPI = InstaLODModule.GetInstaLODAPI();

	// fetch license information and version
	if (InstaLODAPI)
	{
		LicenseInformation = FText::FromString(ANSI_TO_TCHAR(InstaLODAPI->GetAuthorizationInformation()));
		SDKVersion = FText::FromString(ANSI_TO_TCHAR(InstaLODAPI->GetBuildDate()));
		bAuthorized = InstaLODAPI->IsHostAuthorized();
	}
}

/************************************************************************/
/* Utilities                                                            */
/************************************************************************/

void UInstaLODSettings::ClearLODsFromSelection()
{
	if (MeshComponents.Num() == 0)
		return;

	const FText MessageContents = FText::FromString("Do you really want to clear the selection of all LODs?\nThis will remove all LODs for all Instances of this Mesh and is not undoable.");
	const FText MessageTitle = LOCTEXT("InstaLODClearSelectionTitle", "InstaLOD: Clear Selection of all LODs?");

	if (FMessageDialog::Open(EAppMsgType::Ok, MessageContents, &MessageTitle) == EAppReturnType::Cancel)
		return;

	for (TSharedPtr<FInstaLODMeshComponent> MeshComponent : MeshComponents)
	{
		if (MeshComponent.IsValid())
		{
			if (MeshComponent->StaticMeshComponent.IsValid())
			{
				UStaticMesh *const Mesh = MeshComponent->StaticMeshComponent->GetStaticMesh();

				if (Mesh == nullptr)
					continue;

				UInstaLODUtilities::RemoveAllLODsFromStaticMesh(Mesh);
				Mesh->SetLODGroup(NAME_None);
			}
			else if (MeshComponent->SkeletalMeshComponent.IsValid())
			{
				UInstaLODUtilities::RemoveAllLODsFromSkeletalMesh(MeshComponent->SkeletalMeshComponent->GetSkeletalMeshAsset());
			}
		}
	}
}

void UInstaLODSettings::AuthorizeWorkstation()
{
	if (AccountName.IsEmpty() == false && SerialPassword.IsEmpty() == false)
	{
		// callback to the InstaLODAPI
		FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
		if (InstaLODModule.GetInstaLODAPI())
		{
			if (InstaLODModule.GetInstaLODAPI()->AuthorizeMachine(TCHAR_TO_UTF8(*AccountName.ToString()), TCHAR_TO_UTF8(*SerialPassword.ToString())))
			{
				const FText MessageTitle = NSLOCTEXT("InstaLODUI", "InstaLODAuthorization", "InstaLOD: Machine Authorization");
				const FText AuthorizedMessageContent = NSLOCTEXT("InstaLODUI", "InstaLODAuthorizationCompleted", "A valid InstaLOD license has been acquired for this machine.\n\nInstaLOD will automatically refresh the license if necessary.");
				FMessageDialog::Open(EAppMsgType::Ok, AuthorizedMessageContent, &MessageTitle);
				
				LicenseInformation = FText::FromString(ANSI_TO_TCHAR(InstaLODModule.GetInstaLODAPI()->GetAuthorizationInformation()));
				bAuthorized = InstaLODModule.GetInstaLODAPI()->IsHostAuthorized();
				
				// This will result in everything being refreshed
				GetInstaLODWindow()->UpdateToolbar();
				GetInstaLODWindow()->ForceRefreshDetailsView();
				
			}
			else
			{
				FText Title = NSLOCTEXT("InstaLODUI", "AuthorizeError_Title", "Authorization Error");
				FString AuthorizationInformation = UTF8_TO_TCHAR(InstaLODModule.GetInstaLODAPI()->GetAuthorizationInformation());
				
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to authorize machine:\n\n" + AuthorizationInformation), &Title);
			}
		}
		else
		{
			FText Title = NSLOCTEXT("InstaLODUI", "AuthorizeError_Title", "Couldn't retrieve InstaLOD Module.");
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("InstaLODUI", "AuthorizeError_Message", "Please make sure the InstaLOD Module loaded properly!"), &Title);
		}
	}
	else
	{
		FText Title = NSLOCTEXT("InstaLODUI", "AuthorizeMessage_Title", "No Username or Password!");
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("InstaLODUI", "AuthorizeMessage_Message", "Please enter Username and Password!"), &Title);
	}
}

void UInstaLODSettings::DeauthorizeWorkstation()
{
	if (AccountName.IsEmpty() == false && SerialPassword.IsEmpty() == false)
	{
		// callback to the InstaLODAPI
		FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
		if (InstaLODModule.GetInstaLODAPI()->DeauthorizeMachine(TCHAR_TO_UTF8(*AccountName.ToString()), TCHAR_TO_UTF8(*SerialPassword.ToString())))
		{
			LicenseInformation = FText::FromString(ANSI_TO_TCHAR(InstaLODModule.GetInstaLODAPI()->GetAuthorizationInformation()));
			bAuthorized = InstaLODModule.GetInstaLODAPI()->IsHostAuthorized();
			
			// this will result in everything being refreshed
			GetInstaLODWindow()->UpdateToolbar();
			GetInstaLODWindow()->ForceRefreshDetailsView();
		}
	}
	else
	{
		FText Title = NSLOCTEXT("InstaLODUI", "DeauthorizeMessage_Title", "No Username or Password!");
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("InstaLODUI", "DeauthorizeMessage_Message", "Please enter Username and Password!"), &Title);
	}
}

void UInstaLODSettings::ResetToolSettings()
{
	FInstaLODUIModule& InstaLODUIModule = FModuleManager::LoadModuleChecked<FInstaLODUIModule>("InstaLODUI");
	TArray<UInstaLODBaseTool*> Tools = InstaLODUIModule.GetAllTools();

	for (UInstaLODBaseTool* Tool : Tools)
	{
		if (Tool)
		{
			Tool->ResetSettings();
		}
	}
}

void UInstaLODSettings::LoadProfile()
{
	IDesktopPlatform* const Desktop = FDesktopPlatformModule::Get();
	if (Desktop == nullptr)
		return;

	const FString FileTypes = "(InstaLOD JSON Profile)|*.json";
	TArray<FString> OutFileNames;

	if (!Desktop->OpenFileDialog(nullptr, "Load InstaLOD Profile", FString(), FString(), FileTypes, EFileDialogFlags::None, OutFileNames))
		return;

	check(OutFileNames.Num() == 1);

	const FString FilePath = OutFileNames[0];

	if (FilePath.Len() == 0)
		return;

	FString JsonContent;

	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: Reading InstaLOD JSON Profile file failed."));
		return;
	}

	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonContent);
	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);

	if (!FJsonSerializer::Deserialize(JsonReader, Json, FJsonSerializer::EFlags::None))
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
		return;
	}

	if (!Json.IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
		return;
	}

	if (!Json->HasField("Operation"))
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
		return;
	}
	else
	{
		FString OutOperation;
		if (!Json->TryGetStringField("Operation", OutOperation))
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
			return;
		}
		if (OutOperation != "MeshOperation")
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
			return;
		}
	}

	TSharedPtr<FJsonValue> MeshOperationValue = Json->TryGetField("MeshOperation");

	if (!MeshOperationValue.IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
		return;
	}

	TSharedPtr<FJsonObject> MeshOperation = MeshOperationValue->AsObject();

	if (!MeshOperation.IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
		return;
	}

	if (!MeshOperation->HasField("Entries"))
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("JSON profile is missing the required 'Entries' field."));
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;

	if (!MeshOperation->TryGetArrayField("Entries", Entries) ||
		Entries == nullptr ||
		Entries->Num() == 0 ||
		Entries->Num() > 1)
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: Only JSON profiles with a single mesh operation are supported."));
		return;
	}

	TSharedPtr<FJsonObject> MeshOperationEntry = (*Entries)[0]->AsObject();

	if (!MeshOperationEntry.IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: JSON parsing failed."));
		return;
	}

	if (!MeshOperationEntry->HasField("Type"))
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("JSON profile is missing the required 'Type' field."));
		return;
	}

	const FString MeshOperationType = MeshOperationEntry->GetStringField("Type");

	FInstaLODUIModule& InstaLODUIModule = FModuleManager::LoadModuleChecked<FInstaLODUIModule>("InstaLODUI");
	TArray<UInstaLODBaseTool*> Tools = InstaLODUIModule.GetAllTools();

	for (UInstaLODBaseTool *const Tool : Tools)
	{
		if (Tool == nullptr)
			continue;

		if (Tool->ReadSettingsFromJSONObject(MeshOperationEntry))
		{
			InstaLODWindow->SetCurrentActiveTabIndex(Tool->GetOrderId() - 1);
			break;
		}
	}
}

void UInstaLODSettings::OnlineHelp()
{
	FText Title = NSLOCTEXT("InstaLODUI", "OnlineHelp_Title", "Leaving UE");
	EAppReturnType::Type Return = FMessageDialog::Open(EAppMsgType::YesNo, NSLOCTEXT("InstaLODUI", "OnlineHelp_Message", "This will open your default browser. Are you sure?"), &Title);

	if (Return == EAppReturnType::Yes)
	{
		FString TheURL = "http://www.InstaLOD.com/GettingStartedWithUE2018";
		FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);
	}
}

void UInstaLODSettings::SubmitFeedback()
{
	FText Title = NSLOCTEXT("InstaLODUI", "SubmitFeedback_Title", "Leaving UE");
	EAppReturnType::Type Return = FMessageDialog::Open(EAppMsgType::YesNo, NSLOCTEXT("InstaLODUI", "SubmitFeedback_Message", "This will open your default email client/webbrowser. Are you sure?"), &Title);

	if (Return == EAppReturnType::Yes)
	{
		const FString licenseString = LicenseInformation.ToString();

		if (bAuthorized && licenseString.Contains("[InstaLOD_FSL]"))
		{
			FPlatformProcess::LaunchURL(TEXT("https://www.InstaLOD.com/questions"), nullptr, nullptr);
			return;
		}
		
		// NOTE: send unauthorized users to email for now.
		FString FeedbackEmail = FString("mailto:support@InstaLOD.com"
										"?Subject=InstaLOD SDK2020 Feedback"
										"&Body=Please enter your feedback below this line:\n"
										"----------------------------------------------------------\n"
										"\n\n\n"
										"\n\n\n"
										"\n\n\n"
										"----------------------------------------------------------\n"
										"Thank you for submitting feedback!\n"
										"\n\n\n"
										"App: InstaLOD for Unreal Engine 2020\n") +
		FString("Version: ") + SDKVersion.ToString() + FString("\n") +
		FString("License Info:\n") + ((bAuthorized == true) ? licenseString : FString("N.A.")) + FString("\n");
		
		FeedbackEmail = FeedbackEmail.Replace(TEXT("\r\n"), TEXT("%0D%0A"));
		FeedbackEmail = FeedbackEmail.Replace(TEXT("\n"), TEXT("%0D%0A"));
		FeedbackEmail = FeedbackEmail.Replace(TEXT(" "), TEXT("%20"));
		
		FPlatformProcess::LaunchURL(*FeedbackEmail, nullptr, nullptr);
	}
}

/************************************************************************/
/* UInstaLODBaseTool Interface                                          */
/************************************************************************/

FText UInstaLODSettings::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "SettingsFriendlyName", "?");
}

FText UInstaLODSettings::GetToolBarToolTip() const
{
	return NSLOCTEXT("InstaLODUI", "SettingsToolTip", "Shows your License Information and more.");
}

FText UInstaLODSettings::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "SettingsToolComboBoxItemName", "Setup");
}

FText UInstaLODSettings::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "SettingsToolOperationInformation", "");
}

int32 UInstaLODSettings::GetOrderId() const
{
	// just a big number so this is always the last
	return 9999;
}


#undef LOCTEXT_NAMESPACE
