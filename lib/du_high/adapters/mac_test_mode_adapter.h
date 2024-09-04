/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#pragma once

#include "srsran/du/du_test_config.h"
#include "srsran/mac/mac.h"
#include "srsran/mac/mac_cell_result.h"
#include <unordered_map>
#include <chrono>
#include <istream>
#include <streambuf>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <math.h>
static const unsigned TEST_UE_DL_BUFFER_STATE_UPDATE_SIZE = 1000000;

namespace srsran {

class phy_test_mode_adapter : public mac_result_notifier
{
public:
  phy_test_mode_adapter(mac_result_notifier& phy_notifier_) : adapted_phy(phy_notifier_), cells(MAX_NOF_DU_CELLS) {}

  mac_cell_result_notifier& get_cell(du_cell_index_t cell_index) override;

  void connect(du_cell_index_t cell_index, mac_cell_result_notifier& test_mode_cell_notifier);

  void disconnect(du_cell_index_t cell_index);

  mac_result_notifier& adapted_phy;

private:
  class phy_cell final : public mac_cell_result_notifier
  {
  public:
    mac_cell_result_notifier* ptr = nullptr;

    void on_new_downlink_scheduler_results(const mac_dl_sched_result& dl_res) override;
    void on_new_downlink_data(const mac_dl_data_result& dl_data) override;
    void on_new_uplink_scheduler_results(const mac_ul_sched_result& ul_res) override;
    void on_cell_results_completion(slot_point slot) override;
  };

  std::vector<phy_cell> cells;
};

/// \brief Handles information related to the test UE(s).
class test_ue_info_manager
{
public:
  test_ue_info_manager(rnti_t rnti_start_, uint16_t nof_ues_) : rnti_start(rnti_start_), nof_ues(nof_ues_) {}

  du_ue_index_t rnti_to_du_ue_idx(rnti_t rnti) const
  {
    if (rnti_to_ue_info_lookup.count(rnti) == 0) {
      return INVALID_DU_UE_INDEX;
    }
    return rnti_to_ue_info_lookup.at(rnti).ue_idx;
  }

  bool is_test_ue(du_ue_index_t ue_idx) const { return ue_idx < nof_ues; }

  bool is_test_ue(rnti_t rnti) const
  {
    return (rnti >= rnti_start) and (rnti < to_rnti(to_value(rnti_start) + nof_ues));
  }

  void add_ue(rnti_t rnti, du_ue_index_t ue_idx_, const sched_ue_config_request& sched_ue_cfg_req_)
  {
    rnti_to_ue_info_lookup[rnti] =
        test_ue_info{.ue_idx = ue_idx_, .sched_ue_cfg_req = sched_ue_cfg_req_, .msg4_rx_flag = false};
  }

  void remove_ue(rnti_t rnti)
  {
    if (rnti_to_ue_info_lookup.count(rnti) > 0) {
      rnti_to_ue_info_lookup.erase(rnti);
    }
  }

  sched_ue_config_request get_sched_ue_cfg_request(rnti_t rnti) const
  {
    return rnti_to_ue_info_lookup.at(rnti).sched_ue_cfg_req;
  }

  bool is_msg4_rxed(rnti_t rnti) const
  {
    if (rnti_to_ue_info_lookup.count(rnti) > 0) {
      return rnti_to_ue_info_lookup.at(rnti).msg4_rx_flag;
    }
    return false;
  }

  void msg4_rxed(rnti_t rnti, bool msg4_rx_flag_)
  {
    if (rnti_to_ue_info_lookup.count(rnti) > 0) {
      rnti_to_ue_info_lookup.at(rnti).msg4_rx_flag = msg4_rx_flag_;
    }
  }

private:
  struct test_ue_info {
    du_ue_index_t           ue_idx;
    sched_ue_config_request sched_ue_cfg_req;
    bool                    msg4_rx_flag;
  };

  /// Parameters received from configuration.
  rnti_t   rnti_start;
  uint16_t nof_ues;

  /// Mapping between UE RNTI and test UE information.
  std::unordered_map<rnti_t, test_ue_info> rnti_to_ue_info_lookup;
};

/// \brief Adapter of MAC cell for testing purposes. It automatically forces ACK/CRC=OK for the test UE.
class mac_test_mode_cell_adapter : public mac_cell_control_information_handler,
                                   public mac_cell_result_notifier,
                                   public mac_cell_slot_handler
{
public:
  mac_test_mode_cell_adapter(const srs_du::du_test_config::test_ue_config& test_ue_cfg_,
                             const mac_cell_creation_request&              cell_cfg,
                             mac_cell_control_information_handler&         adapted_,
                             mac_pdu_handler&                              pdu_handler_,
                             mac_cell_slot_handler&                        slot_handler_,
                             mac_cell_result_notifier&                     result_notifier_,
                             std::function<void(rnti_t)>                   dl_bs_notifier_,
                             test_ue_info_manager&                         ue_info_mgr_);

  void on_new_downlink_scheduler_results(const mac_dl_sched_result& dl_res) override
  {
    result_notifier.on_new_downlink_scheduler_results(dl_res);
  }

  void on_new_downlink_data(const mac_dl_data_result& dl_data) override
  {
    result_notifier.on_new_downlink_data(dl_data);
  }

  // Intercepts the UL results coming from the MAC.
  void on_new_uplink_scheduler_results(const mac_ul_sched_result& ul_res) override;

  void on_cell_results_completion(slot_point slot) override { result_notifier.on_cell_results_completion(slot); }

  void handle_slot_indication(slot_point sl_tx) override;
  void handle_error_indication(slot_point sl_tx, error_event event) override;

  void handle_crc(const mac_crc_indication_message& msg) override;

  void handle_uci(const mac_uci_indication_message& msg) override;

private:
  struct slot_descision_history {
    std::vector<pucch_info>    pucchs;
    std::vector<ul_sched_info> puschs;
  };

  void fill_csi_bits(rnti_t rnti, bounded_bitset<uci_constants::MAX_NOF_CSI_PART1_OR_PART2_BITS>& payload) const;
  void fill_uci_pdu(mac_uci_pdu::pucch_f0_or_f1_type& pucch_ind, const pucch_info& pucch) const;
  void fill_uci_pdu(mac_uci_pdu::pucch_f2_or_f3_or_f4_type& pucch_ind, const pucch_info& pucch) const;
  void fill_uci_pdu(mac_uci_pdu::pusch_type& pusch_ind, const ul_sched_info& ul_grant) const;
  void forward_uci_ind_to_mac(const mac_uci_indication_message& uci_msg);
  void forward_crc_ind_to_mac(const mac_crc_indication_message& crc_msg);

  const srs_du::du_test_config::test_ue_config& test_ue_cfg;
  mac_cell_control_information_handler&         adapted;
  mac_pdu_handler&                              pdu_handler;
  mac_cell_slot_handler&                        slot_handler;
  mac_cell_result_notifier&                     result_notifier;
  std::function<void(rnti_t)>                   dl_bs_notifier;
  srslog::basic_logger&                         logger;

  std::vector<slot_descision_history> sched_decision_history;

  test_ue_info_manager& ue_info_mgr;
};

class test_mode_buffer_size{
public:
  virtual ~test_mode_buffer_size() = default;
  virtual double get_buffer_size() = 0;
};

class static_test_mode : public test_mode_buffer_size {
public:
  static_test_mode(double _static_size = 0) : static_buffer_size(_static_size){}
  ~static_test_mode() {}
  double get_buffer_size() override {
    return static_buffer_size;
  }
private:
  double static_buffer_size;
};

class range_test_mode : public test_mode_buffer_size {
public:
  range_test_mode(srs_du::du_test_config::test_ue_config _test_ue):
  test_ue(_test_ue){
    min_buffer_size = test_ue.min_buffer_size;
    max_buffer_size = test_ue.max_buffer_size;
    buffer_interval = test_ue.buffer_interval;
    buffer_step = test_ue.buffer_step;
    // start_time = std::chrono::system_clock::now();
  }
  ~range_test_mode() {}
  unsigned min(unsigned x, unsigned y) { return x < y ? x : y; }
  double get_buffer_size() override {
    if(first_called){
      start_time = std::chrono::system_clock::now();
      first_called = false;
      return min_buffer_size;
    }
    std::chrono::system_clock::time_point current = std::chrono::system_clock::now();
    std::chrono::duration<unsigned> running_duration = std::chrono::duration_cast<std::chrono::duration<unsigned>>(current - start_time);
    return min(max_buffer_size,
               min_buffer_size +  running_duration.count() / buffer_interval * buffer_step);
  }
private:
  srs_du::du_test_config::test_ue_config test_ue;
  unsigned min_buffer_size, max_buffer_size, buffer_interval, buffer_step;
  std::chrono::system_clock::time_point start_time;
  bool first_called = true;
};

class trace_test_mode : public test_mode_buffer_size{
public:
  trace_test_mode(std::string CSV_filename){
    std::ifstream csv_data(CSV_filename, std::ios::in);
    std::string line;
    if (!csv_data.is_open()) {
      std::exit(1);
    }
    std::istringstream sin;
    std::string word;
    std::getline(csv_data, line);
    int turn = 0;
    while (std::getline(csv_data, line)) {
        sin.clear();
        sin.str(line);
        while (std::getline(sin, word, ',')) {
            if(turn == 1){
                dl_brate_table.push_back(stod(word));
            }
            turn = (turn + 1) % 4;
        }
    }
    csv_data.close();
    counter = 0;
    total_num = dl_brate_table.size();
  }
  ~trace_test_mode() {}

  double get_buffer_size() override {
    double ret = 7.537320046522576e-05 * dl_brate_table[counter] + 539.8745777874028;
    if(first_called){
      start_time = std::chrono::system_clock::now();
      first_called = false;
    }
    std::chrono::system_clock::time_point current = std::chrono::system_clock::now();
    std::chrono::duration<unsigned> running_duration = std::chrono::duration_cast<std::chrono::duration<unsigned>>(current - start_time);
    counter = (running_duration.count()) % total_num;
    return ret;
  }
private:
  std::vector<double> dl_brate_table;
  unsigned counter;
  unsigned total_num;
  std::chrono::system_clock::time_point start_time;
  bool first_called = true;
};

class mac_test_mode_adapter final : public mac_interface,
                                    public mac_ue_control_information_handler,
                                    public mac_ue_configurator,
                                    public mac_cell_manager
{
public:
  explicit mac_test_mode_adapter(const srs_du::du_test_config::test_ue_config& test_ue_cfg,
                                 mac_result_notifier&                          phy_notifier_);
  ~mac_test_mode_adapter() override;

  void connect(std::unique_ptr<mac_interface> mac_ptr);

  // mac_cell_manager
  void                 add_cell(const mac_cell_creation_request& cell_cfg) override;
  void                 remove_cell(du_cell_index_t cell_index) override;
  mac_cell_controller& get_cell_controller(du_cell_index_t cell_index) override;

  mac_cell_rach_handler& get_rach_handler(du_cell_index_t cell_index) override
  {
    return mac_adapted->get_rach_handler(cell_index);
  }

  mac_cell_slot_handler& get_slot_handler(du_cell_index_t cell_index) override
  {
    return *cell_info_handler[cell_index];
  }

  mac_cell_manager& get_cell_manager() override { return *this; }

  mac_ue_control_information_handler& get_ue_control_info_handler() override { return *this; }

  mac_pdu_handler& get_pdu_handler() override { return mac_adapted->get_pdu_handler(); }

  mac_paging_information_handler& get_cell_paging_info_handler() override
  {
    return mac_adapted->get_cell_paging_info_handler();
  }

  mac_ue_configurator& get_ue_configurator() override { return *this; }

  mac_cell_control_information_handler& get_control_info_handler(du_cell_index_t cell_index) override;

  mac_result_notifier& get_phy_notifier() { return *phy_notifier; }

private:
  void handle_dl_buffer_state_update(const mac_dl_buffer_state_indication_message& dl_bs) override;

  // mac_ue_configurator interface.
  async_task<mac_ue_create_response> handle_ue_create_request(const mac_ue_create_request& cfg) override;
  async_task<mac_ue_reconfiguration_response>
  handle_ue_reconfiguration_request(const mac_ue_reconfiguration_request& cfg) override;
  async_task<mac_ue_delete_response> handle_ue_delete_request(const mac_ue_delete_request& cfg) override;
  bool                               handle_ul_ccch_msg(du_ue_index_t ue_index, byte_buffer pdu) override;

  std::vector<mac_logical_channel_config> adapt_bearers(const std::vector<mac_logical_channel_config>& orig_bearers);

  srs_du::du_test_config::test_ue_config test_ue;
  std::unique_ptr<mac_interface>         mac_adapted;

  std::unique_ptr<phy_test_mode_adapter> phy_notifier;

  std::vector<std::unique_ptr<mac_test_mode_cell_adapter>> cell_info_handler;

  test_ue_info_manager ue_info_mgr;
  std::shared_ptr<test_mode_buffer_size> buffer_size_container;
};

} // namespace srsran
