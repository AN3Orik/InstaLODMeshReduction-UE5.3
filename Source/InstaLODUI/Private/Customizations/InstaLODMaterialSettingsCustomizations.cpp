/**
 * InstaLODMaterialSettingsCustomizations.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMaterialSettingsCustomizations.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODMaterialSettingsCustomizations.h"
#include "InstaLODUIPCH.h"
#include "Tools/InstaLODBakeBaseTool.h"

#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

TSharedRef<IPropertyTypeCustomization> FInstaLODMaterialSettingsCustomizations::MakeInstance()
{
	return MakeShareable(new FInstaLODMaterialSettingsCustomizations);
}

void FInstaLODMaterialSettingsCustomizations::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow.NameContent() [ StructPropertyHandle->CreatePropertyNameWidget() ]
	.ValueContent() [ StructPropertyHandle->CreatePropertyValueWidget() ];
}

void FInstaLODMaterialSettingsCustomizations::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// retrieve structure's child properties
	uint32 ChildPropertyCount;
	StructPropertyHandle->GetNumChildren(ChildPropertyCount);
	TMap<FName, TSharedPtr<IPropertyHandle>> PropertyHandles;
	
	for(uint32 ChildIndex=0; ChildIndex<ChildPropertyCount; ChildIndex++)
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		PropertyHandles.Add(ChildHandle->GetProperty()->GetFName(), ChildHandle);
	}

	// retrieve special case properties
	EnumHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, TextureSizingType));
	TextureSizeHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, TextureSize));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, DiffuseTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, NormalTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, MetallicTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, RoughnessTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, SpecularTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, EmissiveTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, OpacityTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, OpacityMaskTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FInstaLODMaterialSettings, AmbientOcclusionTextureSize)));
	
	auto Parent = StructPropertyHandle->GetParentHandle();
	
	for(auto Iter(PropertyHandles.CreateIterator()); Iter; ++Iter)
	{
		// handle special property cases (done inside the loop to maintain order according to the struct
		if (PropertyTextureSizeHandles.Contains(Iter.Value()))
		{
			IDetailPropertyRow& SizeRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
			SizeRow.Visibility(TAttribute<EVisibility>(this, &FInstaLODMaterialSettingsCustomizations::AreManualOverrideTextureSizesEnabled));
			AddTextureSizeClamping(Iter.Value());
		}
		else if (Iter.Value() == TextureSizeHandle)
		{
			IDetailPropertyRow& SettingsRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
			SettingsRow.Visibility(TAttribute<EVisibility>(this, &FInstaLODMaterialSettingsCustomizations::IsTextureSizeEnabled));
			AddTextureSizeClamping(Iter.Value());
		}
		else 
		{
			IDetailPropertyRow& SettingsRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
		}
	}
}

void FInstaLODMaterialSettingsCustomizations::AddTextureSizeClamping(TSharedPtr<IPropertyHandle> TextureSizeProperty)
{
	TSharedPtr<IPropertyHandle> PropertyX = TextureSizeProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FIntPoint, X));
	TSharedPtr<IPropertyHandle> PropertyY = TextureSizeProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FIntPoint, Y));
	
	const FString MaxTextureResolutionString = FString::FromInt(GetMax2DTextureDimension());
	TextureSizeProperty->GetProperty()->SetMetaData(TEXT("ClampMax"), *MaxTextureResolutionString);
	TextureSizeProperty->GetProperty()->SetMetaData(TEXT("UIMax"), *MaxTextureResolutionString);
	PropertyX->GetProperty()->SetMetaData(TEXT("ClampMax"), *MaxTextureResolutionString);
	PropertyX->GetProperty()->SetMetaData(TEXT("UIMax"), *MaxTextureResolutionString);
	PropertyY->GetProperty()->SetMetaData(TEXT("ClampMax"), *MaxTextureResolutionString);
	PropertyY->GetProperty()->SetMetaData(TEXT("UIMax"), *MaxTextureResolutionString);
	
	const FString MinTextureResolutionString("32");
	PropertyX->GetProperty()->SetMetaData(TEXT("ClampMin"), *MinTextureResolutionString);
	PropertyX->GetProperty()->SetMetaData(TEXT("UIMin"), *MinTextureResolutionString);
	PropertyY->GetProperty()->SetMetaData(TEXT("ClampMin"), *MinTextureResolutionString);
	PropertyY->GetProperty()->SetMetaData(TEXT("UIMin"), *MinTextureResolutionString);
}

EVisibility FInstaLODMaterialSettingsCustomizations::IsTextureSizeEnabled() const
{
	uint8 TypeValue;
	EnumHandle->GetValue(TypeValue);
	
	if (TypeValue == (uint8)EInstaLODTextureSizingType::InstaLOD_UseManualOverrideTextureSize)
	{
		return EVisibility::Hidden;
	}
	
	return EVisibility::Visible;
}

EVisibility FInstaLODMaterialSettingsCustomizations::AreManualOverrideTextureSizesEnabled() const
{
	uint8 TypeValue;
	EnumHandle->GetValue(TypeValue);
	
	if (TypeValue == (uint8)EInstaLODTextureSizingType::InstaLOD_UseManualOverrideTextureSize)
	{
		return EVisibility::Visible;
	}
	
	return EVisibility::Hidden;
}

#undef LOCTEXT_NAMESPACE
