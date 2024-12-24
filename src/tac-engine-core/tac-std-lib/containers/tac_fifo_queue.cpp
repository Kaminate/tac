#include "tac_fifo_queue.h" // self-inc

// todo: run this fn and write better test
void Tac::FifoQueueUnitTest()
{
  // Push 3 elements, pop 3 elements
  {
    FifoQueue< int > myQueue;
    myQueue.push( 1 );
    myQueue.push( 2 );
    myQueue.push( 3 );
    TAC_ASSERT( myQueue.front() == 1 );
    TAC_ASSERT( myQueue.back() == 3 );
    myQueue.pop();
    TAC_ASSERT( myQueue.front() == 2 );
    TAC_ASSERT( myQueue.back() == 3 );
    myQueue.pop();
    TAC_ASSERT( myQueue.front() == 3 );
    TAC_ASSERT( myQueue.back() == 3 );
    myQueue.pop();
    TAC_ASSERT( myQueue.empty() );
  }

  // Push 4 elements, pop 2 elements, push 2 element, pop 4 elements
  // (should cause queue to wrap around)
  {
    FifoQueue< int > myQueue;
    myQueue.push( 1 );
    myQueue.push( 2 );
    myQueue.push( 3 );
    myQueue.push( 4 );
    myQueue.pop();
    TAC_ASSERT( myQueue.front() == 2 );
    TAC_ASSERT( myQueue.back() == 4 );
    myQueue.pop();
    TAC_ASSERT( myQueue.front() == 3 );
    TAC_ASSERT( myQueue.back() == 4 );
    myQueue.push( 5 );
    TAC_ASSERT( myQueue.front() == 3 );
    TAC_ASSERT( myQueue.back() == 5 );
    myQueue.push( 6 );

    // Test iterator ability to wrap around
    int i{};
    for( int j : myQueue )
    {
      TAC_ASSERT( i != 0 || j == 3 );
      TAC_ASSERT( i != 1 || j == 4 );
      TAC_ASSERT( i != 2 || j == 5 );
      TAC_ASSERT( i != 3 || j == 6 );
      i++;
    }
    TAC_ASSERT( myQueue.front() == 3 );
    TAC_ASSERT( myQueue.back() == 6 );
    myQueue.pop();
    TAC_ASSERT( myQueue.front() == 4 );
    TAC_ASSERT( myQueue.back() == 6 );
    myQueue.pop();
    TAC_ASSERT( myQueue.front() == 5 );
    TAC_ASSERT( myQueue.back() == 6 );
    myQueue.pop();
    TAC_ASSERT( myQueue.front() == 6 );
    TAC_ASSERT( myQueue.back() == 6 );
    myQueue.pop();
    TAC_ASSERT( myQueue.empty() );
  }
}

