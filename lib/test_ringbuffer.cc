#include "ringbuffer.h"
#include <gtest/gtest.h>

TEST(RingBuffer, BasicPutGet) {
  uint8_t buf[4];
  ringbuffer_t rb;
  ringbuffer_wrap(&rb, buf, sizeof(buf));

  EXPECT_TRUE(ringbuffer_is_empty(&rb));
  EXPECT_FALSE(ringbuffer_is_full(&rb));
  EXPECT_EQ(ringbuffer_size(&rb), static_cast<size_t>(0U));
  EXPECT_EQ(ringbuffer_capacity(&rb), static_cast<size_t>(3U)); // 4 - 1

  EXPECT_EQ(ringbuffer_put(&rb, 'a'), 0);
  EXPECT_EQ(ringbuffer_size(&rb), static_cast<size_t>(1U));
  EXPECT_FALSE(ringbuffer_is_empty(&rb));

  uint8_t ch;
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, static_cast<uint8_t>('a'));
  EXPECT_TRUE(ringbuffer_is_empty(&rb));
}

TEST(RingBuffer, FullCondition) {
  uint8_t buf[4];
  ringbuffer_t rb;
  ringbuffer_wrap(&rb, buf, sizeof(buf));

  EXPECT_EQ(ringbuffer_put(&rb, '1'), 0);
  EXPECT_EQ(ringbuffer_put(&rb, '2'), 0);
  EXPECT_EQ(ringbuffer_put(&rb, '3'), 0);
  
  EXPECT_TRUE(ringbuffer_is_full(&rb));
  EXPECT_EQ(ringbuffer_size(&rb), static_cast<size_t>(3U));
  
  // This should fail
  EXPECT_EQ(ringbuffer_put(&rb, '4'), -1);
}

TEST(RingBuffer, WrapAround) {
  uint8_t buf[4];
  ringbuffer_t rb;
  ringbuffer_wrap(&rb, buf, sizeof(buf));

  // Fill 2
  ringbuffer_put(&rb, '1');
  ringbuffer_put(&rb, '2');
  
  // Empty 2
  uint8_t ch;
  ringbuffer_get(&rb, &ch);
  ringbuffer_get(&rb, &ch);
  
  // wr_pos should be at 2, rd_pos at 2
  EXPECT_EQ(rb.wr_pos, static_cast<size_t>(2U));
  EXPECT_EQ(rb.rd_pos, static_cast<size_t>(2U));
  
  // Put 2 more to trigger wrap around
  EXPECT_EQ(ringbuffer_put(&rb, '3'), 0); // wr_pos -> 3
  EXPECT_EQ(ringbuffer_put(&rb, '4'), 0); // wr_pos -> 0 (wrap)
  
  EXPECT_EQ(rb.wr_pos, static_cast<size_t>(0U));
  EXPECT_EQ(ringbuffer_size(&rb), static_cast<size_t>(2U));
  
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, static_cast<uint8_t>('3'));
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, static_cast<uint8_t>('4'));
  EXPECT_TRUE(ringbuffer_is_empty(&rb));
}

TEST(RingBuffer, Peek) {
  uint8_t buf[4];
  ringbuffer_t rb;
  ringbuffer_wrap(&rb, buf, sizeof(buf));

  ringbuffer_put(&rb, 'x');
  
  uint8_t ch;
  EXPECT_EQ(ringbuffer_peek(&rb, &ch), 0);
  EXPECT_EQ(ch, static_cast<uint8_t>('x'));
  EXPECT_FALSE(ringbuffer_is_empty(&rb));
  
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, static_cast<uint8_t>('x'));
  EXPECT_TRUE(ringbuffer_is_empty(&rb));
}
