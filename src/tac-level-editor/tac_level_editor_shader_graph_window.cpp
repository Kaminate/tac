#include "tac_level_editor_shader_graph_window.h" // self-inc
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/meta/tac_meta.h"

namespace Tac
{
  enum MaterialInput
  {
    kWorldMatrix,
  };

  // ???
  struct VertexShaderOutputElement
  {
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

    const char*                       mSemanticName;
    FixedVector< const MetaType*, 4 > mAllowedTypes;
  };

  struct SVSemantics
  {
    SVSemantics()
    {
      {
        SVSemantic position;
        position.mSemanticName = "SV_POSITION";
        position.mAllowedTypes.push_back( &GetMetaType< v4 >() );
        mSemantics.push_back( position );
      }

    }

    const SVSemantic* Find( StringView semantic )
    {
      for( const SVSemantic& element : mSemantics )
        if( ( StringView )element.mSemanticName == semantic )
          return &element;
      return nullptr;
    }

    Vector< SVSemantic > mSemantics;
  };

  static SVSemantics sSVSemantics;

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

  static VertexShaderOutput sVertexShaderOutput;

  struct EditUI
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
          for( const SVSemantic& semantic : sSVSemantics.mSemantics )
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

      if( const SVSemantic * svSemantic{ sSVSemantics.Find( displaySemantic ) } )
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

  static EditUI sEditUI;


  bool CreationShaderGraphWindow::sShowWindow{};

  void CreationShaderGraphWindow::Update( Errors& errors )
  {
    if( !sShowWindow )
      return;

    ImGuiSetNextWindowMoveResize();
    if( !ImGuiBegin( "Shader Graph" ) )
      return;

    sShowWindow |= !ImGuiButton( "Close Window" );


    ImGuiText( "--- Begin Vertex Shader Output --- " );

    VertexShaderOutput::Variable* toRemove{};
    VertexShaderOutput::Variable* toMoveUp{};
    VertexShaderOutput::Variable* toMoveDown{};
    for( dynmc VertexShaderOutput::Variable& var : sVertexShaderOutput.mVariables )
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

      if( ImGuiButton( "Edit" ) ) { sEditUI.EditVariable( &var ); }

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
      for( VertexShaderOutput::Variable& element : sVertexShaderOutput.mVariables )
        if( &element != toRemove )
          newElements.push_back( element );
      sVertexShaderOutput.mVariables = newElements;
      if( ImGuiButton( "Edit" ) ) { sEditUI.EditVariable( nullptr ); }
    }

    if( ImGuiButton( "Add Variable" ) )
    {
      VertexShaderOutput::Variable* var{ &sVertexShaderOutput.mVariables.emplace_back() };
      sEditUI.EditVariable( var );
    }

    if( toMoveDown )
    {
      const int i{ ( int )( toMoveDown - sVertexShaderOutput.mVariables.data() ) };
      const int j{ i + 1 };
      const int n{ sVertexShaderOutput.mVariables.size() };
      if( j < n )
      {
        Swap( sVertexShaderOutput.mVariables[ i ],
              sVertexShaderOutput.mVariables[ j ] );
      }

      sEditUI.EditVariable(nullptr);
    }

    if( toMoveUp )
    {
      const int i{ (int)(toMoveUp - sVertexShaderOutput.mVariables.data() )};
      const int j{ i - 1 };
      if( j >= 0 )
      {
        Swap( sVertexShaderOutput.mVariables[ i ],
              sVertexShaderOutput.mVariables[ j ] );
      }

      sEditUI.EditVariable(nullptr);
    }

    sEditUI.ImGui();

    ImGuiText( "--- End Vertex Shader Output --- " );



    if( ImGuiButton( "Open Material" ) )
    {
      const FileSys::Path path{ OS::OSOpenDialog( errors ) };

      ++asdf;

    }


    ImGuiEnd();
  }

} // namespace Tac

