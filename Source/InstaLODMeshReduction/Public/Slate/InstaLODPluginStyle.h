/**
 * InstaLODPluginStyle.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODPluginStyle.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODPluginStyle_h
#define InstaLOD_InstaLODPluginStyle_h

#include "CoreMinimal.h"
#include "SlateFwd.h"

#include "Styling/SlateStyle.h"

/**  */
class INSTALODMESHREDUCTION_API FInstaLODPluginStyle
{
public:
	
	static void Initialize();
	
	static void Shutdown();
	
	/** reloads textures used by slate renderer */
	static void ReloadTextures();
	
	/** @return The Slate style set */
	static const ISlateStyle& Get();
	
	static FName GetStyleSetName();
	
private:
	
	static TSharedRef< class FSlateStyleSet > Create();
	
private:
	
	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};


#endif
