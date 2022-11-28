/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsgnb/ran/band_helper.h"
#include "srsgnb/ran/bs_channel_bandwidth.h"
#include "srsgnb/ran/pdcch/aggregation_level.h"
#include "srsgnb/ran/subcarrier_spacing.h"
#include <string>
#include <vector>

namespace srsgnb {

/// RF cell driver configuration.
struct rf_driver_appconfig {
  /// Sampling frequency in MHz.
  double srate_MHz = 61.44;
  /// RF driver name.
  std::string device_driver = "zmq";
  /// RF driver arguments.
  std::string device_args = "tx_port=tcp://*:5000,rx_port=tcp://localhost:6000";
};

/// PRACH application configuration.
struct prach_appconfig {
  unsigned config_index          = 1;
  unsigned prach_root_sequence   = 1;
  unsigned zero_correlation_zone = 2;
  unsigned fixed_msg3_mcs        = 0;
  unsigned max_msg3_harq_retx    = 4;
};

/// PDCCH application configuration.
struct pdcch_appconfig {
  /// Aggregation level for the UE.
  aggregation_level ue_aggregation_level_index = aggregation_level::n4;
  /// Aggregation level for the RAR.
  aggregation_level rar_aggregation_level_index = aggregation_level::n4;
  /// Aggregation level for the SI.
  aggregation_level si_aggregation_level_index = aggregation_level::n4;
};

/// PDSCH application configuration.
struct pdsch_appconfig {
  /// UE modulation and coding scheme index.
  unsigned fixed_ue_mcs = 10;
  /// RAR modulation and coding scheme index.
  unsigned fixed_rar_mcs = 0;
  /// SI modulation and coding scheme index.
  unsigned fixed_si_mcs = 5;
};

/// PUSCH application configuration.
struct pusch_appconfig {
  /// UE modulation and coding scheme index.
  unsigned fixed_ue_mcs = 15;
};

/// PUCCH application configuration.
struct pucch_appconfig {};

/// CSI-RS application configuration.
struct csi_appconfig {};

/// Base cell configuration.
struct base_cell_appconfig {
  /// Downlink arfcn.
  unsigned dl_arfcn = 536020;
  /// NR band.
  nr_band band = nr_band::n7;
  /// Channel bandwidth in MHz.
  bs_channel_bandwidth_fr1 channel_bw_mhz = bs_channel_bandwidth_fr1::MHz20;
  /// Number of antennas in downlink.
  unsigned nof_antennas_dl = 1;
  /// Number of antennas in uplink.
  unsigned nof_antennas_ul = 1;
  /// PLMN id
  std::string plmn_id = "0x00f110";
  /// TAC.
  int tac = 7;
  /// PDCCH configuration.
  pdcch_appconfig pdcch_cfg;
  /// PDSCH configuration.
  pdsch_appconfig pdsch_cfg;
  /// CSI-RS configuration.
  csi_appconfig csi_cfg;
  /// PRACH configuration.
  prach_appconfig prach_cfg;
  /// PUCCH configuration.
  pucch_appconfig pucch_cfg;
  /// PUSCH configuration.
  pusch_appconfig pusch_cfg;
};

/// Cell configuration
struct cell_appconfig {
  /// Physical cell identifier.
  unsigned pci = 1;
  /// Cell configuration.
  base_cell_appconfig cell;
};

struct amf_appconfig {
  std::string ip_addr   = "127.0.0.1";
  uint16_t    port      = 38412;
  std::string bind_addr = "127.0.0.1";
};

/// Monolithic gnb application configuration.
struct gnb_appconfig {
  /// Log level.
  std::string log_level = "info";
  /// gNodeB identifier.
  int gnb_id = 411;
  /// Node name.
  std::string ran_node_name = "srsgnb01";
  /// AMF configuration.
  amf_appconfig amf_cfg;
  /// RF driver configuration.
  rf_driver_appconfig rf_driver_cfg;
  /// \brief Base cell application configuration.
  ///
  /// \note When a cell is added, it will use the values of this base cell as default values for its base cell
  /// configuration. This parameter usage is restricted for filling cell information in the \remark cell_cfg variable.
  base_cell_appconfig common_cell_cfg;
  /// \brief Cell configuration.
  ///
  /// \note Add one cell by default.
  std::vector<cell_appconfig> cells_cfg = {{}};
};

} // namespace srsgnb
