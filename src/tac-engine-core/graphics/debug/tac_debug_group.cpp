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
    const int n = mNodeStack.size();
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
      StringView name { it.mNodeStack[ i ]->mName };
      it.mRenderContext->DebugEventBegin( name );
    }
  }

  void        Stack::IterateEnd( Iterator& it )
  {
    const int n = it.mNodeStack.size();
    for( int i{}; i < n; ++i )
      it.mRenderContext->DebugEventEnd();

    it.mNodeStack = {};
    it.mFinished = true;
  }

  void        Stack::Push( const StringView& name )
  {
    Push() = name;
  }

  Node*       Stack::FindNode( NodeIndex i )
  {
    return  i == NullNodeIndex ? nullptr : &mNodes[ i ];
  }

  const Node* Stack::FindNode( NodeIndex i ) const
  {
    return i == NullNodeIndex ? nullptr : &mNodes[ i ];
  }

  String&     Stack::Push()
  {
    const Node* parentNode = FindNode( mCurNodeIdx );
    const int nodeCount = mNodes.size();
    const NodeIndex nodeIndex = nodeCount;

    mNodes.resize( nodeCount + 1 );

    Node& node = mNodes.back();
    node.mHeight = parentNode ? parentNode->mHeight + 1 : 0;
    node.mParent = mCurNodeIdx;
    node.mSelf = nodeIndex;

    mCurNodeIdx = nodeIndex;

    return node.mName;
  }

  void        Stack::Pop()
  {
    Node* node = FindNode( mCurNodeIdx );
    TAC_ASSERT( node );
    mCurNodeIdx = node->mParent;
  }

} // namespace Tac
