// Collect a tree built by DebugGroup::Stack::Push/Pop commands
// that can be iterated through later,
// thus deferring the actual Render::BeginGroup/EndGroup calls

#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render::DebugGroup
{
  using NodeIndex = int;

  static const NodeIndex NullNodeIndex { -1 };

  struct Node;
  struct Stack;
  struct Iterator;

  struct Node
  {
    ShortFixedString mName {};
    //String    mName   {};

    // number of nodes below this node ( tree grows upwards)
    // roots have a height of 0
    int       mHeight {};
    NodeIndex mSelf   { NullNodeIndex };
    NodeIndex mParent { NullNodeIndex };
  };

  // -----------------------------------------------------------------------------------------------

  struct Iterator
  {
    friend struct Stack;
    ~Iterator();
    Iterator( Render::IContext* = nullptr );

    void Reset( Render::IContext* );

  private:

    bool Contains( const Node* ) const;
    auto back() const -> const Node*;
    bool empty() const;

    bool                  mFinished      {};
    Vector< const Node* > mNodeStack     {}; // actual stack of nodes that were pushed/popped
    Render::IContext*     mRenderContext {};
  };

  // -----------------------------------------------------------------------------------------------

  // This is more of a tree...
  struct Stack
  {
    void Push( StringView );
    void Pop();
    auto GetInfo() const -> NodeIndex;

    auto IterateBegin( Render::IContext* ) const -> Iterator;
    void IterateElement( Iterator&, NodeIndex ) const;
    void IterateEnd( Iterator& ) const;
    void AssertNodeHeights() const;
    bool empty() const;
    void clear();
    auto GetNodes() const -> Span< const Node >;

  private:
    auto FindNode( NodeIndex ) dynmc -> dynmc Node*;
    auto FindNode( NodeIndex ) const -> const Node*;

    Vector< Node > mNodes      {}; // all nodes of a tree, layed out linearly (flattened?)
    NodeIndex      mCurNodeIdx { NullNodeIndex };
  };


} // namespace Tac

