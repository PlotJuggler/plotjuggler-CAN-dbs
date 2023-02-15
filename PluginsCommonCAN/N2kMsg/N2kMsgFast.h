#ifndef N2K_MSG_FAST_H_
#define N2K_MSG_FAST_H_

#include <cstring>
#include "N2kMsgBase.h"

struct N2kMsgFast : public N2kMsgBase
{
  N2kMsgFast(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len, const double timestamp_secs,
             const uint8_t fast_packet_data_len)
    : N2kMsgBase(frame_id, timestamp_secs), fast_packet_data_len_{ fast_packet_data_len }
  {
    size_t data_len_clipped = data_len > max_data_len_ ? max_data_len_ : data_len;
    memset(data_, 0xFF, max_data_len_);
    memcpy(&data_, data_ptr, data_len_clipped);
    data_len_ = data_len_clipped;
  }
  N2kMsgFast(const N2kMsgFast&) = delete;
  const uint8_t* GetDataPtr() const override
  {
    return data_;
  }
  bool IsComplete() const
  {
    return fast_packet_data_len_ == data_len_;
  }
  bool AppendData(const uint8_t* src_data_ptr, size_t src_data_len) override
  {
    if (data_len_ < fast_packet_data_len_)
    {
      if (data_len_ + src_data_len > fast_packet_data_len_)
      {
        src_data_len = fast_packet_data_len_ - data_len_;
      }
    }
    else
    {
      src_data_len = 0;
    }
    if (src_data_len > 0)
    {
      memcpy(data_ + data_len_, src_data_ptr, src_data_len);
      data_len_ += src_data_len;
      return true;
    }
    else
    {
      return false;
    }
  };

private:
  const static uint8_t max_data_len_{ 223 };
  uint8_t fast_packet_data_len_;
  uint8_t data_[max_data_len_];
};

#endif  // N2K_MSG_FAST_H_
