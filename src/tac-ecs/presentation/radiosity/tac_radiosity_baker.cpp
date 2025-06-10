#include "tac_radiosity_baker.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/shell/tac_shell_timer.h"
#include "tac-engine-core/thirdparty/cgltf/cgltf_write.h"
#include "tac-engine-core/graphics/color/tac_color.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"

 #if TAC_SHOULD_IMPORT_STD()
   import std;
 #else
   #include <limits>
   #include <fstream>
 #endif

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  // Okay bro, heres the deal.
  //
  // I feel like using proper radiometric/photometric values suddenly makes things way complicated.
  // Like, for example, what radiance value should a light source emit?
  // In https://www.youtube.com/watch?v=B0sM7ZU0Nwo mirrors edge talk, they use a sun value
  // of 100000 lux. But then they have problems getting the correct white value.
  // Then theres exposure, tone mapping, etc.
  //
  // Instead, I think we should just go for it with SDR before even thinking about attempting HDR,
  // and use a light value of (1, 1, 1) in magical lighting units instead of some sort of physicaly
  // based W/(m^2sr) spectral radiance scRGB
  //
  // see also
  // https://seenaburns.com/dynamic-range/
  // http://www.gdcvault.com/play/1012351/Uncharted-2-HDR
  // http://filmicgames.com/archives/75
  // https://www.youtube.com/watch?v=B0sM7ZU0Nwo
  // https://bartwronski.com/2016/09/01/dynamic-range-and-evs/
  // https://placeholderart.wordpress.com/2014/11/21/implementing-a-physically-based-camera-manual-exposure/
  //  ^ this guy works at vicarious visions
  // https://knarkowicz.wordpress.com/2016/01/09/automatic-exposure/

  // -----------------------------------------------------------------------------------------------



  auto PreBakeScene::Raycast( PatchPower* fromPatch, RayTriangle::Ray ray ) -> PreBakeScene::RaycastResult
  {
    constexpr float kFloatMax{ std::numeric_limits<float>::max() };
    dynmc RaycastResult result{ .mT { kFloatMax } };

    for( Instance& instance : mInstances )
    {
      for( PatchPower& patchPower : instance.mPatchPowers )
      {
        if( &patchPower == fromPatch )
          continue;

        const RayTriangle::Triangle triangle
        {
          .mP0 { patchPower.mTriVerts[ 0 ] },
          .mP1 { patchPower.mTriVerts[ 1 ] },
          .mP2 { patchPower.mTriVerts[ 2 ] },
        };
        const RayTriangle::Output raycastResult{ RayTriangle::Solve( ray, triangle ) };
        if( !raycastResult.mValid || raycastResult.mT >= result.mT )
          continue;

        result.mHitPatch = &patchPower;
        result.mHitPatchMaterial = instance.mMaterial;
        result.mT = raycastResult.mT;
      }
    }

    return result;
  }

  auto PreBakeScene::GetMesh(const Model* model) -> const Mesh*
  {
    Mesh* mesh{};
    while( !mesh )
    {
      Errors errors;
      const ModelAssetManager::Params getMeshParams
      {
        .mPath       { model->mModelPath },
        .mModelIndex { model->mModelIndex },
      };
      mesh = ModelAssetManager::GetMesh( getMeshParams, errors );
      TAC_ASSERT( !errors );
      OS::OSThreadSleepMsec( 1 ); // wait for it to load
    }
    return mesh;
  }

  auto PreBakeScene::PatchPower::GetUnshotPower() const -> float
  {
    // When you look at this function signature, you may think:
    //
    // Q: How is it a valid operation to turn radiometric power, which irl is a spectrum,
    //    into a single floating point return value?
    //
    // A: As you may be aware, in the radiosity baker program, a spectrum is represented as a
    //    v3 linear srgb, representing a weighted sum of the primary color spectrums.
    //
    //    This function returns the average value of that, which is sort of a heuristic, 
    //    but the important thing is that the return value is used to sample 1 light among many,
    //    and get a probability for choosing that particular light.
    //
    //    A similar thing in PBRT may be the PowerLightSampler and SampledLight 
    //    https://pbr-book.org/4ed/Light_Sources/Light_Sampling
    return ( mCurrentUnshotPower.x + mCurrentUnshotPower.y + mCurrentUnshotPower.z ) / 3;
  }

  auto PreBakeScene::PatchPower::GetUniformRandomSurfacePoint() const -> v3
  {
    return RandomPointInTriangle( mTriVerts[ 0 ], mTriVerts[ 1 ], mTriVerts[ 2 ] );
  }

  void PreBakeScene::Init(const World* origWorld)
  {
    mWorld = TAC_NEW World;
    mWorld->DeepCopy( *origWorld );
    World* world{ mWorld };
    for( const Entity* entity : world->mEntities )
    {
      const Model* model{ Model::GetModel( entity ) };
      if( !model )
        continue;

      const Mesh* mesh{ GetMesh( model ) };
      const Material* material{ Material::GetMaterial( entity ) };
      const JPPTCPUMeshData& jpptCPUMeshData{ mesh->mJPPTCPUMeshData };
      const m4 worldTransform{ model->mEntity->mWorldTransform };

      const PatchPowers patchPowers{ [ & ]()
      {
        dynmc PatchPowers patchPowers;
        dynmc int iTriVert{};
        dynmc PatchPower::VtxPos triVerts{};
        dynmc PatchPower::VtxNor triNormals{};
        for( JPPTCPUMeshData::IndexType i : jpptCPUMeshData.mIndexes )
        {
          const v3& vertPos{ jpptCPUMeshData.mPositions[ i ] };
          const v3& vertNor{ jpptCPUMeshData.mNormals[ i ] };
          triVerts[ iTriVert ] = ( worldTransform * v4( vertPos, 1 ) ).xyz();
          triNormals[ iTriVert ] = ( worldTransform * v4( vertNor, 0 ) ).xyz(); // todo: inverse transpose or the other thing
          if( iTriVert == 2 )
          {
            const v3 e1{ triVerts[ 1 ] - triVerts[ 0 ] };
            const v3 e2{ triVerts[ 2 ] - triVerts[ 0 ] };
            const v3 normal{ Cross( e1, e2 ) };
            const float normalLen{ normal.Length() };
            const float area{ normalLen / 2 };
            const v3 radiance{ material->mEmissive };
            const v3 power{ radiance * area * 3.14f };
            const PatchPower patchPower
            {
              .mTriVerts           { triVerts },
              .mTriNormals         { triNormals },
              .mTotalPower         { power },
              .mCurrentUnshotPower { power },
              .mUnitNormal         { normal / normalLen },
              .mArea               { area },
            };
            patchPowers.push_back( patchPower );
          }
          ( ++iTriVert ) %= 3;
        }
        return patchPowers;
        }( ) };

      Instance instance
      {
        .mEntity      { entity },
        .mModel       { model },
        .mMesh        { mesh },
        .mMaterial    { material },
        .mPatchPowers { patchPowers },
      };
      mInstances.push_back(instance);
    }
  }

  auto PreBakeScene::ComputeTotalUnshotPower() -> float
  {
    float total{};
    for( Instance& instance : mInstances )
      for( PatchPower& patchPower : instance.mPatchPowers )
        total += patchPower.GetUnshotPower();
    return total;
  }

  void PreBakeScene::SaveToFile( Errors& errors )
  {
    // deleteme
    if( false)
    {
      mInstances.clear();
      mWorld = TAC_NEW World;

      Entity* entity = mWorld->SpawnEntity( EntityUUID( 1 ) );

      Material* material{ ( Material* )entity->AddNewComponent( Material{}.GetEntry() ) };
      material->mIsGlTF_PBR_MetallicRoughness = true;
      material->mColor = v4( 1, 1, 1, 1 );

      PatchPower::VtxPos vtxPos;//= Array< v3, 3 >;
      vtxPos[0] = v3( .1f, .2f, .3f );
      vtxPos[1] = v3( 10, 0, 0 );
      vtxPos[2] = v3( 0, 10, 0 );

      PatchPower::VtxNor vtxNor; // = Array< v3, 3 >;
      vtxNor[0] = v3( 0,0,1);
      vtxNor[1] = v3( 0,0,1);
      vtxNor[2] = v3( 0,0,1);

      PatchPower     patchPower
      {
        .mTriVerts             { vtxPos },
        .mTriNormals           { vtxNor },
        //v3      mTotalPower           {},
        //v3      mCurrentReceivedPower {},
        //v3      mCurrentUnshotPower   {},
        //v3      mUnitNormal           {},
        .mArea                 { 1 }, // avoid div 0
      };
      PatchPowers     patchPowers{};
      patchPowers.push_back( patchPower );

      mInstances.push_back(
        Instance
        {
          .mEntity      {entity},
          //const Model*    mModel       {};
          //const Mesh*     mMesh        {};
          .mMaterial    { material },
          .mPatchPowers {patchPowers},
        } );
    }


    const int nAttribs{3};

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
    Vector< cgltf_mesh > meshes( mInstances.size() );
    Vector< cgltf_node > nodes( mInstances.size() );
    Vector< cgltf_primitive > primitives( mInstances.size() );
    Vector< cgltf_material > materials( mInstances.size() );
    Vector< cgltf_accessor> accessors( mInstances.size() * nAttribs );
    Vector< cgltf_buffer_view> bufferViews( mInstances.size() * nAttribs );
    Vector< cgltf_attribute> attribs( mInstances.size() * nAttribs );

    struct Vertex
    {
      v3 mPosition {};
      v3 mNormal   {};
      v3 mColor    {};
    };
    Vector< Vertex > vertexes;
    for( const Instance& instance : mInstances )
      vertexes.resize( vertexes.size() + instance.VertexCount() );

    dynmc int iRunningVertex{};

    dynmc cgltf_buffer buffer
    {
      .size { ( cgltf_size )( vertexes.size() * sizeof( Vertex ) )},
      .data { vertexes.data() },
    };

    dynmc char glTF_attribName_POS[]{ "POSITION" };
    dynmc char glTF_attribName_NOR[]{ "NORMAL" };
    dynmc char glTF_attribName_COL[]{ "COLOR_0" };

#if 0
    struct ManagedBuffers
    {
      struct ManagedBuffer : public Vector< v3 >
      {
        int GetByteCount() const { return size() * sizeof( v3 ); }
      };

      ~ManagedBuffers()
      {
        for( ManagedBuffer* b : mBufferVec )
          TAC_DELETE b;
      }

      ManagedBuffer* AllocBuffer()
      {
        ManagedBuffer* b = TAC_NEW ManagedBuffer;
        mBufferVec.push_back(b);
        return b;
      }

      Vector< ManagedBuffer* > mBufferVec;
    } managedBuffers;
#endif

    int i{};
    for( Instance& inst : mInstances )
    {
      char* name{ inst.mEntity->mName.begin() };

      cgltf_attribute*   cglTF_mesh_attributes   { &attribs[ i * nAttribs ] };
      cgltf_buffer_view* cglTF_mesh_buffer_views { &bufferViews[ i * nAttribs ] };
      cgltf_accessor*    cglTF_mesh_accessors    { &accessors[ i * nAttribs ] };
      //cgltf_buffer*      cglTF_mesh_buffers      { &buffers[ i * nAttribs ] };

      cgltf_attribute*   cglTF_running_attribute   { cglTF_mesh_attributes};
      cgltf_buffer_view* cglTF_running_buffer_view { cglTF_mesh_buffer_views};
      cgltf_accessor*    cglTF_running_accessor    { cglTF_mesh_accessors };
      //cgltf_buffer*      cglTF_running_buffer      { cglTF_mesh_buffers};

      const auto GetAttribute_Pos{ []( const PatchPower& patchPower, int i ) -> v3 { return patchPower.mTriVerts[ i ]; } };
      const auto GetAttribute_Nor{ []( const PatchPower& patchPower, int i ) -> v3 { return patchPower.mTriNormals[ i ]; } };
      const auto GetAttribute_Col{ []( const PatchPower& patchPower, int i ) -> v3 {
        if( patchPower.mArea <= 0 )
          return {};

        Linear_scRGB lin_scRGB( patchPower.mTotalPower / patchPower.mArea / 3.14f );

        // assumptions are being made here
        Linear_sRGB lin_sRGB( Saturate( lin_scRGB.r ),
                              Saturate( lin_scRGB.g ),
                              Saturate( lin_scRGB.b ) );

        Encoded_sRGB encoded_sRGB( lin_sRGB );

        // glTF COLOR_0 expects [0,1] (encoded sRGB?)
        return v3( encoded_sRGB.r, encoded_sRGB.g, encoded_sRGB.b );
        } };

      const int iStartVertex{ iRunningVertex };
      for( const PatchPower& patchPower : inst.mPatchPowers )
      {
        for( int i {}; i < 3; ++i )
        {
          Vertex& vtx{ vertexes[ iRunningVertex++ ] };
          vtx = Vertex
          {
            .mPosition { GetAttribute_Pos( patchPower, i ) },
            .mNormal   { GetAttribute_Nor( patchPower, i ) },
            .mColor    { GetAttribute_Col( patchPower, i ) },
          };
          TAC_NO_OP;
          ++asdf;
        }
      }

      using GetAttribteFn = auto ( * )( const PatchPower& patchPower, int i )->v3;

      const auto FillAttribute{
        [ & ]( int vtxAttribOffset,
               dynmc char* glTF_attribName,
               cgltf_attribute_type glTF_attribType )
        {
#if 0
          ManagedBuffers::ManagedBuffer* managedBuffer{ managedBuffers.AllocBuffer() };
          for( const PatchPower& patchPower : inst.mPatchPowers )
          {
            for( int i{}; i < 3; ++i )
            {
              const v3 attrib_v3 { fn( patchPower, i ) };
              managedBuffer->push_back( attrib_v3 );
            }
          }

          const cgltf_size bufferSize{ ( cgltf_size )managedBuffer->GetByteCount() };
          cgltf_buffer& posBuffer { *cglTF_running_buffer++ };
          posBuffer = cgltf_buffer
          {
            .size { bufferSize },
            .data { managedBuffer->data() },
          };

          cgltf_buffer_view& posBufferView { *cglTF_running_buffer_view++};
          posBufferView = cgltf_buffer_view
          {
            .buffer { &posBuffer },
            .size   { bufferSize },
            .type   { cgltf_buffer_view_type_vertices},
          };
#else
          cgltf_buffer_view& posBufferView { *cglTF_running_buffer_view++};
          posBufferView = cgltf_buffer_view
          {
            .buffer { &buffer },
            .offset { ( cgltf_size )( sizeof( Vertex ) * iStartVertex ) },
            .size   { ( cgltf_size )( sizeof( Vertex ) * inst.VertexCount() ) },
            .stride { ( cgltf_size )sizeof( Vertex ) }, // determined by accessor my ass
            .type   { cgltf_buffer_view_type_vertices },
          };
#endif

          cgltf_accessor& accessor_pos { *cglTF_running_accessor++ };
          accessor_pos = cgltf_accessor
          {
            .component_type { cgltf_component_type_r_32f },
            .type           { cgltf_type_vec3 },
            .offset         { ( cgltf_size )vtxAttribOffset },
            .count          { ( cgltf_size )inst.VertexCount() },
            .stride         { ( cgltf_size )sizeof( Vertex ) },
            .buffer_view    { &posBufferView },
          };

          cgltf_attribute& attribute_pos{ *cglTF_running_attribute++ };
          attribute_pos = cgltf_attribute
          {
            .name { glTF_attribName },
            .type { glTF_attribType },
            .data { &accessor_pos },
          };
        } };

      const int vtxOffsetPos{ TAC_OFFSET_OF( Vertex, mPosition ) };
      const int vtxOffsetNor{ TAC_OFFSET_OF( Vertex, mNormal ) };
      const int vtxOffsetCol{ TAC_OFFSET_OF( Vertex, mColor ) };
      FillAttribute( vtxOffsetPos, glTF_attribName_POS, cgltf_attribute_type_position );
      FillAttribute( vtxOffsetNor, glTF_attribName_NOR, cgltf_attribute_type_normal );
      FillAttribute( vtxOffsetCol, glTF_attribName_COL, cgltf_attribute_type_color );

      const cgltf_pbr_metallic_roughness pbr_metallic_roughness
      {
        .base_color_factor // albedo reflectance
        {
          inst.mMaterial->mColor.x,
          inst.mMaterial->mColor.y,
          inst.mMaterial->mColor.z,
          inst.mMaterial->mColor.w,
        },
      };

      const v3 emissive{ inst.mMaterial->mEmissive };
      cgltf_material& material{ materials[ i ] };
      material = cgltf_material
      {
        .name                       { name },
        .has_pbr_metallic_roughness { true },
        .pbr_metallic_roughness     { pbr_metallic_roughness },
        .emissive_factor            { emissive.x, emissive.y, emissive.z },
        .unlit                      { true },
      };

      cgltf_primitive& prim{ primitives[ i ] };
      prim = cgltf_primitive
      {
        .type             { cgltf_primitive_type_triangles },
        .material         { &material },
        .attributes       { cglTF_mesh_attributes },
        .attributes_count { nAttribs },
      };

      cgltf_mesh& mesh{ meshes[ i ] };
      mesh = cgltf_mesh 
      {
        .name             { name },
        .primitives       { &prim },
        .primitives_count { 1 },
      };

      //const m4 m{ inst.mEntity->mWorldTransform };
      cgltf_node& node{ nodes[ i ] };
      node = cgltf_node 
      {
        .name       { name },
        .mesh       { &mesh },
        .has_matrix { true },
        .matrix // transpose tac(row major) to gltf (col major)
        {
          1,0,0,0, // bake in worldspace
          0,1,0,0,
          0,0,1,0,
          0,0,0,1,
          //m.m00, m.m10, m.m20, m.m30,
          //m.m01, m.m11, m.m21, m.m31,
          //m.m02, m.m12, m.m22, m.m32,
          //m.m03, m.m13, m.m23, m.m33,
        },
      };

      ++i;
    }

    cgltf_data data
    {
      //cgltf_file_type file_type;
      //void* file_data;

      //cgltf_asset asset;
      .meshes             { meshes.data() },
      .meshes_count       { ( cgltf_size )meshes.size() },
      .materials          { materials.data() },
      .materials_count    { ( cgltf_size )materials.size() },
      .accessors          { accessors.data() },
      .accessors_count    { ( cgltf_size )accessors.size() },
      .buffer_views       { bufferViews.data()},
      .buffer_views_count { ( cgltf_size )bufferViews.size()},
      .buffers            { &buffer },
      .buffers_count      { 1 },
      .nodes              { nodes.data() },
      .nodes_count        { ( cgltf_size )nodes.size() },

      //cgltf_scene* scenes;
      //cgltf_size scenes_count;

      //cgltf_scene* scene;

      //const char* json;
      //cgltf_size json_size;

      //const void* bin;
      //cgltf_size bin_size;

      //cgltf_memory_options memory; ( cgltf_size )buffers.size()
      //cgltf_file_options file;
      //    // ...
    };

    const cgltf_options options{ .type { cgltf_file_type_glb } };
    const OS::SaveParams saveParams{ .mSuggestedFilename{}, };
    static FileSys::Path path;
    
    if( path.empty() )
    {
      TAC_CALL( path = OS::OSSaveDialog( saveParams, errors ) );
    }

    if( path.empty() )
      return;

    const String pathu8{ path.u8string() };
    const char* szPath{ pathu8.c_str() };

    const cgltf_result result{ cgltf_write_file( &options, szPath, &data ) };
    TAC_RAISE_ERROR_IF( result != cgltf_result_success, glTF_ResultToString( result ) );

    std::ofstream ofs( szPath, std::ios::binary | std::ios::app );
    TAC_RAISE_ERROR_IF( !ofs.is_open(), String() + "Failed to append data to " + path.u8string() );

    //struct Header
    //u32 magic{ 0x46546C67 }; // "glTF"
    //u32 version{ 2 };
    //u32 length;

    const char* padding{ "   " };
    const u32 chunkLen{ ( u32 )buffer.size };
    const u32 chunkType{ 0x004E4942 }; // "BIN"
    ofs.write( ( const char* )&chunkLen, sizeof( chunkLen ) );
    ofs.write( ( const char* )&chunkType, sizeof( chunkType ) );
    ofs.write( ( const char* )buffer.data, ( std::streamsize )buffer.size );
    if( buffer.size % 4 )
      ofs.write( padding, 4 - ( buffer.size % 4 ) );
  }

  void PreBakeScene::Execute( Errors& )
  {
    float minPowerLimit{ 0.01f };
    int maxJacobiIterations{ 100 };
    int samplesPerIteration { 1000 };

    Timer timer{};
    timer.Start();
    TimestampDifference timeAccum{};

    for( int iJacobi{}; iJacobi < maxJacobiIterations; ++iJacobi )
    {
      const float totalUnshotPower{ ComputeTotalUnshotPower() };

      if( totalUnshotPower < minPowerLimit )
        break;


#if 0
      timeAccum += timer.Tick();
      if( timeAccum > 1 )
      {
        timeAccum = {};
        static Errors saveErrors;
        saveErrors.clear();
        SaveToFile(saveErrors);
        if( saveErrors)
        {
          OS::OSDebugBreak();
        }
      }
#endif

      for( Instance& instance : mInstances)
      {
        for( PatchPower& patchPower_src : instance.mPatchPowers)
        {
          // \Delta P_{src}^{(k)}
          const float unshotPower_src{ patchPower_src.GetUnshotPower() };

          // Probability of sampling this light source
          //
          //          \Delta P_src^{(k)}
          // p(src) = ------------------
          //           \Delta P_T
          const float q_src{ unshotPower_src / totalUnshotPower };

          // Number of samples allocated to this light source
          const int N_src{ ( int )( q_src * samplesPerIteration ) };

          for( int sample_src_index{}; sample_src_index < N_src; ++sample_src_index)
          {
            //          1
            // p(x) = -----
            //        A_src
            const v3 samplePoint_src_worldspace{ patchPower_src.GetUniformRandomSurfacePoint() };

            //              cos(theta)
            //   p(omega) = ----------
            //                  pi
            const v3 sampleDir_src_worldspace{ SampleCosineWeightedHemisphere( patchPower_src.mUnitNormal ) };

            //
            // p( src, dst ) = p( src ) * p( dst | src )
            //                 |          |
            //     +-----------+          +-------------------------------------+
            //     |                                                            |
            //     +-> Probability p( src ) of sampling the src patch           |
            //                                                                  |
            //                    \Delta P_src^{(k)}                            |
            //         p( src ) = ------------------                            |
            //                        \Delta P_T                                |
            //                                                                  |
            //     +------------------------------------------------------------+
            //     |          
            //     +-> Probability p( dst ) of sampling the dst patch
            //      
            //         p( dst ) = F_{src, dst}
            //                    |
            //         +----------+
            //         |
            //         +-> Form factor F_{src, dst} describing power emitted by src, received by dst
            //
            //             F_{src, dst} = P_{src, dst}
            //                            |
            //             +--------------+
            //             |
            //             +-> Probability P_{src, dst} of ray originating on patch src landing on patch dst
            //
            //                 P_{src, dst} = \int_{S_{src}} \int_{\Omega_x} \chi_{dst}(x, \Omega) p(x,\Omega) dA_x d \omega_\Omega
            //                                                               |                     |
            //                 +---------------------------------------------+                     |
            //                 |                                                                   |
            //                 +-> 1 or 0 if a ray shot from src at point x into \Omega hits dst   |
            //                                                                                     |
            //                     \chi_{dst}(x, \Omega) = { 1 if the ray hits }                   |
            //                                             { 0 otherwise }                         |
            //                                                                                     |
            //                 +-------------------------------------------------------------------+
            //                 |
            //                 +-> Probability density p(x,omega) of ray
            //                                
            //                     p(x,omega) = p(x) * p(omega)
            //                                  |      |
            //                     +------------+      +-----------------------------+
            //                     |                                                 |
            //                     +-> Probability of x uniformly chosen on src      |
            //                                                                       |
            //                                  1                                    |
            //                         p(x) = -----                                  |
            //                                A_src                                  |
            //                                                                       |
            //                     +-------------------------------------------------+
            //                     |
            //                     +-> Probability p(omega) ray in direction omega, cos weighted
            //
            //                                    cos(theta)
            //                         p(omega) = ----------
            //                                        pi

            // todo: bounce the light around the scene
            const RayTriangle::Ray ray_src_to_dst
            {
              .mOrigin    { samplePoint_src_worldspace },
              .mDirection { sampleDir_src_worldspace },
            };
            const RaycastResult raycastResult{ Raycast( &patchPower_src, ray_src_to_dst ) };
            if( !raycastResult.mHitPatch )
              continue; // \delta_{li}

            PatchPower& patchPower_dst { *raycastResult.mHitPatch };
            const v3 rho_dst { raycastResult.mHitPatchMaterial->mColor.xyz() };

            // [ ] Q: should raycastResult.mHitPatchMaterial->mColor.xyz() be converted from encded srgb to linear srgb?
            const v3 sample_i
            {
              rho_dst *
              ( 1.0f / samplesPerIteration ) * // ??? averaging a bunch of samples ???
              totalUnshotPower // \Delta P_T^{(k)}
            }; 
            patchPower_dst.mCurrentReceivedPower += sample_i;


            /*
               -----------------------------------------------------------------------------
                                          The Monte Carlo Method
               -----------------------------------------------------------------------------
            
               Say we want to compute the sum S of std::vector<int> numbers { 10, 13, 43, 74, 25 }
            
                 S = \sum_{i=1}^{n} a_i
            
               In the monte carlo method, an "estimator" is a random number whose "expectation"
               is equal to the value we are trying to compute. The estimator we will use is:
            
                 \hat{S} = (n a_i, \frac{1}{n})

                 which has value n a_i and probability \frac{1}{n}
            
               This is what it looks like in code:
            
                 #include <vector>
                 #include <iostream>
                 #include <numeric>
                 int main()
                 {
                   std::vector<int> numbers { 10, 13, 43, 74, 25 };
                   std::cout << "actual sum: " << std::accumulate( numbers.begin(), numbers.end(), 0 ) << std::endl;
                   int a_i = numbers[ std::rand() % numbers.size() ];
                   int S = numbers.size() * a_i;
                   std::cout << "monte carlo sum: " << S << std::endl;
                 }
            
               Only the estimator value is used in computing the sum, but it is important that the
               samples are chosen with the specified probability \frac{1}{n}.

               Now this gives kind of a shitty estimate of the sum, but with a monte carlo estimator
               it is kind of assumed that we reduce the variance by averaging multiple samples.

                 #include <vector>
                 #include <iostream>
                 #include <numeric>
                 int main()
                 {
                   std::vector<int> numbers { 10, 13, 43, 74, 25 };
                   std::cout << "actual sum: " << std::accumulate( numbers.begin(), numbers.end(), 0 ) << std::endl;
                   int N = 10000;
                   float S = 0;
                   for( int i = 0; i < N; ++i )
                   {
                     int a_i = numbers[ std::rand() % numbers.size() ];
                     S += numbers.size() * a_i / (float)N;
                   }
                   std::cout << "monte carlo sum: " << S << std::endl;
                 }
            
            */

            /*
               The value we would like to estimate is:
              
                 \Delta P_i^{k+1} = \sum_j \sum_{l \new j } \Delta P_j^{(k)}F_{jl}\rho_l \delta_{li}
              
               So we will use the following estimator
              
                 \Delta \hat{P_i} = ( \rho_i \Delta P_T^{(k)} \delta_{li}, \frac{\Delta P_j^{(k)}}{\Delta P_T^{(k)}} F_{jl} )
              
               In code, what that means is the sample value of \rho_i \Delta P_T^{(k)} \delta_{li}
               is itself an estimate for the i'th PatchPower::mCurrentReceivedPower.

               To reiterate, only the estimator value is used to compute \Delta P_i^{k+1}
               but it is important that the samples are chosen with the specified probabilty.
               \frac{\Delta P_j^{(k)}}{\Delta P_T^{(k)}} F_{jl}
            */


            if( false )
            {
              mDebugLine = true;
              mDebugSrcPos = ray_src_to_dst.mOrigin;
              mDebugDstPos = ray_src_to_dst.mOrigin + ray_src_to_dst.mDirection * raycastResult.mT;
              mDebugSrcPatch = &patchPower_src;
              mDebugDstPatch = &patchPower_dst;
              return;
            }


          } // for each sample
        } // for each patch
      } // for each instance

      for( Instance& instance : mInstances )
      {
        for( PatchPower& patchPower : instance.mPatchPowers)
        {
          patchPower.mTotalPower += patchPower.mCurrentReceivedPower;
          patchPower.mCurrentUnshotPower = patchPower.mCurrentReceivedPower;
          patchPower.mCurrentReceivedPower = {};
        }
      }

    } // for each jacobi iteration
  } // void PreBakeScene::Bake()



} // namespace Tac

