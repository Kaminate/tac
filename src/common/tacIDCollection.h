namespace Tac
{
  struct IdCollection
  {
    void            Init( int capacity );
    void            Uninit();
    int             Alloc();
    void            Free( int id );
    int*            begin();
    int*            end();
  private:
    //ResourceId      AllocFreeId( StringView name, Tac::StackFrame frame );
    //ResourceId      AllocNewId( StringView name, Tac::StackFrame frame );
    int*            GetDense();
    int*            GetSparse();

    int             mCapacity = 0;
    int*            mData = nullptr;
    int             mSize = 0;
  };

  void UnitTestIdCollection();

  //template< int N > int Size() { return mAllocCounter; };
  //template< int N > ResourceId IdCollection<N>::Alloc( StringView name, Tac::StackFrame frame )
  //{
  //  return mFreeCount ? AllocFreeId( name, frame ) : AllocNewId( name, frame );
  //}

  //template< int N > void IdCollection<N>::Free( ResourceId id )
  //{
  //  TAC_ASSERT( ( unsigned )id < ( unsigned )mAllocCounter );
  //  TAC_ASSERT( !Contains( mFree, mFree + mFreeCount, id ) );
  //  mFree[ mFreeCount++ ] = id;
  //  TAC_ASSERT( mFreeCount <= N );
  //}

  //template< int N > ResourceId IdCollection<N>::AllocFreeId( StringView name, Tac::StackFrame frame )
  //{
  //  const ResourceId result = mFree[ --mFreeCount ];
  //  mNames[ result ] = name;
  //  mFrames[ result ] = frame;
  //  return result;
  //}

  //template< int N > ResourceId IdCollection<N>::AllocNewId( StringView name, Tac::StackFrame frame )
  //{
  //  TAC_ASSERT( mAllocCounter < N );
  //  mNames[ mAllocCounter ] = name;
  //  mFrames[ mAllocCounter ] = frame;
  //  return mAllocCounter++;
  //}
}
