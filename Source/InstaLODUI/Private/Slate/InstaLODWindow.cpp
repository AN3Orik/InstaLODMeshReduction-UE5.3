/**
 * InstaLODWindow.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODWindow.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODWindow.h"
#include "InstaLODUIPCH.h"

#include "InstaLODModule.h"
#include "Slate/InstaLODPluginStyle.h"
#include "Tools/InstaLODBaseTool.h"
#include "Tools/InstaLODSettings.h"
#include "Tools/InstaLODRemeshTool.h"
#include "Tools/InstaLODImposterizeTool.h"
#include "Tools/InstaLODMaterialMergeTool.h"
#include "Editor/EditorStyle/Private/SlateEditorStyle.h"

#include "LevelEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "InstaLOD"

SInstaLODWindow::SInstaLODWindow():
CurrentTool(0)
{}

SInstaLODWindow::~SInstaLODWindow()
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnActorSelectionChanged().RemoveAll(this);

	// remove all delegates
	USelection::SelectionChangedEvent.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);
	FEditorDelegates::MapChange.RemoveAll(this);
	FEditorDelegates::NewCurrentLevel.RemoveAll(this);
}

void SInstaLODWindow::Construct(const FArguments& InArgs)
{
	// reset arrays
	SelectedObjects.Empty();
	SelectedComponents.Empty();
	
	// bind editor delegate to get updates on selected Actors
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnActorSelectionChanged().AddRaw(this, &SInstaLODWindow::OnActorSelectionChanged);

	// get all currently selected Actors
	USelection* SelectedActors = GEditor->GetSelectedActors();

	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		UObject* Actor = Cast<UObject>(*Iter);
		if (Actor)
		{
			SelectedObjects.Add(Actor);
		}
	}

	// reset the selected actor view
	Reset();

	// tools passed by the Module
	RegisteredTools = InArgs._ToolsToRegister;

	// pass each tool a reference to this Window
	for (UInstaLODBaseTool* RegisteredTool : RegisteredTools)
	{
		if (RegisteredTool != nullptr)
		{
			RegisteredTool->SetInstaLODWindow(this);
		}
	}

	// we need the PropertyModule to create a DetailView
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowFavoriteSystem = true;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bShowScrollBar = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowOptions = true;
	
	const FText WarningText = FText::FromString("Operation temporarily unavailable as Unreal Engine core functionality has been removed from macOS.");

	const FSlateBrush* DropDownIcon =  FInstaLODPluginStyle::Get().GetBrush("InstaLODUI.MainComboBoxDownArrowIcon");

	DetailView = PropertyModule.CreateDetailView(DetailsViewArgs);

	ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.Padding(2.5f, 5, 0, 5)
			.AutoHeight()
			.MaxHeight(50)
			[
				SNew(SBox)
				.HAlign(EHorizontalAlignment::HAlign_Left)
				.MaxDesiredWidth(500)
				.MinDesiredHeight(70)
				.MaxDesiredHeight(70)
				.WidthOverride(500)
				.HeightOverride(70)
				[
					SNew(SComboButton)
					.ButtonStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.MainComboboxButton")
					.HasDownArrow(false)
					.ButtonContent()
					[
						 SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f,5.0f,0.0f)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						.VAlign(EVerticalAlignment::VAlign_Center)
						[
							SNew(STextBlock)
							.Margin(0.5f)
							.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.MainComboboxButton.DefaultTextStyle")
							.Text_Lambda([this]() {
								const UInstaLODBaseTool* const Tool = RegisteredTools[CurrentTool];
								if (Tool != nullptr)
								{
									return Tool->GetComboBoxItemName();
								}

								return FText::GetEmpty(); })
							.Justification(ETextJustify::Center)
						]
						+ SHorizontalBox::Slot()
							.Padding(5.0f, 0.0f,10.0f,0.0f)
							.HAlign(EHorizontalAlignment::HAlign_Right)
							.VAlign(EVerticalAlignment::VAlign_Center)
							.AutoWidth()
							.MaxWidth(25.0f)
							[
								SNew(SImage)
								.DesiredSizeOverride(FVector2D(16.0f, 25.0f))
								.Image(DropDownIcon)
						]
					]
					.OnGetMenuContent_Lambda([this]()
						{

							FMenuBuilder MenuBuilder(true, nullptr);
							MenuBuilder.BeginSection("Operations");

							for (int32 ToolIndex = 0; ToolIndex < RegisteredTools.Num(); ToolIndex++)
							{
								const UInstaLODBaseTool* const Tool = RegisteredTools[ToolIndex];
								if (Tool != nullptr)
								{
									FUIAction ItemAction(FExecuteAction::CreateSP(this, &SInstaLODWindow::SetCurrentActiveTabIndex, ToolIndex));
									MenuBuilder.AddMenuEntry(Tool->GetComboBoxItemName(), TAttribute<FText>(), FSlateIcon(), ItemAction);
								}
							}

							MenuBuilder.EndSection();

							return MenuBuilder.MakeWidget();
						})
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(5.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					// put a Scrollbox around the content area, in case someone resizes the window smaller
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.Padding(10, 2.5f, 0, 2.5f)
						.MaxHeight(100)
						[
							SNew(SBox)
							.HAlign(HAlign_Fill)
							[
								SNew(STextBlock)
								.Justification(ETextJustify::Left)
								.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.OperationInformation")
								.AutoWrapText(true)
								.Text_Lambda([this]() {
									const UInstaLODBaseTool* const Tool = RegisteredTools[CurrentTool];
									if (Tool != nullptr)
									{
										return Tool->GetOperationInformation();
									}

									return FText::GetEmpty(); })
							]
						]
						+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f, 4.0f, 2.0f, 4.0f)
							[
								SNew(SSeparator)
							]

						+ SVerticalBox::Slot()
							.Padding(10, 2.5f, 0, 0)
							.AutoHeight()
							.MaxHeight(128.0f)
							[
								// list of selected actors
								SAssignNew(SelectedActorList, SListView<TSharedPtr<FInstaLODMeshComponent>>)
								.ListItemsSource(&SelectedComponents)
								.OnGenerateRow(this, &SInstaLODWindow::MakeComponentListItemWidget)
							]
					]
				+ SScrollBox::Slot()
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					[
						// detail view that shows the details of the used tools (UObject)
						DetailView->AsShared()
					]
				]
			]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(30)
				[
					SNew(SBox)
					.MinDesiredHeight(30)
					.HeightOverride(30)
					.MaxDesiredHeight(50)
						[
							SNew(SButton)
							.VAlign(EVerticalAlignment::VAlign_Center)
							.ButtonStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary")
							.ButtonColorAndOpacity(FLinearColor::FromSRGBColor(FColor(255, 47, 141)))
							.OnClicked_Lambda([this]() -> FReply {
									UInstaLODBaseTool* const Tool = RegisteredTools[CurrentTool];
									if(Tool != nullptr)
										{
											Tool->ExecuteMeshOperation();
										}
									return FReply::Handled();
												})
							.IsEnabled_Lambda([this]() -> bool{
									const UInstaLODBaseTool* const Tool = RegisteredTools[CurrentTool];
									if ((Tool != nullptr) && (Tool->GetInstaLODWindow() != nullptr) && (Tool->GetClass() != UInstaLODSettings::StaticClass()))
										{
											return Tool->GetInstaLODWindow()->GetEnabledSelectedMeshComponents().Num() > 0;
										}
									return false;
												})
							[
								SNew(STextBlock)
								.Text(FText::FromString("Execute"))
								.TextStyle(FInstaLODPluginStyle::Get(), "InstaLODUI.ButtonPrimary.BoldTextStyle")
								.Justification(ETextJustify::Center )
							]
						]
				]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(10, 25, 10, 25))
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("InstaLODUI", "WindowFooter",
						"InstaLOD GmbH 2016 - 2023\n"
						"http://www.InstaLOD.com\n"
						"SDK2023"))
					.Justification(ETextJustify::Center)
				]
		];

	UpdateToolbar();
}


int32 SInstaLODWindow::GetNumCameraComponents() const
{
	int32 Count = 0;
	for(const TSharedPtr<FInstaLODMeshComponent>& InstaLODMeshComponent : SelectedComponents)
	{
		if (InstaLODMeshComponent->bShouldBeIncluded && InstaLODMeshComponent->CameraComponent.IsValid())
			Count++;
	}
	return Count;
}

int32 SInstaLODWindow::GetNumStaticMeshsComponents() const
{
	int32 Count = 0;
	for(const TSharedPtr<FInstaLODMeshComponent>& InstaLODMeshComponent : SelectedComponents)
	{
		if (InstaLODMeshComponent->bShouldBeIncluded && InstaLODMeshComponent->StaticMeshComponent.IsValid())
			Count++;
	}
	return Count;
}

int32 SInstaLODWindow::GetNumSkeletalMeshComponents() const
{
	int32 Count = 0;
	for(const TSharedPtr<FInstaLODMeshComponent>& InstaLODMeshComponent : SelectedComponents)
	{
		if (InstaLODMeshComponent->bShouldBeIncluded && InstaLODMeshComponent->SkeletalMeshComponent.IsValid())
			Count++;
	}
	return Count;
}


TArray<TSharedPtr<FInstaLODMeshComponent>> SInstaLODWindow::GetEnabledSelectedComponents() const
{
	return SelectedComponents;
}

TArray<AActor*> SInstaLODWindow::GetSelectedStaticMeshActors() const
{
	TArray<AActor*> Actors;
	for (UObject* SelectedObject : SelectedObjects)
	{
		AActor *const Actor = Cast<AActor>(SelectedObject);
		if (Actor == nullptr)
			continue;

		const bool bIsStaticMeshActor = Cast<AStaticMeshActor>(Actor) != nullptr;
		if (!bIsStaticMeshActor)
			continue;

		Actors.AddUnique(Actor);
	}
	return Actors;
}

TArray<TSharedPtr<FInstaLODMeshComponent>> SInstaLODWindow::GetEnabledSelectedCameraComponents() const
{
	TArray<TSharedPtr<FInstaLODMeshComponent>> MeshComponents;
	
	for (const TSharedPtr<FInstaLODMeshComponent>& MeshComponent : SelectedComponents)
	{
		// ignore unchecked components
		if (!MeshComponent->bShouldBeIncluded)
			continue;
		
		if (MeshComponent->CameraComponent == nullptr)
			continue;
		
		MeshComponents.Add(MeshComponent);
	}
	
	return MeshComponents;
}

TArray<TSharedPtr<FInstaLODMeshComponent>> SInstaLODWindow::GetEnabledSelectedMeshComponents() const
{
	TArray<TSharedPtr<FInstaLODMeshComponent>> MeshComponents;
	
	for (const TSharedPtr<FInstaLODMeshComponent>& MeshComponent : SelectedComponents)
	{
		// ignore unchecked components
		if (!MeshComponent->bShouldBeIncluded)
			continue;
		
		// require either a static mesh or skeletal mesh
		if (MeshComponent->StaticMeshComponent == nullptr &&
			MeshComponent->SkeletalMeshComponent == nullptr)
			continue;
		
		// ensure mesh data exists on the component
		if (MeshComponent->StaticMeshComponent != nullptr &&
			MeshComponent->StaticMeshComponent->GetStaticMesh() == nullptr)
			continue;
		
		if (MeshComponent->SkeletalMeshComponent != nullptr &&
			MeshComponent->SkeletalMeshComponent->GetSkeletalMeshAsset() == nullptr)
			continue;
		
		MeshComponents.Add(MeshComponent);
	}

	return MeshComponents;
}

void SInstaLODWindow::OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh)
{
	SelectedObjects = NewSelection;
	Reset();
}

void SInstaLODWindow::UpdateToolbar()
{
	DetailView->SetObject(nullptr);

	const ISlateStyle& StyleSet = FAppStyle::Get();

	// create a SHorizontalBox that we will fill based on the registered tools
	TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);
	UInstaLODBaseTool* Settings = nullptr;
	int32 SettingsIndex = -1;

	FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
	bool bIsAuthorized = InstaLODModule.GetInstaLODAPI() ? InstaLODModule.GetInstaLODAPI()->IsHostAuthorized() : false;

	const int kPadding = 5;
	const uint32 kMaximumWidth = 100u;

	// show the settings when not authorized
	if (bIsAuthorized == false)
	{
		CurrentTool = RegisteredTools.Num() - 1;
	}

	// selection change delegates
	USelection::SelectionChangedEvent.AddRaw(this, &SInstaLODWindow::OnLevelSelectionChanged);
	USelection::SelectObjectEvent.AddRaw(this, &SInstaLODWindow::OnLevelSelectionChanged);
	FEditorDelegates::MapChange.AddSP(this, &SInstaLODWindow::OnMapChange);
	FEditorDelegates::NewCurrentLevel.AddSP(this, &SInstaLODWindow::OnNewCurrentLevel);

	// update the details view
	UpdateDetailsView();
}

void SInstaLODWindow::SetCurrentActiveTabIndex(const int32 TabIndex)
{
	if (!RegisteredTools.IsValidIndex(TabIndex))
		return;

	CurrentTool = TabIndex;
	UpdateDetailsView();
}

void SInstaLODWindow::UpdateDetailsView()
{
	if (RegisteredTools.IsValidIndex(CurrentTool))
	{
		// set the viewed object of the DetailsView
		DetailView->SetObject(RegisteredTools[CurrentTool]);
	}
}

void SInstaLODWindow::ForceRefreshDetailsView()
{
	if (RegisteredTools.IsValidIndex(CurrentTool))
	{
		DetailView->ForceRefresh();
	}
}

void SInstaLODWindow::OnToolSelectionChanged(const ECheckBoxState NewCheckedState, int32 ToolIndex)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		CurrentTool = ToolIndex;
		UpdateDetailsView();
	}
}

TSharedRef<ITableRow> SInstaLODWindow::MakeComponentListItemWidget(TSharedPtr<FInstaLODMeshComponent> MeshComponent, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedPtr<SBox> MeshDataBox;

	FText OwningActorName = MeshComponent->IsValid() ? FText::FromString(MeshComponent->GetComponent()->GetOwner()->GetName()) : FText();
	FText ComponentName = MeshComponent->IsValid() ? FText::FromString(MeshComponent->GetComponent()->GetName()) : FText();
	FText MeshName = NSLOCTEXT("InstaLODUI", "NoMeshAvailable", "No Mesh Available");
	
	// fetch persisted state
	ECheckBoxState CheckBoxState = ECheckBoxState::Checked;
	auto PersistedCheckBoxState = MeshComponent->IsValid() ?  PersistedCheckBoxStates.Find(MeshComponent->GetComponent()) : nullptr;
	if (PersistedCheckBoxState)
	{
		CheckBoxState = *PersistedCheckBoxState;
	}
	
	// get mesh names
	if (MeshComponent->IsValid())
	{
		if (MeshComponent->StaticMeshComponent.IsValid() && MeshComponent->StaticMeshComponent->GetStaticMesh() != nullptr)
		{
			MeshName = FText::FromString(MeshComponent->StaticMeshComponent->GetStaticMesh()->GetName());
		}
		else if (MeshComponent->SkeletalMeshComponent.IsValid() && MeshComponent->SkeletalMeshComponent->GetSkeletalMeshAsset() != nullptr)
		{
			MeshName = FText::FromString(MeshComponent->SkeletalMeshComponent->GetSkeletalMeshAsset()->GetName());
		}
		else if (MeshComponent->CameraComponent.IsValid())
		{
			MeshName = NSLOCTEXT("InstaLODUI", "Camera", "Camera");
		}
	}
	
	MeshDataBox = SNew(SBox)
	[
		// disable UI element if the pointer became invalid
		SNew(SHorizontalBox)
		.IsEnabled((MeshComponent->IsValid()))
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.IsChecked(CheckBoxState)
			.ToolTipText(NSLOCTEXT("InstaLODUI", "IncludeMeshInOperation", "If selected, the mesh will be included in the mesh operation."))
			.OnCheckStateChanged_Lambda([=, this](ECheckBoxState NewState) // NOTE: Change [=, this] to [=] if you building with < C++20
								  {
									  MeshComponent->bShouldBeIncluded = (NewState == ECheckBoxState::Checked);
									  OnNewSelectionDelegate.Broadcast();
								  })
		]
		+ SHorizontalBox::Slot()
		.Padding(5.0f, 0, 0, 0)
		.AutoWidth()
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor(FLinearColor(MeshComponent->CameraComponent.IsValid() ? FColor::Orange : FColor::White)))
			.Text(FText::Format(NSLOCTEXT("InstaLODUI", "UIMeshFormatString", "{0} - {1} ({2})"), OwningActorName, MeshName, ComponentName))
		]
	];

	return SNew(STableRow<TSharedPtr<FInstaLODMeshComponent>>, OwnerTable)
		[
			MeshDataBox->AsShared()
		];
}

FReply SInstaLODWindow::OnExpandSelection(const float ExpandRadius)
{
	if (FMath::IsNearlyZero(ExpandRadius))
		return FReply::Handled();

	TArray<AActor*> Actors;
	for (UObject* SelectedObject : SelectedObjects)
	{
		AActor *const Actor = Cast<AActor>(SelectedObject);
		if (Actor == nullptr)
			continue;

		Actors.AddUnique(Actor);
	}

	if (Actors.Num() < 1)
		return FReply::Handled();

	FBox MinimumBoundingBox{Actors[0]->GetComponentsBoundingBox()};
	for (const auto & Actor : Actors)
	{
		MinimumBoundingBox += Actor->GetComponentsBoundingBox();
	}

	const float UnrealEngineUnitToMeterConversionFactor= 100.0f; 
	MinimumBoundingBox = MinimumBoundingBox.ExpandBy(ExpandRadius * UnrealEngineUnitToMeterConversionFactor);

	TArray<AActor*> SceneActors;
	UGameplayStatics::GetAllActorsOfClass(Actors[0]->GetWorld(), AActor::StaticClass(), SceneActors);

	GEditor->SelectNone(true, false);
	for (const auto & SceneActor : SceneActors)
	{
		if (MinimumBoundingBox.Intersect(SceneActor->GetComponentsBoundingBox()))
		{
			GEditor->SelectActor(SceneActor, true, true);
		}
	}
	return FReply::Handled();
}

void SInstaLODWindow::UpdateSelectedComponents()
{
	// filter by actors
	TArray<AActor*> Actors;
	for (UObject* SelectedObject : SelectedObjects)
	{
		AActor *const Actor = Cast<AActor>(SelectedObject);
		if (Actor == nullptr)
			continue;
		
		Actors.AddUnique(Actor);
	}

	// gather selected components relevant for us
	SelectedComponents.Empty();
	for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ++ActorIndex)
	{
		AActor *const Actor = Actors[ActorIndex];
		check(Actor != nullptr);

		const bool bIsCameraActor = Cast<ACameraActor>(Actor) != nullptr;
		bool bIncludeChildActors = true;
		
		if (bIsCameraActor)
			bIncludeChildActors = false;
		
		// add all child actors
		if (bIncludeChildActors)
		{
			TArray<UChildActorComponent*> ChildActorComponents;
			Actor->GetComponents<UChildActorComponent>(ChildActorComponents);
			for (UChildActorComponent* ChildComponent : ChildActorComponents)
			{
				AActor *const ChildActor = ChildComponent->GetChildActor();
				if (ChildActor)
				{
					Actors.AddUnique(ChildActor);
				}
			}
		}
		
		// allocate instalod components
		{
			TArray<USceneComponent*> SceneComponents;
			Actor->GetComponents<USceneComponent>(SceneComponents);
			
			for (USceneComponent* SceneComponent : SceneComponents)
			{
				TSharedPtr<FInstaLODMeshComponent> InstaLODMeshComponent;
				
				// only include compatible components
				if (Cast<UStaticMeshComponent>(SceneComponent) != nullptr && !bIsCameraActor)
					InstaLODMeshComponent = TSharedPtr<FInstaLODMeshComponent>(new FInstaLODMeshComponent(Cast<UStaticMeshComponent>(SceneComponent)));
				else if (Cast<USkeletalMeshComponent>(SceneComponent) != nullptr && !bIsCameraActor)
					InstaLODMeshComponent = TSharedPtr<FInstaLODMeshComponent>(new FInstaLODMeshComponent(Cast<USkeletalMeshComponent>(SceneComponent)));
				else if (Cast<UCameraComponent>(SceneComponent) != nullptr)
					InstaLODMeshComponent = TSharedPtr<FInstaLODMeshComponent>(new FInstaLODMeshComponent(Cast<UCameraComponent>(SceneComponent)));
				else
					continue;
				
				auto PersistedCheckBoxState = PersistedCheckBoxStates.Find(InstaLODMeshComponent->GetComponent());
				if (PersistedCheckBoxState)
				{
					InstaLODMeshComponent->bShouldBeIncluded = (*PersistedCheckBoxState == ECheckBoxState::Checked);
				}
				SelectedComponents.Add(InstaLODMeshComponent);
			}
		}
	}

	// rebuild list widget
	if (SelectedActorList.IsValid())
	{
		SelectedActorList->ClearSelection();
		SelectedActorList->RequestListRefresh();
	}

	OnNewSelectionDelegate.Broadcast();
}

ECheckBoxState SInstaLODWindow::OnIsToolSelected(int32 ToolIndex) const
{
	return (CurrentTool == ToolIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SInstaLODWindow::PersistCheckBoxStates()
{
	PersistedCheckBoxStates.Empty();
	for (const TSharedPtr<FInstaLODMeshComponent>& SelectedComponent : SelectedComponents)
	{
		check(SelectedComponent.IsValid());
		
		const ECheckBoxState CheckBoxState = SelectedComponent->bShouldBeIncluded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		
		PersistedCheckBoxStates.Add(SelectedComponent->GetComponent(), CheckBoxState);
	}
}

void SInstaLODWindow::OnLevelSelectionChanged(UObject* Object)
{
	Reset();
}

void SInstaLODWindow::OnMapChange(uint32 MapFlags)
{
	Reset();
}

void SInstaLODWindow::OnNewCurrentLevel()
{
	Reset();
}

void SInstaLODWindow::Reset()
{
	PersistCheckBoxStates();
	UpdateSelectedComponents();
}

#undef LOCTEXT_NAMESPACE
