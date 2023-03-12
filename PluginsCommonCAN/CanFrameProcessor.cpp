#include "CanFrameProcessor.h"
#include "N2kMsg/GenericFastPacket.h"

CanFrameProcessor::CanFrameProcessor(std::ifstream& dbc_file, PJ::PlotDataMapRef& data_map, CanProtocol protocol)
  : protocol_{ protocol }, data_map_{ data_map }
{
  can_network_ = dbcppp::INetwork::LoadDBCFromIs(dbc_file);
  messages_.clear();
  for (const dbcppp::IMessage& msg : can_network_->Messages())
  {
    messages_.insert(std::make_pair(msg.Id(), &msg));
    // For N2kMsgFast, MessageSize is certainly larger than 8 bytes
    if (protocol_ == CanProtocol::NMEA2K)
    {
      if (msg.MessageSize() > 8)
      {
        fast_packet_pgns_set_.insert(PGN_FROM_FRAME_ID(msg.Id()));
      }
    }
  }
}

bool CanFrameProcessor::ProcessCanFrame(const uint32_t frame_id, const uint8_t* payload_ptr, const size_t data_len,
                                        double timestamp_secs)
{
  if (can_network_)
  {
    switch (protocol_)
    {
      case CanProtocol::RAW: {
        return ProcessCanFrameRaw(frame_id, payload_ptr, data_len, timestamp_secs);
      }
      case CanProtocol::NMEA2K: {
        return ProcessCanFrameN2k(frame_id, payload_ptr, data_len, timestamp_secs);
      }
      case CanProtocol::J1939: {
        return ProcessCanFrameJ1939(frame_id, payload_ptr, data_len, timestamp_secs);
      }
      default:
        return false;
    }
  }
  else
  {
    return false;
  }
}

bool CanFrameProcessor::ProcessCanFrameRaw(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                                           const double timestamp_secs)
{
  auto messages_iter = messages_.find(frame_id);
  if (messages_iter != messages_.end())
  {
    const dbcppp::IMessage* msg = messages_iter->second;
    for (const dbcppp::ISignal& sig : msg->Signals())
    {
      const dbcppp::ISignal* mux_sig = msg->MuxSignal();
      if (sig.MultiplexerIndicator() != dbcppp::ISignal::EMultiplexer::MuxValue ||
          (mux_sig && (mux_sig->Decode(data_ptr) == sig.MultiplexerSwitchValue())))
      {
        double decoded_val = sig.RawToPhys(sig.Decode(data_ptr));
        auto str = QString("can_frames/%1/%2")
                       .arg(QString::number(msg->Id()), QString::fromStdString(sig.Name()))
                       .toStdString();
        // qCritical() << str.c_str();
        auto it = data_map_.numeric.find(str);
        if (it != data_map_.numeric.end())
        {
          auto& plot = it->second;
          plot.pushBack({ timestamp_secs, decoded_val });
        }
        else
        {
          auto& plot = data_map_.addNumeric(str)->second;
          plot.pushBack({ timestamp_secs, decoded_val });
        }
      }
    }
    return true;
  }
  else
  {
    return false;
  }
}
bool CanFrameProcessor::ProcessCanFrameN2k(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                                           const double timestamp_secs)
{
  N2kMsgStandard n2k_msg(frame_id, data_ptr, data_len, timestamp_secs);

  if (fast_packet_pgns_set_.count(n2k_msg.GetPgn()))
  {
    fp_generic_fast_packet_t fp_unpacked;
    fp_generic_fast_packet_unpack(&fp_unpacked, n2k_msg.GetDataPtr(), FP_GENERIC_FAST_PACKET_LENGTH);

    // Find the current fast packet, return null one if it does not exists.
    auto current_fp_it = fast_packets_map_.find(n2k_msg.GetFrameId());
    auto& current_fp = current_fp_it != fast_packets_map_.end() ? current_fp_it->second : null_n2k_fast_ptr_;

    if (fp_unpacked.chunk_id == FP_GENERIC_FAST_PACKET_CHUNK_ID_FIRST_CHUNK_CHOICE)
    {
      // First chunk's data is only 6 bytes
      fast_packets_map_[n2k_msg.GetFrameId()] = std::make_unique<N2kMsgFast>(
          n2k_msg.GetFrameId(), n2k_msg.GetDataPtr() + 2, 6ul, timestamp_secs, fp_unpacked.len_bytes);
    }
    else
    {
      if (current_fp)
      {
        current_fp->AppendData(n2k_msg.GetDataPtr() + 1, 7ul);
      }
    }
    if (current_fp && current_fp->IsComplete())
    {
      ForwardN2kSignalsToPlot(*current_fp);
      current_fp.release();
      return true;
    }
  }
  else
  {
    ForwardN2kSignalsToPlot(n2k_msg);
    return true;
  }
  return false;
}
bool CanFrameProcessor::ProcessCanFrameJ1939(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                                             const double timestamp_secs)
{
  N2kMsgStandard n2k_msg(frame_id, data_ptr, data_len, timestamp_secs);
  ForwardN2kSignalsToPlot(n2k_msg);
  return true;
}

void CanFrameProcessor::ForwardN2kSignalsToPlot(const N2kMsgInterface& n2k_msg)
{
  // Create frame_id with priority 6 and source address null, since it is the way dbc is defined, is it always?
  uint32_t dbc_id = (n2k_msg.GetPgn() << 8) | 0xFE | (0x06 << 26) | 0x80000000;
  // qCritical() << "frame_id:" << QString::number(dbc_id) << "\tcan_id:" << QString::number(n2k_msg.GetFrameId());
  auto messages_iter = messages_.find(dbc_id);
  if (messages_iter != messages_.end())
  {
    const dbcppp::IMessage* msg = messages_iter->second;
    // qCritical() << "msg_name:" << QString::fromStdString(msg->Name());
    for (const dbcppp::ISignal& sig : msg->Signals())
    {
      const dbcppp::ISignal* mux_sig = msg->MuxSignal();
      if (sig.MultiplexerIndicator() != dbcppp::ISignal::EMultiplexer::MuxValue ||
          (mux_sig && (mux_sig->Decode(n2k_msg.GetDataPtr()) == sig.MultiplexerSwitchValue())))
      {
        double decoded_val = sig.RawToPhys(sig.Decode(n2k_msg.GetDataPtr()));
        auto str = QString("n2k_msg/%1/%2/%3")
                       .arg(QString::fromStdString(msg->Name()), QString::number(n2k_msg.GetSourceAddr()),
                            QString::fromStdString(sig.Name()))
                       .toStdString();
        // qCritical() << str.c_str();
        auto it = data_map_.numeric.find(str);
        if (it != data_map_.numeric.end())
        {
          auto& plot = it->second;
          plot.pushBack({ n2k_msg.GetTimeStamp(), decoded_val });
        }
        else
        {
          auto& plot = data_map_.addNumeric(str)->second;
          plot.pushBack({ n2k_msg.GetTimeStamp(), decoded_val });
        }
      }
    }
  }
}
