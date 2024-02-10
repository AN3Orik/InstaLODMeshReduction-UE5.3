/**
 * InstaLODBaseToolCustomization.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODBaseToolCustomization.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODBaseToolCustomization.h"
#include "InstaLODUIPCH.h"
#include "Slate/InstaLODPluginStyle.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"

#include "Slate/InstaLODWindow.h"

#include "Tools/InstaLODOptimizeTool.h"
#include "Tools/InstaLODRemeshTool.h"
#include "Tools/InstaLODImposterizeTool.h"
#include "Tools/InstaLODMaterialMergeTool.h"
#include "Tools/InstaLODOcclusionCullTool.h"
#include "Tools/InstaLODSettings.h"

FInstaLODBaseToolCustomization::FInstaLODBaseToolCustomization()
	: DetailLayoutBuilder(nullptr)
{
}

FInstaLODBaseToolCustomization::~FInstaLODBaseToolCustomization()
{
	DetailLayoutBuilder = nullptr;
}

void FInstaLODBaseToolCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// save the DetailBuilder, so we can use it later
	DetailLayoutBuilder = &DetailBuilder;

	// retrieve the ToolClass from the currently edited Tool (Object)
	UClass* ToolClass = nullptr;
	UObject* Instance = nullptr;
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingEdited);

	// we only await one object to be selected at a time
	if (ObjectsBeingEdited.Num() == 1)
	{
		Instance = ObjectsBeingEdited[0].Get();
		if (Instance)
		{
			ToolInstance = Cast<UInstaLODBaseTool>(Instance);
			ToolClass = ToolInstance->GetClass();
		}
	}

	/************************************************************************/
	/* All Operations                                                       */
	/************************************************************************/

	// create a new Expand Selection Category
	UInstaLODBaseTool* const CurrentTool = Cast<UInstaLODBaseTool>(ToolInstance);

	IDetailCategoryBuilder& ExpandSelectionCategoryBuilder = DetailBuilder.EditCategory("Expand Selection", FText::GetEmpty(), ECategoryPriority::Variable);
	TSharedPtr<IPropertyHandle> ExpandSelectionPropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODBaseTool, ExpandSelectionRadius));
	ExpandSelectionPropertyHandle->MarkHiddenByCustomization();

	const bool bIsMeshOperation = Cast<UInstaLODSettings>(ToolInstance) == nullptr;
	ExpandSelectionCategoryBuilder.SetCategoryVisibility(bIsMeshOperation);

	if (bIsMeshOperation)
	{
		ExpandSelectionCategoryBuilder.AddCustomRow(FText::FromString("Expand Selection"))
			.NameContent()
			[
				SNew(SBox)
				.HAlign(HAlign_Fill)
				[
					ExpandSelectionPropertyHandle->CreatePropertyNameWidget()
				]
			]
			.ValueContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 2.0f, 0.0f)
				[
					ExpandSelectionPropertyHandle->CreatePropertyValueWidget()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.VAlign(EVerticalAlignment::VAlign_Center)
					.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary.DefaultTextStyle")
					.OnClicked_Lambda([CurrentTool]() -> FReply
					{
						if ((CurrentTool != nullptr) && (CurrentTool->GetInstaLODWindow() != nullptr) && (CurrentTool->GetClass() != UInstaLODSettings::StaticClass()))
							return CurrentTool->GetInstaLODWindow()->OnExpandSelection(CurrentTool->ExpandSelectionRadius);

						return FReply::Handled();
					})
					.IsEnabled_Lambda([CurrentTool]() -> bool
					{
						if ((CurrentTool != nullptr) && (CurrentTool->GetInstaLODWindow() != nullptr) && (CurrentTool->GetClass() != UInstaLODSettings::StaticClass()))
							return CurrentTool->GetInstaLODWindow()->GetEnabledSelectedMeshComponents().Num() > 0;

						return false;
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString("Expand Selection"))
						.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary.BoldTextStyle")
						.Justification(ETextJustify::Center)
					]
				]
			];
	}
	ExpandSelectionCategoryBuilder.InitiallyCollapsed(true);

	/************************************************************************/
	/* All Tools                                                            */
	/************************************************************************/

	// create a new Utilities Category
	IDetailCategoryBuilder& UtilitiesCategory = DetailBuilder.EditCategory("Utilities", FText::GetEmpty(), ECategoryPriority::Uncommon);

	// hide the following properties for customization and then add them back with a EditCondition
	{
		TSharedPtr<IPropertyHandle> ResultUsagePropertyHandle = DetailBuilder.GetProperty(FName("ResultUsage"), UInstaLODBaseTool::StaticClass());
		ResultUsagePropertyHandle->MarkHiddenByCustomization();
		UtilitiesCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODBaseTool, ResultUsage));
	}
	{
		TSharedPtr<IPropertyHandle> TargetLODIndexPropertyHandle = DetailBuilder.GetProperty(FName("TargetLODIndex"), UInstaLODBaseTool::StaticClass());
		TargetLODIndexPropertyHandle->MarkHiddenByCustomization();
		UtilitiesCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODBaseTool, TargetLODIndex))
			.EditCondition(TAttribute<bool>(this, &FInstaLODBaseToolCustomization::CanTargetLODIndex), nullptr);
	}
	{
		TSharedPtr<IPropertyHandle> ReplaceSelectedMeshesPropertyHandle = DetailBuilder.GetProperty(FName("bReplaceSelectedMeshes"), UInstaLODBaseTool::StaticClass());
		ReplaceSelectedMeshesPropertyHandle->MarkHiddenByCustomization();
		UtilitiesCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODBaseTool, bReplaceSelectedMeshes))
			.EditCondition(TAttribute<bool>(this, &FInstaLODBaseToolCustomization::CanReplaceSelectedMeshes), nullptr);
	}
	{
		TSharedPtr<IPropertyHandle> PivotPositionHandle = DetailBuilder.GetProperty(FName("PivotPosition"), UInstaLODBaseTool::StaticClass());
		PivotPositionHandle->MarkHiddenByCustomization();
		UtilitiesCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODBaseTool, PivotPosition))
			.EditCondition(TAttribute<bool>(this, &FInstaLODBaseToolCustomization::CanPivotPosition), nullptr);
	}
	{
		TSharedPtr<IPropertyHandle> DesiredPivotPositionHandle = DetailBuilder.GetProperty(FName("BoundingBoxPivotPosition"), UInstaLODBaseTool::StaticClass());
		DesiredPivotPositionHandle->MarkHiddenByCustomization();
		UtilitiesCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODBaseTool, BoundingBoxPivotPosition))
			.EditCondition(TAttribute<bool>(this, &FInstaLODBaseToolCustomization::CanBoundingBoxPivotPosition), nullptr);
	}
	{
		TSharedPtr<IPropertyHandle> DesiredPivotPositionHandle = DetailBuilder.GetProperty(FName("WorldSpacePivotPosition"), UInstaLODBaseTool::StaticClass());
		DesiredPivotPositionHandle->MarkHiddenByCustomization();
		UtilitiesCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODBaseTool, WorldSpacePivotPosition))
			.EditCondition(TAttribute<bool>(this, &FInstaLODBaseToolCustomization::CanWorldSpacePivotPosition), nullptr);
	}

	/************************************************************************/
	/* Optimize Tool                                                        */
	/************************************************************************/

	OptimizeTool = Cast<UInstaLODOptimizeTool>(ToolInstance);

	if (OptimizeTool)
	{	
		IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory("Settings", FText::GetEmpty(), ECategoryPriority::Important);

		int32 CheckBoxIndex = 0;

		for (TFieldIterator<FProperty> PropIt(ToolClass); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;

			// check for the custom "RadioButton" meta that we added to some of the properties
			if (Property->HasMetaData(FName("RadioButton")))
			{
				FName PropertyName = Property->GetFName();

				TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(PropertyName, UInstaLODOptimizeTool::StaticClass());
				PropertyHandle->MarkHiddenByCustomization();

				// add a new custom row for the Property that has a checkbox to enable it
				// these checkboxes only allow one to be selected at a time
				SettingsCategory.AddCustomRow(FText::FromString("Settings"))
				.Visibility(TAttribute<EVisibility>(this, &FInstaLODBaseToolCustomization::IsOptimizeToolSettingsRadioButtonVisible))
				.NameContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						// pass the Checkbox index so we know what Checkbox got clicked
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &FInstaLODBaseToolCustomization::OptimizeToolSettingsCheckBoxClicked, CheckBoxIndex)
						.IsChecked(this, &FInstaLODBaseToolCustomization::IsOptimizeToolSettingsCheckBoxSelected, CheckBoxIndex)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.IsEnabled(this, &FInstaLODBaseToolCustomization::IsOptimizeToolSettingEnabled, CheckBoxIndex)
						[
							PropertyHandle->CreatePropertyNameWidget()
						]
					]
				]
				.ValueContent()
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					.IsEnabled(this, &FInstaLODBaseToolCustomization::IsOptimizeToolSettingEnabled, CheckBoxIndex)
					[
						PropertyHandle->CreatePropertyValueWidget()
					]
				];
				CheckBoxIndex++;
			}
		}

		// add the non hidden property back to the category
		SettingsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOptimizeTool, MaximumDeviation));
	}

	/************************************************************************/
	/* Remesh Tool                                                          */
	/************************************************************************/

	RemeshTool = Cast<UInstaLODRemeshTool>(ToolInstance);

	if (RemeshTool)
	{
		// setup a delegate that is called when the type property changes
		if (!RefreshDetailsDelegate.IsBound())
		{
			RefreshDetailsDelegate = FSimpleDelegate::CreateSP(this, &FInstaLODBaseToolCustomization::OnForceRefreshDetails);
		}
		
		IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory("Settings", FText::GetEmpty(), ECategoryPriority::Important);

		// add the non hidden property back to the category
		SettingsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODRemeshTool, RemeshMode), UInstaLODRemeshTool::StaticClass(), NAME_None, EPropertyLocation::Common);

		// iterate over all Properties of the Remesh class and find the one with "RadioButton" meta
		// then create a custom widget based on the CheckBoxIndex, which is saved inside of the Tool to actually "save" the state
		int32 CheckBoxIndex = 0;
		for (TFieldIterator<FProperty> PropIt(ToolClass); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;

			if (Property->HasMetaData(FName("RadioButton")))
			{
				FName PropertyName = Property->GetFName();

				TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(PropertyName, UInstaLODRemeshTool::StaticClass());
				PropertyHandle->MarkHiddenByCustomization();

				SettingsCategory.AddCustomRow(FText::FromString("Settings"))
				.Visibility(TAttribute<EVisibility>(this, &FInstaLODBaseToolCustomization::IsRemeshToolSettingsRadioButtonVisible))
				.NameContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &FInstaLODBaseToolCustomization::RemeshToolSettingsCheckBoxClicked, CheckBoxIndex)
						.IsChecked(this, &FInstaLODBaseToolCustomization::IsRemeshToolSettingsCheckBoxSelected, CheckBoxIndex)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.IsEnabled(this, &FInstaLODBaseToolCustomization::IsRemeshToolSettingEnabled, CheckBoxIndex)
						[
							PropertyHandle->CreatePropertyNameWidget()
						]
					]
				]
				.ValueContent()
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					.IsEnabled(this, &FInstaLODBaseToolCustomization::IsRemeshToolSettingEnabled, CheckBoxIndex)
					[
						PropertyHandle->CreatePropertyValueWidget()
					]
				];

				CheckBoxIndex++;
			}
		}

		// hide the two properties that should only be visible when "ScreenSizeInPixels" is used
		TSharedPtr<IPropertyHandle> PixelMergeDistancePropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODRemeshTool, PixelMergeDistance), UInstaLODRemeshTool::StaticClass());
		PixelMergeDistancePropertyHandle->MarkHiddenByCustomization();
		TSharedPtr<IPropertyHandle> AutomaticTextureSizePropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODRemeshTool, bAutomaticTextureSize), UInstaLODRemeshTool::StaticClass());
		AutomaticTextureSizePropertyHandle->MarkHiddenByCustomization();

		
		// retrieve the type property from the tool Class and set the delegate for value change of it
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODRemeshTool, RemeshMode), UInstaLODRemeshTool::StaticClass())->SetOnPropertyValueChanged(RefreshDetailsDelegate);
		
		// hide the reconstruct specific settings
		if (RemeshTool->RemeshMode != EInstaLODRemeshMode::InstaLOD_Reconstruct)
		{
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODRemeshTool, RemeshResolution), UInstaLODRemeshTool::StaticClass())->MarkHiddenByCustomization();
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODRemeshTool, bIgnoreBackfaces), UInstaLODRemeshTool::StaticClass())->MarkHiddenByCustomization();
		}

		if (RemeshTool->RemeshMode != EInstaLODRemeshMode::InstaLOD_Optimize)
		{
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODRemeshTool, bLockBoundaries), UInstaLODRemeshTool::StaticClass())->MarkHiddenByCustomization();
		}
		
		// FIXME: Index 2 is ScreenSizeInPixels (use enum)
		if (RemeshTool->GetActiveSettingsIndex() == 2)
		{
			SettingsCategory.AddProperty(PixelMergeDistancePropertyHandle);
			SettingsCategory.AddProperty(AutomaticTextureSizePropertyHandle);
		}
	}

	UInstaLODMaterialMergeTool* MaterialMergeTool = Cast<UInstaLODMaterialMergeTool>(ToolInstance);

	if (MaterialMergeTool)
	{
		// setup a delegate that is called when the type property changes
		if (!RefreshDetailsDelegate.IsBound())
		{
			RefreshDetailsDelegate = FSimpleDelegate::CreateSP(this, &FInstaLODBaseToolCustomization::OnForceRefreshDetails);
		}

		// retrieve the type property from the tool class and set the delegate for value change of it
		TSharedPtr<IPropertyHandle> MaterialMergeTypeProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODMaterialMergeTool, bGenerateZeroAreaUV), UInstaLODMaterialMergeTool::StaticClass());
		MaterialMergeTypeProperty->SetOnPropertyValueChanged(RefreshDetailsDelegate);

		if (MaterialMergeTool->bGenerateZeroAreaUV == false)
		{
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODMaterialMergeTool, ZeroAreaUVThreshold), UInstaLODMaterialMergeTool::StaticClass())->MarkHiddenByCustomization();
		}
	}

	/************************************************************************/
	/* Imposterize Tool                                                     */
	/************************************************************************/

	UInstaLODImposterizeTool* ImposterizeTool = Cast<UInstaLODImposterizeTool>(ToolInstance);

	if (ImposterizeTool)
	{
		// setup a delegate that is called when the type property changes
		if (!RefreshDetailsDelegate.IsBound())
		{
			RefreshDetailsDelegate = FSimpleDelegate::CreateSP(this, &FInstaLODBaseToolCustomization::OnForceRefreshDetails);
		}

		// retrieve the type property from the tool class and set the delegate for value change of it
		TSharedPtr<IPropertyHandle> ImposterizeTypeProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODImposterizeTool, ImposterizeType), UInstaLODImposterizeTool::StaticClass());
		ImposterizeTypeProperty->SetOnPropertyValueChanged(RefreshDetailsDelegate);

		// show and hide the categories based on imposterize Type
		DetailBuilder.EditCategory("Billboard Settings", FText::GetEmpty(), ECategoryPriority::Default).SetCategoryVisibility(ImposterizeTool->GetType() == EInstaLODImposterizeType::InstaLOD_Billboard);
		DetailBuilder.EditCategory("Flipbook Settings", FText::GetEmpty(), ECategoryPriority::Default).SetCategoryVisibility(ImposterizeTool->GetType() == EInstaLODImposterizeType::InstaLOD_Flipbook);
		DetailBuilder.EditCategory("AABB Settings", FText::GetEmpty(), ECategoryPriority::Default).SetCategoryVisibility(ImposterizeTool->GetType() == EInstaLODImposterizeType::InstaLOD_AABB);
		DetailBuilder.EditCategory("Vista Settings", FText::GetEmpty(), ECategoryPriority::Default).SetCategoryVisibility(ImposterizeTool->GetType() == EInstaLODImposterizeType::InstaLOD_Vista);
		DetailBuilder.EditCategory("Hybrid Billboard Cloud Settings", FText::GetEmpty(), ECategoryPriority::Default).SetCategoryVisibility(ImposterizeTool->GetType() == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud);
		DetailBuilder.EditCategory("Alpha Cutout (Preview)", FText::GetEmpty(), ECategoryPriority::Default).SetCategoryVisibility(ImposterizeTool->GetType() != EInstaLODImposterizeType::InstaLOD_Flipbook);
		DetailBuilder.EditCategory("Advanced", FText::GetEmpty(), ECategoryPriority::Default);
	}

	/************************************************************************/
	/* Occlusion Cull Tool                                                  */
	/************************************************************************/

	UInstaLODOcclusionCullTool* OcclusionCullTool = Cast<UInstaLODOcclusionCullTool>(ToolInstance);

	if (OcclusionCullTool)
	{
		// setup a delegate that is called when the type property changes
		if (!RefreshDetailsDelegate.IsBound())
		{
			RefreshDetailsDelegate = FSimpleDelegate::CreateSP(this, &FInstaLODBaseToolCustomization::OnForceRefreshDetails);
		}

		// handle to the settings category
		IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory("Settings", FText::GetEmpty(), ECategoryPriority::Default);

		// retrieve the mode property from the tool class and set the delegate for value change of it
		TSharedPtr<IPropertyHandle> OcclusionCullModeProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOcclusionCullTool, OcclusionCullMode), UInstaLODOcclusionCullTool::StaticClass());
		OcclusionCullModeProperty->SetOnPropertyValueChanged(RefreshDetailsDelegate);
		// also add it to the to DetailsView manually to keep order
		SettingsCategory.AddProperty(OcclusionCullModeProperty);

		// keep order of properties by adding the normal ones first
		SettingsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOcclusionCullTool, CullingStrategy), UInstaLODOcclusionCullTool::StaticClass());
		SettingsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOcclusionCullTool, DataUsage), UInstaLODOcclusionCullTool::StaticClass());
		SettingsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOcclusionCullTool, Resolution), UInstaLODOcclusionCullTool::StaticClass());

		// hide the precision property by default
		TSharedPtr<IPropertyHandle> PrecisionPropertyHandle = DetailBuilder.GetProperty(FName("Precision"), UInstaLODOcclusionCullTool::StaticClass());
		PrecisionPropertyHandle->MarkHiddenByCustomization();

		// ... show it when we're in automatic interior
		if (OcclusionCullTool->GetMode() == EInstaLODOcclusionCullMode::InstaLOD_AutomaticInterior)
		{
			SettingsCategory.AddProperty(PrecisionPropertyHandle);
		}

		// to maintain the order of the properties, we just read these two after the Precision property
		SettingsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOcclusionCullTool, AdjacencyDepth), UInstaLODOcclusionCullTool::StaticClass());
		
#if defined(INSTALOD_OCCLUSIONCULL_ALPHAMASK_THRESHOLD)
		SettingsCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOcclusionCullTool, AlphaMaskThreshold), UInstaLODOcclusionCullTool::StaticClass());
#endif
		DetailBuilder.EditCategory("Advanced", FText::GetEmpty(), ECategoryPriority::Default);
	}

}

bool FInstaLODBaseToolCustomization::ToolButtonEnabled() const
{
	if (ToolInstance && ToolInstance->GetInstaLODWindow())
	{
		return ToolInstance->GetInstaLODWindow()->GetEnabledSelectedMeshComponents().Num() > 0;
	}

	return false;
}

bool FInstaLODBaseToolCustomization::CanTargetLODIndex() const
{
	if (ToolInstance)
	{
		return ToolInstance->ResultUsage == EInstaLODResultUsage::InstaLOD_ReplaceLOD;
	}
	
	return false;
}

bool FInstaLODBaseToolCustomization::CanReplaceSelectedMeshes() const
{
	if (ToolInstance)
	{
		return ToolInstance->ResultUsage == EInstaLODResultUsage::InstaLOD_NewAsset;
	}

	return false;
}

bool FInstaLODBaseToolCustomization::CanPivotPosition() const
{
	if (ToolInstance && ToolInstance->GetInstaLODWindow())
	{
		return ToolInstance->ResultUsage == EInstaLODResultUsage::InstaLOD_NewAsset && ToolInstance->IsFreezingTransformsForMultiSelection() && ToolInstance->GetInstaLODWindow()->GetEnabledSelectedMeshComponents().Num() > 1;
	}

	return false;
}

bool FInstaLODBaseToolCustomization::CanBoundingBoxPivotPosition() const
{
	if (ToolInstance)
	{
		return FInstaLODBaseToolCustomization::CanPivotPosition() && ToolInstance->PivotPosition == EInstaLODPivotPosition::InstaLOD_CustomLimited;
	}

	return false;
}

bool FInstaLODBaseToolCustomization::CanWorldSpacePivotPosition() const
{
	if (ToolInstance)
	{
		return FInstaLODBaseToolCustomization::CanPivotPosition() && ToolInstance->PivotPosition == EInstaLODPivotPosition::InstaLOD_Custom;
	}

	return false;
}

EVisibility FInstaLODBaseToolCustomization::IsOptimizeToolSettingsRadioButtonVisible() const
{
	return OptimizeTool->AutomaticQuality == EInstaLODImportance::InstaLOD_OFF ? EVisibility::Visible : EVisibility::Hidden;
}

void FInstaLODBaseToolCustomization::OptimizeToolSettingsCheckBoxClicked(const ECheckBoxState NewCheckedState, int32 CheckBoxIndex)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		OptimizeTool->SetActiveSettingsIndex(CheckBoxIndex);
	}
}

ECheckBoxState FInstaLODBaseToolCustomization::IsOptimizeToolSettingsCheckBoxSelected(int32 CheckBoxIndex) const
{
	return (OptimizeTool->GetActiveSettingsIndex() == CheckBoxIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool FInstaLODBaseToolCustomization::IsOptimizeToolSettingEnabled(int32 CheckBoxIndex) const
{
	return (OptimizeTool->GetActiveSettingsIndex() == CheckBoxIndex);
}

EVisibility FInstaLODBaseToolCustomization::IsRemeshToolSettingsRadioButtonVisible() const
{
	return RemeshTool->RemeshMode != EInstaLODRemeshMode::InstaLOD_UV ? EVisibility::Visible : EVisibility::Hidden;
}

void FInstaLODBaseToolCustomization::RemeshToolSettingsCheckBoxClicked(const ECheckBoxState NewCheckedState, int32 CheckBoxIndex)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		RemeshTool->SetActiveSettingsIndex(CheckBoxIndex);
		OnForceRefreshDetails();
	}
}

ECheckBoxState FInstaLODBaseToolCustomization::IsRemeshToolSettingsCheckBoxSelected(int32 CheckBoxIndex) const
{
	return (RemeshTool->GetActiveSettingsIndex() == CheckBoxIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool FInstaLODBaseToolCustomization::IsRemeshToolSettingEnabled(int32 CheckBoxIndex) const
{
	return (RemeshTool->GetActiveSettingsIndex() == CheckBoxIndex);
}

FReply FInstaLODBaseToolCustomization::ExecuteTool(IDetailLayoutBuilder* DetailBuilder, UFunction* MethodToExecute)
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

void FInstaLODBaseToolCustomization::OnForceRefreshDetails()
{
	if (DetailLayoutBuilder)
	{
		DetailLayoutBuilder->ForceRefreshDetails();
	}
}
