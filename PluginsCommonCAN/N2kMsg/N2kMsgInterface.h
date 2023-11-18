#ifndef N2K_MSG_INTERFACE_H_
#define N2K_MSG_INTERFACE_H_

#include <cstdint>
#include <stddef.h>

// When PDUF1, clear Destination addr
#define PGN_FROM_FRAME_ID(frame_id) uint32_t( ((frame_id >> 16) & 0xFF) >= 240 ? ((frame_id >> 8) & 0x03FFFF) : ((frame_id >> 8) & 0x03FF00) )

struct N2kMsgInterface
{
  virtual uint32_t GetFrameId() const = 0;
  virtual uint32_t GetPgn() const = 0;
  virtual size_t GetDataLen() const = 0;
  virtual uint32_t GetSourceAddr() const = 0;
  virtual uint32_t GetPduFormat() const = 0;
  virtual uint32_t GetPduSpecific() const = 0;
  virtual const uint8_t* GetDataPtr() const = 0;
  virtual double GetTimeStamp() const = 0;

  virtual bool IsComplete() const = 0;
  virtual bool AppendData(const uint8_t* src_data_ptr, size_t src_data_len) = 0;
};

#endif  // N2K_MSG_INTERFACE_H_
