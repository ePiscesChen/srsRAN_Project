/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/du_manager/du_manager.h"
#include "srsran/f1ap/du/f1ap_du.h"
#include "srsran/mac/mac.h"
#include "srsran/rlc/rlc_rx.h"
#include "srsran/rlc/rlc_tx.h"
#include "srsran/srslog/srslog.h"

namespace srsran {
namespace srs_du {

// class du_ccch_pdu_handler : public ul_ccch_pdu_notifier
//{
// public:
//   void handle_pdu(const byte_buffer& pdu) override
//   {
//     printf("[RLC-DUMANAGER-ADAPTER] Received a packet from F1u layer, forwarding data packet of size = %u bytes to "
//            "upper layer (PDCP)\n",
//            (unsigned)pdu.size());
//     du_manager.ul_ccch_indication(pdu);
//   }
//
// private:
//   du_manager_input_gateway& du_manager;
// };

class du_manager_mac_event_indicator : public mac_ul_ccch_notifier
{
public:
  void connect(du_manager_ccch_handler& du_mng_) { du_mng = &du_mng_; }
  void on_ul_ccch_msg_received(const ul_ccch_indication_message& msg) override
  {
    du_mng->handle_ul_ccch_indication(msg);
  }

private:
  du_manager_ccch_handler* du_mng;
};

class mac_f1ap_paging_handler : public f1ap_du_paging_notifier
{
public:
  void connect(mac_cell_paging_information_handler& mac_) { mac = &mac_; }

  void on_paging_received(const mac_paging_information& msg) override { mac->handle_paging_information(msg); }

private:
  mac_cell_paging_information_handler* mac;
};

} // namespace srs_du
} // namespace srsran
