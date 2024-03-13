// Collect a tree built by DebugGroup::Stack::Push/Pop commands
// that can be iterated through later,
// thus deferring the actual Render::BeginGroup/EndGroup calls

#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac::Render::DebugGroup
{
  using NodeIndex = int;

  static const NodeIndex NullNodeIndex = -1;

  struct Node;
  struct Stack;
  struct Iterator;

  struct Node
  {
    String    mName;

    // number of nodes below this node ( tree grows upwards)
    // roots have a height of 0
    int       mHeight = 0;
    NodeIndex mSelf = NullNodeIndex;
    NodeIndex mParent = NullNodeIndex;
  };

  // -----------------------------------------------------------------------------------------------

  struct Iterator
  {
    friend struct Stack;
    ~Iterator();

  private:
    Iterator() = default; // only constructable by Stack::IterateBegin()

    bool                  Contains( const Node* ) const;
    const Node*           back() const;
    bool                  empty() const;

    bool                  mFinished = false;
    Vector< const Node* > mNodeStack; // actual stack of nodes that were pushed/popped
  };

  // -----------------------------------------------------------------------------------------------

  // This is more of a tree...
  struct Stack
  {
    NodeIndex      GetInfo() const;
    void           Push( const StringView& );
    String&        Push();
    void           Pop();

    Iterator       IterateBegin() const;
    void           IterateElement( Iterator&, NodeIndex, const StackFrame& ) const;
    void           IterateEnd( Iterator&, const StackFrame& );

  private:
    Node*          FindNode( NodeIndex );
    const Node*    FindNode( NodeIndex ) const;

    Vector< Node > mNodes; // all nodes of a tree, layed out linearly (flattened?)
    NodeIndex      mCurNodeIdx = NullNodeIndex;
  };


} // namespace Tac

