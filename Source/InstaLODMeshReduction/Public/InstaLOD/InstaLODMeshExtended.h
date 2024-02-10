/**
 * InstaLODMeshExtended.h (InstaLOD)
 *
 * Copyright 2016-2023 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODMeshExtended.h
 * @copyright 2016-2023 InstaLOD GmbH. All rights reserved.
 * @section License
 */

#ifndef InstaLOD_InstaLODMeshExtended_h
#define InstaLOD_InstaLODMeshExtended_h

namespace InstaLOD
{
	/**
	 * The InstaLODRenderMeshConstructionSettings structure contains settings
	 * used to construct an IInstaLODRenderMesh.
	 */
	struct InstaLODRenderMeshConstructionSettings
	{
		InstaLODRenderMeshConstructionSettings() :
		SplitColor(true),
		SplitTangents(true),
		SplitNormals(true),
		SplitSmoothingGroup(false),
		SplitMaterialIndex(true),
		SplitMirroredUV(true),
		SplitOptimizerWeight(false),
		SplitSubMeshID(true),
		SplitSkinnedVertices(true),
		IgnoreDegenerateFaces(true),
		IgnoreDuplicateFaces(true),
		SplitTexCoordCompareRangeStart(0),
		SplitTexCoordCompareRangeCount(INSTALOD_MAX_MESH_TEXCOORDS),
		SplitColorCompareRangeStart(0),
		SplitColorCompareRangeCount(INSTALOD_MAX_MESH_COLORSETS)
		{
		}

		bool SplitColor;					   /**< Generate a split vertex if wedge colors don't match. */
		bool SplitTangents;					   /**< Generate a split vertex if wedge tangents don't match. */
		bool SplitNormals;					   /**< Generate a split vertex if wedge normals don't match. */
		bool SplitSmoothingGroup;			   /**< Generate a split vertex if wedge smoothing groups don't match. */
		bool SplitMaterialIndex;			   /**< Generate a split vertex if wedge material indices don't match. */
		bool SplitMirroredUV;				   /**< Generate a split vertex if wedge UVs are mirrored. */
		bool SplitOptimizerWeight;			   /**< Generate a split vertex if wedge optimizer weights don't match. */
		bool SplitSubMeshID;				   /**< Generate a split vertex if wedge sub mesh IDs don't match. */
		bool SplitSkinnedVertices;			   /**< Generate a split vertex if bone weights don't match. */
		bool IgnoreDegenerateFaces;			   /**< Ignore degenerate faces when building the render mesh. */
		bool IgnoreDuplicateFaces;			   /**< Ignore duplicate faces when building the render mesh. */
		uint32 SplitTexCoordCompareRangeStart; /**< First texture coordinate set to check for splits. */
		uint32 SplitTexCoordCompareRangeCount; /**< The amount of texture coordinates that generate a split vertex if UVs don't match. */
		uint32 SplitColorCompareRangeStart;	   /**< First color set to check for splits. */
		uint32 SplitColorCompareRangeCount;	   /**< The amount of color sets that generate a split vertex if UVs don't match. */
	};

	/**
	 * The InstaLODRenderMeshVertex represents a vertex of an IInstaLODRenderMesh.
	 */
	struct InstaLODRenderMeshVertex
	{
		InstaVec3F Position;								  /**< The position. */
		InstaVec3F Normal;									  /**< The normal. */
		InstaVec3F Binormal;								  /**< The binormal (also known as bitangent). */
		InstaVec3F Tangent;									  /**< The tangent. */
		InstaColorRGBAF32 Color[INSTALOD_MAX_MESH_COLORSETS]; /**< The vertex color. */
		InstaVec2F TexCoord[INSTALOD_MAX_MESH_TEXCOORDS];	  /**< The UV coordinates. */
		float OptimizerWeight;								  /**< The optimizer weight. */
		uint32 SmoothingGroup;								  /**< The smoothing group. */
		InstaMaterialID MaterialIndex;						  /**< The material index. */
		uint32 SubMeshID;									  /**< The submesh ID. */
	};

	/**
	 * The IInstaLODRenderMeshBase class represets a light-weight triangle-mesh for rendering.
	 */
	class IInstaLODRenderMeshBase
	{
	protected:
		virtual ~IInstaLODRenderMeshBase() {}
		
	public:
		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaLODRenderMeshVertex* GetVertices(uint64* count) = 0;
		
		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaLODRenderMeshVertex* GetVertices(uint64* count) const = 0;
		
		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual uint32* GetIndices(uint64* count) = 0;
		
		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const uint32* GetIndices(uint64* count) const = 0;
	};
	
	/**
	 * The IInstaLODRenderMesh class can be used to convert the wedge-based IInstaLODMesh instance
	 * into a vertexsplit-based mesh representation.
	 * Split-based mesh representations are typically used for real-time rendering.
	 */
	class IInstaLODRenderMesh : public IInstaLODRenderMeshBase
	{
	protected:
		virtual ~IInstaLODRenderMesh() {}

	public:
		/**
		 * Constructs the mesh buffers.
		 *
		 * @param settings The render mesh build settings.
		 * @return true upon success.
		 */
		virtual bool Construct(const InstaLODRenderMeshConstructionSettings& settings) = 0;

		/**
		 * Gets an array that maps the wedge indices of the input mesh
		 * to render vertex indices.
		 * @note It is possible that not all wedge indices map to render vertices as degenerate faces
		 * can be ommited during construction.
		 * Unmapped wedge indices are mapped to ~0u.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual uint32* GetWedgeIndexToRenderVertexMap(uint64* count) = 0;

		/**
		 * Gets an array that maps the wedge indices of the input mesh
		 * to render vertex indices.
		 * @note It is possible that not all wedge indices map to render vertices as degenerate faces
		 * can be ommited during construction.
		 * Unmapped wedge indices are mapped to ~0u.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const uint32* GetWedgeIndexToRenderVertexMap(uint64* count) const = 0;

		/**
		 * Gets the IInstaLODSkinnedVertexData of this instance.
		 *
		 * @return IInstaLODSkinnedVertexData instance
		 */
		virtual IInstaLODSkinnedVertexData& GetSkinnedVertexData() = 0;

		/**
		 * Gets the IInstaLODSkinnedVertexData of this instance.
		 *
		 * @return IInstaLODSkinnedVertexData instance
		 */
		virtual const IInstaLODSkinnedVertexData& GetSkinnedVertexData() const = 0;
		
		/**
		 * Writes a LightWave OBJ representation of this instance to the
		 * specified file path.
		 *
		 * @param path the output path for the OBJ file
		 * @return true if the file has been written.
		 */
		virtual bool WriteLightwaveOBJ(const char* path) const = 0;
	};

	typedef bool (*pfnMeshIntersectorFilterCallback)(const IInstaLODMesh* mesh, const uint32 faceIndex, const double s, const double t, const InstaVec3F& barycentric, void* userData);
	typedef bool (*pfnMeshSphereIntersectorFilterCallback)(const uint32 faceIndex, double distance, const InstaVec3F& barycentric, void* userData);

	/**
	 * The IInstaLODMeshIntersector class can be used to perform accelerated 
	 * segment-triangle intersection tests against an IInstaLODMesh.
	 */
	class IInstaLODMeshIntersector
	{
	protected:
		virtual ~IInstaLODMeshIntersector() {}

	public:
		/**
		 * Performs segment-triangle intersection test and returns the closest point to \p p.
		 *
		 * @param p segment start
		 * @param q segment end
		 * @param outFaceIndex pointer to a variable to store the hit face index
		 * @param outS pointer to a variable to store the intersection fraction
		 * @param outT pointer to a variable to store the intersection fraction
		 * @param outBarycentric pointer to a variable to store the barycentric coordinate for the hit face.
		 * @return true if a face was intersected by the specified segment.
		 */
		virtual bool Intersects(const InstaVec3F& p, const InstaVec3F& q, uint32* outFaceIndex, double* outS, double* outT, InstaVec3F* outBarycentric) const = 0;

		/**
		 * Performs a filtered segment-triangle intersection test.
		 *
		 * @param p segment start
		 * @param q segment end
		 * @param outFaceIndex pointer to a variable to store the hit face index
		 * @param outS pointer to a variable to store the intersection fraction
		 * @param outT pointer to a variable to store the intersection fraction
		 * @param outBarycentric pointer to a variable to store the barycentric coordinate for the hit face.
		 * @param filterCallback user specified function to return true if the hit is valid
		 * @param userData (optional) User data to be passed into the callback.
		 * @return true if a face was intersected by the specified segment.
		 */
		virtual bool IntersectsFiltered(const InstaVec3F& p, const InstaVec3F& q, uint32* outFaceIndex, double* outS, double* outT, InstaVec3F* outBarycentric, pfnMeshIntersectorFilterCallback filterCallback, void* userData) const = 0;

		/**
		 * Performs a filtered triangle-sphere intersection test.
		 *
		 * @param origin sphere origin
		 * @param sphereRadius sphere radius
		 * @param filterCallback user specified function to return true if the hit is valid, if the callback returns false, the search is aborted.
		 * @param userData (optional) User data to be passed into the callback.
		 * @return true if a face was intersected by the specified sphere.
		 */
		virtual bool IntersectsSphereFiltered(const InstaVec3F& origin, const float sphereRadius, pfnMeshSphereIntersectorFilterCallback filterCallback, void* userData) const = 0;

		/**
		 * Renders the BVH to the specified renderer.
		 *
		 * @param renderer the viewport renderer.
		 */
		virtual void DebugRenderBVH(class IInstaLODViewportRenderer* renderer) const = 0;
	};

	typedef void (*pfnMeshProgressCallback)(class IInstaLODMesh* mesh, const float progressInPercent);
	
	/**
	 * The IInstaLODMeshExtended class provides additional functions to IInstaLODMesh.
	 * @note Any IInstaLODMesh instance is guaranteed to be castable into the IInstaLODMeshExtended type.
	 */
	class IInstaLODMeshExtended : public IInstaLODMesh
	{
	protected:
		virtual ~IInstaLODMeshExtended() {}

	public:
		/**
		 * Gets the name of the data element at the specified index.
		 * @note If no name has been assigned, a default name will be returned.
		 *
		 * @param index the element index
		 * @return the name of the set at the specified index
		 */
		virtual const char* GetWedgeTexCoordSetName(const uint64 index) const = 0;

		/**
		 * Sets the name of the data element at the specified index.
		 *
		 * @param index the element index
		 * @param name the name
		 */
		virtual void SetWedgeTexCoordSetName(const uint64 index, const char* name) = 0;

		/**
		 * Gets the name of the data element at the specified index.
		 * @note If no name has been assigned, a default name will be returned.
		 *
		 * @param index the element index
		 * @return the name of the set at the specified index
		 */
		virtual const char* GetWedgeColorSetName(const uint64 index) const = 0;

		/**
		 * Sets the name of the data element at the specified index.
		 *
		 * @param index the element index
		 * @param name the name
		 */
		virtual void SetWedgeColorSetName(const uint64 index, const char* name) = 0;

		/**
		 * Removes degenerate faces.
		 * @note This method classifies faces as degenerate when a single face either has duplicate wedge indices
		 * or overlapping vertex positions.
		 *
		 * @param comparisonThreshold points closer than the specified value will be considered equal.
		 * @return the amount of removed faces.
		 */
		virtual uint32 RemoveDegenerateFaces(float comparisonThreshold) = 0;

		/**
		 * Removes the specified face.
		 *
		 * @param faceIndex the face index
		 */
		virtual void RemoveFace(uint32 faceIndex) = 0;

		/**
		 * Removes the specified faces.
		 *
		 * @param faces an array containing the face indices to remove
		 * @param faceCount the amount of faces stored in the face array
		 */
		virtual void RemoveFaces(const uint32* faces, const uint32 faceCount) = 0;

		/**
		 * Applies the specified transformation.
		 *
		 * @param transformation the transformation
		 * @return true upon success
		 */
		virtual bool ApplyTransformation(const InstaMatrix3F& transformation) = 0;

		/**
		 * Applies the specified UV transformation.
		 *
		 * @param texcoordIndex the texcoord set to transform
		 * @param scale the scale
		 * @param translate the translation
		 * @return true upon success
		 */
		virtual bool ApplyUVTransformation(const uint32 texcoordIndex, const InstaVec2F& scale, const InstaVec2F& translate) = 0;

		/**
		 * Removes duplicate vertex positions in the input mesh.
		 *
		 * @param comparisonThreshold the maximum distance between two vertices to be considered equal
		 * @return the amount of duplicates removed.
		 */
		virtual uint32 FixDuplicateVertexPositions(float comparisonThreshold) = 0;

		/**
		 * Gets the amount of submeshes stored in this instance.
		 * @note This function finds the max submesh index and returns + 1.
		 * For incorrect meshes this might not necessarily be the amount of
		 * extractable submeshes.
		 *
		 * @return the submesh count
		 */
		virtual uint32 GetSubMeshCount() const = 0;

		/**
		 * Calculates the bounding sphere based on the AABB of this instance.
		 *
		 * @param outBoundingSphereCenter the bounding sphere center (output)
		 * @param outBoundingSphereRadius the bounding sphere radius (output)
		 */
		virtual void CalculateBoundingSphere(InstaVec3F& outBoundingSphereCenter, float& outBoundingSphereRadius) const = 0;

		/**
		 * Calculates the minimum volume bounding sphere of this instance.
		 *
		 * @param outBoundingSphereCenter the bounding sphere center (output)
		 * @param outBoundingSphereRadius the bounding sphere radius (output)
		 * @return trupe upon success
		 */
		virtual bool CalculateMinimumVolumeBoundingSphere(InstaVec3F& outBoundingSphereCenter, float& outBoundingSphereRadius) const = 0;

		/**
		 * Smoothes the existing mesh normals.
		 *
		 * @param hardAngleThreshold if the angle between two faces is below this threshold the normals will be smoothed. 
		 * @return true upon success
		 */
		virtual bool SmoothNormals(const float hardAngleThreshold) = 0;

		/**
		 * Calculates the mesh tangents.
		 * @note This requires valid normals to be available in the mesh.
		 * @note It is recommended to use MikkTSpace instead of this method.
		 *
		 * @param texcoordIndex the index of the texcoord set used for the tangent calculation
		 * @param tangentHandness the handness of the tangents. Set to 0 for automatic detection.
		 * @return true upon success
		 */
		virtual bool CalculateTangentsWithDefault(const uint32 texcoordIndex, const float tangentHandness = 0.0f) = 0;

		/**
		 * Calculates the mesh tangents using MikkTSpace
		 * @note This requires valid normals to be available in the mesh.
		 * @note More information is available here: http://image.diku.dk/projects/media/morten.mikkelsen.08.pdf
		 *
		 * @param texcoordIndex the index of the texcoord set used for the tangent calculation
		 * @param tangentHandness the handness of the tangents. Set to 0 for automatic detection.
		 * @return true upon success
		 */
		virtual bool CalculateTangentsWithMikkTSpace(const uint32 texcoordIndex, const float tangentHandness = 0.0f) = 0;

		/**
		 * Gets the min and max UV values for the specified material index.
		 *
		 * @param materialIndex the material index
		 * @param outMin data storage for the minimum value
		 * @param outMax data storage for the maximum value
		 * @return true upon success
		 */
		virtual bool GetMinMaxUV(const int32 materialIndex, InstaVec2F& outMin, InstaVec2F& outMax) const = 0;

		/**
		 * Gets all material indices used by this instance.
		 *
		 * @param outMaterialIndices (optional) the output array, set to NULL to query the amount of material indices first.
		 * @param materialIndicesBufferSize the maximum size of the outMaterialIndices array.
		 * @return the amount of material indices used.
		 */
		virtual uint32 GetUniqueMaterialIndices(int32* outMaterialIndices, uint32 materialIndicesBufferSize) const = 0;

		/**
		 * Clips the mesh using the specified clipping plane.
		 *
		 * @param plane The clipping plane.
		 * @param epsilon The epsilon.
		 * @param progressCallback (optional) The progress callback.
		 * @return true upon success.
		 */
		virtual bool ClipMesh(const InstaPlaneF& plane, const float epsilon, pfnMeshProgressCallback progressCallback) = 0;

		/**
		 * Removes all unreferenced vertices from the mesh.
		 * @note This method also removes per-vertex data from auxiliary data
		 * such as optimizer weights or skinned mesh data.
		 *
		 * @return true upon success.
		 */
		virtual bool RemoveUnreferencedVertices() = 0;

		/**
		 * Allocates an intersector for this instance.
		 *
		 * @return the intersector instance.
		 */
		virtual IInstaLODMeshIntersector* AllocIntersector() const = 0;

		/**
		 * Deallocates an intersector instance.
		 *
		 * @param intersector the intersector instance.
		 */
		virtual void DeallocIntersector(IInstaLODMeshIntersector* intersector) const = 0;

		/**
		 * Allocates a render mesh for this instance.
		 *
		 * @return the render mesh instance.
		 */
		virtual IInstaLODRenderMesh* AllocRenderMesh() const = 0;

		/**
		 * Deallocates a render mesh instance.
		 *
		 * @param renderMesh the render mesh instance.
		 */
		virtual void DeallocRenderMesh(IInstaLODRenderMesh* renderMesh) const = 0;
	};
} // namespace InstaLOD

#define INSTALODMESHEXTENDED_API_VERSION_MAJOR 6
#define INSTALODMESHEXTENDED_API_VERSION_MINOR 0

#endif
