/**
 * InstaLODSettings.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODSettings.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "CoreMinimal.h"
#include "Tools/InstaLODBaseTool.h"
#include "InstaLODSettings.generated.h"

/**
 *	Base Class for the Settings tab
 */
UCLASS(HideCategories="Utilities")
class INSTALODUI_API UInstaLODSettings : public UInstaLODBaseTool
{
	GENERATED_BODY()

	/// VARIABLES ///

public:

	/** License Information */
	UPROPERTY(EditAnywhere, Category = "LicenseInfo")
		FText LicenseInformation;

	/** SDK Version */
	UPROPERTY(EditAnywhere, Category = "SDKVersion")
		FText SDKVersion;

	/** Account Name */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Account"), Category = "Settings")
		FText AccountName;

	/** Serial/Password */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Serial/Password"), Category = "Settings")
		FText SerialPassword;

	/** Authorized? */
	bool bAuthorized = false;

	/// FUNCTIONS ///	

public:

	/** Constructor */
	UInstaLODSettings();

	/************************************************************************/
	/* Utilities                                                            */
	/************************************************************************/

	/** Clears the selection of all LODs. */
	UFUNCTION(Exec, meta = (DisplayName = "Clear Selection of LODs"), Category = "Tools")
		void ClearLODsFromSelection();

	UFUNCTION(Exec, meta = (DisplayName = "Authorize Workstation"), Category = "Authorize")
		void AuthorizeWorkstation();

	UFUNCTION(Exec, meta = (DisplayName = "Deauthorize Workstation"), Category = "Deauthorize")
		void DeauthorizeWorkstation();

	UFUNCTION(Exec, meta = (DisplayName = "Reset Settings"), Category = "Settings")
		void ResetToolSettings();

	UFUNCTION(Exec, meta = (DisplayName = "Load Profile"), Category = "Settings")
		void LoadProfile();

	UFUNCTION(Exec, meta = (DisplayName = "Online Help"), Category = "Settings")
		void OnlineHelp();

	UFUNCTION(Exec, meta = (DisplayName = "Submit Feedback"), Category = "Settings")
		void SubmitFeedback();

	/************************************************************************/
	/* Getters / Setters                                                    */
	/************************************************************************/

	/** Returns License Information. */
	FText GetLicenseInfo() const { return LicenseInformation; }

	/** Returns SDK Version. */
	FText GetSDKVersion() const { return SDKVersion; }

	/** Start - UInstaLODBaseTool Interface */
	virtual FText GetFriendlyName() const override;
	virtual FText GetToolBarToolTip() const override;
	virtual FText GetComboBoxItemName() const override;
	virtual FText GetOperationInformation() const override;
	virtual int32 GetOrderId() const override;
	/** End - UInstaLODBaseTool Interface */
};
