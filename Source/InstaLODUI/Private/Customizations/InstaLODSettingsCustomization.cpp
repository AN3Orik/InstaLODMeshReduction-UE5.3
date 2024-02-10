/**
 * InstaLODSettingsCustomization.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODSettingsCustomization.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODUI/Private/Customizations/InstaLODSettingsCustomization.h"
#include "InstaLODUI/Private/InstaLODUIPCH.h"
#include "Slate/InstaLODPluginStyle.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"

#include "Tools/InstaLODSettings.h"

#define LOCTEXT_NAMESPACE "InstaLOD"

FInstaLODSettingsCustomization::FInstaLODSettingsCustomization()
	: DetailLayoutBuilder(nullptr)
{
}

FInstaLODSettingsCustomization::~FInstaLODSettingsCustomization()
{
	DetailLayoutBuilder = nullptr;
}

void FInstaLODSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailLayoutBuilder = &DetailBuilder;

	// retrieve the settings tool from the currently edited Tool (Object)
	UClass* SettingsClass = nullptr;
	UObject* Instance = nullptr;
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingEdited);

	if (ObjectsBeingEdited.Num() == 1)
	{
		Instance = ObjectsBeingEdited[0].Get();
		if (Instance)
		{
			Settings = Cast<UInstaLODSettings>(Instance);
			SettingsClass = Settings->GetClass();
		}
	}

	const bool bIsNodeLockedLicense = Settings->GetLicenseInfo().ToString().Contains("[InstaLOD_NodeLocked]");

	/************************************************************************/
	/* License Information                                                  */
	/************************************************************************/
	{
		IDetailCategoryBuilder& LicenseInfoCategory = DetailBuilder.EditCategory("LicenseInfo");

		// hide by default, as we will create a custom widget
		TSharedPtr<IPropertyHandle> LicenseInformationPropertyHandle = DetailBuilder.GetProperty(FName("LicenseInformation"), UInstaLODSettings::StaticClass());
		LicenseInformationPropertyHandle->MarkHiddenByCustomization();
		
		// create the custom widget
		LicenseInfoCategory.AddCustomRow(FText::FromString("License"))
		.WholeRowContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			[
				SNew(SEditableTextBox)
				.IsReadOnly(true)
				.Text(Settings->GetLicenseInfo())
				.ForegroundColor(FSlateColor::UseForeground())
			]
		];
	}
	
	/************************************************************************/
	/* SDK Version                                                          */
	/************************************************************************/
	{
		IDetailCategoryBuilder& SDKVersionCategory = DetailBuilder.EditCategory("SDKVersion");
		
		// hide by default, as we will create a custom widget
		TSharedPtr<IPropertyHandle> SDKVersionPropertyHandle = DetailBuilder.GetProperty(FName("SDKVersion"), UInstaLODSettings::StaticClass());
		SDKVersionPropertyHandle->MarkHiddenByCustomization();

		// create the custom widget
		SDKVersionCategory.AddCustomRow(FText::FromString("SDKVersion"))
			.WholeRowContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SBox)
				.HAlign(HAlign_Fill)
				[
					SNew(SEditableTextBox)
					.IsReadOnly(true)
					.Text(Settings->GetSDKVersion())
					.ForegroundColor(FSlateColor::UseForeground())
				]
			];
	}
	
	/************************************************************************/
	/* De-/Authorize                                                        */
	/************************************************************************/
	{
		// Buttons
		TSharedPtr<SVerticalBox> Buttons = SNew(SVerticalBox);

		FText CategoryName;
		FText WarningText;

		if (Settings->bAuthorized)
		{
			// hide the entire Authorize category
			DetailBuilder.HideCategory("Authorize");
			
			CategoryName = NSLOCTEXT("InstaLODUI", "SettingsCategoryName", "Deauthorize");
			WarningText = NSLOCTEXT("InstaLODUI", "SettingsWarning", "Deauthorization takes 24 hours to complete.\nThis node will remain locked until the deauthorization is finished.");

			for (TFieldIterator<UFunction> FuncIt(SettingsClass); FuncIt; ++FuncIt)
			{
				UFunction* Function = *FuncIt;

				if (Function->HasAnyFunctionFlags(FUNC_Exec) && (Function->NumParms == 0) && Function->GetMetaData(FName("Category")).Equals("Deauthorize"))
				{
					if (!bIsNodeLockedLicense)
					{
						const FString FunctionName = Function->GetMetaData(FName("DisplayName"));
						const FText ButtonCaption = FText::FromString(FunctionName);
						const FString DetailCategoryName = Function->GetMetaData(FName("Category"));

						IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(FName(*DetailCategoryName));

						Buttons->AddSlot()
						.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
						[
							SNew(SButton)
							.ButtonStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary")
							.ButtonColorAndOpacity(FLinearColor::FromSRGBColor(FColor(255, 47, 141)))
							.OnClicked(FOnClicked::CreateStatic(&FInstaLODSettingsCustomization::ExecuteTool, &DetailBuilder, Function))
							[
								SNew(STextBlock)
								.Text(ButtonCaption)
								.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary.BoldTextStyle")
								.Justification(ETextJustify::Center)
							]
						];
					}
				}
			}
		}
		else
		{
			// hide the entire Deauthorize category
			DetailBuilder.HideCategory("Deauthorize");

			CategoryName = NSLOCTEXT("InstaLODUI", "SettingsCategoryName", "Authorize");
			WarningText = NSLOCTEXT("InstaLODUI", "SettingsWarning",
									"InstaLOD requires a valid license. Please enter your licensing information in the fields below.\n"
									"In order to acquire a license an active internet connection is required.\n"
									"InstaLOD periodically connects to the InstaLOD servers to validate and refresh the license.");

			for (TFieldIterator<UFunction> FuncIt(SettingsClass); FuncIt; ++FuncIt)
			{
				UFunction* Function = *FuncIt;

				if (Function->HasAnyFunctionFlags(FUNC_Exec) && (Function->NumParms == 0) && Function->GetMetaData(FName("Category")).Equals("Authorize"))
				{
					const FString FunctionName = Function->GetMetaData(FName("DisplayName"));
					const FText ButtonCaption = FText::FromString(FunctionName);
					const FString DetailCategoryName = Function->GetMetaData(FName("Category"));

					IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(FName(*DetailCategoryName));

					Buttons->AddSlot()
					.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
					[
						SNew(SButton)
						.ButtonStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary")
						.ButtonColorAndOpacity(FLinearColor::FromSRGBColor(FColor(255, 47, 141)))
						.OnClicked(FOnClicked::CreateStatic(&FInstaLODSettingsCustomization::ExecuteTool, &DetailBuilder, Function))
						[
							SNew(STextBlock)
							.Text(ButtonCaption)
							.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary.BoldTextStyle")
							.Justification(ETextJustify::Center)
						]
					];
				}
			}
		}

		// hide the AccountName by default so we can customize it
		TSharedPtr<IPropertyHandle> AccountNamePropertyHandle = DetailBuilder.GetProperty(FName("AccountName"), UInstaLODSettings::StaticClass());
		AccountNamePropertyHandle->MarkHiddenByCustomization();

		// hide the SerialPassword by default so we can customize it
		TSharedPtr<IPropertyHandle> SerialPasswordPropertyHandle = DetailBuilder.GetProperty(FName("SerialPassword"), UInstaLODSettings::StaticClass());
		SerialPasswordPropertyHandle->MarkHiddenByCustomization();

		if (!bIsNodeLockedLicense)
		{
			IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(FName(*CategoryName.ToString()));

			// Add Custom Widget
			Category.AddCustomRow(CategoryName)
			.WholeRowContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(5.f, 5.f, 5.f, 20.f))
					[
						SNew(SBorder)
						.HAlign(HAlign_Fill)
						.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
						[
							// Info text
							SNew(STextBlock)
							.Text(WarningText)
							.AutoWrapText(true)
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FSlateColor(FLinearColor(FColor::Red)))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
					[
						// AccountName TextBox
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						[
							SNew(STextBlock)
							.Text(FText::FromString(AccountNamePropertyHandle->GetMetaData(FName("DisplayName"))))
						]
						+ SHorizontalBox::Slot()
						[
							SNew(SEditableTextBox)
							.ForegroundColor(FSlateColor(FLinearColor(FColor::Black)))
							.OnTextChanged(this, &FInstaLODSettingsCustomization::OnAccountNameChanged)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
					[
						// Password TextBox
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						[
							SNew(STextBlock)
							.Text(FText::FromString(SerialPasswordPropertyHandle->GetMetaData(FName("DisplayName"))))
						]
						+ SHorizontalBox::Slot()
						[
							SNew(SEditableTextBox)
							.ForegroundColor(FSlateColor(FLinearColor(FColor::Black)))
							.IsPassword(true)
							.OnTextChanged(this, &FInstaLODSettingsCustomization::OnPasswordChanged)
						]
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						Buttons->AsShared()
					]
				]
			];
		}
		else
		{
			WarningText = NSLOCTEXT("InstaLODUI", "NodeLockedWarning", "This workstation has been authorized with a node-locked license. If you want to deauthorize this workstation, please contact your account manager at InstaLOD.");

			IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(FName(*CategoryName.ToString()));

			// Add Custom Widget
			Category.AddCustomRow(CategoryName)
			.WholeRowContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(5.f, 5.f, 5.f, 20.f))
					[
						SNew(SBorder)
						.HAlign(HAlign_Fill)
						.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
						[
							// Info text
							SNew(STextBlock)
							.Text(WarningText)
							.AutoWrapText(true)
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FSlateColor(FLinearColor(FColor::Red)))
						]
					]
				]
			];
		}
	}
	
	/************************************************************************/
	/* Buttons                                                              */
	/************************************************************************/
	{
		DetailBuilder.HideCategory(FName("Utility"));

		// create a button for each UFunction
		for (TFieldIterator<UFunction> FuncIt(SettingsClass); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;

			// filter De-/Authorize functions, as we've already process them
			if (Function->HasAnyFunctionFlags(FUNC_Exec) && (Function->NumParms == 0) && !Function->GetMetaData(FName("Category")).Equals("Deauthorize") && !Function->GetMetaData(FName("Category")).Equals("Authorize"))
			{
				const FString FunctionName = Function->GetMetaData(FName("DisplayName"));
				const FText ButtonCaption = FText::FromString(FunctionName);
				const FString DetailCategoryName = Function->GetMetaData(FName("Category"));

				IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(FName(*DetailCategoryName));

				// create widgets
				CategoryBuilder.AddCustomRow(FText::FromString(DetailCategoryName))
					.WholeRowContent()
					[
						SNew(SBox)
						.HAlign(HAlign_Fill)
						.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
						[
							SNew(SButton)
							.ButtonStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary")
							.ButtonColorAndOpacity(FLinearColor::FromSRGBColor(FColor(25, 25, 25)))
							.OnClicked(FOnClicked::CreateStatic(&FInstaLODSettingsCustomization::ExecuteTool, &DetailBuilder, Function))
							[
								SNew(STextBlock)
								.Text(ButtonCaption)
								.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary.BoldTextStyle")
								.Justification(ETextJustify::Center)
							]
						]	
					];
			}
		}
	}
}

void FInstaLODSettingsCustomization::OnForceRefreshDetails()
{
	if (DetailLayoutBuilder)
	{
		DetailLayoutBuilder->ForceRefreshDetails();
	}
}

FReply FInstaLODSettingsCustomization::ExecuteTool(IDetailLayoutBuilder* DetailBuilder, UFunction* MethodToExecute)
{
	// get the edited tool and call the passed UFunction of it
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	DetailBuilder->GetObjectsBeingCustomized(ObjectsBeingEdited);

	if (ObjectsBeingEdited.Num() == 1)
	{
		if (UObject* Instance = ObjectsBeingEdited[0].Get())
		{
			Instance->CallFunctionByNameWithArguments(*MethodToExecute->GetName(), *GLog, nullptr, true);
		}
	}

	return FReply::Handled();
}

void FInstaLODSettingsCustomization::OnAccountNameChanged(const FText& NewText)
{
	if (Settings)
	{
		Settings->AccountName = NewText;
	}
}

void FInstaLODSettingsCustomization::OnPasswordChanged(const FText& NewText)
{
	if (Settings)
	{
		Settings->SerialPassword = NewText;
	}
}

#undef LOCTEXT_NAMESPACE
