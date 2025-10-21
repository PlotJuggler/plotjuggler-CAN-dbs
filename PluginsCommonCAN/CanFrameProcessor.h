#ifndef CAN_FRAME_PROCESSOR_H_
#define CAN_FRAME_PROCESSOR_H_

#include <fstream>
#include <unordered_map>
#include <vector>

#include <PlotJuggler/plotdata.h>

#include <dbcppp/Network.h>

#include "N2kMsg/N2kMsgFast.h"
#include "N2kMsg/N2kMsgStandard.h"

class CanFrameProcessor {
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

  // Enable/disable enhanced metadata usage
  void setUseEnhancedMetadata(bool enable) {
    use_enhanced_metadata_ = enable;
  }
  bool isUsingEnhancedMetadata() const {
    return use_enhanced_metadata_;
  }

private:
  // Structure to hold signal metadata
  struct SignalMetadata {
    std::string name;
    double min;  // TODO: min/max could be used to set the min / max values of the plot automatically
    double max;
    std::string unit;
    std::unordered_map<int64_t, std::string> value_encodings;
  };

  void LoadAndMergeNetworks(const std::vector<std::ifstream>& dbc_files);
  void InitializeMessagesMap();
  void InitializeSignalMetadata();

  bool ProcessCanFrameRaw(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                          const double timestamp_secs);
  bool ProcessCanFrameN2k(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                          const double timestamp_secs);
  bool ProcessCanFrameJ1939(const uint32_t frame_id, const uint8_t* data_ptr, const size_t data_len,
                            const double timestamp_secs);
  void ForwardN2kSignalsToPlot(const N2kMsgInterface& n2k_msg);

  // Get enum string for a value if available
  std::string GetEnumString(const dbcppp::ISignal& sig, double value);

  // Format signal name with metadata
  std::string FormatSignalName(const dbcppp::ISignal& sig, const dbcppp::IMessage& msg, double value);

  // Create metadata for a signal
  SignalMetadata CreateSignalMetadata(const dbcppp::ISignal& sig);

  // Common
  CanProtocol protocol_;
  bool use_enhanced_metadata_ = true;

  // Database
  std::unique_ptr<dbcppp::INetwork> can_network_ = nullptr;
  std::unordered_map<uint64_t, const dbcppp::IMessage*> messages_;   // key of the map is dbc_id
  std::unordered_map<std::string, SignalMetadata> signal_metadata_;  // key is unique signal identifier

  // PJ
  PJ::PlotDataMapRef& data_map_;

  // N2k specialization
  std::set<uint32_t> fast_packet_pgns_set_;
  std::unordered_map<uint32_t, std::unique_ptr<N2kMsgFast>> fast_packets_map_;  // key of the map is the frame_id
  std::unique_ptr<N2kMsgFast> null_n2k_fast_ptr_ = nullptr;
};
#endif  // CAN_FRAME_PROCESSOR_H_
