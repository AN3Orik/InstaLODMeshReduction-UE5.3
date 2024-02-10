/**
 * InstaLODBaseTool.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODBaseTool.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#pragma once

#include "InstaLOD/InstaLOD.h"
#include "InstaLOD/InstaLODAPI.h"
#include "InstaLOD/InstaLODMeshExtended.h"
#include "InstaLODUI/Private/InstaLODTypes.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InstaLODBaseTool.generated.h"

UENUM()
enum class EInstaLODImportance : uint8
{
	InstaLOD_OFF		UMETA(DisplayName = "Off"),
	InstaLOD_Lowest		UMETA(DisplayName = "Lowest"),
	InstaLOD_Low		UMETA(DisplayName = "Low"),
	InstaLOD_Normal		UMETA(DisplayName = "Normal"),
	InstaLOD_High		UMETA(DisplayName = "High"),
	InstaLOD_Highest	UMETA(DisplayName = "Highest")
};

UENUM()
enum class EInstaLODTextureSize : uint8
{
	InstaLOD_128		UMETA(DisplayName = "128"),
	InstaLOD_256		UMETA(DisplayName = "256"),
	InstaLOD_512		UMETA(DisplayName = "512"),
	InstaLOD_1024		UMETA(DisplayName = "1024"),
	InstaLOD_2K			UMETA(DisplayName = "2k"),
	InstaLOD_4K			UMETA(DisplayName = "4k"),
	InstaLOD_8K			UMETA(DisplayName = "8k"),
	InstaLOD_16K		UMETA(DisplayName = "16k")
};

UENUM()
enum class EInstaLODSuperSampling : uint8
{
	InstaLOD_None		UMETA(DisplayName = "None"),
	InstaLOD_X2			UMETA(DisplayName = "x2"),
	InstaLOD_X4			UMETA(DisplayName = "x4")
};

UENUM()
enum class EInstaLODTextureFilter : uint8
{
	InstaLOD_Nearest	UMETA(DisplayName = "Nearest"),
	InstaLOD_Bilinear	UMETA(DisplayName = "Bilinear"),
	InstaLOD_Bicubic	UMETA(DisplayName = "Bicubic")
};

UENUM()
enum class EInstaLODResultUsage : uint8
{
	InstaLOD_AppendToLOD	UMETA(DisplayName = "Append to LOD chain"),
	InstaLOD_ReplaceLOD		UMETA(DisplayName = "Replace LOD at target index"),
	InstaLOD_NewAsset		UMETA(DisplayName = "Save as new asset")
};

UENUM()
enum class EInstaLODUnwrapStrategy : uint8
{
	InstaLOD_Organic			UMETA(DisplayName = "Organic"),
	InstaLOD_HardSurfaceAngle	UMETA(DisplayName = "Hard Surface Angle"),
	InstaLOD_HardSurfaceAxial	UMETA(DisplayName = "Hard Surface Axial"),
	InstaLOD_Auto				UMETA(DisplayName = "Auto")
};

UENUM()
enum class EInstaLODPivotPosition : uint8
{
	InstaLOD_Default			UMETA(DisplayName = "Default"),
	InstaLOD_Center				UMETA(DisplayName = "Bounding Box Center"),
	InstaLOD_Top				UMETA(DisplayName = "Bounding Box Center Top"),
	InstaLOD_Bottom				UMETA(DisplayName = "Bounding Box Center Bottom"),
	InstaLOD_Custom				UMETA(DisplayName = "Custom Pivot Position"),
	InstaLOD_CustomLimited		UMETA(DisplayName = "Custom Pivot Position Relative To The Bounding Box")
};

/**
 * The baseclass for all tools that are implemented into the InstaLOD UI Window
 */
UCLASS(Config=InstaLOD, Abstract, HideCategories="Internal")
class INSTALODUI_API UInstaLODBaseTool : public UObject
{
	GENERATED_BODY()

public:
	UInstaLODBaseTool();
	~UInstaLODBaseTool();

	virtual void OnNewSelection();

	class SInstaLODWindow* GetInstaLODWindow() const { return InstaLODWindow; }
	void SetInstaLODWindow(class SInstaLODWindow* NewWindow);
	class IInstaLOD* GetInstaLODInterface() const { return InstaLOD; }

	bool SkeletalMeshsSelected();
	bool StaticMeshsSelected();

	/** Returns the DefaultPackageName for new packages based on the current selection. */
	FString GetDefaultPackageName() const;

	/************************************************************************/
	/* Tool Interface                                                       */
	/************************************************************************/

	virtual void OnMeshOperationError(const FText& Message = FText()) const;


	UFUNCTION(Exec, meta = (DisplayName = "Execute"), Category = "Utilities")
		void ExecuteMeshOperation();

	virtual void DeallocMeshOperation() {
	}

	virtual bool IsMeshOperationSuccessful() const {
		return false;
	}

	virtual InstaLOD::IInstaLODMaterial* GetBakeMaterial() {
		return nullptr;
	}

	virtual class UAnimSequence* GetBakePose() {
		return nullptr;
	}

	virtual bool IsMeshOperationExecutable(FText* OutErrorText) const;

	virtual void OnMeshOperationBegin();
	virtual void OnMeshOperationExecute(bool bIsAsynchronous);
	virtual void OnMeshOperationFinalize();

	virtual bool CanAppendMeshToInput(FInstaLODMeshComponent& Component, InstaLOD::IInstaLODMesh *Mesh, TArray<UMaterialInterface*>* UniqueMaterials) {
		return true;
	}

	virtual bool IsFreezingTransformsForMultiSelection() const {
		return false;
	}

	virtual bool IsMaterialDataRequired() const {
		return false;
	}

	virtual bool IsWorldTransformRequired() const {
		return false;
	}
	
	virtual void FinalizeBakeMaterial(class UMaterialInstanceConstant* Material) const {
	}

	virtual FMaterialProxySettings GetMaterialProxySettings() const
	{
		return FMaterialProxySettings();
	}

	virtual bool ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject);

	/** Gets the Friendly Name that is visible in the ToolBar. */
	virtual FText GetFriendlyName() const;

	/** Gets the ComboBoxItemName that is visible on the main combo box and in the combo box menu*/
	virtual FText GetComboBoxItemName() const;

	/** Gets information about the operation*/
	virtual FText GetOperationInformation() const;

	/** Gets the ToolTip that is visible when hovering the Toolbar. */
	virtual FText GetToolBarToolTip() const;

	/** Used to order the Tools in the ToolBar of the InstaLOD Window. */
	virtual int32 GetOrderId() const;

	inline static bool OrderById(const UInstaLODBaseTool& ToolOne, const UInstaLODBaseTool& ToolTwo)
	{
		return (ToolOne.GetOrderId() < ToolTwo.GetOrderId());
	}

	virtual void ResetSettings();

	virtual UWorld* GetWorld() const override;

	/************************************************************************/
	/* Expand Selection                                                     */
	/************************************************************************/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Expand Selection", ClampMin = 0, ClampMax = 10), Category = "Expand Selection")
	int32 ExpandSelectionRadius = 0.0f;

	/************************************************************************/
	/* Utilities                                                            */
	/************************************************************************/
	
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Save as"), Category = "Utilities")
	EInstaLODResultUsage ResultUsage;
	
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Target LOD Index", ClampMin = 0, ClampMax = 32), Category = "Utilities")
	int32 TargetLODIndex = 0;
 
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Update Selected Components With New Meshes"), Category = "Utilities")
	bool bReplaceSelectedMeshes = false;

	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Output Mesh Pivot Position"), Category = "Utilities")
		EInstaLODPivotPosition PivotPosition;

	/*The pivot position in world space.*/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Desired Pivot Position", EditCondition = "PivotPosition == EInstaLODPivotPosition::InstaLOD_Custom", EditConditionHides), Category = "Utilities")
		FVector WorldSpacePivotPosition = FVector::ZeroVector;

	/*The pivot position is restricted to be inside the bounding box. The vector (0.5, 0.5, 0.5) places the pivot in the center of the bounding box.*/
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Desired Pivot Position Limited In The Bounding Box", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f, EditCondition = "PivotPosition == EInstaLODPivotPosition::InstaLOD_CustomLimited", EditConditionHides), Category = "Utilities")
		FVector BoundingBoxPivotPosition = FVector::ZeroVector;
	
	static EInstaLODImportance GetImportanceValueForString(const FString& Value);
	static EInstaLODUnwrapStrategy GetUnwrapStrategyValueForString(const FString& Value);
	static bool IsValidJSONObject(const TSharedPtr<FJsonObject>& JsonObject, const FString& MeshOperationType);

	InstaLOD::IInstaLODMeshExtended* InputMesh;
	InstaLOD::IInstaLODMeshExtended* OutputMesh;
	InstaLOD::IInstaLODMaterialData* MaterialData;
	InstaLOD::IInstaLODSkeleton* Skeleton;
	TMap<int32, TPair<uint32, FString>> UEBoneIndexToInstaLODBoneIndexAndName;

	UPROPERTY(VisibleDefaultsOnly, Category = "Internal")
	bool bSingleSkeletalMeshSelected = false;

protected:

	UPROPERTY(VisibleDefaultsOnly, Category = "Internal")
	bool bSkeletalMeshsSelected = false;
	bool bStaticMeshsSelected = false;

	struct FScopedSlowTask* SlowTaskProgress;
	
	class SInstaLODWindow* InstaLODWindow;
	class IInstaLOD* InstaLOD;
	
	float ComponentsBoundingSphereRadius;
	TArray<TSharedPtr<FInstaLODMeshComponent>> MeshComponents;
};
