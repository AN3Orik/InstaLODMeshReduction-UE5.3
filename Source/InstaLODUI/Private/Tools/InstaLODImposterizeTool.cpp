/**
 * InstaLODImposterizeTool.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODImposterizeTool.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODImposterizeTool.h"
#include "Utilities/InstaLODUtilities.h"
#include "Slate/InstaLODWindow.h"
#include "InstaLODUIPCH.h"

#include "PropertyEditorModule.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "InstaLODUI"

UInstaLODImposterizeTool::UInstaLODImposterizeTool() :
TargetMesh(nullptr),
Operation(nullptr),
AuxMesh(nullptr),
OperationResult()
{
	SuperSampling = EInstaLODSuperSampling::InstaLOD_None;
	MaterialSettings.BlendMode = BLEND_Masked;
	MaterialSettings.bOpacityMaskMap = true;
	
	bBakeTexturePageOpacity = true;
}

FText UInstaLODImposterizeTool::GetFriendlyName() const
{
	return NSLOCTEXT("InstaLODUI", "ImposterizeToolFriendlyName", "IM");
}

FText UInstaLODImposterizeTool::GetComboBoxItemName() const
{
	return NSLOCTEXT("InstaLODUI", "ImposterizeToolComboBoxItemName", "Imposterize");
}

FText UInstaLODImposterizeTool::GetOperationInformation() const
{
	return NSLOCTEXT("InstaLODUI", "ImposterizeToolOperationInformation", "The Imposterize operation generates a stand-in mesh for foliage rendering. Foliage meshes are typically not well suited for optimization techniques that would reduce the polygon count as that would ultimately lead to the removal of elements like leaves.\n\nThe operation supports the creation of a wide variety of different imposter types.");
}

int32 UInstaLODImposterizeTool::GetOrderId() const
{
	return 3;
}

void UInstaLODImposterizeTool::OnVistaImposter()
{
		UObject* SavedObject = nullptr;
		TArray<AActor*> SelectedStaticMeshActors = GetInstaLODWindow()->GetSelectedStaticMeshActors();
		TArray<TSharedPtr<FInstaLODMeshComponent>> InstaLODCameraComponents = GetInstaLODWindow()->GetEnabledSelectedCameraComponents();

		check(InstaLODCameraComponents.Num()==1);
		check(SelectedStaticMeshActors.Num()>=1);

		// Create a bounding box that encapsulates all the selected actors.
		const uint8 BoundingBoxVerticesNumber = 8u;
		const uint32 TotalNumberOfVertices = SelectedStaticMeshActors.Num() * BoundingBoxVerticesNumber;
		TArray<FVector> ActorsBoundingBoxVertices;
		ActorsBoundingBoxVertices.Reserve(TotalNumberOfVertices);
		FVector Vertices[BoundingBoxVerticesNumber];

		for (const AActor* const Actor : SelectedStaticMeshActors)
		{
			Actor->GetComponentsBoundingBox().GetVertices(Vertices);
			ActorsBoundingBoxVertices.Append(Vertices);
		}

		const FBox ActorsBoundingBox(ActorsBoundingBoxVertices);

		// Obtain camera information.
		TSharedPtr<FInstaLODMeshComponent> Component = InstaLODCameraComponents[0];
		check(Component->CameraComponent.IsValid());

		UCameraComponent *const CameraComponent = Component->CameraComponent.Get();
		const AActor* const CameraActor = CameraComponent->GetAttachParentActor();

		FMinimalViewInfo CameraViewInfo;
		CameraComponent->GetCameraView(0.0f, CameraViewInfo);
		const FRotator NormalizedRotation = CameraViewInfo.Rotation.GetNormalized();

		if (!FMath::IsNearlyZero(NormalizedRotation.Roll) && !FMath::IsNearlyEqual(NormalizedRotation.Roll, 180.0f))
		{
			FText Title = NSLOCTEXT("InstaLODUI", "OptimizeTool_Title", "Operation warning!");
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("InstaLODUI", "OperationWarning", "Rotating the camera around the forward axis (Roll), causes the result to be inaccurate."), &Title);
		}

		// Obtain and loop over the bounding box vertices. Calculate vertices distance from the camera.
		// Create an array of (index, vertex distance) pairs.
		FVector ActorBoundingBoxVertices[BoundingBoxVerticesNumber];
		ActorsBoundingBox.GetVertices(ActorBoundingBoxVertices);
		TArray<TPair<uint8, float>> VerticesDistanceFromCamera;
		const FVector NormalizedCameraObjectVector = (ActorsBoundingBox.GetCenter() - CameraViewInfo.Location).GetSafeNormal();

		FVector ForwardVector = CameraActor->GetActorForwardVector();
		if (!bParallelImposterPlane)
		{
			ForwardVector = NormalizedCameraObjectVector;
		}

		for (uint8 Index=0u; Index<BoundingBoxVerticesNumber; Index++)
		{
			const FVector VertexCameraVector = ActorBoundingBoxVertices[Index] - CameraViewInfo.Location;
			VerticesDistanceFromCamera.Add(TPair<uint8, float>(Index, VertexCameraVector.Dot(NormalizedCameraObjectVector)));
		}

		// Sort the array to get the neareast and the farthest vertex along the camera forward vector.
		VerticesDistanceFromCamera.Sort([](const TPair<uint8, float>& VertexDistance1, const TPair<uint8, float>& VertexDistance2) {
											return  VertexDistance1.Value < VertexDistance2.Value;});
		const FVector BoundingBoxNearestVertex = ActorBoundingBoxVertices[VerticesDistanceFromCamera[0].Key];
		const FVector BoundingBoxFarthestVertex = ActorBoundingBoxVertices[VerticesDistanceFromCamera.Last().Key];

		// Compute projection matrix.
		const FMatrix ProjectionMatrix = CameraViewInfo.CalculateProjectionMatrix();
		const FViewport* const Viewport = static_cast<UEditorEngine*>(GEditor)->GetActiveViewport();

		const FIntPoint ViewportSize = Viewport->GetSizeXY();
		const FIntRect ViewportRectangle(0, 0, ViewportSize.X, ViewportSize.Y);

		FSceneViewProjectionData ProjectionData;
		ProjectionData.SetConstrainedViewRectangle(ViewportRectangle);
		ProjectionData.ProjectionMatrix = ProjectionMatrix;
		ProjectionData.ViewOrigin = CameraViewInfo.Location;
		ProjectionData.ViewRotationMatrix = FInverseRotationMatrix(CameraViewInfo.Rotation) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));

		// Project the bounding box vertices into the screen space.
		FVector2d ProjectedBoundingBoxVertices[BoundingBoxVerticesNumber];
		const FMatrix ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();

		for (uint8 Index=0u; Index<BoundingBoxVerticesNumber; Index++)
		{
			FSceneView::ProjectWorldToScreen(ActorBoundingBoxVertices[Index], ViewportRectangle, ViewProjectionMatrix, ProjectedBoundingBoxVertices[Index]);
		}

		// Create a rectangle that encapsulates all the projected vertices.
		const FBox2d BoundingRectangleInScreenSpace(ProjectedBoundingBoxVertices, BoundingBoxVerticesNumber);
		FVector2D Extent, Center;
		BoundingRectangleInScreenSpace.GetCenterAndExtents(Center, Extent);
		const uint8 RectangleVerticesNumber = 4u;
		const FVector2D ScreenRectVector[RectangleVerticesNumber]{{Center-Extent}, {Center[0] + Extent[0], Center[1] - Extent[1]}, {Center[0] - Extent[0], Center[1] + Extent[1]}, {Center + Extent}};

		// Calculate the maximum, minimum, and desired plane distance from camera.
		const float DefaultPlaneMaximumDistanceFromCamera = (BoundingBoxFarthestVertex - CameraViewInfo.Location).Dot(ForwardVector);
		const float DefaultPlaneMinimumDistanceFromCamera = (BoundingBoxNearestVertex - CameraViewInfo.Location).Dot(ForwardVector);
		const float DesiredPlaneDistanceFromCamera = DefaultPlaneMinimumDistanceFromCamera + (DefaultPlaneMaximumDistanceFromCamera - DefaultPlaneMinimumDistanceFromCamera) * VistaDistance;

		// Create the default, and the desired imposter planes.
		// Note: the default imposter plane should be placed at the farthest bounding box vertex along the camrea forward vector.
		const FPlane IntersectionPlane(CameraViewInfo.Location + ForwardVector * DefaultPlaneMaximumDistanceFromCamera, ForwardVector);
		const FPlane DesiredIntersectionPlane(CameraViewInfo.Location + ForwardVector * DesiredPlaneDistanceFromCamera, ForwardVector);

		FVector DeprojectedVectorsOrigins[RectangleVerticesNumber];
		FVector DeprojectedVectorsDirections[RectangleVerticesNumber];
		FVector DeprojectedMinimumBoundingBoxVertices[RectangleVerticesNumber];
		FVector DeprojectedDesiredMinimumBoundingBoxVertices[RectangleVerticesNumber];

		// Deproject the vertices of the rectangle to world space.
		// Create rays and cast the rays into the planes to get the intersections.
		for (uint8 Index=0u; Index<RectangleVerticesNumber; Index++)
		{
			FSceneView::DeprojectScreenToWorld(ScreenRectVector[Index], ViewportRectangle, ViewProjectionMatrix.Inverse(), DeprojectedVectorsOrigins[Index], DeprojectedVectorsDirections[Index]);
			DeprojectedMinimumBoundingBoxVertices[Index] = FMath::RayPlaneIntersection(DeprojectedVectorsOrigins[Index], DeprojectedVectorsDirections[Index], IntersectionPlane);
			DeprojectedDesiredMinimumBoundingBoxVertices[Index] = FMath::RayPlaneIntersection(DeprojectedVectorsOrigins[Index], DeprojectedVectorsDirections[Index], DesiredIntersectionPlane);
		}

		// Calculate the width and the height of the default plane using the deprojected vertices.
		const FBox DeprojectedMinimumBoundingBox(DeprojectedMinimumBoundingBoxVertices, RectangleVerticesNumber);
		const float Width =  FVector::Dist(DeprojectedMinimumBoundingBoxVertices[0], DeprojectedMinimumBoundingBoxVertices[2]);
		const float Height = FVector::Dist(DeprojectedMinimumBoundingBoxVertices[0], DeprojectedMinimumBoundingBoxVertices[1]);
		const FVector DefaultPlaneCenter = DeprojectedMinimumBoundingBox.GetCenter();

		static const FString kInstaLODMeshImposterSuffix = TEXT("_imposter");
		FString SaveObjectPath = UInstaLODUtilities::OpenSaveDialog(NSLOCTEXT(LOCTEXT_NAMESPACE, "SaveNewMesh", "Save New Mesh"), GetDefaultPackageName() + kInstaLODMeshImposterSuffix, TEXT("SM_"));
		InstaLOD::IInstaLODMesh *const Plane = UInstaLODUtilities::CreatePlane(GetInstaLODInterface(), FVector2d(Width/2, Height/2));

		// Make the plane face the camera.
		// Note: A plane created with "UInstaLODUtilities::CreatePlane" is spawned facing the ground (- Z-Vector). 
		// To ensure that the front side of the plane faces the camera, we rotate the plane with 90 ° or -90° pitch angle.
		const FVector CameraHeadingNormal = CameraViewInfo.Rotation.Vector().GetSafeNormal();
		float PlaneRotation = 90.0f;
		const FVector FrontSide = -FVector::ZAxisVector;
		FRotator CameraRotation = CameraViewInfo.Rotation;
		CameraRotation.Add(PlaneRotation, 0.0f, 0.0f);
		const FVector PlaneVectorRotated = CameraRotation.RotateVector(FrontSide);

		if (FMath::Sign(FVector::VectorPlaneProject(CameraHeadingNormal, FVector::YAxisVector).Dot(FVector::VectorPlaneProject(PlaneVectorRotated, FVector::YAxisVector))) >= 0.0f)
		{
			PlaneRotation *= -1.0f;
		}

		// Assign the plane normals using the normalized vectors from the camera to the plane vertices.
		CameraRotation = ForwardVector.Rotation();
		const FRotator NormalVectorsRotation = CameraRotation.Add(PlaneRotation, 0, 0).GetInverse();

		InstaLOD::InstaVec3F *const  WedgeNormals = Plane->GetWedgeNormals(nullptr);
		FVector PlaneNormals[4u];

		PlaneNormals[0] = NormalVectorsRotation.RotateVector((CameraViewInfo.Location - DeprojectedMinimumBoundingBoxVertices[0])).GetSafeNormal();
		PlaneNormals[1] = NormalVectorsRotation.RotateVector((CameraViewInfo.Location - DeprojectedMinimumBoundingBoxVertices[1])).GetSafeNormal();
		PlaneNormals[2] = NormalVectorsRotation.RotateVector((CameraViewInfo.Location - DeprojectedMinimumBoundingBoxVertices[2])).GetSafeNormal();
		PlaneNormals[3] = NormalVectorsRotation.RotateVector((CameraViewInfo.Location - DeprojectedMinimumBoundingBoxVertices[3])).GetSafeNormal();

		const InstaLOD::InstaVec3F UpLeftVector (PlaneNormals[0].X, PlaneNormals[0].Y, PlaneNormals[0].Z);
		const InstaLOD::InstaVec3F UpRightVector (PlaneNormals[1].X, PlaneNormals[1].Y, PlaneNormals[1].Z);
		const InstaLOD::InstaVec3F DownLeftVector (PlaneNormals[2].X, PlaneNormals[2].Y, PlaneNormals[2].Z);
		const InstaLOD::InstaVec3F DownRightVector (PlaneNormals[3].X, PlaneNormals[3].Y, PlaneNormals[3].Z);

		WedgeNormals[0] = UpRightVector;
		WedgeNormals[1] = DownRightVector;
		WedgeNormals[2] = UpLeftVector;
		WedgeNormals[3] = UpLeftVector;
		WedgeNormals[4] = DownRightVector;
		WedgeNormals[5] = DownLeftVector;

		// Calculate the new scales using the ratio of the edges of the default and desired planes.
		const float FarPlaneFirstEdgeLength = FVector::Dist(DeprojectedMinimumBoundingBoxVertices[0], DeprojectedMinimumBoundingBoxVertices[1]);
		const float NearPlaneFirstEdgeLength = FVector::Dist(DeprojectedDesiredMinimumBoundingBoxVertices[0], DeprojectedDesiredMinimumBoundingBoxVertices[1]);

		const float FarPlaneSecondEdgeLength = FVector::Dist(DeprojectedMinimumBoundingBoxVertices[0], DeprojectedMinimumBoundingBoxVertices[2]);
		const float NearPlaneSecondEdgeLength = FVector::Dist(DeprojectedDesiredMinimumBoundingBoxVertices[0], DeprojectedDesiredMinimumBoundingBoxVertices[2]);

		if (FMath::IsNearlyZero(FarPlaneFirstEdgeLength) || FMath::IsNearlyZero(FarPlaneSecondEdgeLength))
		{
			OnMeshOperationError(NSLOCTEXT("InstaLODUI", "OperationFailed_NoObjectsHeightOrWidth", "The selected objects have no width, or height."));
			return;
		}

		const float FirstDimensionNewScale = NearPlaneFirstEdgeLength / FarPlaneFirstEdgeLength;
		const float SecondDimensionNewScale = NearPlaneSecondEdgeLength / FarPlaneSecondEdgeLength;
		VistaTransform.SetScale3D(FVector(FirstDimensionNewScale, FirstDimensionNewScale, SecondDimensionNewScale));

		// Calculate the new transform using the distance between the default plane and the desired plane along the planes centers vector.
		const FVector DesiredPlaneCenter = FBox(DeprojectedDesiredMinimumBoundingBoxVertices, RectangleVerticesNumber).GetCenter();
		const float DeltaPlanesDistance = DefaultPlaneMaximumDistanceFromCamera - DesiredPlaneDistanceFromCamera;
		VistaTransform.SetTranslation(DeltaPlanesDistance * (DesiredPlaneCenter - DefaultPlaneCenter).GetSafeNormal());

		// Handling edge case where the plane is shifted.
		const float FarthestVertexDistanceFromDeprojectionPlane = IntersectionPlane.PlaneDot(BoundingBoxFarthestVertex);

		if (FarthestVertexDistanceFromDeprojectionPlane  > UE_KINDA_SMALL_NUMBER || FarthestVertexDistanceFromDeprojectionPlane < -UE_KINDA_SMALL_NUMBER)
		{
			VistaTransform.SetTranslation((DeltaPlanesDistance + (FarPlaneSecondEdgeLength * FMath::Tan(FMath::DegreesToRadians(FMath::Max(CameraViewInfo.Rotation.Pitch, 0.0f))))) * (DesiredPlaneCenter - DefaultPlaneCenter).GetSafeNormal());
		}

		SavedObject = UInstaLODUtilities::SaveInstaLODMeshToStaticMeshAsset(GetInstaLODInterface(), Plane, SaveObjectPath);
		GetInstaLODInterface()->GetInstaLOD()->DeallocMesh(Plane);

		UWorld* const World = GetWorld();
		ULevel* const Level = World->GetCurrentLevel();
		FRotator ActorRotation(ForwardVector.Rotation());
		ActorRotation.Add(PlaneRotation, 0.0f, 0.0f);
		const FVector ActorTranslation(DefaultPlaneCenter);
		FActorSpawnParameters Params;
		Params.OverrideLevel = Level;
		Level->Modify();
		UStaticMesh *const StaticMesh = Cast<UStaticMesh>(SavedObject);

		AStaticMeshActor* const InstaLODImposterPlaneActor = World->SpawnActor<AStaticMeshActor>(ActorTranslation, ActorRotation, Params);
		InstaLODImposterPlaneActor->SetActorLabel(FPaths::GetBaseFilename(SaveObjectPath));
		InstaLODImposterPlaneActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
		TargetMesh = InstaLOD->AllocInstaLODMesh();
		UInstaLODUtilities::GetInstaLODMeshFromStaticMeshComponent(GetInstaLODInterface(), InstaLODImposterPlaneActor->GetStaticMeshComponent(), TargetMesh, /*BaseLODIndex:*/0, /*bWorldSpace:*/true);
		InstaLODImposterPlaneActor->Destroy();
}

void UInstaLODImposterizeTool::OnMeshOperationExecute(bool bIsAsynchronous)
{
	// --------------------------
	// can be run on child thread
	// --------------------------
	check(Operation == nullptr);
	InstaLOD::ImposterizeSettings ImposterizeSettings = GetImposterizeSettings();
	bool bIsVistaImposter = IsVistaImposter();
	if (bIsVistaImposter)
	{
		OnVistaImposter();
		if(TargetMesh != nullptr)
		{
			ImposterizeSettings.CustomGeometry = TargetMesh;
		}
	}
	static FScopedSlowTask* TaskProgress = nullptr;
	TaskProgress = SlowTaskProgress;
	static float LastProgress;
	LastProgress = 0.0f;

	InstaLOD::pfnImposterizeProgressCallback ProgressCallback  = [](class InstaLOD::IImposterizeOperation *, InstaLOD::IInstaLODMesh*, const float ProgressInPercent)
	{
		if (FMath::IsNearlyEqual(LastProgress, ProgressInPercent, UE_KINDA_SMALL_NUMBER) || ProgressInPercent < 0.0f)
			return;

		if (LastProgress >= 1.0f)
		{
			LastProgress = 0.0f;
			return;
		}

		const float DeltaProgress = (ProgressInPercent - LastProgress) * 100.0f;
		LastProgress = ProgressInPercent;

		if (!IsInGameThread())
		{
			AsyncTask(ENamedThreads::GameThread, [DeltaProgress]()
				{
					TaskProgress->EnterProgressFrame(DeltaProgress);
				});
		}
		else
		{
			TaskProgress->EnterProgressFrame(DeltaProgress);
		}
	};
	
	// alloc mesh operation
	Operation = GetInstaLODInterface()->GetInstaLOD()->AllocImposterizeOperation();
	Operation->SetProgressCallback(ProgressCallback);
	Operation->SetMaterialData(MaterialData);
	Operation->AddMesh(InputMesh);
	
	if (AuxMesh != nullptr)
	{
		Operation->AddCloudPolygonalMesh(AuxMesh);
		GetInstaLODInterface()->GetInstaLOD()->DeallocMesh(AuxMesh);
		AuxMesh = nullptr;
	}
	
	// execute
	OperationResult = Operation->Execute(OutputMesh, ImposterizeSettings);
}

bool UInstaLODImposterizeTool::IsMeshOperationSuccessful() const
{
	return Operation != nullptr && OperationResult.Success;
}

bool UInstaLODImposterizeTool::ReadSettingsFromJSONObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!UInstaLODBaseTool::IsValidJSONObject(JsonObject, "Imposterize"))
		return false;

	const TSharedPtr<FJsonObject>* SettingsObjectPointer = nullptr;

	if (!JsonObject->TryGetObjectField(FString("Settings"), SettingsObjectPointer) ||
		SettingsObjectPointer == nullptr ||
		!SettingsObjectPointer->IsValid())
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("InstaLOD: Could not retrieve Settings field."));
		return false;
	}

	const TSharedPtr<FJsonObject>& SettingsObject = *SettingsObjectPointer;

	if (SettingsObject->HasField("Type"))
	{
		const FString BillboardType = SettingsObject->GetStringField("Type");

		if (BillboardType == "Flipbook")
		{
			ImposterizeType = EInstaLODImposterizeType::InstaLOD_Flipbook;
		}
		else if (BillboardType == "Billboard")
		{
			ImposterizeType = EInstaLODImposterizeType::InstaLOD_Billboard;
		}
		else if (BillboardType == "AABB")
		{
			ImposterizeType = EInstaLODImposterizeType::InstaLOD_AABB;
		}
		else if (BillboardType == "HybridBillboardCloud")
		{
			ImposterizeType = EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud;
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Type '%s' not supported for key '%s'"), *BillboardType, TEXT("Type"));
		}
	}
	if (SettingsObject->HasField("QuadXYCount"))
	{
		XYAxisQuads_Billboard = SettingsObject->GetIntegerField("QuadXYCount");
	}
	if (SettingsObject->HasField("QuadXZCount"))
	{
		XZAxisQuads_Billboard = SettingsObject->GetIntegerField("QuadXZCount");
	}
	if (SettingsObject->HasField("QuadYZCount"))
	{
		YZAxisQuads_Billboard = SettingsObject->GetIntegerField("QuadYZCount");
	}
	if (SettingsObject->HasField("CloudFaceCount"))
	{
		MaximumFaceCount = SettingsObject->GetIntegerField("CloudFaceCount");
	}
	if (SettingsObject->HasField("CloudPolyFaceFactor"))
	{
		HybridMeshFaceFactor = (float) SettingsObject->GetNumberField("CloudPolyFaceFactor") * 100.0f;
	}
	if (SettingsObject->HasField("EnableCloudQuadTwoSided"))
	{
		bTwoSidedQuads_Hybrid = SettingsObject->GetBoolField("EnableCloudQuadTwoSided");
	}
	if (SettingsObject->HasField("EnableCloudQuadXY"))
	{
		bXYAxisQuads = SettingsObject->GetBoolField("EnableCloudQuadXY");
	}
	if (SettingsObject->HasField("EnableCloudQuadXZ"))
	{
		bXZAxisQuads = SettingsObject->GetBoolField("EnableCloudQuadXZ");
	}
	if (SettingsObject->HasField("EnableCloudQuadYZ"))
	{
		bYZAxisQuads = SettingsObject->GetBoolField("EnableCloudQuadYZ");
	}
	if (SettingsObject->HasField("GutterSizeInPixels"))
	{
		GutterSizeInPixels = SettingsObject->GetIntegerField("GutterSizeInPixels");
	}
	if (SettingsObject->HasField("EnableQuadTwoSided"))
	{
		bTwoSidedQuads_Billboard = SettingsObject->GetBoolField("EnableQuadTwoSided");
	}
	if (SettingsObject->HasField("FlipbookFramesPerAxis"))
	{
		FlipbookFramesPerAxis = SettingsObject->GetIntegerField("FlipbookFramesPerAxis");
	}
	if (SettingsObject->HasField("CloudNormalConformity"))
	{
		CloudNormalConformity = (float)SettingsObject->GetNumberField("CloudNormalConformity");
	}
	if (SettingsObject->HasField("CloudNormal"))
	{
		const FString CloudNormalString = SettingsObject->GetStringField("CloudNormal");
		TArray<FString> Tokens;
		
		if (CloudNormalString.ParseIntoArray(Tokens, TEXT(" ")) == 3)
		{
			CloudNormal.X = FCString::Atof(*Tokens[0]);
			CloudNormal.Y = FCString::Atof(*Tokens[1]);
			CloudNormal.Z = FCString::Atof(*Tokens[2]);
		}
	}
	if (SettingsObject->HasField("Deterministic"))
	{
		bDeterministic = SettingsObject->GetBoolField("Deterministic");
	}

	UInstaLODBakeBaseTool::ReadSettingsFromJSONObject(SettingsObject);

	return true;
}

bool UInstaLODImposterizeTool::CanAppendMeshToInput(FInstaLODMeshComponent& Component, InstaLOD::IInstaLODMesh *Mesh, TArray<UMaterialInterface*>* UniqueMaterials)
{
	if (UniqueMaterials == nullptr)
		return true;
	
	if (ImposterizeType != EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud)
		return true;
	
	if (HybridCloudPolyMaterialSuffix.IsEmpty())
		return true;
	
	uint64 FaceCount;
	int32 *FaceMaterials = Mesh->GetFaceMaterialIndices(&FaceCount);
	
	TArray<int32> CloudPolySections;
	TArray<int32> BillboardSections;
	
	for(uint64 FaceIndex=0; FaceIndex<FaceCount; FaceIndex++)
	{
		const int32 MaterialIndex = FaceMaterials[FaceIndex];
		
		check(UniqueMaterials->IsValidIndex(MaterialIndex));
		
		UMaterialInterface* Material = (*UniqueMaterials)[MaterialIndex];
		
		if (Material->GetName().EndsWith(HybridCloudPolyMaterialSuffix))
		{
			CloudPolySections.AddUnique(MaterialIndex);
		}
		else
		{
			BillboardSections.AddUnique(MaterialIndex);
		}
	}
	
	if (CloudPolySections.Num() > 0)
	{
		InstaLOD::IInstaLODMeshExtended *TempMesh = (InstaLOD::IInstaLODMeshExtended*)GetInstaLODInterface()->AllocInstaLODMesh();
		InstaLOD::IInstaLODMeshExtended *SubMesh = (InstaLOD::IInstaLODMeshExtended*)GetInstaLODInterface()->AllocInstaLODMesh();
		
		TempMesh->AppendMesh(Mesh);
		
		// copy the material IDs to the submesh IDs so we can extract by material ID
		TempMesh->ResizeFaceSubMeshIndices(FaceCount);
		
		int32 *TempFaceMaterials = TempMesh->GetFaceMaterialIndices(&FaceCount);
		uint32 *TempFaceSubmeshIndices = TempMesh->GetFaceSubMeshIndices(&FaceCount);
		for(uint64 FaceIndex=0; FaceIndex<FaceCount; FaceIndex++)
		{
			TempFaceSubmeshIndices[FaceIndex] = TempFaceMaterials[FaceIndex];
		}
		
		// ensure the aux mesh is allocated
		if (AuxMesh == nullptr)
			AuxMesh = (InstaLOD::IInstaLODMeshExtended*)GetInstaLODInterface()->AllocInstaLODMesh();
		
		for(int32 MaterialID : CloudPolySections)
		{
			TempMesh->ExtractSubMesh(MaterialID, SubMesh);
			AuxMesh->AppendMesh(SubMesh);
		}
		
		Mesh->Clear();
		for(int32 MaterialID : BillboardSections)
		{
			TempMesh->ExtractSubMesh(MaterialID, SubMesh);
			static_cast<InstaLOD::IInstaLODMeshExtended*>(Mesh)->AppendMesh(SubMesh);
		}
		
		GetInstaLODInterface()->GetInstaLOD()->DeallocMesh(TempMesh);
		GetInstaLODInterface()->GetInstaLOD()->DeallocMesh(SubMesh);
	}
	
	return true;
}

void UInstaLODImposterizeTool::DeallocMeshOperation()
{
	check(Operation);
	GetInstaLODInterface()->GetInstaLOD()->DeallocImposterizeOperation(Operation);
	Operation = nullptr;
}

bool UInstaLODImposterizeTool::IsMeshOperationExecutable(FText* OutErrorText) const
{
	if (GetInstaLODWindow() == nullptr)
		return false;
	
	if (!Super::IsMeshOperationExecutable(OutErrorText))
		return false;
	
	if (ImposterizeType == EInstaLODImposterizeType::InstaLOD_Billboard)
	{
		if (XYAxisQuads_Billboard == 0 && XZAxisQuads_Billboard == 0 && YZAxisQuads_Billboard == 0)
		{
			if (OutErrorText != nullptr)
			{
				*OutErrorText = NSLOCTEXT("InstaLODUI", "NoBillboards", "You need to enable at least one billboard quad axis to create a billboard imposter.");
			}
			return false;
		}
	}
	if (ImposterizeType == EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud)
	{
		if (!bXYAxisQuads && !bXZAxisQuads && !bYZAxisQuads)
		{
			if (OutErrorText != nullptr)
			{
				*OutErrorText = NSLOCTEXT("InstaLODUI", "NoBillboards", "You need to enable at least one billboard quad axis to create a billboard imposter.");
			}
			return false;
		}
	}

	if (ImposterizeType == EInstaLODImposterizeType::InstaLOD_Vista)
	{
		TArray<AActor*> SelectedStaticMeshActors = GetInstaLODWindow()->GetSelectedStaticMeshActors();
		TArray<TSharedPtr<FInstaLODMeshComponent>> InstaLODCameraComponents = GetInstaLODWindow()->GetEnabledSelectedCameraComponents();
		if ((SelectedStaticMeshActors.Num() < 1 && InstaLODCameraComponents.Num() != 1) || InstaLODCameraComponents.IsEmpty() || SelectedStaticMeshActors.IsEmpty())
		{
			if (OutErrorText != nullptr)
			{
				*OutErrorText = NSLOCTEXT("InstaLODUI", "InvalidSelection", "Please select a camera, and at least one actor.");
			}
			return false;
		}
	}

	return true;
}

InstaLOD::IInstaLODMaterial* UInstaLODImposterizeTool::GetBakeMaterial()
{
	if (!IsMeshOperationSuccessful())
		return nullptr;
	
	return OperationResult.BakeMaterial;
}

void UInstaLODImposterizeTool::FinalizeBakeMaterial(UMaterialInstanceConstant* Material) const
{
	check(Material);
	
	if (ImposterizeType != EInstaLODImposterizeType::InstaLOD_HybridBillboardCloud)
		return;
	
	// we always enable two sided rendering and foliage shading model for billboard cloud imposters
	Material->BasePropertyOverrides.TwoSided = true;
	Material->BasePropertyOverrides.bOverride_TwoSided = true;
	
	Material->BasePropertyOverrides.ShadingModel = MSM_TwoSidedFoliage;
	Material->BasePropertyOverrides.bOverride_ShadingModel = true;
	
	Material->PostEditChange();
}

void UInstaLODImposterizeTool::ResetSettings()
{
	ImposterizeType = EInstaLODImposterizeType::InstaLOD_Billboard;
	GutterSizeInPixels = 5;
	XYAxisQuads_Billboard = 0;
	XZAxisQuads_Billboard = 1;
	YZAxisQuads_Billboard = 1;
	bTwoSidedQuads_Billboard = true;
	SubdivisionsU = 1;
	SubdivisionsV = 1;
	AABB_Displacement = 0;
	VistaDistance = 1.0f;
	bParallelImposterPlane = true;
	MaximumFaceCount = 400;
	bXYAxisQuads = false;
	bXZAxisQuads = true;
	bYZAxisQuads = true;
	bTwoSidedQuads_Hybrid = true;
	CloudNormal = FVector(0.35f, 0.4f, 0.8f);
	CloudNormalConformity = 0.5f;
	HybridCloudPolyMaterialSuffix = "_cloudpoly";
	FlipbookFramesPerAxis = 8;
	HybridMeshFaceFactor = 50.0f;

	// Reset Parent which ultimately ends in a SaveConfig() call to reset everything
	Super::ResetSettings();
	
	SuperSampling = EInstaLODSuperSampling::InstaLOD_None;
	MaterialSettings.BlendMode = BLEND_Masked;
	MaterialSettings.bOpacityMaskMap = true;
	
	bBakeTexturePageOpacity = true;
	
	// HACK: we have to invoke save settings again as we change some of our parent's default fields
	SaveConfig();
}

InstaLOD::ImposterizeSettings UInstaLODImposterizeTool::GetImposterizeSettings()
{
	InstaLOD::ImposterizeSettings Settings;

	Settings.Type = (ImposterizeType != EInstaLODImposterizeType::InstaLOD_Vista? (InstaLOD::ImposterType::Type)ImposterizeType: InstaLOD::ImposterType::Type::CustomGeometry);
	Settings.FlipbookFramesPerAxis = FlipbookFramesPerAxis;
	Settings.CustomGeometry = nullptr;
	Settings.AABBDisplacement = AABB_Displacement;
	Settings.QuadXYCount = XYAxisQuads_Billboard;
	Settings.QuadXZCount = XZAxisQuads_Billboard;
	Settings.QuadYZCount = YZAxisQuads_Billboard;
	Settings.EnableQuadTwoSided = bTwoSidedQuads_Billboard;
	Settings.QuadSubdivisionsU = SubdivisionsU;
	Settings.QuadSubdivisionsV = SubdivisionsV;
	
	Settings.CloudFaceCount =  MaximumFaceCount;
	Settings.CloudPolyFaceFactor = HybridMeshFaceFactor / 100.0f;
 	Settings.EnableCloudQuadXY = bXYAxisQuads;
	Settings.EnableCloudQuadXZ = bXZAxisQuads;
	Settings.EnableCloudQuadYZ = bYZAxisQuads;
	Settings.EnableCloudQuadTwoSided = bTwoSidedQuads_Hybrid;
	Settings.CloudNormal = InstaLOD::InstaVec3F(CloudNormal.X, CloudNormal.Y, CloudNormal.Z);
	Settings.CloudNormalConformity = CloudNormalConformity;

	Settings.AlphaCutOut = bIsAlphaCutOutEnabled;
	Settings.AlphaCutOutSubdivide = bIsAlphaCutOutSubdivideEnabled;
	Settings.AlphaCutOutResolution = AlphaCutOutResolution;
	
	Settings.AlphaMaskThreshold = AlphaMaskThreshold;
 	Settings.GutterSizeInPixels = GutterSizeInPixels;

	Settings.BakeEngine = InstaLOD::BakeEngine::CPU;
	
	Settings.Deterministic = bDeterministic;
	
	Settings.BakeOutput = GetBakeOutputSettings();
	
	return Settings;
}

#undef LOCTEXT_NAMESPACE
