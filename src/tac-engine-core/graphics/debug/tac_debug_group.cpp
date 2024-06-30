#include "tac_debug_group.h" // self-inc

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac::Render::DebugGroup
{
  Iterator::Iterator( Render::IContext* context ) : mRenderContext( context ) { }

  Iterator::~Iterator()
  {
    TAC_ASSERT( mFinished );
  }

  bool        Iterator::Contains( const Node* node ) const
  {
    // search back to front
    const int n { mNodeStack.size() };
    for( int i{}; i < n; ++i )
      if( mNodeStack[ n - i - 1 ] == node )
        return true;

    return false;
  }

  const Node* Iterator::back() const
  {
    return mNodeStack.back();
  }

  bool        Iterator::empty() const
  {
    return mNodeStack.empty();
  }

  // -----------------------------------------------------------------------------------------------

  NodeIndex   Stack::GetInfo() const
  {
    return mCurNodeIdx;
  }

  Iterator    Stack::IterateBegin( Render::IContext* context ) const
  {
    TAC_ASSERT_MSG( mCurNodeIdx == NullNodeIndex, "Mismatched Push/Pop calls" );
    return Iterator( context );
  }

    void           Stack::AssertNodeHeights() const
    {

      for( const Render::DebugGroup::Node& node : mNodes )
      {
        ++asdf;
        TAC_ASSERT( node.mHeight < 100 );
        ++asdf;
        TAC_ASSERT( node.mHeight >= 0 );
        ++asdf;
      }
    }

  void        Stack::IterateElement( Iterator& it,
                                     const NodeIndex info ) const
  {
    const Node* node { FindNode( info ) };
    if( !node )
      return;

    TAC_ASSERT( node->mHeight < 100 );

    const Node* commonParent { node };
    while( commonParent && !it.Contains( commonParent ) )
      commonParent = FindNode( commonParent->mParent );

    if( commonParent == node )
      return;

    while( !it.empty() && it.back() != commonParent )
    {
      it.mRenderContext->DebugEventEnd();
      it.mNodeStack.pop_back();
    }

    const int oldNodeCount { it.mNodeStack.size() };
    for( const Node* curNode{ node };
         curNode != commonParent;
         curNode = FindNode( curNode->mParent ) )
      it.mNodeStack.push_back( curNode );

    Tac::Reverse( it.mNodeStack.begin() + oldNodeCount, it.mNodeStack.end() );
    for( int i{ oldNodeCount }; i < it.mNodeStack.size(); ++i )
    {
      const StringView name { it.mNodeStack[ i ]->mName };
      it.mRenderContext->DebugEventBegin( name );
    }
  }

  void        Stack::IterateEnd( Iterator& it ) const
  {
    const int n { it.mNodeStack.size() };
    for( int i{}; i < n; ++i )
      it.mRenderContext->DebugEventEnd();

    it.mNodeStack = {};
    it.mFinished = true;
  }

  Node*       Stack::FindNode( NodeIndex i )
  {
    return i == NullNodeIndex ? nullptr : &mNodes[ i ];
  }

  const Node* Stack::FindNode( NodeIndex i ) const
  {
    return i == NullNodeIndex ? nullptr : &mNodes[ i ];
  }

  void     Stack::Push( StringView str )
  {
    const NodeIndex nodeIndex{ mNodes.size() };

    mNodes.resize( nodeIndex + 1 );

    // Find parent node after resize (or else it can be invalidated)
    const Node* parentNode { FindNode( mCurNodeIdx ) };

    mNodes.back() = Node
    {
      .mName   { str },
      .mHeight { parentNode ? parentNode->mHeight + 1 : 0 },
      .mSelf   { nodeIndex },
      .mParent { mCurNodeIdx },
    };

    mCurNodeIdx = nodeIndex;
  }

  void        Stack::Pop()
  {
    Node* node { FindNode( mCurNodeIdx ) };
    TAC_ASSERT( node );
    mCurNodeIdx = node->mParent;
  }

} // namespace Tac
