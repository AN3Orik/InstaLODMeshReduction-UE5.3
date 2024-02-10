/**
 * InstaLODWindow.h (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODWindow.h
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODWindow_h
#define InstaLOD_InstaLODWindow_h

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "InstaLODTypes.h"

DECLARE_MULTICAST_DELEGATE(FOnNewSelection)

/**
 * This is the window that is placed inside of a dockable tab.
 * All information and tools will be displayed inside of this window using a DetailView and Customizations.
 */
class SInstaLODWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SInstaLODWindow) {}
	SLATE_ARGUMENT(TArray<class UInstaLODBaseTool*>, ToolsToRegister)
	SLATE_END_ARGS()
 
	SInstaLODWindow();
	~SInstaLODWindow();

	void Construct(const FArguments& InArgs);

	void UpdateToolbar();
	void UpdateDetailsView();
	
	void ForceRefreshDetailsView();
	
	TArray<TSharedPtr<FInstaLODMeshComponent>> GetEnabledSelectedCameraComponents() const;
	TArray<TSharedPtr<FInstaLODMeshComponent>> GetEnabledSelectedMeshComponents() const;
	TArray<TSharedPtr<FInstaLODMeshComponent>> GetEnabledSelectedComponents() const;
	/**
	 * Gets the selected static mesh actors.
	 *
	 * @return The selected static mesh actors.
	 */
	TArray<AActor*> GetSelectedStaticMeshActors() const;
	/**
	 * Returns the Delegate which is called when the Selection changed. 
	 *
	 * @return the delegate
	 */
	FOnNewSelection& OnNewSelection() { return OnNewSelectionDelegate; }
	
	int32 GetNumCameraComponents() const;		/**< returns the number of selected camera components. */
	int32 GetNumStaticMeshsComponents() const;	/**< returns the number of selected static meshs. */
	int32 GetNumSkeletalMeshComponents() const;	/**< returns the number of selected skeletal meshs. */

	/**
	 * Sets the current active tab by index.
	 *
	 * @param TabIndex the active tab index.
	 */
	void SetCurrentActiveTabIndex(const int32 TabIndex);

	/**
	 * Called on expand selection.
	 *
	 * @param ExpandRadius the expansion radius.
	 * @return unreal engine event reply.
	 */
	FReply OnExpandSelection(const float ExpandRadius);

private:
	void OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh);
	void OnToolSelectionChanged(const ECheckBoxState NewCheckedState, int32 ToolIndex);
	
	/** delegate for the creation of the list view item's widget. */
	TSharedRef<ITableRow> MakeComponentListItemWidget(TSharedPtr<FInstaLODMeshComponent> MeshComponent, const TSharedRef<STableViewBase>& OwnerTable);
	
	void UpdateSelectedComponents();
	
	ECheckBoxState OnIsToolSelected(int32 ToolIndex) const;
	
	void PersistCheckBoxStates();
	
	void OnLevelSelectionChanged(UObject* Object);

	void OnMapChange(uint32 MapFlags);
	void OnNewCurrentLevel();
	
	void Reset();

	TSharedPtr<class IDetailsView> DetailView;	/**< Detail Viewer that shows the details of the used tools (UObject) */
	TSharedPtr<SListView<TSharedPtr<FInstaLODMeshComponent>>> SelectedActorList; /**< the list view for the selected actors. */
	TArray<TSharedPtr<FInstaLODMeshComponent>> SelectedComponents;	/**< selected components (cameras, meshes). */
	TArray<class UInstaLODBaseTool*> RegisteredTools;	/**< all registered tools. */
	
	int32 CurrentTool;										/**< the index of the currently active tool. */
	TArray<UObject*> SelectedObjects;						/**< teh list of currently selected objects. */
	TMap<UObject*, ECheckBoxState> PersistedCheckBoxStates;	/**< A map to save the state on the checkBoxes. */
	FOnNewSelection OnNewSelectionDelegate;					/**< called when a new selection in the viewport was made. */
};

#endif