/**
 * InstaLODOptimizeToolCustomization.h (InstaLOD)
 *
 * Copyright 2016-2020 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODOptimizeToolCustomization.h
 * @copyright 2016-2020 InstaLOD GmbH. All rights reserved.
 * @section License
 */



#ifndef InstaLOD_InstaLODOptimizeToolCustomization_h
#define InstaLOD_InstaLODOptimizeToolCustomization_h

#include "IDetailCustomization.h"

 // Additional Includes
#include "DetailLayoutBuilder.h"

 /**
 *	Special Customization for the Optimize Tool.
 */
class FInstaLODOptimizeToolCustomization : public IDetailCustomization
{
public:
	FInstaLODOptimizeToolCustomization();
	~FInstaLODOptimizeToolCustomization();


	/************************************************************************/
	/* Utilities                                                            */
	/************************************************************************/

	/** called when something wants to refresh the DetailsView. */
	void OnForceRefreshDetails();


	/************************************************************************/
	/* IDetailCustomization Interface                                       */
	/************************************************************************/

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	
	class UInstaLODOptimizeTool* OptimizeTool;
	IDetailLayoutBuilder* DetailLayoutBuilder;

	using Super = IDetailCustomization;
};

#endif /*InstaLOD_InstaLODOptimizeToolCustomization_h*/