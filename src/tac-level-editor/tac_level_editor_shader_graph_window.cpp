#include "tac_level_editor_shader_graph_window.h" // self-inc
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/meta/tac_meta.h"

namespace Tac
{

  struct MaterialInput
  {
    using UnderlyingType = u32;

    // $$$ instead of creating a custom constant buffer type, just fucking make a uberbuffer
    enum Type : UnderlyingType
    {
      kWorldMatrix,
      kVertexBuffer,
      kCount,
    };

    static StringView ToString( Type mi )
    {
      switch( mi )
      {
      case Type::kWorldMatrix: return "world_matrix";
      case Type::kVertexBuffer: return "vertex_buffer";
      default: TAC_ASSERT_INVALID_CASE( mi ); return "";
      }
    }
    static Type       FromString( StringView s )
    {
      for( int i{}; i < ( int )Type::kCount; ++i )
      {
        const Type type{ ( Type )i };
        if( ( StringView )ToString( type ) == s )
          return type;
      }
      TAC_ASSERT_INVALID_CODE_PATH;
      return Type::kCount;
    }

    void Set( Type t )                                                                              { ( UnderlyingType& )mBitfield |= ( UnderlyingType )( 1 << t  ); }
    void Set( Type t, bool b )                                                                      { if( b ) Set( t ); else Clear( t ); }
    void Clear( Type t )                                                                            { ( UnderlyingType& )mBitfield &= ~( 1 << t  ); }
    void Clear()                                                                                    { mBitfield = {}; }
    bool IsSet( Type t ) const                                                                      { return ( UnderlyingType )mBitfield & ( 1 << t ); }

  private:

    Type mBitfield{};
  };

  struct SVSemantic
  {
    bool Contains(  const MetaType* metaType )const
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


  struct VertexShaderOutput
  {
    struct Variable
    {
      const MetaType* mMetaType     {};
      String          mName         {};
      String          mSemantic     {};
    };


    Vector< Variable > mVariables;
  };




  struct ShaderGraph
  {
    VertexShaderOutput  mVertexShaderOutput {};
    MaterialInput       mMaterialInputs     {};
    String              mMaterialShader     {};
  };

  static ShaderGraph sShaderGraph;

  struct InputLayoutElementEditUI
  {
    void EditVariable(  VertexShaderOutput::Variable* var )

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
          *mToEdit = VertexShaderOutput::Variable
          {
            .mMetaType{ mMetaType },
            .mName    { mVariableNameInput },
            .mSemantic{ displaySemantic }
          };

          EditVariable( nullptr );
        }
      }

    }

    const MetaType* mMetaType          {};
    String          mVariableNameInput {};
    String          mSemanticNameInput {};
    bool            mAutoSemantic      {};

    VertexShaderOutput::Variable* mToEdit {};
  };

  static InputLayoutElementEditUI sInputLayoutElementEditUI;

  static AssetPathString sCurrentFile;

  static const char* sJsonKeyInputLayout     { "input_layout" };
  static const char* sJsonKeyMaterialInputs  { "material_inputs" };

  static const MetaType* FindMetaType( StringView str )
  {
    const Array allMetaTypes
    {
      &GetMetaType<v2>(),
      &GetMetaType<v3>(),
      &GetMetaType<v4>(),
      &GetMetaType<float>(),
      &GetMetaType<u32>(),
    };

    for( const MetaType* metaType : allMetaTypes )
      if( ( StringView )metaType->GetName() == str )
        return metaType;

    return nullptr;
  }

  static VertexShaderOutput VertexShaderOutputFromJson( const Json* inputLayout )
  {
    VertexShaderOutput vso;

    if( inputLayout )
    {
      for( Json* varJson : inputLayout->mArrayElements )
      {
        const String typeStr{ varJson->GetChild( "type" ).mString };

        VertexShaderOutput::Variable var;
        var.mMetaType = FindMetaType(  typeStr );
        var.mName = varJson->GetChild( "name" ).mString;
        var.mSemantic = varJson->GetChild( "semantic" ).mString;

        vso.mVariables.push_back( var );
      }
    }

    return vso;
  }

  static Json VertexShaderOutputToJson( const VertexShaderOutput& vso )
  {
    Json inputLayout;
    for( const VertexShaderOutput::Variable& var : vso.mVariables )
    {
      Json& varJson{ *inputLayout.AddChild() };
      varJson[ "name" ] = ( StringView )var.mName;
      varJson[ "semantic" ] = ( StringView )var.mSemantic;
      varJson[ "type" ] = ( StringView )var.mMetaType->GetName();
    }
    return inputLayout;
  }

  static MaterialInput MaterialInputsFromJson( const Json* materialInputsJson )
  {
    MaterialInput materialInputs{};
    if( materialInputsJson )
    {
      for( Json* varJson : materialInputsJson->mArrayElements )
      {
        materialInputs.Set( MaterialInput::FromString( varJson->mString ) );
      }
    }

    return materialInputs;
  }

  static Json MaterialInputsToJson( const MaterialInput& materialInputs )
  {
    Json json;
    for( int i{}; i < (int)MaterialInput::Type::kCount; ++i)
    {
      const MaterialInput::Type type{ ( MaterialInput::Type )i };
      if( materialInputs.IsSet( type ) )
      {
        *json.AddChild() = MaterialInput::ToString( type );
      }
    }

    return json;
  }

  static Json SaveToJson()
  {
    Json json;
    json[ sJsonKeyInputLayout ] = VertexShaderOutputToJson( sShaderGraph.mVertexShaderOutput );
    json[ sJsonKeyMaterialInputs ] = MaterialInputsToJson( sShaderGraph.mMaterialInputs );
    return json;
  }

  static void LoadFromJson( const Json& json )
  {
    sShaderGraph = {};
    sShaderGraph.mVertexShaderOutput =
      VertexShaderOutputFromJson( json.FindChild( sJsonKeyInputLayout ) );
    sShaderGraph.mMaterialInputs =
      MaterialInputsFromJson( json.FindChild( sJsonKeyMaterialInputs ) );
  }

  static void SaveToPath( AssetPathStringView path, Errors& errors )
  {
    if( path.empty() )
      return;

    sCurrentFile = path;

    const Json json{ SaveToJson() };
    const String string{ json.Stringify() };

    TAC_CALL( SaveToFile( sCurrentFile, string.data(), string.size(), errors ) );
  }

  static void LoadFromPath( AssetPathStringView path, Errors& errors )
  {
    if( path.empty() )
      return;

    sCurrentFile = path;

    TAC_CALL( const String string{ LoadAssetPath( sCurrentFile, errors ) } );

    Json json;
    TAC_CALL( json.Parse( string, errors ) );
    LoadFromJson( json );
  }

  static void InputLayoutImGui( VertexShaderOutput& vso )
  {
    if( !ImGuiCollapsingHeader( "Vertex Shader Output" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;


    VertexShaderOutput::Variable* toRemove{};
    VertexShaderOutput::Variable* toMoveUp{};
    VertexShaderOutput::Variable* toMoveDown{};
    for( dynmc VertexShaderOutput::Variable& var : vso.mVariables )
    {
      const StringView displayType{ var.mMetaType? var.mMetaType->GetName(): "(missing type)" };
      const StringView displayName{ var.mName.empty() ? "(missing name)" : var.mName };
      const StringView displaySemantic{ var.mSemantic.empty() ? "(missing semantic)" : var.mSemantic };

      ImGuiText( displayType );
      ImGuiSameLine();
      ImGuiText( displayName );
      ImGuiSameLine();
      ImGuiText( " : " );
      ImGuiSameLine();
      ImGuiText( displaySemantic );

      ImGuiSameLine();

      if( ImGuiButton( "Edit" ) ) { sInputLayoutElementEditUI.EditVariable( &var ); }

      ImGuiSameLine();

      if( ImGuiButton( "Remove" ) ) { toRemove = &var; }

      ImGuiSameLine();
      if( ImGuiButton( "^" ) ) { toMoveUp = &var; }
      ImGuiSameLine();
      if( ImGuiButton( "v" ) ) { toMoveDown = &var; }
    }

    if( toRemove )
    {
      Vector< VertexShaderOutput::Variable > newElements;
      for( VertexShaderOutput::Variable& element : vso.mVariables )
        if( &element != toRemove )
          newElements.push_back( element );
      vso.mVariables = newElements;
      if( ImGuiButton( "Edit" ) ) { sInputLayoutElementEditUI.EditVariable( nullptr ); }
    }

    if( ImGuiButton( "Add Variable" ) )
    {
      VertexShaderOutput::Variable* var{ &vso.mVariables.emplace_back() };
      sInputLayoutElementEditUI.EditVariable( var );
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

      sInputLayoutElementEditUI.EditVariable(nullptr);
    }

    if( toMoveUp )
    {
      const int i{ (int)(toMoveUp - vso.mVariables.data() )};
      const int j{ i - 1 };
      if( j >= 0 )
      {
        Swap( vso.mVariables[ i ],
              vso.mVariables[ j ] );
      }

      sInputLayoutElementEditUI.EditVariable(nullptr);
    }

    sInputLayoutElementEditUI.ImGui();
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
        ImGuiText( MaterialInput::ToString( miType ) );
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
        if( ImGuiButton( "Add " +  MaterialInput::ToString( miType )  ) )
          mi.Set( miType );
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
      TAC_CALL( SaveToPath( savePath, errors ) );
    }
  }

  static void FileLoadImGui( Errors& errors )
  {
    if( ImGuiButton( "File --> Open" ) )
    {
      TAC_CALL( const AssetPathStringView openPath{ AssetOpenDialog( errors ) } );
      TAC_CALL( LoadFromPath( openPath, errors ) );
    }
  }

  bool CreationShaderGraphWindow::sShowWindow{};

  void CreationShaderGraphWindow::Update( Errors& errors )
  {
    if( !sShowWindow )
      return;

    ImGuiSetNextWindowMoveResize();
    if( !ImGuiBegin( "Shader Graph" ) )
      return;

    sShowWindow |= !ImGuiButton( "Close Window" );

    InputLayoutImGui( sShaderGraph.mVertexShaderOutput );

    MaterialInputImGui( sShaderGraph.mMaterialInputs );

    ImGuiText( "Current shader graph: " +
               ( sCurrentFile.empty() ? ( StringView )"none" : ( StringView )sCurrentFile ) );

    TAC_CALL( FileSaveImGui( errors ) );
    TAC_CALL( FileLoadImGui( errors ) );

    ImGuiEnd();
  }

} // namespace Tac

