#ifndef N2K_MSG_STANDARD_H_
#define N2K_MSG_STANDARD_H_

#include <cstring>
#include "N2kMsgBase.h"

struct N2kMsgStandard : public N2kMsgBase
{
  N2kMsgStandard(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len, const double timestamp_secs)
    : N2kMsgBase(frame_id, timestamp_secs)
  {
    size_t data_len_clipped = data_len > max_data_len_ ? max_data_len_ : data_len;
    memset(data_, 0xFF, max_data_len_);
    memcpy(&data_, data_ptr, data_len_clipped);
    data_len_ = data_len_clipped;
  }
  N2kMsgStandard(const N2kMsgStandard&) = delete;
  const uint8_t* GetDataPtr() const override
  {
    return data_;
  }
  bool IsComplete() const
  {
    return true;
  }
  bool AppendData(const uint8_t* src_data_ptr, size_t src_data_len) override
  {
    // Standard msg consist of 8 byte max, so returning false on append data.
    return false;
  };

private:
  const static uint8_t max_data_len_{ 8 };
  uint8_t data_[max_data_len_];
};

#endif  // N2K_MSG_STANDARD_H_
