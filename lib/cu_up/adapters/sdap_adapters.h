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

#include "srsran/gtpu/gtpu_tunnel_tx.h"
#include "srsran/pdcp/pdcp_tx.h"
#include "srsran/sdap/sdap.h"
#include "srsran/gtpu/gtpu_teid.h"
#include "srsran/gateways/udp_network_gateway.h"
#include "srsran/support/bit_encoding.h"

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>

namespace srsran {

constexpr uint32_t network_gateway_udp_max_len = 9100;

namespace srs_cu_up {

/// Adapter between SDAP and GTP-U
class sdap_gtpu_adapter : public sdap_rx_sdu_notifier
{
public:
  // sdap_gtpu_adapter()  = default;
  sdap_gtpu_adapter(): logger(srslog::fetch_basic_logger("SDAP-ADAPTER")) {}
  ~sdap_gtpu_adapter() = default;

  // void connect_gtpu(gtpu_tunnel_tx_lower_layer_interface& gtpu_handler_) { gtpu_handler = &gtpu_handler_; }
  void connect_upf(udp_network_gateway_data_handler& upf_handler_) { upf_handler = &upf_handler_; }
  void connect_dn(udp_network_gateway_data_handler& dn_handler_) { dn_handler = &dn_handler_; }
  void add_peer_teid(gtpu_teid_t teid) 
  { 
    peer_teid = teid; 
    logger.debug("PEER_TEID = {}", peer_teid.value());
  }
  void add_peer_sockaddr(std::string peer_addr, uint16_t peer_port) 
  { 
    get_upf_sockaddr(upf_sockaddr, peer_addr.c_str(), peer_port);
  }

  void on_new_sdu(byte_buffer sdu, qos_flow_id_t qfi) override
  {
    // srsran_assert(gtpu_handler != nullptr, "GTPU handler must not be nullptr");
    // gtpu_handler->handle_sdu(std::move(sdu), qfi);
    srsran_assert(upf_handler != nullptr, "UPF Network Gateway handler must not be nullptr");
    srsran_assert(dn_handler != nullptr, "DN Network Gateway handler must not be nullptr");

    // upf_handler->handle_pud
    uint32_t teid = peer_teid.value();
    uint8_t qfi_uint = qos_flow_id_to_uint(qfi);
    uint32_t pdu_len = sdu.length();
    logger.debug("TEID = {}, QFI = {}, PDU_LEN = {}", 
                  teid, 
                  qfi_uint, 
                  pdu_len);

    byte_buffer upf_pdu = {};
    // encoder 会将数据打包到 upf_pdu 字节缓冲区中
    bit_encoder encoder{upf_pdu};
    bool        pack_ok = true;
    pack_ok &= encoder.pack(peer_teid.value(), 32);
    pack_ok &= encoder.pack(qfi_uint, 8);
    pack_ok &= encoder.pack(pdu_len, 32);

    if (!pack_ok) {
      logger.error(
        "Failed to pack GTP-U payload. TEID={} QFI={} pdu_len={}", teid, qfi_uint, pdu_len);
        }

    logger.debug("UPF PDU is {}", upf_pdu);
    upf_handler->handle_upf_pdu(std::move(upf_pdu), upf_sockaddr);
    
    // dn_handler->handle_pdu
    handle_pdu_dn(std::move(sdu));
    
  }

  void handle_pdu_dn(byte_buffer pdu)
  {
    logger.debug("pdu length is : {}", pdu.length());
    // 得到sdu
    if(!pdu.empty()){
      logger.debug("success get sdu!! length is {}", pdu.length());
    } else{
      logger.error("Failed to get sdu!!");
    }
    
    logger.debug("DN PDU is {}", pdu);
    
    span<const uint8_t> pdu_span = to_span(pdu, tx_mem);
    struct ip *ip_h = (struct ip *)pdu_span.data();
    // uint16_t dst_post = 0;
    // dst_post = (pdu_span[22] << 8) | pdu_span[23];
    // logger.debug("dst_post is {}", dst_post);
    srsran_assert(get_dn_sockaddr(dn_sockaddr, ip_h), "failed to get DN sockaddr!!");
    
    // 把pdu发送到目标ip
    upf_handler->handle_pdu(std::move(pdu), dn_sockaddr);
  }

  bool get_upf_sockaddr(sockaddr_storage& out_sockaddr, const char* addr, uint16_t port)
  {
    // Validate configuration
    if (inet_pton(AF_INET, addr, &((::sockaddr_in*)&out_sockaddr)->sin_addr) == 1) {
      ((::sockaddr_in*)&out_sockaddr)->sin_family = AF_INET;
      ((::sockaddr_in*)&out_sockaddr)->sin_port   = htons(port);
      char ip_str[INET6_ADDRSTRLEN];
      const sockaddr_in* addr_in = reinterpret_cast<const sockaddr_in*>(&out_sockaddr);
      inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str));
      logger.debug("UPF IPv4 Address: {}, Port: {}", ip_str, ntohs(addr_in->sin_port));
    } else if (inet_pton(AF_INET6, addr, &((::sockaddr_in6*)&out_sockaddr)->sin6_addr) == 1) {
      ((::sockaddr_in6*)&out_sockaddr)->sin6_family = AF_INET6;
      ((::sockaddr_in6*)&out_sockaddr)->sin6_port   = htons(port);
    } else {
      return false;
    }
    return true;
  }

  bool get_dn_sockaddr(sockaddr_storage& out_sockaddr, const struct ip *ip_head)
  {
    int ip_version = ip_head->ip_v;
    // Validate configuration
    if(ip_version == 4){
      char ipv4_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &ip_head->ip_dst, ipv4_str, INET_ADDRSTRLEN);

      ((::sockaddr_in*)&out_sockaddr)->sin_family = AF_INET;
      ((::sockaddr_in*)&out_sockaddr)->sin_addr.s_addr = ip_head->ip_dst.s_addr;
      logger.debug("IP version is {}, dst_dn_addr is {}", ip_version, ipv4_str);

    } else if(ip_version == 6){
      struct ip6_hdr *ip6_head = (struct ip6_hdr *)ip_head;
      char ipv6_str[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &ip6_head->ip6_dst, ipv6_str, INET6_ADDRSTRLEN);

      ((::sockaddr_in6*)&out_sockaddr)->sin6_family = AF_INET6;
      ((::sockaddr_in6*)&out_sockaddr)->sin6_addr = ip6_head->ip6_dst;
      logger.debug("IP version is {}, dst_dn_addr is {}", ip_version, ipv6_str);
    } else {
      return false;
    }
    return true;
  }
  


private:
  gtpu_teid_t peer_teid;

  // gtpu_tunnel_tx_lower_layer_interface* gtpu_handler = nullptr;
  udp_network_gateway_data_handler* upf_handler = nullptr;
  udp_network_gateway_data_handler* dn_handler = nullptr;

  // Temporary Tx buffer for transmission.
  std::array<uint8_t, network_gateway_udp_max_len> tx_mem;

  sockaddr_storage upf_sockaddr;
  sockaddr_storage dn_sockaddr;

  srslog::basic_logger&                        logger;
};

class sdap_pdcp_adapter : public sdap_tx_pdu_notifier
{
public:
  sdap_pdcp_adapter()  = default;
  ~sdap_pdcp_adapter() = default;

  void connect_pdcp(pdcp_tx_upper_data_interface& pdcp_handler_) { pdcp_handler = &pdcp_handler_; }
  void disconnect_pdcp() { pdcp_handler = nullptr; }

  void on_new_pdu(byte_buffer pdu) override
  {
    if (pdcp_handler == nullptr) {
      srslog::fetch_basic_logger("SDAP").warning("Unconnected PDCP handler. Dropping SDAP PDU");
    } else {
      pdcp_handler->handle_sdu(std::move(pdu));
    }
  }

private:
  pdcp_tx_upper_data_interface* pdcp_handler = nullptr;
};

} // namespace srs_cu_up
} // namespace srsran
