/**
 * InstaLOD.cpp (InstaLOD)
 *
 * Copyright 2016-2020 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLOD.cpp
 * @copyright 2016-2020 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "InstaLODMeshReductionPCH.h"
#include "InstaLOD/InstaLOD.h"

#include "Animation/AnimSequence.h"

#include "ComponentReregisterContext.h"

#include "InstaLOD/InstaLODAPI.h"

#include "Slate/InstaLODPluginStyle.h"
#include "InstaLODPluginCommands.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "MaterialUtilities.h"

#include "ClothingAsset.h"
#include "MeshBoneReduction.h"

#include "StaticMeshOperations.h"
#include "StaticMeshAttributes.h"
#include "MeshAttributes.h"
#include "MeshDescription.h"
#include "MeshDescriptionOperations.h"
#include "BonePose.h"

#include "RawMesh.h"
#include "LODUtilities.h"
#include "MeshMergeData.h" 

#include "StaticMeshEditorModule.h"
#include "Engine/SkeletalMeshEditorData.h"
#include "Animation/AnimSequenceHelpers.h"

#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "InstaLOD/InstaLODMeshExtended.h"

#define LOCTEXT_NAMESPACE "InstaLOD"

static TAutoConsoleVariable<int32> CVarOverrideSmoothingGroups(TEXT("InstaLOD.OverrideSmoothingGroups"), 1, TEXT("Overrides the smoothing groups of the source mesh. Use 0 to use smoothing groups of input mesh."));
static TAutoConsoleVariable<int32> CVarAlgorithmStrategy(TEXT("InstaLOD.AlgorithmStrategy"), InstaLOD::AlgorithmStrategy::Smart_v2, TEXT("The currently used algorithm strategy."));
static TAutoConsoleVariable<float> CVarDefaultMaximumError(TEXT("InstaLOD.DefaultMaximumError"), 0.0f, TEXT("Default maximum error. Active when per LOD 'Max Deviation' is set to 0."));
static TAutoConsoleVariable<float> CVarDecimatedErrorFactor(TEXT("InstaLOD.DecimatedErrorFactor"), 8.0f, TEXT("Deviation factor used for screen size calculation."));
static TAutoConsoleVariable<int32> CVarLockBoundaries(TEXT("InstaLOD.LockBoundaries"), 0, TEXT("Locks mesh boundary vertices in place."));
static TAutoConsoleVariable<int32> CVarLockSplits(TEXT("InstaLOD.LockSplits"), 0, TEXT("Locks split vertices in place."));
static TAutoConsoleVariable<int32> CVarOptimalPlacement(TEXT("InstaLOD.OptimalPlacement"), 1, TEXT("Enables optimal placement of vertices."));
static TAutoConsoleVariable<int32> CVarWeldingProtectDistinctUVShells(TEXT("InstaLOD.WeldingProtectDistinctUVShells"), 1, TEXT("Enables protection of distinct UV shells during weld operations."));
static TAutoConsoleVariable<int32> CVarWeightedNormals(TEXT("InstaLOD.WeightedNormals"), 1, TEXT("Enables weighted normals when recalculating normals."));
static TAutoConsoleVariable<int32> CVarForceOptimizerWeights(TEXT("InstaLOD.ForceOptimizerWeights"), 0, TEXT("Force enables optimizer vertex weights via vertex colors."));
static TAutoConsoleVariable<int32> CVarDeviationBasedScreenSize(TEXT("InstaLOD.DeviationBasedScreenSize"), 1, TEXT("Base the auto LOD screen size calculation on the mesh deviation."));
static TAutoConsoleVariable<int32> CVarOptimizeDeterministic(TEXT("InstaLOD.OptimizeDeterministic"), 0, TEXT("Makes the algorithm deterministic at the cost of speed."));
static TAutoConsoleVariable<int32> CVarProxyDeterministic(TEXT("InstaLOD.ProxyDeterministic"), 0, TEXT("Makes the algorithm deterministic at the cost of speed."));

static TAutoConsoleVariable<float> CVarHLODScreenSizeFactor(TEXT("InstaLOD.HLODScreenSizeFactor"), 1.0f, TEXT("Controls the screen size based maximum deviation calculation."));
static TAutoConsoleVariable<int32> CVarHLODRemesh(TEXT("InstaLOD.HLODRemesh"), 1, TEXT("Determines whether HLOD proxies use remeshing or mesh merging and optimize."));

static TAutoConsoleVariable<int32> CVarInstaDebug(TEXT("InstaLOD.Debug"), 0, TEXT("Internal debugging cvar."));
static TAutoConsoleVariable<FString> CVarWriteOBJ(TEXT("InstaLOD.WriteOBJ"), TEXT(""), TEXT("Write a OBJ file containing a representation of the optimized mesh to the path specified in the cvar."));

static TAutoConsoleVariable<int32> CVarAssertOnKeyMesh(TEXT("InstaLOD.AssertOnKeyMesh"), 0, TEXT("In case InstaLOD is not authorized while processing, InstaLOD for Unreal Engine will throw an error before adding the generated key mesh to the DDC."));
static TAutoConsoleVariable<int32> CVarShowAuthorizationWindow(TEXT("InstaLOD.ShowAuthorizationWindow"), 1, TEXT("In case InstaLOD is not authorized on startup, InstaLOD for Unreal Engine will not show up the authorization window when starting Unreal Engine.\n"));

static const FString InstaLODAssertOnKeyMeshMessage("InstaLOD not authorized, 'InstaLOD.AssertOnKeyMesh' is enabled, stopping execution before generating key meshes.");

namespace UEInstaLODMeshHelper
{
	static inline InstaLOD::InstaVec2F FVectorToInstaVec(const FVector2f& Vector)
	{
		InstaLOD::InstaVec2F OutVector;
		OutVector.X = Vector.X;
		OutVector.Y = Vector.Y;
		return OutVector;
	}
	
	static inline InstaLOD::InstaVec3F FVectorToInstaVec(const FVector3f& Vector)
	{
		InstaLOD::InstaVec3F OutVector;
		OutVector.X = Vector.X;
		OutVector.Y = Vector.Y;
		OutVector.Z = Vector.Z;
		return OutVector;
	}

	static inline InstaLOD::InstaVec3F FVectorToInstaVec(const FVector3d& Vector)
	{
		InstaLOD::InstaVec3F OutVector;
		OutVector.X = Vector.X;
		OutVector.Y = Vector.Y;
		OutVector.Z = Vector.Z;
		return OutVector;
	}

	static inline InstaLOD::InstaQuaternionF FQuatToInstaQuaternion(const FQuat& Quaternion)
	{
		InstaLOD::InstaQuaternionF OutQuaternion;
		OutQuaternion.W = Quaternion.W;
		OutQuaternion.X = Quaternion.X;
		OutQuaternion.Y = Quaternion.Y;
		OutQuaternion.Z = Quaternion.Z;
		return OutQuaternion;
	}
	
	static inline InstaLOD::InstaColorRGBAF32 FColorToInstaColorRGBAF32(const FColor& Color)
	{
		const FLinearColor LinearColor = Color.ReinterpretAsLinear();
		InstaLOD::InstaColorRGBAF32 OutColor;
		OutColor.R = LinearColor.R;
		OutColor.G = LinearColor.G;
		OutColor.B = LinearColor.B;
		OutColor.A = LinearColor.A;
		return OutColor;
	}
	
	static inline FVector2f InstaVecToFVector(const InstaLOD::InstaVec2F& Vector)
	{
		return FVector2f(Vector.X, Vector.Y);
	}
	
	static inline FVector3f InstaVecToFVector(const InstaLOD::InstaVec3F& Vector)
	{
		return FVector3f(Vector.X, Vector.Y, Vector.Z);
	}

	static inline FVector3d InstaVecToFVector3d(const InstaLOD::InstaVec3F& Vector)
	{
		return FVector3d(Vector.X, Vector.Y, Vector.Z);
	}

	static inline FColor InstaColorRGBAF32ToFColor(const InstaLOD::InstaColorRGBAF32& Color)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Color.A).ToFColor(false);
	}
	
	static inline void FillArray(InstaLOD::InstaVec3F *const OutData, const FVector3f *const InData, const size_t NumElements)
	{
		for(size_t Index=0; Index<NumElements; Index++)
		{
			OutData[Index].X = InData[Index].X;
			OutData[Index].Y = InData[Index].Y;
			OutData[Index].Z = InData[Index].Z;
		}
	}
	
	static inline void FillArray(InstaLOD::InstaVec2F *const OutData, const FVector2f *const InData, const size_t NumElements)
	{
		for(size_t Index =0; Index <NumElements; Index++)
		{
			OutData[Index].X = InData[Index].X;
			OutData[Index].Y = InData[Index].Y;
		}
	}


	static inline void FillArray(InstaLOD::InstaVec2F* const OutData, const FVector2d* const InData, const size_t NumElements)
	{
		for (size_t Index=0; Index<NumElements; Index++)
		{
			OutData[Index].X = InData[Index].X;
			OutData[Index].Y = InData[Index].Y;
		}
	}

	static inline void FillArray(InstaLOD::InstaColorRGBAF32 *const OutData, const FColor *const InData, const size_t NumElements)
	{
		for(size_t Index =0; Index <NumElements; Index++)
		{
			FLinearColor color = InData[Index].ReinterpretAsLinear();
			
			OutData[Index].R = color.R;
			OutData[Index].G = color.G;
			OutData[Index].B = color.B;
			OutData[Index].A = color.A;
		}
	}
	
	template<typename T>
	static inline void FillArray(T *const OutData, const T *const InData, const size_t NumElements)
	{
		for(size_t Index=0; Index<NumElements; Index++)
		{
			OutData[Index] = InData[Index];
		}
	}
	
	static inline void SanitizeFloatArray(float *const OutData, const size_t NumElements, const float InDefaultValue)
	{
		// NOTE: it is possible that UE sends invalid data to InstaLOD that contains NaNs or infinite numbers
		// in order to avoid invalid meshes we sanitze all float arrays by default
		for(size_t Index=0; Index<NumElements; Index++)
		{
			if (FMath::IsNaN(OutData[Index]) || !FMath::IsFinite(OutData[Index]))
			{
				UE_LOG(LogInstaLOD, Warning, TEXT("Sanitzing NaN/infinite value in data received from Unreal Engine."));
				OutData[Index] = InDefaultValue;
			}
		}
	}
	
	template<typename T>
	static inline void FillTArray(TArray<T>& OutArray, const T *const InData, const size_t NumElements)
	{
		OutArray.Reset();
		OutArray.AddUninitialized(NumElements);
		for(size_t Index=0; Index<NumElements; Index++)
		{
			OutArray[Index] = InData[Index];
		}
	}
	
	template<typename T, typename O, O CONVERTER(const T&) >
	static inline void FillTArrayWithConverter(TArray<O>& OutArray, const T *const InData, const size_t NumElements)
	{
		OutArray.Reset();
		OutArray.AddUninitialized(NumElements);
		
		for(size_t Index=0; Index<NumElements; Index++)
		{
			OutArray[Index] = CONVERTER(InData[Index]);
		}
	}

	static void InstaLODMeshToRawMesh(InstaLOD::IInstaLODMesh *const InstaMesh, FRawMesh& OutputMesh)
	{
		OutputMesh.Empty();
		
		InstaLOD::uint64 ElementCount;
		
		// per-vertex data
		InstaLOD::InstaVec3F *const Vertices = InstaMesh->GetVertexPositions(&ElementCount);
		UEInstaLODMeshHelper::FillTArrayWithConverter<InstaLOD::InstaVec3F, FVector3f, InstaVecToFVector>(OutputMesh.VertexPositions, Vertices, ElementCount);
		
		// per-wedge data
		InstaLOD::uint32 *const WedgeIndices = InstaMesh->GetWedgeIndices(&ElementCount);
		UEInstaLODMeshHelper::FillTArray<InstaLOD::uint32>(OutputMesh.WedgeIndices, WedgeIndices, ElementCount);
		
		InstaLOD::InstaVec3F *const WedgeTangents = InstaMesh->GetWedgeTangents(&ElementCount);
		UEInstaLODMeshHelper::FillTArrayWithConverter<InstaLOD::InstaVec3F, FVector3f, InstaVecToFVector>(OutputMesh.WedgeTangentX, WedgeTangents, ElementCount);
		InstaLOD::InstaVec3F *const WedgeBinormals = InstaMesh->GetWedgeBinormals(&ElementCount);
		UEInstaLODMeshHelper::FillTArrayWithConverter<InstaLOD::InstaVec3F, FVector3f, InstaVecToFVector>(OutputMesh.WedgeTangentY, WedgeBinormals, ElementCount);
		InstaLOD::InstaVec3F *const WedgeNormals = InstaMesh->GetWedgeNormals(&ElementCount);
		UEInstaLODMeshHelper::FillTArrayWithConverter<InstaLOD::InstaVec3F, FVector3f, InstaVecToFVector>(OutputMesh.WedgeTangentZ, WedgeNormals, ElementCount);
		
		InstaLOD::InstaColorRGBAF32 *const WedgeColors = InstaMesh->GetWedgeColors(0, &ElementCount);
		UEInstaLODMeshHelper::FillTArrayWithConverter<InstaLOD::InstaColorRGBAF32, FColor, InstaColorRGBAF32ToFColor>(OutputMesh.WedgeColors, WedgeColors, ElementCount);
		
		// texture coordinate sets
		const int32 TexCoordCount = FMath::Min<int>(InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS, MAX_MESH_TEXTURE_COORDS);
		for (size_t TexCoordSetIndex=0; TexCoordSetIndex<TexCoordCount; TexCoordSetIndex++)
		{
			InstaLOD::InstaVec2F *const TexCoords = InstaMesh->GetWedgeTexCoords(TexCoordSetIndex, &ElementCount);
			UEInstaLODMeshHelper::FillTArrayWithConverter<InstaLOD::InstaVec2F, FVector2f, InstaVecToFVector>(OutputMesh.WedgeTexCoords[TexCoordSetIndex], TexCoords, ElementCount);
		}
		
		// per-face data
		InstaLOD::InstaMaterialID *const FaceMaterialIndices = InstaMesh->GetFaceMaterialIndices(&ElementCount);
		UEInstaLODMeshHelper::FillTArray<InstaLOD::InstaMaterialID>(OutputMesh.FaceMaterialIndices, FaceMaterialIndices, ElementCount);
		
		InstaLOD::uint32 *const FaceSmoothingGroups = InstaMesh->GetFaceSmoothingGroups(&ElementCount);
		UEInstaLODMeshHelper::FillTArray<InstaLOD::uint32>(OutputMesh.FaceSmoothingMasks, FaceSmoothingGroups, ElementCount);
	}
	
	static void MeshDescriptionToInstaLODMesh(const FMeshDescription& SourceMeshDescription, const TMap<FName, int32>& MaterialMapIn, InstaLOD::IInstaLODMesh *const InstaMesh)
	{
		InstaMesh->Clear();
		
		const FStaticMeshConstAttributes SourceMeshAttributes(SourceMeshDescription);
		const TVertexAttributesConstRef<FVector3f> VertexPositions = SourceMeshAttributes.GetVertexPositions();
		const TVertexInstanceAttributesConstRef<FVector3f> VertexInstanceNormals = SourceMeshAttributes.GetVertexInstanceNormals();
		const TVertexInstanceAttributesConstRef<FVector3f> VertexInstanceTangents = SourceMeshAttributes.GetVertexInstanceTangents();
		const TVertexInstanceAttributesConstRef<float> VertexInstanceBinormalSigns = SourceMeshAttributes.GetVertexInstanceBinormalSigns();
		const TVertexInstanceAttributesConstRef<FVector4f> VertexInstanceColors = SourceMeshAttributes.GetVertexInstanceColors();
		const TVertexInstanceAttributesConstRef<FVector2f> VertexInstanceUVs = SourceMeshAttributes.GetVertexInstanceUVs();
		const TPolygonGroupAttributesConstRef<FName> PolygonGroupMaterialSlotName = SourceMeshAttributes.GetPolygonGroupMaterialSlotNames();
		
		// set vertex positions 
		InstaMesh->ResizeVertexPositions(SourceMeshDescription.Vertices().GetArraySize());
		InstaLOD::InstaVec3F *const OutVertexPositions = InstaMesh->GetVertexPositions(nullptr);

		// lookup table to get from UE VertexId to VertexIndex
		TMap<FVertexID, uint32> VertexIDToVertexIndex;
		VertexIDToVertexIndex.Reserve(SourceMeshDescription.Vertices().GetArraySize());

		uint32 VertexIndex = 0u;
		for (const FVertexID& VertexID : SourceMeshDescription.Vertices().GetElementIDs())
		{
			OutVertexPositions[VertexIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(VertexPositions[VertexID]);
			VertexIDToVertexIndex.Add(VertexID, VertexIndex);
			VertexIndex++;
		}

		const uint32 VertexCount = VertexIndex;
		const uint32 TriangleCount = SourceMeshDescription.Triangles().Num();
		const uint32 WedgeCount = TriangleCount * 3u;
		const FVector4f White(FLinearColor::White);

		// determine whether mesh has vertex colors
		bool bHasVertexColors = false;
		for (const FVertexInstanceID& VertexInstanceID : SourceMeshDescription.VertexInstances().GetElementIDs())
		{
			if (VertexInstanceColors[VertexInstanceID] != White)
			{
				bHasVertexColors = true;
				break;
			}
		}

		if (bHasVertexColors)
		{
			InstaMesh->ResizeWedgeColors(0u, WedgeCount);
		}

		// preallocate InstaLODMesh data
		InstaMesh->ResizeFaceMaterialIndices(TriangleCount);
		InstaMesh->ResizeFaceSmoothingGroups(TriangleCount);
		InstaMesh->ResizeWedgeIndices(WedgeCount);
		InstaMesh->ResizeWedgeTangents(WedgeCount);
		InstaMesh->ResizeWedgeBinormals(WedgeCount);
		InstaMesh->ResizeWedgeNormals(WedgeCount);

		const uint32 UVChannelCount = FMath::Clamp<int>(VertexInstanceUVs.GetNumChannels(), 0, InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS);

		TArray<InstaLOD::InstaVec2F*> WedgeUVChannels;
		for (uint32 UVChannelIndex=0u; UVChannelIndex<UVChannelCount; UVChannelIndex++)
		{
			InstaMesh->ResizeWedgeTexCoords(UVChannelIndex, WedgeCount);
			WedgeUVChannels.Add(InstaMesh->GetWedgeTexCoords(UVChannelIndex, nullptr));
		}

		// get handles to InstaLODMesh attributes
		InstaLOD::InstaMaterialID *const OutMaterialIndices = InstaMesh->GetFaceMaterialIndices(nullptr);
		InstaLOD::InstaColorRGBAF32 *const OutWedgeColors = InstaMesh->GetWedgeColors(0, nullptr);
		InstaLOD::InstaVec3F *const OutWedgeTangents = InstaMesh->GetWedgeTangents(nullptr);
		InstaLOD::InstaVec3F *const OutWedgeBinormals = InstaMesh->GetWedgeBinormals(nullptr);
		InstaLOD::InstaVec3F *const OutWedgeNormals = InstaMesh->GetWedgeNormals(nullptr);
		uint32 *const OutSmoothingGroups = InstaMesh->GetFaceSmoothingGroups(nullptr);
		uint32 *const OutWedgeIndices = InstaMesh->GetWedgeIndices(nullptr);

		// fill InstaLODMesh with data
		uint32 TriangleIndex = 0u;
		uint32 WedgeIndex = 0u;
		for (const FPolygonID PolygonID : SourceMeshDescription.Polygons().GetElementIDs())
		{
			const FPolygonGroupID& PolygonGroupID = SourceMeshDescription.GetPolygonPolygonGroup(PolygonID);
			const int32 PolygonIDValue = PolygonID.GetValue();
			const TArrayView<const FTriangleID>& TriangleIDs = SourceMeshDescription.GetPolygonTriangles(PolygonID);

			for (const FTriangleID TriangleID : TriangleIDs)
			{
				for (uint32 TriangleCorner=0u; TriangleCorner<3u; TriangleCorner++)
				{
					const FVertexInstanceID VertexInstanceID = SourceMeshDescription.GetTriangleVertexInstance(TriangleID, TriangleCorner);
					OutWedgeIndices[WedgeIndex] = VertexIDToVertexIndex[SourceMeshDescription.GetVertexInstanceVertex(VertexInstanceID)];
					OutWedgeNormals[WedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(VertexInstanceNormals[VertexInstanceID]);

					const FVector3f Tangent = VertexInstanceTangents[VertexInstanceID];
					OutWedgeTangents[WedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(Tangent);
					OutWedgeBinormals[WedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(FVector3f::CrossProduct(VertexInstanceNormals[VertexInstanceID], Tangent).GetSafeNormal() * VertexInstanceBinormalSigns[VertexInstanceID] * -1.0f);

					for (uint32 UVChannelIndex=0u; UVChannelIndex<UVChannelCount; UVChannelIndex++)
					{
						WedgeUVChannels[UVChannelIndex][WedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(VertexInstanceUVs.Get(VertexInstanceID, UVChannelIndex));
					}

					if (bHasVertexColors)
					{
						OutWedgeColors[WedgeIndex] = UEInstaLODMeshHelper::FColorToInstaColorRGBAF32(FLinearColor(VertexInstanceColors[VertexInstanceID]).ToFColor(true));
					}

					WedgeIndex++;
				}

				// set materialindex for triangle
				if (MaterialMapIn.Contains(PolygonGroupMaterialSlotName[PolygonGroupID]))
				{
					OutMaterialIndices[TriangleIndex] = MaterialMapIn[PolygonGroupMaterialSlotName[PolygonGroupID]];
				}
				else
				{
					OutMaterialIndices[TriangleIndex] = PolygonGroupID.GetValue();
				}

				// smoothing groups not supported set default value
				OutSmoothingGroups[TriangleIndex] = 0u;
				TriangleIndex++;
			}
		}

		// sanitize data
		{
			UEInstaLODMeshHelper::SanitizeFloatArray((float*)OutVertexPositions, VertexCount * 3u, 0.0f);
			UEInstaLODMeshHelper::SanitizeFloatArray((float*)OutWedgeNormals, WedgeCount * 3u, 0.0f);
			UEInstaLODMeshHelper::SanitizeFloatArray((float*)OutWedgeBinormals, WedgeCount * 3u, 0.0f);
			UEInstaLODMeshHelper::SanitizeFloatArray((float*)OutWedgeTangents, WedgeCount * 3u, 0.0f);

			for (uint32 UVChannelIndex=0u; UVChannelIndex<UVChannelCount; UVChannelIndex++)
			{
				UEInstaLODMeshHelper::SanitizeFloatArray((float*)WedgeUVChannels[UVChannelIndex], WedgeCount * 2u, 0.0f);
			}

			if (bHasVertexColors)
			{
				UEInstaLODMeshHelper::SanitizeFloatArray((float*)OutWedgeColors, WedgeCount * 4u, 0.0f);
			}
		}
		InstaMesh->ReverseFaceDirections(/*flipNormals:*/ false);
	}

	static void InstaLODMeshToMeshDescription(InstaLOD::IInstaLODMesh *const InstaMesh, const TMap<int32, FName>& MaterialMapOut, FMeshDescription& DestinationMeshDescription)
	{
		check(InstaMesh);
		DestinationMeshDescription.Empty();

		InstaMesh->ReverseFaceDirections(/*flipNormals:*/ false);
		
		uint64 WedgeIndexCount = 0u;
		uint64 VertexPositionCount = 0u;
		const uint32 *const WedgeIndices = InstaMesh->GetWedgeIndices(&WedgeIndexCount);
		const InstaLOD::InstaVec3F *const OutVertexPositions = InstaMesh->GetVertexPositions(&VertexPositionCount);
		const uint64 TriangleCount = WedgeIndexCount / 3u;

		// preallocate mesh description data
		DestinationMeshDescription.ReserveNewVertices(VertexPositionCount);
		DestinationMeshDescription.ReserveNewVertexInstances(WedgeIndexCount);
		DestinationMeshDescription.ReserveNewPolygons(TriangleCount);
		DestinationMeshDescription.ReserveNewEdges(TriangleCount*2.5f); // approx.
		TVertexAttributesRef<FVector3f> VertexPositions = DestinationMeshDescription.VertexAttributes().GetAttributesRef<FVector3f>(MeshAttribute::Vertex::Position);
		
		// set vertex positions and create map to get from vertex index to vertexID
		TMap<uint32, FVertexID> VertexIndexToVertexID;
		VertexIndexToVertexID.Reserve(VertexPositionCount);

		for (uint32 VertexIndex=0u; VertexIndex<VertexPositionCount; VertexIndex++)
		{
			const FVertexID VertexId = DestinationMeshDescription.CreateVertex();
			VertexPositions.Set(VertexId, InstaVecToFVector(OutVertexPositions[VertexIndex]));
			VertexIndexToVertexID.Add(VertexIndex, VertexId);
		}
		
		// Array references for attributes
		TAttributesSet<FVertexInstanceID>& InstanceAttributes = DestinationMeshDescription.VertexInstanceAttributes();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = InstanceAttributes.GetAttributesRef<FVector3f>(MeshAttribute::VertexInstance::Normal);
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = InstanceAttributes.GetAttributesRef<FVector3f>(MeshAttribute::VertexInstance::Tangent);
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = InstanceAttributes.GetAttributesRef<float>(MeshAttribute::VertexInstance::BinormalSign);
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = InstanceAttributes.GetAttributesRef<FVector4f>(MeshAttribute::VertexInstance::Color);
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = InstanceAttributes.GetAttributesRef<FVector2f>(MeshAttribute::VertexInstance::TextureCoordinate);
		TPolygonGroupAttributesRef<FName> PolygonGroupImportedMaterialSlotNames = DestinationMeshDescription.PolygonGroupAttributes().GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

		// clamp maximum texture coordinate channel to either InstaLOD max or unreal max
		const int32 MaxTexCoordChannels = MAX_MESH_TEXTURE_COORDS > InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS ? InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS : MAX_MESH_TEXTURE_COORDS;
		
		TArray<int32> TextureCoordinateRemapIndex;
		TextureCoordinateRemapIndex.AddZeroed(MaxTexCoordChannels);
		
		TArray<InstaLOD::InstaVec2F*> WedgeTextureCoordinates;
		WedgeTextureCoordinates.Reserve(MaxTexCoordChannels);

		// map texture coordinates
		int32 TexcoordsCount = 0;
		for (int32 TextureCoordinateIndex=0; TextureCoordinateIndex<MaxTexCoordChannels; TextureCoordinateIndex++)
		{
			TextureCoordinateRemapIndex[TextureCoordinateIndex] = INDEX_NONE;
			uint64 TexcoordCountInChannel = 0;
			WedgeTextureCoordinates.Add(InstaMesh->GetWedgeTexCoords((uint64) TextureCoordinateIndex, &TexcoordCountInChannel));
			
			if (TexcoordCountInChannel == WedgeIndexCount)
			{
				TextureCoordinateRemapIndex[TextureCoordinateIndex] = TexcoordsCount;
				TexcoordsCount++;
			}
		}

		VertexInstanceUVs.SetNumChannels(TexcoordsCount);
		
		// get wedge data from InstaLODMesh
		uint64 WedgeColorCount = 0u;
		uint64 WedgeTangentCount = 0u;
		uint64 WedgeBinormalCount = 0u;
		uint64 WedgeNormalCount = 0u;
		uint64 FaceMaterialIndexCount = 0u;
		uint64 SmoothingGroupCount = 0u;
		const InstaLOD::InstaVec3F *const WedgeTangents = InstaMesh->GetWedgeTangents(&WedgeTangentCount);
		const InstaLOD::InstaVec3F *const WedgeBinormals = InstaMesh->GetWedgeBinormals(&WedgeBinormalCount);
		const InstaLOD::InstaVec3F *const WedgeNormals = InstaMesh->GetWedgeNormals(&WedgeNormalCount);
		const InstaLOD::InstaMaterialID *const MaterialIndices = InstaMesh->GetFaceMaterialIndices(&FaceMaterialIndexCount);
		const InstaLOD::InstaColorRGBAF32 *const WedgeColors = InstaMesh->GetWedgeColors(0, &WedgeColorCount);
		const uint32 *const SmoothingGroups = InstaMesh->GetFaceSmoothingGroups(&SmoothingGroupCount);
		
		const bool bHasColors = WedgeColorCount > 0u;
		const bool bHasTangents = WedgeTangentCount > 0u && WedgeBinormalCount > 0u;
		const bool bHasNormals = WedgeNormalCount > 0u;
		
		// Materialindices are separated by polygongroups in Mesh Description
		uint32 PolygonGroupIDSource = 0u;
		TMap<int32, FPolygonGroupID> MaterialIndexToPolygonGroup;

		for (uint32 FaceIndex=0u; FaceIndex<FaceMaterialIndexCount; FaceIndex++)
		{
			const int32 MaterialIndex = MaterialIndices[FaceIndex];

			if (!MaterialIndexToPolygonGroup.Contains(MaterialIndex))
			{
				const FPolygonGroupID PolygonGroupID(MaterialIndex);
				DestinationMeshDescription.CreatePolygonGroupWithID(PolygonGroupID);

				// use material map if index is set
				if (MaterialMapOut.Contains(MaterialIndex))
				{
					PolygonGroupImportedMaterialSlotNames.Set(PolygonGroupID, MaterialMapOut[MaterialIndex]);
				}
				else
				{
					PolygonGroupImportedMaterialSlotNames.Set(PolygonGroupID, FName(*FString::Printf(TEXT("MaterialSlot_%d"), MaterialIndex)));
				}
				MaterialIndexToPolygonGroup.Add(MaterialIndex, PolygonGroupID);
			}
		}

		// calculate the binormal sign
		const auto fnCalculateBinormalSign = [](const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent) -> float
		{
			const FVector3f CrossTangent = FVector3f::CrossProduct(Binormal, Normal);
			return FVector3f::DotProduct(Tangent, CrossTangent) < 0 ? -1.0f : 1.0f;
		};

		// Triangles 
		FVertexID VertexIDs[3];
		for (uint32 TriangleIndex=0u; TriangleIndex<TriangleCount; TriangleIndex++)
		{
			const uint32 VertexFaceIndexBasis = TriangleIndex * 3u;

			// determine whether degenerates are present
			for (uint32 CornerIndex=0u; CornerIndex<3u; CornerIndex++)
			{
				const FVertexID VertexID = VertexIndexToVertexID[WedgeIndices[VertexFaceIndexBasis + CornerIndex]];
				VertexIDs[CornerIndex] = VertexID;
			}

			if (VertexIDs[0] == VertexIDs[1] || VertexIDs[0] == VertexIDs[2] || VertexIDs[1] == VertexIDs[2])
				continue;

			const int32 MaterialIndex = MaterialIndices[TriangleIndex];
			FPolygonGroupID PolygonGroupID = INDEX_NONE;
			FName PolygonGroupImportedMaterialSlotName = NAME_None;

			if (MaterialIndexToPolygonGroup.Contains(MaterialIndex))
			{
				PolygonGroupID = MaterialIndexToPolygonGroup[MaterialIndex];
				PolygonGroupImportedMaterialSlotName = PolygonGroupImportedMaterialSlotNames[PolygonGroupID];
			}
			else if (MaterialMapOut.Num() > 0 && MaterialMapOut.Contains(MaterialIndex))
			{
				PolygonGroupImportedMaterialSlotName = MaterialMapOut[MaterialIndex];

				for (const FPolygonGroupID& SearchPolygonGroupID : DestinationMeshDescription.PolygonGroups().GetElementIDs())
				{
					if (PolygonGroupImportedMaterialSlotNames[SearchPolygonGroupID] == PolygonGroupImportedMaterialSlotName)
					{
						PolygonGroupID = SearchPolygonGroupID;
						break;
					}
				}
			}

			if (PolygonGroupID == INDEX_NONE)
			{
				PolygonGroupID = DestinationMeshDescription.CreatePolygonGroup();
				PolygonGroupImportedMaterialSlotNames.Set(PolygonGroupID, PolygonGroupImportedMaterialSlotName == NAME_None ? FName(*FString::Printf(TEXT("MaterialSlot_%d"), MaterialIndex)) : PolygonGroupImportedMaterialSlotName);
				MaterialIndexToPolygonGroup.Add(MaterialIndex, PolygonGroupID);
			}

			TArray<FVertexInstanceID> TriangleVertexInstanceIDs;
			TriangleVertexInstanceIDs.SetNum(3);
			const FVector3f ZeroVector = FVector3f(ForceInitToZero);

			for (uint32 CornerIndex=0u; CornerIndex<3u; CornerIndex++)
			{
				const uint32 WedgeIndex = VertexFaceIndexBasis + CornerIndex;
				const uint32 VertexPositionIndex = WedgeIndices[WedgeIndex];
				const FVertexID VertexID = VertexIndexToVertexID[VertexPositionIndex];
				const FVertexInstanceID VertexInstanceID = DestinationMeshDescription.CreateVertexInstance(VertexID);
				TriangleVertexInstanceIDs[CornerIndex] = VertexInstanceID;

				FLinearColor Color = FLinearColor::White;
				FVector3f Normal = ZeroVector;
				FVector3f Tangent = ZeroVector;
				float Sign = 1.0f;

				if (bHasNormals)
				{
					Normal = UEInstaLODMeshHelper::InstaVecToFVector(WedgeNormals[WedgeIndex]);
				}
				VertexInstanceNormals[VertexInstanceID] = Normal;

				if (bHasTangents)
				{
					Tangent = UEInstaLODMeshHelper::InstaVecToFVector(WedgeTangents[WedgeIndex]);
				}
				VertexInstanceTangents[VertexInstanceID] = Tangent;

				if (bHasTangents && bHasNormals)
				{
					const FVector3f Binormal = UEInstaLODMeshHelper::InstaVecToFVector(WedgeBinormals[WedgeIndex]);
					Sign = fnCalculateBinormalSign(Normal, Binormal, Tangent);
				}
				VertexInstanceBinormalSigns[VertexInstanceID] = Sign;

				if (bHasColors)
				{
					Color = FLinearColor::FromSRGBColor(UEInstaLODMeshHelper::InstaColorRGBAF32ToFColor(WedgeColors[WedgeIndex]));
				}
				VertexInstanceColors[VertexInstanceID] = Color;

				for (int32 TextureCoordinateIndex=0; TextureCoordinateIndex<TexcoordsCount; TextureCoordinateIndex++)
				{
					const int32 TextureCoordinateIndexRemapped = TextureCoordinateRemapIndex[TextureCoordinateIndex];

					if (TextureCoordinateIndexRemapped == INDEX_NONE)
						continue;

					VertexInstanceUVs.Set(VertexInstanceID, TextureCoordinateIndexRemapped, UEInstaLODMeshHelper::InstaVecToFVector(WedgeTextureCoordinates[TextureCoordinateIndex][WedgeIndex]));
				}
			}
			
			TArray<FVertexInstanceID> TriangleVertexList;
			TriangleVertexList.Add(TriangleVertexInstanceIDs[0]);
			TriangleVertexList.Add(TriangleVertexInstanceIDs[1]);
			TriangleVertexList.Add(TriangleVertexInstanceIDs[2]);
			DestinationMeshDescription.CreateTriangle(PolygonGroupID, TriangleVertexList);
		}

		TArray<uint32> SmoothingGroupArray;
		SmoothingGroupArray.AddZeroed(TriangleCount);

		if (SmoothingGroupCount > 0u)
		{
			for (uint32 SmoothingGroupIndex=0u; SmoothingGroupIndex<SmoothingGroupCount; SmoothingGroupIndex++)
			{
				SmoothingGroupArray[SmoothingGroupIndex] = SmoothingGroups[SmoothingGroupIndex];
			}
		}

		FStaticMeshOperations::ConvertSmoothGroupToHardEdges(SmoothingGroupArray, DestinationMeshDescription);

		// If normals or tangents are missing let's unreal handle this
		if (!bHasNormals || !bHasTangents)
		{
			FStaticMeshOperations::ComputeTriangleTangentsAndNormals(DestinationMeshDescription, 0.0f);
			
			EComputeNTBsFlags NormalFlags = EComputeNTBsFlags::Tangents|EComputeNTBsFlags::UseMikkTSpace|EComputeNTBsFlags::BlendOverlappingNormals;
			if (!bHasNormals)
			{
				NormalFlags |= EComputeNTBsFlags::Normals;
			}
			FStaticMeshOperations::ComputeTangentsAndNormals(DestinationMeshDescription, NormalFlags);
		}
	}

	static InstaLOD::MeshFeatureImportance::Type GetInstaLODMeshFeatureImportance(EMeshFeatureImportance::Type Value)
	{
		switch(Value)
		{
			case EMeshFeatureImportance::Off:
				return InstaLOD::MeshFeatureImportance::Off;
			case EMeshFeatureImportance::Lowest:
				return InstaLOD::MeshFeatureImportance::Lowest;
			case EMeshFeatureImportance::Low:
				return InstaLOD::MeshFeatureImportance::Low;
			case EMeshFeatureImportance::Normal:
				return InstaLOD::MeshFeatureImportance::Normal;
			case EMeshFeatureImportance::High:
				return InstaLOD::MeshFeatureImportance::High;
			case EMeshFeatureImportance::Highest:
				return InstaLOD::MeshFeatureImportance::Highest;
			default:
				return InstaLOD::MeshFeatureImportance::Normal;
		}
	}
	
	static InstaLOD::OptimizeSettings ConvertMeshReductionSettingsToInstaLOD(const FMeshReductionSettings& Settings)
	{
		InstaLOD::OptimizeSettings OptimizeSettings;
		OptimizeSettings.ScreenSizeInPixels = 0.0f;
		OptimizeSettings.PercentTriangles = Settings.PercentTriangles;
		OptimizeSettings.AlgorithmStrategy = (InstaLOD::AlgorithmStrategy::Type) CVarAlgorithmStrategy.GetValueOnAnyThread();
		OptimizeSettings.HardAngleThreshold = Settings.HardAngleThreshold;
		OptimizeSettings.MaxDeviation = Settings.MaxDeviation > 0.0f ? Settings.MaxDeviation : CVarDefaultMaximumError.GetValueOnAnyThread();
		OptimizeSettings.ShadingImportance = GetInstaLODMeshFeatureImportance(Settings.ShadingImportance);
		OptimizeSettings.TextureImportance = GetInstaLODMeshFeatureImportance(Settings.TextureImportance);
		OptimizeSettings.SilhouetteImportance = GetInstaLODMeshFeatureImportance(Settings.SilhouetteImportance == EMeshFeatureImportance::High ? EMeshFeatureImportance::Normal :
																				 (EMeshFeatureImportance::Type)Settings.SilhouetteImportance);
		OptimizeSettings.WeldingThreshold = Settings.WeldingThreshold;
		OptimizeSettings.LockSplits = CVarLockSplits.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.LockBoundaries = CVarLockBoundaries.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.RecalculateNormals = Settings.bRecalculateNormals;
		OptimizeSettings.WeightedNormals = CVarWeightedNormals.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.WeldingProtectDistinctUVShells = CVarWeldingProtectDistinctUVShells.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.OptimalPlacement = CVarOptimalPlacement.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.OptimizerVertexWeights = CVarForceOptimizerWeights.GetValueOnAnyThread() > 0 || Settings.SilhouetteImportance >= EMeshFeatureImportance::High ? true : false;
		OptimizeSettings.Deterministic = CVarOptimizeDeterministic.GetValueOnAnyThread() > 0 ? true : false;
		return OptimizeSettings;
	}

	static void FillNamedMaterialMap(TMap<int32, FName> &OutMaterialMap, const TArray<FStaticMaterial>& StaticMaterials)
	{
		OutMaterialMap.Empty(StaticMaterials.Num());

		for (int32 MaterialIndex=0; MaterialIndex<StaticMaterials.Num(); MaterialIndex++)
		{
			FName MaterialName = StaticMaterials[MaterialIndex].ImportedMaterialSlotName;
			if (MaterialName == NAME_None)
			{
				MaterialName = *(TEXT("MaterialSlot_") + FString::FromInt(MaterialIndex));
			}
			OutMaterialMap.Add(MaterialIndex, MaterialName);
		}
	}

	static void CreateInputOutputMaterialMapFromMeshDescription(const FMeshDescription& InMesh, TMap<FName, int32>& InMaterialMap, TMap<int32, FName>& OutMaterialMap)
	{
		TPolygonGroupAttributesConstRef<FName> PolygonGroupMaterialSlotName = InMesh.PolygonGroupAttributes().GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

		for (const FPolygonGroupID& PolygonGroupID : InMesh.PolygonGroups().GetElementIDs())
		{
			const FName& Name = PolygonGroupMaterialSlotName[PolygonGroupID];
			
			InMaterialMap.Add(Name, PolygonGroupID.GetValue());
			OutMaterialMap.Add(PolygonGroupID.GetValue(), Name);
		}
	}

	static class ITargetPlatform* GetRunningTargetPlatform()
	{
		class ITargetPlatform* TargetPlatform = nullptr;

		if (ITargetPlatformManagerModule* const PlatformManager = FModuleManager::LoadModulePtr<ITargetPlatformManagerModule>("TargetPlatform"))
		{
			TargetPlatform = PlatformManager->GetRunningTargetPlatform();
		}
		else
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Could not retrieve Target Platform Manager."));
		}

		return TargetPlatform;
	}
};

namespace UEInstaLODSkeletalMeshHelper
{
	using UE_StaticLODModel = IInstaLOD::UE_StaticLODModel;

	static TArray<uint32> GetSkipSectionsForLODModel(const FSkeletalMeshLODModel *const SourceLODModel, const int32 LODIndex = -1)
	{
		if (SourceLODModel == nullptr)
			return TArray<uint32>();

		TArray<uint32> SkipSections;

		for (uint32 SectionIndex=0u; SectionIndex<(uint32)SourceLODModel->Sections.Num(); SectionIndex++)
		{
			const FSkelMeshSection& Section = SourceLODModel->Sections[SectionIndex];

			if (Section.bDisabled)
			{
				SkipSections.Add(SectionIndex);
				continue;
			}

			if (LODIndex != -1)
			{
				const int32 LODEnabledIndex = Section.GenerateUpToLodIndex;

				if (LODEnabledIndex != -1 && LODEnabledIndex < LODIndex)
				{
					SkipSections.Add(SectionIndex);
				}
			}
		}
		return SkipSections;
	}
		
	static void SkeletalLODModelToInstaLODMesh(const UE_StaticLODModel& SourceLODModel, InstaLOD::IInstaLODMesh *const InstaMesh, TArray<uint32>& SkipSections, UE_SkeletalBakePoseData *const BakePoseData = nullptr, const int32 LODIndex = -1)
	{
		using MatrixType = UE::Math::TMatrix<float>;

		// build buffers
		TArray<FSoftSkinVertex> Vertices;
		SourceLODModel.GetVertices(Vertices);

		// create a list of all valid section indices from the section indices which should be skipped
		TArray<uint32> ValidSectionIndices;
		for (uint32 SectionIndex=0u; SectionIndex<(uint32) SourceLODModel.Sections.Num(); SectionIndex++)
		{
			if (SkipSections.Contains(SectionIndex))
				continue;

			ValidSectionIndices.Add(SectionIndex);
		}

		uint32 VertexCount = 0u;
		uint32 TriangleCount = 0u;

		// collect total triangle and vertex count from valid sections
		for (uint32 SectionIndex : ValidSectionIndices)
		{
			const FSkelMeshSection& Section = SourceLODModel.Sections[SectionIndex];

			VertexCount += Section.NumVertices;
			TriangleCount += Section.NumTriangles;
		}

		InstaMesh->Clear();

		// setup per-vertex data buffers
		InstaMesh->ResizeVertexPositions(VertexCount);
		InstaMesh->GetSkinnedVertexData().Initialize(VertexCount, MAX_TOTAL_INFLUENCES);

		// setup per-wedge data buffers
		const uint32 WedgeCount = TriangleCount * 3u;
		InstaMesh->ResizeWedgeIndices(WedgeCount);
		InstaMesh->ResizeWedgeTangents(WedgeCount);
		InstaMesh->ResizeWedgeBinormals(WedgeCount);
		InstaMesh->ResizeWedgeNormals(WedgeCount);

		// setup per-face data buffers
		InstaMesh->ResizeFaceSmoothingGroups(TriangleCount);
		InstaMesh->ResizeFaceMaterialIndices(TriangleCount);

		// setup texcoords
		const int32 TexcoordCount = FMath::Min<int32>(InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS, SourceLODModel.NumTexCoords);
		for (int32 TexcoordIndex=0; TexcoordIndex<TexcoordCount; TexcoordIndex++)
		{
			InstaMesh->ResizeWedgeTexCoords(TexcoordIndex, WedgeCount);
		}

		// is color data available?
		InstaMesh->ResizeWedgeColors(0, WedgeCount);

		if (BakePoseData != nullptr &&
			BakePoseData->BakePoseAnimation != nullptr &&
			BakePoseData->ReferenceSkeleton != nullptr &&
			BakePoseData->SkeletalMesh != nullptr)
		{
			FMemMark Mark(FMemStack::Get());

			const int32 NumBones=BakePoseData->SkeletalMesh->GetRefSkeleton().GetNum();

#define INSTALOD_ENABLE_RETARGET_BONEPOSE_ANIMATION
#if defined (INSTALOD_ENABLE_RETARGET_BONEPOSE_ANIMATION)

			TArray<FTransform> ComponentSpaceRefPose;
			FAnimationRuntime::FillUpComponentSpaceTransformsRetargetBasePose(BakePoseData->SkeletalMesh, ComponentSpaceRefPose);
			TMap<FBoneIndexType, FBoneIndexType> BonesToRemove;
			IMeshBoneReductionModule* const MeshBoneReductionModule=FModuleManager::Get().LoadModulePtr<IMeshBoneReductionModule>("MeshBoneReduction");

			if (LODIndex != -1 && MeshBoneReductionModule != nullptr)
			{
				if (IMeshBoneReduction* const MeshBoneReductionInterface=MeshBoneReductionModule->GetMeshBoneReductionInterface())
				{
					MeshBoneReductionInterface->GetBoneReductionData(BakePoseData->SkeletalMesh, LODIndex, BonesToRemove);
				}
			}

			TArray<FMatrix> RelativeToRefPoseMatrices;
			RelativeToRefPoseMatrices.AddDefaulted(NumBones);

			// Set initial matrices to identity
			for (int32 Index=0; Index < NumBones; ++Index)
			{
				RelativeToRefPoseMatrices[Index]= FMatrix::Identity;
			}

			// Setup BoneContainer and CompactPose
			TArray<FBoneIndexType> RequiredBoneIndexArray;
			RequiredBoneIndexArray.AddUninitialized(BakePoseData->SkeletalMesh->GetRefSkeleton().GetNum());
			for (int32 BoneIndex=0; BoneIndex < RequiredBoneIndexArray.Num(); ++BoneIndex)
			{
				RequiredBoneIndexArray[BoneIndex]=BoneIndex;
			}
			
			FBoneContainer RequiredBones(RequiredBoneIndexArray, false, *(BakePoseData->SkeletalMesh));
			RequiredBones.SetUseRAWData(true);

			FCompactPose Pose;
			Pose.SetBoneContainer(&RequiredBones);
			Pose.ResetToRefPose();

			USkeletalMesh *const SkeletalMesh = BakePoseData->SkeletalMesh;
			const USkeleton *const Skeleton = SkeletalMesh->GetSkeleton();
			const FName RetargetSourceName = Skeleton->GetRetargetSourceForMesh(SkeletalMesh);

			FBlendedCurve Curve;
			Curve.InitFrom(RequiredBones);
			UE::Anim::FStackAttributeContainer Attributes;
			FAnimationPoseData AnimationPoseData(Pose, Curve, Attributes);

			UE::Anim::BuildPoseFromModel(BakePoseData->BakePoseAnimation->GetDataModel(), AnimationPoseData, 0.0, EAnimInterpolationType::Step, RetargetSourceName, Skeleton->GetRefLocalPoses(RetargetSourceName));
			
			Pose.NormalizeRotations();

			TArray<FMatrix> ComponentSpaceAnimatedPose;
			ComponentSpaceAnimatedPose.AddDefaulted(NumBones);

			for (int32 BoneIndex=0; BoneIndex<NumBones; ++BoneIndex)
			{
				const FCompactPoseBoneIndex PoseBoneIndex(BoneIndex);
				const int32 ParentIndex = BakePoseData->SkeletalMesh->GetRefSkeleton().GetParentIndex(BoneIndex);
				if (ParentIndex != INDEX_NONE)
				{
					// If the bone will be removed, get the local-space retarget-ed animation bone transform
					if (BonesToRemove.Contains(BoneIndex))
					{
						ComponentSpaceAnimatedPose[BoneIndex]=Pose[PoseBoneIndex].ToMatrixWithScale() * ComponentSpaceAnimatedPose[ParentIndex];
					}
					else
					{
						// Otherwise use the component-space retarget base pose transform
						ComponentSpaceAnimatedPose[BoneIndex]=ComponentSpaceRefPose[BoneIndex].ToMatrixWithScale();
					}
				}
				else
				{
					// If the bone will be removed, get the retarget-ed animation bone transform
					if (BonesToRemove.Contains(BoneIndex))
					{
						ComponentSpaceAnimatedPose[BoneIndex]=Pose[PoseBoneIndex].ToMatrixWithScale();
					}
					else
					{
						// Otherwise use the retarget base pose transform
						ComponentSpaceAnimatedPose[BoneIndex]=ComponentSpaceRefPose[BoneIndex].ToMatrixWithScale();
					}
				}
			}

			// Calculate relative to retarget base (ref) pose matrix
			for (int32 BoneIndex=0; BoneIndex<NumBones; ++BoneIndex)
			{
				RelativeToRefPoseMatrices[BoneIndex] = ComponentSpaceRefPose[BoneIndex].ToMatrixWithScale().Inverse() * ComponentSpaceAnimatedPose[BoneIndex];
			}
#else
			const FReferenceSkeleton& ReferenceSkeleton = *BakePoseData->ReferenceSkeleton;
			TArray<FName> BoneNames;
			BoneNames.Reserve(NumBones);
			for (int32 CurrentBoneIndex=0; CurrentBoneIndex<NumBones; CurrentBoneIndex++)
			{
				BoneNames.Add(ReferenceSkeleton.GetBoneName(CurrentBoneIndex));
			}

			TArray<UE::Math::TTransform<float>> BonePoses;
			UAnimationBlueprintLibrary::GetBonePosesForFrame(BakePoseData->BakePoseAnimation, BoneNames, 0, true, BonePoses, BakePoseData->SkeletalMesh);

			const TArray<FTransform>& ReferencePoseInLocalSpace = ReferenceSkeleton.GetRefBonePose();

			// Collect transformation from bind pose and animation pose
			// and apply the animation pose to the vertices
			TArray<FTransform> ReferencePoseInComponentSpace;
			ReferencePoseInComponentSpace.AddUninitialized(ReferenceSkeleton.GetNum());

			for (int32 CurrentBoneIndex=0; CurrentBoneIndex<ReferencePoseInLocalSpace.Num(); CurrentBoneIndex++)
			{
				ReferencePoseInComponentSpace[CurrentBoneIndex] = FTransform::Identity;
			}

			// bind pose
			for (int32 CurrentBoneIndex=0; CurrentBoneIndex<NumBones; CurrentBoneIndex++)
			{
				const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(CurrentBoneIndex);

				if (ParentIndex != INDEX_NONE)
				{
					ReferencePoseInComponentSpace[CurrentBoneIndex] = ReferencePoseInLocalSpace[CurrentBoneIndex] * ReferencePoseInComponentSpace[ParentIndex];
				}
				else
				{
					ReferencePoseInComponentSpace[CurrentBoneIndex] = ReferencePoseInLocalSpace[CurrentBoneIndex];
				}
			}

			TArray<MatrixType> ComponentSpacePose;
			TArray<MatrixType> ComponentSpaceReferencePose;
			TArray<MatrixType> AnimationPoseMatrices;
			ComponentSpacePose.AddUninitialized(ReferenceSkeleton.GetNum());
			ComponentSpaceReferencePose.AddUninitialized(ReferenceSkeleton.GetNum());
			AnimationPoseMatrices.AddUninitialized(ReferenceSkeleton.GetNum());

			for (int32 CurrentBoneIndex=0; CurrentBoneIndex<ReferenceSkeleton.GetNum(); CurrentBoneIndex++)
			{
				ComponentSpaceReferencePose[CurrentBoneIndex] = ReferencePoseInComponentSpace[CurrentBoneIndex].ToMatrixWithScale();
				AnimationPoseMatrices[CurrentBoneIndex] = BonePoses[CurrentBoneIndex].ToMatrixWithScale();
			}

			// animation pose
			for (int32 CurrentBoneIndex=0; CurrentBoneIndex<NumBones; CurrentBoneIndex++)
			{
				const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(CurrentBoneIndex);
				if (ParentIndex != INDEX_NONE)
				{
					ComponentSpacePose[CurrentBoneIndex] = AnimationPoseMatrices[CurrentBoneIndex] * ComponentSpacePose[ParentIndex];
				}
				else
				{
					ComponentSpacePose[CurrentBoneIndex] = AnimationPoseMatrices[CurrentBoneIndex];
				}
			}

			// calculate relative to reference pose from animation pose
			TArray<MatrixType> RelativeToRefPoseMatrices;
			RelativeToRefPoseMatrices.AddUninitialized(NumBones);

			for (int32 BoneIndex=0; BoneIndex<NumBones; BoneIndex++)
			{
				RelativeToRefPoseMatrices[BoneIndex] = ComponentSpaceReferencePose[BoneIndex].Inverse() * ComponentSpacePose[BoneIndex];
			}
#endif

			TArray<float> BoneInfluences;
			BoneInfluences.Reserve(MAX_TOTAL_INFLUENCES);
			TArray<uint8> BoneIndices;
			BoneIndices.Reserve(MAX_TOTAL_INFLUENCES);

			for (const uint32 SectionIndex : ValidSectionIndices)
			{
				const FSkelMeshSection& Section = SourceLODModel.Sections[SectionIndex];
				const uint32 EndVertexIndex = Section.BaseVertexIndex + Section.NumVertices;

				for (uint32 VertexIndex=Section.BaseVertexIndex; VertexIndex<EndVertexIndex; VertexIndex++)
				{
					FSoftSkinVertex& Vertex = Vertices[VertexIndex];
					int32 TotalInfluence=0;

					// accumulate influences
					for (const uint8 Influence : Vertex.InfluenceWeights)
					{
						BoneInfluences.Add(Influence);
						TotalInfluence += Influence;
					}

					if (TotalInfluence != 255)
					{
						UE_LOG(LogInstaLOD, Warning, TEXT("Skeletal mesh contains invalid weight distribution, please consider renormalizing weights."))
					}

					for (const uint8 boneIndex : Vertex.InfluenceBones)
					{
						BoneIndices.Add(boneIndex);
					}

					// Calculate blended matrix
					FMatrix BlendedMatrix(ForceInitToZero);

					if (TotalInfluence > 0)
					{
						for (int32 Index=0; Index<MAX_TOTAL_INFLUENCES; Index++)
						{
							const uint8 CurrentBoneIndex = Section.BoneMap[BoneIndices[Index]];
							const float CurrentBoneInfluence = BoneInfluences[Index];

							if (CurrentBoneInfluence > 0.0f)
							{
								BlendedMatrix += RelativeToRefPoseMatrices[CurrentBoneIndex] * (CurrentBoneInfluence/(float) TotalInfluence);
							}
						}
					}

					const auto fnVector3dToVector3f = [](const FVector3d& InVector)
					{
						return FVector3f(InVector.X, InVector.Y, InVector.Z);
					};

					// apply transformation to vertex
					const FVector3f OldPosition = Vertex.Position;
					const FVector3f OldTangentX = Vertex.TangentX;
					const FVector3f OldTangentY = Vertex.TangentY;
					const FVector3f OldTangentZ = Vertex.TangentZ;

					const FVector3d Position = BlendedMatrix.TransformPosition(FVector3d(Vertex.Position));
					Vertex.Position = fnVector3dToVector3f(Position);
					const FVector3d WeightedTangentX = BlendedMatrix.TransformVector(FVector3d(Vertex.TangentX));
					const FVector3d WeightedTangentY = BlendedMatrix.TransformVector(FVector3d(Vertex.TangentY));
					const FVector3d WeightedTangentZ = BlendedMatrix.TransformVector(FVector3d(Vertex.TangentZ));

					const uint8 WComponent = FMath::Clamp(Vertex.TangentZ.W, 0, 255);
					Vertex.TangentX = fnVector3dToVector3f(WeightedTangentX.GetSafeNormal());
					Vertex.TangentY = fnVector3dToVector3f(WeightedTangentY.GetSafeNormal());
					Vertex.TangentZ = fnVector3dToVector3f(WeightedTangentZ.GetSafeNormal());
					Vertex.TangentZ.W = WComponent;

					if (Vertex.Position.ContainsNaN())
					{
						UE_LOG(LogInstaLOD, Warning, TEXT("Skeletal mesh Bake Pose calculation has invalid values."));
						Vertex.Position = OldPosition;
						Vertex.TangentX = OldTangentX;
						Vertex.TangentY = OldTangentY;
						Vertex.TangentZ = OldTangentZ;
					}

					BoneInfluences.Reset();
					BoneIndices.Reset();
				}
			}
		}

		// setup bone and vertex data 
		InstaLOD::InstaVec3F *const OutVertexPositions = InstaMesh->GetVertexPositions(nullptr);

		// index for InstaLOD vertex buffer
		uint32 OutVertexIndex = 0u;
		for (const uint32 ValidSectionIndex : ValidSectionIndices)
		{
			const FSkelMeshSection& Section = SourceLODModel.Sections[ValidSectionIndex];
			const uint32 EndVertexIndex = Section.BaseVertexIndex + Section.NumVertices;

			for (uint32 VertexIndex=Section.BaseVertexIndex; VertexIndex<EndVertexIndex && VertexIndex < (uint32) Vertices.Num(); VertexIndex++, OutVertexIndex++)
			{
				const FSoftSkinVertex& Vertex = Vertices[VertexIndex];
				OutVertexPositions[OutVertexIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(Vertex.Position);

				// setup weights for this vertex
				InstaLOD::InstaLODSkeletalMeshBoneData* const OutBoneData = InstaMesh->GetSkinnedVertexData().GetBoneDataForVertex(OutVertexIndex);

				for (uint32 BoneIndex=0u; BoneIndex<MAX_TOTAL_INFLUENCES; BoneIndex++)
				{
					if (Vertex.InfluenceWeights[BoneIndex] == 0)
					{
						OutBoneData[BoneIndex] = InstaLOD::InstaLODSkeletalMeshBoneData();
						continue;
					}
					OutBoneData[BoneIndex].BoneIndex = Section.BoneMap[Vertex.InfluenceBones[BoneIndex]];
					OutBoneData[BoneIndex].BoneInfluence = Vertex.InfluenceWeights[BoneIndex] / 255.0;
				}
			}
		}

		// setup wedge/face data
		uint32 OutWedgeIndex=0u;
		InstaLOD::uint32* const OutWedgeIndices = InstaMesh->GetWedgeIndices(nullptr);
		InstaLOD::InstaVec3F* const OutWedgeTangents = InstaMesh->GetWedgeTangents(nullptr);
		InstaLOD::InstaVec3F* const OutWedgeBinormals = InstaMesh->GetWedgeBinormals(nullptr);
		InstaLOD::InstaVec3F* const OutWedgeNormals = InstaMesh->GetWedgeNormals(nullptr);
		InstaLOD::InstaColorRGBAF32* const OutWedgeColors = InstaMesh->GetWedgeColors(0, nullptr);
		InstaLOD::InstaVec2F* OutWedgeTexcoords[InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS];

		for (uint32 TexcoordIndex=0u; TexcoordIndex<InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS; TexcoordIndex++)
		{
			OutWedgeTexcoords[TexcoordIndex] = InstaMesh->GetWedgeTexCoords(TexcoordIndex, nullptr);
		}

		InstaLOD::uint32* const OutFaceSmoothingMasks = InstaMesh->GetFaceSmoothingGroups(nullptr);
		InstaLOD::InstaMaterialID* const OutFaceMaterialIndices = InstaMesh->GetFaceMaterialIndices(nullptr);
		uint32 OutFaceIndex = 0u;

		constexpr int32 kDefaultSmoothingGroup = 1;

		const auto SkippedIndexOffset=[](const TArray<FSkelMeshSection>& Sections, const TArray<uint32>& SkipSections, const uint32 SectionIndex) -> uint32
		{
			if (SectionIndex == 0u)
				return 0u;

			uint32 IndexOffset = 0u;
			for (uint32 CurrentSectionIndex=0u; CurrentSectionIndex<SectionIndex; CurrentSectionIndex++)
			{
				if (!SkipSections.Contains(CurrentSectionIndex))
					continue;

				IndexOffset += Sections[CurrentSectionIndex].NumVertices;
			}
			return IndexOffset;
		};

		for (const uint32 ValidSectionIndex : ValidSectionIndices)
		{
			const FSkelMeshSection& Section = SourceLODModel.Sections[ValidSectionIndex];
			const uint32 EndVertexIndex = Section.BaseVertexIndex + Section.NumVertices;
			const uint32 SectionWedgeCount = Section.NumTriangles*3u;
			const uint32 IndexOffset = SkippedIndexOffset(SourceLODModel.Sections, SkipSections, ValidSectionIndex);

			for (uint32 Index=0u; Index<SectionWedgeCount; Index++)
			{
				const int32 WedgeIndex = Section.BaseIndex + Index;
				const uint32 VertexIndex = SourceLODModel.IndexBuffer[WedgeIndex];
				const FSoftSkinVertex& Vertex = Vertices[VertexIndex];

				OutWedgeIndices[OutWedgeIndex] = VertexIndex - IndexOffset;
				OutWedgeNormals[OutWedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(Vertex.TangentZ);

				OutWedgeBinormals[OutWedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(-Vertex.TangentY);
				OutWedgeTangents[OutWedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(Vertex.TangentX);

				for (int32 TexcoordIndex=0; TexcoordIndex<TexcoordCount; TexcoordIndex++)
				{
					OutWedgeTexcoords[TexcoordIndex][OutWedgeIndex] = UEInstaLODMeshHelper::FVectorToInstaVec(Vertex.UVs[TexcoordIndex]);
				}

				OutWedgeColors[OutWedgeIndex] = UEInstaLODMeshHelper::FColorToInstaColorRGBAF32(Vertex.Color);
				OutWedgeIndex++;
			}

			for (uint32 TriangleIndex=0u; TriangleIndex<Section.NumTriangles; TriangleIndex++)
			{
				OutFaceSmoothingMasks[OutFaceIndex] = kDefaultSmoothingGroup;
				OutFaceMaterialIndices[OutFaceIndex] = Section.MaterialIndex;
				OutFaceIndex++;
			}
		}

		UEInstaLODMeshHelper::SanitizeFloatArray((float*) OutVertexPositions, VertexCount * 3, 0.0f);
		UEInstaLODMeshHelper::SanitizeFloatArray((float*) OutWedgeNormals, WedgeCount * 3, 0.0f);
		UEInstaLODMeshHelper::SanitizeFloatArray((float*) OutWedgeBinormals, WedgeCount * 3, 0.0f);
		UEInstaLODMeshHelper::SanitizeFloatArray((float*) OutWedgeTangents, WedgeCount * 3, 0.0f);
		
		for (int32 TexcoordIndex=0; TexcoordIndex<TexcoordCount; TexcoordIndex++)
		{
			UEInstaLODMeshHelper::SanitizeFloatArray((float*) OutWedgeTexcoords[TexcoordIndex], WedgeCount * 2, 0.0f);
		}

		UEInstaLODMeshHelper::SanitizeFloatArray((float*) OutWedgeColors, WedgeCount * 4, 0.0f);

		InstaMesh->ReverseFaceDirections(/*flipNormals:*/ false);
	}

	static bool ReferenceSkeletonToInstaLODSkeleton(const FReferenceSkeleton& ReferenceSkeleton, InstaLOD::IInstaLODSkeleton *const InstaSkeleton, TMap<int32, TPair<uint32, FString>>& OutUEBoneIndexToInstaLODBoneIndexAndName)
	{
		if (InstaSkeleton == nullptr)
			return false;

		// set bone transforms as localspace transforms
		InstaSkeleton->SetJointTransformsInWorldSpace(false);

		// NOTE: don't use raw num as we don't need potential IK bones 
		const int32 BoneCount = ReferenceSkeleton.GetNum();
		const TArray<FTransform>& ReferencePoseInLocalSpace = ReferenceSkeleton.GetRefBonePose();

		// register all bones from ReferenceSkeleton
		for (int32 CurrentBoneIndex=0; CurrentBoneIndex<BoneCount; CurrentBoneIndex++)
		{
			const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(CurrentBoneIndex);
			const FName BoneName = ReferenceSkeleton.GetBoneName(CurrentBoneIndex);
			const FTransform BoneTransform = ReferencePoseInLocalSpace[CurrentBoneIndex];  
			const uint32 InstaLODParentBoneIndex = ParentIndex == INDEX_NONE ? InstaLOD::INSTALOD_JOINT_INDEX_INVALID : ParentIndex; 

			const FVector Position = BoneTransform.GetLocation();
			const FVector Scale = BoneTransform.GetScale3D();
			const FQuat Orientation = BoneTransform.GetRotation();

			const uint32 InstaLODBoneIndex = InstaSkeleton->AddJoint(
				InstaLODParentBoneIndex,
				CurrentBoneIndex, 
				UEInstaLODMeshHelper::FVectorToInstaVec(Position),
				UEInstaLODMeshHelper::FQuatToInstaQuaternion(Orientation),
				UEInstaLODMeshHelper::FVectorToInstaVec(Scale),
				/*Bone Name*/ "InstaLOD",
				/*User Pointer*/ nullptr
				);

			OutUEBoneIndexToInstaLODBoneIndexAndName.Emplace(CurrentBoneIndex, TPair<uint32, FString>(InstaLODBoneIndex, BoneName.ToString()));
		}

		return true;
	}

	static void InstaLODMeshToSkeletalLODModel(InstaLOD::IInstaLODMesh *const InstaMesh, USkeletalMesh* SourceSkeletalMesh, UE_StaticLODModel& OutputLODModel, FSkeletalMeshImportData& ImportData, const IInstaLOD::UE_MeshBuildOptions &BuildOptions)
	{ 
		using namespace SkeletalMeshImportData;
		check(InstaMesh != nullptr);
		check(InstaMesh->IsValid());

		InstaMesh->ReverseFaceDirections(/*flipNormals:*/ false);
		
		TArray<FVector3f> OutPoints;
		TArray<FVertInfluence> OutInfluences;
		InstaLOD::uint64 VertexCount;
		InstaLOD::InstaVec3F *const Vertices = InstaMesh->GetVertexPositions(&VertexCount);
		OutPoints.AddUninitialized(VertexCount);
		
		if (InstaMesh->GetSkinnedVertexData().IsInitialized())
		{
			OutInfluences.Reserve(VertexCount * (MAX_TOTAL_INFLUENCES/2));
			
			for (uint32 VertexIndex=0u; VertexIndex<VertexCount; VertexIndex++)
			{
				OutPoints[VertexIndex] = UEInstaLODMeshHelper::InstaVecToFVector(Vertices[VertexIndex]);
				InstaLOD::InstaLODSkeletalMeshBoneData *const BoneData = InstaMesh->GetSkinnedVertexData().GetBoneDataForVertex(VertexIndex);
				
				for (uint32 InfluenceIndex=0u; InfluenceIndex<MAX_TOTAL_INFLUENCES; InfluenceIndex++)
				{
					if (BoneData[InfluenceIndex].BoneInfluence <= 0.0 ||
						BoneData[InfluenceIndex].BoneIndex == InstaLOD::INSTALOD_BONE_INDEX_INVALID)
						continue;
					
					FVertInfluence VertexInfluence;
					VertexInfluence.BoneIndex = BoneData[InfluenceIndex].BoneIndex;
					VertexInfluence.Weight = BoneData[InfluenceIndex].BoneInfluence;
					VertexInfluence.VertIndex = VertexIndex;
					
					// NOTE: if a weight is specified with a value < (1/255) horrible glitches will occur
					// this is due to the fact that UE converts the value to a uint8 type and does not check
					// whether the influence is still > 0
					// A minimum threshold for bone influences can be specified in the InstaLOD settings
					// InstaLOD will then remove all weights that fall below the threshold and renormalize all weights
					// we're doing this, but just in case a developer decided to change the value we've got this safety here
					if (VertexInfluence.Weight < (1.0f/250.0f))
					{
						UE_LOG(LogInstaLOD, Warning, TEXT("Invalid bone influence for vert=%i bone=%i (%s) weight=%f"),
							   VertexInfluence.VertIndex,
							   VertexInfluence.BoneIndex,
							   *(SourceSkeletalMesh->GetRefSkeleton().GetBoneName(VertexInfluence.BoneIndex).ToString()),
							   VertexInfluence.Weight);
						continue;
					}
					
					OutInfluences.Add(VertexInfluence);
				}
			}
		}
		else
		{
			for (uint32 VertexIndex=0u; VertexIndex<VertexCount; VertexIndex++)
			{
				OutPoints[VertexIndex] = UEInstaLODMeshHelper::InstaVecToFVector(Vertices[VertexIndex]);
			}
		}
		
		// build triangle and wedge data
		InstaLOD::uint64 WedgeCount;
		InstaLOD::uint32 *const WedgeIndices = InstaMesh->GetWedgeIndices(&WedgeCount);
		InstaLOD::InstaColorRGBAF32 *const WedgeColors = InstaMesh->GetWedgeColors(0, nullptr);
		InstaLOD::InstaVec3F *const WedgeTangents = InstaMesh->GetWedgeTangents(nullptr);
		InstaLOD::InstaVec3F *const WedgeBinormals = InstaMesh->GetWedgeBinormals(nullptr);
		InstaLOD::InstaVec3F *const WedgeNormals = InstaMesh->GetWedgeNormals(nullptr);
		InstaLOD::InstaVec2F* WedgeTexcoords[MAX_TEXCOORDS]; /**< NOTE: array size is UE5's max texcoord define */
		uint32 TexcoordCount = 0u;

		for (uint32 TexcoordIndex=0u; TexcoordIndex<InstaLOD::INSTALOD_MAX_MESH_TEXCOORDS; TexcoordIndex++)
		{
			WedgeTexcoords[TexcoordCount] = InstaMesh->GetWedgeTexCoords(TexcoordIndex, nullptr);
			
			if (WedgeTexcoords[TexcoordCount] != nullptr)
			{
				TexcoordCount++;
				
				if (TexcoordCount >= MAX_TEXCOORDS)
					break;
			}
		}

		InstaLOD::uint64 FaceCount;
		InstaLOD::uint32 *const FaceSmoothingGroups = InstaMesh->GetFaceSmoothingGroups(&FaceCount);
		InstaLOD::InstaMaterialID *const FaceMaterialIndices = InstaMesh->GetFaceMaterialIndices(nullptr);
		check(FaceCount * 3u == WedgeCount);
		
		TArray<FMeshWedge> OutWedges;
		TArray<FMeshFace> OutFaces;
		OutWedges.AddUninitialized(WedgeCount);
		OutFaces.AddUninitialized(FaceCount);
		
		static const FColor kDefaultColor(0, 0, 0, 255);
		
		for (uint32 FaceIndex=0u; FaceIndex<FaceCount; FaceIndex++)
		{
			FMeshFace &Face = OutFaces[FaceIndex];
			
			Face.MeshMaterialIndex = FaceMaterialIndices[FaceIndex];
			Face.SmoothingGroups = FaceSmoothingGroups[FaceIndex];
			
			// NOTE: this should unroll well
			for (uint32 CornerIndex=0u; CornerIndex<3u; CornerIndex++)
			{
				const uint32 WedgeIndex = FaceIndex*3u + CornerIndex;
				
				Face.iWedge[CornerIndex] = WedgeIndex;
				Face.TangentX[CornerIndex] = UEInstaLODMeshHelper::InstaVecToFVector(WedgeTangents[WedgeIndex]);
				Face.TangentY[CornerIndex] = UEInstaLODMeshHelper::InstaVecToFVector(WedgeBinormals[WedgeIndex]);
				Face.TangentZ[CornerIndex] = UEInstaLODMeshHelper::InstaVecToFVector(WedgeNormals[WedgeIndex]);
				
				FMeshWedge &wedge = OutWedges[WedgeIndex];
				wedge.Color = (WedgeColors == nullptr) ? kDefaultColor : UEInstaLODMeshHelper::InstaColorRGBAF32ToFColor(WedgeColors[WedgeIndex]);
				wedge.iVertex = WedgeIndices[WedgeIndex];
				
				for (uint32 TexcoordIndex=0u; TexcoordIndex<TexcoordCount; TexcoordIndex++)
				{
					wedge.UVs[TexcoordIndex] = UEInstaLODMeshHelper::InstaVecToFVector(WedgeTexcoords[TexcoordIndex][WedgeIndex]);
				}

				for (uint32 TexcoordIndex=TexcoordCount; TexcoordIndex<MAX_TEXCOORDS; TexcoordIndex++)
				{
					wedge.UVs[TexcoordIndex] = FVector2f(0.0f, 0.0f);
				}
			}
		}
		
		// NOTE: we don't know how the vertices related to the original import vertices, so create a faux-map
		TArray<int32> TempPointToOriginalMap;
		TempPointToOriginalMap.AddUninitialized(OutPoints.Num());
		for(int32 VertexIndex=0; VertexIndex<OutPoints.Num(); VertexIndex++)
		{
			TempPointToOriginalMap[VertexIndex] = VertexIndex;
		}
		
		IMeshUtilities& MeshUtilities = FModuleManager::Get().GetModuleChecked<IMeshUtilities>("MeshUtilities");
		
		FString SkeletalMeshName;
		SourceSkeletalMesh->GetName(SkeletalMeshName);

		bool bBuildResult = MeshUtilities.BuildSkeletalMesh(OutputLODModel,
															SkeletalMeshName,
															SourceSkeletalMesh->GetRefSkeleton(),
															OutInfluences,
															OutWedges,
															OutFaces,
															OutPoints,
															TempPointToOriginalMap,
															BuildOptions);
		
		// Set texture coordinate count on the new model.
		OutputLODModel.NumTexCoords = TexcoordCount;

		bool bHasVertexColors = false;
		int32 MaximumMaterialIndex = -1;

		// build import mesh
		ImportData.Points.Empty(OutPoints.Num());
		ImportData.Points.AddUninitialized(OutPoints.Num());
		for (int32 Index=0; Index<OutPoints.Num(); Index++)
		{
			ImportData.Points[Index] = OutPoints[Index];
		}

		ImportData.Wedges.Empty(OutWedges.Num());
		ImportData.Wedges.AddUninitialized(OutWedges.Num());
		for (int32 Index=0; Index<OutWedges.Num(); Index++)
		{
			ImportData.Wedges[Index].VertexIndex = OutWedges[Index].iVertex;
			ImportData.Wedges[Index].Color = OutWedges[Index].Color;

			if (!bHasVertexColors && ImportData.Wedges[Index].Color != FColor::Black)
			{
				bHasVertexColors = true;
			}

			for (int32 TexcoordIndex=0; TexcoordIndex<MAX_TEXCOORDS; TexcoordIndex++)
			{
				ImportData.Wedges[Index].UVs[TexcoordIndex] = OutWedges[Index].UVs[TexcoordIndex];
			}
		}

		ImportData.Faces.Empty(OutFaces.Num());
		ImportData.Faces.AddUninitialized(OutFaces.Num());
		for (int32 Index=0; Index<OutFaces.Num(); Index++)
		{
			const SkeletalMeshImportData::FMeshFace& Face = OutFaces[Index];
			FTriangle& Triangle = ImportData.Faces[Index];

			Triangle.WedgeIndex[0] = Face.iWedge[0];
			Triangle.WedgeIndex[1] = Face.iWedge[1];
			Triangle.WedgeIndex[2] = Face.iWedge[2];

			Triangle.MatIndex = Face.MeshMaterialIndex;

			Triangle.TangentX[0] = Face.TangentX[0];
			Triangle.TangentX[1] = Face.TangentX[1];
			Triangle.TangentX[2] = Face.TangentX[2];

			Triangle.TangentY[0] = Face.TangentY[0];
			Triangle.TangentY[1] = Face.TangentY[1];
			Triangle.TangentY[2] = Face.TangentY[2];

			Triangle.TangentZ[0] = Face.TangentZ[0];
			Triangle.TangentZ[1] = Face.TangentZ[1];
			Triangle.TangentZ[2] = Face.TangentZ[2];

			Triangle.SmoothingGroups = Face.SmoothingGroups;

			if (MaximumMaterialIndex < Triangle.MatIndex)
			{
				MaximumMaterialIndex = Triangle.MatIndex;
			}
		}

		ImportData.Influences.Empty(OutInfluences.Num());
		ImportData.Influences.AddUninitialized(OutInfluences.Num());
		for (int32 Index=0; Index<OutInfluences.Num(); Index++)
		{
			ImportData.Influences[Index].Weight = OutInfluences[Index].Weight;
			ImportData.Influences[Index].BoneIndex = OutInfluences[Index].BoneIndex;
			ImportData.Influences[Index].VertexIndex = OutInfluences[Index].VertIndex;
		}

		ImportData.PointToRawMap = TempPointToOriginalMap;
		ImportData.MaxMaterialIndex = MaximumMaterialIndex;
		ImportData.bHasNormals = true;
		ImportData.bHasTangents = true;
		ImportData.bHasVertexColors = bHasVertexColors;
		ImportData.NumTexCoords = MAX_TEXCOORDS;
	}
	
	static InstaLOD::MeshFeatureImportance::Type GetInstaLODMeshFeatureImportance(SkeletalMeshOptimizationImportance value)
	{
		switch(value)
		{
			case SMOI_Off:
				return InstaLOD::MeshFeatureImportance::Off;
			case SMOI_Lowest:
				return InstaLOD::MeshFeatureImportance::Lowest;
			case SMOI_Low:
				return InstaLOD::MeshFeatureImportance::Low;
			case SMOI_Normal:
				return InstaLOD::MeshFeatureImportance::Normal;
			case SMOI_High:
				return InstaLOD::MeshFeatureImportance::High;
			case SMOI_Highest:
				return InstaLOD::MeshFeatureImportance::Highest;
			default:
				return InstaLOD::MeshFeatureImportance::Normal;
		}
	}
	
	static InstaLOD::OptimizeSettings ConvertMeshReductionSettingsToInstaLOD(const FSkeletalMeshOptimizationSettings& Settings, USkeletalMesh* SkeletalMesh)
	{
		InstaLOD::OptimizeSettings OptimizeSettings;

		// Settings.BoneReductionRatio
		OptimizeSettings.SkinningImportance = GetInstaLODMeshFeatureImportance(Settings.SkinningImportance);
		OptimizeSettings.SkeletonOptimize.MinimumBoneInfluenceThreshold = 1.0f / 250.0f;
		OptimizeSettings.SkeletonOptimize.MaximumBoneInfluencesPerVertex = Settings.MaxBonesPerVertex;

		OptimizeSettings.ScreenSizeInPixels = 0.0f;
		OptimizeSettings.PercentTriangles = Settings.NumOfTrianglesPercentage;
		OptimizeSettings.AlgorithmStrategy = (InstaLOD::AlgorithmStrategy::Type)CVarAlgorithmStrategy.GetValueOnAnyThread();
		OptimizeSettings.HardAngleThreshold = Settings.NormalsThreshold;

		if (Settings.ReductionMethod == SkeletalMeshOptimizationType::SMOT_MaxDeviation)
		{
			OptimizeSettings.PercentTriangles = 0;
			OptimizeSettings.MaxDeviation = Settings.MaxDeviationPercentage;
		}
		else
		{
			OptimizeSettings.MaxDeviation = CVarDefaultMaximumError.GetValueOnAnyThread();
		}

		OptimizeSettings.ShadingImportance = GetInstaLODMeshFeatureImportance(Settings.ShadingImportance);
		OptimizeSettings.TextureImportance = GetInstaLODMeshFeatureImportance(Settings.TextureImportance);
		OptimizeSettings.SilhouetteImportance = GetInstaLODMeshFeatureImportance(Settings.SilhouetteImportance == SMOI_High ? SMOI_Normal : (SkeletalMeshOptimizationImportance)Settings.SilhouetteImportance);
		OptimizeSettings.SkinningImportance = GetInstaLODMeshFeatureImportance(Settings.SkinningImportance);
		OptimizeSettings.WeldingThreshold = Settings.WeldingThreshold;
		OptimizeSettings.LockSplits = CVarLockSplits.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.LockBoundaries = CVarLockBoundaries.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.RecalculateNormals = Settings.bRecalcNormals;
		OptimizeSettings.WeightedNormals = CVarWeightedNormals.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.WeldingProtectDistinctUVShells = CVarWeldingProtectDistinctUVShells.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.OptimalPlacement = CVarOptimalPlacement.GetValueOnAnyThread() > 0 ? true : false;
		OptimizeSettings.OptimizerVertexWeights = CVarForceOptimizerWeights.GetValueOnAnyThread() > 0 || Settings.SilhouetteImportance >= SMOI_High ? true : false;
		OptimizeSettings.Deterministic = CVarOptimizeDeterministic.GetValueOnAnyThread() > 0 ? true : false;
		return OptimizeSettings;
	}
};

FInstaLOD::FInstaLOD(InstaLOD::IInstaLOD *InstaLODAPI)
{
	InstaLOD = InstaLODAPI;
	VersionString = InstaLODShared::Version;
}

void FInstaLOD::UnbindClothAtLODIndex(USkeletalMesh* SkeletalMesh, const int32 LODIndex)
{
	if(SkeletalMesh == nullptr)
		return;

	TArray<ClothingAssetUtils::FClothingAssetMeshBinding> ClothingBindings;
	FLODUtilities::UnbindClothingAndBackup(SkeletalMesh, ClothingBindings, LODIndex);
}

void FInstaLOD::ReduceMeshDescription(FMeshDescription& OutReducedMesh, float& OutMaxDeviation, const FMeshDescription& InMesh,
									  const FOverlappingCorners& InOverlappingCorners, const struct FMeshReductionSettings& ReductionSettings)
{
	TMap<FName, int32> InMaterialMap;
	TMap<int32, FName> OutMaterialMap;

	UEInstaLODMeshHelper::CreateInputOutputMaterialMapFromMeshDescription(InMesh, InMaterialMap, OutMaterialMap);

	InstaLOD::IInstaLODMesh* InstaMesh = AllocInstaLODMesh();
	InstaLOD::IInstaLODMesh* OutMesh = AllocInstaLODMesh();
	UEInstaLODMeshHelper::MeshDescriptionToInstaLODMesh(InMesh, InMaterialMap, InstaMesh);
	InstaLOD::OptimizeSettings OptimizeSettings = UEInstaLODMeshHelper::ConvertMeshReductionSettingsToInstaLOD(ReductionSettings);

	InstaMesh->SanitizeMesh();

	if (OptimizeSettings.OptimizerVertexWeights)
	{
		if (!InstaMesh->ConvertColorDataToOptimizerWeights(0))
		{
			UE_LOG(LogInstaLOD, Log, TEXT("Failed to convert color data to optimizer weights. Is color data available for mesh?"));
		}
	}
	
	const double T0 = FPlatformTime::Seconds();
	
	// generate optimized insta mesh
	InstaLOD::OptimizeResult InstaResult = InstaLOD->Optimize(InstaMesh, OutMesh, OptimizeSettings);
	
	if (CVarAssertOnKeyMesh.GetValueOnAnyThread() != 0)
	{
		if (!InstaResult.IsAuthorized)
		{
			UE_LOG(LogInstaLOD, Fatal, TEXT("%s"), *InstaLODAssertOnKeyMeshMessage);
		}
	}

	const double T1 = FPlatformTime::Seconds();
	const float ElapsedTime = (float)(T1-T0);
	
	if (InstaResult.Success)
	{
		// scale deviation per cvar
		InstaResult.MeshDeviation *= CVarDecimatedErrorFactor.GetValueOnAnyThread();
		OutMaxDeviation = InstaResult.MeshDeviation;
	}
	else
	{
		char InstaLog[8192];
		InstaLOD->GetMessageLog(InstaLog, sizeof(InstaLog), nullptr);
		UE_LOG(LogInstaLOD, Error, TEXT("Optimization failed. Log: %s"), UTF8_TO_TCHAR(InstaLog));
		OutMaxDeviation = 0;

		// restore input mesh
		InstaLOD::IInstaLODMesh *const Temp = InstaMesh;
		InstaMesh = OutMesh;
		OutMesh = Temp;
	}
	
	if (CVarWriteOBJ.GetValueOnAnyThread().Len() > 0)
	{
		FText NotificationText;
		if (OutMesh->WriteLightwaveOBJ(TCHAR_TO_UTF8(*CVarWriteOBJ.GetValueOnAnyThread())))
		{
			NotificationText = FText::Format(LOCTEXT("LODPluginWriteOBJSuccess", "Wrote Lightwave OBJ to \"{0}\""), FText::FromString(CVarWriteOBJ.GetValueOnAnyThread()));
			DispatchNotification(NotificationText, SNotificationItem::CS_Success);
		}
		else
		{
			NotificationText = FText::Format(LOCTEXT("LODPluginWriteOBJSuccess", "Failed to write Lightwave OBJ to \"{0}\""), FText::FromString(CVarWriteOBJ.GetValueOnAnyThread()));
			DispatchNotification(NotificationText, SNotificationItem::CS_Fail);
		}
		UE_LOG(LogInstaLOD, Log, TEXT("%s"), *NotificationText.ToString());
	}

	// remove possible duplicates
	InstaLOD->CastToInstaLODMeshExtended(OutMesh)->FixDuplicateVertexPositions(0.0f);
	
	UEInstaLODMeshHelper::InstaLODMeshToMeshDescription(OutMesh, OutMaterialMap, OutReducedMesh);

	InstaLOD->DeallocMesh(InstaMesh);
	InstaLOD->DeallocMesh(OutMesh);
}
 
bool FInstaLOD::ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex)
{
	bool bReregisterComponent = false;
	const FSkeletalMeshOptimizationSettings ReductionSettings = SkeletalMesh->GetLODInfoArray().IsValidIndex(LODIndex) ? SkeletalMesh->GetLODInfo(LODIndex)->ReductionSettings  : FSkeletalMeshOptimizationSettings();
	const bool bCalcLODDistance = !SkeletalMesh->GetLODInfoArray().IsValidIndex(LODIndex); // From UE4.19, LODUtilities.cpp, line 106 

	return ReduceSkeletalMesh(SkeletalMesh, LODIndex, ReductionSettings, bCalcLODDistance, bReregisterComponent);
}

bool FInstaLOD::ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex, const struct FSkeletalMeshOptimizationSettings& Settings,
								   bool bCalcLODDistance, bool bReregisterComponent, const class ITargetPlatform* TargetPlatform)
{
	if (TargetPlatform == nullptr)
	{
		TargetPlatform = UEInstaLODMeshHelper::GetRunningTargetPlatform();
	}

	if (bReregisterComponent)
	{
		// Should only be used from the rendering thread.
		if (!GIsThreadedRendering)
		{
			TComponentReregisterContext<USkinnedMeshComponent> ReregisterContext;
		}

		SkeletalMesh->ReleaseResources();
		SkeletalMesh->ReleaseResourcesFence.Wait();
	}
	
	ReduceSkeletalMeshAtLODIndex(SkeletalMesh, LODIndex, Settings, bCalcLODDistance, TargetPlatform);
	
	if (bReregisterComponent)
	{
		SkeletalMesh->PostEditChange();
		SkeletalMesh->InitResources();
	}
	
	return true;
}

bool FInstaLOD::ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex, const class ITargetPlatform* TargetPlatform)
{
	bool bReregisterComponent = false;
	const FSkeletalMeshOptimizationSettings ReductionSettings = SkeletalMesh->GetLODInfoArray().IsValidIndex(LODIndex) ? SkeletalMesh->GetLODInfo(LODIndex)->ReductionSettings : FSkeletalMeshOptimizationSettings();
	const bool bCalcLODDistance = !SkeletalMesh->GetLODInfoArray().IsValidIndex(LODIndex);

	return ReduceSkeletalMesh(SkeletalMesh, LODIndex, ReductionSettings, bCalcLODDistance, bReregisterComponent, TargetPlatform);
}

bool FInstaLOD::ReduceSkeletalMesh(class USkeletalMesh* SkeletalMesh, int32 LODIndex,
								   const struct FSkeletalMeshOptimizationSettings& Settings, bool bCalcLODDistance, const class ITargetPlatform* TargetPlatform)
{
	ReduceSkeletalMesh(SkeletalMesh, LODIndex, Settings, bCalcLODDistance, true, TargetPlatform);
	return true;
}

void FInstaLOD::ReduceSkeletalMeshAtLODIndex(class USkeletalMesh* SkeletalMesh, int32 LODIndex,
	const FSkeletalMeshOptimizationSettings& Settings, bool bCalcLODDistance, const class ITargetPlatform* TargetPlatform)
{
	// NOTE: This code is based on Unreal Engine internal skeletalmesh optimization routine
	// only reduction code is replaced with InstaLOD code.

	check(SkeletalMesh);

	if (TargetPlatform == nullptr)
	{
		TargetPlatform = UEInstaLODMeshHelper::GetRunningTargetPlatform();
		UE_LOG(LogInstaLOD, Warning, TEXT("Using running target platform for SkeletalMesh reduction."));
	}

	if (IsRunningCommandlet() && !InstaLOD->IsHostAuthorized())
	{
		UE_LOG(LogInstaLOD, Fatal, TEXT("This machine is not authorized to run InstaLOD. Please authorize this machine before cooking using the editor or InstaLOD Pipeline."));
	}

	//If the Current LOD is an import from file
	bool bOldLodWasFromFile = SkeletalMesh->IsValidLODIndex(LODIndex) && SkeletalMesh->GetLODInfo(LODIndex)->bHasBeenSimplified == false;

	//True if the LOD is added by this reduction
	bool bLODModelAdded = false;

	UE_SkeletalMeshResource* const SkeletalMeshResource=SkeletalMesh->GetImportedModel();
	check(SkeletalMeshResource);
	check(LODIndex <= SkeletalMeshResource->LODModels.Num());

	// Insert a new LOD model entry if needed.
	if (LODIndex == SkeletalMeshResource->LODModels.Num())
	{
		FSkeletalMeshLODModel* ModelPtr = nullptr;
		SkeletalMeshResource->LODModels.Add(ModelPtr);
		bLODModelAdded = true;
	}

	// Copy over LOD info from LOD0 if there is no previous info.
	if (!SkeletalMesh->GetLODInfoArray().IsValidIndex(LODIndex))
	{
		// if there is no LOD, add one more
		SkeletalMesh->AddLODInfo();
	}

	// Swap in a new model, delete the old. 
	FSkeletalMeshLODModel** LODModels = SkeletalMeshResource->LODModels.GetData();

	// get settings
	FSkeletalMeshLODInfo *const LODInfo = SkeletalMesh->GetLODInfo(LODIndex);

	FSkeletalMeshLODModel DstModelBackup;
	if (!bLODModelAdded)
	{
		DstModelBackup.Sections = SkeletalMeshResource->LODModels[LODIndex].Sections;
		DstModelBackup.UserSectionsData = SkeletalMeshResource->LODModels[LODIndex].UserSectionsData;
	}

	struct FImportantBones
	{
		TArray<int32> Ids;
		float Weight;
	};

	// Struct to identify important bones.  Vertices associated with these bones
	// will have additional collapse weight added to them.
	FImportantBones  ImportantBones;
	{
		const TArray<FBoneReference>& BonesToPrioritize = LODInfo->BonesToPrioritize;
		const float BonePrioritizationWeight = LODInfo->WeightOfPrioritization;

		ImportantBones.Weight = BonePrioritizationWeight;
		for (const FBoneReference& BoneReference : BonesToPrioritize)
		{
			int32 BoneId = SkeletalMesh->GetRefSkeleton().FindRawBoneIndex(BoneReference.BoneName);

			// Q: should we exclude BoneId = 0?
			ImportantBones.Ids.AddUnique(BoneId);
		}
	}

	// select which mesh we're reducing from
	// use BaseLOD
	int32 BaseLOD = 0;
	FSkeletalMeshModel* const SkelResource = SkeletalMesh->GetImportedModel();

	FSkeletalMeshLODModel* SrcModel = &SkelResource->LODModels[0];

	// only allow to set BaseLOD if the LOD is less than this
	if (Settings.BaseLOD > 0)
	{
		if (Settings.BaseLOD == LODIndex && (!SkeletalMesh->IsLODImportedDataBuildAvailable(Settings.BaseLOD) || SkeletalMesh->IsLODImportedDataEmpty(Settings.BaseLOD)))
		{
			//Cannot reduce ourself if we are not imported
			UE_LOG(LogInstaLOD, Warning, TEXT("Building LOD %d - Cannot generate LOD with himself if the LOD do not have imported Data. Using Base LOD 0 instead"), LODIndex);
		}
		else if (Settings.BaseLOD <= LODIndex && SkeletalMeshResource->LODModels.IsValidIndex(Settings.BaseLOD))
		{
			BaseLOD = Settings.BaseLOD;
			SrcModel = &SkeletalMeshResource->LODModels[BaseLOD];
		}
		else
		{
			// warn users
			UE_LOG(LogInstaLOD, Warning, TEXT("Building LOD %d - Invalid Base LOD entered. Using Base LOD 0 instead"), LODIndex);
		}
	}

	//We backup only the sections and the user sections data
	FSkeletalMeshLODModel SrcModelBackup;
	SrcModelBackup.Sections = SrcModel->Sections;
	SrcModelBackup.UserSectionsData = SrcModel->UserSectionsData;

	//Restore the source sections data
	const auto RestoreUserSectionsData = [](const FSkeletalMeshLODModel& SourceLODModel, FSkeletalMeshLODModel& DestinationLODModel) -> bool
	{
		//Now restore the reduce section user change and adjust the originalDataSectionIndex to point on the correct UserSectionData
		TBitArray<> SourceSectionMatched;
		SourceSectionMatched.Init(false, SourceLODModel.Sections.Num());

		TBitArray<> AllSectionsMatched;
		AllSectionsMatched.Init(true, FMath::Max(SourceLODModel.Sections.Num(), DestinationLODModel.Sections.Num()));

		for (int32 SectionIndex=0; SectionIndex<DestinationLODModel.Sections.Num(); ++SectionIndex)
		{
			FSkelMeshSection& Section = DestinationLODModel.Sections[SectionIndex];
			FSkelMeshSourceSectionUserData& DestinationUserData = FSkelMeshSourceSectionUserData::GetSourceSectionUserData(DestinationLODModel.UserSectionsData, Section);
			
			for (int32 SourceSectionIndex=0; SourceSectionIndex<SourceLODModel.Sections.Num(); ++SourceSectionIndex)
			{
				if (SourceSectionMatched[SourceSectionIndex])
					continue;

				const FSkelMeshSection& SourceSection = SourceLODModel.Sections[SourceSectionIndex];
				if (const FSkelMeshSourceSectionUserData* SourceUserData = SourceLODModel.UserSectionsData.Find(SourceSection.OriginalDataSectionIndex))
				{
					if (Section.MaterialIndex == SourceSection.MaterialIndex)
					{
						DestinationUserData =* SourceUserData;
						SourceSectionMatched[SourceSectionIndex] = true;
						break;
					}
				}
			}
		}
		DestinationLODModel.SyncronizeUserSectionsDataArray();

		return SourceSectionMatched == AllSectionsMatched;
	};

	FString BackupLodModelBuildStringID = TEXT("");
	FString BackupRawSkeletalMeshBulkDataID = TEXT("");

	// Unbind any existing clothing assets before we reimport the geometry
	TArray<ClothingAssetUtils::FClothingAssetMeshBinding> ClothingBindings;

	//Do not play with cloth if the LOD is added
	if (!bLODModelAdded)
	{
		FLODUtilities::UnbindClothingAndBackup(SkeletalMesh, ClothingBindings, LODIndex);
		//We have to put back the exact UserSectionsData to not invalidate the DDC key
		BackupLodModelBuildStringID = SkeletalMeshResource->LODModels[LODIndex].BuildStringID;
		BackupRawSkeletalMeshBulkDataID = SkeletalMeshResource->LODModels[LODIndex].RawSkeletalMeshBulkDataID;
	}

	bool bReducingSourceModel = false;

	//Reducing source LOD, we need to use the temporary data so it can be iterative
	if (BaseLOD == LODIndex && (SkelResource->OriginalReductionSourceMeshData_DEPRECATED.IsValidIndex(BaseLOD) && !SkelResource->OriginalReductionSourceMeshData_DEPRECATED[BaseLOD]->IsEmpty()))
	{
		TMap<FString, TArray<FMorphTargetDelta>> TempLODMorphTargetData;
		SkelResource->OriginalReductionSourceMeshData_DEPRECATED[BaseLOD]->LoadReductionData(*SrcModel, TempLODMorphTargetData, SkeletalMesh);

		//Restore the section data state (like disabled...)
		RestoreUserSectionsData(SrcModelBackup, *SrcModel);

		//Rebackup the source model since we update it, source always have empty LODMaterial map
		//If you swap a material ID and after you do inline reduction, you have to remap it again, but not if you reduce and then remap the materialID
		//this is by design currently
		bReducingSourceModel = true;
	}
	else
	{
		check(BaseLOD < LODIndex);
	}

	check(SrcModel);

	// now try bone reduction process if it's setup
	TMap<FBoneIndexType, FBoneIndexType> BonesToRemove;
	IMeshBoneReduction* const MeshBoneReductionInterface = FModuleManager::Get().LoadModuleChecked<IMeshBoneReductionModule>("MeshBoneReduction").GetMeshBoneReductionInterface();

	TArray<FName> BoneNames;
	const int32 NumBones = SkeletalMesh->GetRefSkeleton().GetNum();
	for (int32 BoneIndex=0; BoneIndex < NumBones; ++BoneIndex)
	{
		BoneNames.Add(SkeletalMesh->GetRefSkeleton().GetBoneName(BoneIndex));
	}

	// get the relative to ref pose matrices
	TArray<FMatrix> RelativeToRefPoseMatrices;
	RelativeToRefPoseMatrices.AddUninitialized(NumBones);

	FSkeletalMeshLODModel *const NewModel = new FSkeletalMeshLODModel();

	// Swap out the old model.
	FSkeletalMeshImportData RawMesh;
	bool bPutBackRawMesh = false;
	ESkeletalMeshGeoImportVersions GeoImportVersion=ESkeletalMeshGeoImportVersions::Before_Versionning;
	ESkeletalMeshSkinningImportVersions SkinningImportVersion=ESkeletalMeshSkinningImportVersions::Before_Versionning;
	{
		FSkeletalMeshLODModel* Old = LODModels[LODIndex];
		LODModels[LODIndex] = NewModel;

		if (!bReducingSourceModel && Old)
		{
			bool bIsOldRawSkelMeshEmpty = Old->bIsRawSkeletalMeshBulkDataEmpty;
			//We need to backup the original RawSkeletalMeshBulkData in case it was an imported LOD
			if (!bLODModelAdded && !bIsOldRawSkelMeshEmpty)
			{
				SkeletalMesh->LoadLODImportedData(LODIndex, RawMesh);
				SkeletalMesh->GetLODImportedDataVersions(LODIndex, GeoImportVersion, SkinningImportVersion);
				bPutBackRawMesh = true;
			}

			//If the delegate is not bound 
			if (!Settings.OnDeleteLODModelDelegate.IsBound())
			{
				//If not in game thread we should never delete a structure containing bulkdata since it can crash when the bulkdata is detach from the archive
				//Use the delegate and delete the pointer in the main thread if you reduce in other thread then game thread (main thread).
				check(IsInGameThread());
				delete Old;
			}
			else
			{
				Settings.OnDeleteLODModelDelegate.Execute(Old);
			}
		}
		else if (bReducingSourceModel)
		{
			SkeletalMesh->LoadLODImportedData(BaseLOD, RawMesh);
			SkeletalMesh->GetLODImportedDataVersions(BaseLOD, GeoImportVersion, SkinningImportVersion);
			bPutBackRawMesh = true;
		}
	}

	bool bMeshIsReduced = false;

	// Reduce LOD model with SrcMesh if src mesh has more then 1 triangle
	if (SrcModel->NumVertices > 3)
	{
		// InstaLOD reduction here: 
		UE_SkeletalBakePoseData BakePoseData;

		// NOTE: if a bake pose is set we must apply it to the mesh
		// the actual BakePose is the first frame of the set animation
		if (UAnimSequence* const BakePoseAnim = SkeletalMesh->GetLODInfo(LODIndex)->BakePose)
		{
			BakePoseData.BakePoseAnimation = BakePoseAnim;
			BakePoseData.ReferenceSkeleton = &SkeletalMesh->GetRefSkeleton();
			BakePoseData.SkeletalMesh=SkeletalMesh;
		}

		// create our insta meshes and feed data into our mesh from UE
		InstaLOD::IInstaLODMesh* const InstaInputMesh = AllocInstaLODMesh();
		InstaLOD::IInstaLODMesh* const InstaOutputMesh = AllocInstaLODMesh();

		// collect all skip sections
		TArray<uint32> SkipSections = UEInstaLODSkeletalMeshHelper::GetSkipSectionsForLODModel(SrcModel, LODIndex);
		UEInstaLODSkeletalMeshHelper::SkeletalLODModelToInstaLODMesh(*SrcModel, InstaInputMesh, SkipSections, &BakePoseData, LODIndex);

		const double T0 = FPlatformTime::Seconds();

		// generate optimized insta mesh
		const InstaLOD::OptimizeSettings OptimizeSettings = UEInstaLODSkeletalMeshHelper::ConvertMeshReductionSettingsToInstaLOD(Settings, SkeletalMesh);
		InstaLOD::OptimizeResult InstaResult = InstaLOD->Optimize(InstaInputMesh, InstaOutputMesh, OptimizeSettings);

		if (CVarAssertOnKeyMesh.GetValueOnAnyThread() != 0)
		{
			if (!InstaResult.IsAuthorized)
			{
				UE_LOG(LogInstaLOD, Fatal, TEXT("%s"), *InstaLODAssertOnKeyMeshMessage);
			}
		}

		if (InstaResult.Success)
		{
			// scale deviation per cvar
			InstaResult.MeshDeviation *= CVarDecimatedErrorFactor.GetValueOnAnyThread();

			IInstaLOD::UE_MeshBuildOptions BuildOptions;
			BuildOptions.bRemoveDegenerateTriangles = false; /**< InstaLOD takes care of mesh healing */
			BuildOptions.bUseMikkTSpace = false;
			BuildOptions.bComputeNormals = false;
			BuildOptions.bComputeTangents = false;
			bMeshIsReduced = true;

			FSkeletalMeshImportData ImportData;
			UEInstaLODSkeletalMeshHelper::InstaLODMeshToSkeletalLODModel(InstaOutputMesh, SkeletalMesh, *NewModel, ImportData, BuildOptions);

			if (bOldLodWasFromFile)
			{
				SkeletalMesh->GetLODInfo(LODIndex)->LODMaterialMap.Empty();
			}
			else if (SkeletalMesh->GetLODInfo(LODIndex)->LODMaterialMap.Num() == 0 && SkeletalMesh->GetLODInfo(BaseLOD)->LODMaterialMap.Num() != 0)
			{
				// update LOD material map
				const TArray<int32>& BaseLODMaterialMap = SkeletalMesh->GetLODInfo(BaseLOD)->LODMaterialMap;
				TArray<int32>& LODMaterialMap = SkeletalMesh->GetLODInfo(LODIndex)->LODMaterialMap;

				for (int32 MaterialMapIndex=0; MaterialMapIndex<BaseLODMaterialMap.Num(); MaterialMapIndex++)
				{
					if (SkipSections.Contains(MaterialMapIndex))
						continue;
					LODMaterialMap.Add(BaseLODMaterialMap[MaterialMapIndex]);
				}
			}

			SkeletalMesh->GetLODInfoArray()[LODIndex].bHasBeenSimplified = true;
			SkeletalMesh->SetHasBeenSimplified(true);

			if (bCalcLODDistance)
			{
				if (LODIndex <= 0)
				{
					SkeletalMesh->GetLODInfoArray()[LODIndex].ScreenSize = 1.0f;
				}
				else
				{
					constexpr float ToleranceFactor = 0.95f;
					constexpr float AutoLODError = 1.0f;
					const float MaxScreenSize = (LODIndex == 1 ? 1.0 : SkeletalMesh->GetLODInfoArray()[LODIndex - 1].ScreenSize.Default) * ToleranceFactor;
					const float MinScreenSize = MaxScreenSize * 0.5f;
					const float ViewDistance = (InstaResult.MeshDeviation * 960.0f) / AutoLODError;
					const float AutoScreenSize = 2.0f * SkeletalMesh->GetBounds().SphereRadius / ViewDistance;
					SkeletalMesh->GetLODInfoArray()[LODIndex].ScreenSize = FPerPlatformFloat(FMath::Clamp(AutoScreenSize, MinScreenSize, MaxScreenSize));
				}
			}

			if (CVarWriteOBJ.GetValueOnAnyThread().Len() > 0)
			{
				FText NotificationText;
				if (InstaOutputMesh->WriteLightwaveOBJ(TCHAR_TO_UTF8(*CVarWriteOBJ.GetValueOnAnyThread())))
				{
					NotificationText = FText::Format(LOCTEXT("LODPluginWriteOBJSuccess", "Wrote Lightwave OBJ to \"{0}\""), FText::FromString(CVarWriteOBJ.GetValueOnAnyThread()));
					DispatchNotification(NotificationText, SNotificationItem::CS_Success);
				}
				else
				{
					NotificationText = FText::Format(LOCTEXT("LODPluginWriteOBJSuccess", "Failed to write Lightwave OBJ to \"{0}\""), FText::FromString(CVarWriteOBJ.GetValueOnAnyThread()));
					DispatchNotification(NotificationText, SNotificationItem::CS_Fail);
				}
				UE_LOG(LogInstaLOD, Log, TEXT("%s"), *NotificationText.ToString());
			}

			InstaLOD->DeallocMesh(InstaInputMesh);
			InstaLOD->DeallocMesh(InstaOutputMesh);

			FSkeletalMeshLODInfo* ReducedLODInfoPtr = SkeletalMesh->GetLODInfo(LODIndex);
			check(ReducedLODInfoPtr);

			// Do any joint-welding / bone removal.
			if (MeshBoneReductionInterface != nullptr && MeshBoneReductionInterface->GetBoneReductionData(SkeletalMesh, LODIndex, BonesToRemove))
			{
				// fix up chunks to remove the bones that set to be removed
				for (int32 SectionIndex=0; SectionIndex<NewModel->Sections.Num(); ++SectionIndex)
				{
					MeshBoneReductionInterface->FixUpSectionBoneMaps(NewModel->Sections[SectionIndex], BonesToRemove, NewModel->SkinWeightProfiles);
				}
			}

			if (bOldLodWasFromFile)
			{
				ReducedLODInfoPtr->LODMaterialMap.Empty();
			}

			// Flag this LOD as having been simplified.
			ReducedLODInfoPtr->bHasBeenSimplified = true;
			SkeletalMesh->SetHasBeenSimplified(true);

			//Restore the user sections data to what it was. It must be done if we want to avoid changing the DDC key. I.E. UserSectionData is part of the key
			//DDC key cannot be change during the build
			{
				FSkeletalMeshLODModel& ImportedModelLOD = SkeletalMesh->GetImportedModel()->LODModels[LODIndex];
				RestoreUserSectionsData(SrcModelBackup, ImportedModelLOD);

				if (!bLODModelAdded)
				{
					if (RestoreUserSectionsData(DstModelBackup, ImportedModelLOD))
					{
						//If its an existing LOD put back the buildStringID and all sections matched
						ImportedModelLOD.BuildStringID = BackupLodModelBuildStringID;
						ImportedModelLOD.RawSkeletalMeshBulkDataID = BackupRawSkeletalMeshBulkDataID;
					}
				}
			}

			PostProcessReducedSkeletalMesh(SkeletalMesh, *NewModel, InstaResult.MeshDeviation, Settings, FPlatformTime::Seconds() - T0);
		}
	}

	if (!bMeshIsReduced)
	{
		FSkeletalMeshLODModel::CopyStructure(NewModel, SrcModel);

		// Do any joint-welding / bone removal.
		if (MeshBoneReductionInterface != nullptr && MeshBoneReductionInterface->GetBoneReductionData(SkeletalMesh, LODIndex, BonesToRemove))
		{
			// fix up chunks to remove the bones that set to be removed
			for (int32 SectionIndex=0; SectionIndex<NewModel->Sections.Num(); ++SectionIndex)
			{
				MeshBoneReductionInterface->FixUpSectionBoneMaps(NewModel->Sections[SectionIndex], BonesToRemove, NewModel->SkinWeightProfiles);
			}
		}

		SkeletalMesh->GetLODInfo(LODIndex)->LODMaterialMap=SkeletalMesh->GetLODInfo(BaseLOD)->LODMaterialMap;

		// Required bones are recalculated later on.
		NewModel->RequiredBones.Empty();
		SkeletalMesh->GetLODInfo(LODIndex)->bHasBeenSimplified = true;
		SkeletalMesh->SetHasBeenSimplified(true);
	}

	if (!bLODModelAdded)
	{
		//Put back the clothing for this newly reduce LOD
		if (ClothingBindings.Num() > 0)
		{
			FLODUtilities::RestoreClothingFromBackup(SkeletalMesh, ClothingBindings, LODIndex);
		}
	}

	if (bPutBackRawMesh)
	{
		check(bReducingSourceModel || !bLODModelAdded);
		//Put back the original import data, we need it to allow inline reduction and skeletal mesh split workflow
		//It also warranty that we do not change the ddc key
		SkeletalMesh->SaveLODImportedData(LODIndex, RawMesh);
		SkeletalMesh->SetLODImportedDataVersions(LODIndex, GeoImportVersion, SkinningImportVersion);
	}

	SkeletalMesh->CalculateRequiredBones(SkeletalMeshResource->LODModels[LODIndex], SkeletalMesh->GetRefSkeleton(), &BonesToRemove);
}

void FInstaLOD::PostProcessReducedSkeletalMesh(USkeletalMesh* SkeletalMesh, UE_StaticLODModel& OutputLODModel, float& OutMaxDeviation,
											   const FSkeletalMeshOptimizationSettings& Settings, const float ElapsedTime)
{
	// NOTE: the max deviation has been scaled for auto LOD calculation, but we want to display the actual deviation
	const float DisplayMaxDeviation = OutMaxDeviation / CVarDecimatedErrorFactor.GetValueOnAnyThread();
	
	FText DetailText = FText::FromString(FString::Printf(TEXT("in %.2fs with %.3f mesh deviation"), ElapsedTime, DisplayMaxDeviation));
	FText NotificationText = FText::Format(LOCTEXT("LODPluginOptimizeComplete", "Built LOD {0}"), DetailText);
	
	UE_LOG(LogInstaLOD, Log, TEXT("%s"), *NotificationText.ToString());
}

static const char *const kInstaLODPageNameDiffuse = "UE_Diffuse";
static const char *const kInstaLODPageNameNormal = "UE_Normal";
static const char *const kInstaLODPageNameRoughness = "UE_Roughness";
static const char *const kInstaLODPageNameSpecular = "UE_Specular";
static const char *const kInstaLODPageNameMetallic = "UE_Metallic";
static const char *const kInstaLODPageNameEmissive = "UE_Emissive";
static const char *const kInstaLODPageNameOpacity = "UE_Opacity";
static const char *const kInstaLODPageNameOpacityMask = "UE_OpacityMask";
static const char *const kInstaLODPageNameSubSurface = "UE_SubSurface";
static const char *const kInstaLODPageNameAmbientOcclusion = "UE_AmbientOcclusion";

static const char *const kInstaLODPageInternalNameNormalTangentSpace = "NormalTangentSpace";
static const char *const kInstaLODPageInternalNameOpacity = "Opacity";
static const char *const kInstaLODPageInternalNameAmbientOcclusion = "AmbientOcclusion";


namespace InstaLOD
{
	struct InstaColorRGB16
	{
		uint16 R, G, B;
	};
	struct InstaColorRGBA8
	{
		uint8 R, G, B, A;
	};
	struct InstaColorR8L
	{
		uint8 L;
	};
}

class UEInstaLODMaterialHelper
{
public:
	static void CopyColorArrayAny(TArray<FColor>& PixelData, InstaLOD::IInstaLODTexturePage* InstaLODTexturePage)
	{
		check(InstaLODTexturePage);
		
		// NOTE: try to use the fast path if possible
		if (InstaLODTexturePage->GetComponentType() == InstaLOD::IInstaLODTexturePage::ComponentTypeUInt8 &&
			InstaLODTexturePage->GetPixelType() == InstaLOD::IInstaLODTexturePage::PixelTypeRGBA)
		{
			CopyColorArray8(PixelData, InstaLODTexturePage->GetData(nullptr), InstaLODTexturePage->GetWidth(), InstaLODTexturePage->GetHeight());
			return;
		}
		
		// NOTE: texture data always needs to be 8bpp RGBA
		const uint32 Width = InstaLODTexturePage->GetWidth();
		const uint32 Height = InstaLODTexturePage->GetHeight();
		
		PixelData.SetNumUninitialized(InstaLODTexturePage->GetWidth() * InstaLODTexturePage->GetHeight());
		
		FColor *OutData = PixelData.GetData();
		
		for (uint32 Y=0; Y<Height; Y++)
		{
			for(uint32 X=0; X<Width; X++)
			{
				const InstaLOD::InstaColorRGBAF32 Color = InstaLODTexturePage->SampleFloat(X, Height - (Y+1u));
				*OutData++ = FLinearColor(Color.R, Color.G, Color.B, Color.A).ToFColor(false);
			}
		}
		
	}
	
	static void CopyColorArray16(TArray<FColor>& OutData, const InstaLOD::uint8* InstaLODTexturePageData, const uint32 Width, const uint32 Height)
	{ 
		const uint32 NumElems = Width * Height;
		const InstaLOD::InstaColorRGB16* const InData = (InstaLOD::InstaColorRGB16*)InstaLODTexturePageData;
		OutData.SetNumUninitialized(NumElems);

		for (uint32 Y = 0; Y < Height; Y++)
		{
			for (uint32 X = 0; X < Width; X++)
			{
				const uint32 Index = X + Y * Width;
				const uint32 IndexFlippedY = X + Width * (Width - (Y + 1u));

				OutData[Index].R = FMath::Clamp((uint32)InData[IndexFlippedY].R * 255u / 65535u, 0u, 255u);
				OutData[Index].G = FMath::Clamp((uint32)InData[IndexFlippedY].G * 255u / 65535u, 0u, 255u);
				OutData[Index].B = FMath::Clamp((uint32)InData[IndexFlippedY].B * 255u / 65535u, 0u, 255u);
				OutData[Index].A = 255;
			}
		}
	}
	
	static void CopyColorArray8(TArray<FColor>& OutData, const InstaLOD::uint8* InstaLODTexturePageData, const uint32 Width, const uint32 Height)
	{
		const uint32 NumElems = Width * Height;
		const InstaLOD::InstaColorRGBA8 *InData = (InstaLOD::InstaColorRGBA8*)InstaLODTexturePageData;
		OutData.SetNumUninitialized(NumElems);
		
		for (uint32 Y=0; Y<Height; Y++)
		{
			for (uint32 X=0; X<Width; X++)
			{
				const uint32 Index = X + Y * Width;
				const uint32 IndexFlippedY = X + Width * (Width - (Y+1u));

				OutData[Index].R = InData[IndexFlippedY].R;
				OutData[Index].G = InData[IndexFlippedY].G;
				OutData[Index].B = InData[IndexFlippedY].B;
				OutData[Index].A = InData[IndexFlippedY].A;
			}
		}
	}
	
	static void CopyColorArray(InstaLOD::uint8* OutInstaLODTexturePageData, const TArray<FColor>& InData, const uint32 Width, const uint32 Height)
	{
		const size_t NumElems = InData.Num();
		// NOTE: UE texture pages are always uint8, RGBA
		InstaLOD::InstaColorRGBA8* const OutData = (InstaLOD::InstaColorRGBA8*) OutInstaLODTexturePageData;

		for (uint32 Y = 0; Y < Height; Y++)
		{
			for (uint32 X = 0; X < Width; X++)
			{
				const uint32 Index = X + Y * Width;
				const uint32 IndexFlippedY = X + Width * (Width - (Y + 1u));

				OutData[Index].R = InData[IndexFlippedY].R;
				OutData[Index].G = InData[IndexFlippedY].G;
				OutData[Index].B = InData[IndexFlippedY].B;
				OutData[Index].A = InData[IndexFlippedY].A;
			}
		}
	}
	
	static void SetColorArrayToConstant(TArray<FColor>& OutData, const FColor& Value)
	{
		OutData.SetNum(1);
		const size_t NumElems = OutData.Num();
		
		for(size_t Index=0; Index<NumElems; Index++)
		{
			OutData[Index] = Value;
		}
	}
	
#	define GetFFlattenMaterialPageSize(MATERIAL, PAGENAME) (MATERIAL).GetPropertySize( EFlattenMaterialProperties :: PAGENAME)
#	define SetFFlattenMaterialPageSize(MATERIAL, PAGENAME, VALUE) (MATERIAL).SetPropertySize( EFlattenMaterialProperties :: PAGENAME, (VALUE))
#	define GetFFlattenMaterialPageData(MATERIAL, PAGENAME) (MATERIAL).GetPropertySamples( EFlattenMaterialProperties :: PAGENAME)
		
	static void ConvertInstaLODMaterialToFlattenMaterial(InstaLOD::IInstaLODMaterial *const InstaMaterial, FFlattenMaterial &OutMaterial, const IInstaLOD::UE_MaterialProxySettings& settings)
	{
		uint32 PageWidth, PageHeight;
		OutMaterial = FMaterialUtilities::CreateFlattenMaterialWithSettings(settings);
		FColor DefaultColor(255, 255, 255, 255);
		FColor DefaultNormalColor(127, 127, 127, 255);
		
		InstaLOD::IInstaLODTexturePage *InstaLODTexturePage = nullptr;
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameDiffuse);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, Diffuse, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, Diffuse), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		else
		{
			SetColorArrayToConstant(GetFFlattenMaterialPageData(OutMaterial, Diffuse), DefaultColor);
		}
		
		// NOTE: check if a built-in normal map sampler is available
		if (InstaMaterial != nullptr && InstaMaterial->GetTexturePage(kInstaLODPageInternalNameNormalTangentSpace))
		{
			InstaLODTexturePage = InstaMaterial->GetTexturePage(kInstaLODPageInternalNameNormalTangentSpace);
			
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, Normal, FIntPoint(PageWidth, PageHeight));
			CopyColorArray16(GetFFlattenMaterialPageData(OutMaterial, Normal), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		else
		{
			InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameNormal);
			if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
			{
				InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
				SetFFlattenMaterialPageSize(OutMaterial, Normal, FIntPoint(PageWidth, PageHeight));
				CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, Normal), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
			}
			else
			{
				SetFFlattenMaterialPageSize(OutMaterial, Normal, FIntPoint(1,1));
				SetColorArrayToConstant(GetFFlattenMaterialPageData(OutMaterial, Normal), DefaultNormalColor);
			}
		}
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameRoughness);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, Roughness, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, Roughness), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		else
		{
			FLinearColor Roughness(settings.RoughnessConstant, settings.RoughnessConstant, settings.RoughnessConstant);
			SetFFlattenMaterialPageSize(OutMaterial, Roughness, FIntPoint(1,1));
			SetColorArrayToConstant(GetFFlattenMaterialPageData(OutMaterial, Roughness), Roughness.ToFColor(true));
		}
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameMetallic);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, Metallic, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, Metallic), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		else
		{
			FLinearColor Metallic(settings.MetallicConstant, settings.MetallicConstant, settings.MetallicConstant);
			SetFFlattenMaterialPageSize(OutMaterial, Metallic, FIntPoint(1,1));
			SetColorArrayToConstant(GetFFlattenMaterialPageData(OutMaterial, Metallic), Metallic.ToFColor(true));
		}
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameSpecular);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, Specular, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, Specular), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		else
		{
			FLinearColor Specular(settings.SpecularConstant, settings.SpecularConstant, settings.SpecularConstant);
			SetFFlattenMaterialPageSize(OutMaterial, Specular, FIntPoint(1,1));
			SetColorArrayToConstant(GetFFlattenMaterialPageData(OutMaterial, Specular), Specular.ToFColor(true));
		}
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameEmissive);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, Emissive, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, Emissive), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameOpacity);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, Opacity, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, Opacity), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameSubSurface);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, SubSurface, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, SubSurface), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		
		// NOTE: the internally generated opacity map is more important!
		{
			// convert internal opacity texture (8bpp luminance)
			InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageInternalNameOpacity);
			if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
			{
				InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
				SetFFlattenMaterialPageSize(OutMaterial, OpacityMask, FIntPoint(PageWidth, PageHeight));
				CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, OpacityMask), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
			}
			else
			{
				InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameOpacityMask);
				if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
				{
					InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
					SetFFlattenMaterialPageSize(OutMaterial, OpacityMask, FIntPoint(PageWidth, PageHeight));
					CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, OpacityMask), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
				}
			}
		}
		
		InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageNameAmbientOcclusion);
		if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
		{
			InstaLODTexturePage->GetSize(&PageWidth, &PageHeight);
			SetFFlattenMaterialPageSize(OutMaterial, AmbientOcclusion, FIntPoint(PageWidth, PageHeight));
			CopyColorArray8(GetFFlattenMaterialPageData(OutMaterial, AmbientOcclusion), InstaLODTexturePage->GetData(nullptr), PageWidth, PageHeight);
		}
		else
		{
			// convert internal ambient occlusion texture (8bpp luminance)
			InstaLODTexturePage = InstaMaterial == nullptr ? nullptr : InstaMaterial->GetTexturePage(kInstaLODPageInternalNameAmbientOcclusion);
			if (InstaLODTexturePage != nullptr && InstaLODTexturePage->IsValid())
			{
				SetFFlattenMaterialPageSize(OutMaterial, AmbientOcclusion, FIntPoint(InstaLODTexturePage->GetWidth(), InstaLODTexturePage->GetHeight()));
				CopyColorArrayAny(GetFFlattenMaterialPageData(OutMaterial, AmbientOcclusion), InstaLODTexturePage);
			}
		}
	}
	
	static void ConvertFlattenMaterialPageToInstaTexturePage(const TArray<FColor>& InData, const FIntPoint& InSize, const char* const InName,
															  InstaLOD::IInstaLODMaterialData *const MaterialData, InstaLOD::IInstaLODMaterial *const OutInstaMaterial)
	{
		// do not create empty pages
		if (InData.Num() == 0)
			return;
		
		const InstaLOD::IInstaLODTexturePage::ComponentType ComponentType = InstaLOD::IInstaLODTexturePage::ComponentTypeUInt8;
		const InstaLOD::IInstaLODTexturePage::PixelType PixelType = InstaLOD::IInstaLODTexturePage::PixelTypeRGBA;
		InstaLOD::IInstaLODTexturePage::Type TexturePageType = InstaLOD::IInstaLODTexturePage::TypeColor;
		
		// NOTE: we transfer all texture pages as simple color texture pages, the only exception being tangnetspace normal maps
		if (InName == kInstaLODPageNameNormal)
			TexturePageType = InstaLOD::IInstaLODTexturePage::TypeNormalMapTangentSpace;
		else if (InName == kInstaLODPageNameOpacityMask || InName == kInstaLODPageNameOpacity)
			TexturePageType = InstaLOD::IInstaLODTexturePage::TypeOpacity;
		else if (InName == kInstaLODPageNameAmbientOcclusion)
			TexturePageType = InstaLOD::IInstaLODTexturePage::TypeAmbientOcclusion;
		else if (InName == kInstaLODPageNameRoughness)
			TexturePageType = InstaLOD::IInstaLODTexturePage::TypeRoughness;
		else if (InName == kInstaLODPageNameMetallic)
			TexturePageType = InstaLOD::IInstaLODTexturePage::TypeMetalness;
		else if (InName == kInstaLODPageNameSpecular)
			TexturePageType = InstaLOD::IInstaLODTexturePage::TypeSpecular;
		else if (InName == kInstaLODPageNameEmissive)
			TexturePageType = InstaLOD::IInstaLODTexturePage::TypeEmissive;
		
		InstaLOD::IInstaLODTexturePage *const TexturePage = OutInstaMaterial->AddTexturePage(InName, TexturePageType, ComponentType, PixelType);
		TexturePage->Reallocate(InSize.X, InSize.Y);
		CopyColorArray(TexturePage->GetData(nullptr), InData, InSize.X, InSize.Y);
		
		if (TexturePageType == InstaLOD::IInstaLODTexturePage::TypeOpacity)
		{
			// ensure alpha values are setup
			const uint32 ElementCount = InSize.X * InSize.Y;
			
			InstaLOD::InstaColorRGBA8* OutData = (InstaLOD::InstaColorRGBA8*)TexturePage->GetData(nullptr);
			for (uint32 ElementIndex=0u; ElementIndex<ElementCount; ElementIndex++)
			{
				OutData->A = OutData->R;
				OutData++;
			}
			
			OutInstaMaterial->SetUseTexturePageAsAlphaMask(TexturePage);
		}
		else if (TexturePageType == InstaLOD::IInstaLODTexturePage::TypeNormalMapTangentSpace)
		{
			// Ensure OpenGL tangent space map
			const uint32 ElementCount = InSize.X * InSize.Y;

			InstaLOD::InstaColorRGBA8* OutData = (InstaLOD::InstaColorRGBA8*)TexturePage->GetData(nullptr);

			for (uint32 ElementIndex=0u; ElementIndex<ElementCount; ElementIndex++)
			{
				OutData->G = 1.0f - OutData->G;
				OutData++;
			}
		}
		
		// enable writing of all texture page matching the specified name and it's exact specification
		MaterialData->EnableOutputForTexturePage(InName, TexturePageType, ComponentType, PixelType);
	}
	
	static void ConvertFlattenMaterialToInstaMaterial(const FFlattenMaterial& InMaterial, InstaLOD::IInstaLODMaterialData *const MaterialData, InstaLOD::IInstaLODMaterial *const OutInstaMaterial)
	{
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, Diffuse), GetFFlattenMaterialPageSize(InMaterial, Diffuse), kInstaLODPageNameDiffuse, MaterialData, OutInstaMaterial);
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, Normal), GetFFlattenMaterialPageSize(InMaterial, Normal), kInstaLODPageNameNormal, MaterialData, OutInstaMaterial);
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, Roughness), GetFFlattenMaterialPageSize(InMaterial, Roughness), kInstaLODPageNameRoughness, MaterialData, OutInstaMaterial);
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, Metallic), GetFFlattenMaterialPageSize(InMaterial, Metallic), kInstaLODPageNameMetallic, MaterialData, OutInstaMaterial);
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, Specular), GetFFlattenMaterialPageSize(InMaterial, Specular), kInstaLODPageNameSpecular, MaterialData, OutInstaMaterial);

		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, Emissive), GetFFlattenMaterialPageSize(InMaterial, Emissive), kInstaLODPageNameEmissive, MaterialData, OutInstaMaterial);
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, Opacity), GetFFlattenMaterialPageSize(InMaterial, Opacity), kInstaLODPageNameOpacity, MaterialData, OutInstaMaterial);
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, SubSurface), GetFFlattenMaterialPageSize(InMaterial, SubSurface), kInstaLODPageNameSubSurface, MaterialData, OutInstaMaterial);

		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, OpacityMask), GetFFlattenMaterialPageSize(InMaterial, OpacityMask), kInstaLODPageNameOpacityMask, MaterialData, OutInstaMaterial);
		ConvertFlattenMaterialPageToInstaTexturePage(GetFFlattenMaterialPageData(InMaterial, AmbientOcclusion), GetFFlattenMaterialPageSize(InMaterial, AmbientOcclusion), kInstaLODPageNameAmbientOcclusion, MaterialData, OutInstaMaterial);
	}
};

struct UEProxyWrapper
{
	UEProxyWrapper(InstaLOD::IInstaLOD* InInstaLOD, const bool InIsMergeOperation) :
	InstaLOD(InInstaLOD),
	InstaMaterial(nullptr),
	MergeOperation(nullptr),
	RemeshOperation(nullptr)
	{
		check(InInstaLOD);
		
		if (InIsMergeOperation)
		{
			MergeOperation = InstaLOD->AllocMeshMergeOperation();
		}
		else
		{
			RemeshOperation = InstaLOD->AllocRemeshingOperation();
		}
	}
	
	~UEProxyWrapper()
	{
		Dealloc();
	}
	
	void Dealloc()
	{
		if (MergeOperation)
		{
			InstaLOD->DeallocMeshMergeOperation(MergeOperation);
			MergeOperation = nullptr;
		}
		if (RemeshOperation)
		{
			InstaLOD->DeallocRemeshingOperation(RemeshOperation);
			RemeshOperation = nullptr;
		}
		
		InstaMaterial = nullptr;
		InstaLOD = nullptr;
	}
	
	void AddMesh(InstaLOD::IInstaLODMesh *InstaMesh)
	{
		check(InstaMesh);
		check(InstaLOD);
		
		if (MergeOperation)
		{
			MergeOperation->AddMesh(InstaMesh);
		}
		else
		{
			RemeshOperation->AddMesh(InstaMesh);
		}
	}
	
	void SetMaterialData(InstaLOD::IInstaLODMaterialData *MaterialData)
	{
		check(MaterialData);
		check(InstaLOD);
		
		if (MergeOperation)
		{
			MergeOperation->SetMaterialData(MaterialData);
		}
		else
		{
			RemeshOperation->SetMaterialData(MaterialData);
		}
	}
	
	bool Execute(InstaLOD::IInstaLODMesh *OutputMesh, const struct FMeshProxySettings& InProxySettings)
	{
		check(InstaLOD);
		
		InstaMaterial = nullptr;
		
		if (MergeOperation != nullptr)
		{
			InstaLOD::MeshMergeSettings MeshMergeSettings;
			MeshMergeSettings.SolidifyTexturePages = true;
			// NOTE: we super sample at 2x for UE by default. Unfortunately UE does not provide an API for this yet
			MeshMergeSettings.SuperSampling = InstaLOD::SuperSampling::X2;
			MeshMergeSettings.GutterSizeInPixels = 5;
			MeshMergeSettings.StackDuplicateShells = true;
			MeshMergeSettings.Deterministic = CVarProxyDeterministic.GetValueOnAnyThread() > 0 ? true : false;

			InstaLOD::MeshMergeResult MergeResult = MergeOperation->Execute(OutputMesh, MeshMergeSettings);
			
			if (CVarAssertOnKeyMesh.GetValueOnAnyThread() != 0)
			{
				if (!MergeResult.IsAuthorized)
				{
					UE_LOG(LogInstaLOD, Fatal, TEXT("%s"), *InstaLODAssertOnKeyMeshMessage);
				}
			}

			if (!MergeResult.Success)
				return false;
			
			// also optimize with InstaLOD
			if (CVarHLODScreenSizeFactor.GetValueOnAnyThread() > 0.0f)
			{
				InstaLOD::OptimizeSettings OptimizeSettings = UEInstaLODMeshHelper::ConvertMeshReductionSettingsToInstaLOD(FMeshReductionSettings());
				OptimizeSettings.ScreenSizeInPixels = InProxySettings.ScreenSize * CVarHLODScreenSizeFactor.GetValueOnAnyThread();
				OptimizeSettings.RecalculateNormals = InProxySettings.bRecalculateNormals;
				OptimizeSettings.HardAngleThreshold = InProxySettings.HardAngleThreshold;
				OptimizeSettings.Deterministic = CVarProxyDeterministic.GetValueOnAnyThread() > 0 ? true : false;
				
				// NOTE: as we're only optimizing a single mesh, we're using the shortcut API
				// if you're optimizing multiple objects and need progress information use IInstaLOD::AllocOptimizeOperation
				InstaLOD::OptimizeResult OptimizeResult = InstaLOD->Optimize(OutputMesh, OutputMesh, OptimizeSettings);

				if (CVarAssertOnKeyMesh.GetValueOnAnyThread() != 0)
				{
					if (!OptimizeResult.IsAuthorized)
					{
						UE_LOG(LogInstaLOD, Fatal, TEXT("%s"), *InstaLODAssertOnKeyMeshMessage);
					}
				}

				if (OptimizeResult.Success == false)
					return false;
			}
			
			InstaMaterial = MergeResult.MergeMaterial;
			return true;
		}
		else
		{
			InstaLOD::RemeshingSettings RemeshSettings;
			RemeshSettings.ScreenSizeInPixels = InProxySettings.ScreenSize;
			RemeshSettings.HardAngleThreshold = InProxySettings.HardAngleThreshold;
			RemeshSettings.BakeOutput.TangentSpaceFormat = InstaLOD::MeshFormat::DirectX;
			RemeshSettings.BakeOutput.SuperSampling = InstaLOD::SuperSampling::X2;
			RemeshSettings.BakeAutomaticRayLengthFactor = 1.5f;
			RemeshSettings.ScreenSizePixelMergeDistance = InProxySettings.MergeDistance;
			RemeshSettings.BakeEngine = InstaLOD::BakeEngine::CPU;
			
			// NOTE: disable 'SurfaceConstructionIgnoreBackface' to switch to standard remeshing
			// behavior where non-watertight meshes will result in a mesh with interior/backface
			// geometry
			// enabling 'SurfaceConstructionIgnoreBackface' will instruct InstaLOD to construct
			// a surface without interior faces even for non-watertight meshes - however
			// if the surface contains flipped faces the resulting mesh may contain holes.
			RemeshSettings.SurfaceConstructionIgnoreBackface = true;
			RemeshSettings.ScreenSizeInPixelsAutomaticTextureSize = InProxySettings.MaterialSettings.TextureSizingType == TextureSizingType_UseSimplygonAutomaticSizing;

			RemeshSettings.BakeOutput.TexturePageNormalTangentSpace = true;
			RemeshSettings.BakeOutput.TexturePageNormalObjectSpace = false;
			RemeshSettings.Deterministic = CVarProxyDeterministic.GetValueOnAnyThread() > 0 ? true : false;

			InstaLOD::RemeshingResult RemeshResult = RemeshOperation->Execute(OutputMesh, RemeshSettings);

			if (CVarAssertOnKeyMesh.GetValueOnAnyThread() != 0)
			{
				if (!RemeshResult.IsAuthorized)
				{
					UE_LOG(LogInstaLOD, Fatal, TEXT("%s"), *InstaLODAssertOnKeyMeshMessage);
				}
			}

			if (RemeshResult.Success == false)
				return false;
			
			InstaMaterial = RemeshResult.BakeMaterial;
			return true;
		}
	}
	InstaLOD::IInstaLOD *InstaLOD;
	InstaLOD::IInstaLODMaterial* InstaMaterial;
	InstaLOD::IMeshMergeOperation2 *MergeOperation;
	InstaLOD::IRemeshingOperation *RemeshOperation;
};

InstaLOD::IInstaLODMesh* FInstaLOD::AllocInstaLODMesh()
{
	return InstaLOD->AllocMesh();
}

InstaLOD::IInstaLODSkeleton* FInstaLOD::AllocInstaLODSkeleton()
{
	return InstaLOD->AllocSkeleton();
}

bool FInstaLOD::ConvertInstaLODMeshToRawMesh(InstaLOD::IInstaLODMesh* InMesh, struct FRawMesh &OutMesh)
{
	check(InMesh);
	UEInstaLODMeshHelper::InstaLODMeshToRawMesh(InMesh, OutMesh);
	return true;
}

bool FInstaLOD::ConvertInstaLODMeshToMeshDescription(InstaLOD::IInstaLODMesh* InMesh, const TMap<int32, FName> &MaterialMapOut, struct FMeshDescription &OutMesh)
{
	check(InMesh);
	UEInstaLODMeshHelper::InstaLODMeshToMeshDescription(InMesh, MaterialMapOut, OutMesh);
	return true;
}
 
bool FInstaLOD::ConvertMeshDescriptionToInstaLODMesh(const struct FMeshDescription &InMesh, InstaLOD::IInstaLODMesh* OutMesh)
{
	check(OutMesh);
	TMap <FName, int32> map;
	UEInstaLODMeshHelper::MeshDescriptionToInstaLODMesh(InMesh, map, OutMesh);
	return true;
}

bool FInstaLOD::ConvertSkeletalLODModelToInstaLODMesh(const UE_StaticLODModel& InMesh, InstaLOD::IInstaLODMesh *const OutMesh, UE_SkeletalBakePoseData *const BakePoseData)
{
	check(OutMesh);
	TArray<uint32> SkipSections = UEInstaLODSkeletalMeshHelper::GetSkipSectionsForLODModel(static_cast<const FSkeletalMeshLODModel*>(&InMesh));
	UEInstaLODSkeletalMeshHelper::SkeletalLODModelToInstaLODMesh(InMesh, OutMesh, SkipSections, BakePoseData);
	return true;
}

bool FInstaLOD::ConvertInstaLODMeshToSkeletalLODModel(InstaLOD::IInstaLODMesh *const InMesh, class USkeletalMesh* SourceSkeletalMesh, class FSkeletalMeshImportData& ImportMesh, UE_StaticLODModel& OutMesh)
{
	check(InMesh);
	check(SourceSkeletalMesh);
	
	IInstaLOD::UE_MeshBuildOptions BuildOptions;
	BuildOptions.bRemoveDegenerateTriangles = false; /**< InstaLOD takes care of mesh healing */
	BuildOptions.bUseMikkTSpace = false;
	BuildOptions.bComputeNormals = false;
	BuildOptions.bComputeTangents = false;
	
	UEInstaLODSkeletalMeshHelper::InstaLODMeshToSkeletalLODModel(InMesh, SourceSkeletalMesh, OutMesh, ImportMesh, BuildOptions);
	return true;
}

bool FInstaLOD::ConvertFlattenMaterialsToInstaLODMaterialData(const TArray<FFlattenMaterial>& InputMaterials, InstaLOD::IInstaLODMaterialData* MaterialData, const IInstaLOD::UE_MaterialProxySettings& MaterialSettings)
{
	check(MaterialData);
	
	// setup default size
	{
		FIntPoint TextureSize;
		TextureSize = MaterialSettings.TextureSize;
		MaterialData->SetDefaultOutputTextureSize(TextureSize.X, TextureSize.Y);
	}

	// setup texture page default colors
	{
		InstaLOD::InstaColorRGBAF32 kDefaultColorWhite;
		kDefaultColorWhite.R = kDefaultColorWhite.G = kDefaultColorWhite.B = kDefaultColorWhite.A = 1.0f;
		InstaLOD::InstaColorRGBAF32 kDefaultColorBlack;
		kDefaultColorBlack.R = kDefaultColorBlack.G = kDefaultColorBlack.B = 0.0f; kDefaultColorBlack.A = 1.0f;
		
		MaterialData->SetDefaultColorForTexturePage(kInstaLODPageNameOpacity, kDefaultColorWhite);
		MaterialData->SetDefaultColorForTexturePage(kInstaLODPageNameOpacityMask, kDefaultColorWhite);
		MaterialData->SetDefaultColorForTexturePage(kInstaLODPageNameAmbientOcclusion, kDefaultColorWhite);
		MaterialData->SetDefaultColorForTexturePage(kInstaLODPageNameEmissive, kDefaultColorBlack);
	}
	
	if (MaterialSettings.TextureSizingType == TextureSizingType_UseAutomaticBiasedSizes ||
		MaterialSettings.TextureSizingType == TextureSizingType_UseSimplygonAutomaticSizing)
	{
		int32 NormalSize = MaterialSettings.TextureSize.X;
		int32 DiffuseSize = FMath::Max(MaterialSettings.TextureSize.X >> 1, 32);
		int32 PropertiesSize = FMath::Max(MaterialSettings.TextureSize.X >> 2, 16);
		
		// NOTE: we can specify all page sizes here, it doesn't matter if they're not actually exported, InstaLOD will ignore it then
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameDiffuse, DiffuseSize, DiffuseSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameNormal, NormalSize, NormalSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameRoughness, PropertiesSize, PropertiesSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameMetallic, PropertiesSize, PropertiesSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameSpecular, PropertiesSize, PropertiesSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameEmissive, PropertiesSize, PropertiesSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameOpacity, PropertiesSize, PropertiesSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameOpacityMask, PropertiesSize, PropertiesSize);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameAmbientOcclusion, PropertiesSize, PropertiesSize);
		
		// NOTE: set internal sampler size
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageInternalNameNormalTangentSpace, NormalSize, NormalSize);
	}
	else if (MaterialSettings.TextureSizingType == TextureSizingType_UseManualOverrideTextureSize)
	{
		// NOTE: we can specify all page sizes here, it doesn't matter if they're not actually exported, InstaLOD will ignore it then
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameDiffuse, MaterialSettings.DiffuseTextureSize.X, MaterialSettings.DiffuseTextureSize.Y);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameNormal, MaterialSettings.NormalTextureSize.X, MaterialSettings.NormalTextureSize.Y);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameRoughness, MaterialSettings.RoughnessTextureSize.X, MaterialSettings.RoughnessTextureSize.Y);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameMetallic, MaterialSettings.MetallicTextureSize.X, MaterialSettings.MetallicTextureSize.Y);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameSpecular, MaterialSettings.SpecularTextureSize.X, MaterialSettings.SpecularTextureSize.Y);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameEmissive, MaterialSettings.EmissiveTextureSize.X, MaterialSettings.EmissiveTextureSize.Y);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameOpacity, MaterialSettings.OpacityTextureSize.X, MaterialSettings.OpacityTextureSize.Y);

		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameOpacityMask, MaterialSettings.OpacityMaskTextureSize.X, MaterialSettings.OpacityMaskTextureSize.Y);
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageNameAmbientOcclusion, MaterialSettings.AmbientOcclusionTextureSize.X, MaterialSettings.AmbientOcclusionTextureSize.Y);

		// NOTE: set internal sampler size
		MaterialData->SetOutputTextureSizeForPage(kInstaLODPageInternalNameNormalTangentSpace, MaterialSettings.NormalTextureSize.X, MaterialSettings.NormalTextureSize.Y);
	}
	
	// NOTE: it's important that material IDs match those specified in the meshes
	for (int32 MaterialIndex=0; MaterialIndex<InputMaterials.Num(); MaterialIndex++)
	{
		FString MaterialName = FString::Printf(TEXT("INSTALOD_TEMP_MATERIAL_%i"), MaterialIndex);
		InstaLOD::IInstaLODMaterial *const InstaMaterial = MaterialData->AddMaterialWithID(TCHAR_TO_ANSI(*MaterialName), MaterialIndex);
		UEInstaLODMaterialHelper::ConvertFlattenMaterialToInstaMaterial(InputMaterials[MaterialIndex], MaterialData, InstaMaterial);
	}
	
	return true;
}

bool FInstaLOD::ConvertInstaLODMaterialToFlattenMaterial(InstaLOD::IInstaLODMaterial* InMaterial, FFlattenMaterial &OutMaterial, const UE_MaterialProxySettings& MaterialSettings)
{
	check(InMaterial)
	UEInstaLODMaterialHelper::ConvertInstaLODMaterialToFlattenMaterial(InMaterial, OutMaterial, MaterialSettings);
	return true;
}

bool FInstaLOD::ConvertReferenceSkeletonToInstaLODSkeleton(const FReferenceSkeleton& ReferenceSkeleton, InstaLOD::IInstaLODSkeleton *const InstaSkeleton, TMap<int32, TPair<uint32, FString>>& OutUEBoneIndexToInstaLODBoneIndexAndName)
{
	return UEInstaLODSkeletalMeshHelper::ReferenceSkeletonToInstaLODSkeleton(ReferenceSkeleton, InstaSkeleton, OutUEBoneIndexToInstaLODBoneIndexAndName);
}

void FInstaLOD::ProxyLOD(
	const TArray<struct FMeshMergeData>& InData, 
	const struct FMeshProxySettings& InProxySettings,
	const TArray<struct FFlattenMaterial>& InputMaterials, 
	const FGuid InJobGUID)
{
	UE_LOG(LogInstaLOD, Log, TEXT("Building Proxy"));
	
	if (IsRunningCommandlet() && !InstaLOD->IsHostAuthorized())
	{
		UE_LOG(LogInstaLOD, Fatal, TEXT("This machine is not authorized to run InstaLOD. Please authorize this machine before cooking using the editor or InstaLOD Pipeline."));
	}
	
	const double T0 = FPlatformTime::Seconds();
	
	UEProxyWrapper InstaOperation(InstaLOD, CVarHLODRemesh.GetValueOnAnyThread() == 0);
	
	// setup material data
	InstaLOD::IInstaLODMaterialData *const MaterialData = InstaLOD->AllocMaterialData();
	InstaOperation.SetMaterialData(MaterialData);

	IInstaLOD::UE_MaterialProxySettings MaterialSettings;
	MaterialSettings = InProxySettings.MaterialSettings;
	
	ConvertFlattenMaterialsToInstaLODMaterialData(InputMaterials, MaterialData, MaterialSettings);
	
	TArray<InstaLOD::IInstaLODMesh*> SourceInstaMeshes;

	for(int32 DataIndex=0; DataIndex<InData.Num(); DataIndex++)
	{
		TMap<FName, int32> InMaterialMap;
		TMap<int32, FName> OutMaterialMap;

		UEInstaLODMeshHelper::CreateInputOutputMaterialMapFromMeshDescription(*InData[DataIndex].RawMesh, InMaterialMap, OutMaterialMap);
		InstaLOD::IInstaLODMesh *const InstaMesh = AllocInstaLODMesh();
		UEInstaLODMeshHelper::MeshDescriptionToInstaLODMesh(*InData[DataIndex].RawMesh, InMaterialMap, InstaMesh);

		// replace custom UVs when vertex colors needed baking
		if (InData[DataIndex].NewUVs.Num() > 0)
		{
			InstaLOD::uint64 NumTexCoordElems;
			InstaLOD::InstaVec2F *OutTexcoords = InstaMesh->GetWedgeTexCoords(0, &NumTexCoordElems);
			
			if (NumTexCoordElems == InData[DataIndex].NewUVs.Num())
			{
				InstaMesh->ReverseFaceDirections(/*flipNormals:*/ false);

				// we assume that these are for texcoord0
				UEInstaLODMeshHelper::FillArray(OutTexcoords, InData[DataIndex].NewUVs.GetData(), NumTexCoordElems);
				UEInstaLODMeshHelper::SanitizeFloatArray((float*)OutTexcoords, NumTexCoordElems * 2, 0.0f);

				InstaMesh->ReverseFaceDirections(/*flipNormals:*/ false);
			}
		}
		InstaOperation.AddMesh(InstaMesh);
		SourceInstaMeshes.Add(InstaMesh);
	}
	
	// create output mesh
	InstaLOD::IInstaLODMesh *const OutputMesh = AllocInstaLODMesh();
	
	FMeshDescription OutProxyMesh;
	FStaticMeshAttributes(OutProxyMesh).Register();
	FFlattenMaterial OutMaterial; 

	// execute InstaLOD merge operation
	bool InstaOperationSuccess = InstaOperation.Execute(OutputMesh, InProxySettings);
	
	if (!InstaOperationSuccess)
	{
		char InstaLog[8192];
		InstaLOD->GetMessageLog(InstaLog, sizeof(InstaLog), nullptr);
		UE_LOG(LogInstaLOD, Error, TEXT("Remeshing failed. Log: %s"), UTF8_TO_TCHAR(InstaLog));
	}
	else
	{
		// store merge mesh result in output FRawMesh 
		TMap<int32, FName> MaterialMap;
		ConvertInstaLODMeshToMeshDescription(OutputMesh, MaterialMap, OutProxyMesh);
		// store merge material result in output FFlattenMaterial
		ConvertInstaLODMaterialToFlattenMaterial(InstaOperation.InstaMaterial, OutMaterial, MaterialSettings);
	}
	
	const double T1 = FPlatformTime::Seconds();
	const float ElapsedTime = (float)(T1-T0);
	
	TMap<int32, FName> OutMaterialMap;
	OutMaterialMap.Add(0, FName(*FString::Printf(TEXT("MaterialSlot_%d"), 0)));

	PostProcessMergedRawMesh(OutProxyMesh, OutMaterial, InProxySettings, ElapsedTime);
	
	// free data
	for (int32 MeshIndex=0; MeshIndex<SourceInstaMeshes.Num(); MeshIndex++)
	{
		InstaLOD->DeallocMesh(SourceInstaMeshes[MeshIndex]);
	}
	InstaLOD->DeallocMesh(OutputMesh);
	InstaLOD->DeallocMaterialData(MaterialData);
	InstaOperation.Dealloc();
	
	CompleteDelegate.ExecuteIfBound(OutProxyMesh, OutMaterial, InJobGUID);
}

void FInstaLOD::PostProcessMergedRawMesh(FMeshDescription& OutProxyMesh, FFlattenMaterial& OutMaterial, const FMeshProxySettings& InProxySettings, const float ElapsedTime)
{
	IInstaLOD::UE_MaterialProxySettings MaterialSettings;
	MaterialSettings = InProxySettings.MaterialSettings;
	
	// setup material constants
	auto MetallicSamples = GetFFlattenMaterialPageData(OutMaterial, Metallic);
	if (MetallicSamples.Num() == 0)
	{
		FLinearColor Metallic(MaterialSettings.MetallicConstant, MaterialSettings.MetallicConstant, MaterialSettings.MetallicConstant);
		SetFFlattenMaterialPageSize(OutMaterial, Metallic, FIntPoint(1,1));
		MetallicSamples.SetNum(1);
		MetallicSamples[0] = Metallic.ToFColor(true);
	}
	
	auto RoughnessSamples = GetFFlattenMaterialPageData(OutMaterial, Roughness);
	if (RoughnessSamples.Num() == 0)
	{
		FLinearColor Roughness(MaterialSettings.RoughnessConstant, MaterialSettings.RoughnessConstant, MaterialSettings.RoughnessConstant);
		SetFFlattenMaterialPageSize(OutMaterial, Roughness, FIntPoint(1,1));
		RoughnessSamples.SetNum(1);
		RoughnessSamples[0] = Roughness.ToFColor(true);
	}
	
	auto SpecularSamples = GetFFlattenMaterialPageData(OutMaterial, Specular);
	if (SpecularSamples.Num() == 0)
	{
		FLinearColor Specular(MaterialSettings.SpecularConstant, MaterialSettings.SpecularConstant, MaterialSettings.SpecularConstant);
		SetFFlattenMaterialPageSize(OutMaterial, Specular, FIntPoint(1,1));
		SpecularSamples.SetNum(1);
		SpecularSamples[0] = Specular.ToFColor(true);
	}
	
	const FText DetailText = FText::FromString(FString::Printf(TEXT("in %.2fs"), ElapsedTime));
	const FText NotificationText = FText::Format(LOCTEXT("LODPluginMergeComplete", "Built Proxy LOD {0}"), DetailText);
	
	UE_LOG(LogInstaLOD, Log, TEXT("%s"), *NotificationText.ToString());
}

void FInstaLOD::AggregateLOD()
{
}

void FInstaLOD::DispatchNotification(const FText& NotificationText, /*SNotificationItem::ECompletionState*/int32 type)
{
	if (GEditor == nullptr)
		return;
	
	// send a visual notification to the user
	FNotificationInfo Info(NotificationText);
	Info.ExpireDuration = 4.0f;
	Info.bAllowThrottleWhenFrameRateIsLow = true;
	
	SNotificationItem::ECompletionState state = (SNotificationItem::ECompletionState)type;
	
	// NOTE: we will create a lambda that will dispatch the notification, else it will disappear immediately due to the huge timeout
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda([Info, state]() {
		TWeakPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		
		if (Notification.IsValid())
		{
			Notification.Pin()->SetCompletionState(state);
		}
		
	});
	GEditor->GetTimerManager()->SetTimerForNextTick(TimerDelegate);
}


#undef LOCTEXT_NAMESPACE
