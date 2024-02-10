/**
 * InstaLODOptimizeToolCustomization.cpp (InstaLOD)
 *
 * Copyright 2016-2020 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODOptimizeToolCustomization.cpp
 * @copyright 2016-2020 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODOptimizeToolCustomization.h"

#include "Slate/InstaLODWindow.h"
#include "Tools/InstaLODOptimizeTool.h"

#include "PropertyCustomizationHelpers.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"

#include "Widgets/Input/SCheckBox.h"
#include "Animation/AnimSequence.h"

FInstaLODOptimizeToolCustomization::FInstaLODOptimizeToolCustomization() : Super(),
OptimizeTool(nullptr),
DetailLayoutBuilder(nullptr)
{}

FInstaLODOptimizeToolCustomization::~FInstaLODOptimizeToolCustomization()
{
	OptimizeTool = nullptr;
	DetailLayoutBuilder = nullptr;
}

void FInstaLODOptimizeToolCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// save the DetailBuilder, so we can use it later
	DetailLayoutBuilder = &DetailBuilder;

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingEdited);

	UObject* Instance = nullptr;

	if (ObjectsBeingEdited.Num() == 1)
	{
		Instance = ObjectsBeingEdited[0].Get();
		OptimizeTool = Cast<UInstaLODOptimizeTool>(Instance);
	}
	
	if(OptimizeTool == nullptr)
		return;

	// get skeleton category
	IDetailCategoryBuilder& SkeletonOptimizationCategory = DetailLayoutBuilder->EditCategory("Skeleton Optimization", FText::GetEmpty(), ECategoryPriority::Important);

	TSharedPtr<IPropertyHandle> BakePoseHandle = DetailBuilder.GetProperty(FName("BakePose"), UInstaLODOptimizeTool::StaticClass());
	BakePoseHandle->MarkHiddenByCustomization();

	const FText BakePoseLabel = FText::FromString("Bake Pose");

	auto fnFilterValidBakePoses = [this](const FAssetData& AssetData) -> bool
	{
		auto* InstaLODWindow = this->OptimizeTool->GetInstaLODWindow();

		if (InstaLODWindow == nullptr)
			return false; 

		auto Components = InstaLODWindow->GetEnabledSelectedComponents();
		if (Components.Num() == 1 && InstaLODWindow->GetNumSkeletalMeshComponents() == 1)
			return false;

		TSharedPtr<FInstaLODMeshComponent> Component = Components[0];
		USkeletalMeshComponent *const SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component->GetComponent());

		if (SkeletalMeshComponent == nullptr)
			return false;

		USkeletalMesh *const SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();

		if (SkeletalMesh == nullptr)
			return false;

		const FString CurrentMeshSkeletonName = FString::Printf(TEXT("%s'%s'"), *SkeletalMesh->GetSkeleton()->GetClass()->GetName(), *SkeletalMesh->GetSkeleton()->GetPathName());
		const FString SkeletonName = AssetData.GetTagValueRef<FString>("TargetSkeleton");

		return SkeletonName != CurrentMeshSkeletonName;
	};

	SkeletonOptimizationCategory.AddCustomRow(BakePoseLabel)
	.NameContent()
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(NSLOCTEXT("InstaLODUI", "BakePoseSkeletal", "Bake Pose"))
	]
	.ValueContent()
	[
		SNew(SObjectPropertyEntryBox)
		.PropertyHandle(BakePoseHandle)
		.AllowedClass(UAnimSequence::StaticClass())
		.OnShouldFilterAsset_Lambda(fnFilterValidBakePoses)
	].IsEnabled(MakeAttributeLambda([this]() -> bool { return this->OptimizeTool->bSingleSkeletalMeshSelected; }));

	// Edit Mesh Editor CVar category
	IDetailCategoryBuilder& CVarCategory = DetailLayoutBuilder->EditCategory("Mesh Editor Reduction Settings", FText::FromString("Mesh Editor Reduction Settings"), ECategoryPriority::Default);
	DetailLayoutBuilder->HideProperty(DetailLayoutBuilder->GetProperty(GET_MEMBER_NAME_CHECKED(UInstaLODOptimizeTool, bIsMeshReductionSettingsEnabled)));

	struct CVarInfo
	{
		FString CVarName;
		FString CVarDisplayName;
		FString CVarToolTip;
	};

	const TArray<CVarInfo> CVarInfos = 
	{
		{ "InstaLOD.ForceOptimizerWeights", "Vertex Colors As Optimizer Weights", "Mesh Editor: Vertex Colors As Optimizer Weights"},
		{ "InstaLOD.LockBoundaries", "Lock Boundaries", "Mesh Editor: Prevents optimization of vertices on the mesh boundary. This is useful if the mesh contains holes that need to be preserved as is."},
		{ "InstaLOD.LockSplits", "Lock Splits", "Mesh Editor: Prevents optimization of split vertices. This is useful if UV and normal splits need to be preserved as is."},
		{ "InstaLOD.OptimizeDeterministic", "Deterministic", "Mesh Editor: Makes the algorithm deterministic at the cost of speed."}
	};

	for (const CVarInfo& Info : CVarInfos)
	{
		IConsoleVariable *const CVarVariable = IConsoleManager::Get().FindConsoleVariable(*Info.CVarName);

		if (CVarVariable == nullptr)
			continue;

		auto fnValueChanged = [CVarVariable](ECheckBoxState Value) 
		{
			// NOTE: SetByConsole to override priority block
			CVarVariable->Set(Value == ECheckBoxState::Checked ? 1 : 0, ECVF_SetByConsole);
		};
		auto fnValueIsSet = [CVarVariable]() -> ECheckBoxState
		{
			return CVarVariable->GetInt() > 0 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		};

		CVarCategory.AddCustomRow(FText::FromString("Mesh Editor Reduction Settings"))
		.NameContent()
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(Info.CVarToolTip))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(FText::FromString(Info.CVarDisplayName))
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.ToolTipText(FText::FromString(Info.CVarToolTip))
			.IsChecked_Lambda(fnValueIsSet)
			.OnCheckStateChanged_Lambda(fnValueChanged)
		];
	}
}