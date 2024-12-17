#include "tac_fifo_queue.h" // self-inc

// todo: run this fn and write better test
void Tac::FifoQueueUnitTest()
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
}

