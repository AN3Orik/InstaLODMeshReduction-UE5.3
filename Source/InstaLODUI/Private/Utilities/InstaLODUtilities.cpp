/**
 * InstaLODUtilities.cpp (InstaLOD)
 *
 * Copyright 2016-2019 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODUtilities.cpp
 * @copyright 2016-2019 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#include "Utilities/InstaLODUtilities.h"
#include "InstaLODUIPCH.h"

#include "InstaLODModule.h"
#include "Tools/InstaLODBaseTool.h"

#include "RawMesh.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#include "MaterialUtilities.h"
#include "MeshMergeUtilities/Private/MeshMergeUtilities.h"
#include "MeshMergeModule.h"

#include "MaterialOptions.h"
#include "IMaterialBakingModule.h"
#include "MeshMergeUtilities/Private/MeshMergeHelpers.h"
#include "MaterialBakingStructures.h"
#include "MeshMergeData.h"
#include "ObjectTools.h"
#include "UObject/UObjectGlobals.h"
#include "MeshDescriptionOperations.h" 
#include "LODUtilities.h"
#include "StaticMeshOperations.h"
#include "StaticMeshAttributes.h"

#include "Components/StaticMeshComponent.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h" 

#define LOCTEXT_NAMESPACE "InstaLODUI"

namespace InstaLODVectorHelper
{
	static inline InstaLOD::InstaVec3F FVectorToInstaVec(const FVector& Vector)
	{
		InstaLOD::InstaVec3F OutVector;
		OutVector.X = Vector.X;
		OutVector.Y = Vector.Y;
		OutVector.Z = Vector.Z;
		return OutVector;
	}

	static inline TArray<FVector2d> FVector2fArrayToFVector2dArray(const TArray<FVector2f>& Values)
	{
		TArray<FVector2d> DoubleValues;

		DoubleValues.Reserve(Values.Num());

		for (const FVector2f& Value : Values)
		{
			DoubleValues.Push(FVector2d(Value.X, Value.Y));
		}
		return DoubleValues;
	}

	static inline TArray<FVector2f> FVector2dArrayToFVector2fArray(const TArray<FVector2d>& Values)
	{
		TArray<FVector2f> SinglePrecisionValues;

		SinglePrecisionValues.Reserve(Values.Num());

		for (const FVector2d& Value : Values)
		{
			SinglePrecisionValues.Push(FVector2f(Value.X, Value.Y));
		}
		return SinglePrecisionValues;
	}

	static inline FVector InstaVecToFVector(const InstaLOD::InstaVec3F& Vector)
	{
		return FVector(Vector.X, Vector.Y, Vector.Z);
	}

	static inline void TransformRawMeshVertexData(const FTransform& InTransform, FMeshDescription& OutRawMesh)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = OutRawMesh.VertexAttributes().GetAttributesRef<FVector3f>(MeshAttribute::Vertex::Position);
		TEdgeAttributesRef<bool> EdgeHardnesses = OutRawMesh.EdgeAttributes().GetAttributesRef<bool>(MeshAttribute::Edge::IsHard);
		TPolygonGroupAttributesRef<FName> PolygonGroupImportedMaterialSlotNames = OutRawMesh.PolygonGroupAttributes().GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = OutRawMesh.VertexInstanceAttributes().GetAttributesRef<FVector3f>(MeshAttribute::VertexInstance::Normal);
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = OutRawMesh.VertexInstanceAttributes().GetAttributesRef<FVector3f>(MeshAttribute::VertexInstance::Tangent);
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = OutRawMesh.VertexInstanceAttributes().GetAttributesRef<float>(MeshAttribute::VertexInstance::BinormalSign);
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = OutRawMesh.VertexInstanceAttributes().GetAttributesRef<FVector4f>(MeshAttribute::VertexInstance::Color);
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = OutRawMesh.VertexInstanceAttributes().GetAttributesRef<FVector2f>(MeshAttribute::VertexInstance::TextureCoordinate);

		for (const FVertexID VertexID : OutRawMesh.Vertices().GetElementIDs())
		{
			const FVector3d VertexPosition = InTransform.TransformPosition(FVector3d(VertexPositions[VertexID]));
			VertexPositions[VertexID] = FVector3f(VertexPosition.X, VertexPosition.Y, VertexPosition.Z);
		}

		FMatrix Matrix = InTransform.ToMatrixWithScale();
		FMatrix AdjointT = Matrix.TransposeAdjoint();
		AdjointT.RemoveScaling();

		const float MulBy = Matrix.Determinant() < 0.f ? -1.f : 1.f;
		const auto fnGetTransformedNormal = [&AdjointT, MulBy](const FVector3f& Normal) -> FVector3f
		{
			const FVector Result = AdjointT.TransformVector(FVector(Normal)) * MulBy;
			return FVector3f(Result.X, Result.Y, Result.Z);
		};

		for (const FVertexInstanceID VertexInstanceID : OutRawMesh.VertexInstances().GetElementIDs())
		{
			FVector3f TangentY = FVector3f::CrossProduct(VertexInstanceNormals[VertexInstanceID], VertexInstanceTangents[VertexInstanceID]).GetSafeNormal() * VertexInstanceBinormalSigns[VertexInstanceID];
			VertexInstanceTangents[VertexInstanceID] = fnGetTransformedNormal(VertexInstanceTangents[VertexInstanceID]);
			TangentY = fnGetTransformedNormal(TangentY);
			VertexInstanceNormals[VertexInstanceID] = fnGetTransformedNormal(VertexInstanceNormals[VertexInstanceID]);
			VertexInstanceBinormalSigns[VertexInstanceID] = GetBasisDeterminantSign(FVector3d (VertexInstanceTangents[VertexInstanceID]), FVector3d(TangentY), FVector3d(VertexInstanceNormals[VertexInstanceID]));
		}

		const bool bIsMirrored = InTransform.GetDeterminant() < 0.f;
		if (bIsMirrored)
		{
			//Reverse the vertex instance
			OutRawMesh.ReverseAllPolygonFacing();
		}
	}
}

class FInstaLODMeshMergeUtilities : public FMeshMergeUtilities
{
public:
	void CreateMaterialData(IInstaLOD* InstaLOD, TArray<InstaLODMergeData>& MergeData, InstaLOD::IInstaLODMaterialData* MaterialData, const struct FMaterialProxySettings& InMaterialProxySettings, TArray<UMaterialInterface*>& OutMaterials) const
	{
		check(InstaLOD);
		check(MaterialData);

		if (MergeData.Num() == 0)
		{
			UE_LOG(LogInstaLOD, Log, TEXT("No merge data specified."));
			return;
		}

		FScopedSlowTask SlowTask(100.f, (NSLOCTEXT("InstaLODUI", "CreateMaterialData", "Creating Material Data")));
		SlowTask.MakeDialog();

		// progress state 10/100
		SlowTask.EnterProgressFrame(10.0f, NSLOCTEXT("InstaLODUI", "CreateMaterialData_GatheringData", "Gathering Data"));
		SlowTask.MakeDialog();

		IMaterialBakingModule& MaterialBakingModule = FModuleManager::Get().LoadModuleChecked<IMaterialBakingModule>("MaterialBaking");
		TArray<FBakeOutput> BakeOutputs;
		TArray<FFlattenMaterial> FlattenedMaterials;
		TArray<FMaterialData> MaterialSettings;
		TArray<UMaterialInterface*> Materials;

		// setup bake material properties
		UMaterialOptions* const Options = PopulateMaterialOptions(InMaterialProxySettings);
		TArray<EMaterialProperty> MaterialProperties;

		for (const FPropertyEntry& Entry : Options->Properties)
		{
			if (Entry.Property != MP_MAX)
			{
				MaterialProperties.Add(Entry.Property);
			}
		}

		const auto fnScaleTextureCoordinatesToBox = [](const FBox2D& Box, TArray<FVector2D>& InOutTextureCoordinates)
		{
			const FBox2D CoordinateBox(InOutTextureCoordinates);
			const FVector2D CoordinateRange = CoordinateBox.GetSize();
			const FVector2D Offset = CoordinateBox.Min + Box.Min;
			const FVector2D Scale = Box.GetSize() / CoordinateRange;
			for (FVector2D& Coordinate : InOutTextureCoordinates)
			{
				Coordinate = (Coordinate - Offset) * Scale;
			}
		};

		constexpr int32 BaseLODIndex = 0;
		uint32 SectionIndex = 0u; 
		uint32 NumSections = 0u;
		for (const InstaLODMergeData& InstaLODMergeData : MergeData)
		{
			TArray<FSectionInfo> Sections;
			
			// check if it's a Static Mesh or Skeletal Mesh
			if (InstaLODMergeData.Component->StaticMeshComponent.IsValid())
			{
				FMeshMergeHelpers::ExtractSections(InstaLODMergeData.Component->StaticMeshComponent.Get(), BaseLODIndex, Sections);
			}
			else if (InstaLODMergeData.Component->SkeletalMeshComponent.IsValid())
			{
				FMeshMergeHelpers::ExtractSections(InstaLODMergeData.Component->SkeletalMeshComponent.Get(), BaseLODIndex, Sections);
				IInstaLOD::UE_SkeletalMeshResource* const Resource = InstaLODMergeData.Component->SkeletalMeshComponent->GetSkeletalMeshAsset()->GetImportedModel();

				checkf(Resource->LODModels.IsValidIndex(BaseLODIndex), TEXT("Invalid LOD Index"));

				// update LOD Sections 
				for (int32 Index=0; Index<Sections.Num(); Index++)
				{
					Sections[Index].MaterialIndex = Resource->LODModels[BaseLODIndex].Sections[Index].MaterialIndex;
				}
			}

			NumSections += Sections.Num();
			
			// NOTE: we are still using the RawMesh for convenience reasons
			// This will be changed in a future release.
			FRawMesh RawMesh;
			FMeshDescription MeshDescription = FMeshDescription();
			FStaticMeshAttributes(MeshDescription).Register();

			// we convert our InstaLOD Mesh back into a raw mesh
			// this way we can operate both on skeletal and static meshes in a unified way
			// NOTE: flip InstaLODMesh FaceDirections to revert the ReverseFaceDirections in ConvertInstaLODMeshToMeshDescription
			InstaLOD->ConvertInstaLODMeshToRawMesh(InstaLODMergeData.InstaLODMesh, RawMesh);
			InstaLODMergeData.InstaLODMesh->ReverseFaceDirections(/*flipNormals:*/ false);
			InstaLOD->ConvertInstaLODMeshToMeshDescription(InstaLODMergeData.InstaLODMesh, TMap<int32, FName>(), MeshDescription);
			TMap<uint32 /*Section Material Index*/, uint32 /*Global Section Index*/> MaterialIndexToSection;

			FMeshData MeshSetting;
			TArray<bool> MaterialRequiresFullBake;

			// determine if Section materials require full bake
			for (const FSectionInfo& Section : Sections)
			{
				int32 NumTexCoords = 0;
				bool bUseVertexData = false;
				FMaterialUtilities::AnalyzeMaterial(Section.Material, MaterialProperties, NumTexCoords, bUseVertexData);
				MaterialRequiresFullBake.Add(NumTexCoords > 1 || bUseVertexData);
			}

			// update full bake settings
			for (const bool bRequiresFullBake : MaterialRequiresFullBake)
			{
				if (!bRequiresFullBake)
					continue;

				const UStaticMeshComponent* const StaticMeshComponent = InstaLODMergeData.Component->StaticMeshComponent.Get();
				const UStaticMesh* const StaticMesh = StaticMeshComponent != nullptr ? StaticMeshComponent->GetStaticMesh() : nullptr;

				// if we already have lightmap uvs generated or the lightmap coordinate index != 0 and available we can reuse those instead of having to generate new ones
				if (StaticMesh != nullptr && (StaticMesh->GetSourceModel(0).BuildSettings.bGenerateLightmapUVs ||
					(StaticMesh->GetLightMapCoordinateIndex() != 0 && RawMesh.WedgeTexCoords[StaticMesh->GetLightMapCoordinateIndex()].Num() != 0)))
				{
					MeshSetting.CustomTextureCoordinates = InstaLODVectorHelper::FVector2fArrayToFVector2dArray(RawMesh.WedgeTexCoords[StaticMeshComponent->GetStaticMesh()->GetLightMapCoordinateIndex()]);

					fnScaleTextureCoordinatesToBox(FBox2D(FVector2D::ZeroVector, FVector2D(1, 1)), MeshSetting.CustomTextureCoordinates);
				}
				else
				{
					// generate new non overlapping UVs
					IMeshUtilities& MeshUtilities = FModuleManager::GetModuleChecked<IMeshUtilities>("MeshUtilities");
					TArray<FVector2f> UniqueUVs;
					MeshUtilities.GenerateUniqueUVsForStaticMesh(RawMesh, Options->TextureSize.GetMax(), UniqueUVs);
					MeshSetting.CustomTextureCoordinates = InstaLODVectorHelper::FVector2fArrayToFVector2dArray(UniqueUVs);
					fnScaleTextureCoordinatesToBox(FBox2D(FVector2D::ZeroVector, FVector2D(1, 1)), MeshSetting.CustomTextureCoordinates);
				}

				FStaticMeshOperations::ConvertFromRawMesh(RawMesh, MeshDescription, /*MaterialMap*/ TMap<int32, FName>(), /*bSkipNormalsAndTangents*/ false);
				MeshSetting.TextureCoordinateBox = FBox2D(MeshSetting.CustomTextureCoordinates);

				if (StaticMeshComponent != nullptr && StaticMeshComponent->LODData.IsValidIndex(0))
				{ 
					if (const FMeshMapBuildData* const MeshMapBuildData = StaticMeshComponent->GetMeshMapBuildData(*StaticMeshComponent->LODData.GetData()))
					{
						MeshSetting.LightMap = MeshMapBuildData->LightMap;
						MeshSetting.LightMapIndex = StaticMeshComponent->GetStaticMesh()->GetLightMapCoordinateIndex();
					}
				}

				break;
			}

			MeshSetting.MeshDescription = &MeshDescription;

			// progress state 
			SlowTask.EnterProgressFrame(10.0f, NSLOCTEXT("InstaLODUI", "CreateMaterialData_FlatteningMaterialData", "Flattening Material Data"));
			SlowTask.MakeDialog();

			TArray<FBakeOutput> TempBakeOutputs;

			for (const FSectionInfo& Section : Sections)
			{
				MeshSetting.MaterialIndices.Empty();
				MeshSetting.MaterialIndices.Add(Section.MaterialIndex);
				
				FMaterialData MaterialSetting;
				MaterialSetting.Material = Section.Material;
				MaterialSettings.Add(MaterialSetting);
				Materials.Add(Section.Material);

				for (const FPropertyEntry& Entry : Options->Properties)
				{
					if (!Entry.bUseConstantValue && MaterialSetting.Material->IsPropertyActive(Entry.Property) && Entry.Property != MP_MAX)
					{
						MaterialSetting.PropertySizes.Add(Entry.Property, Entry.bUseCustomSize ? Entry.CustomSize : Options->TextureSize);
					}
				}

				TempBakeOutputs.Empty();
				MaterialBakingModule.BakeMaterials(TArray<FMaterialData*>({ &MaterialSetting }), TArray<FMeshData*>({ &MeshSetting }), TempBakeOutputs);
				BakeOutputs.Append(TempBakeOutputs);
				
				// progress state 
				const float Progress = (SectionIndex+1) / (float) NumSections;
				SlowTask.EnterProgressFrame(Progress, FText::FromString(FString::Printf(TEXT("Flattening Material (%i/%i)"), SectionIndex + 1, NumSections)));
				SlowTask.MakeDialog();

				MaterialIndexToSection.Add(Section.MaterialIndex, SectionIndex);
				SectionIndex++;
			}

			InstaLOD::IInstaLODMesh* const InstaLODMesh = InstaLODMergeData.InstaLODMesh;
			TArray<FVector2f> NewUVs;

			if (MeshSetting.MeshDescription != nullptr)
			{
				NewUVs = MeshSetting.CustomTextureCoordinates.Num() ? InstaLODVectorHelper::FVector2dArrayToFVector2fArray(MeshSetting.CustomTextureCoordinates) : RawMesh.WedgeTexCoords[MeshSetting.TextureCoordinateIndex];
			}

			// update texcoords
			if (NewUVs.Num() > 0)
			{
				uint64 WedgeCount = 0;
				InstaLOD::InstaVec2F* const InstaLODWedgeTexCoords0 = InstaLODMesh->GetWedgeTexCoords(0, &WedgeCount);
				check(WedgeCount == NewUVs.Num());
				for (int32 WedgeIndex=0; WedgeIndex<WedgeCount; WedgeIndex++)
				{
					InstaLODWedgeTexCoords0[WedgeIndex].X = NewUVs[WedgeIndex].X;
					InstaLODWedgeTexCoords0[WedgeIndex].Y = NewUVs[WedgeIndex].Y;
				}
			}

			// update the MeshDescription/RawMesh data so that local material indices match the global material indices 
			TMap<int32/*SectionIndex*/, int32 /*UniqueMaterialIndex*/> Remap;
			TMap<FPolygonGroupID, FPolygonGroupID> PolygonGroupRemapping;

			for (int32& FaceMaterialIndex : RawMesh.FaceMaterialIndices)
			{
				if (!MaterialIndexToSection.Contains(FaceMaterialIndex))
					continue;

				const FPolygonGroupID OldID(FaceMaterialIndex);
				const FPolygonGroupID NewID(MaterialIndexToSection[FaceMaterialIndex]);

				PolygonGroupRemapping.Add(OldID, NewID);
				FaceMaterialIndex = NewID.GetValue(); 
			}
			if (PolygonGroupRemapping.Num() > 0)
			{
				MeshDescription.RemapPolygonGroups(PolygonGroupRemapping);
			}
			
			// update face material indices
			uint64 FaceCount = 0;
			int32* const InstaLODFaceMaterialIndices = InstaLODMesh->GetFaceMaterialIndices(&FaceCount);
			check(FaceCount == RawMesh.FaceMaterialIndices.Num());

			for (int32 FaceIndex=0; FaceIndex<FaceCount; FaceIndex++)
			{
				InstaLODFaceMaterialIndices[FaceIndex] = RawMesh.FaceMaterialIndices[FaceIndex];
			}
		}

		// append constant properties
		TArray<FColor> ConstantData;
		FIntPoint ConstantSize(1, 1);
		for (const FPropertyEntry& Entry : Options->Properties)
		{
			if (Entry.bUseConstantValue && Entry.Property != MP_MAX)
			{
				ConstantData.SetNum(1, false);
				ConstantData[0] = FLinearColor(Entry.ConstantValue, Entry.ConstantValue, Entry.ConstantValue).ToFColor(true);

				for (FBakeOutput& Ouput : BakeOutputs)
				{
					Ouput.PropertyData.Add(Entry.Property, ConstantData);
					Ouput.PropertySizes.Add(Entry.Property, ConstantSize);
				}
			}
		}

		OutMaterials.Empty();
		OutMaterials.Append(Materials);

		TransferOutputToFlatMaterials(MaterialSettings, BakeOutputs, FlattenedMaterials);
		InstaLOD->ConvertFlattenMaterialsToInstaLODMaterialData(FlattenedMaterials, MaterialData, InMaterialProxySettings);
	}
};

void UInstaLODUtilities::CreateMaterialData(class IInstaLOD* InstaLOD, TArray<InstaLODMergeData>& MergeData, class InstaLOD::IInstaLODMaterialData *MaterialData, const struct FMaterialProxySettings& InMaterialProxySettings, TArray<UMaterialInterface*>& OutMaterials)
{
	// NOTE: we need to avoid calling the constructor of FMeshMergeUtilities
	// otherwise it will rebind delegates to the base class and breaking the proxy job processing.
	// The C cast is a hack, but we're taking care as to not modify the class members or vtable so it should pass
	const IMeshMergeUtilities& Module = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
	((FInstaLODMeshMergeUtilities*)&Module)->CreateMaterialData(InstaLOD, MergeData, MaterialData, InMaterialProxySettings, OutMaterials);
}

bool UInstaLODUtilities::WriteInstaLODMessagesToLog(IInstaLOD* InstaLOD)
{
	check(InstaLOD);
	
	char InstaLog[8192];
	
	if (InstaLOD->GetInstaLOD()->GetMessageLog(InstaLog, sizeof(InstaLog), nullptr) == 0)
		return false;
	
	UE_LOG(LogInstaLOD, Error, TEXT("%s"), UTF8_TO_TCHAR(InstaLog));
	
	return true;
}

void UInstaLODUtilities::AppendMeshComponentToInstaLODMesh(IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex)
{
	check(InstaLOD);
	check(OutInstaLODMesh);

	if (InstaLOD == nullptr)
		return;
	
	InstaLOD::IInstaLODMesh* const InstaLODMesh = InstaLOD->AllocInstaLODMesh();
	UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(InstaLOD, MeshComponent, InstaLODMesh, BaseLODIndex);
	InstaLOD::IInstaLODMeshExtended* const ExtendedMesh = InstaLOD->GetInstaLOD()->CastToInstaLODMeshExtended(OutInstaLODMesh);

	if (ExtendedMesh && InstaLODMesh)
	{
		ExtendedMesh->AppendMesh(InstaLODMesh);
		InstaLOD->GetInstaLOD()->DeallocMesh(InstaLODMesh);
	}
}

/**
 * The FProxyMaterialUtilities_Copy namespace contains functions that 
 * are copied from FMaterialUtilities (ProxyMaterialUtilities.cpp) that are not publicly available.
 */
namespace FProxyMaterialUtilities_Copy
{
	/** 
 	 * Find the parameter name to use with the provided material, for the given property
	 */
	static TArray<FString> GetPotentialParamNames(EFlattenMaterialProperties InProperty)
	{
		switch (InProperty)
		{
		case EFlattenMaterialProperties::Diffuse: return { "BaseColor", "Diffuse" };
		case EFlattenMaterialProperties::Normal: return { "Normal" };
		case EFlattenMaterialProperties::Metallic: return { "Metallic" };
		case EFlattenMaterialProperties::Roughness: return { "Roughness" };
		case EFlattenMaterialProperties::Specular: return { "Specular" };
		case EFlattenMaterialProperties::Opacity: return { "Opacity" };
		case EFlattenMaterialProperties::OpacityMask: return { "OpacityMask" };
		case EFlattenMaterialProperties::AmbientOcclusion: return { "AmbientOcclusion" };
		case EFlattenMaterialProperties::Emissive: return { "EmissiveColor", "Emissive" };
		}

		return TArray<FString>();
	}

	static EMaterialParameterType GetConstantParamType(EFlattenMaterialProperties InProperty)
	{
		EMaterialParameterType ParamType = EMaterialParameterType::None;

		switch (InProperty)
		{
		case EFlattenMaterialProperties::Metallic:
		case EFlattenMaterialProperties::Roughness:
		case EFlattenMaterialProperties::Specular:
		case EFlattenMaterialProperties::Opacity:
		case EFlattenMaterialProperties::OpacityMask:
		case EFlattenMaterialProperties::AmbientOcclusion:
			ParamType = EMaterialParameterType::Scalar;
			break;

		case EFlattenMaterialProperties::Diffuse:
		case EFlattenMaterialProperties::Emissive:
			ParamType = EMaterialParameterType::Vector;
			break;
		}

		return ParamType;
	}

	/**
	 * Find the parameter name to use with the provided material, for the given property. 
	 */ 
	static bool GetMatchingParamName(EFlattenMaterialProperties InProperty, const UMaterialInterface* InBaseMaterial, FString& OutParamName, TArray<FString>* OutMissingNames = nullptr)
	{
		const TArray<FString> PotentialNames = GetPotentialParamNames(InProperty);

		// Missing names, for error reporting
		FString MissingNames;

		for (const FString& PotentialName : PotentialNames)
		{
			const FName TextureName(PotentialName + TEXT("Texture"));
			const FName ConstName(PotentialName + TEXT("Const"));
			const FName UseTexture(TEXT("Use") + PotentialName);

			UTexture* DefaultTexture = nullptr;
			bool DefaultSwitchValue = false;
			float DefaultScalarValue;
			FLinearColor DefaultVectorValue;
			FGuid ExpressionGuid;

			if (!MissingNames.IsEmpty())
			{
				MissingNames += TEXT("|");
			}

			bool bHasRequiredParams = InBaseMaterial->GetTextureParameterValue(TextureName, DefaultTexture);

			switch (GetConstantParamType(InProperty))
			{
			case EMaterialParameterType::Scalar:
				MissingNames += UseTexture.ToString() + "+" + TextureName.ToString() + "+" + ConstName.ToString();
				bHasRequiredParams &= InBaseMaterial->GetStaticSwitchParameterDefaultValue(UseTexture, DefaultSwitchValue, ExpressionGuid) &&
					InBaseMaterial->GetScalarParameterDefaultValue(ConstName, DefaultScalarValue);
				break;

			case EMaterialParameterType::Vector:
				MissingNames += UseTexture.ToString() + "+" + TextureName.ToString() + "+" + ConstName.ToString();
				bHasRequiredParams &= InBaseMaterial->GetStaticSwitchParameterDefaultValue(UseTexture, DefaultSwitchValue, ExpressionGuid) &&
					InBaseMaterial->GetVectorParameterDefaultValue(ConstName, DefaultVectorValue);
				break;

			case EMaterialParameterType::None:
				MissingNames += UseTexture.ToString();
				break;
			}

			if (bHasRequiredParams)
			{
				OutParamName = PotentialName;
				return true;
			}
		}

		if (OutMissingNames != nullptr)
		{
			OutMissingNames->Add(MissingNames);
		}

		OutParamName = "";
		return false;
	}

	static FString GetMatchingParamName(EFlattenMaterialProperties InProperty, UMaterialInterface* InBaseMaterial)
	{
		FString ParamName = "";
		if (!GetMatchingParamName(InProperty, InBaseMaterial, ParamName))
		{
			UE_LOG(LogInstaLOD, Fatal, TEXT("Invalid base material, should have been rejected by IsValidFlattenMaterial()"));
		}
		return ParamName;
	}

	static bool CalculatePackedTextureData(const FFlattenMaterial& InMaterial, bool& bOutPackMetallic, bool& bOutPackSpecular, bool& bOutPackRoughness, int32& OutNumSamples, FIntPoint& OutSize)
	{
		// Whether or not a material property is baked down
		const bool bHasMetallic = InMaterial.DoesPropertyContainData(EFlattenMaterialProperties::Metallic) && !InMaterial.IsPropertyConstant(EFlattenMaterialProperties::Metallic);
		const bool bHasRoughness = InMaterial.DoesPropertyContainData(EFlattenMaterialProperties::Roughness) && !InMaterial.IsPropertyConstant(EFlattenMaterialProperties::Roughness);
		const bool bHasSpecular = InMaterial.DoesPropertyContainData(EFlattenMaterialProperties::Specular) && !InMaterial.IsPropertyConstant(EFlattenMaterialProperties::Specular);

		// Check for same texture sizes
		bool bSameTextureSize = false;

		// Determine whether or not the properties sizes match
		const FIntPoint MetallicSize = InMaterial.GetPropertySize(EFlattenMaterialProperties::Metallic);
		const FIntPoint SpecularSize = InMaterial.GetPropertySize(EFlattenMaterialProperties::Specular);
		const FIntPoint RoughnessSize = InMaterial.GetPropertySize(EFlattenMaterialProperties::Roughness);

		bSameTextureSize = (MetallicSize == RoughnessSize) || (MetallicSize == SpecularSize);
		if (bSameTextureSize)
		{
			OutSize = MetallicSize;
			OutNumSamples = InMaterial.GetPropertySamples(EFlattenMaterialProperties::Metallic).Num();
		}
		else
		{
			bSameTextureSize = (RoughnessSize == SpecularSize);
			if (bSameTextureSize)
			{
				OutSize = RoughnessSize;
				OutNumSamples = InMaterial.GetPropertySamples(EFlattenMaterialProperties::Roughness).Num();
			}
		}

		// Now that we know if the data matches determine whether or not we should pack the properties
		int32 NumPacked = 0;
		if (OutNumSamples != 0)
		{
			bOutPackMetallic = bHasMetallic ? (OutNumSamples == InMaterial.GetPropertySamples(EFlattenMaterialProperties::Metallic).Num()) : false;
			NumPacked += (bOutPackMetallic) ? 1 : 0;
			bOutPackRoughness = bHasRoughness ? (OutNumSamples == InMaterial.GetPropertySamples(EFlattenMaterialProperties::Roughness).Num()) : false;
			NumPacked += (bOutPackRoughness) ? 1 : 0;
			bOutPackSpecular = bHasSpecular ? (OutNumSamples == InMaterial.GetPropertySamples(EFlattenMaterialProperties::Specular).Num()) : false;
			NumPacked += (bOutPackSpecular) ? 1 : 0;
		}
		else
		{
			bOutPackMetallic = bOutPackRoughness = bOutPackSpecular = false;
		}

		// Need atleast two properties to pack
		return NumPacked >= 2;
	}
}

UMaterialInstanceConstant* UInstaLODUtilities::CreateFlattenMaterialInstance(const FFlattenMaterial& FlattenMaterial, const FMaterialProxySettings& InMaterialProxySettings, const FString& SaveObjectPath, TArray<UObject*>& OutAssetsToSync, bool bIsFlipbookMaterial)
{
	const FString AssetBasePath = FPackageName::GetLongPackagePath(SaveObjectPath) + TEXT("/");
	const FString AssetBaseName = FPackageName::GetShortName(SaveObjectPath);
	UPackage* InOuter = nullptr;
	
	const FString BaseMaterialAsset = bIsFlipbookMaterial ? TEXT("/InstaLODMeshReduction/InstaLODFlattenFlipbookMaterial.InstaLODFlattenFlipbookMaterial") : TEXT("/InstaLODMeshReduction/InstaLODFlattenMaterial.InstaLODFlattenMaterial");

	UMaterial* const BaseMaterial = LoadObject<UMaterial>(nullptr, *BaseMaterialAsset, nullptr, LOAD_None, nullptr);
	check(BaseMaterial);
	
	UMaterialInstanceConstant* const OutMaterial = FMaterialUtilities::CreateInstancedMaterial(BaseMaterial, nullptr, AssetBasePath + AssetBaseName, RF_Public | RF_Standalone);
	OutAssetsToSync.Add(OutMaterial);

	OutMaterial->BasePropertyOverrides.TwoSided = FlattenMaterial.bTwoSided;
	OutMaterial->BasePropertyOverrides.bOverride_TwoSided = FlattenMaterial.bTwoSided != false;
	OutMaterial->BasePropertyOverrides.DitheredLODTransition = FlattenMaterial.bDitheredLODTransition;
	OutMaterial->BasePropertyOverrides.bOverride_DitheredLODTransition = FlattenMaterial.bDitheredLODTransition != false;
	
	if (InMaterialProxySettings.BlendMode != BLEND_Opaque)
	{
		OutMaterial->BasePropertyOverrides.bOverride_BlendMode = true;
		OutMaterial->BasePropertyOverrides.BlendMode = InMaterialProxySettings.BlendMode;
	}
	
	bool bPackMetallic, bPackSpecular, bPackRoughness;
	int32 NumSamples = 0;
	FIntPoint PackedSize;
	const bool bPackTextures = FProxyMaterialUtilities_Copy::CalculatePackedTextureData(FlattenMaterial, bPackMetallic, bPackSpecular, bPackRoughness, NumSamples, PackedSize);
	
	const bool bSRGB = true;
	const bool bRGB = false;

	// NOTE: lambdas are copied from ProxyMaterialUtilities.cpp with some 
	// minor changes such as Compression settings and sRGB fields.
	const auto CreateTextureFromDefault = [&](const FName TextureName, const FString& AssetLongName, FIntPoint Size, const TArray<FColor>& Samples, const TextureCompressionSettings Compression, const bool bSRGB)
	{
		FCreateTexture2DParameters CreateParams;
		CreateParams.bSRGB = bSRGB;
		CreateParams.bVirtualTexture = false;
		CreateParams.CompressionSettings = Compression;
		CreateParams.TextureGroup = TextureGroup::TEXTUREGROUP_World;

		return FMaterialUtilities::CreateTexture(InOuter, AssetLongName, Size, Samples, CreateParams, RF_Public | RF_Standalone);
	};

	const auto SetTextureParam = [&](EFlattenMaterialProperties FlattenProperty, const TextureCompressionSettings Compression, const bool bSRGB)
	{
		if (FlattenMaterial.DoesPropertyContainData(FlattenProperty) && !FlattenMaterial.IsPropertyConstant(FlattenProperty))
		{
			const FString PropertyName = FProxyMaterialUtilities_Copy::GetMatchingParamName(FlattenProperty, OutMaterial);
			const FName TextureName(PropertyName + TEXT("Texture"));
			const FName UseTexture(TEXT("Use") + PropertyName);
			UTexture2D* const Texture = CreateTextureFromDefault(TextureName, AssetBasePath + TEXT("T_") + AssetBaseName + TEXT("_") + PropertyName, FlattenMaterial.GetPropertySize(FlattenProperty), FlattenMaterial.GetPropertySamples(FlattenProperty), Compression, bSRGB);

			OutMaterial->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(UseTexture), true);
			OutMaterial->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(TextureName), /*Value:*/ Texture);

			OutAssetsToSync.Add(Texture);
		}
	};

	const auto SetTextureParamConstVector = [&](EFlattenMaterialProperties FlattenProperty, const TextureCompressionSettings Compression, const bool bSRGB)
	{
		if (FlattenMaterial.DoesPropertyContainData(FlattenProperty) && !FlattenMaterial.IsPropertyConstant(FlattenProperty))
		{
			SetTextureParam(FlattenProperty, Compression, bSRGB);
		}
		else
		{
			const FString PropertyName = FProxyMaterialUtilities_Copy::GetMatchingParamName(FlattenProperty, OutMaterial);
			const FName ConstName(PropertyName + TEXT("Const"));
			OutMaterial->SetVectorParameterValueEditorOnly(ConstName, FlattenMaterial.GetPropertySamples(FlattenProperty)[0]);
		}
	};

	const auto SetTextureParamConstScalar = [&](EFlattenMaterialProperties FlattenProperty, float ConstantValue, const TextureCompressionSettings Compression, const bool bSRGB)
	{
		if (FlattenMaterial.DoesPropertyContainData(FlattenProperty) && !FlattenMaterial.IsPropertyConstant(FlattenProperty))
		{
			SetTextureParam(FlattenProperty, Compression, bSRGB);
		}
		else
		{
			const FString PropertyName = FProxyMaterialUtilities_Copy::GetMatchingParamName(FlattenProperty, OutMaterial);
			const FName ConstName(PropertyName + TEXT("Const"));
			const FLinearColor Colour = FlattenMaterial.IsPropertyConstant(FlattenProperty) ? FLinearColor::FromSRGBColor(FlattenMaterial.GetPropertySamples(FlattenProperty)[0]) : FLinearColor(ConstantValue, 0, 0, 0);
			OutMaterial->SetScalarParameterValueEditorOnly(ConstName, Colour.R);
		}
	};

	const auto SetTextureParamConstLinear = [&](EFlattenMaterialProperties FlattenProperty, float ConstantValue, const TextureCompressionSettings Compression, const bool bSRGB)
	{
		if (FlattenMaterial.DoesPropertyContainData(FlattenProperty) && !FlattenMaterial.IsPropertyConstant(FlattenProperty))
		{
			SetTextureParam(FlattenProperty, Compression, bSRGB);
		}
		else
		{
			const FString PropertyName = FProxyMaterialUtilities_Copy::GetMatchingParamName(FlattenProperty, OutMaterial);
			const FName ConstName(PropertyName + TEXT("Const"));
			OutMaterial->SetVectorParameterValueEditorOnly(ConstName, FlattenMaterial.GetPropertySamples(FlattenProperty)[0].ReinterpretAsLinear());
		}
	};

	// load textures and set switches accordingly
	if (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::Diffuse).Num() > 0 &&
		!(FlattenMaterial.IsPropertyConstant(EFlattenMaterialProperties::Diffuse) &&
		FlattenMaterial.GetPropertySamples(EFlattenMaterialProperties::Diffuse)[0] == FColor::Black))
	{
		SetTextureParamConstVector(EFlattenMaterialProperties::Diffuse, TC_Default, /*bSRGB:*/ true);
	}

	if (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::Normal).Num() > 1)
	{
		SetTextureParam(EFlattenMaterialProperties::Normal, TC_Default, /*bSRGB:*/ false);
	}
	
	if (!bPackMetallic && (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::Metallic).Num() > 0 || !InMaterialProxySettings.bMetallicMap))
	{
		SetTextureParamConstScalar(EFlattenMaterialProperties::Metallic, 0.0f, TC_Default, /*bSRGB:*/ true);
	}

	if (!bPackRoughness && (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::Roughness).Num() > 0 || !InMaterialProxySettings.bRoughnessMap))
	{
		SetTextureParamConstScalar(EFlattenMaterialProperties::Roughness, 0.0f, TC_Default, /*bSRGB:*/ true);
	}

	if (!bPackSpecular && (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::Specular).Num() > 0 || !InMaterialProxySettings.bSpecularMap))
	{
		SetTextureParamConstScalar(EFlattenMaterialProperties::Specular, 0.0f, TC_Default, /*bSRGB:*/ true);
	}

	if (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::Opacity).Num() > 0 || !InMaterialProxySettings.bOpacityMap)
	{
		SetTextureParamConstScalar(EFlattenMaterialProperties::Opacity, 1.0f, TC_Grayscale, /*bSRGB:*/ false);
	}

	if (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::OpacityMask).Num() > 0 || !InMaterialProxySettings.bOpacityMaskMap)
	{
		SetTextureParamConstScalar(EFlattenMaterialProperties::OpacityMask, 1.0f, TC_Grayscale, /*bSRGB:*/ false);
	}

	if (FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::AmbientOcclusion).Num() > 0 || !InMaterialProxySettings.bAmbientOcclusionMap)
	{
		SetTextureParamConstScalar(EFlattenMaterialProperties::AmbientOcclusion, 0.0f, TC_Grayscale, /*bSRGB:*/ false);
	}

	// handle the packed texture if applicable
	if (bPackTextures)
	{
		TArray<FColor> MergedTexture;
		MergedTexture.AddZeroed(NumSamples);

		// merge properties into one texture using the separate colour channels
		const EFlattenMaterialProperties Properties[3] = { EFlattenMaterialProperties::Metallic , EFlattenMaterialProperties::Roughness, EFlattenMaterialProperties::Specular };

		// property that is not part of the pack (because of a different size), will see is reserve pack space fill with black color.
		const bool PropertyShouldBePack[3] = { bPackMetallic , bPackRoughness , bPackSpecular };

		// red mask (all properties are rendered into the red channel)
		FColor NonAlphaRed = FColor::Red;
		NonAlphaRed.A = 0;
		const uint32 ColorMask = NonAlphaRed.DWColor();
		const uint32 Shift[3] = { 0, 8, 16 };
		for (int32 PropertyIndex = 0; PropertyIndex < 3; ++PropertyIndex)
		{
			const EFlattenMaterialProperties Property = Properties[PropertyIndex];
			const bool HasProperty = PropertyShouldBePack[PropertyIndex] && FlattenMaterial.DoesPropertyContainData(Property) && !FlattenMaterial.IsPropertyConstant(Property);

			if (HasProperty)
			{
				const TArray<FColor>& PropertySamples = FlattenMaterial.GetPropertySamples(Property);
				// OR masked values (samples initialized to zero, so no random data)
				for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
				{
					// black adds the alpha + red channel value shifted into the correct output channel
					MergedTexture[SampleIndex].DWColor() |= (FColor::Black.DWColor() + ((PropertySamples[SampleIndex].DWColor() & ColorMask) >> Shift[PropertyIndex]));
				}
			}
		}

		// create texture using the merged property data
		UTexture2D* const PackedTexture = FMaterialUtilities::CreateTexture(InOuter, AssetBasePath + TEXT("T_") + AssetBaseName + TEXT("_MRS"),
			PackedSize, MergedTexture, TC_Default, TEXTUREGROUP_HierarchicalLOD, RF_Public | RF_Standalone, bSRGB);
		check(PackedTexture);
		OutAssetsToSync.Add(PackedTexture);

		// setup switches for whether or not properties will be packed into one texture
		OutMaterial->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("PackMetallic")), bPackMetallic);
		OutMaterial->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("PackSpecular")), bPackSpecular);
		OutMaterial->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("PackRoughness")), bPackRoughness);

		// set up switch and texture values
		OutMaterial->SetTextureParameterValueEditorOnly(TEXT("PackedTexture"), PackedTexture);
	}

	// emissive is a special case due to the scaling variable
	if (FlattenMaterial.GetPropertySamples(EFlattenMaterialProperties::Emissive).Num() > 0 &&
		!(FlattenMaterial.GetPropertySize(EFlattenMaterialProperties::Emissive).Num() == 1 &&
		FlattenMaterial.GetPropertySamples(EFlattenMaterialProperties::Emissive)[0] == FColor::Black))
	{
		SetTextureParamConstLinear(EFlattenMaterialProperties::Emissive, 0.0f, TC_Default, false);

		if (FlattenMaterial.EmissiveScale != 1.0f)
		{
			OutMaterial->SetScalarParameterValueEditorOnly(TEXT("EmissiveScale"), FlattenMaterial.EmissiveScale);
		}
	}
	
	// force initializing the static permutations according to the switches we have set
	OutMaterial->InitStaticPermutation();
	OutMaterial->PostEditChange();
	
	return OutMaterial;
}

UMaterialInstanceConstant* UInstaLODUtilities::CreateFlattenMaterialInstanceFromInstaMaterial(InstaLOD::IInstaLODMaterial* const Material, const FMaterialProxySettings& InMaterialProxySettings, const FString& SaveObjectPath, TArray<UObject*>& OutAssetsToSync, bool bIsFlipbookMaterial)
{
	FFlattenMaterial FlattenMaterial;

	FInstaLODModule& InstaLODModule = FModuleManager::LoadModuleChecked<FInstaLODModule>("InstaLODMeshReduction");
	InstaLODModule.GetInstaLODInterface()->ConvertInstaLODMaterialToFlattenMaterial(Material, FlattenMaterial, InMaterialProxySettings);

	const FString AssetBaseName = FPackageName::GetShortName(SaveObjectPath);
	const FString AssetBasePath = FPackageName::GetLongPackagePath(SaveObjectPath) + TEXT("/");

	// optimize the flattened material
	FMaterialUtilities::OptimizeFlattenMaterial(FlattenMaterial);

	// create a proxy material instance
	UMaterialInstanceConstant* const BakeMaterial = UInstaLODUtilities::CreateFlattenMaterialInstance(FlattenMaterial, InMaterialProxySettings, SaveObjectPath, OutAssetsToSync, bIsFlipbookMaterial);

	// set static lighting flag if necessary
	static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));
	const bool bAllowStaticLighting = (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnGameThread() != 0);
	if (bAllowStaticLighting)
	{
		BakeMaterial->CheckMaterialUsage(MATUSAGE_StaticLighting);
	}

	FStaticParameterSet StaticParameters;
	BakeMaterial->GetStaticParameterValues(StaticParameters);
	bool bStaticParametersDirty = false;

	// save out all additional materials
	for (uint32 TexturePageIndex = 0u; TexturePageIndex < Material->GetTexturePageCount(); TexturePageIndex++)
	{
		InstaLOD::IInstaLODTexturePage* const TexturePage = Material->GetTexturePageAtIndex(TexturePageIndex);
		check(TexturePage);

		const FString TexturePageName = FString(UTF8_TO_TCHAR(TexturePage->GetName()));

		// ignore internal UE_ texture pages, they're part of our proxy material instance
		if (TexturePageName.StartsWith(TEXT("UE_")))
			continue;

		// skip bake texture pages that we can directly hookup to the material
		if (TexturePageName.Equals(TEXT("NormalTangentSpace")) ||
			TexturePageName.Equals(TEXT("Opacity")) ||
			TexturePageName.Equals(TEXT("AmbientOcclusion")))
			continue;

		const FString SaveTexturePagePath = AssetBasePath + TEXT("T_") + AssetBaseName + TEXT("_") + TexturePageName;
		UTexture* const Texture = UInstaLODUtilities::ConvertInstaLODTexturePageToTexture(TexturePage, SaveTexturePagePath);

		if (!Texture)
		{
			UE_LOG(LogInstaLOD, Warning, TEXT("Failed to create new uasset for texture data."));
			continue;
		}

		if (TexturePageName.Equals(TEXT("Displacement")))
		{
			BakeMaterial->SetTextureParameterValueEditorOnly(TEXT("ParallaxTexture"), Texture);
			BakeMaterial->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("UseParallaxMapping")), true);
			bStaticParametersDirty = true;
		}
		else if (TexturePageName.Equals(TEXT("BentNormals")))
		{
			BakeMaterial->SetTextureParameterValueEditorOnly(TEXT("BentNormalsTexture"), Texture);
			BakeMaterial->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("UseBentNormals")), true);
			bStaticParametersDirty = true;
		}
		else if (TexturePageName.Equals(TEXT("NormalObjectSpace")))
		{
			BakeMaterial->SetTextureParameterValueEditorOnly(TEXT("ObjectSpaceNormalTexture"), Texture);
			BakeMaterial->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("UseObjectSpaceNormalMap")), true);
			bStaticParametersDirty = true;
		}

		UE_LOG(LogInstaLOD, Warning, TEXT("%s"), *TexturePageName);
		OutAssetsToSync.Add(Texture);
	}

	if (bStaticParametersDirty)
	{
		// force update of the static permutations
		BakeMaterial->InitStaticPermutation();
		BakeMaterial->PostEditChange();
	}

	return BakeMaterial;
}

FVector UInstaLODUtilities::CustomizePivotPosition(UStaticMesh* const StaticMesh, const FVector& PivotPosition, const bool bLimitedRange)
{
	check(StaticMesh != nullptr);

	const FBox BoundingBox = StaticMesh->GetBoundingBox();

	FVector CenterPoint;
	FVector Extents;
	BoundingBox.GetCenterAndExtents(CenterPoint, Extents);

	FTransform Transform;
	if (bLimitedRange)
	{
		Transform.SetTranslation(CenterPoint);
		Transform.AddToTranslation(Extents * PivotPosition);
	}
	else
	{
		Transform.SetTranslation(PivotPosition);
	}

	FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0u);
	FStaticMeshOperations::ApplyTransform(*MeshDescription, Transform.Inverse());

	StaticMesh->CommitMeshDescription(0u);
	StaticMesh->Build();
	StaticMesh->PostEditChange();

	return Transform.GetTranslation();
}

void UInstaLODUtilities::ApplyPostTransformForVista(UStaticMesh* const StaticMesh, const FTransform& PostTransform)
{
	const FVector Translation = CustomizePivotPosition(StaticMesh, FVector::ZeroVector, true);
	const FTransform InverseTransform(Translation);

	FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0u);
	FStaticMeshOperations::ApplyTransform(*MeshDescription, PostTransform * InverseTransform);

	StaticMesh->CommitMeshDescription(0u);
	StaticMesh->Build();
	StaticMesh->PostEditChange();
}

void UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex, bool IsStaticMeshInWorldSpaceRequired)
{
	check(InstaLOD);
	check(OutInstaLODMesh);

	if (MeshComponent == nullptr)
	{
		UE_LOG(LogInstaLOD, Error, TEXT("MeshComponent is null."));
		return;
	}
	
	if (MeshComponent->StaticMeshComponent.IsValid())
	{
		UInstaLODUtilities::GetInstaLODMeshFromStaticMeshComponent(InstaLOD, MeshComponent->StaticMeshComponent.Get(), OutInstaLODMesh, BaseLODIndex, IsStaticMeshInWorldSpaceRequired);
	}
	else if (MeshComponent->SkeletalMeshComponent.IsValid())
	{
		UInstaLODUtilities::GetInstaLODMeshFromSkeletalMeshComponent(InstaLOD, MeshComponent->SkeletalMeshComponent.Get(), OutInstaLODMesh, BaseLODIndex);
	}
	else
	{
		UE_LOG(LogInstaLOD, Error, TEXT("MeshComponent without valid StaticMesh or SkeletalMesh."));
	}
}

void UInstaLODUtilities::RetrieveMesh(UStaticMeshComponent *const StaticMeshComponent, int32 LODIndex, FMeshDescription& OutMeshDescription, bool bPropagateVertexColours, bool bInWorldSpace)
{
	const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	const FStaticMeshSourceModel& StaticMeshModel = StaticMesh->GetSourceModel(LODIndex);
	// Imported meshes will have a valid mesh description
	const bool bImportedMesh = StaticMesh->IsMeshDescriptionValid(LODIndex);

	// Export the raw mesh data using static mesh render data
	FMeshMergeHelpers::ExportStaticMeshLOD(StaticMesh->GetRenderData()->LODResources[LODIndex], OutMeshDescription, StaticMesh->GetStaticMaterials());

	// Make sure the raw mesh is not irreparably malformed.
	if (OutMeshDescription.VertexInstances().Num() <= 0)
		return;

	// NOTE: The polygongroup ids don't correspondent to the material indices, only to the section index. 
	// In order to make sure that baking operations have the right materials assigned we need to remap the 
	// polygon group ids to the material indices
	{
		const TArray<FStaticMaterial>& StaticMaterials = StaticMesh->GetStaticMaterials();
		FStaticMeshAttributes Attributes(OutMeshDescription);
		const TPolygonGroupAttributesRef<FName> MaterialSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
		const TPolygonAttributesRef<FPolygonGroupID> PolygonGroupIDs = Attributes.GetPolygonPolygonGroupIndices();
		TMap<FPolygonGroupID, int32> PolygonGroupIDToMaterialIndex;

		const TArrayView<FPolygonGroupID>& GroupIDs = PolygonGroupIDs.GetRawArray(0);

		/// The fnGetStaticMaterialIndexForMaterialSlotName gets the material index for the specified material slot name
		const auto fnGetStaticMaterialIndexForMaterialSlotName = [&StaticMaterials](const FName& MaterialSlotName) -> int32
		{
			for (int32 i=0; i<StaticMaterials.Num(); i++)
			{
				const FStaticMaterial& Material = StaticMaterials[i];

				if (Material.ImportedMaterialSlotName.Compare(MaterialSlotName) == 0)
					return i;
			}
			return INDEX_NONE;
		};

		TMap<FPolygonGroupID, FPolygonGroupID> PolygonGroupRemapping;
		for (const auto GroupID : GroupIDs)
		{
			const FName& SlotName = MaterialSlotNames[GroupID];
			const int32 SlotIndex = fnGetStaticMaterialIndexForMaterialSlotName(SlotName);

			// NOTE: Section index is material index
			if (SlotIndex == INDEX_NONE)
				continue;

			const FPolygonGroupID NewGroupID(SlotIndex);
			PolygonGroupRemapping.Add(GroupID, NewGroupID);
		}

		if (PolygonGroupRemapping.Num() > 0)
		{
			OutMeshDescription.RemapPolygonGroups(PolygonGroupRemapping);
		}
	}
	
	// Use build settings from base mesh for LOD entries that was generated inside Editor.
	const FMeshBuildSettings& BuildSettings = bImportedMesh ? StaticMeshModel.BuildSettings : StaticMesh->GetSourceModel(0).BuildSettings;

	// Transform raw mesh to world space
	const FTransform ComponentToWorldTransform = StaticMeshComponent->GetComponentTransform();

	// If specified propagate painted vertex colors into our raw mesh
	if (bPropagateVertexColours)
	{
		FMeshMergeHelpers::PropagatePaintedColorsToMesh(StaticMeshComponent, LODIndex, OutMeshDescription);
	}

	if (bInWorldSpace)
	{
		// Transform raw mesh vertex data by the Static Mesh Component's component to world transformation	
		InstaLODVectorHelper::TransformRawMeshVertexData(ComponentToWorldTransform, OutMeshDescription);
	}

	if (OutMeshDescription.VertexInstances().Num() <= 0)
		return;

	// Figure out if we should recompute normals and tangents. By default generated LODs should not recompute normals	
	EComputeNTBsFlags TangentOptions = EComputeNTBsFlags::BlendOverlappingNormals;
	if (BuildSettings.bRemoveDegenerates)
	{
		// If removing degenerate triangles, ignore them when computing tangents.
		TangentOptions |= EComputeNTBsFlags::IgnoreDegenerateTriangles;
	}
	
	if(BuildSettings.bUseMikkTSpace)
	{
		TangentOptions |= EComputeNTBsFlags::UseMikkTSpace;
	}

	// NOTE: if the function actually recalulcates the normals, it will do so by using the polygon normals 
	if(BuildSettings.bRecomputeNormals)
	{
		TangentOptions |= EComputeNTBsFlags::Normals;
		
		TTriangleAttributesRef<FVector3f> TriangleNormals = OutMeshDescription.TriangleAttributes().GetAttributesRef<FVector3f>(MeshAttribute::Triangle::Normal);
		bool bRecreateFaceNormals = false;

		// make sure valid polygon normals are set
		if(TriangleNormals.GetNumElements() != OutMeshDescription.Triangles().Num())
		{
			bRecreateFaceNormals = true;
		}
		else 
		{	
			// check validity of face normals
			const float Threshold = 0.1f;
			for (const FTriangleID& TriangleID : OutMeshDescription.Triangles().GetElementIDs())
			{
				if(TriangleNormals[TriangleID].ContainsNaN() || TriangleNormals[TriangleID].IsNearlyZero(Threshold))
				{
					bRecreateFaceNormals = true;
					break;
				}
			}
		}

		if(bRecreateFaceNormals)
		{
			FStaticMeshOperations::ComputeTriangleTangentsAndNormals(OutMeshDescription, 0.0f);
		}
	}

	// NOTE: we need to make sure that triangle normals, binormals and tangents are present
	FStaticMeshAttributes Attributes(OutMeshDescription);

	TArrayView<FVector3f> TriangleNormals = Attributes.GetTriangleNormals().GetRawArray();
	TArrayView<FVector3f> TriangleTangents = Attributes.GetTriangleTangents().GetRawArray();
	TArrayView<FVector3f> TriangleBinormals = Attributes.GetTriangleBinormals().GetRawArray();

	// If the data isn't present we need to make sure to provide valid data
	if (TriangleNormals.Num() == 0 || TriangleTangents.Num() == 0 || TriangleBinormals.Num() == 0)
	{
		Attributes.RegisterTriangleNormalAndTangentAttributes();
	}

	FStaticMeshOperations::RecomputeNormalsAndTangentsIfNeeded(OutMeshDescription, (EComputeNTBsFlags) TangentOptions);
}

void UInstaLODUtilities::GetInstaLODMeshFromStaticMeshComponent(IInstaLOD* InstaLOD, UStaticMeshComponent* StaticMeshComponent, InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex, bool bWorldSpace)
{
	check(InstaLOD);
	check(StaticMeshComponent);
	check(OutInstaLODMesh);

	// load the MeshDescription and convert it to an InstaLOD Mesh
	FMeshDescription NewMeshDescription;

	FStaticMeshAttributes MeshAttributes(NewMeshDescription);
	MeshAttributes.Register();

	// NOTE: Register mesh attributes doesn't register polygon attributes for normals
	// to avoid an assert in Unreals internal normal recalculation we register them by hand
	NewMeshDescription.PolygonAttributes().RegisterAttribute<FVector3f>(MeshAttribute::Triangle::Normal, 1, FVector3f::ZeroVector, EMeshAttributeFlags::Transient);
	NewMeshDescription.PolygonAttributes().RegisterAttribute<FVector3f>(MeshAttribute::Triangle::Tangent, 1, FVector3f::ZeroVector, EMeshAttributeFlags::Transient);
	NewMeshDescription.PolygonAttributes().RegisterAttribute<FVector3f>(MeshAttribute::Triangle::Binormal, 1, FVector3f::ZeroVector, EMeshAttributeFlags::Transient);

	UStaticMesh *const StaticMesh = StaticMeshComponent->GetStaticMesh();

	BaseLODIndex = FMath::Clamp(BaseLODIndex, 0, StaticMesh->GetNumSourceModels() - 1);
	FMeshBuildSettings BuildSettings = StaticMesh->GetSourceModel(BaseLODIndex).BuildSettings;

	// retrieve data
	{
		UInstaLODUtilities::RetrieveMesh(StaticMeshComponent, BaseLODIndex, NewMeshDescription, true, bWorldSpace);
	}

	// fallback to LOD 0 
	if (NewMeshDescription.IsEmpty())
	{
		UInstaLODUtilities::RetrieveMesh(StaticMeshComponent, 0, NewMeshDescription, true, bWorldSpace);
		BuildSettings = StaticMesh->GetSourceModel(0).BuildSettings;
	}

	if (NewMeshDescription.IsEmpty())
	{
		OutInstaLODMesh->Clear();
		return;
	}
	
	// if necessary, recompute tangent basis
	{
		// NOTE: check if there are any normals/tangents or binormal signs set in the MeshDescription
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = MeshAttributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = MeshAttributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = MeshAttributes.GetVertexInstanceBinormalSigns();

		const bool bRecomputeNormals = BuildSettings.bRecomputeNormals || VertexInstanceNormals.GetNumElements() == 0;
		const bool bRecomputeTangents = BuildSettings.bRecomputeTangents || VertexInstanceTangents.GetNumElements() == 0 || VertexInstanceBinormalSigns.GetNumElements() == 0;

		if (bRecomputeNormals || bRecomputeTangents)
		{
			FStaticMeshOperations::ComputeTriangleTangentsAndNormals(NewMeshDescription, 0.0f);
			EComputeNTBsFlags NormalFlags = EComputeNTBsFlags::UseMikkTSpace | EComputeNTBsFlags::BlendOverlappingNormals | EComputeNTBsFlags::Tangents ;
			
			if (bRecomputeNormals)
			{
				NormalFlags |= EComputeNTBsFlags::Normals;
			}
			FStaticMeshOperations::ComputeTangentsAndNormals(NewMeshDescription, NormalFlags);
		}
	}

	InstaLOD->ConvertMeshDescriptionToInstaLODMesh(NewMeshDescription, OutInstaLODMesh);
}

void UInstaLODUtilities::GetInstaLODMeshFromSkeletalMeshComponent(IInstaLOD* InstaLOD, USkeletalMeshComponent* SkeletalMeshComponent, InstaLOD::IInstaLODMesh* OutInstaLODMesh, int32 BaseLODIndex, UAnimSequence *const BakePose)
{
	check(InstaLOD);
	check(SkeletalMeshComponent);
	check(OutInstaLODMesh);
	
	USkeletalMesh *const SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	check(SkeletalMesh);
	
	if (BaseLODIndex < 0 || BaseLODIndex > SkeletalMesh->GetLODInfoArray().Num())
		return;

	UE_SkeletalBakePoseData BakePoseData;

	if(BakePose != nullptr)
	{
		BakePoseData.BakePoseAnimation = BakePose;
		BakePoseData.ReferenceSkeleton = &SkeletalMesh->GetRefSkeleton();
		BakePoseData.SkeletalMesh = SkeletalMesh;
	}
	 
	IInstaLOD::UE_SkeletalMeshResource *const SkeletalMeshResource = SkeletalMesh->GetImportedModel();
	InstaLOD->ConvertSkeletalLODModelToInstaLODMesh(SkeletalMeshResource->LODModels[0], OutInstaLODMesh, BakePose != nullptr ? &BakePoseData : nullptr);
}

void UInstaLODUtilities::TransformInstaLODMesh(InstaLOD::IInstaLODMesh* InstaLODMesh, const FTransform& Transform, bool LocalToWorld)
{
	check(InstaLODMesh);
	check(InstaLODMesh->IsValid());
	
	uint64 VertexCount, WedgeCount;
	
	InstaLOD::InstaVec3F *const Vertices = InstaLODMesh->GetVertexPositions(&VertexCount);
	InstaLOD::InstaVec3F *const WedgeNormals = InstaLODMesh->GetWedgeNormals(&WedgeCount);
	InstaLOD::InstaVec3F *const WedgeBinormals = InstaLODMesh->GetWedgeBinormals(nullptr);
	InstaLOD::InstaVec3F *const WedgeTangents = InstaLODMesh->GetWedgeTangents(nullptr);
	
	if (LocalToWorld)
	{
		for (uint64 VertexIndex=0; VertexIndex<VertexCount; VertexIndex++)
		{
			Vertices[VertexIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.TransformPosition(InstaLODVectorHelper::InstaVecToFVector(Vertices[VertexIndex])));
		}

		for (uint64 WedgeIndex=0; WedgeIndex<WedgeCount; WedgeIndex++)
		{
			WedgeNormals[WedgeIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.TransformVector(InstaLODVectorHelper::InstaVecToFVector(WedgeNormals[WedgeIndex])).GetSafeNormal());
			WedgeBinormals[WedgeIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.TransformVector(InstaLODVectorHelper::InstaVecToFVector(WedgeBinormals[WedgeIndex])).GetSafeNormal());
			WedgeTangents[WedgeIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.TransformVector(InstaLODVectorHelper::InstaVecToFVector(WedgeTangents[WedgeIndex])).GetSafeNormal());
		}
	}
	else
	{
		for (uint64 VertexIndex=0; VertexIndex<VertexCount; VertexIndex++)
		{
			Vertices[VertexIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.InverseTransformPosition(InstaLODVectorHelper::InstaVecToFVector(Vertices[VertexIndex])));
		}
		
		for (uint64 WedgeIndex=0; WedgeIndex<WedgeCount; WedgeIndex++)
		{
			WedgeNormals[WedgeIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.InverseTransformVector(InstaLODVectorHelper::InstaVecToFVector(WedgeNormals[WedgeIndex])).GetSafeNormal());
			WedgeBinormals[WedgeIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.InverseTransformVector(InstaLODVectorHelper::InstaVecToFVector(WedgeBinormals[WedgeIndex])).GetSafeNormal());
			WedgeTangents[WedgeIndex] = InstaLODVectorHelper::FVectorToInstaVec(Transform.InverseTransformVector(InstaLODVectorHelper::InstaVecToFVector(WedgeTangents[WedgeIndex])).GetSafeNormal());
		}
	}
	
	// handle mirrored transforms
	if (Transform.GetDeterminant() < 0.0f)
	{
		InstaLODMesh->ReverseFaceDirections(/*flipNormals*/false);
	}
}

UTexture* UInstaLODUtilities::ConvertInstaLODTexturePageToTexture(InstaLOD::IInstaLODTexturePage* InstaLODTexturePage, const FString& SaveObjectPath)
{
	check(InstaLODTexturePage);
	
	const FString AssetBaseName = FPackageName::GetShortName(SaveObjectPath);
	const FString AssetBasePath = FPackageName::GetLongPackagePath(SaveObjectPath) + TEXT("/");
	
	TextureCompressionSettings TextureCompression = TC_Default;
	const bool bIsSRGB = false;
	const bool bDither = false;
	
	TArray<FColor> PixelData;
	PixelData.SetNumUninitialized(InstaLODTexturePage->GetWidth() * InstaLODTexturePage->GetHeight());
	
	// copy samples
	{
		if (bDither)
		{
			InstaLODTexturePage->Dither(InstaLOD::IInstaLODTexturePage::ComponentTypeUInt8);
		}
		
		const uint32 Width = InstaLODTexturePage->GetWidth();
		const uint32 Height = InstaLODTexturePage->GetHeight();
		
		FColor *OutData = PixelData.GetData();
		
		for (uint32 Y=0u; Y<Height; Y++)
		{
			for (uint32 X=0u; X<Width; X++)
			{
				const InstaLOD::InstaColorRGBAF32 Color = InstaLODTexturePage->SampleFloat(X, Y);
				*OutData++ = FLinearColor(Color.R, Color.G, Color.B, Color.A).ToFColor(false);
			}
		}
	}
	
	if (InstaLODTexturePage->GetType() == InstaLOD::IInstaLODTexturePage::TypeDisplacementMap ||
		InstaLODTexturePage->GetType() == InstaLOD::IInstaLODTexturePage::TypeCurvatureMap ||
		InstaLODTexturePage->GetType() == InstaLOD::IInstaLODTexturePage::TypeThicknessMap)
	{
		TextureCompression = TC_Grayscale;
	}
	else if (InstaLODTexturePage->GetType() == InstaLOD::IInstaLODTexturePage::TypeBentNormals)
	{
		TextureCompression = TC_Normalmap;
	}
	
	UTexture2D *const Texture = FMaterialUtilities::CreateTexture(nullptr, AssetBasePath + AssetBaseName,
																  FIntPoint(InstaLODTexturePage->GetWidth(), InstaLODTexturePage->GetHeight()),
																  PixelData,
																  TextureCompression,
																  TEXTUREGROUP_HierarchicalLOD, RF_Public | RF_Standalone,
																  bIsSRGB);
	
	if (Texture == nullptr)
		return nullptr;
	
	Texture->PostEditChange();
	return Texture;
}

void UInstaLODUtilities::InsertLODToMeshComponent(class IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, InstaLOD::IInstaLODMesh* InstaLODMesh, int32 TargetLODIndex, UMaterialInterface* NewMaterial)
{
	// check if it's a Static Mesh or Skeletal Mesh
	if (MeshComponent->StaticMeshComponent.IsValid())
	{
		UInstaLODUtilities::InsertLODToStaticMesh(InstaLOD, MeshComponent->StaticMeshComponent->GetStaticMesh(), InstaLODMesh, TargetLODIndex, NewMaterial);
	}
	else if (MeshComponent->SkeletalMeshComponent.IsValid())
	{
		UInstaLODUtilities::InsertLODToSkeletalMesh(InstaLOD, MeshComponent->SkeletalMeshComponent->GetSkeletalMeshAsset(), InstaLODMesh, TargetLODIndex, NewMaterial);
	}
}

void UInstaLODUtilities::InsertLODToStaticMesh(IInstaLOD* InstaLOD, UStaticMesh* StaticMesh, InstaLOD::IInstaLODMesh* InstaLODMesh, int32 TargetLODIndex, UMaterialInterface* NewMaterial)
{
	check(InstaLOD);
	check(StaticMesh);
	check(InstaLODMesh);

	FMeshSectionInfoMap SectionInfoMap = StaticMesh->GetSectionInfoMap();

	// no TargetLODIndex Specified, so we assume the user wants to add to the existing LODs
	if (TargetLODIndex < 0)
	{
		TargetLODIndex = StaticMesh->GetNumSourceModels();
	}

	const int32 BaseLOD = 0;

	if (StaticMesh->LODGroup != NAME_None)
	{
		UE_LOG(LogInstaLOD, Log, TEXT("Changing LODGroup for mesh from '%s' to 'None'."), *StaticMesh->LODGroup.ToString());
		StaticMesh->GenerateLodsInPackage();

		// NOTE: GenerateLodsInPackage does not properly copy the BuildSettings from LOD0 to the LODs
		// now baked into the package, so we have to fix this
		for (int32 Index=1; Index<StaticMesh->GetNumSourceModels(); Index++)
		{
			StaticMesh->GetSourceModel(Index).BuildSettings = StaticMesh->GetSourceModel(BaseLOD).BuildSettings;
		}
	}

	FStaticMeshSourceModel* SourceModel = nullptr;
	if (TargetLODIndex >= 0 && TargetLODIndex < StaticMesh->GetNumSourceModels())
	{
		// override Existing
		SourceModel = &StaticMesh->GetSourceModel(TargetLODIndex);
	}
	else
	{
		// add new LOD
		SourceModel = &StaticMesh->AddSourceModel();
	}

	// convert the mesh back 
	FMeshDescription& NewMeshDescription = *SourceModel->CreateMeshDescription();
	FStaticMeshAttributes(NewMeshDescription).Register();
	TMap<int32, FName> MaterialMapOut;
	TMap<FName, int32> MaterialMapIn;

	// if we want to insert a new material, we have to update the face mat indices
	if (NewMaterial != nullptr)
	{
		const int32 NewMaterialIndex = StaticMesh->GetStaticMaterials().Num();
		FStaticMaterial NewStaticMaterial(NewMaterial);
		StaticMesh->GetStaticMaterials().Add(NewStaticMaterial);

		uint64 MaterialIndexCount = 0;
		InstaLOD::InstaMaterialID *const materialIndices = InstaLODMesh->GetFaceMaterialIndices(&MaterialIndexCount);

		// replace material indices in InstaLODMesh, with new index
		for (uint32 MaterialIndex=0u; MaterialIndex<MaterialIndexCount; MaterialIndex++)
		{
			materialIndices[MaterialIndex] = InstaLOD::InstaMaterialID(NewMaterialIndex);
		}

		MaterialMapOut.Add(NewMaterialIndex, NewStaticMaterial.MaterialSlotName);
		MaterialMapIn.Add(NewStaticMaterial.MaterialSlotName, NewMaterialIndex);
	}

	InstaLOD->ConvertInstaLODMeshToMeshDescription(InstaLODMesh, MaterialMapOut, NewMeshDescription);

	if (NewMaterial == nullptr)
	{
		// we don't need the slot name as the polygongroupid correspondents to the materialindex
		const FPolygonGroupArray::TElementIDs& PolygonGroupIDs = NewMeshDescription.PolygonGroups().GetElementIDs();
		for (const FPolygonGroupID& PolygonGroupID : PolygonGroupIDs)
		{
			MaterialMapOut.Add(PolygonGroupID.GetValue(), FName());
		}
	}

	// save to bulk
	if (SourceModel->IsMeshDescriptionValid())
	{
		SourceModel->CommitMeshDescription(/*bUseHashAsGuid*/ true);
	}
	
	SourceModel->BuildSettings = StaticMesh->GetSourceModel(BaseLOD).BuildSettings;

	SourceModel->BuildSettings.bRecomputeNormals = false;
	SourceModel->BuildSettings.bRecomputeTangents = false;
	SourceModel->BuildSettings.bRemoveDegenerates = false;
	SourceModel->BuildSettings.BuildScale3D = FVector(1, 1, 1);

	// rebuild section info map
	if (NewMaterial != nullptr || SectionInfoMap.Map.Num() == 0)
	{
		TArray<int32> UniqueMaterialIndices;
		TArray<int32> MaterialIndices;
		MaterialMapOut.GetKeys(MaterialIndices);

		for (int32 MaterialIndex : MaterialIndices)
		{
			UniqueMaterialIndices.AddUnique(MaterialIndex);
		}

		FMeshSectionInfoMap OriginalSectionInfoMap = StaticMesh->GetOriginalSectionInfoMap();

		if (OriginalSectionInfoMap.Map.Num() == 0)
		{
			int32 SectionIndex = 0;
			for (int32 UniqueMaterialIndex : UniqueMaterialIndices)
			{
				SectionInfoMap.Set(TargetLODIndex, SectionIndex, FMeshSectionInfo(UniqueMaterialIndex));
				SectionIndex++;
			}
		}
		else
		{
			int32 SectionIndex = 0;
			for (int32 UniqueMaterialIndex : UniqueMaterialIndices)
			{
				if (OriginalSectionInfoMap.IsValidSection(0, UniqueMaterialIndex))
				{
					const FMeshSectionInfo SectionInfo = OriginalSectionInfoMap.Get(0, UniqueMaterialIndex);
					SectionInfoMap.Set(TargetLODIndex, SectionIndex, FMeshSectionInfo(SectionInfo.MaterialIndex));
				}
				else
				{
					SectionInfoMap.Set(TargetLODIndex, SectionIndex, FMeshSectionInfo(UniqueMaterialIndex));
				}
				SectionIndex++;
			}
		}
	}
	else
	{
		TArray<int32> UniqueMaterialIndices;
		TArray<int32> MaterialIndices;
		MaterialMapOut.GetKeys(MaterialIndices);

		for (int32 MaterialIndex : MaterialIndices)
		{
			UniqueMaterialIndices.AddUnique(MaterialIndex);
		}

		for (int32 SectionIndex=0; SectionIndex<UniqueMaterialIndices.Num(); SectionIndex++)
		{
			if (SectionInfoMap.IsValidSection(BaseLOD, UniqueMaterialIndices[SectionIndex]))
			{
				FMeshSectionInfo SectionInfo = SectionInfoMap.Get(TargetLODIndex, UniqueMaterialIndices[SectionIndex]);
				if (SectionInfoMap.IsValidSection(TargetLODIndex, SectionIndex))
				{
					FMeshSectionInfo OriginalLODSectionInfo = SectionInfoMap.Get(TargetLODIndex, SectionIndex);
					if (OriginalLODSectionInfo.MaterialIndex == SectionInfo.MaterialIndex)
					{
						SectionInfo.bCastShadow = OriginalLODSectionInfo.bCastShadow;
						SectionInfo.bEnableCollision = OriginalLODSectionInfo.bEnableCollision;
					}
				}
				StaticMesh->GetSectionInfoMap().Set(TargetLODIndex, SectionIndex, SectionInfo);
			}
		}
	}

	StaticMesh->GetSectionInfoMap().CopyFrom(SectionInfoMap);

	// On index 0 the original map needs to be reset to the current
	if (TargetLODIndex == 0)
	{
		StaticMesh->GetOriginalSectionInfoMap().CopyFrom(SectionInfoMap);
	}
	StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
	StaticMesh->Build();
	StaticMesh->PostEditChange();
	StaticMesh->MarkPackageDirty();
}

void UInstaLODUtilities::InsertLODToSkeletalMesh(IInstaLOD* InstaLOD, USkeletalMesh* SkeletalMesh, InstaLOD::IInstaLODMesh* InstaLODMesh, int32 TargetLODIndex, UMaterialInterface* NewMaterial)
{
	if (InstaLOD == nullptr || SkeletalMesh == nullptr)
		return;

	// release resources, otherwise we crash on delete of the LODModel if LODIndex is already used
	SkeletalMesh->ReleaseResources();
	SkeletalMesh->ReleaseResourcesFence.Wait();
		
	// get default resource 
	IInstaLOD::UE_SkeletalMeshResource *const SkeletalMeshResource = SkeletalMesh->GetImportedModel(); 

	// no TargetLODIndex Specified, so we assume the user wants to add to the existing LODs
	// also make sure we only add to existing LODs and don't allow more than + 1
	if (TargetLODIndex < 0 || TargetLODIndex > SkeletalMeshResource->LODModels.Num())
	{
		TargetLODIndex = SkeletalMeshResource->LODModels.Num();
	}
		
	// NOTE: we are either inserting or replacing the model in this index
	// all meshes that have this LOD index as baseModel can't rely that this 
	// mesh is the actual source, we just replace the source mesh with base index 0 
	if (TargetLODIndex != SkeletalMeshResource->LODModels.Num())
	{
		for(int32 LODInfoIndex=0; LODInfoIndex<SkeletalMeshResource->LODModels.Num(); LODInfoIndex++)
		{
			if(LODInfoIndex == TargetLODIndex)
				continue;

			FSkeletalMeshLODInfo *const LODInfo = SkeletalMesh->GetLODInfo(LODInfoIndex);
			check(LODInfo);
				
			if(LODInfo->ReductionSettings.BaseLOD != TargetLODIndex)
				continue;

			LODInfo->ReductionSettings.BaseLOD = 0;
		}
	}

	// we need the base LOD index to copy settings from
	const int32 BaseLOD = FMath::Max(TargetLODIndex - 1, 0);

	if (!SkeletalMesh->GetLODInfoArray().IsValidIndex(TargetLODIndex))
	{
		SkeletalMesh->AddLODInfo();
	}
		
	IInstaLOD::UE_StaticLODModel *const OutputLODModel = new IInstaLOD::UE_StaticLODModel();

	if (!SkeletalMeshResource->LODModels.IsValidIndex(TargetLODIndex))
	{
		SkeletalMeshResource->LODModels.Add(OutputLODModel);
	}
	else
	{
		if (SkeletalMeshResource->LODModels.GetData()[TargetLODIndex] != nullptr)
		{
			// unbind cloth if bound to lod at index
			InstaLOD->UnbindClothAtLODIndex(SkeletalMesh, TargetLODIndex);

			// delete the previous Model
			delete SkeletalMeshResource->LODModels.GetData()[TargetLODIndex]; 
		}
			
		SkeletalMeshResource->LODModels.GetData()[TargetLODIndex] = OutputLODModel;
	}
	
	// copy settings from BaseLOD
	SkeletalMesh->GetLODInfoArray()[TargetLODIndex] = SkeletalMesh->GetLODInfoArray()[BaseLOD];

	// NOTE: the new Reduction settings have optimization percentage set to 0 to avoid regeneration
	FSkeletalMeshOptimizationSettings DefaultOptimizationSettings = FSkeletalMeshOptimizationSettings();
	DefaultOptimizationSettings.NumOfTrianglesPercentage = 1.0f;
	DefaultOptimizationSettings.NumOfVertPercentage = 1.0f;
	SkeletalMesh->GetLODInfoArray()[TargetLODIndex].ReductionSettings = DefaultOptimizationSettings;

	if (SkeletalMesh->GetLODInfoArray()[TargetLODIndex].ScreenSize.Default <= 0.0f)
	{
		// some meshes have 0.0 as screensize, use 0.5 in these cases as we're typically LOD1
		SkeletalMesh->GetLODInfoArray()[TargetLODIndex].ScreenSize = FPerPlatformFloat(0.5f);
	}
	else
	{
		// NOTE: use half the screen size of our base LOD
		SkeletalMesh->GetLODInfoArray()[TargetLODIndex].ScreenSize = FPerPlatformFloat(SkeletalMesh->GetLODInfoArray()[TargetLODIndex].ScreenSize.Default * 0.5f);
	} 

	// if we have a custom material we have to insert it and update the face indices
	// to make the process easier, we do this on the InstaLODMesh before building the skeletal mesh
	if (NewMaterial != nullptr)
	{
		const int32 NewMaterialIndex = SkeletalMesh->GetMaterials().Num();
		SkeletalMesh->GetMaterials().Add(FSkeletalMaterial(NewMaterial));
		
		uint64 FaceCount;
		int32 *const FaceMaterials = InstaLODMesh->GetFaceMaterialIndices(&FaceCount);
		for (uint64 FaceIndex=0; FaceIndex<FaceCount; FaceIndex++)
		{
			FaceMaterials[FaceIndex] = NewMaterialIndex;
		}
	}

	FSkeletalMeshImportData ImportMesh;
	// convert the InstaLODMesh back and use the previously created LODModel that we already added to the TargetLODIndex
	InstaLOD->ConvertInstaLODMeshToSkeletalLODModel(InstaLODMesh, SkeletalMesh, ImportMesh, *OutputLODModel);
	TMap<FString, TArray<FMorphTargetDelta>> TempMap;
	
	SkeletalMesh->SaveLODImportedData(TargetLODIndex, ImportMesh);

	// mark the Mesh and LOD (not as simplified to avoid regeneration)
	SkeletalMesh->GetLODInfoArray()[TargetLODIndex].bHasBeenSimplified = false; 
	SkeletalMesh->SetHasBeenSimplified(true);

	OutputLODModel->SyncronizeUserSectionsDataArray();

	// reinit the resources after we cleaned them up earlier in the process
	SkeletalMesh->PostEditChange();
	SkeletalMesh->InitResources();

	// calculate the required bones
	SkeletalMesh->CalculateRequiredBones(*OutputLODModel, SkeletalMesh->GetRefSkeleton(), nullptr);
	SkeletalMesh->MarkPackageDirty();
}

void UInstaLODUtilities::UpdateMeshOfMeshComponent(TSharedPtr<FInstaLODMeshComponent> MeshComponent, UObject* NewMesh, bool bRemoveOverrideMaterials)
{
	if (NewMesh == nullptr)
		return;

	if (MeshComponent->StaticMeshComponent.IsValid())
	{
		UStaticMesh* const NewStaticMesh = Cast<UStaticMesh>(NewMesh);
		check(NewStaticMesh);
		
		MeshComponent->StaticMeshComponent->SetStaticMesh(NewStaticMesh);
	}
	else if (MeshComponent->SkeletalMeshComponent.IsValid())
	{
		USkeletalMesh* const NewSkeletalMesh = Cast<USkeletalMesh>(NewMesh);
		check(NewSkeletalMesh);
		
		MeshComponent->SkeletalMeshComponent->SetSkeletalMesh(NewSkeletalMesh);
	}
	
	// remove all override materials
	if (bRemoveOverrideMaterials)
	{
		UMeshComponent *const MeshComponentBase = Cast<UMeshComponent>(MeshComponent->GetComponent());
		check(MeshComponentBase);
		
		MeshComponentBase->EmptyOverrideMaterials();
		MeshComponentBase->PostEditChange();
	}
}

UObject* UInstaLODUtilities::SaveInstaLODMeshToDuplicateAsset(class IInstaLOD* InstaLOD, TSharedPtr<FInstaLODMeshComponent> MeshComponent, InstaLOD::IInstaLODMesh* InstaLODMesh, const FString& SaveObjectPath, bool clearMaterialAndSectionInfo)
{
	FAssetToolsModule& AssetTools = FAssetToolsModule::GetModule();
	
	const FString AssetBasePath = FPackageName::GetLongPackagePath(SaveObjectPath);
	const FString AssetBaseName = FPackageName::GetShortName(SaveObjectPath);
	
	UObject* OriginalAsset = nullptr;
	UObject* DuplicatedAsset = nullptr;

	if (MeshComponent->StaticMeshComponent.IsValid())
	{
		OriginalAsset = MeshComponent->StaticMeshComponent->GetStaticMesh();
		DuplicatedAsset = AssetTools.Get().DuplicateAsset(AssetBaseName, AssetBasePath, OriginalAsset);
		
		if (DuplicatedAsset == nullptr)
			return nullptr;
		
		UStaticMesh *const StaticMesh = Cast<UStaticMesh>(DuplicatedAsset);

		// we want to make sure that we remove all existing LODs, as this new Mesh should only contain the LOD 0
		UInstaLODUtilities::RemoveAllLODsFromStaticMesh(StaticMesh);

		// if a new material is generated the section info map should be cleared beforehand
		if (clearMaterialAndSectionInfo)
		{
			StaticMesh->GetOriginalSectionInfoMap().Clear();
			StaticMesh->GetSectionInfoMap().Clear();
			StaticMesh->GetStaticMaterials().Empty();
		}
		else
		{
			// NOTE: we need the original section info map to rebuild the section to material index mapping
			StaticMesh->GetOriginalSectionInfoMap().CopyFrom(StaticMesh->GetSectionInfoMap());
			StaticMesh->GetSectionInfoMap().Clear();
		}
		UInstaLODUtilities::InsertLODToStaticMesh(InstaLOD, StaticMesh, InstaLODMesh, 0, nullptr);
	}
	else if (MeshComponent->SkeletalMeshComponent.IsValid())
	{
		OriginalAsset = MeshComponent->SkeletalMeshComponent->GetSkeletalMeshAsset();
		DuplicatedAsset = AssetTools.Get().DuplicateAsset(AssetBaseName, AssetBasePath, OriginalAsset);
		
		if (DuplicatedAsset == nullptr)
			return nullptr;
		
		USkeletalMesh *const SkeletalMesh = Cast<USkeletalMesh>(DuplicatedAsset);

		// we want to make sure that we remove all existing LODs, as this new Mesh should only contain the LOD 0
		UInstaLODUtilities::RemoveAllLODsFromSkeletalMesh(SkeletalMesh);

		// insert the InstaLODMesh to LODIndex 0
		UInstaLODUtilities::InsertLODToSkeletalMesh(InstaLOD, SkeletalMesh, InstaLODMesh, 0, nullptr);
	}

	return DuplicatedAsset;
}

InstaLOD::IInstaLODMesh* UInstaLODUtilities::CreatePlane(class IInstaLOD* InstaLOD, const FVector2d Extents)
{
	check(InstaLOD);
	using namespace InstaLOD;
	
	constexpr int32 PositionCount = 4;
	constexpr int32 WedgeCount = 6;
	constexpr int32 FaceCount = 2;
	
	IInstaLODMesh *const InstaMesh = InstaLOD->AllocInstaLODMesh();
	InstaMesh->ResizeVertexPositions(PositionCount);
	InstaMesh->ResizeWedgeNormals(WedgeCount);
	InstaMesh->ResizeWedgeTexCoords(0, WedgeCount);
	InstaMesh->ResizeWedgeIndices(WedgeCount);
	InstaMesh->ResizeWedgeTangents(WedgeCount);
	InstaMesh->ResizeWedgeBinormals(WedgeCount);
	InstaMesh->ResizeFaceSmoothingGroups(FaceCount);
	InstaMesh->ResizeFaceSubMeshIndices(FaceCount);
	InstaMesh->ResizeFaceMaterialIndices(FaceCount);
	
	InstaVec3F *const VertexPositions = InstaMesh->GetVertexPositions(nullptr);
	InstaVec3F *const WedgeNormals = InstaMesh->GetWedgeNormals(nullptr);
	InstaVec2F *const WedgeTexCoords = InstaMesh->GetWedgeTexCoords(0, nullptr);
	uint32 *const WedgeIndices = InstaMesh->GetWedgeIndices(nullptr);
	uint32 *const FaceSmoothingGroups = InstaMesh->GetFaceSmoothingGroups(nullptr);
	uint32 *const FaceSubmeshIndices = InstaMesh->GetFaceSubMeshIndices(nullptr);
	InstaMaterialID *const FaceMaterialIndices = InstaMesh->GetFaceMaterialIndices(nullptr);
	
	VertexPositions[0] = InstaVec3F(-Extents[0], Extents[1], 0);
	VertexPositions[1] = InstaVec3F(Extents[0], Extents[1], 0);
	VertexPositions[2] = InstaVec3F(-Extents[0], -Extents[1], 0);
	VertexPositions[3] = InstaVec3F(Extents[0], -Extents[1], 0);
	
	const InstaVec3F UpVector = InstaVec3F(0, 0, 1.0f);
	WedgeNormals[0] = UpVector;
	WedgeNormals[1] = UpVector;
	WedgeNormals[2] = UpVector;
	WedgeNormals[3] = UpVector;
	WedgeNormals[4] = UpVector;
	WedgeNormals[5] = UpVector;

	check(!FMath::IsNearlyZero(Extents[0]) && !FMath::IsNearlyZero(Extents[0]));
	const float XYRatio = Extents[0]/Extents[1];
	float XDimension = 1.0f;
	float YDimension = 1.0f;

	if (XYRatio > 1.0f)
	{
		YDimension = 1.0f/XYRatio;
	}
	else if (XYRatio < 1.0f)
	{
		XDimension = XYRatio;
	}

	WedgeTexCoords[0] = InstaVec2F(YDimension, 0.0f);
	WedgeTexCoords[1] = InstaVec2F(YDimension, XDimension);
	WedgeTexCoords[2] = InstaVec2F(0.0f, 0.0f);
	WedgeTexCoords[3] = InstaVec2F(0.0f, 0.0f);
	WedgeTexCoords[4] = InstaVec2F(YDimension, XDimension);
	WedgeTexCoords[5] = InstaVec2F(0.0f, XDimension);
	
	WedgeIndices[0] = 0u;
	WedgeIndices[1] = 1u;
	WedgeIndices[2] = 2u;
	WedgeIndices[3] = 2u;
	WedgeIndices[4] = 1u;
	WedgeIndices[5] = 3u;
	
	FaceSubmeshIndices[0] = 0u;
	FaceSubmeshIndices[1] = 0u;
	
	FaceSmoothingGroups[0] = 0u;
	FaceSmoothingGroups[1] = 0u;

	FaceMaterialIndices[0] = 0;
	FaceMaterialIndices[1] = 0;

	InstaLOD->GetInstaLOD()->CastToInstaLODMeshExtended(InstaMesh)->CalculateTangentsWithMikkTSpace(0);
	
	return InstaMesh;
}

UObject* UInstaLODUtilities::SaveInstaLODMeshToSkeletalMeshAsset(class IInstaLOD* InstaLOD, class InstaLOD::IInstaLODMesh* InstaLODMesh, const FString& SaveObjectPath)
{
	const FString AssetBaseName = FPackageName::GetShortName(SaveObjectPath);
	const FString AssetBasePath = FPackageName::GetLongPackagePath(SaveObjectPath) + TEXT("/");
	const FString PackageName = AssetBasePath + AssetBaseName;

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	if (!UPackage::IsEmptyPackage(Package))
	{
		// NOTE: we have to delete the old object in the package or UE will crash if the type is different from the one we're creating
		UObject* const ExistingObject = FindObject<UObject>(Package, *AssetBaseName);

		if (ExistingObject != nullptr)
		{
			TArray<UObject*> DeleteObjects;
			DeleteObjects.Add(ExistingObject);

			// delete previous object
			if (!ObjectTools::ForceDeleteObjects(DeleteObjects, true))
				return nullptr;

			// trigger GC to ensure it's fully removed
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

			// reload the package
			Package = CreatePackage(*PackageName);
			Package->FullyLoad();
		}
	}
	Package->Modify();

	USkeletalMesh* const SkeletalMesh = NewObject<USkeletalMesh>(Package, FName(*AssetBaseName), RF_Public | RF_Standalone);
	SkeletalMesh->InitResources();

	// insert the InstaLODMesh to LODIndex 0
	UInstaLODUtilities::InsertLODToSkeletalMesh(InstaLOD, SkeletalMesh, InstaLODMesh, 0, nullptr);

	SkeletalMesh->Build();
	SkeletalMesh->PostEditChange();

	return SkeletalMesh;
}

UObject* UInstaLODUtilities::SaveInstaLODMeshToStaticMeshAsset(class IInstaLOD* InstaLOD, class InstaLOD::IInstaLODMesh* InstaLODMesh, const FString& SaveObjectPath)
{
	const FString AssetBaseName = FPackageName::GetShortName(SaveObjectPath);
	const FString AssetBasePath = FPackageName::GetLongPackagePath(SaveObjectPath) + TEXT("/");
	const FString PackageName = AssetBasePath + AssetBaseName;

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();
	
	if (!UPackage::IsEmptyPackage(Package))
	{
		// NOTE: we have to delete the old object in the package or UE will crash if the type is different from the one we're creating
		UObject *const ExistingObject = FindObject<UObject>(Package, *AssetBaseName);
		
		if (ExistingObject != nullptr)
		{
			TArray<UObject*> DeleteObjects;
			DeleteObjects.Add(ExistingObject);
			
			// delete previous object
			if (!ObjectTools::ForceDeleteObjects(DeleteObjects, true))
				return nullptr;
			
			// trigger GC to ensure it's fully removed
			CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
			
			// reload the package
			Package = CreatePackage(*PackageName);
			Package->FullyLoad();
		}
	}
	Package->Modify();
	
	UStaticMesh *const StaticMesh = NewObject<UStaticMesh>(Package, FName(*AssetBaseName), RF_Public | RF_Standalone);
	StaticMesh->InitResources();
	
	// assign new lighting guid
	StaticMesh->SetLightingGuid();
	StaticMesh->SetLightMapCoordinateIndex(1);
	
	FStaticMeshSourceModel& SourceModel = StaticMesh->AddSourceModel();

	SourceModel.BuildSettings.bRecomputeNormals = false;
	SourceModel.BuildSettings.bRecomputeTangents = false;
	SourceModel.BuildSettings.bRemoveDegenerates = false;
	SourceModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SourceModel.BuildSettings.bUseFullPrecisionUVs = false;
	
	// insert the InstaLODMesh to LODIndex 0
	UInstaLODUtilities::InsertLODToStaticMesh(InstaLOD, StaticMesh, InstaLODMesh, 0, nullptr);
	
	StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
	StaticMesh->Build();
	StaticMesh->PostEditChange();
	
	return StaticMesh;
}

void UInstaLODUtilities::RemoveAllLODsFromStaticMesh(class UStaticMesh* StaticMesh)
{
	if (StaticMesh == nullptr)
		return;

	for (int32 Index=StaticMesh->GetNumSourceModels() - 1; Index>0; Index--)
	{
		StaticMesh->RemoveSourceModel(Index);
	}

	StaticMesh->PostEditChange();
	StaticMesh->InitResources();
}

void UInstaLODUtilities::RemoveAllLODsFromSkeletalMesh(class USkeletalMesh* SkeletalMesh)
{
	if (SkeletalMesh == nullptr)
		return;
	
	IInstaLOD::UE_SkeletalMeshResource* const SkeletalMeshResource = SkeletalMesh->GetImportedModel(); 
	check(SkeletalMeshResource);
	
	SkeletalMesh->ReleaseResources();
	SkeletalMesh->ReleaseResourcesFence.Wait();
	
	// block until this is done
	FlushRenderingCommands();
	
	while (SkeletalMeshResource->LODModels.Num() > 1)
	{
		SkeletalMeshResource->LODModels.RemoveAt(1);
	}
	
	while (SkeletalMesh->GetLODInfoArray().Num() > 1)
	{
		SkeletalMesh->GetLODInfoArray().RemoveAt(1);
	}

	SkeletalMesh->GetImportedModel()->InlineReductionCacheDatas.Empty();
	FLODUtilities::RefreshLODChange(SkeletalMesh);

	SkeletalMesh->PostEditChange();
	SkeletalMesh->InitResources();
}

FString UInstaLODUtilities::OpenSaveDialog(const FText& DialogTitle, const FString& DefaultPackageName, const FString &AssetNamePrefix, bool bMultiSave)
{
	const FString DefaultPath = FPackageName::GetLongPackagePath(DefaultPackageName);
	const FString DefaultName = FPackageName::GetShortName(DefaultPackageName);

	FSaveAssetDialogConfig SaveAssetDialogConfig;
	SaveAssetDialogConfig.DialogTitleOverride = DialogTitle;
	SaveAssetDialogConfig.DefaultPath = DefaultPath;
	SaveAssetDialogConfig.DefaultAssetName = AssetNamePrefix + DefaultName;
	SaveAssetDialogConfig.ExistingAssetPolicy = bMultiSave ? ESaveAssetDialogExistingAssetPolicy::Disallow : ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	const FString Path = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);

	return FPackageName::ObjectPathToPackageName(Path);
}

FString UInstaLODUtilities::EnsureSavePathIsUniqueForEntry(const FString& SavePath, UObject* const Entry, TSet<FString>& PathNamesSet, const FString& Default)
{
	check(Entry);

	FString SaveObjectPath = SavePath;
	FString ProjectContentDirectory = FPaths::Combine(FPaths::ProjectContentDir(), SaveObjectPath);

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	
	if (FileManager.DirectoryExists(*ProjectContentDirectory))
	{
		if (Default.IsEmpty())
		{
			SaveObjectPath = SaveObjectPath + TEXT("/") + Entry->GetName();
		}
		else
		{
			SaveObjectPath = SaveObjectPath + TEXT("/") + Default;
		}
	}
	else
	{
		UE_LOG(LogInstaLOD, Warning, TEXT("Directory does not exist, '%s'"), *ProjectContentDirectory);
	}

	if (PathNamesSet.Num() > 0)
	{
		int32 FileSuffix = 1;

		while (PathNamesSet.Contains(SaveObjectPath))
		{
			SaveObjectPath.RemoveFromEnd(FString::FromInt(FileSuffix - 1));
			SaveObjectPath = SaveObjectPath + FString::FromInt(FileSuffix);
			FileSuffix++;
		}
	}
	PathNamesSet.Add(SaveObjectPath);
	return SaveObjectPath;
}

FString UInstaLODUtilities::GetValidSavePath(const FString& Path)
{
	check(!Path.IsEmpty());

	FString SavePath = Path;
	FPaths::NormalizeFilename(SavePath);
	FPaths::NormalizeDirectoryName(SavePath); 
	
	if (SavePath.StartsWith(TEXT("/")))
	{
		SavePath.RemoveAt(0);
	}

	if (SavePath.StartsWith(TEXT("Game/")))
	{
		SavePath.RemoveFromStart(TEXT("Game/"));
	}

	return SavePath;
}

FString UInstaLODUtilities::GetInvalidCharactersForSavePath()
{
	return FString(" .,\"'$%&()[]{}<>=*`~#:;|@?\\^");
}

bool UInstaLODUtilities::DoesSavePathContainIllegalCharacters(const FString& SavePath)
{
	// NOTE: FPaths::GetInvalidFileSystemChars() is not restrictive enough.
	const FString InvalidCharacters = GetInvalidCharactersForSavePath();

	for (const TCHAR Character : SavePath)
	{
		if (InvalidCharacters.Contains(&Character))
			return true;
	}

	return false;
}

bool UInstaLODUtilities::CheckIfAssetExists(const FString& SaveObjectPath)
{
	const FString AssetBaseName = FPackageName::GetShortName(SaveObjectPath);
	const FString AssetBasePath = FPackageName::GetLongPackagePath(SaveObjectPath) + TEXT("/");
	const FString PackageName = AssetBasePath + AssetBaseName;

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	if (!UPackage::IsEmptyPackage(Package))
	{
		UObject* const ExistingObject = FindObject<UObject>(Package, *AssetBaseName);

		if (ExistingObject != nullptr)
			return true;
	}
	return false;
}

TArray<InstaLODMergeData> UInstaLODUtilities::CreateMergeData(TArray<TSharedPtr<FInstaLODMeshComponent>>& MeshComponents, IInstaLOD* const InstaLODInterface, const int BaseLODIndex)
{
	check(InstaLODInterface);
	check(BaseLODIndex >= 0);

	// create merge data array that contains instalod meshes and their original component
	TArray<InstaLODMergeData> MergeData;

	for (TSharedPtr<FInstaLODMeshComponent>& Component : MeshComponents)
	{
		InstaLODMergeData MergeDataEntry;
		MergeDataEntry.Component = Component.Get();
		MergeDataEntry.InstaLODMesh = InstaLODInterface->AllocInstaLODMesh();

		UInstaLODUtilities::GetInstaLODMeshFromMeshComponent(InstaLODInterface, Component, MergeDataEntry.InstaLODMesh, 0, false);
		MergeData.Add(MergeDataEntry);

		check(MergeDataEntry.InstaLODMesh);
	}

	return MergeData;
}

bool UInstaLODUtilities::FinalizeScriptProcessResult(UObject* const Entry, IInstaLOD* const InstaLODInterface, TSharedPtr<FInstaLODMeshComponent>& MeshComponent, InstaLOD::IInstaLODMesh* const OutputMesh, UInstaLODResultSettings* const ResultSettings, TArray<UObject*>& OutputArray, UMaterialInstanceConstant* const Material, const bool bIsFreezingTransformsForMultiSelection)
{
	if (Entry == nullptr || InstaLODInterface == nullptr || OutputMesh == nullptr || ResultSettings == nullptr)
		return false;

	if (!Entry->IsA<UStaticMesh>() && !Entry->IsA<USkeletalMesh>())
	{
		UE_LOG(LogInstaLOD, Error, TEXT("Object is not of Static or Skeletal Mesh type."));
		return false;
	}

	switch (ResultSettings->SavingOption)
	{
		case EInstaLODSavingOption::InsertAsLOD:
		{
			UInstaLODUtilities::InsertLODToMeshComponent(InstaLODInterface, MeshComponent, OutputMesh, ResultSettings->TargetLODIndex, Material);
			OutputArray.Add(Entry);
			break;
		}
		case EInstaLODSavingOption::OnlyReturnInOutArray:
		{
			if (Entry->IsA<UStaticMesh>())
			{
				UStaticMesh* const StaticMesh = NewObject<UStaticMesh>();
				UInstaLODUtilities::InsertLODToStaticMesh(InstaLODInterface, StaticMesh, OutputMesh, 0, Material);

				if (ResultSettings->PivotPosition != EInstaLODPivotPosition::InstaLOD_Default && bIsFreezingTransformsForMultiSelection)
				{
					FVector Position;
					bool bLimitedRange = true;
					switch (ResultSettings->PivotPosition)
					{
					case EInstaLODPivotPosition::InstaLOD_Center:
						Position.Set(0.0f, 0.0f, 0.0f);
						break;

					case EInstaLODPivotPosition::InstaLOD_Top:
						Position.Set(0.0f, 0.0f, 1.0f);
						break;

					case EInstaLODPivotPosition::InstaLOD_Bottom:
						Position.Set(0.0f, 0.0f, -1.0f);
						break;

					case EInstaLODPivotPosition::InstaLOD_Custom:
						Position = ResultSettings->Position;
						bLimitedRange = false;
						break;

					case EInstaLODPivotPosition::InstaLOD_CustomLimited:
						Position = (ClampVector(ResultSettings->Position, FVector::ZeroVector, FVector::OneVector) - 0.5f) * 2;
						break;

					default:
						break;
					}

					UInstaLODUtilities::CustomizePivotPosition(StaticMesh, Position, bLimitedRange);
				}

				UStaticMeshComponent* const StaticMeshComponent = NewObject<UStaticMeshComponent>();
				StaticMeshComponent->SetStaticMesh(StaticMesh);
				UObject* const OutputObject = Cast<UObject>(StaticMeshComponent);
				OutputArray.Add(OutputObject);
			}
			else if (Entry->IsA<USkeletalMesh>())
			{
				USkeletalMesh* const SkeletalMesh = NewObject<USkeletalMesh>();
				UInstaLODUtilities::InsertLODToSkeletalMesh(InstaLODInterface, SkeletalMesh, OutputMesh, 0, Material);
				USkeletalMeshComponent* const SkeletalMeshComponent = NewObject<USkeletalMeshComponent>();
				SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
				UObject* const OutputObject = Cast<UObject>(SkeletalMeshComponent);
				OutputArray.Add(OutputObject);
			}
			break;
		}
		default:
			check(false);
	}

	return true;
}