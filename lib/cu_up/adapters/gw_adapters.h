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

#include "srsran/gateways/network_gateway.h"
#include "srsran/gtpu/gtpu_demux.h"

namespace srsran {
namespace srs_cu_up {

/// Adapter between Network Gateway (Data) and GTP-U demux
class network_gateway_data_gtpu_demux_adapter : public srsran::network_gateway_data_notifier_with_src_addr
{
public:
  network_gateway_data_gtpu_demux_adapter()           = default;
  ~network_gateway_data_gtpu_demux_adapter() override = default;

  void connect_gtpu_demux(gtpu_demux_rx_upper_layer_interface& gtpu_demux_) { gtpu_demux = &gtpu_demux_; }

  void on_new_pdu(byte_buffer pdu, const sockaddr_storage& src_addr) override
  {
    srsran_assert(gtpu_demux != nullptr, "GTP-U handler must not be nullptr");
    gtpu_demux->handle_pdu(std::move(pdu), src_addr);
  }

private:
  gtpu_demux_rx_upper_layer_interface* gtpu_demux = nullptr;
};

/// Adapter between Network Gateway (Data) and SDAP
class network_gateway_data_sdap_adapter : public srsran::network_gateway_data_notifier_with_src_addr
{
public:
  network_gateway_data_sdap_adapter()           = default;
  ~network_gateway_data_sdap_adapter() override = default;

  void connect_sdap(sdap_tx_sdu_handler& sdap_handler_) override { 
    sdap_handler = &sdap_handler_; 
    std::cout << "GW CONNECT SDAP handler" << std::endl;
  }

  void on_new_pdu(byte_buffer sdu, qos_flow_id_t qos_flow_id) override
  {
    srsran_assert(sdap_handler != nullptr, "SDAP handler must not be nullptr");
    sdap_handler->handle_sdu(std::move(sdu), qos_flow_id);
  }

private:
  sdap_tx_sdu_handler* sdap_handler = nullptr;
};

} // namespace srs_cu_up
} // namespace srsran
