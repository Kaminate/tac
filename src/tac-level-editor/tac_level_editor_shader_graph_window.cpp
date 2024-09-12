#include "tac_level_editor_shader_graph_window.h" // self-inc
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-ecs/graphics/material/tac_shader_graph.h"

namespace Tac
{
  // Helper struct that defines the allowed data types for system-value semantic
  struct SVSemantic
  {
    bool Contains( const MetaType* metaType )const
    {
      for( const MetaType* mt : mAllowedTypes )
        if( mt == metaType )
          return true;
      return false;
    }

    static Span< const SVSemantic > GetSemantics()
    {
      static Vector< SVSemantic > mSemantics;
      if( mSemantics.empty() )
      {
        SVSemantic position;
        position.mSemanticName = "SV_POSITION";
        position.mAllowedTypes.push_back( &GetMetaType< v4 >() );
        mSemantics.push_back( position );
      }

      return Span< const SVSemantic >( mSemantics.data(), mSemantics.size() );
    }

    static const SVSemantic*        Find( StringView semantic )
    {
      for( const SVSemantic& element : GetSemantics() )
        if( ( StringView )element.mSemanticName == semantic )
          return &element;

      return nullptr;
    }

    const char*                       mSemanticName;
    FixedVector< const MetaType*, 4 > mAllowedTypes;
  };

  // Helper struct to edit a MaterialVSOut::Variable*
  struct MaterialVSOutEditUI
  {
    void EditVariable( MaterialVSOut::Variable* var )

    {
      mToEdit = var;
      if( var )
      {
        mAutoSemantic = var->mName == var->mSemantic;
        mMetaType = var->mMetaType;
        mVariableNameInput = var->mName;
        mSemanticNameInput = var->mSemantic;
      }
    }

    void ImGui()
    {
      if( !mToEdit )
        return;

      ImGuiText( "Edit Variable" );
      TAC_IMGUI_INDENT_BLOCK;

      const StringView displaySemantic{ mAutoSemantic ? mVariableNameInput : mSemanticNameInput };

      String varAsString;
      varAsString += mMetaType ? mMetaType->GetName() : "(missing type)";
      varAsString += " ";
      varAsString += mVariableNameInput .empty() ? "(missing name)" : mVariableNameInput;
      varAsString += " : ";
      varAsString += displaySemantic.empty() ? ( StringView )"(missing semantic)" : displaySemantic;

      ImGuiText( varAsString );

      ImGuiInputText( "Variable Name", mVariableNameInput );
      ImGuiCheckbox( "Auto semantic", &mAutoSemantic );
      if( !mAutoSemantic )
      {
        ImGuiInputText( "Semantic Name", mSemanticNameInput );
        if( ImGuiCollapsingHeader( "SV_SEMANTIC" ) )
        {
          TAC_IMGUI_INDENT_BLOCK;
          for( const SVSemantic& semantic :  SVSemantic::GetSemantics()  )
          {
            if( ImGuiButton( semantic.mSemanticName ) )
            {
              mSemanticNameInput = semantic.mSemanticName;
              if( !semantic.Contains( mMetaType ) )
                mMetaType = nullptr;

              if( !mMetaType && semantic.mAllowedTypes.size() == 1 )
                mMetaType = semantic.mAllowedTypes[ 0 ];

            }
          }
        }
      }

      const Array allMetaTypes
      {
        &GetMetaType<v2>(),
        &GetMetaType<v3>(),
        &GetMetaType<v4>(),
        &GetMetaType<float>(),
        &GetMetaType<u32>(),
      };

      Span< const MetaType* const > allowedMetaTypes( allMetaTypes.data(),
                                                      allMetaTypes.size() );

      if( const SVSemantic * svSemantic{ SVSemantic::Find( displaySemantic ) } )
      {
        allowedMetaTypes = Span< const MetaType* const >( svSemantic->mAllowedTypes.data(),
                                                          svSemantic->mAllowedTypes.size() );
      }

      if( allowedMetaTypes.size() > 1 && ImGuiCollapsingHeader( "Select type" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;

        for( const MetaType* mT : allowedMetaTypes )
        {
          if( ImGuiButton( mT->GetName() ) )
          {
            mMetaType = mT;
          }
        }
      }

      if( mMetaType && !mVariableNameInput.empty() && !displaySemantic.empty() )
      {
        if( ImGuiButton( "Apply Edits" ) )
        {
          *mToEdit = MaterialVSOut::Variable
          {
            .mMetaType { mMetaType },
            .mName     { mVariableNameInput },
            .mSemantic { displaySemantic }
          };

          EditVariable( nullptr );
        }
      }

    }

    const MetaType*          mMetaType          {};
    String                   mVariableNameInput {};
    String                   mSemanticNameInput {};
    bool                     mAutoSemantic      {};

    MaterialVSOut::Variable* mToEdit            {};
  };

  // -----------------------------------------------------------------------------------------------

  static ShaderGraph              sShaderGraph;
  static MaterialVSOutEditUI      sVSOutEditUI;
  static AssetPathString          sCurrentFile;

  // -----------------------------------------------------------------------------------------------

  static void MaterialVSOutImGui( MaterialVSOut& vso )
  {
    if( !ImGuiCollapsingHeader( "Vertex Shader Output" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;


    MaterialVSOut::Variable* toRemove{};
    MaterialVSOut::Variable* toMoveUp{};
    MaterialVSOut::Variable* toMoveDown{};
    for( dynmc MaterialVSOut::Variable& var : vso.mVariables )
    {
      const StringView displayType{ var.mMetaType? var.mMetaType->GetName(): "(missing type)" };
      const StringView displayName{ var.mName.empty() ? "(missing name)" : var.mName };
      const StringView displaySemantic{
        var.mSemantic.empty() ? "(missing semantic)" : var.mSemantic };

      ImGuiText( displayType );
      ImGuiSameLine();
      ImGuiText( displayName );
      ImGuiSameLine();
      ImGuiText( " : " );
      ImGuiSameLine();
      ImGuiText( displaySemantic );

      ImGuiSameLine();

      if( ImGuiButton( "Edit" ) ) { sVSOutEditUI.EditVariable( &var ); }

      ImGuiSameLine();

      if( ImGuiButton( "Remove" ) ) { toRemove = &var; }

      if( vso.mVariables.size() > 1 )
      {
        ImGuiSameLine();
        if( ImGuiButton( "^" ) ) { toMoveUp = &var; }
        ImGuiSameLine();
        if( ImGuiButton( "v" ) ) { toMoveDown = &var; }
      }
    }

    if( toRemove )
    {
      Vector< MaterialVSOut::Variable > newElements;
      for( MaterialVSOut::Variable& element : vso.mVariables )
        if( &element != toRemove )
          newElements.push_back( element );

      vso.mVariables = newElements;
      if( ImGuiButton( "Edit" ) ) { sVSOutEditUI.EditVariable( nullptr ); }
    }

    if( ImGuiButton( "Add Variable" ) )
    {
      MaterialVSOut::Variable* var{ &vso.mVariables.emplace_back() };
      sVSOutEditUI.EditVariable( var );
    }

    if( toMoveDown )
    {
      const int i{ ( int )( toMoveDown - vso.mVariables.data() ) };
      const int j{ i + 1 };
      const int n{ vso.mVariables.size() };
      if( j < n )
      {
        Swap( vso.mVariables[ i ],
              vso.mVariables[ j ] );
      }

      sVSOutEditUI.EditVariable(nullptr);
    }

    if( toMoveUp )
    {
      const int i{ ( int )( toMoveUp - vso.mVariables.data() ) };
      const int j{ i - 1 };
      if( j >= 0 )
      {
        Swap( vso.mVariables[ i ],
              vso.mVariables[ j ] );
      }

      sVSOutEditUI.EditVariable(nullptr);
    }

    sVSOutEditUI.ImGui();
  }

  static void MaterialInputImGui( MaterialInput& mi )
  {
    if( !ImGuiCollapsingHeader("Material Inputs") )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    const int n{ ( int )MaterialInput::Type::kCount };
    for( int i{}; i < n; ++i )
    {
      const MaterialInput::Type miType{ ( MaterialInput::Type )i };
      if( mi.IsSet( miType ) )
      {
        ImGuiText( MaterialInput::Type_to_String( miType ) );
        ImGuiSameLine();
        if( ImGuiButton( "Remove" ) )
          mi.Clear( miType );
      }
    }

    for( int i{}; i < n; ++i )
    {
      const MaterialInput::Type miType{ ( MaterialInput::Type )i };
      if( !mi.IsSet( miType ) )
      {
        if( ImGuiButton( "Add " + MaterialInput::Type_to_String( miType ) ) )
        {
          mi.Set( miType );
        }
      }
    }

  }

  static void ShaderGraphImGui( ShaderGraph& sg, Errors& errors )
  {
    if( !ImGuiCollapsingHeader( "Shader Graph" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;
    MaterialVSOutImGui( sg.mMaterialVSOut );

    MaterialInputImGui( sg.mMaterialInputs );

    ImGuiText( "Material Shader: " + ( sg.mMaterialShader.empty() ? "n/a" : sg.mMaterialShader ) );
    ImGuiSameLine();
    if( ImGuiButton( "Select" ) )
    {
      TAC_CALL( dynmc String assetPath{ AssetOpenDialog( errors ) } );
      if( !assetPath.empty() )
      {
        const Render::ProgramAttribs programAttribs{
          Render::RenderApi::GetRenderDevice()->GetInfo().mProgramAttribs };

        if( assetPath.starts_with( programAttribs.mDir ) )
          assetPath = assetPath.substr( programAttribs.mDir.size() );

        if( assetPath.ends_with( programAttribs.mExt ) )
          assetPath = assetPath.substr( 0, assetPath.size() - programAttribs.mExt.size() );

        //const StringView ext{ assetPath.GetFileExtension() };
        sg.mMaterialShader = assetPath; //.substr( 0, assetPath.size() - ext.size() );
      }
    }
    if( !sg.mMaterialShader.empty() )
    {
      ImGuiSameLine();

      const Render::ProgramAttribs programAttribs{
        Render::RenderApi::GetRenderDevice()->GetInfo().mProgramAttribs };

      const AssetPathString assetPath{
        programAttribs.mDir + sg.mMaterialShader + programAttribs.mExt };

      if( ImGuiButton( "Open " + assetPath ) )
      {
        //FileSys::Path path{ sg.mMaterialShader };
        //FileSys::Path dir{ path.parent_path() };

        //TAC_CALL( FileSys::Paths paths{
        //  FileSys::IterateFiles( dir, FileSys::IterateType::Default, errors ) } );

        OS::OSOpenPath( assetPath, errors );
      }
    }
  }

  static void FileSaveImGui( Errors& errors )
  {
    AssetPathString savePath{ sCurrentFile };
    bool shouldSave{};

    if( ImGuiButton( "File --> Save As" ) )
    {
      const AssetSaveDialogParams saveParams{};
      TAC_CALL( savePath = AssetSaveDialog( saveParams, errors ) );
      shouldSave = !savePath.empty();
    }

    if( !sCurrentFile.empty() &&  ImGuiButton( "File --> Save" ) )
    {
      shouldSave = true;
    }

    if( shouldSave )
    {
      sCurrentFile = savePath;
      TAC_CALL( ShaderGraph::FileSave( sShaderGraph, savePath, errors ) );
    }
  }

  static void FileLoadImGui( Errors& errors )
  {
    if( !ImGuiButton( "File --> Open" ) )
      return;

    TAC_CALL( const AssetPathStringView openPath{ AssetOpenDialog( errors ) } );
    if( openPath.empty() )
      return;

    TAC_CALL( sShaderGraph = ShaderGraph::FileLoad( openPath, errors ) );
    sCurrentFile = openPath;
  }

  // -----------------------------------------------------------------------------------------------

  bool CreationShaderGraphWindow::sShowWindow{};

  void CreationShaderGraphWindow::Update( Errors& errors )
  {
    if( !sShowWindow )
      return;

    ImGuiSetNextWindowMoveResize();
    if( !ImGuiBegin( "Shader Graph" ) )
      return;

    sShowWindow |= !ImGuiButton( "Close Window" );

    TAC_CALL( ShaderGraphImGui( sShaderGraph, errors ) );

    ImGuiText( "Current shader graph: " +
               ( sCurrentFile.empty() ? ( StringView )"none" : ( StringView )sCurrentFile ) );

    TAC_CALL( FileSaveImGui( errors ) );
    TAC_CALL( FileLoadImGui( errors ) );

    ImGuiEnd();
  }

} // namespace Tac

