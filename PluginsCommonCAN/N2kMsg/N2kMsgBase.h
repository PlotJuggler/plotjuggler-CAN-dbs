#ifndef N2K_MSG_BASE_H_
#define N2K_MSG_BASE_H_

#include "N2kMsgInterface.h"

struct N2kMsgBase : public N2kMsgInterface
{
  N2kMsgBase(const uint32_t frame_id, const double timestamp_secs)
    : frame_id_{ frame_id }
    , pgn_{ (frame_id_ & 0x03FFFF00) >> 8 }
    , source_addr_{ (frame_id_ & 0xFF) }
    , data_len_{ 0 }
    , timestamp_secs_{ timestamp_secs }
  {
  }
  uint32_t GetFrameId() const override
  {
    return frame_id_;
  }
  uint32_t GetPgn() const override
  {
    return { (frame_id_ & 0x03FFFF00) >> 8 };
  }
  size_t GetDataLen() const override
  {
    return data_len_;
  }
  uint32_t GetSourceAddr() const override
  {
    return source_addr_;
  }
  uint32_t GetPduFormat() const override
  {
    return { (frame_id_ >> 16) & 0xFF };
  };
  uint32_t GetPduSpecific() const override
  {
    return { (frame_id_ >> 8) & 0xFF };
  }
  double GetTimeStamp() const override
  {
    return timestamp_secs_;
  }

protected:
  uint32_t frame_id_, pgn_, source_addr_;
  size_t data_len_;
  double timestamp_secs_;
};

#endif  // N2K_MSG_BASE_H_
