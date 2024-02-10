/**
 * InstaLODPluginStyle.cpp (InstaLOD)
 *
 * Copyright 2016-2018 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODPluginStyle.cpp
 * @copyright 2016-2018 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODMeshReductionPCH.h"
#include "Slate/InstaLODPluginStyle.h"

#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Runtime/Launch/Resources/Version.h"

TSharedPtr< FSlateStyleSet > FInstaLODPluginStyle::StyleInstance = nullptr;

void FInstaLODPluginStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FInstaLODPluginStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FInstaLODPluginStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("InstaLODPluginStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

TSharedRef<FSlateStyleSet> FInstaLODPluginStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("InstaLODPluginStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("InstaLODMeshReduction")->GetBaseDir() / TEXT("Resources"));

	const FVector2D Icon18x18(18.0f, 18.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	const FVector2D MainComboBoxIcon16x25(16.0f, 25.0f);

	const FSlateFontInfo MainComboboxFont = TTF_FONT("Fonts/Roboto-Bold", 18);
	const FSlateFontInfo BoldFont = TTF_FONT("Fonts/Roboto-Bold", 10 );
	const FSlateFontInfo NormalFont = TTF_FONT("Fonts/Roboto-Regular", 10 );
	
	Style->Set("InstaLODMeshReduction.PluginAction", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon40x40));
	Style->Set("InstaLODUI.OpenInstaLODWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon40x40));
	Style->Set("InstaLODUI.OpenInstaLODWindow.Small", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon20x20));
	Style->Set("InstaLODUI.OpenInstaLODWindowFromStaticMeshEditor", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon40x40));
	Style->Set("InstaLODUI.OpenInstaLODWindowFromStaticMeshEditor.Small", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon20x20));
	Style->Set("InstaLODUI.OpenInstaLODWindowFromSkeletalMeshEditor", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon40x40));
	Style->Set("InstaLODUI.OpenInstaLODWindowFromSkeletalMeshEditor.Small", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon20x20));
	Style->Set("InstaLODUI.TabIcon", new IMAGE_BRUSH(TEXT("ButtonIcon_40x40"), Icon18x18));
	Style->Set( "InstaLODUI.MainComboBoxDownArrowIcon", new IMAGE_BRUSH( "MainComboBoxButton", MainComboBoxIcon16x25 ));
	
	
	{
		Style->Set("InstaLODUI.MainComboboxButton", FButtonStyle()
			.SetNormal(BOX_BRUSH("FlatButton", 2.0f / 8.0f, FLinearColor(0.01f, 0.01f, 0.01f, 1)))
			.SetHovered(BOX_BRUSH("FlatButton", 2.0f / 8.0f, FLinearColor(0.01f, 0.01f, 0.01f, 1)))
			.SetPressed(BOX_BRUSH("FlatButton", 2.0f / 8.0f, FLinearColor(0.01f, 0.01f, 0.01f, 1)))
			.SetNormalPadding(FMargin(2, 2, 2, 2))
			.SetPressedPadding(FMargin(2, 3, 2, 1))
		);

		Style->Set("InstaLODUI.MainComboboxButton.DefaultTextStyle", FTextBlockStyle()
			.SetFont(MainComboboxFont)
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f))
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Black)
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightShape(BOX_BRUSH("TextBlockHighlightShape", FMargin(3.0f / 8.0f)))
		);

		Style->Set("InstaLODUI.OperationInformation", FTextBlockStyle()
			.SetFont(NormalFont)
			.SetColorAndOpacity(FSlateColor::UseForeground())
		);

		Style->Set("InstaLODUI.ButtonPrimary", FButtonStyle()
				   .SetNormal(BOX_BRUSH("FlatButton", 2.0f / 8.0f, FLinearColor(0.72267, 0.72267, 0.72267, 1)))
				   .SetHovered(BOX_BRUSH("FlatButton", 2.0f / 8.0f, FLinearColor(0.85, 0.85, 0.85, 1)))
				   .SetPressed(BOX_BRUSH("FlatButton", 2.0f / 8.0f, FLinearColor(0.58597, 0.58597, 0.58597, 1)))
				   .SetNormalPadding(FMargin( 2,2,2,2 ))
				   .SetPressedPadding(FMargin( 2,3,2,1 ))
			);
		
		Style->Set("InstaLODUI.ButtonPrimary.BoldTextStyle", FTextBlockStyle()
				   .SetFont(BoldFont)
				   .SetColorAndOpacity(FSlateColor::UseForeground())
				   .SetShadowOffset(FVector2D(1, 1))
				   .SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f))
				   .SetShadowOffset(FVector2D::ZeroVector)
				   .SetShadowColorAndOpacity(FLinearColor::Black)
				   .SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
				   .SetHighlightShape( BOX_BRUSH( "TextBlockHighlightShape", FMargin(3.0f/8.0f)))
				   );
		
		Style->Set("InstaLODUI.ButtonPrimary.DefaultTextStyle", FTextBlockStyle()
				   .SetFont(NormalFont)
				   .SetColorAndOpacity(FSlateColor::UseForeground())
				   .SetShadowOffset(FVector2D(1, 1))
				   .SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f))
				   .SetShadowOffset(FVector2D::ZeroVector)
				   .SetShadowColorAndOpacity(FLinearColor::Black)
				   .SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
				   .SetHighlightShape( BOX_BRUSH( "TextBlockHighlightShape", FMargin(3.0f/8.0f)))
				   );
	}
	
	const FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(NormalFont)
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor( FLinearColor( 0.02f, 0.3f, 0.0f ) );
	
	// Credits screen
	{
		const FLinearColor EditorInstaLODPink = FLinearColor(FColor(255,0, 141));
		
		const FTextBlockStyle HelpNormal = FTextBlockStyle(NormalText)
		.SetFont(TTF_FONT("Fonts/Roboto-Regular", 12))
		.SetShadowOffset(FVector2D::UnitVector);
		
		Style->Set("InstaLODMeshReduction.Normal", HelpNormal);
		
		Style->Set("InstaLODMeshReduction.Strong", FTextBlockStyle(HelpNormal)
				   .SetFont(TTF_FONT("Fonts/Roboto-Bold", 12))
				   .SetShadowOffset(FVector2D::UnitVector));
		
		Style->Set("InstaLODMeshReduction.H1", FTextBlockStyle(HelpNormal)
				   .SetColorAndOpacity(EditorInstaLODPink)
				   .SetFont(TTF_FONT("Fonts/Roboto-Bold", 36))
				   .SetShadowOffset(FVector2D::UnitVector));
		
		Style->Set("InstaLODMeshReduction.H2", FTextBlockStyle(HelpNormal)
				   .SetColorAndOpacity(EditorInstaLODPink)
				   .SetFont(TTF_FONT("Fonts/Roboto-Bold", 30))
				   .SetShadowOffset(FVector2D::UnitVector));
		
		Style->Set("InstaLODMeshReduction.H3", FTextBlockStyle(HelpNormal)
				   .SetFont(TTF_FONT("Fonts/Roboto-Bold", 24))
				   .SetColorAndOpacity(EditorInstaLODPink)
				   .SetShadowOffset(FVector2D::UnitVector));
		
		Style->Set("InstaLODMeshReduction.H4", FTextBlockStyle(HelpNormal)
				   .SetColorAndOpacity(EditorInstaLODPink)
				   .SetFont(TTF_FONT("Fonts/Roboto-Bold", 18))
				   .SetShadowOffset(FVector2D::UnitVector));
		
		Style->Set("InstaLODMeshReduction.H5", FTextBlockStyle(HelpNormal)
				   .SetFont(TTF_FONT("Fonts/Roboto-Bold", 12))
				   .SetShadowOffset(FVector2D::UnitVector));
		
		Style->Set("InstaLODMeshReduction.H6", FTextBlockStyle(HelpNormal)
				   .SetFont(TTF_FONT("Fonts/Roboto-Bold", 10))
				   .SetShadowOffset(FVector2D::UnitVector));
		
		const FTextBlockStyle LinkText = FTextBlockStyle(NormalText)
			.SetFont(TTF_FONT("Fonts/Roboto-Bold", 12))
			.SetColorAndOpacity(EditorInstaLODPink)
			.SetShadowOffset(FVector2D::UnitVector);
		
		const FButtonStyle HoverOnlyHyperlinkButton = FButtonStyle()
			.SetNormal(FSlateNoResource())
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f)));
		
		const FHyperlinkStyle HoverOnlyHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(HoverOnlyHyperlinkButton)
			.SetTextStyle(LinkText)
			.SetPadding(FMargin(0.0f));
		
		Style->Set("InstaLODMeshReduction.Hyperlink", HoverOnlyHyperlink);
		
		Style->Set( "InstaLODMeshReduction.HeaderImage", FInlineTextImageStyle()
				   .SetImage( IMAGE_BRUSH( "InstaLOD_Logo_880x187", FVector2D(880, 187) ) )
				   .SetBaseline( 0 ));
		
		Style->Set( "InstaLODMeshReduction.FooterImage", FInlineTextImageStyle()
				   .SetImage( IMAGE_BRUSH( "InstaLOD_Logo_302x64", FVector2D(302, 64) ) )
				   .SetBaseline( 0 ));
		
		Style->Set( "InstaLODMeshReduction.LogoLarge", new IMAGE_BRUSH( "InstaLOD_Logo_880x187", FVector2D(880, 187) ));
		Style->Set( "InstaLODMeshReduction.LogoSmall", new IMAGE_BRUSH( "InstaLOD_Logo_302x64", FVector2D(302, 64) ));
		Style->Set( "InstaLODMeshReduction.LogoTiny", new IMAGE_BRUSH( "InstaLOD_Logo_302x64", FVector2D(151, 32) ));
	}

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FInstaLODPluginStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FInstaLODPluginStyle::Get()
{
	return *StyleInstance;
}


