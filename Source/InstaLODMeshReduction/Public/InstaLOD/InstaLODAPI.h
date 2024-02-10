/**
 * InstaLODAPI.h (InstaLOD)
 *
 * Copyright 2016-2023 InstaLOD GmbH - All Rights Reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * This file and all it's contents are proprietary and confidential.
 *
 * @file InstaLODAPI.h
 * @copyright 2016-2023 InstaLOD GmbH. All rights reserved.
 * @section License
 */

/*!@mainpage notitle
 * @section welcome_sec Welcome to InstaLOD SDK2023
 * We are thrilled that you want to integrate our SDK into your product. If you have any questions or need code level support, please don't
 * hesitate to get in touch with us at hello@InstaLOD.com.
 *
 * Integrating our SDK is a straight forward and fun task. There are no external dependencies on heavyweight runtimes like .NET and
 * great care has been taken to create a modern and clean API with little in your way to creating complex optimizations.
 *
 * InstaLOD SDK for Windows has been using Visual Studio 2019 on Windows 10 and requires
 * the installation of the [Visual C++ Redistributables for Visual Studio 2019 (X64)](http://aka.ms/vs/17/release/vc_redist.x64.exe).
 * If you already have installed Visual Studio 2019 or later on your workstation, the installation of the
 * redistributables is not necessary. More information on the Visual C++ Redistributables can be found in the [Microsoft Documentation](http://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).
 * The InstaLOD SDK is compatible with Windows XP Service Pack 3 and later.
 *
 * InstaLOD SDK for macOS has been built using Xcode 14.1 on macOS Ventura.
 *
 * > The InstaLOD SDK is a 64-bit dynamically linked library that is loaded and linked into your application during runtime.
 * > Statically linked libraries and exotic platform binaries can be provided upon request.
 *
 * @section gettingstarted_sec Getting Started with InstaLOD SDK2023
 *
 * @subsection step1 Step 1: Building the Sample Code
 * Before integrating the InstaLOD SDK into your technology, it is important to ensure that your workstation is setup to run
 * the InstaLOD SDK. The best way to do this is by building and running one of the bundled sample projects.
 *
 * InstaLOD SDK2023 ships with limited sample code. However, a fully documented sample command-line OBJ optimizer is included
 * in the distribution that demonstrates several tasks:
 *
 *   - Initializing the InstaLOD SDK
 *   - Load OBJs from disk
 *   - Convert OBJ data into an IInstaLODMesh instance using vertex arrays, similar to how GPUs handle data.
 *   - Optimize a mesh to 50%
 *   - Mesh operation progress callbacks
 *   - Export an InstaLODMesh as OBJ
 *   - Basic Multithreading
 *   - Tangent and binormal calculation using MikkTSpace
 *   - Improving InstaLODMesh data by removing duplicate vertex positions.
 *   - Basic time profiling using C++ 11
 *
 * Please review the tutorial specific configuration at the top of the `main.cpp` file.
 * The working directory of the solution should be set to the location of the `main.cpp` file or
 * the application will not be able to locate the bundled OBJ files.
 *
 * > If you want to review the capabilities and optimization performance of InstaLOD, check out our
 * > InstaLOD for Autodesk Maya integration or optimize your FBX files directly with InstaLODCmd.
 * > Due to the limitations of the OBJ file format and the OBJ importer the results are not in best-mode.
 *
 *
 * @subsection step2 Step 2: Loading the Library
 * The first thing you have to do is to install our header files in a location that is accessible by your project.
 * Once that is done you can include our C++ header files in your C++ source file.
 * Add the following lines to the top of your C++ source file:
 *
 *     #define INSTALOD_LIB_DYNAMIC
 *     #include "InstaLODAPI.h"
 *     #include "InstaLODMeshExtended.h"
 *
 * Once you have included our header you will be able to load the InstaLOD SDK's dynamic library.
 * > The `INSTALOD_LIB_DYNAMIC` define can also be placed among your project's other pre-processor defines in your solution setup.
 * If your project does not have it's own DLL loading functions you can use the platform agnostic macro the InstaLOD SDK provides.
 * However, for these to work you will have to `#include <windows.h>` on Windows platforms or `#include <dlfcn.h>` on macOS platforms.
 * To load the InstaLOD SDK with the InstaLOD DLL macros, add the following lines to the initialization of your app:
 *
 *     INSTALOD_DLHANDLE dllHandle = INSTALOD_DLOPEN(INSTALOD_LIBRARY_NAME);
 *     InstaLOD::IInstaLOD *instaLOD = nullptr;
 *
 *     if (dllHandle != nullptr) {
 *         fprintf(stdout, "Loaded InstaLOD Library\n");
 *         pfnGetInstaLOD pGetInstaLOD = (pfnGetInstaLOD)INSTALOD_DLSYM(dllHandle, "GetInstaLOD");
 *
 *         if (!pGetInstaLOD(INSTALOD_API_VERSION, &instaLOD)) {
 *             fprintf(stderr, "Failed to get InstaLOD SDK API. Invalid SDK version?\n");
 *             return;
 *         }
 *     }
 *
 *
 * @subsection step3 Step 3: Initializing and Authorizing InstaLOD
 * > If you are using a custom build of the SDK with customized licensing you can skip this step!
 *
 * Before using any public methods of the InstaLOD API you will have to initialize the licensing subsystems.
 * Using the following code, you can initialize your SDK build:
 *
 *     if (!instaLOD->InitializeAuthorization("InstaLOD_SDK", nullptr)) {
 *         fprintf(stderr, "Failed to initialize authorization module:\n%s\n", instaLOD->GetAuthorizationInformation());
 *     }
 *
 * If you have already authorized your workstation, InstaLOD will automatically load the license from the OS shared preferences path.
 * On Windows the shared license path is `C:\ProgramData\InstaLOD\` on macOS the path is `/Users/Shared/InstaLOD/`
 * Otherwise you can authorize your workstation using your license information from within your app:
 *
 *     if (!instaLOD->AuthorizeMachine("username", "password")) {
 *        fprintf(stderr, "Failed to acquire license:\n%s\n", instaLOD->GetAuthorizationInformation());
 *        return;
 *     }
 *     // output current license information to console
 *     fprintf(stdout, "%s\n", instaLOD->GetAuthorizationInformation());
 *     fprintf(stdout, "Is host authorized: %s\n", instaLOD->IsHostAuthorized() ? "YES" : "NO");
 *
 *
 * @subsection step4 Step 4: Converting Mesh Data
 * Before you can optimize or bake your meshes you need to convert your geometry into a `IInstaLODMesh` class.
 * The InstaLODMesh is a wedge-based mesh structure. This means that there is no need to manage duplicate vertex positions due to splits
 * as every wedge has all attributes: Tangent-Basis (Normal, Binormal and Tangent), Colors, TexCoords etc.
 * The InstaLODMesh is easy to work with as faces can be removed or inserted, wedge attributes can be changed without
 * having to worry about splitting vertices.
 * Any kind of mesh buffer - including familiar vertex split-based mesh representations - can be easily converted into
 * an InstaLOD mesh using the `IInstaLODMesh::SetAttributeArray` method. Alternatively, you can directly access the InstaLODMesh buffers.
 *
 * Converting an InstaLODMesh back into a vertex split-based mesh representation can be done using the `IInstaLODRenderMesh` class
 * (see Step 6 for more information).
 *
 * All interaction with InstaLOD is done via the API handle you've allocated in step 2.
 * To allocate an IInstaLODMesh instance, simply invoke `IInstaLOD::AllocMesh`:
 *
 *     InstaLOD::IInstaLODMesh *const mesh = instaLOD->AllocMesh();
 *
 * To deallocate an IInstaLODMesh, simply invoke `IInstaLOD::DeallocMesh`:
 *
 *     instaLOD->DeallocMesh(mesh);
 *
 * To initialize the mesh buffers of your mesh, use the available APIs of the InstaLODMesh class.
 * At a bare minimum your mesh needs to have the following mesh attributes properly setup:
 *
 *   - Front face winding order (Default: counter-clockwise)
 *   - Mesh format type (Default: OpenGL)
 *   - Vertex positions
 *   - Wedge indices (array elements must point to valid elements in the vertex position array)
 *   - Wedge normals (array size must be equal to size of wedge indices array)
 *   - Wedge tangents and binormals (array sizes must be equal to size of wedge indices array)
 *   - Wedge texture coordinate set 0 (array size must be equal to size of wedge indices array)
 *   - Face smoothing groups (array size must be equal to size of wedge indices / 3)
 *   - Face material indices (array size must be wedge indices / 3)
 *
 * > Wedge normals, tangents and binormals can be automatically computed by InstaLOD. To calculate mesh normals,
 * > simply cast your mesh into the `IInstaLODMeshExtended` type and invoke `CalculateNormals`.
 * > To calculate mesh tangents, invoke `CalculateTangentsWithMikkTSpace`. Calculating tangents requires valid texture coordinates to be present.
 * > If your input mesh has no texture coordinates, simply fill the array with `InstaVec2F(0, 0)` and the tangents with `InstaVec3F(1, 0, 0)` and binormals with `InstaVec3F(0, 1, 0)`.
 * > Alternatively, create valid texture coordinates by unwrapping your mesh using the `IUnwrapOperation` class and calculate tangents by invoking `CalculateTangentsWithMikkTSpace`.
 * > If your input mesh data structure does not contain smoothing groups or materials, simply fill the corresponding IInstaLODMesh array with 0.
 *
 * Once you have converted your input mesh into an InstaLODMesh representation you can use the `IInstaLODMesh::IsValid` method to
 * check if the InstaLODMesh is in a healthy state.
 * If valid wedge indices and vertex positions are available a call to `IInstaLODMesh::SanitizeMesh`
 * will either calculate all missing data or fill it with default values to ensure the mesh is considered valid.
 *
 * Skeletal mesh bone weights can be specified via the `IInstaLODSkinnedVertexData` class which can be obtained by
 * invoking `IInstaLODMesh::GetSkinnedVertexData` on your InstaLOD mesh instance.
 * Specifying the skeleton joint hierarchy by constructing an `IInstaLODSkeleton` is only necessary for operations requiring an InstaLOD skeleton instance.
 *
 *
 * @subsection step5 Step 5: Mesh Optimization
 * To perform mesh optimization, simply allocate an instance of the operation you want to perform using the `IInstaLOD` handle you have allocated in step 2.
 * In the following example, a basic polygon optimization is performed. The output mesh will have reduced the input mesh triangle count by 50%.
 *
 *      InstaLOD::OptimizeSettings settings;
 *      settings.PercentTriangles = 0.5f;
 *
 *      InstaLOD::IInstaLODMesh *const outputMesh = instaLOD->AllocMesh();
 *      InstaLOD::IOptimizeOperation *const optimize = instaLOD->AllocOptimizeOperation();
 *
 *      const InstaLOD::OptimizeResult result = optimize->Execute(mesh, outputMesh, settings);
 *      if (result.Success == true)
 *            outputMesh->WriteLightwaveOBJ("Optimized.obj");
 *
 *      instaLOD->DeallocOptimizeOperation(optimize);
 *      instaLOD->DeallocMesh(outputMesh);
 *
 * To reduce memory pressure, most optimization operations can use the same mesh as both input and output mesh.
 * If the input mesh has vertex skinning data setup, the vertex skinning data will automatically be optimized.
 *
 * > Optimizing multiple meshes at once can be done by appending all InstaLOD mesh instances to a single InstaLOD mesh using
 * > `IInstaLODMeshExtended::AppendMesh`. Once the optimization has been completed successfully, simply use `IInstaLODMeshExtended::ExtractSubmesh`
 * > to extract the optimized meshes.
 *
 * The next example performs remeshing on an input mesh with two materials. Input tangent space normal maps will also be transferred to the
 * output mesh.
 *
 *     using namespace InstaLOD;
 *
 *     // setup material data, this is consistent for all material based operations
 *     IInstaLODMaterialData *const materialData = instaLOD->AllocMaterialData();
 *     IInstaLODMaterial *const material0 = materialData->AddMaterialWithID("Material 0", 0);
 *     material0->AddTexturePageFromDisk("Color", "color0.png", IInstaLODTexturePage::TypeColor);
 *     material0->AddTexturePageFromDisk("Normal", "normal0.png", IInstaLODTexturePage::TypeNormalMapTangentSpace);
 *
 *     IInstaLODMaterial *const material1 = materialData->AddMaterialWithID("Material 1", 1);
 *     material1->AddTexturePageFromDisk("Color", "color1.png", IInstaLODTexturePage::TypeColor);
 *     material1->AddTexturePageFromDisk("Roughness", "roughness1.png", IInstaLODTexturePage::TypeMask);
 *     material1->AddTexturePageFromDisk("Normal", "normal1.png", IInstaLODTexturePage::TypeNormalMapTangentSpace);
 *
 *     // configure outputs, this is the same for all material based operations that generate textures
 *     materialData->SetDefaultOutputTextureSize(1024, 1024);
 *     materialData->SetOutputTextureSizeForPage("Color", 512, 512);
 *     materialData->EnableOutputForTexturePage("Color", IInstaLODTexturePage::TypeColor, IInstaLODTexturePage::ComponentTypeUInt16, IInstaLODTexturePage::PixelTypeRGB);
 *     materialData->EnableOutputForTexturePage("Roughness", IInstaLODTexturePage::TypeMask, IInstaLODTexturePage::ComponentTypeUInt8, IInstaLODTexturePage::PixelTypeLuminance);
 *
 *     // setup remeshing with max. 3000 triangles as output
 *     RemeshingSettings settings;
 *     settings.BakeOutput.TexturePageNormalTangentSpace = true;
 *     settings.BakeOutput.TexturePageCustom = true;
 *     settings.MaximumTriangles = 3000;
 *
 *     IRemeshingOperation *const remesh = instaLOD->AllocRemeshingOperation();
 *     remesh->SetMaterialData(materialData);
 *     // add all input meshes
 *     remesh->AddMesh(mesh);
 *
 *     // start the remesh, optionally set a callback prior to Execute()
 *     IInstaLODMesh *const outputMesh = instaLOD->AllocMesh();
 *     const RemeshingResult result = remesh->Execute(outputMesh, settings);
 *     if (result.Success) {
 *         outputMesh->WriteLightwaveOBJ("Remesh.obj");
 *         // retrieve output textures and write to disk
 *         for (int i=0; i<result.BakeMaterial->GetTexturePageCount(); i++) {
 *             IInstaLODTexturePage *const texturePage = result.BakeMaterial->GetTexturePageAtIndex(i);
 *             char filename[256];
 *             snprintf(filename, sizeof(filename), "Remesh_%s.png", texturePage->GetName());
 *             texturePage->WritePNG(filename);
 *         }
 *     }
 *
 *     instaLOD->DeallocRemeshingOperation(remesh);
 *     instaLOD->DeallocMaterialData(materialData);
 *     instaLOD->DeallocMesh(outputMesh);
 *
 * This example will write four files to the current working directory:
 *   - `Remesh.obj`, the output mesh
 *   - `Remesh_Color.png`, the combined texture pages named "Color"
 *   - `Remesh_Roughness.png`, the texture page named "Roughness" - as only a single material references this texture it contains blank parts for those input materials that do not reference the texture page.
 *   - `Remesh_NormalTangentSpace.png`, the output tangent-space normal map with the input mesh normal maps transferred to the remesh surface.
 *
 * By default, custom input texture pages will not be output by operations that generate textures.
 * Texture page output must be enabled on a per-texture page basis by invoking `IInstaLODMaterialData::EnableOutputForTexturePage`.
 * It is possible to specify a different output pixel specification from the input texture pages, and input texture page pixel specifications can be mixed and matched.
 * If no output size for a texture page has been specified using `IInstaLODMaterialData::SetOutputTextureSizeForPage` the default output size as specified by invoking
 * `IInstaLODMaterialData::SetDefaultOutputTextureSize` will be used.
 * Internally generated texture pages can be output by modifying the members of the `BakeOutputSettings` structure.
 * The `IInstaLODTexturePage` features convenience methods to directly write texture page contents to disk. Alternatively, the underlying data arrays can be
 * accessed by invoking `IInstaLODTexturePage::GetData`. The `IInstaLODTexturePage` is a fairly complex image representation, it features resizing and blitting operations as well as
 * sampling methods via pixel and UV coordinates.
 *
 *
 * @subsection step6 Step 6: Creating a renderable IInstaLODMesh representation
 * Converting a regular vertex-split mesh into a wedge-based IInstaLODMesh is a trivial task. However, converting a wedge-based mesh representation back to
 * a vertex-split mesh can be a bit trickier due to the need to determine when to insert vertex splits.
 * InstaLOD SDK2023 ships with a new class that will help you to quickly build renderable mesh representations from your InstaLOD mesh instances.
 * To build a GPU compatible mesh, cast your InstaLOD mesh into the `IInstaLODMeshExtended` type to allocate an `IInstaLODRenderMesh` and construct the buffers.
 *
 *     IInstaLODRenderMesh *const renderMesh = outputMeshExtended->AllocRenderMesh();
 *     InstaLODRenderMeshConstructionSettings constructionSettings;
 *     renderMesh->Construct(constructionSettings);
 *     const InstaLODRenderMeshVertex *vertices = renderMesh->GetVertices(&renderMeshVertexCount);
 *     const uint32 *indices = renderMesh->GetIndices(&renderMeshIndexCount);
 *     // feed vertices and indices into your app's native mesh structure
 *     ...
 *     outputMeshExtended->DeallocRenderMesh(renderMesh);
 *
 * Due to the insertion of vertex splits the indices of the render mesh do not line up with the input InstaLOD mesh instance.
 * However, the InstaLOD render mesh class provides an array that maps InstaLOD mesh wedge indices to InstaLOD render mesh vertex indices.
 * To access the wedge index map array, simply invoke `IInstaLODRenderMesh::GetWedgeIndexToRenderVertexMap`.
 * Skinning data can be obtained by invoking `IInstaLODRenderMesh::GetSkinnedVertexData`.
 *
 *
 * @subsection step7 Step 7: Multithreading
 * InstaLOD supports multi-threading in various ways. Certain operations, like remeshing and baking automatically spawn
 * child threads to accelerate certain computations. The amount of child threads spawned is automatically determined according to the
 * amount of cores available to your operating system.
 * Additionally, all operations can be allocated and executed in parallel on a child thread.
 *
 *
 * @subsection step8 Step 8: Explore InstaLOD
 * You're now ready to explore the vast amount of features present in InstaLOD SDK2023. Once you have implemented the conversion between meshes and materials
 * from your app to InstaLOD you will be able to execute most of the operations available in SDK2023 with just a few lines of code.
 * The InstaLOD SDK API is very consistent and has been designed with the developer in mind.
 *
 * If you have any questions, please don't hesitate to get in touch with us at hello@InstaLOD.com. We can provide code level assistance via eMail and web-meetings.
 */

#ifndef InstaLOD_InstaLODAPI_h
#define InstaLOD_InstaLODAPI_h

#include <stdlib.h>
#include <stdio.h>

/**
 * The project that builds the dylib/dll needs to define the following preprocessor defines
 *		INSTALOD_LIB INSTALOD_LIB_DYNAMIC INSTALOD_LIB_VECTOR
 * The user of the dylib/dll needs to define the following preprocessor defines
 *		INSTALOD_LIB_DYNAMIC
 */

#if defined(__cplusplus)
#	define INSTALOD_EXTERNC extern "C"
#else
#	define INSTALOD_EXTERNC
#endif

#if defined(_WIN32)
#	define INSTALOD_DLL_EXPORT __declspec(dllexport)
#	define INSTALOD_DLL_IMPORT __declspec(dllimport)
#elif defined(__APPLE__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#	define INSTALOD_DLL_EXPORT __attribute__((visibility("default")))
#	define INSTALOD_DLL_IMPORT
#endif

#if defined(INSTALOD_LIB_DYNAMIC)
#	if defined(INSTALOD_LIB)
#		define INSTALOD_API INSTALOD_EXTERNC INSTALOD_DLL_EXPORT
#	else
#		define INSTALOD_API INSTALOD_EXTERNC INSTALOD_DLL_IMPORT
#	endif
#elif defined(INSTALOD_LIB_STATIC)
#	if defined(INSTALOD_LIB)
#		define INSTALOD_API INSTALOD_EXTERNC INSTALOD_DLL_EXPORT
#	else
#		define INSTALOD_API INSTALOD_EXTERNC
#	endif
#else
#	define INSTALOD_API
#endif

#if defined(INSTALOD_LIB_VECTOR)
#	include "InstaLODMath.h"
#endif

namespace InstaLOD
{
	typedef char int8;
	typedef unsigned char uint8;
	typedef short int16;
	typedef unsigned short uint16;
	typedef int int32;
	typedef unsigned int uint32;
	typedef long long int64;
	typedef unsigned long long uint64;

	enum
	{
		INSTALOD_MAX_MESH_TEXCOORDS = 8,   /**< Maximum amount of texture coordinate sets per IInstaLODMesh instance. */
		INSTALOD_MAX_MESH_COLORSETS = 4,   /**< Maximum amount of wedge color sets per IInstaLODMesh instance */
		INSTALOD_BONE_INDEX_INVALID = ~0u, /**< Set when influence is not used. */
		INSTALOD_JOINT_INDEX_INVALID = ~0u /**< Invalid joint index. */
	};

#if !defined(INSTALOD_LIB_VECTOR)
	struct InstaVec2F
	{
		inline InstaVec2F() {}
		inline InstaVec2F(float x, float y) :
		X(x), Y(y) {}
		float X, Y;
	};
	struct InstaVec2D
	{
		inline InstaVec2D() {}
		inline InstaVec2D(double x, double y) :
		X(x), Y(y) {}
		double X, Y;
	};
	struct InstaVec3F
	{
		inline InstaVec3F() {}
		inline InstaVec3F(float x, float y, float z) :
		X(x), Y(y), Z(z) {}
		float X, Y, Z;
	};
	struct InstaVec3D
	{
		inline InstaVec3D() {}
		inline InstaVec3D(double x, double y, double z) :
		X(x), Y(y), Z(z) {}
		double X, Y, Z;
	};
	struct InstaVec4F
	{
		inline InstaVec4F() {}
		inline InstaVec4F(float x, float y, float z, float w) :
		X(x), Y(y), Z(z), W(w) {}
		float X, Y, Z, W;
	};
	struct InstaVec4D
	{
		inline InstaVec4D() {}
		inline InstaVec4D(double x, double y, double z, double w) :
		X(x), Y(y), Z(z), W(w) {}
		double X, Y, Z, W;
	};
	struct InstaQuaternionF
	{
		inline InstaQuaternionF() {}
		inline InstaQuaternionF(float x, float y, float z, float w) :
		X(x), Y(y), Z(z), W(w) {}
		float X, Y, Z, W;
	};
	struct InstaQuaternionD
	{
		inline InstaQuaternionD() {}
		inline InstaQuaternionD(double x, double y, double z, double w) :
		X(x), Y(y), Z(z), W(w) {}
		double X, Y, Z, W;
	};
	struct InstaMatrix3F
	{
		float M[9]; /**< A 3x3 matrix in column-major order, identical to the way OpenGL stores matrices. */
	};
	struct InstaMatrix3D
	{
		double M[9]; /**< A 3x3 matrix in column-major order, identical to the way OpenGL stores matrices. */
	};
	struct InstaMatrix4F
	{
		float M[16]; /**< A 4x4 matrix in column-major order, identical to the way OpenGL stores matrices. */
	};
	struct InstaMatrix4D
	{
		double M[16]; /**< A 4x4 matrix in column-major order, identical to the way OpenGL stores matrices. */
	};
	struct InstaPlaneF
	{
		inline InstaPlaneF() {}
		inline InstaPlaneF(const InstaVec3F& normal, float distance) :
		Normal(normal), Distance(distance) {}
		InstaVec3F Normal;
		float Distance;
	};
	struct InstaColorRGBAF32
	{
		inline InstaColorRGBAF32() {}
		inline InstaColorRGBAF32(float r, float g, float b, float a) :
		R(r), G(g), B(b), A(a) {}
		float R, G, B, A;
	};
	struct InstaBoundingBox3F
	{
	public:
		inline InstaBoundingBox3F() {}
		inline InstaBoundingBox3F(const InstaVec3F& mins, const InstaVec3F& maxs) :
		Mins(mins), Maxs(maxs) {}
		InstaVec3F Mins;
		InstaVec3F Maxs;
	};
#endif
	typedef int32 InstaMaterialID;

	/**
	 * The MeshFeatureImportance enumeration represents feature importance settings.
	 */
	namespace MeshFeatureImportance
	{
		enum Type
		{
			Off = 0,
			Lowest = 1,
			Low = 2,
			Normal = 3,
			High = 4,
			Highest = 5,
			Count = 6
		};
	}

	/**
	 * The FeatureImportance enumeration represents feature importance settings.
	 */
	namespace FeatureImportance
	{
		enum Type
		{
			Off = 0,
			Lowest = 1,
			Low = 2,
			Normal = 3,
			High = 4,
			Highest = 5,
			Count = 6
		};
	}

	/**
	 * The MeshFormat enumeration specifies the graphics API that will consume this mesh instance.
	 */
	namespace MeshFormat
	{
		enum Type
		{
			DirectX = 0, /**< UV(0,0) at top-left. DirectX (LH) tangent space. */
			OpenGL = 1,	 /**< UV(0,0) at bottom-left. OpenGL (RH) tangent space (Default). */
			Count = 2
		};
	}

	/**
	 * The SuperSampling enumeration specifies various types of super sampling.
	 */
	namespace SuperSampling
	{
		enum Type
		{
			None = 0, /**< No super sampling. */
			X2 = 1,	  /**< 2x super sampling. */
			X4 = 2,	  /**< 4x super sampling. */
			X8 = 3,	  /**< 8x super sampling. */
			X16 = 4,  /**< 16x super sampling. */
			Maximum = X16,
			Count = 5
		};
	}

	/**
	 * The TextureFilter enumeration specifies various types of texture filtering.
	 */
	namespace TextureFilter
	{
		enum Type
		{
			Nearest = 0,  /**< Nearest neighbor filtering. */
			Bilinear = 1, /**< Bilinear filtering. */
			Bicubic = 2,  /**< Bicubic filtering. */
			Count = 3
		};
	}

	/**
	 * The InstaLODSkeletalMeshBoneData structure represents a bone influence associated to a vertex.
	 */
	struct InstaLODSkeletalMeshBoneData
	{
		InstaLODSkeletalMeshBoneData() :
		BoneIndex(uint32(INSTALOD_BONE_INDEX_INVALID)),
		BoneInfluence(0.0f)
		{
		}

		uint32 BoneIndex;	 /**< Bone ID */
		float BoneInfluence; /**< Influence of bone with ID=BoneIndex in range [0...1] */
	};

	/**
	 * The InstaLODSkinnedVertexData is used to configure per-vertex bone influences.
	 */
	class IInstaLODSkinnedVertexData
	{
	protected:
		virtual ~IInstaLODSkinnedVertexData() {}

	public:
		/**
		 * Initializes this instance.
		 *
		 * @param vertexCount the vertex count
		 * @param maximumInfluencePerVertex the maximum bone influences per vertex
		 */
		virtual void Initialize(const uint32 vertexCount, const uint32 maximumInfluencePerVertex) = 0;

		/**
		 * Determines if this instance has been initialized.
		 *
		 * @return true if this instance has been initialized
		 */
		virtual bool IsInitialized() const = 0;

		/**
		 * Gets the vertex count
		 *
		 * @return the vertex count
		 */
		virtual uint32 GetVertexCount() const = 0;

		/**
		 * Gets the maximum influences per vertex
		 *
		 * @return the maximum influences per vertex
		 */
		virtual uint32 GetMaximumInfluencesPerVertex() const = 0;

		/**
		 * Gets the bone data array and optionally writes the current size to the specified pointer.
		 * The amount of elements of the bone data array is VertexCount * MaximumInfluencesPerVertex
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the bone data array
		 */
		virtual InstaLODSkeletalMeshBoneData* GetBoneData(uint64* count) = 0;

		/**
		 * Gets the bone data array and optionally writes the current size to the specified pointer.
		 * The amount of elements of the bone data array is VertexCount * MaximumInfluencesPerVertex
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the bone data array
		 */
		virtual const InstaLODSkeletalMeshBoneData* GetBoneData(uint64* count) const = 0;

		/**
		 * Gets the bone data array for the specified vertex index.
		 * The amount of elements of the bone data array is MaximumInfluencesPerVertex.
		 *
		 * @param index the vertex index
		 * @return the bone data array
		 */
		virtual InstaLODSkeletalMeshBoneData* GetBoneDataForVertex(const uint32 index) = 0;

		/**
		 * Gets the bone data array for the specified vertex index.
		 * The amount of elements of the bone data array is MaximumInfluencesPerVertex.
		 *
		 * @param index the vertex index
		 * @return the bone data array
		 */
		virtual const InstaLODSkeletalMeshBoneData* GetBoneDataForVertex(const uint32 index) const = 0;

		/**
		 * Sets the bone data array for the specified vertex index.
		 * The amount of elements of the bone data array is MaximumInfluencesPerVertex.
		 *
		 * @param boneData the bone data array, the amount of elements must be equal to MaximumInfluencesPerVertex.
		 * @param index the vertex index
		 */
		virtual void SetBoneDataForVertex(const InstaLODSkeletalMeshBoneData* boneData, const uint32 index) = 0;
	};

	/**
	 * The IInstaLODMeshBase represents an abstract mesh in the InstaLODSDK.
	 * The IInstaLODMesh, a triangle based mesh and IInstaLODPolygonMesh a flexible n-wedge polygon capable mesh.
	 */
	class IInstaLODMeshBase
	{
	protected:
		virtual ~IInstaLODMeshBase() {}

	public:
		/**
		 * The MeshType enumeration specifies mesh types.
		 */
		enum MeshType
		{
			MeshTypeTriangle = 0, /**< The mesh is of type IInstaLODMesh. */
			MeshTypePolygon = 1,  /**< The mesh if of type IInstaLODPolygonMesh. */
			MeshTypeInvalid = ~0
		};

		/**
		 * The AttributeType enumeration specifies mesh attributes.
		 */
		enum AttributeType
		{
			AttributeTypeVertexPositions = 0,		 /**< Size must be 3 */
			AttributeTypeVertexOptimizerWeights = 1, /**< Size must be 1 */
			AttributeTypeFaceMaterialIndices = 2,	 /**< Size must be 1 */
			AttributeTypeFaceSmoothingGroups = 3,	 /**< Size must be 1 */
			AttributeTypeFaceSubMeshIndices = 4,	 /**< Size must be 1 */
			AttributeTypeWedgeIndices = 5,			 /**< Size must be 1 */
			AttributeTypeWedgeNormals = 6,			 /**< Size must be 3 */
			AttributeTypeWedgeBinormals = 7,		 /**< Size must be 3 */
			AttributeTypeWedgeTangents = 8,			 /**< Size must be 3 */
			AttributeTypeWedgeColors0 = 9,			 /**< Size must be 4 (RGBA) */
			AttributeTypeWedgeColorsMax = AttributeTypeWedgeColors0 + INSTALOD_MAX_MESH_COLORSETS,
			AttributeTypeWedgeTexCoords0 = AttributeTypeWedgeColorsMax, /**< Size must be 2 */
			AttributeTypeWedgeTexCoordsMax = AttributeTypeWedgeTexCoords0 + INSTALOD_MAX_MESH_TEXCOORDS
		};

		/**
		 * The WindingOrder enumeration specifies the winding order of mesh polygons.
		 */
		enum WindingOrder
		{
			WindingOrderClockwise = 0,		 /**< Clockwise polygon winding order. */
			WindingOrderCounterClockwise = 1 /**< Counter-clockwise polygon winding order (Default). */
		};

		/**
		 * Gets the mesh type.
		 */
		virtual MeshType GetMeshType() const = 0;

		/**
		 * Gets the winding order of front-facing polygons.
		 *
		 * @return the winding order.
		 */
		virtual WindingOrder GetFrontFaceWindingOrder() const = 0;

		/**
		 * Gets the mesh format type of this instance.
		 *
		 * @return the mesh format type.
		 */
		virtual MeshFormat::Type GetMeshFormatType() const = 0;

		/**
		 * Determines if this instance meets minimum mesh requirements and checks the integrity
		 * of mesh attribute arrays.
		 *
		 * @note This method determines whether all required mesh data arrays are valid.
		 * The integrity of the WedgeIndices array elements will also be verified.
		 * Minimum mesh requirements are:
		 *   - Vertex Positions
		 *   - Wedge Indices
		 *   - Normals, Binormals and Tangents
		 *   - Texture Coordinate Set 0
		 *   - Face material indices
		 *   - Face smoothing groups
		 *
		 * @note If additional mesh arrays contain data, they will be verified to be in a valid state.
		 * The size of vertex attribute arrays must match the size of the vertex position array.
		 * The size of wedge attribute arrays must match the size of the wedge indices array.
		 * The size of face attribute arrays must match the size of the wedge indices array divided by 3.
		 *
		 * @note If the mesh does not meet the minimum mesh requirements and vertex positions
		 * and wedge indices are available SantizeMesh() can be invoked to compute the missing information.
		 *
		 * @return false if the mesh data is invalid
		 */
		virtual bool IsValid() const = 0;

		/**
		 * Determines if this instance is empty.
		 * @note A mesh is considered empty if it contains neither vertex positions and wedge indices.
		 *
		 * @return true if this instance is empty
		 */
		virtual bool IsEmpty() const = 0;

		/**
		 * Modifies this instance to make it pass the validity test.
		 *
		 * @note This method requires that at least vertex positions and indices have been specified and are in a valid state.
		 * If texture coordinate set 0 is invalid, it will be filled with default values.
		 * Normals will automatically be computed if not already available.
		 * Tangents and Binormals will be set to default values if not already available and texture coordinates set 0 is invalid.
		 * If texture coordinates set 0 is valid. Tangents and binormals will be computed using MikkTSpace.
		 * Face material and smoothing groups will be set to default values if not already available.
		 *
		 * @note If data arrays do not match expected sizes they will be resized and filled with default values if necessary.
		 * If float arrays contain NAN or infinite elements these invalid elements will be set to 0.
		 *
		 * @return true if the mesh was sanitized or false if the mesh does not meet minimum requirements
		 */
		virtual bool SanitizeMesh() = 0;

		/**
		 * Clears all data buffers of the mesh.
		 */
		virtual void Clear() = 0;

		/**
		 * Writes a LightWave OBJ representation of this instance to the
		 * specified file path.
		 *
		 * @param path the output path for the OBJ file
		 * @return true if the file has been written.
		 */
		virtual bool WriteLightwaveOBJ(const char* path) const = 0;

		/**
		 * Serializes this instance with an optional material data instance.
		 *
		 * @param path the output path for the InstaLODMesh file.
		 * @param materialData (optional) material data to serialize.
		 * @return true if the file has been written.
		 */
		virtual bool Serialize(const char* path, const class IInstaLODMaterialData* materialData) const = 0;

		/**
		 * Gets the user pointer
		 *
		 * @return the user pointer or nullptr if not set
		 */
		virtual void* GetUserPointer() const = 0;

		/**
		 * Sets the user pointer
		 * @remarks The user pointer will not be retained by the mesh.
		 * The caller of this function is responsible for allocating and
		 * deallocating the data of the user pointer.
		 *
		 * @param userPointer the user pointer
		 */
		virtual void SetUserPointer(void* userPointer) = 0;

		/**
		 * Calculates the mesh normals.
		 *
		 * @param hardAngleThreshold if the angle between two faces is below this threshold the normals will be smoothed. Set to 0 for face normals.
		 * @param weighted set to true to enable weighted normals when calculating smooth normals.
		 * @return true upon success
		 */
		virtual bool CalculateNormals(const float hardAngleThreshold, const bool weighted) = 0;

		/**
		 * Appends the specified mesh to this instance.
		 * @note All data will be appended, including skinned vertex data.
		 * Face submesh indices will also be adjusted an appended accordingly.
		 * @note The \p mesh MeshType must match this instace.
		 *
		 * @param mesh the mesh to append.
		 * @return true upon success.
		 */
		virtual bool AppendMesh(const IInstaLODMeshBase* mesh) = 0;

		/**
		 * Removes the specified mesh from this instance.
		 * @note Face data as well as unused vertex data will be removed.
		 * This includes the removal of auxiliary data such as skinned vertex data.
		 * Other submesh IDs will not be modified.
		 *
		 * @param submeshID the mesh to remove.
		 * @return true upon success.
		 */
		virtual bool RemoveSubMesh(const uint32 submeshID) = 0;

		/**
		 * Extracts the specified submesh and stores it in the specified output mesh.
		 * @note All data will be extracted, including skinned vertex data.
		 * @note The \p outputMesh MeshType must match this instance.
		 *
		 * @param submeshID The submeshID to extract.
		 * @param [out] outputMesh The output mesh buffer.
		 * @return true upon success.
		 */
		virtual bool ExtractSubMesh(const uint32 submeshID, IInstaLODMeshBase* outputMesh) const = 0;

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
	};

	/**
	 * The IInstaLODMesh interface represents a triangle mesh.
	 * The InstaLOD SDK also supports a n-polygon mesh through the IInstaLODPolygonMesh interface.
	 */
	class IInstaLODMesh : public IInstaLODMeshBase
	{
	protected:
		virtual ~IInstaLODMesh() {}

	public:
		/**
		 * The ScalarType enumeration specifies scalar types.
		 */
		enum ScalarType
		{
			ScalarTypeInt8 = 0,	   /**< A scalar is a signed 8-bit integer. */
			ScalarTypeInt16 = 1,   /**< A scalar is a signed 16-bit integer. */
			ScalarTypeInt32 = 2,   /**< A scalar is a signed 32-bit integer. */
			ScalarTypeUInt8 = 3,   /**< A scalar is an unsigned 8-bit integer. */
			ScalarTypeUInt16 = 4,  /**< A scalar is an unsigned 16-bit integer. */
			ScalarTypeUInt32 = 5,  /**< A scalar is an unsigned 32-bit integer. */
			ScalarTypeFloat32 = 6, /**< A scalar is a 32-bit float. */
			ScalarTypeDouble64 = 7 /**< A scalar is a 64-bit double. */
		};

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaVec3F* GetVertexPositions(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaVec3F* GetVertexPositions(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note vertex optimizer weights are optional
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual float* GetVertexOptimizerWeights(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note vertex optimizer weights are optional
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const float* GetVertexOptimizerWeights(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual uint32* GetWedgeIndices(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const uint32* GetWedgeIndices(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaVec3F* GetWedgeNormals(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaVec3F* GetWedgeNormals(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaVec3F* GetWedgeBinormals(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaVec3F* GetWedgeBinormals(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaVec3F* GetWedgeTangents(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaVec3F* GetWedgeTangents(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaMaterialID* GetFaceMaterialIndices(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaMaterialID* GetFaceMaterialIndices(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual uint32* GetFaceSmoothingGroups(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const uint32* GetFaceSmoothingGroups(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note sub mesh indices are optional
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual uint32* GetFaceSubMeshIndices(uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note sub mesh indices are optional
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const uint32* GetFaceSubMeshIndices(uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note only wedge texcoords 0 are required
		 *
		 * @param index the texture coordinate set index
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaVec2F* GetWedgeTexCoords(const uint64 index, uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note only wedge texcoords 0 are required
		 *
		 * @param index the texture coordinate set index
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaVec2F* GetWedgeTexCoords(const uint64 index, uint64* count) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param index the wedge colors set index
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual InstaColorRGBAF32* GetWedgeColors(const uint64 index, uint64* count) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param index the wedge colors set index
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the mesh data array
		 */
		virtual const InstaColorRGBAF32* GetWedgeColors(const uint64 index, uint64* count) const = 0;

		/**
		 * Resizes the data array to match the specified size.
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeVertexPositions(const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 * @note the size must be either 0 or match the size of the VertexPositions array
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeVertexOptimizerWeights(const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeWedgeIndices(const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 * @note the size must match the size of the WedgeIndices array
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeWedgeTangents(const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 * @note the size must match the size of the WedgeIndices array
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeWedgeBinormals(const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 * @note the size must match the size of the WedgeIndices array
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeWedgeNormals(const uint64 size) = 0;

		/**
		 * Resizes the data array at the specified index to match the specified size.
		 * @note the size must be either 0 or match the size of the WedgeIndices array.
		 *
		 * @param index the texcoord index
		 * @param size the new size of the data array
		 */
		virtual void ResizeWedgeTexCoords(const uint64 index, const uint64 size) = 0;

		/**
		 * Resizes the data array at the specified index to match the specified size.
		 * @note the size must be either 0 or match the size of the WedgeIndices array.
		 *
		 * @param index the texcoord index
		 * @param size the new size of the data array
		 */
		virtual void ResizeWedgeColors(const uint64 index, const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 * @note the size must match the size of the WedgeIndices array /3
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeFaceSubMeshIndices(const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 * @note the size must match the size of the WedgeIndices array /3
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeFaceMaterialIndices(const uint64 size) = 0;

		/**
		 * Resizes the data array to match the specified size.
		 * @note the size must match the size of the WedgeIndices array /3
		 *
		 * @param size the new size of the data array
		 */
		virtual void ResizeFaceSmoothingGroups(const uint64 size) = 0;

		/**
		 * Convenience method to convert color data to optimizer weights.
		 * If no color data is available this method does nothing
		 *
		 * @param index the index of the color set
		 * @return true if color data was converted.
		 */
		virtual bool ConvertColorDataToOptimizerWeights(const uint64 index) = 0;

		/**
		 * Populates the specified mesh array using vertex array pointers similar to the GPU data layout.
		 * @note Before filling wedge attributes vertex position and wedge indices need to be populated.
		 * If stride is 0, the stride in bytes will automatically be calculated based on the scalar type and the size.
		 * See the IInstaLODMesh::AttributeType documentation for size specifications.
		 * Texture coordinate sets greater than 0 can be specified by adding the texture set index to IInstaLODMesh::AttributeTypeWedgeTexCoords0
		 * e.g. in order to set Texture coordinate set at index 2, the following attribute needs to be specified "IInstaLODMesh::AttributeTypeWedgeTexCoords0+2"
		 * It is recommended to invoke IInstaLODMesh::IsValid after setting up the mesh vertex buffers to ensure the mesh is in a valid state.
		 *
		 * @param attribute the mesh attribute to populate. @note Vertex positions and wedge indices need to be populated first.
		 * @param data the input data pointer
		 * @param size the amount of elements based per input attribute entry @note See the IInstaLODMesh::AttributeType documentation for size specifications
		 * @param type the input data scalar type @note InstaLOD supports automatic type conversions, however normalization is not supported.
		 * @param stride the stride in bytes of the input data buffer. @note For densely packed arrays this is 0 or "sizeof(type)*size", for sparse arrays this is typically sizeof(YourVertexType)
		 * @param count the amount entries in the array. @note Vertex attributes expect the vertex count. Face attributes expect the face count.
		 *		  Wedge attributes except wedge indices expect the vertex count as indirection via wedge indices is used to access data in the input data buffer. Wedge indices expect the index count.
		 * @return true upon success
		 */
		virtual bool SetAttributeArray(const AttributeType attribute, const void* data, const uint32 size, const ScalarType type, const uint32 stride, const uint32 count) = 0;

		/**
		 * Reverses the face directions.
		 * @note This method reverses the order of face and wedge attribute arrays
		 * to flip the winding order of faces.
		 *
		 * @param flipNormals Set to true to flip normals.
		 * @return true upon success
		 */
		virtual bool ReverseFaceDirections(const bool flipNormals) = 0;
	};

	struct InstaLODPolygonMeshPolygon;
	struct InstaLODPolygonMeshWedge;
	class IInstaLODPolygonMeshLayer;

	/**
	 * The IInstaLODPolygonMesh represents an n-polygon wedge-based mesh structure.
	 */
	class IInstaLODPolygonMesh : public IInstaLODMeshBase
	{
	protected:
		virtual ~IInstaLODPolygonMesh() {}

	public:
		using Real = double;
#if !defined(INSTALOD_LIB_VECTOR)
		using Vector2 = InstaVec2D;
		using Vector3 = InstaVec3D;
		using Vector4 = InstaVec4D;
#else
		using Vector2 = InstaLODVector2<Real>;
		using Vector3 = InstaLODVector3<Real>;
		using Vector4 = InstaLODVector4<Real>;
#endif

		/**
		 * The DataType enumeration specifies fundamental data types.
		 */
		enum DataType
		{
			DataTypeInt8 = 0,	   /**< A signed 8-bit integer. */
			DataTypeInt16 = 1,	   /**< A signed 16-bit integer. */
			DataTypeInt32 = 2,	   /**< A signed 32-bit integer. */
			DataTypeUInt8 = 3,	   /**< An unsigned 8-bit integer. */
			DataTypeUInt16 = 4,	   /**< An unsigned 16-bit integer. */
			DataTypeUInt32 = 5,	   /**< An unsigned 32-bit integer. */
			DataTypeFloat32 = 6,   /**< A 32-bit float. */
			DataTypeDouble64 = 7,  /**< A 64-bit double. */
			DataTypeVector2F = 8,  /**< A 32-bit float 2D vector. */
			DataTypeVector3F = 9,  /**< A 32-bit float 3D vector. */
			DataTypeVector2D = 10, /**< A 64-bit float 2D vector. */
			DataTypeVector3D = 11, /**< A 64-bit float 3D vector. */
			DataTypeString = 12,   /**< A variable length string.*/
			DataTypePolygon = 13,  /**< A polygon.*/
			DataTypeWedge = 14,	   /**< A wedge.*/
			DataTypeColor = 15,	   /**< A 32-bit float RGBA color.*/
			DataTypeInvalid = ~0
		};

		/**
		 * The ComponentType enumeration specifies mesh component types.
		 */
		enum ComponentType
		{
			ComponentTypePolygon = 0,
			ComponentTypeVertex = 1,
			ComponentTypeWedge = 2,
			ComponentTypeWedgeEdge = 3,
			ComponentTypeVertexEdge = 4,
			ComponentTypeInvalid = ~0
		};

		/**
		 * Gets the vertex optimizer weights sparse mesh attribute layer.
		 *
		 * @return The mesh data array.
		 */
		virtual IInstaLODPolygonMeshLayer& GetVertexPositions() = 0;

		/**
		 * Gets the vertex optimizer weights sparse mesh attribute layer.
		 *
		 * @return The mesh data array.
		 */
		virtual const IInstaLODPolygonMeshLayer& GetVertexPositions() const = 0;

		/**
		 * Gets the vertex optimizer weights sparse mesh attribute layer.
		 *
		 * @return The mesh data array.
		 */
		virtual IInstaLODPolygonMeshLayer& GetVertexOptimizerWeights() = 0;

		/**
		 * Gets the vertex optimizer weights sparse mesh attribute layer.
		 *
		 * @return The mesh data array.
		 */
		virtual const IInstaLODPolygonMeshLayer& GetVertexOptimizerWeights() const = 0;

		/**
		 * Gets the dense mesh attribute layer.
		 *
		 * @return The mesh attribute layer.
		 */
		virtual IInstaLODPolygonMeshLayer& GetPolygons() = 0;

		/**
		 * Gets the dense mesh attribute layer.
		 *
		 * @return The mesh attribute layer.
		 */
		virtual const IInstaLODPolygonMeshLayer& GetPolygons() const = 0;

		/**
		 * Gets the dense mesh attribute layer.
		 *
		 * @return The mesh attribute layer.
		 */
		virtual IInstaLODPolygonMeshLayer& GetWedges() = 0;

		/**
		 * Gets the dense mesh attribute layer.
		 *
		 * @return The mesh attribute layer.
		 */
		virtual const IInstaLODPolygonMeshLayer& GetWedges() const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note only wedge texcoords 0 are required
		 *
		 * @param index the texture coordinate set index
		 * @return The mesh data array.
		 */
		virtual IInstaLODPolygonMeshLayer& GetWedgeTexCoords(const uint64 index) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 * @note only wedge texcoords 0 are required
		 *
		 * @param index the texture coordinate set index.
		 * @return The mesh data array.
		 */
		virtual const IInstaLODPolygonMeshLayer& GetWedgeTexCoords(const uint64 index) const = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param index the wedge colors set index.
		 * @return The mesh data array.
		 */
		virtual IInstaLODPolygonMeshLayer& GetWedgeColors(const uint64 index) = 0;

		/**
		 * Gets the mesh data array and optionally writes the current size to the specified pointer.
		 *
		 * @param index the wedge colors set index.
		 * @return The mesh data array.
		 */
		virtual const IInstaLODPolygonMeshLayer& GetWedgeColors(const uint64 index) const = 0;

		/**
		 * Convenience method to convert color data to optimizer weights.
		 * If no color data is available this method does nothing
		 *
		 * @param index the index of the color set
		 * @return true if color data was converted.
		 */
		virtual bool ConvertColorDataToOptimizerWeights(const uint64 index) = 0;

		/**
		 * Copies the data from the specified mesh \p rhs.
		 * @note This method will preserve the UserPointer if set.
		 *
		 * @param rhs The mesh to copy data.
		 */
		virtual void CopyDataFromMesh(const IInstaLODPolygonMesh& rhs) = 0;

		/**
		 * Copies the data from the specified triangle mesh \p rhs.
		 * @note This method will preserve the UserPointer if set.
		 * @note This method will return false if \p rhs is invalid.
		 *
		 * @param rhs The mesh to copy data.
		 * @return true upon success
		 */
		virtual bool CopyDataFromTriangleMesh(const IInstaLODMesh& rhs) = 0;

		/**
		 * Populates the specified mesh array using vertex array pointers similar to the GPU data layout.
		 * @note Before filling wedge attributes vertex position and wedge indices need to be populated.
		 * If stride is 0, the stride in bytes will automatically be calculated based on the scalar type and the size.
		 * See the IInstaLODMesh::AttributeType documentation for size specifications.
		 * Texture coordinate sets greater than 0 can be specified by adding the texture set index to IInstaLODMesh::AttributeTypeWedgeTexCoords0
		 * e.g. in order to set Texture coordinate set at index 2, the following attribute needs to be specified "IInstaLODMesh::AttributeTypeWedgeTexCoords0+2"
		 * It is recommended to invoke IInstaLODMesh::IsValid after setting up the mesh vertex buffers to ensure the mesh is in a valid state.
		 *
		 * @param attribute the mesh attribute to populate. @note Vertex positions and wedge indices need to be populated first.
		 * @param data the input data pointer
		 * @param size the amount of elements based per input attribute entry @note See the IInstaLODMesh::AttributeType documentation for size specifications
		 * @param type the input data scalar type @note InstaLOD supports automatic type conversions. However, normalization is not supported.
		 * @param stride the stride in bytes of the input data buffer. @note For densely packed arrays this is 0 or "sizeof(type)*size", for sparse arrays this is typically sizeof(YourVertexType)
		 * @param count the amount entries in the array. @note Vertex attributes expect the vertex count. Face attributes expect the face count.
		 *		  Wedge attributes except wedge indices expect the vertex count as indirection via wedge indices is used to access data in the input data buffer. Wedge indices expect the index count.
		 * @return true upon success
		 */
		virtual bool SetAttributeArray(const IInstaLODMesh::AttributeType attribute, const void* data, const uint32 size, const DataType type, const uint32 stride, const uint32 count) = 0;

		/**
		 * Appens the specified \p polygon.
		 * @note The wedge data must be manually added to this instance.
		 *
		 * @param polygon The polygon.
		 */
		virtual void AppendPolygon(const InstaLODPolygonMeshPolygon& polygon) = 0;

		/**
		 * Appens the specified \p polygon and the \p wedge data.
		 * @note The WedgeOffset for the \p polygon will be automatically
		 * set. However, the WedgeCount value of the \p polygon must match the size of \p wedges.
		 *
		 * @param polygon The polygon.
		 * @param wedges The wedge data.
		 */
		virtual void AppendPolygon(const InstaLODPolygonMeshPolygon& polygon, const InstaLODPolygonMeshWedge* wedges) = 0;

		/**
		 * Removes duplicate vertex positions in the input mesh.
		 *
		 * @param comparisonThreshold the maximum distance between two vertices to be considered equal
		 * @return the amount of duplicates removed.
		 */
		virtual uint32 FixDuplicateVertexPositions(const double comparisonThreshold) = 0;

		/**
		 * Triangulates the mesh and stores the result in \p outMesh.
		 *
		 * @param [out] outMesh The output mesh buffer for the tringulation.
		 * @return true upon success
		 */
		virtual bool TriangulateMesh(IInstaLODMesh* outMesh) const = 0;
	};

	/**
	 * The InstaLODPolygonMeshPolygon structure represents a polygon with an arbitrary number of corners (wedges).
	 */
	struct InstaLODPolygonMeshPolygon
	{
		InstaLODPolygonMeshPolygon() :
		InstaLODPolygonMeshPolygon(0u, 0u) {}
		InstaLODPolygonMeshPolygon(const uint32 wedgeIndex, const uint32 wedgeCount) :
		WedgeIndex(wedgeIndex),
		WedgeCount(wedgeCount),
		SubmeshIndex(0),
		SmoothingGroups(0),
		MaterialIndex(0)
		{
		}

		uint32 WedgeIndex;			   /**< The offset into the mesh's Wedges array.*/
		uint32 WedgeCount;			   /**< The number of wedges in this polygon in counter-clockwise order. */
		uint32 SubmeshIndex;		   /**< The submesh index. */
		uint32 SmoothingGroups;		   /**< The bit-wise smoothing groups. */
		InstaMaterialID MaterialIndex; /**< The material index. */
	};

	/**
	 * The InstaLODPolygonMeshWedge structure represents an attribute polygon wedge.
	 * Additional attributes can be stored on the polygon mesh through the IInstaLODPolygonMeshLayer mechanism.
	 */
	struct InstaLODPolygonMeshWedge
	{
		using Vector3 = IInstaLODPolygonMesh::Vector3;

		InstaLODPolygonMeshWedge() :
		InstaLODPolygonMeshWedge(~0u) {}
		explicit InstaLODPolygonMeshWedge(const uint32 vertexIndex) :
		VertexIndex(vertexIndex), Normal(0, 0, 1), Binormal(0, 1, 0), Tangent(1, 0, 0) {}

		uint32 VertexIndex; /**< The vertex index in the parent mesh VertexPositions array.*/
		Vector3 Normal;		/**< Wedge nornal. */
		Vector3 Binormal;	/**< Wedge binormal. */
		Vector3 Tangent;	/**< Wedge tangent. */
	};

	/**
	 * The IInstaLODPolygonMeshLayer class represents a per mesh-component attribute layer.
	 * Mesh layers can be either dense or sparse. The raw data of dense layers can be retrieved by a
	 * call to GetData(). The data of sparse layers can only be accessed through the component-wise
	 * getters and setters such as GetDataForComponentAtIndex or SetDataForComponentAtIndex.
	 */
	class IInstaLODPolygonMeshLayer
	{
	protected:
		virtual ~IInstaLODPolygonMeshLayer() {}

	public:
		/**
		 * The MeshType enumeration specifies mesh types.
		 */
		enum LayerType
		{
			LayerTypeDense = 0,	 /**< The layer is dense. */
			LayerTypeSparse = 1, /**< The layer is sparse. */
			MeshTypeInvalid = ~0
		};

		/**
		 * Gets the name of this instance.
		 *
		 * @return The name.
		 */
		virtual const char* GetName() const = 0;

		/**
		 * Sets the name of this instance.
		 *
		 * @param name The name.
		 */
		virtual void SetName(const char* name) = 0;

		/**
		 * Gets the layer type of this instance.
		 *
		 * @return The layer type.
		 */
		virtual LayerType GetLayerType() const = 0;

		/**
		 * Gets the data type of this instance.
		 *
		 * @return The data type.
		 */
		virtual IInstaLODPolygonMesh::DataType GetDataType() const = 0;

		/**
		 * Gets the component type of this instance.
		 *
		 * @return The component type.
		 */
		virtual IInstaLODPolygonMesh::ComponentType GetComponentType() const = 0;

		/**
		 * Gets the data.
		 * @note Sparse mesh attribute layers will return nullptr as the underlying
		 * data can only be accessed through component-wise setters and getters.
		 *
		 * @return The data.
		 */
		virtual void* GetData() = 0;

		/**
		 * Gets the data.
		 * @note Sparse mesh attribute layers will return nullptr as the underlying
		 * data can only be accessed through component-wise setters and getters.
		 *
		 * @return The data.
		 */
		virtual const void* GetData() const = 0;

		/**
		 * Gets the size of this instance.
		 *
		 * @return The size.
		 */
		virtual uint64 GetSize() const = 0;

		/**
		 * Gets the size of an element in this instance.
		 *
		 * @return The size.
		 */
		virtual uint64 GetElementSize() const = 0;

		/**
		 * Determines if this instance contains data.
		 *
		 * @return True if empty.
		 */
		virtual bool IsEmpty() const = 0;

		/**
		 * Clears all data.
		 *
		 * @return True upon succes.
		 */
		virtual bool Clear() = 0;

		/**
		 * Reallocates the buffers of this instance.
		 * @note This method is only available for dense mesh attribute layers.
		 * Other layer types will return false.
		 *
		 * @param size The new size.
		 * @param defaultData (optional) The default data element for newly added entries.
		 * @param defaultDataSize The size of  \p defaultData in bytes.
		 * @return true upon success.
		 */
		virtual bool Reallocate(const uint64 size, const void* defaultData, uint64 defaultDataSize) = 0;

		/**
		 * Gets the raw data pointer for the component data at \p index.
		 * @note Sparse layers will return nullptr if the value at \p index does not exist.
		 *
		 * @param index The component index.
		 * @return The component data.
		 */
		virtual const void* GetDataForComponentAtIndex(const uint64 index) const = 0;

		/**
		 * Sets the data for the component data at \p index.
		 * @note Sparse layers support setting \p data to nullptr to erase
		 * the value for the specified \p index.
		 *
		 * @param index The component index.
		 * @param data The component data.
		 * @param dataSize The size of \p data in bytes.
		 * @return True upon success.
		 */
		virtual bool SetDataForComponentAtIndex(const uint64 index, const void* data, uint64 dataSize) = 0;

		/**
		 * Gets the value for the component at \p index.
		 *
		 * @param index The component index.
		 * @param [out] value The component value.
		 * @return true upon success.
		 */
		template<typename T> bool GetValue(const uint64 index, T& value) const
		{
			if (const void* data = GetDataForComponentAtIndex(index))
			{
				value = *static_cast<const T*>(data);
				return true;
			}
			return false;
		}

		/**
		 * Sets the value for the component at \p index.
		 *
		 * @param index The component index.
		 * @param value The component value.
		 * @return true upon success.
		 */
		template<typename T> bool SetValue(const uint64 index, const T& value)
		{
			return SetDataForComponentAtIndex(index, &value, sizeof(T));
		}

		/**
		 * Gets the data.
		 * @note Sparse mesh attribute layers will return nullptr as the underlying
		 * data can only be accessed through component-wise setters and getters.
		 *
		 * @return The data.
		 */
		template<typename T> T* GetDataAs(const IInstaLODPolygonMesh::DataType dataType)
		{
			if (sizeof(T) == GetElementSize() && dataType == GetDataType())
			{
				return static_cast<T*>(GetData());
			}
			return nullptr;
		}

		/**
		 * Gets the data.
		 * @note Sparse mesh attribute layers will return nullptr as the underlying
		 * data can only be accessed through component-wise setters and getters.
		 *
		 * @return The data.
		 */
		template<typename T> const T* GetDataAs(const IInstaLODPolygonMesh::DataType dataType) const
		{
			if (sizeof(T) == GetElementSize() && dataType == GetDataType())
			{
				return static_cast<const T*>(GetData());
			}
			return nullptr;
		}
	};

	/**
	 * The IInstaLODSkeleton interface represents a hierarchically structured joint based skeleton structure.
	 * @note Building the IInstaLODSkeleton data is not necessary when optimizing a skinned mesh. It is only required when the actual
	 * skeleton needs to be optimized, by removing or collapsing joints.
	 * @remarks specifying per joint transform data is not necessary when leaf bone welding is unused. When transform data is not available, specify (0,0,0) as scale.
	 * However, the parent-child relations and joint indices need to precisely match the data specified in the corresponding IInstaLODSkinnedVertexData.
	 */
	class IInstaLODSkeleton
	{
	protected:
		virtual ~IInstaLODSkeleton() {}

	public:
		/**
		 * Specifies whether joint transforms stored in this instance are specified in worldspace or in the parent joint's local space (default).
		 * @note By default, joint transforms are specified in the parent joint's local space.
		 *
		 * @param isWorldSpace true if joint transforms are specified in worldspace
		 */
		virtual void SetJointTransformsInWorldSpace(const bool isWorldSpace) = 0;

		/**
		 * Adds a named joint to the skeleton.
		 * @note By default, joint transforms are specified in the parent joint's local space.
		 *
		 * @param parentIndex the parent joint index, specify INSTALOD_JOINT_INDEX_INVALID for root joints.
		 * @param jointIndex the index of the joint to be added
		 * @param position the position of the joint
		 * @param orientation the orientation of the joint
		 * @param scale the scale of the joint, set to (0,0,0) when transform data is not available.
		 * @param name (optional) name of the joint
		 * @param userPointer (optional) a pointer that can be used to store joint data
		 * @return the index of newly added joint, or INSTALOD_JOINT_INDEX_INVALID if the specified data was invalid
		 */
		virtual uint32 AddJoint(const uint32 parentIndex, const uint32 jointIndex, const InstaVec3F& position, const InstaQuaternionF& orientation, const InstaVec3F& scale, const char* name, void* userPointer) = 0;

		/**
		 * Removes the joint with the specified index and all it's children.
		 *
		 * @param index the index of the joint
		 * @return true if the joint index is valid and it has been removed.
		 */
		virtual bool RemoveJointWithIndex(const uint32 index) = 0;

		/**
		 * Determines if a joint with the specified index has been added to this instance.
		 *
		 * @return true if a joint with the specified index has been added to this instance.
		 */
		virtual bool ContainsJointWithIndex(const uint32 index) const = 0;

		/**
		 * Gets the amount of joints stored in this instance.
		 *
		 * @return the amount of joints stored in this instance.
		 */
		virtual uint32 GetJointCount() const = 0;

		/**
		 * Writes all joint indices to the specified array.
		 * @note pass a nullptr array or dataSize to query the amount of joints.
		 *
		 * @param outData output data array (set to nullptr to query the joint count)
		 * @param dataSize the size of the data array (pass 0 to query the joint count)
		 * @return the total amount of joints
		 */
		virtual uint32 GetJointIndices(uint32* outData, const uint32 dataSize) const = 0;

		/**
		 * Gets the information of the joint with the specified index.
		 * @remarks In general, it should not be necessary to retrieve previously added joint information from a skeleton
		 * as skeleton operations do not modify the actual joint data.
		 *
		 * @param index the index of the joint
		 * @param outPosition data storage for joint position
		 * @param outOrientation data storage for joint orientation
		 * @param outScale data storage for joint scale
		 * @param outName data storage for joint name
		 * @param outUserPointer data storage for joint user pointer
		 * @return true if the joint index is valid
		 */
		virtual bool GetJointWithIndex(const uint32 index, InstaVec3F& outPosition, InstaQuaternionF& outOrientation, InstaVec3F& outScale, const char** outName, void** outUserPointer) const = 0;

		/**
		 * Gets the amount of direct child joints for the specified joint index.
		 * @note This method should be used to get the amount of children available for a joint.
		 * The child joints can then be iterated using GetChildJointAtIndex.
		 *
		 * @param index the index of the joint
		 * @return the amount of joints that are direct children of the specified joint.
		 */
		virtual uint32 GetChildJointCount(const uint32 index) const = 0;

		/**
		 * Gets an indexed child joint of the specified joint index.
		 *
		 * @param parentIndex the index of the joint
		 * @param childArrayIndex the array index of the child joint. Invoke GetChildJointCount() to get the amount of child joints for a specified joint index.
		 * @param outPosition data storage for child joint position
		 * @param outOrientation data storage for child joint orientation
		 * @param outScale data storage for child joint scale
		 * @param outName data storage for child joint name
		 * @param outUserPointer data storage for child joint user pointer
		 * @return true if the joint index and the child array index are valid
		 */
		virtual bool GetChildJointAtIndex(const uint32 parentIndex, const uint32 childArrayIndex, InstaVec3F& outPosition, InstaQuaternionF& outOrientation, InstaVec3F& outScale, const char** outName, void** outUserPointer) const = 0;

		/**
		 * Returns the index of the first joint found with the specified user pointer.
		 *
		 * @param userPointer the user point
		 * @return the index of the joint, or INSTALOD_JOINT_INDEX_INVALID if the specified user pointer was not found
		 */
		virtual uint32 GetJointIndexForUserPointer(const void* userPointer) const = 0;

		/**
		 * Gets the user pointer of the joint with the specified index.
		 *
		 * @param index the index of the joint
		 * @param outUserPointer data storage for joint user pointer
		 * @return true if the joint index is valid
		 */
		virtual bool GetJointUserPointer(const uint32 index, const void** outUserPointer) const = 0;
	};

	/**
	 * The InstaLODTexturePageWrapMode enumeration specifies different wrap modes available to UV transforms.
	 */
	namespace InstaLODTexturePageWrapMode
	{
		enum Type
		{
			Repeat, /**< Repeated (tiling) texture sampling. */
			Clamp	/**< Fragments will be clamped to the texture border when sampling outside of [0...1]. */
		};
	}

	/**
	 * The InstaLODTexturePageUVTransform structure represents a transformation applied to UV coordinates.
	 * The order of transformations applied is identical to Autodesk Maya's place2dTexture node:
	 * Rotate then scale and finally translate.
	 */
	struct InstaLODTexturePageUVTransform
	{
		InstaLODTexturePageUVTransform() :
		WrapModeU(InstaLODTexturePageWrapMode::Repeat),
		WrapModeV(InstaLODTexturePageWrapMode::Repeat),
		SwapUV(false),
		Translate(0.0f, 0.0f),
		RotateDegrees(0.0f),
		RotatePivot(0.0f, 0.0f),
		Scale(1.0f, 1.0f),
		ScalePivot(0.0f, 0.0f)
		{
		}

		InstaLODTexturePageWrapMode::Type WrapModeU; /**< Wrap mode along U axis. */
		InstaLODTexturePageWrapMode::Type WrapModeV; /**< Wrap mode along V axis. */
		bool SwapUV;								 /**< Determines whether UV axes will be flipped. */
		InstaVec2F Translate;						 /**< UV translation. */
		float RotateDegrees;						 /**< UV rotation. */
		InstaVec2F RotatePivot;						 /**< UV rotation pivot. */
		InstaVec2F Scale;							 /**< UV scale. */
		InstaVec2F ScalePivot;						 /**< UV scale pivot. */
	};

	/**
	 * The DelightSettings structure contains settings related to the delighting of texture pages.
	 */
	struct DelightSettings
	{
		DelightSettings() :
		ShadowKernel(10.0f),
		Strength(1.0f),
		HighlightRemoval(FeatureImportance::Normal),
		ShadowRemoval(FeatureImportance::Normal),
		Falloff(FeatureImportance::Normal)
		{
		}

		float ShadowKernel;						  /**< The kernel size in pixels when detecting shadow or highlight areas. */
		float Strength;							  /**< The overall strength of the delighting. */
		FeatureImportance::Type HighlightRemoval; /**< The highlight removal strength. */
		FeatureImportance::Type ShadowRemoval;	  /**< The shadow removal strength. */
		FeatureImportance::Type Falloff;		  /**< The falloff size for highlight/shadow detection. */
	};

	/**
	 * The IInstaLODTexturePage represents a texture page as referenced by a material.
	 * A material can reference multiple texture pages of different types and pixel specifications.
	 */
	class IInstaLODTexturePage
	{
	protected:
		virtual ~IInstaLODTexturePage() {}

	public:
		class SharedHandle;

		/**
		 * The Type enumeration specifies the content of the texture page.
		 */
		enum Type
		{
			TypeColor = 0,				   /**< Default color texture page. */
			TypeNormalMapTangentSpace = 1, /**< Normal map texture page. Only a single instance of this type is allowed per material. Can be sampled from source mesh during baking. */
			TypeNormalMapObjectSpace = 2,  /**< Normal map texture page with object space oriented normals.*/
			TypeMask = 3,				   /**< Color map with each component used independently as mask. */
			TypeOpacity = 4,			   /**< Greyscale opacity map. */
			TypeAmbientOcclusion = 5,	   /**< Ambient occlusion map. */
			TypeDisplacementMap = 6,	   /**< Displacement map, 0.5 as mid-point. */
			TypeThicknessMap = 7,		   /**< Thickness map, black are thin parts, white are thick parts. */
			TypePositionMap = 8,		   /**< Position map, normalized against bounding sphere. */
			TypeCurvatureMap = 9,		   /**< Curvature map, 0.0 concave, 0.5 flat, 1.0 convex. */
			TypeTransferMap = 10,		   /**< Transfer map to map target to source UV space. */
			TypeMeshID = 11,			   /**< Unique color per submesh. */
			TypeRoughness = 12,			   /**< Roughness map. */
			TypeSpecular = 13,			   /**< Specular map. */
			TypeMetalness = 14,			   /**< Metalness map. */
			TypeEmissive = 15,			   /**< Emissive map. */
			TypeBentNormals = 16,		   /**< Bent normals map. */
			TypeBarycentric = 17,		   /**< Barycentric coordinate map. */
			TypeSubsurfaceScattering = 18, /**< Subsurface scattering map. */
			TypeSheen = 19,				   /**< Sheen map. */
			TypeReflectance = 20,		   /**< Reflectance map. This map can be used to cancel specularity to prevent light leakage. */
			TypeCount = 21
		};

		/**
		 * The ComponentType specifies the scalar type of a pixel component.
		 */
		enum ComponentType
		{
			ComponentTypeUInt8 = 0,	 /**< A pixel component is an unsigned 8-bit integer. */
			ComponentTypeUInt16 = 1, /**< A pixel component is an unsigned 16-bit integer. */
			ComponentTypeFloat32 = 2 /**< A pixel component is a 32-bit float. */
		};

		/**
		 * The PixelType enumeration specifies the component semantic and amount of components.
		 */
		enum PixelType
		{
			PixelTypeLuminance = 0,		 /**< Single channel. */
			PixelTypeLuminanceAlpha = 1, /**< Two channels. */
			PixelTypeRGB = 2,			 /**< Three channels. */
			PixelTypeRGBA = 3			 /**< Four channels. */
		};

		/**
		 * The CompressionType enumeration specifies different types of image compression.
		 */
		enum CompressionType
		{
			CompressionTypeNone = 0,	/**< No compression. */
			CompressionTypeDeflate = 1, /**< Deflate compression. */
			CompressionTypeLZW = 2,		/**< LZW compression. */
			CompressionTypeJPEG = 3		/**< JPEG compression. */
		};

		/**
		 * Gets the bits per component.
		 *
		 * @return bits per component.
		 */
		virtual uint32 GetBitsPerComponent() const = 0;

		/**
		 * Gets the bytes per component.
		 *
		 * @return bytes per component.
		 */
		virtual uint32 GetBytesPerComponent() const = 0;

		/**
		 * Gets the bytes per pixel.
		 *
		 * @return bytes per pixel.
		 */
		virtual uint32 GetBytesPerPixel() const = 0;

		/**
		 * Gets the component count per pixel.
		 *
		 * @return component count per pixel.
		 */
		virtual uint32 GetComponentCount() const = 0;

		/**
		 * Gets the component type.
		 *
		 * @return component type.
		 */
		virtual ComponentType GetComponentType() const = 0;

		/**
		 * Gets the pixel type.
		 *
		 * @return pixel type
		 */
		virtual PixelType GetPixelType() const = 0;

		/**
		 * Gets the type.
		 *
		 * @return texture type.
		 */
		virtual IInstaLODTexturePage::Type GetType() const = 0;

		/**
		 * Determines if this instance has an alpha channel.
		 *
		 * @return true if this instance has an alpha channel.
		 */
		virtual bool HasAlphaChannel() const = 0;

		/**
		 * Determines if this instance is valid.
		 *
		 * @return true if this instance is valid.
		 */
		virtual bool IsValid() const = 0;

		/**
		 * Gets the name of this instance.
		 *
		 * @return the name of this instance.
		 */
		virtual const char* GetName() const = 0;

		/**
		 * Gets the data array of this instance.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the data array of this instance
		 */
		virtual uint8* GetData(uint64* count) = 0;

		/**
		 * Gets the data array of this instance.
		 *
		 * @param count (optional) the size of the data array will be written to this pointer
		 * @return the data array of this instance
		 */
		virtual const uint8* GetData(uint64* count) const = 0;

		/**
		 * Reallocates the data array of this instance to the specified dimensions.
		 *
		 * @param width the width
		 * @param height the height
		 */
		virtual void Reallocate(const uint32 width, const uint32 height) = 0;

		/**
		 * Writes the dimensions of this instance to the specified pointers
		 *
		 * @param outWidth (optional) the width will be written to this pointer
		 * @param outHeight (optional) the height will be written to this pointer
		 */
		virtual void GetSize(uint32* outWidth, uint32* outHeight) const = 0;

		/**
		 * Gets the width of this instance.
		 *
		 * @return the width of this instance.
		 */
		virtual uint32 GetWidth() const = 0;

		/**
		 * Gets the height of this instance.
		 *
		 * @return the height of this instance.
		 */
		virtual uint32 GetHeight() const = 0;

		/**
		 * Determines whether this instance has been internally computed.
		 * @note This method returns true for all maps computed
		 * by InstaLOD. Custom texture pages will always return false.
		 *
		 * @return true if internal
		 */
		virtual bool IsInternal() const = 0;

		/**
		 * Sets the specified pixel value at the specified position.
		 * @note If the component type is not float it will be converted and unused channels will be ignored.
		 *  - RGBA = all channels used
		 *  - RGB = alpha channel undefined
		 *  - LA = green and blue undefined, red = luminance
		 *  - L = green, blue and alpha undefined, red = luminance
		 * @param x the x coordinate
		 * @param y the y coordinate
		 * @param value the color value
		 */
		virtual void SetPixelFloat(const uint32 x, const uint32 y, const InstaColorRGBAF32& value) = 0;

		/**
		 * Samples the pixel value and returns the result as float color.
		 * @note If the component type is not float it will be converted and unused channels will be ignored.
		 *  - RGBA = all channels used
		 *  - RGB = alpha channel undefined
		 *  - LA = green and blue undefined, red = luminance
		 *  - L = green, blue and alpha undefined, red = luminance
		 * @param x the x coordinate
		 * @param y the y coordinate
		 * @return the color value as float
		 */
		virtual InstaColorRGBAF32 SampleFloat(const uint32 x, const uint32 y) const = 0;

		/**
		 * Samples the pixel value with nearest filtering and returns the result as float color.
		 * @note If the component type is not float it will be converted and unused channels will be ignored.
		 *  - RGBA = all channels used
		 *  - RGB = alpha channel undefined
		 *  - LA = green and blue undefined, red = luminance
		 *  - L = green, blue and alpha undefined, red = luminance
		 * @param uv the uv coordinate
		 * @param isOpenGL true if sampling with OpenGL UV coordinates
		 * @return the color value as float
		 */
		virtual InstaColorRGBAF32 SampleNearestInterpolatedFloat(const InstaVec2F& uv, const bool isOpenGL) const = 0;

		/**
		 * Samples the pixel value with bilinear(2x2) filtering and returns the result as float color.
		 * @note If the component type is not float it will be converted and unused channels will be ignored.
		 *  - RGBA = all channels used
		 *  - RGB = alpha channel undefined
		 *  - LA = green and blue undefined, red = luminance
		 *  - L = green, blue and alpha undefined, red = luminance
		 * @param uv the uv coordinate
		 * @param isOpenGL true if sampling with OpenGL UV coordinates
		 * @return the color value as float
		 */
		virtual InstaColorRGBAF32 SampleBilinearInterpolatedFloat(const InstaVec2F& uv, const bool isOpenGL) const = 0;

		/**
		 * Samples the pixel value with bicubic(4x4) filtering and returns the result as float color.
		 * @note This method has a potentially high processing cost due to 16 texture reads per invocation.
		 * @note If the component type is not float it will be converted and unused channels will be ignored.
		 *  - RGBA = all channels used
		 *  - RGB = alpha channel undefined
		 *  - LA = green and blue undefined, red = luminance
		 *  - L = green, blue and alpha undefined, red = luminance
		 * @param uv the uv coordinate
		 * @param isOpenGL true if sampling with OpenGL UV coordinates
		 * @return the color value as float
		 */
		virtual InstaColorRGBAF32 SampleBicubicInterpolatedFloat(const InstaVec2F& uv, const bool isOpenGL) const = 0;

		/**
		 * Samples the pixel value with specified filtering and returns the result as float color.
		 * @note If the component type is not float it will be converted and unused channels will be ignored.
		 *  - RGBA = all channels used
		 *  - RGB = alpha channel undefined
		 *  - LA = green and blue undefined, red = luminance
		 *  - L = green, blue and alpha undefined, red = luminance
		 * @param uv the uv coordinate
		 * @param isOpenGL true if sampling with OpenGL UV coordinates
		 * @return the color value as float
		 */
		virtual InstaColorRGBAF32 SampleInterpolatedFloat(const TextureFilter::Type filter, const InstaVec2F& uv, const bool isOpenGL) const = 0;

		/**
		 * Sets the specified UV transform of this instance.
		 *
		 * @param transform the UV transform.
		 */
		virtual void SetUVTransform(const InstaLODTexturePageUVTransform& transform) = 0;

		/**
		 * Gets the current UV transform of this instance.
		 *
		 * @return the UV transform.
		 */
		virtual InstaLODTexturePageUVTransform GetInstaLODTexturePageUVTransform() const = 0;

		/**
		 * Fills the specified channel of this texture page with the
		 * channel of the specified value.
		 * @note the color will be converted to match the pixel specification.
		 *
		 * @param index the channel index
		 * @param value the value
		 */
		virtual void FillChannel(const uint32 index, const InstaColorRGBAF32& value) = 0;

		/**
		 * Fills all channels of this texture page with the specified color.
		 * @note the color will be converted to match the pixel specification.
		 *
		 * @param value fill value
		 */
		virtual void Fill(const InstaColorRGBAF32& value) = 0;

		/**
		 * Sets all bytes of the data storage to 0.
		 */
		virtual void Clear() = 0;

		/**
		 * Solidifies this instance.
		 * @note This method is only available for texture pages with an alpha channel.
		 *
		 * @return true if this instance was solidified
		 */
		virtual bool Solidify() = 0;

		/**
		 * Writes the contents of this image to disk as PNG.
		 * @note If the output bits per pixel component value is lower than the bits per pixel component of
		 * the texture page, the resulting PNG can be dithered during downsampling to
		 * improve the image quality by avoiding banding by enabling \p allowDithering.
		 *
		 * @param path the output path for the image file
		 * @param bitsPerComponent bits per pixel component of the output image, must be either 8 or 16.
		 * @param allowDithering enables dithering when downsampling on export
		 * @return true if successful
		 */
		virtual bool WritePNG(const char* path, const uint32 bitsPerComponent, const bool allowDithering) const = 0;

		/**
		 * Writes the contents of this image to disk as BMP.
		 * @note If the output bits (8) per pixel component value is lower than the bits per pixel component of
		 * the texture page, the resulting BMP can be dithered during downsampling to
		 * improve the image quality by avoiding banding by enabling \p allowDithering.
		 *
		 * @param path the output path for the image file
		 * @param allowDithering enables dithering when downsampling on export
		 * @return true if successful
		 */
		virtual bool WriteBMP(const char* path, const bool allowDithering) const = 0;

		/**
		 * Writes the contents of this image to disk as TGA.
		 * @note If the output bits (8) per pixel component value is lower than the bits per pixel component of
		 * the texture page, the resulting TGA can be dithered during downsampling to
		 * improve the image quality by avoiding banding by enabling allowDithering.
		 *
		 * @param path the output path for the image file
		 * @param allowDithering enables dithering when downsampling on export
		 * @return true if successful
		 */
		virtual bool WriteTGA(const char* path, const bool allowDithering) const = 0;

		/**
		 * Writes the contents of this image to disk as 8-bit JPEG.
		 *
		 * @param path the output path for the image file
		 * @param quality the JPG quality [0...100]
		 * @param allowDithering enables dithering when downsampling on export
		 * @return true if successful
		 */
		virtual bool WriteJPEG(const char* path, int quality, bool allowDithering) const = 0;

		/**
		 * Writes the contents of this image to disk as 32-bit linear HDR.
		 *
		 * @param path the output path for the image file
		 * @return true if successful
		 */
		virtual bool WriteHDR(const char* path) const = 0;

		/**
		 * Writes the contents of this image to disk as 16 or 32-bit linear EXR.
		 *
		 * @param path the output path for the image file
		 * @param bitsPerComponent bits per pixel component of the output image, must be either 16 (half-float) or 32 (float)
		 * @return true if successful
		 */
		virtual bool WriteEXR(const char* path, const uint32 bitsPerComponent) const = 0;

		/**
		 * Writes the contents of a texture page to disk as 8bit, 16bit or 32-bit TIFF.
		 * Additionally the TIFF can be compressed.
		 *
		 * @note With texture pages with pixel types IInstaLODTexturePage::PixelTypeRGBA and IInstaLODTexturePage::PixelTypeLuminanceAlpha,
		 * setting isAlphaPremultiplied to true will result in image data written and subsequently read as-is, setting isAlphaPremultiplied
		 * to false may lead to libTIFF later transforming the image data read from the file and filling the texture page with the modified data.
		 *
		 * @note When compression type is CompressionTypeJPEG, \p outputComponentType must be set to IInstaLODTexturePage::ComponentTypeUInt8,
		 * any other component type will result in false returned and no file created on disk.
		 *
		 * @param path the output path for the image file
		 * @param outputComponentType the output component type
		 * @param compressionType the compression type to use
		 * @param isAlphaPremultiplied sets alpha as premultiplied in the resulting file
		 * @param allowDithering enables dithering when downsampling on export
		 * @return true if successful
		 */
		virtual bool WriteTIFF(const char* const path, const IInstaLODTexturePage::ComponentType outputComponentType, const CompressionType compressionType, const bool isAlphaPremultiplied, const bool allowDithering) const = 0;

		/**
		 * Blits the specified texture page into the specified region of this instance.
		 * @note both texture pages must be of the same pixel specification
		 *
		 * @param page the texture page to blit into this instance
		 * @param x the x coordinate
		 * @param y the y coordinate
		 * @param width the width
		 * @param height the height
		 * @return true if successful
		 */
		virtual bool BlitIntoRegion(IInstaLODTexturePage* page, const uint32 x, const uint32 y, const uint32 width, const uint32 height) = 0;

		/**
		 * Resizes this instance to the specified size using bilinear filtering.
		 *
		 * @param width the width
		 * @param height the height
		 */
		virtual void ResizeBilinear(const uint32 width, const uint32 height) = 0;

		/**
		 * Dithers this instance based on the specified output component type.
		 * @note This method is useful when downsampling 16bit content for use in 8bit
		 * applications.
		 * @note The WritePNG() method can dither the output automatically when saving.
		 * @note This method does not change the underlying component type.
		 *
		 * @param componentType the output component type, must be less precise than the component type of this instance-
		 * @return true if successful
		 */
		virtual bool Dither(const IInstaLODTexturePage::ComponentType componentType) = 0;

		/**
		 * Delights this instance using the specified settings.
		 *
		 * @param settings the input settings.
		 * @return true if successful.
		 */
		virtual bool Delight(const DelightSettings& settings) = 0;

		/**
		 * Swaps the underyling data buffers between both instance.
		 * @note This method only works if the dimensions and pixel types are identical matches.
		 * This method is useful if a front/backbuffer iteration is used.
		 *
		 * @param rhs The other texture page.
		 * @return true upon success.
		 */
		virtual bool Swap(IInstaLODTexturePage& rhs) = 0;
	};

	typedef InstaColorRGBAF32 (*pfnMaterialShadingCallback)(const class IInstaLODMaterialData* materialData, const class IInstaLODMaterial* material, const char* outputTexturePageName,
															const uint32 x, const uint32 y, const uint32 width, const uint32 height,
															const class IInstaLODMesh* mesh, const uint32 faceIndex, const InstaVec3F& barycentric, const InstaVec2F& uv);

	/**
	 * The IInstaLODMaterial interface represents a renderable material.
	 * An IInstaLODMaterial is made up of several IInstaLODTexturePages of varying pixel specifications.
	 * Texture pages can be added manually by adding it using AddTexturePage() and feeding the data into the texture page.
	 * Alternatively, InstaLOD can load texture from disk by using AddTexturePageFromDisk().
	 * Supported image file formats are:
	 *	PNG 8bit, 16bit  (RGB, RGBA, GREYSCALE, GREYSCALE ALPHA)
	 *  JPG, BMP, TGA, 32bit HDR
	 *  PSD (composited view only, no extra channels, 8/16 bit-per-channel)
	 *  TIFF
	 *  EXR 16bit, 32bit
	 */
	class IInstaLODMaterial
	{
	protected:
		virtual ~IInstaLODMaterial() {}

	public:
		class SharedHandle;

		/**
		 * Gets the ID of this instance.
		 *
		 * @return the material ID of this instance
		 */
		virtual InstaMaterialID GetID() const = 0;

		/**
		 * Gets the name of this instance
		 *
		 * @return the name of this instance.
		 */
		virtual const char* GetName() const = 0;

		/**
		 * Sets the material shading callback.
		 * The material shading callback is invoked whenever an operation
		 * transfers material data from a source mesh to a target mesh.
		 * Typical examples are remeshing, baking, imposterize and material merge operations.
		 * @note Material shading callbacks can be called from child threads.
		 *
		 * @param callback the shading callback
		 */
		virtual void SetShadingCallback(pfnMaterialShadingCallback callback) = 0;

		/**
		 * Adds a texture page to this instance.
		 * @note The ownership of the \p page is transferred to this instance.
		 *
		 * @param page the texture page
		 * @return the texture page or nullptr if the texture page was not added
		 */
		virtual IInstaLODTexturePage* AddTexturePage(IInstaLODTexturePage* page) = 0;

		/**
		 * Adds a texture page with the specified name and type to this instance.
		 *
		 * @param name the name of the new texture page
		 * @param type the type of the new texture page
		 * @param componentType the component type of the new texture page
		 * @param pixelType the pixel type of the new texture page
		 * @return the new texture page or nullptr if the texture page was not added
		 */
		virtual IInstaLODTexturePage* AddTexturePage(const char* name, const IInstaLODTexturePage::Type type, const IInstaLODTexturePage::ComponentType componentType, const IInstaLODTexturePage::PixelType pixelType) = 0;

		/**
		 * Loads texture page data from the specified file location and adds it to this instance.
		 * @note InstaLOD will best-guess the component and pixel type.
		 *
		 * @param name the name of the new texture page
		 * @param filename the filename of the texture data
		 * @param type the type of the new texture page
		 * @return the new texture page or nullptr if the image could not be loaded
		 */
		virtual IInstaLODTexturePage* AddTexturePageFromDisk(const char* name, const char* filename, const IInstaLODTexturePage::Type type) = 0;

		/**
		 * Adds a texture page using a shared handle.
		 *
		 * @param handle the texture page shared handle
		 * @return the texture page or nullptr if the texture page was not added
		 */
		virtual IInstaLODTexturePage* AddTexturePage(IInstaLODTexturePage::SharedHandle* handle) = 0;

		/**
		 * Gets the texture page with the specified name to this instance.
		 *
		 * @param name the name of the texture page
		 * @return the texture page or nullptr if the texture page is not found
		 */
		virtual IInstaLODTexturePage* GetTexturePage(const char* name) = 0;

		/**
		 * Gets the texture page with the specified name to this instance.
		 *
		 * @param name the name of the texture page
		 * @return the texture page or nullptr if the texture page is not found
		 */
		virtual const IInstaLODTexturePage* GetTexturePage(const char* name) const = 0;

		/**
		 * Allocates a texture page handle for the texture page with the specified name.
		 * @note To avoid memory leaks, invoke DeallocTexturePageHandle once the handle is not
		 * required anymore. This is typically after passing the handle to another IInstaLODMaterial instance.
		 *
		 * @param name the name of the texture page
		 * @return the shared texture page handle or nullptr if the texture page is not found
		 */
		virtual IInstaLODTexturePage::SharedHandle* AllocTexturePageSharedHandle(const char* name) = 0;

		/**
		 * Gets the texture page at the specified index.
		 * @remarks Texture page indices may change when new pages are added.
		 *
		 * @param index the texture page index
		 * @return the texture page at the specified index
		 */
		virtual IInstaLODTexturePage* GetTexturePageAtIndex(const uint32 index) = 0;

		/**
		 * Gets the texture page at the specified index.
		 * @remarks Texture page indices may change when new pages are added.
		 *
		 * @param index the texture page index
		 * @return the texture page at the specified index
		 */
		virtual const IInstaLODTexturePage* GetTexturePageAtIndex(const uint32 index) const = 0;

		/**
		 * Allocates a texture page handle for the texture page at the specified index.
		 * @note To avoid memory leaks, invoke DeallocTexturePageHandle once the handle is not
		 * required anymore. This is typically after passing the handle to another IInstaLODMaterial instance.
		 *
		 * @param index the texture page index
		 * @return the shared texture page handle or nullptr if the texture page is not found
		 */
		virtual IInstaLODTexturePage::SharedHandle* AllocTexturePageSharedHandleAtIndex(const uint32 index) = 0;

		/**
		 * Dellocates a texture page handle.
		 * @note To avoid memory leaks, invoke DeallocTexturePageHandle once the handle is not
		 * required anymore. This is typically after passing the handle to another IInstaLODMaterial instance.
		 *
		 * @param handle the texture handle
		 */
		virtual void DeallocTexturePageSharedHandle(IInstaLODTexturePage::SharedHandle* handle) = 0;

		/**
		 * Gets the amount of texture pages stored in this instance.
		 *
		 * @return the texture page count
		 */
		virtual uint32 GetTexturePageCount() const = 0;

		/**
		 * Specifies the texture page that is to be used for masking during ray tracing.
		 * @note The texture page must have been previously added to this instance.
		 * @note The texture page must have an alpha channel.
		 *
		 * @param texturePage the texture page to add or nullptr to disable alpha masking.
		 * @return true if successful
		 */
		virtual bool SetUseTexturePageAsAlphaMask(IInstaLODTexturePage* texturePage) = 0;

		/**
		 * Gets the texture page currently used as mask.
		 *
		 * @return the texture page or nullptr if no mask set.
		 */
		virtual IInstaLODTexturePage* GetAlphaMaskTexturePage() = 0;

		/**
		 * Gets the texture page currently used as mask.
		 *
		 * @return the texture page or nullptr if no mask set.
		 */
		virtual const IInstaLODTexturePage* GetAlphaMaskTexturePage() const = 0;

		/**
		 * Sets the default color of a specific texture page for this material.
		 * @note Default colors can also be specified for a IInstaLODMaterialData instance.
		 * Specifying on a per IInstaLODMaterialData will provide default values for all
		 * materials unless overridden on a per IInstaLODMaterial basis.
		 *
		 * @param name the name of the texture page
		 * @param color the default color
		 */
		virtual void SetDefaultColorForTexturePage(const char* name, const InstaColorRGBAF32& color) = 0;

		/**
		 * Gets the default color of a specific texture page for this material.
		 * @note Default colors can also be specified for a IInstaLODMaterialData instance.
		 * This method will not return default color on a per IInstaLODMaterialData basis.
		 *
		 * @param name the name of the texture page
		 * @param outColor the default color output storage
		 * @return true if a default color for the specified texture page has been found
		 */
		virtual bool GetDefaultColorForTexturePage(const char* name, InstaColorRGBAF32& outColor) const = 0;
	};

	/**
	 * The IInstaLODMaterialData interface represents a material collection
	 * used for material related operations.
	 * All materials referenced by the input meshes of a mesh operation should be added to the material data that is
	 * passed to the mesh operation.
	 * If operations generate output textures, like remeshing or baking. Custom textures must be enabled manually
	 * by invoking EnableOutputForTexturePage() with the name of the input texture page.
	 * Input texture pages must have the identical name (like "Color" or "Roughness"), in order for InstaLOD to recognize
	 * them as a single texture page group type and write them to a single output texture with the same name.
	 * However, InstaLOD is capable of sampling from different texture page pixel specifications within the same texture page group.
	 * If no output texture size for a specific texture group has been specified by invoking SetOutputTextureSizeForPage()
	 * InstaLOD will use the default output texture size.
	 */
	class IInstaLODMaterialData
	{
	protected:
		virtual ~IInstaLODMaterialData() {}

	public:
		/**
		 * Adds a material to this instance.
		 * @note The material IDs have to match the IDs as specified in the associated geometry.
		 * @note The contents of name will be copied and can be freed after passing to AddMaterialWithID.
		 *
		 * @param name the name of the material
		 * @param materialID the ID of the material
		 * @return material or nullptr if the material couldn't be added
		 */
		virtual IInstaLODMaterial* AddMaterialWithID(const char* name, const InstaMaterialID materialID) = 0;

		/**
		 * Gets the material with the specified name.
		 *
		 * @return the material with the specified name or nullptr if the material does not exist
		 */
		virtual IInstaLODMaterial* GetMaterial(const char* name) = 0;

		/**
		 * Gets the material with the specified name.
		 *
		 * @return the material with the specified name or nullptr if the material does not exist
		 */
		virtual const IInstaLODMaterial* GetMaterial(const char* name) const = 0;

		/**
		 * Gets the material with the specified ID.
		 *
		 * @return the material with the specified ID or nullptr if the material does not exist
		 */
		virtual IInstaLODMaterial* GetMaterialWithID(const InstaMaterialID materialID) = 0;

		/**
		 * Gets the material with the specified ID.
		 *
		 * @return the material with the specified ID or nullptr if the material does not exist
		 */
		virtual const IInstaLODMaterial* GetMaterialWithID(const InstaMaterialID materialID) const = 0;

		/**
		 * Sets the default output texture size.
		 * @note The default size can be overridden on a per texture page basis using SetOutputTextureSizeForPage()
		 *
		 * @param width the width
		 * @param height the height
		 */
		virtual void SetDefaultOutputTextureSize(const uint32 width, const uint32 height) = 0;

		/**
		 * Sets the output texture size for the specified page.
		 * @note Set width or height to a value of 0 to disable page specific
		 * output texture size.
		 *
		 * @param name the name of the page (case sensitive)
		 * @param width the output width
		 * @param height the output height
		 * @return true if the output texture size has been set
		 */
		virtual bool SetOutputTextureSizeForPage(const char* name, const uint32 width, const uint32 height) = 0;

		/**
		 * Gets the output texture size for the specified page.
		 *
		 * @param name the name of the page (case sensitive)
		 * @param width the output width
		 * @param height the output height
		 * @return true if the specified texture page has custom dimensions, false if the
		 * default texture dimensions are used.
		 */
		virtual bool GetOutputTextureSizeForPage(const char* name, uint32* width, uint32* height) const = 0;

		/**
		 * Enables the writing of the specified texture page.
		 * All texture pages that should be created during a merge, bake or remesh operation
		 * must be manually enabled.
		 * @note If output has already been enabled, output page specifications will be overwritten
		 * with the updated settings.
		 *
		 * @param name the name of the page (case sensitive)
		 * @param type the type of the output page
		 * @param componentType the component type of the output page
		 * @param pixelType the pixel type of the output page
		 * @return true if outputting of the specified page has been enabled.
		 */
		virtual bool EnableOutputForTexturePage(const char* name, const IInstaLODTexturePage::Type type, const IInstaLODTexturePage::ComponentType componentType, const IInstaLODTexturePage::PixelType pixelType) = 0;

		/**
		 * Disables the writing of the specified texture page.
		 * All texture pages that should be created during a merge, bake or remesh operation
		 * must be manually enabled.
		 *
		 * @param name the name of the page (case sensitive)
		 * @return true if outputting of the specified page has been disabled.
		 */
		virtual bool DisableOutputForTexturePage(const char* name) = 0;

		/**
		 * Determines whether writing of the specified texture page is enabled.
		 * All texture pages that should be created during a merge, bake or remesh operation
		 * must be manually enabled.
		 *
		 * @param name the name of the page (case sensitive)
		 * @return true if output for the specified page is enabled.
		 */
		virtual bool IsOutputEnabledForTexturePage(const char* name) const = 0;

		/**
		 * Determines whether a texture page of the specified type has been added to this instance.
		 *
		 * @param type the type of the page
		 * @return true if a page exists.
		 */
		virtual bool HasMaterialWithTexturePageOfType(const IInstaLODTexturePage::Type type) const = 0;

		/**
		 * Sets the default color of a specific texture page.
		 * @note Default colors can also be specified for a IInstaLODMaterial instance.
		 * Specifying on a per IInstaLODMaterialData will provide default values for all
		 * materials unless overridden on a per IInstaLODMaterial basis.
		 *
		 * @param name the name of the texture page
		 * @param color the default color
		 */
		virtual void SetDefaultColorForTexturePage(const char* name, const InstaColorRGBAF32& color) = 0;

		/**
		 * Gets the default color of a specific texture page.
		 * @note Default colors can also be specified for a IInstaLODMaterial instance.
		 * Specifying on a per IInstaLODMaterialData will provide default values for all
		 * materials unless overridden on a per IInstaLODMaterial basis.
		 *
		 * @param name the name of the texture page
		 * @param outColor the default color output storage
		 * @return true if a default color for the specified texture page has been found
		 */
		virtual bool GetDefaultColorForTexturePage(const char* name, InstaColorRGBAF32& outColor) const = 0;
	};

	/**
	 * The InstaLODDistanceFieldStorageOrder enumeration represents the storage order in which the cells in the octree get stored in linear memory.
	 */
	namespace InstaLODDistanceFieldStorageOrder
	{
		enum Type
		{
			ZAsImagesXYAsRows = 0,		/**< Stores a sequence of slices along the Z direction. Each slice is an image stored as rows along the X axis. */
			ZAsImagesXYAsColumns = 1	/**< Stores a sequence of slices along the Z direction. Each slice is an image stored as columns along the Y axis. */
		};
	}

	/**
	 * The InstaLODDistanceFieldInterleavingMode enumeration represents the interleaving mode in which multiple data fields of a cell gets stored in linear memory.
	 */
	namespace InstaLODDistanceFieldInterleavingMode
	{
		enum Type
		{
			ArrayOfStructures = 0,	/**< All data fields of a cell are stored together before the next cell gets stored. */
			StructureOfArrays = 1	/**< The data gets stored in pages. The first page contains the first attribute for all cells, the second page contains the second attribute, etc. */
		};
	}

	/**
	 * The InstaLODDistanceFieldSerializationOptions class provides the options to configure serialization of a distance field.
	 */
	class InstaLODDistanceFieldSerializationOptions
	{
	public:
		InstaLODDistanceFieldSerializationOptions()
			: PersistAllLevels(true)
			, Level(0u)
		{
		}

		bool PersistAllLevels; /**< The flag to switch between saving all levels and only a singel level. */
		uint32 Level;		   /**< If PersistAllLevels is false, the level that gets saved. Otherwise this value is ignored. */
	};

	/**
	 * The InstaLODDistanceFieldSamplingOptions class provides the options to configure sampling of values of a distance field into an array.
	 */
	class InstaLODDistanceFieldSamplingOptions
	{
	public:
		InstaLODDistanceFieldSamplingOptions()
			: HermiteData(false)
			, StorageOrder(InstaLODDistanceFieldStorageOrder::ZAsImagesXYAsRows)
			, InterleavingMode(InstaLODDistanceFieldInterleavingMode::ArrayOfStructures)
		{
		}

		bool HermiteData;											  /**< The flag to select between just the distance information (false) or distance and direction information (true). */
		InstaLODDistanceFieldStorageOrder::Type StorageOrder;		  /**< The array storage order. */
		InstaLODDistanceFieldInterleavingMode::Type InterleavingMode; /**< The interleaving mode (array of structures or structure of arrays) that determines how the components of the cells get stored in the linear output array. */
	};

	/**
	 * The DistanceFieldStorageType enum represents the storage type of a distance field.
	 */
	namespace DistanceFieldStorageType
	{
		enum Type
		{
			SparseStorage,	/**< The distance field is stored in a sparse data structure that grows as needed. */
			DenseStorage,	/**< The distance fiel is stored in a preallocated dense data structure. Empty cells are still possible, but they consume memory. */
			Count		    /**< The number of supported enum values. */
		};
	}

	/**
	 * The InstaLODDistanceFieldSparseCell struct stores the cell position and the distance value.
	 *
	 * This struct is used to export a sparse distance field into an array, so that it can be read into a custom distance field.
	 */
	struct InstaLODDistanceFieldSparseCell
	{
		uint16 X;			/**< The X index of the cell position. */
		uint16 Y;			/**< The Y index of the cell position. */
		uint16 Z;			/**< The Z index of the cell position. */
		double Distance;	/**< The distance value. */
	};


	typedef void (*pfnDistanceFieldDownsampleProgressCallback)(const float progressInPercent, void* callbackUserData);
	typedef void (*pfnDistanceFieldMakeDenseLayerProgressCallback)(const float progressInPercent, void* callbackUserData);

	/**
	 * The IInstaLODDistanceField class is the interface for signed or unsigned distance fields.
	 *
	 * During allocation, there is choice of creating a distance field with dense or sparse storage.
	 * This is only for allocation purposes. Even with dense storage, cells can be empty.
	 * By default, the distance field is created sparse, with only a thin layer around the mesh surface.
	 * It can be turned into a dense distance field with MakeDenseLayer.
	 *
	 * The distance field can have downsampled layers, which are can be empty.
	 * If the downsampled levels are required, CalculateDownsampledLevel must be called.
	 */
	class IInstaLODDistanceField
	{
	public:
		virtual ~IInstaLODDistanceField() = default;

		/**
		 * Creates a dense array of the distance field.
		 *
		 * @note If hermite data is requested, the order is distance, direction.X, direction.Y, direction.Z.
		 * @note In array of structures mode, the ordering is distance0, direction0.X, direction0.Y, direction0.Z, distance1, direction1.X, ...
		 * @note In structure of array mode, the ordering is distance0, distance1, distance2, ..., direction0.X, direction1.X, ..., direction0.Y, direction1.Y, ...
		 *
		 * @param level The octree level at which the array gets created.
		 * @param options The options to control the array layout.
		 * @param outData The pointer to the start of the array. The array must be provided by the user.
		 * @param dataSizeInBytes The number of bytes of the outData array.
		 * @return True upon success.
		 */
		virtual bool GetArrayDataForLevel(uint32 level, const InstaLODDistanceFieldSamplingOptions& options, float* outData, uint64 dataSizeInBytes) const = 0;

		/**
		 * Gets the number of stored nodes at an octree level.
		 *
		 * @note If the distance field has dense storage, the returned value is always equal to the total number of cells in the grid.
		 *
		 * @param level The octree level.
		 * @return The number of stored cells at the octree level.
		 */
		virtual uint64 GetSparseCellCountAtLevel(uint32 level) const = 0;

		/**
		 * Copies the sparse cell data at an octree level into an array.
		 *
		 * @note The output array must be large enough to hold all cells. You can get the required size by calling GetSparseCellCountAtLevel.
		 *
		 * The output array contains the sparse cell data. The sparse cells appear in random order.
		 *
		 * @param outSparseCellData The output array of sparse cells. It must be large enough to hold GetSparseCellCountAtLevel(level) elements.
		 * @param sparseCellDataElementCount The number of elements allocated in the outSparseCellData.
		 * @param level The octree level.
		 * @return The number of elements written into outSparseCellData.
		 */
		virtual uint64 GetSparseCellDataAtLevel(InstaLODDistanceFieldSparseCell* outSparseCellData, uint64 sparseCellDataElementCount, uint32 level) const = 0;

		/**
		 * Serializes the distance field to a file.
		 *
		 * @param options The serialization options.
		 * @param path The path for the output file.
		 * @return True upon success.
		 */
		virtual bool Serialize(InstaLODDistanceFieldSerializationOptions options, const char* path) const = 0;

		/**
		 * Gets the bounding box of the distance field volume.
		 *
		 * @return The bounding box.
		 */
		virtual InstaBoundingBox3F GetBoundingBox() const = 0;

		/**
		 * Gets the voxel size at the leaf node level.
		 *
		 * @return The voxel size at the leaf node level.
		 */
		virtual InstaVec3F GetVoxelSize() const = 0;

		/**
		 * Gets the resolution of the distance field at the leaf node level.
		 *
		 * The resolution is the edge length in voxels of distance field volume.
		 * For example at resolution 16 there are 16x16x16 voxels at the leaf node level (level 4).
		 *
		 * @return The resolution of the distance field at the leaf node level.
		 */
		virtual uint32 GetResolution() const = 0;

		/**
		 * Samples the distance field value at a voxel center.
		 *
		 * @param level The level for sampling.
		 * @param x The index of the voxel in x direction.
		 * @param y The index of the voxel in y direction.
		 * @param z The index of the voxel in z direction.
		 * @param outDistance The distance field value at the voxel center.
		 * @return True upon success.
		 */
		virtual bool SampleDistance(uint32 level, uint32 x, uint32 y, uint32 z, double& outDistance) const = 0;

		/**
		 * Samples the distance field value and direction at a voxel center.
		 *
		 * @param level The level for sampling.
		 * @param x The index of the voxel in x direction.
		 * @param x The index of the voxel in y direction.
		 * @param x The index of the voxel in z direction.
		 * @param outDistance The distance field value at the voxel center.
		 * @param outDirection The direction at the voxel center.
		 * @return True upon success.
		 */
		virtual bool SampleHermiteData(uint32 level, uint32 x, uint32 y, uint32 z, double& outDistance, InstaVec3D& outDirection) const = 0;

		/**
		 * Samples the distance field values by interpolating between the nearest voxels.
		 *
		 * @note This API uses trilinear interpolation and samples the octree at the level with the highest resolution.
		 *
		 * @param position The sampling position.
		 * @param outDistance The interpolated distance.
		 * @return True upon success.
		 */
		virtual bool SampleInterpolatedDistance(const InstaVec3F& position, float& outDistance) const = 0;

		/**
		 * Samples the hermite data by interpolating between the nearest voxels.
		 *
		 * This uses trilinear interpolation at the dense octree level with the greatest resolution.
		 *
		 * @param position The sampling position.
		 * @param outDistance The interpolated distance.
		 * @param outDirection The interpolated direction.
		 * @return True upon success.
		 */
		virtual bool SampleInterpolatedHermiteData(const InstaVec3F& position, float& outDistance, InstaVec3F& outDirection) const = 0;

		/**
		 * Gets the number of levels in the distance field.
		 *
		 * @return The number of levels in the distance field.
		 */
		virtual uint32 GetLevelCount() const = 0;

		/**
		 * Gets the leaf level of the distance field.
		 *
		 * @return The leaf level of the distance field.
		 */
		virtual uint32 GetLeafLevel() const = 0;

		/**
		 * Checks is a level is the leaf level.
		 *
		 * @param level The level.
		 * @return True if the level is the leaf level.
		 */
		virtual bool IsLeafLevel(const uint32 level) const = 0;

		/**
		 * Calculates a downsampled level.
		 *
		 * @param sourceLevel The source level.
		 * @param targetLevel The downsampled level.
		 * @param mesh The mesh that was used to create the original distance field.
		 * @param progressCallback The optional progress callback.
		 * @param progressCallbackUserData The opaque pointer passed through to the progress callback.
		 * @return True upon Success.
		 */
		virtual bool CalculateDownsampledLevel(uint32 sourceLevel, uint32 targetLevel, const IInstaLODMesh* mesh, pfnDistanceFieldDownsampleProgressCallback progressCallback, void* progressCallbackUserData) = 0;

		/**
		 * Makes an octree level dense.
		 *
		 * @param denseLevel The level to make dense.
		 * @param mesh The mesh.
		 * @param progressCallback The optional progress callback.
		 * @param progressCallbackUserData The opaque pointer passed through to the progress callback.
		 * @return True upon success.
		 */
		virtual bool MakeDenseLayer(uint32 denseLevel, const IInstaLODMesh* mesh, pfnDistanceFieldMakeDenseLayerProgressCallback progressCallback, void* progressCallbackUserData) = 0;

		/**
		 * Checks if the level is downsampled and available for sampling.
		 *
		 * @param level The level.
		 * @return True if the level is downsampled and available for sampling.
		 */
		virtual bool IsDownsampledLevel(uint32 level) = 0;

		/**
		 * Checks if the level has a coarser, downsampled level available.
		 *
		 * @param level The level.
		 * @return True if there is a coarser, downsampled level available.
		 */
		virtual bool HasDownsampledLevel(uint32 level) = 0;

		/**
		 * Gets the size of a cell at a level.
		 *
		 * @param level The level.
		 * @return The cell size.
		 */
		virtual double GetCellSizeAtLevel(const uint32 level) const  = 0;

		/**
		 * Gets the storage type of the distance field instance.
		 *
		 * @return The storage type.
		 */
		virtual DistanceFieldStorageType::Type GetStorageType() const = 0;
	};

	/**
	 * The HemisphereSamplingType enumeration specifies different types of hemisphere sampling typically used during bake operations.
	 */
	namespace HemisphereSamplingType
	{
		enum Type
		{
			Cosine = 0,	 /**< Cosine weighted samples. */
			Uniform = 1, /**< Generate uniform random samples over the hemisphere over the surface normal. */
			Count = 2
		};
	}

	/**
	 * The AmbientOcclusionSettings structure contains settings related to the creation of ambient occlusion and bent normals texture pages.
	 * The settings are also used when calculating BentNormals or the Reflectance as those maps are a bi-product of calculating AmbientOcclusion.
	 */
	struct AmbientOcclusionSettings
	{
		AmbientOcclusionSettings() :
		RayLengthPercentage(0.25f),
		IntensityScale(1.0f),
		SurfaceOffset(0.0001f),
		ConeAngle(0.25f),
		AttenuateByDistance(true),
		RandomDeviation(true),
		SampleCount(16u),
		BlurKernel(0u),
		SamplingType(HemisphereSamplingType::Cosine),
		SubmeshFiltered(false),
		FalloffSquare(false)
		{
		}

		float RayLengthPercentage;				   /**< Ray length in percent of the bounding sphere of the mesh. */
		float IntensityScale;					   /**< Intensity scale factor for AO value. */
		float SurfaceOffset;					   /**< Surface offset in percent of the bounding sphere of the mesh. */
		float ConeAngle;						   /**< Minimum angle for sampling cone 0 = 180°, 1 = 0°. */
		bool AttenuateByDistance;				   /**< True to attenuate AO values by distance. */
		bool RandomDeviation;					   /**< Randomizes the deviation for each sample, this results in the common noisy appearance. */
		uint32 SampleCount;						   /**< The amount of sub rays to cast for each pixel. */
		uint32 BlurKernel;						   /**< The size of the AO blur kernel. Set to 0 to disable. @note BentNormals output will not be blurred. */
		HemisphereSamplingType::Type SamplingType; /**< Sampling type used to generate subsamples. */
		bool SubmeshFiltered;					   /**< Enable to filter AO rays based on the submesh filtering settings. */
		bool FalloffSquare;						   /**< True if the falloff is square, which results in a smoother map. */
	};

	/**
	 * The ThicknessSettings structure contains settings related to the creation of thickness texture pages.
	 */
	struct ThicknessSettings
	{
		ThicknessSettings() :
		RayLengthPercentage(0.25f),
		SurfaceOffset(0.0001f),
		ConeAngle(0.5f),
		Normalize(true),
		RandomDeviation(true),
		SampleCount(16u),
		BlurKernel(0u),
		SamplingType(HemisphereSamplingType::Cosine),
		SubmeshFiltered(true)
		{
		}

		float RayLengthPercentage;				   /**< Ray length in percent of the bounding sphere of the mesh. */
		float SurfaceOffset;					   /**< Surface offset in percent of the bounding sphere of the mesh. */
		float ConeAngle;						   /**< Minimum angle for thickness sampling cone 0 = 180°, 1 = 0°. */
		bool Normalize;							   /**< Enables normalization of minimum and maximum thickness values. */
		bool RandomDeviation;					   /**< Randomizes the deviation for each sample, this results in the common noisy appearance. */
		uint32 SampleCount;						   /**< The amount of sub rays to cast for each pixel. */
		uint32 BlurKernel;						   /**< The size of the blur kernel. Set to 0 to disable. */
		HemisphereSamplingType::Type SamplingType; /**< Sampling type used to generate subsamples. */
		bool SubmeshFiltered;					   /**< Enable to filter thickness rays based on the submesh filtering settings. */
	};

	/**
	 * The BakeOutputSettings controls various parameters related to the
	 * texture pages and vertex information created during bake operations.
	 */
	struct BakeOutputSettings
	{
		BakeOutputSettings() :
		SourceMeshUVChannelIndex(0),
		SolidifyTexturePages(true),
		SuperSampling(SuperSampling::X2),
		DilationInPixels(5),
		ComputeBinormalPerFragment(false),
		NormalizeTangentSpacePerFragment(false),
		TangentSpaceFormat(MeshFormat::OpenGL),
		TextureFilter(TextureFilter::Bilinear),
		TexturePageNormalTangentSpace(false),
		TexturePageNormalObjectSpace(true),
		TexturePageMeshID(false),
		TexturePageVertexColor(false),
		TexturePageAmbientOcclusion(false),
		TexturePageBentNormals(false),
		TexturePageThickness(false),
		TexturePageDisplacement(false),
		TexturePagePosition(false),
		TexturePageCurvature(false),
		TexturePageTransfer(false),
		TexturePageBarycentric(false),
		TexturePageReflectance(false),
		TexturePageOpacity(false),
		TexturePageCustom(true),
		SkeletalMeshVertexWeights(true),
		WedgeColorSourceTexturePageNames(),
		AmbientOcclusion(),
		Thickness(),
		CurvatureStrength(0.5f),
		MeshIDByFaceSubMeshIndices(true),
		AutoDelightColorOutput(false),
		NormalizePositionByAABB(true),
		Delight(),
		DisplacementFullRange(false)
		{
			for (uint32 colorIndex = 0u; colorIndex < INSTALOD_MAX_MESH_COLORSETS; colorIndex++)
			{
				WedgeColorSourceTexturePageNames[colorIndex] = nullptr;
			}
		}

		uint32 SourceMeshUVChannelIndex;	   /**< The UV channel of the source mesh used when sampling texture pages. */
		bool SolidifyTexturePages;			   /**< Enables solidification of texture pages to avoid pixel bleed. */
		SuperSampling::Type SuperSampling;	   /**< Super sampling. */
		uint32 DilationInPixels;			   /**< The dilation size to avoid mipmap pixel bleed. */
		bool ComputeBinormalPerFragment;	   /**< This setting depends on how your renderer/engine computes the tangent basis. */
		bool NormalizeTangentSpacePerFragment; /**< This setting depends on how your renderer/engine computes the tangent basis. */
		MeshFormat::Type TangentSpaceFormat;   /**< The output type of the automatically computed tangent space. */

		TextureFilter::Type TextureFilter; /**< Texture filter used when sampling texture. */

		bool TexturePageNormalTangentSpace; /**< 16bpp RGB, Name: NormalTangentSpace */
		bool TexturePageNormalObjectSpace;	/**< 16bpp RGB, Name: NormalObjectSpace */
		bool TexturePageMeshID;				/**<  8bpp RGB, Name: MeshID */
		bool TexturePageVertexColor;		/**< 16bpp RGBA, Name: VertexColor0...n where n is INSTALOD_MAX_MESH_COLORSETS-1 */
		bool TexturePageAmbientOcclusion;	/**<  8bpp LUMINANCE, Name: AmbientOcclusion */
		bool TexturePageBentNormals;		/**< 16bpp RGB, Name: BentNormals */
		bool TexturePageThickness;			/**< 16bpp LUMINANCE, Name: Thickness */
		bool TexturePageDisplacement;		/**< 16bpp LUMINANCE, Name: Displacement */
		bool TexturePagePosition;			/**< 16bpp RGB, Name: Position */
		bool TexturePageCurvature;			/**< 16bpp LUMINANCE, Name: Curvature */
		bool TexturePageTransfer;			/**< 16bpp RGB, Name: Transfer */
		bool TexturePageBarycentric;		/**< 16bpp RGBA, Name: Barycentric
													@note This texture page should only be used by SDK users.
													The RG channels of the resulting texture page store the XY barycentric coordinates. Z barycentric coordinate = 1.0f - barycentric.X - barycentric.Y.
													The BA channels of the resulting texture page store a 32bit unsigned integer face index value. */
		bool TexturePageReflectance;		/**<  8bpp LUMINANCE, Name: Reflectance */
		bool TexturePageOpacity;			/**<  8bpp RGBA, Name: Opacity */
		bool TexturePageCustom;				/**< Enable to generate output texture pages as specified in the BakeOperation MaterialData.
											 Use MaterialData::SetOutputForTexturePageEnabled to enable baking of source pages. */

		bool SkeletalMeshVertexWeights;											   /**< Enable to transfer skeletal mesh weights. */
		const char* WedgeColorSourceTexturePageNames[INSTALOD_MAX_MESH_COLORSETS]; /**< The name of the texture page that will be baked into the WedgeColors set at the specified index of the target mesh.
																						@note If name matches the name of an internally generated texture page (e.g. AmbientOcclusion, Thickness, etc.)
																						and the page has been generated it will be used even if a page with the name has been setup in the BakeOperation MaterialData.
																						Set to nullptr or a valid texture page name. */
		AmbientOcclusionSettings AmbientOcclusion;								   /**< Ambient occlusion and bent normals settings. */
		ThicknessSettings Thickness;											   /**< Thickness settings. */
		float CurvatureStrength;												   /**< Visibility of details on the curvature map. */
		bool MeshIDByFaceSubMeshIndices;										   /**< Enable to calculate MeshID based on FaceSubMeshIndices of the source mesh.
														If disabled, InstaLOD will calculate submeshes based on vertex adjacency. */
		bool AutoDelightColorOutput;											   /**< Enable to automatically delight all output color textures. */
		bool NormalizePositionByAABB;											   /**< Normalize the Position map output by AABB. If disabled, InstaLOD will normalize the position map using the bounding sphere. */
		DelightSettings Delight;												   /**< Delight settings. */
		bool DisplacementFullRange;												   /**< Enable to output a full range displacement map. This maps the lowest point in the displacement map to 0 and the highset to 1 in the output image. */
	};

	/**
	 * The BakeResult struct contains information about a completed bake operation.
	 */
	struct BakeResult
	{
		BakeResult() :
		Success(false),
		IsAuthorized(true),
		BakeMaterial(nullptr),
		DisplacementMinimum(0.0f),
		DisplacementMaximum(0.0f),
		MaterialIndex(0u)
		{
		}

		bool Success;					 /**< True, if the operation was successful. */
		bool IsAuthorized;				 /**< True, if the host was properly authorized during the operation. */
		IInstaLODMaterial* BakeMaterial; /**< The generated material. Valid until the operation is deallocated. */
		float DisplacementMinimum;		 /**< Minimum value for the displacement map. */
		float DisplacementMaximum;		 /**< Maximum value for the displacement map. */
		uint32 MaterialIndex;			 /**< The material index of this result. */
	};

	/**
	 * The MultiBakeResult struct contains information about a completed multi-bake operation.
	 */
	struct MultiBakeResult
	{
		MultiBakeResult() :
		Success(false),
		IsAuthorized(true),
		Results(nullptr),
		ResultCount(0u)
		{
		}

		bool Success;		 /**< True, if the operation was successful. */
		bool IsAuthorized;	 /**< True, if the host was properly authorized during the operation. */
		BakeResult* Results; /**< An array containing all bake results. The array size is \# ResultCount. */
		uint32 ResultCount;	 /**< The result count.*/
	};

	/**
	 * The BakeEngine represents different types of bake engines supported by InstaLOD.
	 */
	namespace BakeEngine
	{
		enum Type
		{
			CPU = 0, /**< Uses the high performance CPU baking engine. */
			GPU = 1, /**< Uses the GPU baking engine if available on the target platform.
					  The InstaLOD GPU baking engine supports Vulkan on PC and Metal on macOS.*/
			Count = 2
		};
	}

	/**
	 * The BakeSettings structure contains settings exposed by the bake API.
	 */
	struct BakeSettings
	{
		BakeSettings() :
		BakeEngine(BakeEngine::CPU),
		CageMesh(nullptr),
		CageMeshNormalsAsRayDirections(true),
		RayOutwardsLengthPercentage(0.010f),
		RayInwardsLengthPercentage(0.025f),
		RayOutwardsLengthWorldSpace(0.0f),
		RayInwardsLengthWorldSpace(0.0f),
		VertexSkinWeightRayLengthPercentage(0.05f),
		VertexSkinWeightRayLengthWorldSpace(0.0f),
		AlphaMaskThreshold(1.0f),
		IgnoreBackface(true),
		FloodFillCustomTexturePagePixelsWithoutIntersection(true),
		AverageNormalsAsRayDirections(false),
		Output()
		{
		}

		BakeEngine::Type BakeEngine;		 /**< The bake engine. If the selected bake engine is not available, InstaLOD will revert to the CPU baker. */
		const IInstaLODMesh* CageMesh;		 /**< Optional cage mesh. Pointer must stay valid until bake is finished.
												@note The cage mesh geometry can be different from the target mesh geometry.
												The only constraint is that the UV shells map to the same texel.
												However, additional edges and vertices are not problematic.
												This makes it trivial to generate reliable orthogonal projection on faces by inserting additional edge loops.
												The benefit to this is you don't have to blend two bake results (averaged and not averaged) together
												as the cage generation is easy and predictable. */
		bool CageMeshNormalsAsRayDirections; /**< Use cage mesh normals when computing ray directions instead of the target mesh normals. */

		float RayOutwardsLengthPercentage;						  /**< Outwards ray length in percentage of the bounding sphere of the source mesh. */
		float RayInwardsLengthPercentage;						  /**< Inwards ray length in percentage of the bounding sphere of the source mesh. */
		float RayOutwardsLengthWorldSpace;						  /**< Outwards ray length in world space units. If set, this value precedes RayOutwardsLengthPercentage. */
		float RayInwardsLengthWorldSpace;						  /**< Inwards ray length in world space units. If set, this value precedes RayInwardsLengthPercentage. */
		float VertexSkinWeightRayLengthPercentage;				  /**< Vertex skin weight ray length in percentage of the bounding sphere of the source mesh. */
		float VertexSkinWeightRayLengthWorldSpace;				  /**< Vertex skin weight ray length in world space units. If set, this value precedes VertexSkinWeightRayLengthPercentage. */
		float AlphaMaskThreshold;								  /**< Alpha mask values below this threshold will be considered transparent. */
		bool IgnoreBackface;									  /**< Ignore backfaces. */
		bool FloodFillCustomTexturePagePixelsWithoutIntersection; /**< True to flood fill pixels of custom texture pages that do not intersect the source mesh. */
		bool AverageNormalsAsRayDirections;						  /**< Use averaged face normals of target mesh as ray directions. */

		BakeOutputSettings Output; /**< Output settings. */
	};

	typedef bool (*pfnBakeExecuteCustomRaytrace)(class IInstaLOD*, class IBakeOperation*, struct InstaLODRaytraceData&, struct InstaLODOutputSamplerData&);
	typedef void (*pfnBakeProgressCallback)(class IBakeOperation* operation, const IInstaLODMesh* sourceMesh, IInstaLODMesh* targetMesh, const float progressInPercent);

	/**
	 * The IBakeOperation interface represents a mesh projection (baking) operation.
	 * It supports a variety of different settings and transfer options like per-pixel texture data and vertex skinning weights.
	 */
	class IBakeOperation
	{
	protected:
		virtual ~IBakeOperation() {}

	public:
		/**
		 * Sets a custom raytracing engine callback.
		 * @note The custom raytrace engine will receive all necessary data to perform the
		 * actual raytracing as expected.
		 *
		 * @param callback the raytracing callback
		 */
		virtual void SetRaytraceExecutionCallback(pfnBakeExecuteCustomRaytrace callback) = 0;

		/**
		 * Sets the material data used for this operation.
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The material data should contain a material for each referenced material ID.
		 * @note Each material should contain at least one texture page. Texture pages that should be outputted
		 * on the same texture page should have identical names.
		 *
		 * @param data the material data
		 */
		virtual void SetMaterialData(IInstaLODMaterialData* data) = 0;

		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnBakeProgressCallback callback) = 0;

		/**
		 * Adds a sub mesh filter relation to this instance.
		 * @note Sub mesh filters can be used to prevent baking of unwanted sub meshes
		 * onto target surfaces.
		 *
		 * @param sourceSubMeshID the ID of the source sub mesh
		 * @param targetSubMeshID the ID of the target sub mesh
		 */
		virtual void AddSubMeshFilter(const uint32 sourceSubMeshID, const uint32 targetSubMeshID) = 0;

		/**
		 * Sets the source sub mesh world transform.
		 * @note If the world transform is not specified, the ObjectSpace texture pages
		 * will be treated as WorldSpaceNormals.
		 *
		 * @param sourceSubMeshID The ID of the target sub mesh
		 * @param transform The world transform of the submesh at \p sourceSubMeshID
		 */
		virtual void SetSourceSubMeshWorldTransform(const uint32 sourceSubMeshID, const InstaMatrix4D& transform) = 0;

		/**
		 * Sets the target sub mesh world transform.
		 * @note If the world transform is not specified, the ObjectSpace sampler
		 * will produce WorldSpaceNormals.
		 *
		 * @param targetSubMeshID The ID of the target sub mesh
		 * @param transform The world transform of the submesh at \p targetSubMeshID
		 */
		virtual void SetTargetSubMeshWorldTransform(const uint32 targetSubMeshID, const InstaMatrix4D& transform) = 0;

		/**
		 * Performs the operation.
		 *
		 * @param sourceMesh the source mesh
		 * @param targetMesh the target mesh
		 * @param settings the settings
		 * @return result
		 */
		virtual BakeResult Execute(const IInstaLODMesh* sourceMesh, IInstaLODMesh* targetMesh, const BakeSettings& settings) = 0;

		/**
		 * Performs the operation.
		 * @note This API generates one output material for each material index of the \p targetMesh.
		 * This is useful to perform UDIM baking or when generating baking results for larger scenes.
		 *
		 * @param sourceMesh the source mesh
		 * @param targetMesh the target mesh
		 * @param settings the settings
		 * @return result
		 */
		virtual MultiBakeResult ExecuteMulti(const IInstaLODMesh* sourceMesh, IInstaLODMesh* targetMesh, const BakeSettings& settings) = 0;
	};

	/**
	 * The UnwrapStrategy enumeration specifies different types of strategies
	 * used by the IUnwrapOperation and IRemeshingOperation.
	 */
	namespace UnwrapStrategy
	{
		enum Type
		{
			Organic = 0,		  /**< Stretch based unwrapping suitable for organic type meshes. Computationally intensive for high poly meshes. */
			HardSurfaceAngle = 1, /**< Angle-based projections, suitable for hard-surface type meshes. */
			HardSurfaceAxial = 2, /**< Axial projections, suitable for hard-surface type meshes. */
			Auto = 3,			  /**< Automatic selection based on input mesh. */
			Count = 4
		};
	}

	/**
	 * The RemeshFaceCountTarget enumeration specifies different types of fuzzy
	 * face count targets used by the IRemeshingOperation.
	 */
	namespace RemeshFaceCountTarget
	{
		enum Type
		{
			Lowest = 0,	 /**< Very low poly count. */
			Low = 1,	 /**< Low poly count. */
			Normal = 2,	 /**< Default poly count. */
			High = 3,	 /**< High poly count. */
			Highest = 4, /**< Very high poly count. */
			Count = 5
		};
	}

	/**
	 * The RemeshSurfaceMode enumeration specifies different types of
	 * surface construction used by the IRemeshingOperation.
	 */
	namespace RemeshSurfaceMode
	{
		enum Type
		{
			Reconstruct = 0, /**< Reconstruct the input geometry. */
			Optimize = 1,	 /**< Optimize the input geometry. */
			ConvexHull = 2,	 /**< Reconstruct the input geometry as convex hull. */
			UV = 3,			 /**< Only modifies the UV channel and material indices of the input geometry. */
			Count = 4
		};
	}

	/**
	 * The RemeshingSettings structure contains settings exposed by the remeshing API.
	 */
	struct RemeshingSettings
	{
		RemeshingSettings() :
		SurfaceMode(RemeshSurfaceMode::Reconstruct),
		DistinctSurfaceConstructionPerMesh(false),
		DistinctSurfaceConstructionAdaptiveResolution(true),
		SurfaceConstructionIgnoreBackface(false),
		Resolution(256),
		FaceCountTarget(RemeshFaceCountTarget::Normal),
		MaximumTriangles(0),
		ScreenSizeInPixels(0),
		ScreenSizePixelMergeDistance(2),
		ScreenSizeInPixelsAutomaticTextureSize(false),
		HardAngleThreshold(70.0f),
		WeldDistance(0.0f),
		GutterSizeInPixels(5),
		BakeAutomaticRayLengthFactor(1.0f),
		AlphaMaskThreshold(1.0f),
		IgnoreBackface(true),
		AutomaticOcclusionGeometry(false),
		BakeMesh(nullptr),
		UnwrapStrategy(UnwrapStrategy::Auto),
		StretchImportance(MeshFeatureImportance::Normal),
		ShellStitching(true),
		InsertNormalSplits(false),
		PostProcessLayout(true),
		SurfaceModeOptimizeLockBoundaries(false),
		BakeAverageNormalsAsRayDirections(false),
		BakeEngine(BakeEngine::CPU),
		BakeOutput(),
		Deterministic(false)
		{
		}

		RemeshSurfaceMode::Type SurfaceMode;				/**< The surface mode. */
		bool DistinctSurfaceConstructionPerMesh;			/**< When 'Mode' is set to 'Reconstruct', InstaLOD constructs a surface that fuses all meshes added to the remesh operation together and culls interior faces.
															 Set 'DistinctSurfaceConstructionPerMesh' to true to enable surface construction on a per mesh basis.
															 UV parameterization, UV packing and mesh projection will result in a single material and therefore a single draw call.
															 Constructing a surface for each mesh will increase memory consumption and processing time, but it also yields additional benefits.
															 For instance, a filter is applied during mesh projection that will prevent wrong parts of the original mesh to be projected onto the surface. */
		bool DistinctSurfaceConstructionAdaptiveResolution; /**< When 'DistinctSurfaceConstructionPerMesh' is enabled, scale the resolution per mesh according to its bounding sphere radius in relation to the
															 biggest bounding sphere radius when reconstructing the input geometry. */
		bool SurfaceConstructionIgnoreBackface;				/**< When 'Mode' is set to 'Reconstruct', ignores backfaces during surface construction.
															 @note This can cause holes in the constructed geometry if face normals are not pointing outwards. */
		uint32 Resolution;									/**< When 'Mode' is set to 'Reconstruct', the resolution of the surface construction in range [16...4096].
															 The resolution is ^3 so maximum memory pressure and processing time grows exponentially.
															 In our UI we're using the following values: Lowest=100 Low=150 Normal=256 High=512 Highest=1024 Extreme=2048
															 @note When 'ScreenSizeInPixels' is enabled, the resolution is automatically based on 'ScreenSizeInPixels' and 'ScreenSizePixelMergeDistance'. */
		RemeshFaceCountTarget::Type FaceCountTarget;		/**< The fuzzy face count target of the output geometry. */
		uint32 MaximumTriangles;							/**< The maximum amount of polygons for the target mesh. If set, this value precedes FaceCountTarget. */
		uint32 ScreenSizeInPixels;							/**< If set, InstaLOD calculates the amount of polygons to remove based on the display size of the output model. If set, this value precedes FaceCountTarget. */
		uint32 ScreenSizePixelMergeDistance;				/**< When 'Mode' is set to 'Reconstruct' and 'ScreenSizeInPixels' is enabled, the pixel distance in screen size that will be merged together.
															 This can be used to avoid the construction of unnecessary geometrical detail.
															 @note When 'ScreenSizeInPixels' is enabled, the resolution is automatically based on 'ScreenSizeInPixels' and 'ScreenSizePixelMergeDistance'. */
		bool ScreenSizeInPixelsAutomaticTextureSize;		/**< When 'ScreenSizeInPixels' is enabled, automatically compute output texture dimensions based on screen size in pixels. */
		float HardAngleThreshold;							/**< When calculating normals for the output mesh, smoothes faces if the normal angle is below this value (in degrees). */
		float WeldDistance;									/**< The welding distance can be used to weld holes in the input geometry. The welded mesh is only used for the surface construction it is not used during baking. */
		uint32 GutterSizeInPixels;							/**< The minimum distance between two UV shells in pixels. */
		float BakeAutomaticRayLengthFactor;					/**< Factor to increase or decrease the automatically calculated ray length used during bake. */
		float AlphaMaskThreshold;							/**< Alpha mask values below this threshold will be considered transparent. */
		bool IgnoreBackface;								/**< Ignore backfaces during bake. */
		bool AutomaticOcclusionGeometry;					/**< If the input geometry is not watertight due to small holes or gaps, InstaLOD will construct a mesh that contains interior faces.
															 In such cases manually placed occlusion geometry can be used to conceal these holes.
															 Alternatively, automatic occlusion geometry can be built by InstaLOD to avoid interior faces from being built.
															 However, enabling automatic occlusion geometry will result in a dramatically increased processing time. */
		const IInstaLODMesh* BakeMesh;						/**< Optional, used as bake source.
															 @note Using a bake mesh will prevent InstaLOD from setting up bake filters when DistinctSurfaceConstructionPerMesh is enabled. */

		UnwrapStrategy::Type UnwrapStrategy;		   /**< Strategy used during unwrap. */
		MeshFeatureImportance::Type StretchImportance; /**< The importance of UV space stretch during the unwrap. Off: high levels of stretch allowed, Highest: no stretching allowed.
															Stretch impacts the UV-shell and vertex split count. Allowing for higher stretch values means less UV-shells generated. */
		bool ShellStitching;						   /**< When 'UnwrapStrategy' is either 'HardSurfaceAxial' or 'HardSurfaceAngle', enables stitching of shells to reduce the amount of splits and shells. */
		bool InsertNormalSplits;					   /**< Insert UV shell splits for edges that do not share the same wedge normals. */
		bool PostProcessLayout;						   /**< [Preview] Performs a post-process to minimize the UV shell count. */
		bool SurfaceModeOptimizeLockBoundaries;		   /**< When 'SurfaceMode' is 'Optimize', lock boundary vertices in place. */
		bool BakeAverageNormalsAsRayDirections;		   /**< Use averaged face normals of target mesh as ray directions. */

		BakeEngine::Type BakeEngine;   /**< The bake engine type. */
		BakeOutputSettings BakeOutput; /**< The bake output settings. */
		bool Deterministic;			  /**< Makes the algorithm deterministic at the cost of speed. */
	};

	/**
	 * The RemeshingResult struct contains information about a completed remeshing operation.
	 */
	struct RemeshingResult
	{
		RemeshingResult() :
		Success(false),
		IsAuthorized(true),
		BakeMaterial(nullptr)
		{
		}

		bool Success;					 /**< True, if the operation was successful. */
		bool IsAuthorized;				 /**< True, if the host was properly authorized during the operation. */
		IInstaLODMaterial* BakeMaterial; /**< The generated material. Valid until the operation is deallocated. */
	};

	typedef void (*pfnRemeshProgressCallback)(class IRemeshingOperation* operation, IInstaLODMesh* outputMesh, const float progressInPercent);

	/**
	 * The IRemeshingOperation interface represents a remeshing operation.
	 */
	class IRemeshingOperation
	{
	protected:
		virtual ~IRemeshingOperation() {}

	public:
		/**
		 * Adds a mesh to this instance
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The IDs are created in a sequential fashion starting at 0.
		 *
		 * @param mesh the mesh data
		 * @return true if the mesh has been added successfully
		 */
		virtual bool AddMesh(const IInstaLODMesh* mesh) = 0;

		/**
		 * Sets the source sub mesh world transform.
		 * @note If the world transform is not specified, the ObjectSpace texture pages
		 * will be treated as WorldSpaceNormals.
		 *
		 * @param sourceSubMeshID The ID of the target sub mesh
		 * @param transform The world transform of the submesh at \p sourceSubMeshID
		 */
		virtual void SetSourceSubMeshWorldTransform(const uint32 sourceSubMeshID, const InstaMatrix4D& transform) = 0;

		/**
		 * Sets the target sub mesh world transform.
		 * @note If the world transform is not specified, the ObjectSpace sampler
		 * will produce WorldSpaceNormals.
		 * @note SubmeshIDs are generated in sequential fashion for each mesh added with a call to \# AddMesh.
		 * If DistinctSurfaceConstructionPerMesh is enabled, a transform for each mesh added to the operation
		 * must be specified. Otherwise, only the a worldtransform for a submesh with ID 0 needs to be set.
		 *
		 * @param targetSubMeshID The ID of the target sub mesh
		 * @param transform The world transform of the submesh at \p targetSubMeshID
		 */
		virtual void SetTargetSubMeshWorldTransform(const uint32 targetSubMeshID, const InstaMatrix4D& transform) = 0;

		/**
		 * Adds an occlusion mesh to this instance
		 * @note Occlusion meshes can be used to plug holes in the input geometry to avoid
		 * the construction of interior faces.
		 * @note There are no constraints in regards to the occlusion mesh geometry.
		 * However, water tight meshes should be used when plugging holes.
		 *
		 * @param mesh the mesh data
		 * @return true if the mesh has been added successfully
		 */
		virtual bool AddOcclusionMesh(const IInstaLODMesh* mesh) = 0;

		/**
		 * Sets the material data used for this operation.
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The material data should contain a material for each referenced material ID.
		 * @note Each material should contain at least one texture page. Texture pages that should be outputted
		 * on the same texture page should have identical names.
		 *
		 * @param data the material data
		 */
		virtual void SetMaterialData(IInstaLODMaterialData* data) = 0;

		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnRemeshProgressCallback callback) = 0;

		/**
		 * Adds the specified clipping plane to this instance.
		 *
		 * @param plane the plane
		 * @return the index of the plane
		 */
		virtual uint32 AddClippingPlane(const InstaPlaneF& plane) = 0;

		/**
		 * Gets the clipping plane at the specified index.
		 *
		 * @param index the index of the plane
		 * @param outPlane (output) plane
		 * @return true if index was valid and plane has been written
		 */
		virtual bool GetClippingPlaneAtIndex(const uint32 index, InstaPlaneF& outPlane) = 0;

		/**
		 * Removes the clipping plane at the specified index.
		 *
		 * @param index the index of the plane
		 * @return true if index was valid and plane has been removed
		 */
		virtual bool RemoveClippingPlaneAtIndex(const uint32 index) = 0;

		/**
		 * Gets the amount of clipping planes stored in this instance.
		 *
		 * @return the amount of clipping planes
		 */
		virtual uint32 GetClippingPlaneCount() const = 0;

		/**
		 * Removes all clipping planes from this instance.
		 */
		virtual void RemoveAllClippingPlanes() = 0;

		/**
		 * Performs the operation.
		 *
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return result
		 */
		virtual RemeshingResult Execute(IInstaLODMesh* outputMesh, const RemeshingSettings& settings) = 0;
	};

	/**
	 * The IsotropicRemeshingEdgeMode enumeration specifies different types of
	 * criteria to be used to determine the target edge size.
	 */
	namespace IsotropicRemeshingEdgeMode
	{
		enum Type
		{
			Automatic = 0,				/**< Use automatic edge detection mode. A value of 1.0 matches the automatic default.
										 Lower values decrease, higher values increase edge sizes. */
			Absolute = 1,				/**< Use absolute edge size. */
			BoundingSphereRelative = 2, /**< The specified edge size is specified in percentage [0...1] of the bounding sphere of the input mesh. */
			Count = 3
		};
	}

	/**
	 * The IsotropicRemeshingSettings structure contains settings exposed by the isotropic remeshing API.
	 */
	struct IsotropicRemeshingSettings
	{
		IsotropicRemeshingSettings() :
		EdgeMode(IsotropicRemeshingEdgeMode::Automatic),
		TargetEdgeSize(1.0f),
		Precision(MeshFeatureImportance::Normal),
		WeldingThreshold(0.0f),
		GeometricFeatureAngleInDegrees(0.0f),
		CollapseThreshold(0.75f),
		PreserveColorSplits(false),
		PreserveTexCoordSplits(true),
		PreserveNormalSplits(false),
		PreserveVolume(false),
		RecalculateNormals(false),
		HardAngleThreshold(80.0f),
		WeightedNormals(true),
		Deterministic(false)
		{
		}

		IsotropicRemeshingEdgeMode::Type EdgeMode; /**< The target edge mode. */
		float TargetEdgeSize;					   /**< The target edge size. */
		MeshFeatureImportance::Type Precision;	   /**< How many refinement iterations to execute, more iterations lead to a higher degree of isotropy. */
		float WeldingThreshold;					   /**< If >= 0.0 vertices will be welded as pre-pass. */
		float GeometricFeatureAngleInDegrees;	   /**< If > 0.0 geometrical features to preserve will be detected based on the dihedral angle. */
		float CollapseThreshold;				   /**< Edges smaller than the specified value will be collapsed.
													The value is specified in percentage of the computed target edge size:
													`AbsoluteThreshold = AbsoluteTargetEdgeSize * CollapseThreshold`. */
		bool PreserveColorSplits;				   /**< Determines if color splits will be preserved. */
		bool PreserveTexCoordSplits;			   /**< Determines if texcoord splits will be preserved. */
		bool PreserveNormalSplits;				   /**< Determines if normal splits will be preserved. */
		bool PreserveVolume;					   /**< Determines if volume preservation is enabled. @note Volume preservation minimizes the distances between the
												isotropic mesh vertices and the input mesh geometry. This constraint can impact the overall isotropy and
												smoothness of the output mesh.  */
		bool RecalculateNormals;				   /**< Recalculate normals of the output mesh. */
		float HardAngleThreshold;				   /**< When recalculating normals: smooth faces if the normal angle is below this value (in degrees). */
		bool WeightedNormals;					   /**< When recalculating normals: smoothed normals are weighted by various geometric properties. */
		bool Deterministic;						   /**< Makes the algorithm deterministic at the cost of speed. */
	};

	/**
	 * The IsotropicRemeshingResult struct contains information about a completed remeshing operation.
	 */
	struct IsotropicRemeshingResult
	{
		IsotropicRemeshingResult() :
		Success(false),
		IsAuthorized(true),
		TargetEdgeSize(-1.0f)
		{
		}

		bool Success;		  /**< True, if the operation was successful. */
		bool IsAuthorized;	  /**< True, if the host was properly authorized during the operation. */
		float TargetEdgeSize; /**< The target edge size in world-space units. */
	};

	typedef void (*pfnIsoTropicRemeshingProgressCallback)(class IIsotropicRemeshingOperation* operation, IInstaLODMesh* outputMesh, const float progressInPercent);
	/**
	 * The IIsotropicRemeshingOperation interface represents a isotropic remeshing operation.
	 */
	class IIsotropicRemeshingOperation
	{
	protected:
		virtual ~IIsotropicRemeshingOperation() {}

	public:
		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnIsoTropicRemeshingProgressCallback callback) = 0;

		/**
		 * Performs the operation.
		 * @note the input mesh and the output mesh can be the same instance.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return unwrap result
		 */
		virtual IsotropicRemeshingResult Execute(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const IsotropicRemeshingSettings& settings) = 0;
	};

	/**
	 * The QuadRemeshingEdgeMode enumeration specifies different types of
	 * criteria to be used to determine the target edge size.
	 */
	namespace QuadRemeshingEdgeMode
	{
		enum Type
		{
			Automatic = 0,				/**< Use automatic edge detection mode. A value of 1.0 matches the automatic default.
										 Lower values decrease, higher values increase edge sizes. */
			Absolute = 1,				/**< Use absolute edge size. */
			BoundingSphereRelative = 2, /**< The specified edge size is specified in percentage [0...1] of the bounding sphere of the input mesh. */
			Count = 3
		};
	}

	/**
	 * The QuadRemeshingSettings structure contains settings exposed by the quad remeshing API.
	 */
	struct QuadRemeshingSettings
	{
		QuadRemeshingSettings() :
		EdgeMode(QuadRemeshingEdgeMode::Automatic),
		TargetEdgeSize(1.0f),
		FeatureAlignment(0.0f),
		PreserveSharpFeatures(false),
		PreserveBoundaries(true),
		SmoothQuadMesh(true),
		Deterministic(false)
		{
		}

		QuadRemeshingEdgeMode::Type EdgeMode; /**< The target edge mode. */
		float TargetEdgeSize;				  /**< Controls the size of the quads on the resulting mesh. */
		float FeatureAlignment;				  /**< Controls the influence of the input mesh features (ex. principal curvature directions) on the resulting quad mesh topology. */
		bool PreserveSharpFeatures;			  /**< Determines if the quads are aligned to sharp feature of the input mesh. */
		bool PreserveBoundaries;			  /**< Determines if the quads are aligned to input mesh boundaries. */
		bool SmoothQuadMesh;				  /**< Determines if the resulting quad mesh topology will be smoothed. */
		bool Deterministic;					  /**< Makes the algorithm deterministic at the cost of speed. */
	};

	/**
	 * The QuadRemeshingResult struct contains information about a completed remeshing operation.
	 */
	struct QuadRemeshingResult
	{
		QuadRemeshingResult() :
		Success(false),
		IsAuthorized(true),
		TargetEdgeSize(-1.0f)
		{
		}

		bool Success;		  /**< True, if the operation was successful. */
		bool IsAuthorized;	  /**< True, if the host was properly authorized during the operation. */
		float TargetEdgeSize; /**< The target edge size in world-space units. */
	};

	typedef void (*pfnQuadRemeshingProgressCallback)(class IQuadRemeshingOperation* operation, IInstaLODPolygonMesh* outputMesh, const float progressInPercent);
	/**
	 * The IQuadRemeshingOperation interface represents a quad remeshing operation.
	 */
	class IQuadRemeshingOperation
	{
	protected:
		virtual ~IQuadRemeshingOperation() {}

	public:
		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnQuadRemeshingProgressCallback callback) = 0;

		/**
		 * Performs the operation.
		 * @note the input mesh and the output mesh can be the same instance.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return unwrap result
		 */
		virtual QuadRemeshingResult Execute(const IInstaLODMesh* inputMesh, IInstaLODPolygonMesh* outputMesh, const QuadRemeshingSettings& settings) = 0;
	};

	/**
	 * The RemeshSurfaceMode enumeration specifies different types of
	 * surface construction used by the IRemeshingOperation.
	 */
	namespace VoxelizeAlgorithm
	{
		enum Type
		{
			Version0 = 0,		/**< The V0 algorithm. */
			Version1 = 1,		/**< The V1 algorithm. @note V1 is disabled in this build of the SDK. */
			Default = Version0, /**< The default algorithm. */
			Count = 2
		};
	}

	/**
	 * The VoxelizeSettings structure contains settings exposed by the voxelize API.
	 */
	struct VoxelizeSettings
	{
		VoxelizeSettings() :
		Resolution(128u),
		Algorithm(VoxelizeAlgorithm::Default)
		{
		}

		uint32 Resolution;				   /**< When 'Mode' is set to 'Reconstruct', the resolution of the surface construction in range [16...4096].
											The resolution is ^3 so maximum memory pressure and processing time grows exponentially. */
		VoxelizeAlgorithm::Type Algorithm; /**< The algorithm used for the voxelization. */
	};

	/**
	 * The VoxelizeResult struct contains information about a completed voxelize operation.
	 */
	struct VoxelizeResult
	{
		VoxelizeResult() :
		Success(false),
		IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	typedef void (*pfnVoxelizeProgressCallback)(class IVoxelizeOperation* operation, const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const float progressInPercent);

	/**
	 * The IVoxelizeOperation interface represents a voxelize operation.
	 */
	class IVoxelizeOperation
	{
	protected:
		virtual ~IVoxelizeOperation() {}

	public:
		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnVoxelizeProgressCallback callback) = 0;

		/**
		 * Performs the operation.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return result
		 */
		virtual VoxelizeResult Execute(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const VoxelizeSettings& settings) = 0;
	};

	/**
	 * The CreateSignedDistanceFieldSettings structure contains the settings for extracting a signed distance field.
	 */
	struct CreateSignedDistanceFieldSettings
	{
		CreateSignedDistanceFieldSettings()
			: Resolution(32u)
			, DenseResolution(4u)
			, NarrowBandWidth(1.01f)
			, BoundingBoxEnlargementFactor(0.1f)
			, DistanceComparisonToleranceRelative(1.0e-5f)
			, HardAngleThreshold(70.0f /*degrees*/)
			, EnableDenseResolutionInterpolationFromCoarserDistanceFieldLevel(false)
			, UseAccurateDistances(true)
		{
		}

		uint32 Resolution;													  /**< The resolution of the signed distance field for the longest axis of the axis-aligned bounding box. Only the narrow band gets created at this resolution. If this is not a power of two it gets rounded up to the next power of two. */
		uint32 DenseResolution;												  /**< The octree resolution that gets made dense so that the distance field data can be sampled outside of the narrow band. This can be resolution at the leaf level, or a coarser resolution that is a power of two. */
		float NarrowBandWidth;												  /**< The one-sided size of the narrow band relative to the diagonal of the signed distance field cells. Setting the narrow band width to 1.0 + epsilon (= cell diagonal + epsilon) makes sure that there is always a cell above and a cell below a face. If the distance field does not need to be downsampled, it is sufficient to use 0.5 + epsilon to get better performance. */
		float BoundingBoxEnlargementFactor;									  /**< The factor by which each direction of the mesh bounding gets enlarged in order to create the bounding box for the signed distance field. A value of 0.01 increases the value by 1 percent. */
		float DistanceComparisonToleranceRelative;							  /**< The tolerance with which distances get compared for equality while taking into account rounding errors. The absolute value is this relative value times the average absolute value of both distances. */
		float HardAngleThreshold;											  /**< The hard angle threshold in degrees for detection of sharp edges. */
		bool EnableDenseResolutionInterpolationFromCoarserDistanceFieldLevel; /**< The flag to enable the experimental option to calculate the distances of the dense distance field level that are far away from the narrow band from a coarser dense distance field. */
		bool UseAccurateDistances;											  /**< The flag to to select between accurate distances from cell center to nearest face (the default) and the experimental preservation of features at the cost of accurate distances. */
	};

	/**
	 * The CreateSignedDistanceFieldResult structure contains information about a create signed distance field operation.
	 */
	struct CreateSignedDistanceFieldResult
	{
		CreateSignedDistanceFieldResult()
			: Success(false)
			, IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	typedef void (*pfnCreateSignedDistanceFieldProgressCallback)(class ICreateSignedDistanceFieldOperation* operation, const IInstaLODMesh* inputMesh, IInstaLODDistanceField* outputSignedDistanceField, const float progressInPercent);

	class ICreateSignedDistanceFieldOperation
	{
	protected:
		virtual ~ICreateSignedDistanceFieldOperation() {}

	public:
		/**
		 * Sets the progress callback.
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback The progress callback.
		 */
		virtual void SetProgressCallback(pfnCreateSignedDistanceFieldProgressCallback callback) = 0;

		/**
		 * Creates a signed distance field from a mesh.
		 *
		 * @param inputMesh The mesh.
		 * @param outputSignedDistanceField The signed distance field.
		 * @param settings The settings.
		 * @return The result.
		 */
		virtual CreateSignedDistanceFieldResult Execute(const IInstaLODMesh* inputMesh, IInstaLODDistanceField* outputSignedDistanceField, const CreateSignedDistanceFieldSettings& settings) = 0;
	};

	/**
	 * The SurfaceExtractionSettings struct contains the settings for the surface extraction operation.
	 */
	struct SurfaceExtractionSettings
	{
		SurfaceExtractionSettings()
			: ExtractionLevel(2u)
			, IsoValue(0.0f)
		{
		}

		uint32 ExtractionLevel; /**< The maximum octree level for surface extraction. */
		float IsoValue;			/**< The iso value at which the surface gets extracted. */
	};

	/**
	 * The SurfaceExtractionResult struct contains information about a completed surface extraction.
	 */
	struct SurfaceExtractionResult
	{
		SurfaceExtractionResult()
			: Success(false)
			, IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	typedef void (*pfnSurfaceExtractionProgressCallback)(class ISurfaceExtractionOperation* operation, const IInstaLODDistanceField* distanceField, IInstaLODMesh* outputMesh, const float progressInPercent);

	/**
	 * The ISurfaceExtractionOperation class provides the interface to extract a surface mesh from a distance field.
	 */
	class ISurfaceExtractionOperation
	{
	protected:
		virtual ~ISurfaceExtractionOperation() = default;

	public:
		/**
		 * Sets the progress callback.
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback The progress callback.
		 */
		virtual void SetProgressCallback(pfnSurfaceExtractionProgressCallback callback) = 0;

		/**
		 * Performs a mesh surface extraction.
		 *
		 * @param distanceField The distance field.
		 * @param settings The extraction settings.
		 * @param outExtractedMesh The mesh in which the extracted surface gets stored.
		 * @return The MeshExtractionResult structure.
		 */
		virtual SurfaceExtractionResult Execute(const IInstaLODDistanceField& distanceField, const SurfaceExtractionSettings& settings, IInstaLODMesh* outExtractedMesh) = 0;
	};

	/**
	 * The ImposterType enumeration specifies different types of imposters
	 * that can be created by the IImposterizeOperation.
	 */
	namespace ImposterType
	{
		enum Type
		{
			AABB = 0,				  /**< AABB imposter. */
			Billboard = 1,			  /**< Billboard imposter with configurable quads per axis. */
			HybridBillboardCloud = 2, /**< Hybrid billboard cloud imposter is capable of rendering with self shadowing, ambient occlusion and transmissive lighting. */
			Flipbook = 3,			  /**< The flipbook imposter is a matrix consisting of sprites where, columns represent rotations around the Z axis, and rows represent rotations around the X axis. */
			CustomGeometry = 4,		  /**< Imposter based on user specified geometry. */
			Count = 5
		};
	}

	/**
	 * The ImposterizeSettings structure contains settings exposed by the imposterize API.
	 */
	struct ImposterizeSettings
	{
		ImposterizeSettings() :
		Type(ImposterType::Billboard),
		FlipbookFramesPerAxis(8),
		CustomGeometry(nullptr),
		AABBDisplacement(0.0f),
		QuadXYCount(1),
		QuadXZCount(1),
		QuadYZCount(0),
		QuadXYOffset(0.0f, 0.0f, 0.0f),
		QuadXZOffset(0.0f, 0.0f, 0.0f),
		QuadYZOffset(0.0f, 0.0f, 0.0f),
		EnableQuadTwoSided(true),
		QuadSubdivisionsU(1),
		QuadSubdivisionsV(1),
		CloudFaceCount(700),
		CloudPolyFaceFactor(0.5f),
		EnableCloudQuadXY(true),
		EnableCloudQuadXZ(false),
		EnableCloudQuadYZ(true),
		EnableCloudQuadTwoSided(false),
		CloudNormal(0.35f, 0.85f, 0.4f),
		CloudNormalConformity(0.5f),
		Orientation(0.0f, 0.0f, 0.0f, 1.0f),
		AABBPadding(0.0f),
		AlphaMaskThreshold(1.0f),
		GutterSizeInPixels(5),
		AlphaCutOut(false),
		AlphaCutOutResolution(16),
		AlphaCutOutSubdivide(false),
		BakeEngine(BakeEngine::CPU),
		BakeOutput(),
		Deterministic(false)
		{
		}

		ImposterType::Type Type; /**< The imposter type. */

		uint32 FlipbookFramesPerAxis; /**< The amount of flipbook frames to generate for each rotation axis.
										 @note By default the flipbook's top left frame is projected along the negative Z axis.
										 @note Use the Orientation field to rotate the view matrix. */

		IInstaLODMesh* CustomGeometry; /**< Custom geometry used as imposter geometry. Custom geometry should be unwrapped on TC0 with valid normals. */

		float AABBDisplacement; /**< AABB imposter faces will be displaced along the face normal by the specified value in world space units.
										 This is useful to create hay or hedges with overhanging faces. */

		uint32 QuadXYCount;		  /**< The amount of quads to spawn for the XY axis of the input geometry AABB. Additional quads will be rotated by 180°/Count along the Y axis. */
		uint32 QuadXZCount;		  /**< The amount of quads to spawn for the XZ axis of the input geometry AABB. Additional quads will be rotated by 180°/Count along the Z axis. */
		uint32 QuadYZCount;		  /**< The amount of quads to spawn for the YZ axis of the input geometry AABB. Additional quads will be rotated by 180°/Count along the Z axis. */
		InstaVec3F QuadXYOffset;  /**< Offset for the XY axis imposter quads. By default imposter quads are centered on the input geometry AABB center.  */
		InstaVec3F QuadXZOffset;  /**< Offset for the XZ axis imposter quads. By default imposter quads are centered on the input geometry AABB center.  */
		InstaVec3F QuadYZOffset;  /**< Offset for the YZ axis imposter quads. By default imposter quads are centered on the input geometry AABB center.  */
		bool EnableQuadTwoSided;  /**< If enabled, quads will be generated with polygons for each side of a quad. */
		uint32 QuadSubdivisionsU; /**< Subdivisions along the U axis for billboard imposters. */
		uint32 QuadSubdivisionsV; /**< Subdivisions along the V axis for billboard imposters. */

		uint32 CloudFaceCount;		  /**< The amount of faces for the billboard cloud. */
		float CloudPolyFaceFactor;	  /**< The amount of faces in percentage of the CloudFaceCount to allocate for the polygon part of hybrid billboard cloud imposters. */
		bool EnableCloudQuadXY;		  /**< Spawn XY quads for cloud imposters. */
		bool EnableCloudQuadXZ;		  /**< Spawn XZ quads for cloud imposters. */
		bool EnableCloudQuadYZ;		  /**< Spawn YZ quads for cloud imposters. */
		bool EnableCloudQuadTwoSided; /**< If enabled, billboard cloud quads will be generated with polygons for each side of a quad. */
		InstaVec3F CloudNormal;		  /**< The face normal to apply to cloud imposters, used CloudNormalConformity to control strength. */
		float CloudNormalConformity;  /**< Controls the conformity of the cloud imposters to the CloudNormal. */

		InstaQuaternionF Orientation; /**< Orientation of the imposter. @note Applied prior to rendering the textures. */
		float AABBPadding;			  /**< Worldspace padding that is applied to the input geometry AABB. */
		float AlphaMaskThreshold;	  /**< Alpha mask values below this threshold will be considered transparent. */
		uint32 GutterSizeInPixels;	  /**< The minimum distance between two UV shells in pixels. */

		bool AlphaCutOut;			  /**< Removes transparent areas by cutting out the opaque shape. */
		uint32 AlphaCutOutResolution; /**< The resolution for the alpha cutout. */
		bool AlphaCutOutSubdivide;	  /**< Subdivides the cutout result based on the AlphaCutResolution. */

		BakeEngine::Type BakeEngine;   /**< The bake engine. If the selected bake engine is not available, InstaLOD will revert to the CPU baker. */
		BakeOutputSettings BakeOutput; /**< Bake output settings. */
		bool Deterministic;			   /**< Makes the algorithm deterministic at the cost of speed. */
	};

	/**
	 * The ImposterizeResult struct contains information about a completed imposterize operation.
	 */
	struct ImposterizeResult
	{
		ImposterizeResult() :
		Success(false),
		IsAuthorized(true),
		BakeMaterial(nullptr)
		{
		}

		bool Success;					 /**< True, if the operation was successful. */
		bool IsAuthorized;				 /**< True, if the host was properly authorized during the operation. */
		IInstaLODMaterial* BakeMaterial; /**< The generated material. Valid until the operation is deallocated. */
	};

	typedef void (*pfnImposterizeProgressCallback)(class IImposterizeOperation* operation, IInstaLODMesh* outputMesh, const float progressInPercent);

	/**
	 * The IImposterizeOperation interface represents an imposterize operation.
	 */
	class IImposterizeOperation
	{
	protected:
		virtual ~IImposterizeOperation(){};

	public:
		/**
		 * Adds a mesh to this instance
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The IDs are created in a sequential fashion starting at 0.
		 *
		 * @param mesh the mesh data
		 * @return true if the mesh has been added successfully
		 */
		virtual bool AddMesh(const IInstaLODMesh* mesh) = 0;

		/**
		 * Adds a mesh to this instance that is used as a polygonal mesh when creating billboard cloud hybrid imposters.
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The IDs are created in a sequential fashion starting at 0.
		 *
		 * @param mesh the mesh data
		 * @return true if the mesh has been added successfully
		 */
		virtual bool AddCloudPolygonalMesh(const IInstaLODMesh* mesh) = 0;

		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnImposterizeProgressCallback callback) = 0;

		/**
		 * Sets the material data used for this operation.
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The material data should contain a material for each referenced material ID.
		 * @note Each material should contain at least one texture page. Texture pages that should be outputted
		 * on the same texture page should have identical names.
		 *
		 * @param data the material data
		 */
		virtual void SetMaterialData(IInstaLODMaterialData* data) = 0;

		/**
		 * Sets the source sub mesh world transform.
		 * @note If the world transform is not specified, the ObjectSpace texture pages
		 * will be treated as WorldSpaceNormals.
		 * @note This feature is not supported for HybridBillboardCloud imposter generation.
		 *
		 * @param sourceSubMeshID The ID of the target sub mesh
		 * @param transform The world transform of the submesh at \p sourceSubMeshID
		 */
		virtual void SetSourceSubMeshWorldTransform(const uint32 sourceSubMeshID, const InstaMatrix4D& transform) = 0;

		/**
		 * Sets the target mesh world transform.
		 * @note If the world transform is not specified, the ObjectSpace sampler
		 * will produce WorldSpaceNormals.
		 *
		 * @param transform The world transform of the submesh at \p targetSubMeshID
		 */
		virtual void SetTargetMeshWorldTransform(const InstaMatrix4D& transform) = 0;

		/**
		 * Performs the operation.
		 *
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return imposterize result
		 */
		virtual ImposterizeResult Execute(IInstaLODMesh* outputMesh, const ImposterizeSettings& settings) = 0;
	};

	/**
	 * The SkeletonOptimizeResult struct contains information about a completed skeleton optimization operation.
	 */
	struct SkeletonOptimizeResult
	{
		SkeletonOptimizeResult() :
		RemovedJointIndices(nullptr),
		RemovedJointCount(0),
		UnusedJointIndices(nullptr),
		UnusedJointCount(0),
		Success(false),
		IsAuthorized(true)
		{
		}

		uint32* RemovedJointIndices; /**< An array containing all joint indices that were removed. @note the pointer is valid until the operation is deallocated or executed again. */
		uint32 RemovedJointCount;	 /**< The amount of joints removed. */
		uint32* UnusedJointIndices;	 /**< An array containing all joint indices that are unused. @note the pointer is valid until the operation is deallocated or executed again. */
		uint32 UnusedJointCount;	 /**< The amount of unused joints. */
		bool Success;				 /**< True, if the operation was successful. */
		bool IsAuthorized;			 /**< True, if the host was properly authorized during the operation. */
	};

	/**
	 * The SkeletonOptimizeSettings structure contains settings exposed by the skeleton optimize API.
	 */
	struct SkeletonOptimizeSettings
	{
		SkeletonOptimizeSettings() :
		LeafBoneWeldDistance(0.0f),
		MaximumBoneDepth(0),
		MaximumBoneInfluencesPerVertex(0),
		MinimumBoneInfluenceThreshold(0.0f),
		IgnoreJointIndices(nullptr),
		IgnoreJointIndicesCount(0)
		{
		}

		float LeafBoneWeldDistance;			   /**< Leaf bone weld distance. Leaf bones with a distance to their parent under the specified threshold will be culled. */
		uint32 MaximumBoneDepth;			   /**< Bones that are below the specified level in the hierarchy will be removed. */
		uint32 MaximumBoneInfluencesPerVertex; /**< The maximum amount of bone influences per vertex. If a vertex references more bones than allowed, the lowest bone influences will be removed first. */
		float MinimumBoneInfluenceThreshold;   /**< The minimum threshold for bone influences. Influences that fall below the threshold will be removed. */
		uint32* IgnoreJointIndices;			   /**< Optional pointer to an array of joint indices that will be not be removed. */
		uint32 IgnoreJointIndicesCount;		   /**< If IgnoreJointIndices is set to an array of joint indices, set this field to the array size. */
	};

	typedef void (*pfnSkeletonOptimizeProgressCallback)(class ISkeletonOptimizeOperation* operation, const IInstaLODSkeleton* skeleton, const IInstaLODMesh* sourceMesh, IInstaLODMesh* targetMesh, const float progressInPercent);

	/**
	 * The ISkeletonOptimizeOperation interface represents an skeleton optimize operation.
	 */
	class ISkeletonOptimizeOperation
	{
	protected:
		virtual ~ISkeletonOptimizeOperation() {}

	public:
		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnSkeletonOptimizeProgressCallback callback) = 0;

		/**
		 * Performs the operation.
		 * @note the input mesh and the output mesh can be the same instance.
		 *
		 * @param skeleton the skeleton
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return skeleton optimize result
		 */
		virtual SkeletonOptimizeResult Execute(const IInstaLODSkeleton* skeleton, const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const SkeletonOptimizeSettings& settings) = 0;
	};

	/**
	 * The AlgorithmStrategy represents different types of strategies used by the optimizer.
	 */
	namespace AlgorithmStrategy
	{
		enum Type
		{
			Default = 0,		   /**< Fast optimizer that respects mesh attributes. */
			IgnoreAttributes = 1,  /**< Very fast optimizer that ignores mesh attributes. */
			DiscardAttributes = 2, /**< Extremely fast and memory efficient optimizer that discards all mesh attributes. */
			Smart_v1_Legacy = 3,   /**< (Legacy) AI based smart optimizer that dynamically adjusts it's optimization strategy for best results. */
			Smart_v2 = 4,		   /**< (Default, Recommended) Fast and efficient AI based smart optimizer that dynamically adjusts it's optimization strategy for best results. */
			Count = 5
		};
	}

	/**
	 * The NormalHealingMode enumeration represents different normal healing modes used by the optimizer.
	 */
	namespace NormalHealingMode
	{
		enum Type
		{
			Off = 0,
			Minimal = 1,
			Default = 2,
			Count = 3
		};
	}

	/**
	 * The OptimizeResult struct contains information about a completed mesh optimization operation.
	 */
	struct OptimizeResult
	{
		OptimizeResult() :
		Success(false),
		IsAuthorized(true),
		MeshDeviation(-1.0f)
		{
		}

		bool Success;		 /**< True, if the operation was successful. */
		bool IsAuthorized;	 /**< True, if the host was properly authorized during the operation. */
		float MeshDeviation; /**< The mesh deviation from the input mesh. */
	};

	/**
	 * The OptimizeSettings structure contains settings exposed by the optimizer API.
	 * @note For best-mode results, it is recommended to keep the importance settings on the default values unless required.
	 */
	struct OptimizeSettings
	{
		OptimizeSettings() :
		AlgorithmStrategy(AlgorithmStrategy::Smart_v2),
		AutomaticQuality(MeshFeatureImportance::Off),
		PercentTriangles(1.0f),
		AbsoluteTriangles(0u),
		MaxDeviation(0.0f),
		WeldingThreshold(0.0f),
		WeldingProtectDistinctUVShells(true),
		WeldingNormalAngleThreshold(90.0f),
		HealTJunctionThreshold(0.0f),
		ScreenSizeInPixels(0u),
		OptimizerVertexWeights(false),
		OptimalPlacement(true),
		RecalculateNormals(false),
		HardAngleThreshold(70.0f),
		WeightedNormals(true),
		NormalHealingMode(NormalHealingMode::Default),
		LockBoundaries(false),
		LockSplits(false),
		ProtectSplits(true),
		ProtectBoundaries(true),
		UnitScaleFactor(1.0f),
		NormalizeMeshScale(false),
		SkeletonOptimize(),
		Skeleton(nullptr),
		BoundaryImportance(MeshFeatureImportance::Normal),
		TextureImportance(MeshFeatureImportance::Normal),
		ShadingImportance(MeshFeatureImportance::Normal),
		SilhouetteImportance(MeshFeatureImportance::Normal),
		SkinningImportance(MeshFeatureImportance::Normal),
		Deterministic(false)
		{
		}

		AlgorithmStrategy::Type AlgorithmStrategy;	  /**< The algorithm strategy used by the polygon optimization operation. */
		MeshFeatureImportance::Type AutomaticQuality; /**< Automatically adjusts the quality of the mesh. If enabled, no other option to control the target polygon count is available. */
		float PercentTriangles;						  /**< The amount of polygons in percent of the output mesh in relation to the input mesh polygon count. */
		uint32 AbsoluteTriangles;					  /**< The absolute amount of polygons for the target mesh. If set, this value precedes PercentTriangles. */
		float MaxDeviation;							  /**< Controls the maximum allowed mesh error. The optimizer stops before exceeding this value. */
		float WeldingThreshold;						  /**< If set to a value > 0, automatically welds vertices within the specified distance. */
		bool WeldingProtectDistinctUVShells;		  /**< Prevents welding of UVs that are not in the same UV shell. */
		float WeldingNormalAngleThreshold;			  /**< Prevents welding of normals if the angle between the welded normal and the wedge normal is above the specified value in degrees. */
		float HealTJunctionThreshold;				  /**< If set to a value > 0, automatically heals T-Junctions if the vertex-edge distance is below the specified threshold. */
		uint32 ScreenSizeInPixels;					  /**< If set calculates the amount of polygons to remove based on the display size of the output model. If set, this value precedes PercentTriangles. */
		bool OptimizerVertexWeights;				  /**< Enables the use of optimizer vertex weights of the input mesh.
													Optimizer weights are set on in the input mesh where -1 means the vertex will be removed immediately and 1 means the vertex will not be optimized. */
		bool OptimalPlacement;						  /**< Enable optimal placement of vertices to reduce mesh error. If disabled vertices will always move along edges. */
		bool RecalculateNormals;					  /**< Recalculate normals of the output mesh. */
		float HardAngleThreshold;					  /**< When recalculating normals: smooth faces if the normal angle is below this value (in degrees). */
		bool WeightedNormals;						  /**< When recalculating normals: smoothed normals are weighted by various geometric properties. */
		NormalHealingMode::Type NormalHealingMode;	  /**< Invalid normals that can occur during optimization can automatically be fixed by InstaLOD.
													 @note When normals are distorted to produce custom shading effects, it can be desirable to set normal healing to minimal. */
		bool LockBoundaries;						  /**< Prevents optimization of vertices on the mesh boundary. This is useful if the mesh contains holes that need to be preserved as is. */
		bool LockSplits;							  /**< Prevents optimization of split vertices. This is useful if uv and normal splits need to be preserved as is. */
		bool ProtectSplits;							  /**< Prevents topological violations. Disabling can cause holes, but can enable a better optimization for low poly models.
														Disabling 'ProtectSplits' when using the Smart Optimizer allows InstaLOD to toggle split protection dynamically during optimization. */
		bool ProtectBoundaries;						  /**< Prevents vertices on mesh boundaries from moving into the surface.
														When optimizing surfaces like trees or foliage without optimizer weights, it is recommended to disable 'ProtectBoundaries'. */
		float UnitScaleFactor;						  /**< If AlgorithmStrategy is Smart_v2: The unit system of the optimizer is centimeters.
														Therefore, 100 scene units is equal to 1 meter. If your scene or geometry is built in another scale,
														it is highly recommended to use the UnitScaleFactor field to ensure both systems match.
														If your scene is built in a scale where 1 scene-unit equals 1 meter, set the UnitScaleFactor to 100.
														If your scene is built in a scale where 1 scene-unit equals 1 millimeter, set the UnitScaleFactor to 0.1.
														This is not required, but best results are achieved when both unit systems match. */
		bool NormalizeMeshScale;					  /**< If AlgorithmStrategy is Smart_v2: Enables normalization of the mesh scale to fit a 250u bounding sphere.
														When using NormalizeMeshScale, MaxDeviation is specified as a percentage value.
														This should be enabled when automatically processing data with unpredictable scene scales.
														Otherwise, it is recommended to use UnitScaleFactor. Best quality is achieved when the scene scale is matching the optimizer's unit system. */

		SkeletonOptimizeSettings SkeletonOptimize; /**< Skeleton optimization settings. */
		IInstaLODSkeleton* Skeleton;			   /**< (optional) Skeleton used for the input mesh. Skeletal meshes can be optimized even when no skeleton is specified. Skinning weights will be
														properly optimized and can influence the optimization via importance settings. However, skeleton optimization (e.g. by removing bones) is only
														available when a skeleton is specified. */

		MeshFeatureImportance::Type BoundaryImportance;	  /**< Determines importance of weights generated by evaluating the boundaries on the geometry. */
		MeshFeatureImportance::Type TextureImportance;	  /**< Determines importance of weights generated by evaluating the UV space. */
		MeshFeatureImportance::Type ShadingImportance;	  /**< Determines importance of weights generated by evaluating the tangent space. */
		MeshFeatureImportance::Type SilhouetteImportance; /**< Determines importance of weights generated by evaluating the geometry. */
		MeshFeatureImportance::Type SkinningImportance;	  /**< Determines importance of weights generated by evaluating the skeleton and vertex bone influences.
																@note if no skeleton and no skinning data has been specified, this field does not influence the optimization. */
		bool Deterministic;								  /**< Makes the algorithm deterministic at the cost of speed. */
	};

	typedef void (*pfnOptimizationProgressCallback)(class IOptimizeOperation* operation, const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const float progressInPercent, const uint32 currentPolygonCount, const uint32 targetPolygonCount);

	/**
	 * The IOptimizeOperation interface represents an optimize operation.
	 */
	class IOptimizeOperation
	{
	protected:
		virtual ~IOptimizeOperation() {}

	public:
		/**
		 * Sets the progress callback.
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnOptimizationProgressCallback callback) = 0;

		/**
		 * Performs the optimization operation.
		 * @note the input mesh and the output mesh can be the same instance.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return optimize result
		 */
		virtual OptimizeResult Execute(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const OptimizeSettings& settings) = 0;
	};

	/**
	 * The UnwrapResult struct contains information about a completed mesh unwrap operation.
	 */
	struct UnwrapResult
	{
		UnwrapResult() :
		Success(false),
		IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	/**
	 * The UnwrapSettings structure contains settings exposed by the unwrap API.
	 */
	struct UnwrapSettings
	{
		UnwrapSettings() :
		UnwrapStrategy(UnwrapStrategy::Auto),
		TextureWidth(1024),
		TextureHeight(1024),
		GutterSizeInPixels(5),
		StretchImportance(MeshFeatureImportance::Normal),
		TexCoordIndexOutput(0),
		ShellStitching(true),
		InsertNormalSplits(true),
		UVScale(1.0f),
		PostProcessLayout(true),
		CustomHardSurfaceAxialNormals(nullptr),
		CustomHardSurfaceAxialNormalCount(0),
		Deterministic(false)
		{
		}

		UnwrapStrategy::Type UnwrapStrategy;		   /**< The unwrap strategy used by the unwrap operation. */
		uint32 TextureWidth;						   /**< The output texture width. */
		uint32 TextureHeight;						   /**< The output texture height. */
		uint32 GutterSizeInPixels;					   /**< The minimum distance between two UV shells in pixels. @note The texture size must be setup for this to work reliably. */
		MeshFeatureImportance::Type StretchImportance; /**< The importance of UV space stretch during the unwrap. Off: high levels of stretch allowed, Highest: no stretching allowed.
															Stretch impacts the UV-shell and vertex split count. Allowing for higher stretch values means less UV-shells generated. */
		uint32 TexCoordIndexOutput;					   /**< The UV channel to write the UV data to. */
		bool ShellStitching;						   /**< When 'UnwrapStrategy' is either 'HardSurfaceAxial' or 'HardSurfaceAngle', disables stitching of shells. */
		bool InsertNormalSplits;					   /**< Insert UV shell splits for edges that do not share the same wedge normals. */
		float UVScale;								   /**< A scale factor to apply to the unwrap. */
		bool PostProcessLayout;						   /**< [Preview] Performs a post-process to minimize the UV shell count. */

		const InstaVec3F* CustomHardSurfaceAxialNormals; /**< Optional pointer to an array of plane normals used when using HardSurfaceAxial unwrap strategy. */
		uint32 CustomHardSurfaceAxialNormalCount;		 /**< If CustomHardSurfaceAxialNormals is set to an array of plane normals, set this field to the array size. */
		bool Deterministic;								 /**< Makes the algorithm deterministic at the cost of speed. */
	};

	typedef void (*pfnUnwrapProgressCallback)(class IUnwrapOperation* operation, const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const float progressInPercent);

	/**
	 * The IUnwrapOperation interface represents an unwrap operation.
	 */
	class IUnwrapOperation
	{
	protected:
		virtual ~IUnwrapOperation() {}

	public:
		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnUnwrapProgressCallback callback) = 0;

		/**
		 * Performs the operation.
		 * @note the input mesh and the output mesh can be the same instance.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return unwrap result
		 */
		virtual UnwrapResult Execute(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const UnwrapSettings& settings) = 0;
	};

	/**
	 * The UVPackResult struct contains information about a completed mesh unwrap operation.
	 */
	struct UVPackResult
	{
		UVPackResult() :
		Success(false),
		IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	/**
	 * The UVPackShellRotation represents different types of shell rotations used by the UV packer.
	 */
	namespace UVPackShellRotation
	{
		enum Type
		{
			None = 0,	   /**< Do not rotate shells. */
			Allow90 = 1,   /**< Allow 90 degree shell rotations when packing. */
			Arbitrary = 2, /**< Allow arbitrary rotations when packing. */
			Count = 3
		};
	}

	/**
	 * The UVPackSettings structure contains settings exposed by the UV pack API.
	 */
	struct UVPackSettings
	{
		UVPackSettings() :
		TextureWidth(1024),
		TextureHeight(1024),
		GutterSizeInPixels(5),
		TexCoordIndexInput(0),
		TexCoordIndexOutput(0),
		ShellRotation(UVPackShellRotation::Arbitrary),
		StackDuplicateShells(true),
		InsertNormalSplits(true),
		Deterministic(false)
		{
		}

		uint32 TextureWidth;		/**< The output texture width. */
		uint32 TextureHeight;		/**< The output texture height. */
		uint32 GutterSizeInPixels;	/**< The minimum distance between two UV shells in pixels. @note The texture size must be setup for this to work reliably. */
		uint32 TexCoordIndexInput;	/**< The UV channel to repack. */
		uint32 TexCoordIndexOutput; /**< The UV channel to write the packed data to. */

		UVPackShellRotation::Type ShellRotation; /**< Enable to allow shells to be rotated for improved packing. Disabling rotations can cause certain scenarios to be unpackable. */
		bool StackDuplicateShells;				 /**< Places duplicate UV shells referencing the same material ID on top of each other to save UV space. */
		bool InsertNormalSplits;				 /**< Insert UV shell splits for edges that do not share the same wedge normals. */
		bool Deterministic;						 /**< Makes the algorithm deterministic at the cost of speed. */
	};

	typedef void (*pfnUVPackProgressCallback)(class IUVPackOperation* operation, const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const float progressInPercent);

	/**
	 * The IUVPackOperation interface represents an UV chart repacking operation.
	 */
	class IUVPackOperation
	{
	protected:
		virtual ~IUVPackOperation() {}

	public:
		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnUVPackProgressCallback callback) = 0;

		/**
		 * Performs the operation.
		 * @note the input mesh and the output mesh can be the same instance.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return uv pack result
		 */
		virtual UVPackResult Execute(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const UVPackSettings& settings) = 0;
	};

	/**
	 * The MeshMergeResult struct contains information about a completed mesh merge operation.
	 */
	struct MeshMergeResult
	{
		MeshMergeResult() :
		Success(false),
		IsAuthorized(true),
		MergeMaterial(nullptr)
		{
		}

		bool Success;					  /**< True, if the operation was successful. */
		bool IsAuthorized;				  /**< True, if the host was properly authorized during the operation. */
		IInstaLODMaterial* MergeMaterial; /**< The merged material. Valid until the operation is deallocated. */
	};

	/**
	 * The MeshMergeMode represents different types of merge modes used by the mesh merge operation.
	 */
	namespace MeshMergeMode
	{
		enum Type
		{
			AutoRepack = 0, /**< Automatically repacks input UV channel based on feature importance settings. */
			Transfer = 1,	/**< Transfer texture data from input UV channel to output UV channel without modifying output UV channel. */
			Count = 2
		};
	}

	/**
	 * The MeshMergeSettings structure contains settings exposed by the mesh merge API.
	 */
	struct MeshMergeSettings
	{
		MeshMergeSettings() :
		Mode(MeshMergeMode::AutoRepack),
		SuperSampling(SuperSampling::X2),
		TextureFilter(TextureFilter::Bilinear),
		SolidifyTexturePages(true),
		StackDuplicateShells(true),
		InsertNormalSplits(true),
		ComputeBinormalPerFragment(false),
		NormalizeTangentSpacePerFragment(false),
		WorldspaceNormalizeShells(true),
		GutterSizeInPixels(5),
		TexCoordIndexInput(0),
		TexCoordIndexOutput(0),
		ShellRotation(UVPackShellRotation::Arbitrary),
		GenerateZeroAreaUV(false),
		ZeroAreaUVThreshold(1e-10f),
		UVImportance(MeshFeatureImportance::Normal),
		GeometricImportance(MeshFeatureImportance::Normal),
		TextureImportance(MeshFeatureImportance::Normal),
		VisualImportance(MeshFeatureImportance::Normal),
		Deterministic(false)
		{
		}

		MeshMergeMode::Type Mode;				 /**< Mesh merge mode. */
		SuperSampling::Type SuperSampling;		 /**< Super sampling. */
		TextureFilter::Type TextureFilter;		 /**< Texture filter used when sampling texture. */
		bool SolidifyTexturePages;				 /**< Enables solidification of texture pages to avoid pixel bleed. */
		bool StackDuplicateShells;				 /**< Places duplicate UV shells referencing the same material ID on top of each other to save UV space. */
		bool InsertNormalSplits;				 /**< Insert UV shell splits for edges that do not share the same wedge normals. */
		bool ComputeBinormalPerFragment;		 /**< This setting depends on how your renderer/engine computes the tangent basis. */
		bool NormalizeTangentSpacePerFragment;	 /**< This setting depends on how your renderer/engine computes the tangent basis. */
		bool WorldspaceNormalizeShells;			 /**< Normalize UV shells according to worldspace scale. */
		uint32 GutterSizeInPixels;				 /**< The minimum distance between two UV shells in pixels. @note The texture size must be setup for this to work reliably. */
		uint32 TexCoordIndexInput;				 /**< The input UV channel. */
		uint32 TexCoordIndexOutput;				 /**< The output UV channel. */
		UVPackShellRotation::Type ShellRotation; /**< Enable to allow shells to be rotated for improved packing. Disabling rotations can cause certain scenarios to be unpackable. */
		bool GenerateZeroAreaUV;				 /**< Creates a basic UV for 0-area triangle to allow copying solid color information. */
		float ZeroAreaUVThreshold;				 /**< UV faces with an area smaller or equal to this value will be considered zero area. */

		MeshFeatureImportance::Type UVImportance;		 /**< Determines importance of weights generated by evaluating occupied UV space. */
		MeshFeatureImportance::Type GeometricImportance; /**< Determines importance of weights generated by evaluating worldspace geometry. */
		MeshFeatureImportance::Type TextureImportance;	 /**< Determines importance of weights generated by evaluating input texture dimensions. */
		MeshFeatureImportance::Type VisualImportance;	 /**< Determines importance of weights generated by evaluating visual importance (raytraced). */
		bool Deterministic;								 /**< Makes the algorithm deterministic at the cost of speed. */
	};

	typedef void (*pfnMeshMergeProgressCallback)(class IMeshMergeOperation2* operation, IInstaLODMesh* outputMesh, const float progressInPercent);

	/**
	 * The IMeshMergeOperation interface represents a merge operation for
	 * meshes and materials.
	 */
	class IMeshMergeOperation2
	{
	protected:
		virtual ~IMeshMergeOperation2() {}

	public:
		/**
		 * Adds a mesh to this instance
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The IDs are created in a sequential fashion starting at 0.
		 *
		 * @param mesh the mesh data
		 * @return true if the mesh has been added successfully
		 */
		virtual bool AddMesh(const IInstaLODMesh* mesh) = 0;

		/**
		 * Sets the material data used for this operation.
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The material data should contain a material for each referenced material ID.
		 * @note Each material should contain at least one texture page. Texture pages that should be outputted
		 * on the same texture page should have identical names.
		 *
		 * @param data the material data
		 */
		virtual void SetMaterialData(IInstaLODMaterialData* data) = 0;

		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnMeshMergeProgressCallback callback) = 0;

		/**
		 * Performs the merge operation.
		 *
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return merge result
		 */
		virtual MeshMergeResult Execute(IInstaLODMesh* outputMesh, const MeshMergeSettings& settings) = 0;
	};

	typedef void (*pfnOcclusionCullProgressCallback)(class IOcclusionCullOperation* operation, const IInstaLODMesh* sourceMesh, IInstaLODMesh* targetMesh, const float progressInPercent);

	struct OcclusionCullResult
	{
		OcclusionCullResult() :
		Success(false),
		IsAuthorized(true),
		OccludedFaceCount(0),
		OccludedFacePercentage(0.0f)
		{
		}

		bool Success;				  /**< True, if the operation was successful. */
		bool IsAuthorized;			  /**< True, if the host was properly authorized during the operation. */
		uint32 OccludedFaceCount;	  /**< The amount of faces culled. */
		float OccludedFacePercentage; /**< The amount of faces culled in percent. */
	};

	/**
	 * The OcclusionCullingStrategy enum represents different types of strategies
	 * used by the occlusion operation.
	 */
	namespace OcclusionCullingStrategy
	{
		enum Type
		{
			Face = 0,						 /**< Cull occluded individual faces. */
			SubMeshByFaceSubMeshIndices = 1, /**< Cull occluded submeshes, submeshes derived from InstaLODMesh.FaceSubMeshIndices data. */
			SubMeshByFaceAdjacency = 2,		 /**< Cull occluded submeshes, submeshes derived from vertex adjacency. */
			Count = 3
		};
	}

	/**
	 * The OcclusionCullDataUsage enum represents different ways on how
	 * the computed occlusion data can be used by the occlusion operation.
	 */
	namespace OcclusionCullDataUsage
	{
		enum Type
		{
			RemoveGeometry = 0,						/**< Remove occluded geometry. */
			WriteWedgeColors = 1,					/**< Write wedge colors (black occluded, white visible). */
			WriteOptimizerWeights = 2,				/**< Write optimizer weights. */
			WriteOptimizerWeightsToWedgeColors = 3, /**< Write optimizer weights into the wedge color channel. */
			Count = 4
		};
	}

	/**
	 * The OcclusionCullMode enum represents different ways on how
	 * the occlusion culling will operate.
	 */
	namespace OcclusionCullMode
	{
		enum Type
		{
			AutomaticInterior = 0, /**< Automatically remove interior surfaces. This mode is designed to operate on individual objects, it is less suited for large scenes. */
			CameraBased = 1,	   /**< Cameras added to the operation will be used to determine occluded surfaces. */
			Count = 2
		};
	}

	/**
	 * The OcclusionCullCamera structure represents a camera instance as used by
	 * an occlusion cull operation.
	 */
	struct OcclusionCullCamera
	{
		OcclusionCullCamera() :
		Position(0.0f, 0.0f, 0.0f),
		Right(1.0f, 0.0f, 0.0f),
		Up(0.0f, 1.0f, 0.0f),
		Forward(0.0f, 0.0f, 1.0f),
		NearPlane(0.0f),
		FarPlane(1000.0f),
		FieldOfViewInDegrees(90.0f),
		IsOrthogonal(true),
		OrthogonalScale(250.0f),
		ResolutionX(1024),
		ResolutionY(1024)
		{
		}

		InstaVec3F Position;		/**< Camera position in world space. */
		InstaVec3F Right;			/**< Camera right vector. */
		InstaVec3F Up;				/**< Camera up vector. */
		InstaVec3F Forward;			/**< Camera forward vector. */
		float NearPlane;			/**< Near plane. */
		float FarPlane;				/**< Far plane. */
		float FieldOfViewInDegrees; /**< When 'IsOrthogonal' is false, the horizontal field of view in degrees. */
		bool IsOrthogonal;			/**< Determines whether this camera's projection is orthogonal. */
		float OrthogonalScale;		/**< The scale of the orthogonal projection. */
		uint32 ResolutionX;			/**< The X-axis resolution in pixels. */
		uint32 ResolutionY;			/**< The Y-axis resolution in pixels. */
	};

	/**
	 * The OcclusionCullSettings structure contains settings exposed by the occlusion cull API.
	 */
	struct OcclusionCullSettings
	{
		OcclusionCullSettings() :
		Mode(OcclusionCullMode::AutomaticInterior),
		CullingStrategy(OcclusionCullingStrategy::SubMeshByFaceAdjacency),
		DataUsage(OcclusionCullDataUsage::RemoveGeometry),
		AutomaticPrecision(MeshFeatureImportance::Normal),
		Resolution(1024),
		AdjacencyDepth(5),
		SubMeshVisbilityFaceThreshold(1),
		IgnoreBackface(false),
		AlphaMaskThreshold(1.0f),
		OptimizerWeight(-0.8f),
		Orientation(0.0f, 0.0f, 0.0f, 1.0f),
		RenderOutputPath(nullptr),
		Deterministic(false)
		{
		}

		OcclusionCullMode::Type Mode;					/**< The operation mode. */
		OcclusionCullingStrategy::Type CullingStrategy; /**< The culling strategy. */
		OcclusionCullDataUsage::Type DataUsage;			/**< The occlusion data usage. */
		MeshFeatureImportance::Type AutomaticPrecision; /**< When 'Mode' is set to 'AutomaticInterior': the precision of the automatic interior culling. */
		uint32 Resolution;								/**< The resolution of the rasterization. Higher resolutions improve quality by reducing the amount of missed faces due to subpixel inaccuracies.
													Dense geometry benefits from a higher resolution. However, a low resolution can be accommodated by increasing 'AdjacencyDepth' or 'AutomaticPrecision'. */
		uint32 AdjacencyDepth;							/**< When 'CullingStrategy' is set to 'Face': the recursion depth when protecting neighbors of visible faces.
													This can be used to prevent holes in outside geometry that occurred due to a low resolution or rotation count.
													0: all faces that are not visible during rasterization will be culled.
													1: direct neighbors of faces that are visible will also be marked visible.
													2..: neighbors of neighbors will also be marked visible. */
		uint32 SubMeshVisbilityFaceThreshold;			/**< A submesh is marked visible if it's visible face count is greater or equal this value. */
		bool IgnoreBackface;							/**< If enabled, back faces will not occlude geometry. When disabled, backfaces will occlude geometry. */
		float AlphaMaskThreshold;						/**< Alpha mask values below this threshold will be considered transparent. */
		float OptimizerWeight;							/**< When 'DataUsage' is set to 'WriteOptimizerWeights' or 'WriteOptimizerWeightsToWedgeColors':
													Optimizer weight value [-1...1] assigned to occluded faces. */
		InstaQuaternionF Orientation;					/**< Orientation of the scene. */

		const char* RenderOutputPath; /**< Optional, a valid path used to write the render frames to disk as PNG. Useful to validate camera settings. */
		bool Deterministic;			  /**< Makes the algorithm deterministic at the cost of speed. */
	};

	/**
	 * The IOcclusionCullOperation interface represents an occlusion culling operation for
	 * arbitrary input meshes.
	 */
	class IOcclusionCullOperation
	{
	protected:
		virtual ~IOcclusionCullOperation() {}

	public:
		/**
		 * Sets the material data used for this operation.
		 * @note The material data should contain information about all materials as referenced
		 * by the input geometry. The material IDs should match for both the material data and the input geometry.
		 * @note The material data should contain a material for each referenced material ID.
		 * @note Each material should contain at least one texture page. Texture pages that should be outputted
		 * on the same texture page should have identical names.
		 *
		 * @param data the material data
		 */
		virtual void SetMaterialData(IInstaLODMaterialData* data) = 0;

		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnOcclusionCullProgressCallback callback) = 0;

		/**
		 * Adds the specified camera to this instance.
		 *
		 * @param camera the camera
		 * @param name (optional) camera name used when rendering to a file
		 * @return the index of the camera
		 */
		virtual uint32 AddCamera(const OcclusionCullCamera& camera, const char* name) = 0;

		/**
		 * Gets the camera at the specified index.
		 *
		 * @param index the index of the camera
		 * @param outCamera (output) camera
		 * @return true if index was valid and camera has been written
		 */
		virtual bool GetCameraAtIndex(const uint32 index, OcclusionCullCamera& outCamera) const = 0;

		/**
		 * Removes the camera at the specified index.
		 *
		 * @param index the index of the camera
		 * @return true if index was valid and camera has been removed
		 */
		virtual bool RemoveCameraAtIndex(const uint32 index) = 0;

		/**
		 * Gets the amount of cameras stored in this instance.
		 *
		 * @return the amount of cameras
		 */
		virtual uint32 GetCameraCount() const = 0;

		/**
		 * Removes all cameras from this instance.
		 */
		virtual void RemoveAllCameras() = 0;

		/**
		 * Performs the occlusion culling operation.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return occlusion culling result
		 */
		virtual OcclusionCullResult Execute(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const OcclusionCullSettings& settings) = 0;
	};

	/**
	 * The ConstructiveSolidGeometryOperationType enum represents different
	 * modes of operation for a CSG operation.
	 */
	namespace ConstructiveSolidGeometryOperationType
	{
		enum Type
		{
			Union = 0,				 /**< Performs a union. */
			Subtract = 1,			 /**< Performs a subtraction. */
			Intersection = 2,		 /**< Performs a intersection. */
			SymmetricDifference = 3, /**< Performs a symmetric difference. */
			Count = 4
		};
	}

	/**
	 * The ConstructiveSolidGeometryResult struct contains information about a completed CSG operation.
	 */
	struct ConstructiveSolidGeometryResult
	{
		ConstructiveSolidGeometryResult() :
		Success(false),
		IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	/**
	 * The ConstructiveSolidGeometrySettings structure contains settings exposed by the CSG API.
	 */
	struct ConstructiveSolidGeometrySettings
	{
		ConstructiveSolidGeometrySettings() :
		OperationType(ConstructiveSolidGeometryOperationType::Union),
		Tolerance(1.0e-8)
		{
		}

		ConstructiveSolidGeometryOperationType::Type OperationType; /**< The type of the CSG operation. */
		double Tolerance;											/**< The tolerance for comparing floating point values. */
	};

	typedef void (*pfnConstructiveSolidGeometryProgressCallback)(class IConstructiveSolidGeometryOperation* operation, const IInstaLODMesh* meshA, const IInstaLODMesh* meshB, const float progressInPercent);

	/**
	* The IConstructiveSolidGeometryOperation interface represents a CSG operation for
	* arbitrary input meshes.
	*/
	class IConstructiveSolidGeometryOperation
	{
	protected:
		virtual ~IConstructiveSolidGeometryOperation() {}

	public:
		/**
		* Sets the progress callback
		* @note Progress callbacks can be called from child threads.
		*
		* @param callback the progress callback
		*/
		virtual void SetProgressCallback(pfnConstructiveSolidGeometryProgressCallback callback) = 0;

		/**
		* Performs a CSG operation on \p meshA and \p meshB.
		* @note Both meshA and meshB buffers can be used as \p outputMesh.
		*
		* @param meshA the A mesh
		* @param meshB the B mesh
		* @param settings the settings
		* @return CSG result
		*/
		virtual ConstructiveSolidGeometryResult Execute(const IInstaLODMesh* meshA, const IInstaLODMesh* meshB, IInstaLODMesh* outputMesh, const ConstructiveSolidGeometrySettings& settings) = 0;
	};

	/**
	 * The SubdivisionMode enum represents different ways on how
	 * the subdivision will operate.
	 */
	namespace SubdivisionMode
	{
		enum Type
		{
			CatmullClark = 0, /**< Uses the Catmull-Clark algorithm to subdivide smooth surfaces. */
			Simple = 1,		  /**< Simple subdivision of each polygon, assuming flat surfaces. */
			Count = 2
		};
	}

	/**
	 * The MeshToolKitResult struct contains information about a completed mesh toolkit operation.
	 */
	struct MeshToolKitResult
	{
		MeshToolKitResult() :
		Success(false),
		IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	/**
	 * The MeshToolKitSettings structure contains settings exposed by the mesh toolkit API.
	 */
	struct MeshToolKitSettings
	{
		MeshToolKitSettings() :
		WeldingThreshold(0.0f),
		WeldingNormalAngleThreshold(80.0f),
		WeldingBoundaries(false),
		HealTJunctionThreshold(0.0f),
		RemoveDegenerateFacesThreshold(0.0f),
		FixNonManifold(false),
		ConformNormals(false),
		ConformWindingOrder(false),
		FlipNormals(false),
		FlipWindingOrder(false),
		FillHoles(false),
		NormalHealingMode(NormalHealingMode::Off),
		RecalculateNormals(false),
		HardAngleThreshold(80.0f),
		WeightedNormals(true),
		MinimumSubMeshBoundingSphereRadius(0.0f),
		MinimumFaceArea(0.0f),
		RemoveSubmeshIndices(false),
		CalculateSubmeshIndicesByAdjacency(false),
		CalculateSubmeshIndicesByMaterial(false),
		CreateConvexHull(false),
		SubdivisionLevel(0u),
		SubdivisionMode(SubdivisionMode::CatmullClark),
		Translate(0.0f, 0.0f, 0.0f),
		Scale(1.0f, 1.0f, 1.0f),
		RotationEulerXYZ(0.0f, 0.0f, 0.0f),
		BevelDistanceAbsolute(0.0f),
		BevelDihedralAngleThresholdDegrees(30.0f),
		BevelTolerance(1e-5f),
		BevelSegmentCount(2u),
		BevelProfileParameter(2.0f),
		AbsoluteBoundingSphereSize(0.0f),
		Quiet(false),
		Deterministic(false)
		{
		}

		float WeldingThreshold;					   /**< If set to a value > 0, automatically welds vertices within the specified distance. */
		float WeldingNormalAngleThreshold;		   /**< Prevents welding of normals if the angle between the welded normal and the wedge normal is above the specified value in degrees. */
		bool WeldingBoundaries;					   /**< True to only weld boundary vertices. */
		float HealTJunctionThreshold;			   /**< If set to a value > 0, automatically heals T-Junctions if the vertex-edge distance is below the specified threshold. */
		float RemoveDegenerateFacesThreshold;	   /**< If set to a value > 0, automatically removes degenerates if two or more corners are shared for a polygon. */
		bool FixNonManifold;					   /**< Rebuilds the mesh topology to be manifold. */
		bool ConformNormals;					   /**< Unifies the direction of the wedge normals. */
		bool ConformWindingOrder;				   /**< Unifies the polygon winding order. */
		bool FlipNormals;						   /**< Flips the normals of the mesh. */
		bool FlipWindingOrder;					   /**< Flips the winding order of the mesh. */
		bool FillHoles;							   /**< Fill all holes in the mesh. */
		NormalHealingMode::Type NormalHealingMode; /**< Invalid normals can automatically be fixed by InstaLOD without recomputing all normals. */
		bool RecalculateNormals;				   /**< Recalculate normals of the output mesh. */
		float HardAngleThreshold;				   /**< When recalculating normals: smooth faces if the normal angle is below this value (in degrees). */
		bool WeightedNormals;					   /**< When recalculating normals: smoothed normals are weighted by various geometric properties. */
		float MinimumSubMeshBoundingSphereRadius;  /**< If set to a value > 0, automatically remove all submeshes (determined by adjacency) where the bounding sphere radius is below the threshold. */
		float MinimumFaceArea;					   /**< If set to a value > 0, automatically remove all faces with an area smaller than the specified value. */
		bool RemoveSubmeshIndices;				   /**< Remove face submesh indices. */
		bool CalculateSubmeshIndicesByAdjacency;   /**< Calculate new face submesh indices based on adjacency. */
		bool CalculateSubmeshIndicesByMaterial;	   /**< Calculate new face submesh indices based on face materials. */
		bool CreateConvexHull;					   /**< True to create a convex hull for the input mesh. */
		uint32 SubdivisionLevel;				   /**< If set to a value >0, repeatedly subdivides the mesh until the subdivision level is reached. */
		SubdivisionMode::Type SubdivisionMode;	   /**< Selects the subdivision mode. */
		InstaVec3F Translate;					   /**< Translates the mesh. */
		InstaVec3F Scale;						   /**< Scales the mesh. */
		InstaVec3F RotationEulerXYZ;			   /**< Rotates the mesh by the specified euler rotation with order XYZ. */
		float BevelDistanceAbsolute;			   /**< If set to a value > 0, perform a bevel operation with the bevel distance in absolute units.
												        @note The bevel operation is currently under development and is considered to be a feature preview. The feature is expected to be unstable and not work reliably on all input meshes.
												    */
		float BevelDihedralAngleThresholdDegrees;  /**< The threshold in degrees for the dihedral angles. Edges with a dihedral angle less than the threshold do not get beveled. */
		float BevelTolerance;					   /**< The tolerance for comparing floating point values. */
		uint32 BevelSegmentCount;				   /**< The segment count for subdividing the bevel. In this feature preview, the segment count gets rounded up to a power of two. */
		float BevelProfileParameter;			   /**< The parameter to control the shape of the bevel profile using a super ellipse. A value of 1.0 creates a flat profile, a value of 2.0 creates a circle. Values between 0 and 1 create a concave profile, and values greater than 2 create sharper edges. */
		float AbsoluteBoundingSphereSize;		   /**< If set to a value > 0, scales the mesh by a factor that makes the minimum volume bounding sphere of the scaled mesh equal to that value. */
		bool Quiet;								   /**< Prevents any output to the console. */
		bool Deterministic;						   /**< Makes the algorithm deterministic at the cost of speed. */
	};

	typedef void (*pfnMeshToolKitProgressCallback)(class IMeshToolKitOperation* operation, const IInstaLODMesh* sourceMesh, IInstaLODMesh* targetMesh, const float progressInPercent);

	/**
	 * The IMeshToolKitOperation interface represents a mesh toolkit operation for
	 * arbitrary input meshes.
	 */
	class IMeshToolKitOperation
	{
	protected:
		virtual ~IMeshToolKitOperation() {}

	public:
		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnMeshToolKitProgressCallback callback) = 0;

		/**
		 * Performs a mesh tool kit operation.
		 *
		 * @param inputMesh the input mesh
		 * @param outputMesh the output mesh
		 * @param settings the settings
		 * @return mesh tool kit result
		 */
		virtual MeshToolKitResult Execute(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const MeshToolKitSettings& settings) = 0;
	};

	/**
	 * The BevelResult structure contains information about a completed bevel operation.
	 */
	struct BevelResult
	{
		BevelResult() :
		Success(false),
		IsAuthorized(true)
		{
		}

		bool Success;	   /**< True, if the operation was successful. */
		bool IsAuthorized; /**< True, if the host was properly authorized during the operation. */
	};

	/**
	 * The BevelSettings structure contains settings exposed by the bevel API.
	 */
	struct BevelSettings
	{
		BevelSettings() :
		DihedralAngleThresholdDegrees(30.0f),
		BevelDistanceAbsolute(1.0f),
		Tolerance(1e-5f),
		SegmentCount(2u),
		ProfileParameter(2.0f)
		{
		}

		float DihedralAngleThresholdDegrees; /**< The threshold in degrees for the dihedral angles. Edges with a dihedral angle less than the threshold do not get beveled. */
		float BevelDistanceAbsolute;		 /**< The bevel distance in absolute units. */
		float Tolerance;					 /**< The tolerance for comparing floating point values. */
		uint32 SegmentCount;				 /**< The segment count for subdividing the bevel. In this feature preview, the segment count gets rounded up to a power of two. */
		float ProfileParameter;				 /**< The parameter to control the shape of the bevel profile using a super ellipse. A value of 1.0 creates a flat profile, a value of 2.0 creates a circle. Values between 0 and 1 create a concave profile, and values greater than 2 create sharper edges. */
	};

	typedef void (*pfnBevelProgressCallback)(class IBevelOperation* operation, const IInstaLODMeshBase* sourceMesh, IInstaLODMeshBase* targetMesh, const float progressInPercent);

	/**
	 * The IBevelOperation interface represents a bevel operation.
	 *
	 * @note The IBevelOperation is currently under development and is considered to be a feature preview. The feature is expected to be unstable and not work reliably on all input meshes.
	 */
	class IBevelOperation
	{
	public:
		/**
		 * Performs a bevel operation.
		 *
		 * @param inputMesh The input mesh.
		 * @param outputMesh The output mesh.
		 * @param settings The settings.
		 * @return bevel result.
		 */
		virtual BevelResult Execute(const IInstaLODMeshBase* inputMesh, IInstaLODMeshBase* outputMesh, const BevelSettings& settings) = 0;

		/**
		 * Sets the progress callback
		 * @note Progress callbacks can be called from child threads.
		 *
		 * @param callback the progress callback
		 */
		virtual void SetProgressCallback(pfnBevelProgressCallback callback) = 0;
	};

	/**
	 * The Plugin enum represents different plugin types.
	 */
	namespace Plugin
	{
		enum Type
		{
			Undefined = 0,		 /**< Undefined. */
			MeshOperation = 1,	 /**< Mesh operation plugin. */
			UserInterace = 2,	 /**< User interface plugin. */
			Viewport = 3,		 /**< Viewport plugin. */
			Factory = 4,		 /**< Factory plugin. */
			FileImport = 5,		 /**< File import plugin. */
			FileExport = 6,		 /**< File export plugin. */
			SceneImportRule = 7, /**< Scene import rule plugin. */
			Application = 8,	 /**< Application specific plugin. */
			Count = 9
		};
	}

	/**
	 * The PluginAttribute enum represents different plugin attributes.
	 */
	namespace PluginAttribute
	{
		enum Type
		{
			Undefined = 0,	 /**< Undefined. */
			Name = 1,		 /**< The plugin name. */
			Fullname = 2,	 /**< The plugin full name, including the owning factory name. */
			Category = 3,	 /**< The plugin category. */
			Description = 4, /**< The plugin description. */
			APIVersion = 5,	 /**< The plugin api version. */
			Count = 6
		};
	}

	/**
	 * The IInstaLOD interface is the main API for the InstaLOD SDK.
	 * @note Invoke GetInstaLOD to allocate an instance that implements this interface.
	 * @note Use the authorization methods to make sure the SDK is properly configured prior to using it.
	 */
	class IInstaLOD
	{
	protected:
		virtual ~IInstaLOD() {}

	public:
		/**
		 * Returns the SDK version.
		 *
		 * @return the SDK version.
		 */
		virtual int32 GetVersion() const = 0;

		/**
		 * Returns the SDK build date as string.
		 *
		 * @return the SDK build date.
		 */
		virtual const char* GetBuildDate() const = 0;

		/**
		 * Determines if the SDK supports GPGPU features.
		 * @note The InstaLOD SDK requires Vulkan compatible GPU drivers on PC Windows and Linux
		 * and leverages the GPU of the M1 chip on the Apple Silicon platform.
		 * GPGPU on Intel based Apple devices is not supported.
		 *
		 * @return true if the platform supports GPGPU.
		 */
		virtual bool IsGPUComputeAvailable() const = 0;

		/**
		 * Initializes the GPGPU features.
		 * @note This does not need to be done manually as all subsystems that would
		 * use GPGPU would initialize it lazily.
		 *
		 * @return true if the platform supports GPGPU.
		 */
		virtual bool InitializeGPUCompute() = 0;

		/**
		 * Determines if GPGPU features are initialized.
		 *
		 * @return true if GPGPU features are initialized.
		 */
		virtual bool IsGPUComputeInitialized() const = 0;

		/**
		 * Is GPGPU device available.
		 * @note This method returns false, unless GPGPU was initialize
		 * either automatically by a subsystem or manually by the user.
		 *
		 * @return true if the platform supports GPGPU.
		 */
		virtual bool IsGPUComputeDeviceAvailable() const = 0;

		/**
		 * Allocates an InstaLOD mesh.
		 *
		 * @return InstaLOD mesh
		 */
		virtual IInstaLODMesh* AllocMesh() = 0;

		/**
		 * Deallocates an InstaLOD mesh.
		 *
		 * @param mesh the InstaLOD mesh.
		 */
		virtual void DeallocMesh(IInstaLODMesh* mesh) = 0;

		/**
		 * Allocates an InstaLOD mesh.
		 * @note A successfully allocated instance must be manually deallocated by the caller of this API.
		 *
		 * @param path the file path to deserialize.
		 * @param index the index of the mesh to deserialize.
		 * @return InstaLOD mesh or nullptr if deserialization failed.
		 */
		virtual IInstaLODMeshBase* DeserializeMesh(const char* path, uint32 index) = 0;

		/**
		 * Allocates an InstaLOD polygon mesh.
		 *
		 * @return InstaLOD polygon mesh.
		 */
		virtual IInstaLODPolygonMesh* AllocPolygonMesh() = 0;

		/**
		 * Deallocates an InstaLOD polygon mesh.
		 *
		 * @param mesh the InstaLOD mesh.
		 */
		virtual void DeallocPolygonMesh(IInstaLODPolygonMesh* mesh) = 0;

		/**
		 * Allocates an InstaLOD skeleton.
		 *
		 * @return InstaLOD skeleton.
		 */
		virtual IInstaLODSkeleton* AllocSkeleton() = 0;

		/**
		 * Deallocates an InstaLOD skeleton.
		 *
		 * @param mesh the InstaLOD skeleton.
		 */
		virtual void DeallocSkeleton(IInstaLODSkeleton* mesh) = 0;

		/**
		 * Allocates an InstaLOD distance field.
		 *
		 * @param storageType The storage type to select between dense and sparse storage.
		 * @return The signed distance field.
		 */
		virtual IInstaLODDistanceField* AllocDistanceField(const DistanceFieldStorageType::Type storageType) = 0;

		/**
		 * Reads a distance field from a file.
		 *
		 * @param path The file path.
		 * @param storageType The enum to select between dense or sparse storage for the distance field cells.
		 * @return The signed distance field upon success or nullptr on failure. This needs to be released with DeallocDistanceField.
		 */
		virtual IInstaLODDistanceField* DeserializeDistanceField(const char* path, const DistanceFieldStorageType::Type storageType) = 0;

		/**
		 * Deallocates a distance field.
		 *
		 * @param distanceField The distance field.
		 */
		virtual void DeallocDistanceField(IInstaLODDistanceField* distanceField) = 0;

		/**
		 * Allocates an optimize operation.
		 *
		 * @return optimize operation.
		 */
		virtual IOptimizeOperation* AllocOptimizeOperation() = 0;

		/**
		 * Deallocates an optimize operation.
		 *
		 * @param operation optimize operation.
		 */
		virtual void DeallocOptimizeOperation(IOptimizeOperation* operation) = 0;

		/**
		 * Allocates a skeleton optimize operation.
		 *
		 * @return skeleton optimize operation.
		 */
		virtual ISkeletonOptimizeOperation* AllocSkeletonOptimizeOperation() = 0;

		/**
		 * Deallocates a skeleton optimize operation.
		 *
		 * @param operation skeleton optimize operation.
		 */
		virtual void DeallocSkeletonOptimizeOperation(ISkeletonOptimizeOperation* operation) = 0;

		/**
		 * Allocates a mesh/material merge operation.
		 *
		 * @return merge operation.
		 */
		virtual IMeshMergeOperation2* AllocMeshMergeOperation() = 0;

		/**
		 * Deallocates a mesh/material merge operation.
		 *
		 * @param operation merge operation.
		 */
		virtual void DeallocMeshMergeOperation(IMeshMergeOperation2* operation) = 0;

		/**
		 * Allocates a remeshing operation.
		 *
		 * @return remeshing operation.
		 */
		virtual IRemeshingOperation* AllocRemeshingOperation() = 0;

		/**
		 * Deallocates a remeshing operation.
		 *
		 * @param operation remesh operation.
		 */
		virtual void DeallocRemeshingOperation(IRemeshingOperation* operation) = 0;

		/**
		 * Allocates an isotropic remeshing operation.
		 *
		 * @return remeshing operation.
		 */
		virtual IIsotropicRemeshingOperation* AllocIsotropicRemeshingOperation() = 0;

		/**
		 * Deallocates an isotropic remeshing operation.
		 *
		 * @param operation remesh operation.
		 */
		virtual void DeallocIsotropicRemeshingOperation(IIsotropicRemeshingOperation* operation) = 0;

		/**
		 * Allocates a quad remeshing operation.
		 *
		 * @return remeshing operation.
		 */
		virtual IQuadRemeshingOperation* AllocQuadRemeshingOperation() = 0;

		/**
		 * Deallocates a quad remeshing operation.
		 *
		 * @param operation remesh operation.
		 */
		virtual void DeallocQuadRemeshingOperation(IQuadRemeshingOperation* operation) = 0;

		/**
		 * Allocates a voxelize operation.
		 *
		 * @return voxelize operation.
		 */
		virtual IVoxelizeOperation* AllocVoxelizeOperation() = 0;

		/**
		 * Deallocates a voxelize operation.
		 *
		 * @param operation voxelize operation.
		 */
		virtual void DeallocVoxelizeOperation(IVoxelizeOperation* operation) = 0;

		/**
		 * Allocates a create signed distance field operation.
		 *
		 * @return The create signed distance field operation.
		 */
		virtual ICreateSignedDistanceFieldOperation* AllocCreateSignedDistanceFieldOperation() = 0;

		/**
		 * Deallocates a create signed distance field operation.
		 *
		 * @param operation The create signed distance field operation.
		 */
		virtual void DeallocCreateSignedDistanceFieldOperation(ICreateSignedDistanceFieldOperation* operation) = 0;

		/**
		 * Allocates a create signed distance field operation.
		 *
		 * @return The create signed distance field operation.
		 */
		virtual ISurfaceExtractionOperation* AllocSurfaceExtractionOperation() = 0;

		/**
		 * Deallocates a create signed distance field operation.
		 *
		 * @param operation The create signed distance field operation.
		 */
		virtual void DeallocSurfaceExtractionOperation(ISurfaceExtractionOperation* operation) = 0;

		/**
		 * Allocates a CSG operation.
		 *
		 * @return CSG operation.
		 */
		virtual IConstructiveSolidGeometryOperation* AllocConstructiveSolidGeometryOperation() = 0;

		/**
		 * Deallocates a CSG operation.
		 *
		 * @param operation CSG operation.
		 */
		virtual void DeallocConstructiveSolidGeometryOperation(IConstructiveSolidGeometryOperation* operation) = 0;

		/**
		 * Allocates an imposterize operation.
		 *
		 * @return imposterize operation.
		 */
		virtual IImposterizeOperation* AllocImposterizeOperation() = 0;

		/**
		 * Deallocates an imposterize operation.
		 *
		 * @param operation imposterize operation.
		 */
		virtual void DeallocImposterizeOperation(IImposterizeOperation* operation) = 0;

		/**
		 * Allocates a bake operation.
		 *
		 * @return bake operation.
		 */
		virtual IBakeOperation* AllocBakeOperation() = 0;

		/**
		 * Deallocates a bake operation.
		 *
		 * @param operation bake operation.
		 */
		virtual void DeallocBakeOperation(IBakeOperation* operation) = 0;

		/**
		 * Allocates an unwrap operation.
		 *
		 * @return unwrap operation.
		 */
		virtual IUnwrapOperation* AllocUnwrapOperation() = 0;

		/**
		 * Deallocates an unwrap operation.
		 *
		 * @param operation unwrap operation.
		 */
		virtual void DeallocUnwrapOperation(IUnwrapOperation* operation) = 0;

		/**
		 * Allocates an UV pack operation.
		 *
		 * @return UV pack operation.
		 */
		virtual IUVPackOperation* AllocUVPackOperation() = 0;

		/**
		 * Deallocates an UV pack operation.
		 *
		 * @param operation UV pack operation.
		 */
		virtual void DeallocUVPackOperation(IUVPackOperation* operation) = 0;

		/**
		 * Allocates an occlusion culling operation.
		 *
		 * @return occlusion culling operation.
		 */
		virtual IOcclusionCullOperation* AllocOcclusionCullOperation() = 0;

		/**
		 * Deallocates an occlusion culling operation.
		 *
		 * @param operation occlusion culling operation.
		 */
		virtual void DeallocOcclusionCullOperation(IOcclusionCullOperation* operation) = 0;

		/**
		 * Allocates an mesh toolkit operation.
		 *
		 * @return mesh toolkit operation.
		 */
		virtual IMeshToolKitOperation* AllocMeshToolKitOperation() = 0;

		/**
		 * Deallocates an mesh toolkit operation.
		 *
		 * @param operation mesh toolkit operation.
		 */
		virtual void DeallocMeshToolKitOperation(IMeshToolKitOperation* operation) = 0;

		/**
		 * Allocates a bevel operation.
		 *
		 * @return bevel operation.
		 */
		virtual IBevelOperation* AllocBevelOperation() = 0;

		/**
		 * Deallocates a bevel operation.
		 *
		 * @param operation bevel operation.
		 */
		virtual void DeallocBevelOperation(IBevelOperation* operation) = 0;

		/**
		 * Allocates a material collection.
		 *
		 * @return material collection.
		 */
		virtual IInstaLODMaterialData* AllocMaterialData() = 0;

		/**
		 * Allocates a material collection.
		 * @note A successfully allocated instance must be manually deallocated by the caller of this API.
		 *
		 * @param path the file path to deserialize.
		 * @return InstaLOD mesh or nullptr if deserialization failed.
		 */
		virtual IInstaLODMaterialData* DeserializeMaterialData(const char* path) = 0;

		/**
		 * Deallocates a material collection.
		 *
		 * @param data material collection.
		 */
		virtual void DeallocMaterialData(IInstaLODMaterialData* data) = 0;

		/**
		 * Reads compressed image \p data and writes the raw image data into \p outBuffer.
		 * If \p outBuffer is nullptr, only the properties of the data will be acquired.
		 * @note Errors are logged to the InstaLOD error stream.
		 * @note The supported formats are detailed in the documentation of IInstaLODMaterial.
		 *
		 * @param data The compressed data.
		 * @param dataSize The compressed data size.
		 * @param [out] outWidth The output width.
		 * @param [out] outHeight The output height.
		 * @param [out] outComponentType The output component type.
		 * @param [out] outPixelType The output pixel type.
		 * @param [out] outBuffer (Optional) The output pointer for the decompressed data. @note The buffer must be freed using `DeallocCompressedImageData`.
		 * @return true upon success.
		 */
		virtual bool ReadCompressedImageData(const uint8* data, const uint64 dataSize, uint32& outWidth, uint32& outHeight,
											 IInstaLODTexturePage::ComponentType& outComponentType, IInstaLODTexturePage::PixelType& outPixelType, uint8** outBuffer) = 0;

		/**
		 * Deallocates the \p data.
		 *
		 * @param data The data to deallocate
		 */
		virtual void DeallocCompressedImageData(uint8* data) = 0;

		/**
		 * Allocates a texture page using the specified file location.
		 * @note InstaLOD will best-guess the image format component and pixel type.
		 * @note The supported formats are detailed in the documentation of IInstaLODMaterial.
		 * @note The texture page must be deallocated using `DeallocTexturePage`.
		 *
		 * @param name The name of the new texture page.
		 * @param filename The filename of the texture data.
		 * @param type The type of the new texture page.
		 * @return The new texture page or nullptr if the image could not be loaded.
		 */
		virtual IInstaLODTexturePage* AllocTexturePageFromDisk(const char* name, const char* filename, const IInstaLODTexturePage::Type type) = 0;

		/**
		 * Deallocates the \p data.
		 * @note Only deallocate texture pages that have been allocated by a call to `AddTexturePageFromDisk`.
		 * Texture pages that are allocated via material data are memory managed by the material data.
		 *
		 * @param texturePage The texture page to deallocate.
		 */
		virtual void DeallocTexturePage(IInstaLODTexturePage* texturePage) = 0;

		/**
		 * Sets the debug pointer (internal)
		 *
		 * @param debug the debug data storage.
		 */
		virtual void SetDebug(class IInstaLODDebugData* debug) = 0;

		/**
		 * Gets the debug pointer (internal)
		 *
		 * @return the debug data storage.
		 */
		virtual class IInstaLODDebugData* GetDebug() = 0;

		/**
		 * Deallocates the InstaLOD API.
		 */
		virtual void Dealloc() = 0;

		/**
		 * Initializes machine authorization module.
		 * The authorization storage path must be writable by the current process.
		 *
		 * @param entitlements the entitlements.
		 * @param authorizationStoragePath The authorization storage path. Set to nullptr to use platform specific shared user storage path.
		 * @return true if licensing was initialized.
		 */
		virtual bool InitializeAuthorization(const char* entitlements, const char* authorizationStoragePath) = 0;

		/**
		 * Authorizes this machine to use InstaLOD using the specified license file.
		 * @note Before the InstaLOD API can be used the activation needs to be performed.
		 * @note The method requires an active internet connection in order to perform
		 * machine authorization.
		 * @note InstaLOD will try to communicate with servers at "api.instalod.io" via HTTPS.
		 *
		 * @param username the username/email.
		 * @param password the password/license key.
		 * @return true if the machine has been activated.
		 */
		virtual bool AuthorizeMachine(const char* username, const char* password) = 0;

		/**
		 * Deauthorizes this machine and removes the license file.
		 * @note Before uninstalling InstaLOD it is required to deactivate this machine
		 * in order to avoid hitting the active machine limit of the license.
		 * @note Indie licenses are limited to two active machines.
		 * @note AAA and regular license machine limits depend on the license term.
		 * @note InstaLOD will try to communicate with servers at "api.instalod.io" via HTTPS.
		 *
		 * @param username the username/email.
		 * @param password the password/license key.
		 * @return true if the machine has been deactivated.
		 */
		virtual bool DeauthorizeMachine(const char* username, const char* password) = 0;

		/**
		 * Determines whether the host is authorized to use InstaLOD.
		 * @note This method may contact InstaLOD servers to verify or update the current license.
		 * @note An active internet connection is required in order to perform license refresh.
		 * @note InstaLOD will try to communicate with servers at "api.instalod.io" via HTTPS.
		 * @remarks Due to thread synchronization mechanisms this method is not const.
		 *
		 * @return true if the host is authorized.
		 */
		virtual bool IsHostAuthorized() = 0;

		/**
		 * Determines whether the host is authorized for the specified entitlements.
		 * @note This method may contact InstaLOD servers to verify or update the current license.
		 * @note An active internet connection is required in order to perform license refresh.
		 * @note InstaLOD will try to communicate with servers at "api.instalod.io" via HTTPS.
		 * @remarks Due to thread synchronization mechanisms this method is not const.
		 *
		 * @param productEntitlement The product's entitlement identifier.
		 * @return true if the host is authorized.
		 */
		virtual bool IsHostEntitledForProduct(const char* productEntitlement) = 0;

		/**
		 * Gets the machine authorization key that is necessary to start a license request with the InstaLOD licensing API.
		 * @note This call does not require an active connection to the internet. This method is intended to be used
		 * to obtain a machine specific key that can be used to generate a license file from a different computer using
		 * the AuthorizeMachineWithKey() method.
		 * Regular SDK authorization should use the AuthorizeMachine() method to authorize this machine.
		 *
		 *
		 * @param buffer (optional) the output buffer. Pass a null pointer to query the size of the key string.
		 * @param bufferSize the output buffer size
		 * @param keySize (optional) the size of the key string
		 * @return the amount of bytes written to buffer.
		 */
		virtual uint64 GetMachineAuthorizationKey(char* buffer, const uint64 bufferSize, uint64* keySize) = 0;

		/**
		 * Authorizes a machine with the specified machine authorization key and writes the license file to the specified file path.
		 * The license can be copied to the machine where the key originated from and ingested using IngestMachineAuthorizationLicense().
		 *
		 * @note Before the InstaLOD API can be used the activation needs to be performed.
		 * @note The method requires an active internet connection in order to perform
		 * machine authorization.
		 * @note InstaLOD will try to communicate with servers at "api.instalod.io" via HTTPS.
		 *
		 * @param key the machine key.
		 * @param username the username/email.
		 * @param password the password/license key.
		 * @param licenseFilePath the path the license file will be written to.
		 * @return true if the machine has been activated and the license was written to disk.
		 */
		virtual bool AuthorizeMachineWithKey(const char* key, const char* username, const char* password, const char* licenseFilePath) = 0;

		/**
		 * Authorizes a machine using the SDK \p licenseData.
		 *
		 * @note This API should only be used when integrating the SDK in an application
		 * that is redistributed to third-parties under the InstaLOD OEM licensing program.
		 *
		 * @param licenseData the license data.
		 * @return true if the machine has been activated and the license was written to disk.
		 */
		virtual bool AuthorizeWithSDKLicense(const char* licenseData) = 0;

		/**
		 * Loads the specified license file from disk and authorizes this machine if the license file
		 * was generated using the machine authorization key that originated from this host.
		 *
		 * @param licenseFilePath the license file path.
		 * @return true if the machine has been activated.
		 */
		virtual bool IngestMachineAuthorizationLicense(const char* licenseFilePath) = 0;

		/**
		 * Returns authorization information.
		 * @note Whenever a call to a authorization method fails. The authorization information
		 * will return error information.
		 * @note The authorization error log will be cleared whenever an authorization method
		 * is invoked. Therefore the error should be evaluated as soon as a method fails.
		 * @remarks Due to thread synchronization mechanisms this method is not const.
		 *
		 * @return the authorization information.
		 */
		virtual const char* GetAuthorizationInformation() = 0;

		/**
		 * Enables or disables the generation of the unauthorized key mesh.
		 * @note The unauthorized key mesh is generated if an operation is executed
		 * without an authorized SDK. This prevents automated pipelines from failing.
		 * This feature is enabled by default.
		 *
		 * @param isEnabled Set to true to enable the generation of the unauthorized key mesh.
		 */
		virtual void SetGenerateUnauthorizedMesh(bool isEnabled) = 0;

		/**
		 * Determines if the generation of the unauthorized key mesh is enabled.
		 * @note The unauthorized key mesh is generated if an operation is executed
		 * without an authorized SDK. This prevents automated pipelines from failing.
		 * This feature is enabled by default.
		 *
		 * @return true if the generation of the unauthorized key mesh is enabled.
		 */
		virtual bool IsGeneratingUnauthorizedMesh() const = 0;

		/**
		 * Enables or disables stdout and stderr output.
		 * @note By default InstaLOD writes log messages to the built-in message log and to stdout/stderr.
		 * In some cases it can be desirable to disable the stdout/stderr output.
		 * To retrieve the message log use GetMessageLog().
		 *
		 * @param isEnabled Set to true to enable stdout/stderr output.
		 */
		virtual void SetStandardOutputEnabled(bool isEnabled) = 0;

		/**
		 * Gets the message log that contains warning or errors generated by the InstaLOD SDK.
		 * @note Authorization information is not logged here. See IInstaLOD::GetAuthorizationInformation
		 * for information related to machine authorization.
		 *
		 * @param buffer (optional) the output buffer. Pass a null pointer to query the size of the message log.
		 * @param bufferSize the output buffer size
		 * @param logSize (optional) the size of the message log
		 * @return the amount of bytes written to buffer.
		 */
		virtual uint64 GetMessageLog(char* buffer, const uint64 bufferSize, uint64* logSize) const = 0;

		/**
		 * Removes all log entries from the message log.
		 */
		virtual void ClearMessageLog() = 0;

		/**
		 * Gets the output streams of the specified type.
		 * @note If your C runtime is using a different pointer to stdout or stderr
		 * This method will return nullptr. To circumvent runtime difference
		 * it is possible to pass (FILE*)1 for stdout or (FILE*)2 for stderr.
		 *
		 * @param type pass stdout or stderr.
		 * @return The output stream used by InstaLOD for the specified type.
		 */
		virtual FILE* GetOutputStream(FILE* type) = 0;

		/**
		 * Sets the output streams for the specified type.
		 * @note If your C runtime is using a different pointer to stdout or stderr
		 * This method will return false. To circumvent runtime difference
		 * it is possible to pass (FILE*)1 for stdout or (FILE*)2 for stderr.
		 *
		 * @param stream the output stream.
		 * @param type pass stdout or stderr.
		 * @return true upon success.
		 */
		virtual bool SetOutputStream(FILE* stream, const FILE* type) = 0;

		/**
		 * Optimizes the provided \p inputMesh using the specified settings and writes the results to the specified \p outputMesh.
		 * @note This is a convenience method, use AllocOptimizeOperation() for progress handling and improved memory efficiency
		 * when optimizing multiple meshes.
		 *
		 * @param inputMesh the input mesh.
		 * @param outputMesh the output mesh.
		 * @param settings the settings.
		 * @return optimize result.
		 */
		virtual OptimizeResult Optimize(const IInstaLODMesh* inputMesh, IInstaLODMesh* outputMesh, const OptimizeSettings& settings) = 0;

		/**
		 * Allocates a mesh for demonstration and test purposes.
		 *
		 * @return a test mesh instance.
		 */
		virtual IInstaLODMesh* AllocTestMesh() = 0;

		/**
		 * Casts the specified instance into an IInstaLODMeshExtended instance.
		 *
		 * @param mesh the IInstaLODMesh instance to be casted.
		 * @return the type casted instance.
		 */
		virtual class IInstaLODMeshExtended* CastToInstaLODMeshExtended(IInstaLODMesh* mesh) const = 0;

		/**
		 * Registers the specified plugin factory.
		 *
		 * @return true upon success.
		 */
		virtual bool RegisterPluginFactory(class IInstaLODPluginFactory* plugin) = 0;

		/**
		 * Unregisters the specified plugin factory.
		 *
		 * @return true upon success.
		 */
		virtual bool UnregisterPluginFactory(class IInstaLODPluginFactory* plugin) = 0;

		/**
		 * Loads and registers all plugin factories at the specified path.
		 *
		 * @param path the plugin path.
		 * @return the number of plugins registered.
		 */
		virtual uint32 RegisterPluginFactoriesFromPath(const char* path) = 0;

		/**
		 * Ignores all files that case-insensitive match the \p filename when
		 * registering plugins via RegisterPluginFactoriesFromPath .
		 *
		 * @param filename the filename to ignore
		 */
		virtual void IgnorePluginFactoryLibraryFilename(const char* filename) = 0;

		/**
		 * Allocates the specified plugin.
		 * @remarks The plugin full name is the name of the plugin factory followed by a forward slash and the actual plugin name.
		 *
		 * @param fullname the name of the plugin.
		 * @param type the type of the plugin.
		 * @param [out] outPlugin Output storage for a pointer to the plugin instance.
		 * @return true upon success.
		 */
		virtual bool AllocPluginWithName(const char* fullname, const Plugin::Type type, class IInstaLODPlugin** outPlugin) = 0;

		/**
		 * Deallocates the specified plugin.
		 *
		 * @param plugin Output storage for a pointer to the plugin instance.
		 * @return true upon success.
		 */
		virtual bool DeallocPlugin(class IInstaLODPlugin* plugin) = 0;

		/**
		 * Gets the number of plugins for the specified.
		 *
		 * @param pluginType The plugin type.
		 * @return The number of plugins.
		 */
		virtual uint32 GetPluginCount(const Plugin::Type pluginType) = 0;

		/**
		 * Determines if a plugin with the specified name and type is available.
		 * @remarks The plugin full name is the name of the plugin factory followed by a forward slash and the actual plugin name.
		 *
		 * @param fullname The name of the plugin.
		 * @param pluginType The type of the plugin.
		 * @return True if this instance can allocate/deallocate the specified plugin.
		 */
		virtual bool IsPluginAvailable(const char* fullname, const Plugin::Type pluginType) = 0;

		/**
		 * Gets a plugin attribute value as string.
		 *
		 * @param pluginAttributeType The plugin attribute type.
		 * @param pluginType The type of the plugin.
		 * @param index The plugin index in the plugin type list.
		 * @param [out] outAttribute (optional) The string buffer. Pass a null pointer to query the size of the attribute value as string.
		 * @param attributeSize The name buffer size.
		 * @return The number of bytes written to buffer.
		 */
		virtual uint64 GetPluginAttribute(const PluginAttribute::Type pluginAttributeType, const Plugin::Type pluginType, const uint32 index, char* outAttribute, const uint64 attributeSize) = 0;

		/**
		 * Gets the task scheduler if available.
		 *
		 * @return The task scheduler or nullptr if no task scheduler is available.
		 */
		virtual class ITaskScheduler* GetTaskScheduler() = 0;

		/**
		 * Gets the math kernel if available.
		 *
		 * @return The math kernel or nullptr if no math kernel is available.
		 */
		virtual class IMathKernel* GetMathKernel() = 0;

		/**
		 * Gets the internal API.
		 * 
		 * @param internalAPIVersion The internal API version.
		 * @return The internal API or nullptr if the internal API version does not match.
		 */
		virtual class IInstaLODInternal* GetInternal(const int32 internalAPIVersion) = 0;
	};
} // namespace InstaLOD

#if !defined(INSTALOD_STRINGIFY)
#	define INSTALOD_STRINGIFY(x) #	  x
#	define INSTALOD_TOSTRING(x)  INSTALOD_STRINGIFY(x)
#endif

#define INSTALOD_API_VERSION_MAJOR	6
#define INSTALOD_API_VERSION_MINOR	11
#define INSTALOD_API_VERSION_STRING (INSTALOD_TOSTRING(INSTALOD_API_VERSION_MAJOR) "." INSTALOD_TOSTRING(INSTALOD_API_VERSION_MINOR))
#define INSTALOD_API_VERSION		(((INSTALOD_API_VERSION_MAJOR) << 16) | ((INSTALOD_API_VERSION_MINOR)&0xFFFF))

#if defined(_MSC_VER)
#	define INSTALOD_LIBRARY_NAME "InstaLOD.dll"
#	define INSTALOD_DLOPEN(x)	  LoadLibrary(x)
#	define INSTALOD_DLSYM(x, y)  GetProcAddress((HMODULE)(x), (y))
#	define INSTALOD_DLCLOSE(x)	  FreeLibrary((HMODULE)(x))
#	define INSTALOD_DLHANDLE	  HMODULE
#else
#	if defined(__linux__)
#		define INSTALOD_LIBRARY_NAME "libInstaLOD.so"
#	else
#		define INSTALOD_LIBRARY_NAME "libInstaLOD.dylib"
#	endif
#	define INSTALOD_DLOPEN(x)	 dlopen(x, RTLD_NOW)
#	define INSTALOD_DLSYM(x, y) dlsym(x, y)
#	define INSTALOD_DLCLOSE(x)	 dlclose(x)
#	define INSTALOD_DLHANDLE	 void*
#endif

/**
 * Gets the InstaLOD API.
 *
 * @param version The requested API version number. Pass the define INSTALOD_API_VERSION.
 * This is used to protect against mixing incompatible header versions.
 * Requesting a wrong version number will result in an undefined API pointer.
 * @param [out] outInstaLOD The output pointer for the InstaLOD API.
 * @return 1u upon success.
 */
INSTALOD_API InstaLOD::uint32 GetInstaLOD(const InstaLOD::int32 version, InstaLOD::IInstaLOD** outInstaLOD);
typedef InstaLOD::uint32 (*pfnGetInstaLOD)(const InstaLOD::int32 version, InstaLOD::IInstaLOD** outInstaLOD);

/**
 * Gets the InstaLOD SDK build date.
 * @note This method is helpful to debug SDK incompatibility issues.
 *
 * @param [out] outVersion (optional) The output storage for the version.
 * @return The InstaLOD build date.
 */
INSTALOD_API const char* GetInstaLODBuildDate(InstaLOD::int32* outVersion);
typedef const char* (*pfnGetInstaLODBuildDate)(InstaLOD::int32* outVersion);

#endif
