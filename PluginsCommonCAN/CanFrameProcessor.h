#ifndef CAN_FRAME_PROCESSOR_H_
#define CAN_FRAME_PROCESSOR_H_

#include <fstream>
#include <vector>

#include <dbcppp/Network.h>
#include <PlotJuggler/plotdata.h>

#include "N2kMsg/N2kMsgStandard.h"
#include "N2kMsg/N2kMsgFast.h"

class CanFrameProcessor
{
public:
  enum CanProtocol
  {
    RAW,
    NMEA2K,
    J1939
  };
  
  // Constructor for loading a single DBC file (for backwards compatibility)
  CanFrameProcessor(std::ifstream& dbc_file, PJ::PlotDataMapRef& data_map, CanProtocol protocol);
  
  // Constructor for loading multiple DBC files
  CanFrameProcessor(const std::vector<std::ifstream>& dbc_files, PJ::PlotDataMapRef& data_map, CanProtocol protocol);

  bool ProcessCanFrame(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                       const double timestamp_secs);

private:
  void LoadAndMergeNetworks(const std::vector<std::ifstream>& dbc_files);
  void InitializeMessagesMap();
  
  bool ProcessCanFrameRaw(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                          const double timestamp_secs);
  bool ProcessCanFrameN2k(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                          const double timestamp_secs);
  bool ProcessCanFrameJ1939(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                            const double timestamp_secs);
  void ForwardN2kSignalsToPlot(const N2kMsgInterface& n2k_msg);

  // Common
  CanProtocol protocol_;

  // Database
  std::unique_ptr<dbcppp::INetwork> can_network_ = nullptr;
  std::unordered_map<uint64_t, const dbcppp::IMessage*> messages_;  // key of the map is dbc_id

  // PJ
  PJ::PlotDataMapRef& data_map_;

  // N2k specialization
  std::set<uint32_t> fast_packet_pgns_set_;
  std::unordered_map<uint32_t, std::unique_ptr<N2kMsgFast>> fast_packets_map_;  // key of the map is the frame_id
  std::unique_ptr<N2kMsgFast> null_n2k_fast_ptr_ = nullptr;
};
#endif  // CAN_FRAME_PROCESSOR_H_
