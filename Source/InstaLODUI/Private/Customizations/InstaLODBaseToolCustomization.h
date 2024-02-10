/**
 * InstaLODBaseToolCustomization.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODBaseToolCustomization.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODBaseToolCustomization_h
#define InstaLOD_InstaLODBaseToolCustomization_h

#include "IDetailCustomization.h"

// Additional Includes
#include "DetailLayoutBuilder.h"

/**
 * Customization for all the tools that inherit from InstaLODBaseTool.
 * Used to modify the way the properties of each object are displayed.
 */
class FInstaLODBaseToolCustomization : public IDetailCustomization
{
public:
	FInstaLODBaseToolCustomization();
	~FInstaLODBaseToolCustomization();

	/************************************************************************/
	/* All Tools                                                            */
	/************************************************************************/

	bool ToolButtonEnabled() const;
	bool CanTargetLODIndex() const;
	bool CanReplaceSelectedMeshes() const;
	bool CanPivotPosition() const;
	bool CanBoundingBoxPivotPosition() const;
	bool CanWorldSpacePivotPosition() const;

	/************************************************************************/
	/* Optimize Tool                                                        */
	/************************************************************************/
	EVisibility IsOptimizeToolSettingsRadioButtonVisible() const;
	void OptimizeToolSettingsCheckBoxClicked(const ECheckBoxState NewCheckedState, int32 CheckBoxIndex);
	ECheckBoxState IsOptimizeToolSettingsCheckBoxSelected(int32 CheckBoxIndex) const;
	bool IsOptimizeToolSettingEnabled(int32 CheckBoxIndex) const;

	/************************************************************************/
	/* Remesh Tool                                                          */
	/************************************************************************/
	EVisibility IsRemeshToolSettingsRadioButtonVisible() const;
	void RemeshToolSettingsCheckBoxClicked(const ECheckBoxState NewCheckedState, int32 CheckBoxIndex);
	ECheckBoxState IsRemeshToolSettingsCheckBoxSelected(int32 CheckBoxIndex) const;
	bool IsRemeshToolSettingEnabled(int32 CheckBoxIndex) const;

	/************************************************************************/
	/* Utilities                                                            */
	/************************************************************************/
	void OnForceRefreshDetails();
	static FReply ExecuteTool(IDetailLayoutBuilder* DetailBuilder, UFunction* MethodToExecute);

	/************************************************************************/
	/* IDetailCustomization Interface                                       */
	/************************************************************************/
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	
private:
	class UInstaLODBaseTool* ToolInstance;	/**< reference to the tool that is being inspected. */
	IDetailLayoutBuilder* DetailLayoutBuilder;
	
	FSimpleDelegate RefreshDetailsDelegate; /**< called when someone wants to refresh the DetailsView. */
	
	class UInstaLODOptimizeTool* OptimizeTool;
	class UInstaLODRemeshTool* RemeshTool;
};

#endif
