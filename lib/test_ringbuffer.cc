#include "ringbuffer.h"
#include <gtest/gtest.h>

TEST(RingBuffer, BasicPutGet) {
  uint8_t buf[4];
  ringbuffer_t rb;
  ringbuffer_wrap(&rb, buf, sizeof(buf));

  EXPECT_TRUE(ringbuffer_is_empty(&rb));
  EXPECT_FALSE(ringbuffer_is_full(&rb));
  EXPECT_EQ(ringbuffer_size(&rb), 0);
  EXPECT_EQ(ringbuffer_capacity(&rb), 3); // 4 - 1

  EXPECT_EQ(ringbuffer_put(&rb, 'a'), 0);
  EXPECT_EQ(ringbuffer_size(&rb), 1);
  EXPECT_FALSE(ringbuffer_is_empty(&rb));

  uint8_t ch;
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, 'a');
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
  EXPECT_EQ(ringbuffer_size(&rb), 3);
  
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
  EXPECT_EQ(rb.wr_pos, 2);
  EXPECT_EQ(rb.rd_pos, 2);
  
  // Put 2 more to trigger wrap around
  EXPECT_EQ(ringbuffer_put(&rb, '3'), 0); // wr_pos -> 3
  EXPECT_EQ(ringbuffer_put(&rb, '4'), 0); // wr_pos -> 0 (wrap)
  
  EXPECT_EQ(rb.wr_pos, 0);
  EXPECT_EQ(ringbuffer_size(&rb), 2);
  
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, '3');
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, '4');
  EXPECT_TRUE(ringbuffer_is_empty(&rb));
}

TEST(RingBuffer, Peek) {
  uint8_t buf[4];
  ringbuffer_t rb;
  ringbuffer_wrap(&rb, buf, sizeof(buf));

  ringbuffer_put(&rb, 'x');
  
  uint8_t ch;
  EXPECT_EQ(ringbuffer_peek(&rb, &ch), 0);
  EXPECT_EQ(ch, 'x');
  EXPECT_FALSE(ringbuffer_is_empty(&rb));
  
  EXPECT_EQ(ringbuffer_get(&rb, &ch), 0);
  EXPECT_EQ(ch, 'x');
  EXPECT_TRUE(ringbuffer_is_empty(&rb));
}
