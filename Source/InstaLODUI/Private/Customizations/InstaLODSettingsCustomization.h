/**
 * InstaLODSettingsCustomization.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODSettingsCustomization.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODSettingsCustomization_h
#define InstaLOD_InstaLODSettingsCustomization_h

#include "IDetailCustomization.h"

// Additional Includes
#include "DetailLayoutBuilder.h"

/**
*	Special Customization for the Settings Tool.
*/
class FInstaLODSettingsCustomization : public IDetailCustomization
{
public:
	FInstaLODSettingsCustomization();
	~FInstaLODSettingsCustomization();
	
	/************************************************************************/
	/* Utilities                                                            */
	/************************************************************************/
	
	/** called when something wants to refresh the DetailsView. */
	void OnForceRefreshDetails();
	
	/** called when the Matching Button is pressed. */
	static FReply ExecuteTool(IDetailLayoutBuilder* DetailBuilder, UFunction* MethodToExecute);
	
	/** called when the AccountName box is changed. */
	void OnAccountNameChanged(const FText& NewText);
	
	/** called when the Password box is changed. */
	void OnPasswordChanged(const FText& NewText);
	
	/************************************************************************/
	/* IDetailCustomization Interface                                       */
	/************************************************************************/
	
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	class UInstaLODSettings* Settings;
	IDetailLayoutBuilder* DetailLayoutBuilder;
	FSimpleDelegate RefreshDetailsDelegate;
};

#endif
