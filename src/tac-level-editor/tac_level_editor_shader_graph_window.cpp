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

  struct VertexShaderOutputElement
  {
  };

  static const Array sSVSemantics
  {
    "SV_POSITION",
  };

  struct VertexShaderOutput
  {
    struct Variable
    {
      String ToString() const
      {
        return String() + mMetaType->GetName() + " " + mName + " : " + mSemantic;
      }

      const MetaType* mMetaType     {};
      String          mName         {};
      String          mSemantic     {};
    };

    Variable* Find( StringView semantic )
    {
      for( Variable& element : mVariables )
        if( ( StringView )element.mSemantic == semantic )
          return &element;
      return nullptr;
    }

    Vector< Variable > mVariables;
  };

  static VertexShaderOutput sVertexShaderOutput;

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

      for( StringView svSemantic : sSVSemantics )
      {
        if( !sVertexShaderOutput.Find( svSemantic ) )
        {
          if( ImGuiButton( "Add " + svSemantic ) )
          {
            const VertexShaderOutput::Variable element{ .mSemantic{ svSemantic } };
            sVertexShaderOutput.mVariables.push_back( element );
          }
        }
      }


      VertexShaderOutput::Variable* toRemove{};
      for( dynmc VertexShaderOutput::Variable& var : sVertexShaderOutput.mVariables )
      {
        ImGuiText( var.ToString() );
        ImGuiSameLine();
        if( ImGuiButton( "Remove" ) )
        {
          toRemove = &var;
        }
      }

      if( toRemove )
      {
        Vector< VertexShaderOutput::Variable > newElements;
        for( VertexShaderOutput::Variable& element : sVertexShaderOutput.mVariables )
          if( &element != toRemove )
            newElements.push_back( element );
        sVertexShaderOutput.mVariables = newElements;
      }


      static MetaType* metaType;
      static String variableName;
      static String semanticName;

      const VertexShaderOutput::Variable var
      {
        .mMetaType{ metaType },
        .mName{variableName},
        .mSemantic{ semanticName },
      };

      String varAsString;
      varAsString += metaType ? metaType->GetName() : "(missing type)";
      varAsString += " ";
      varAsString += variableName.empty() ? "(missing name)" : variableName;
      varAsString += " : ";
      varAsString += semanticName.empty() ? "(missing semantic)" : semanticName;

      ImGuiText( varAsString );

      ImGuiInputText( "Variable Name", variableName );
      ImGuiInputText( "Semantic Name", semanticName );
      

      
      ImGuiText( "--- End Vertex Shader Output --- " );



    if( ImGuiButton( "Open Material" ) )
    {
      const FileSys::Path path{ OS::OSOpenDialog( errors ) };

      ++asdf;

    }


    ImGuiEnd();
  }

} // namespace Tac

