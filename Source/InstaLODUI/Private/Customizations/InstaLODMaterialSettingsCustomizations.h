/**
 * InstaLODMaterialSettingsCustomizations.h (InstaLOD)
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

#ifndef InstaLOD_InstaLODMaterialSettingsCustomizations_h
#define InstaLOD_InstaLODMaterialSettingsCustomizations_h


#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"

class FDetailWidgetRow;
class IPropertyHandle;

class FInstaLODMaterialSettingsCustomizations : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	
protected:
	void AddTextureSizeClamping(TSharedPtr<IPropertyHandle> TextureSizeProperty);
	
	EVisibility AreManualOverrideTextureSizesEnabled() const;
	EVisibility IsTextureSizeEnabled() const;
	
	TArray<TSharedPtr<IPropertyHandle>> PropertyTextureSizeHandles;
	TSharedPtr< IPropertyHandle > EnumHandle;
	TSharedPtr< IPropertyHandle > TextureSizeHandle;
};

#endif
