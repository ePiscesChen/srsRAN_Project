/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "dlt_pcap_impl.h"
#include "srsran/adt/byte_buffer.h"
#include <stdint.h>

namespace srsran {

constexpr uint16_t pcap_dlt_max_pdu_len = 9000;

dlt_pcap_impl::dlt_pcap_impl(unsigned dlt_, const std::string& layer_name_, os_sched_affinity_bitmask cpu_mask_) :
  dlt(dlt_), layer_name(layer_name_), cpu_mask(cpu_mask_)
{
  tmp_mem.resize(pcap_dlt_max_pdu_len);
}

dlt_pcap_impl::~dlt_pcap_impl()
{
  close();
}

void dlt_pcap_impl::open(const std::string& filename_)
{
  worker =
      std::make_unique<task_worker>(layer_name + "_pcap", 1024, os_thread_realtime_priority::no_realtime(), cpu_mask);
  // Capture filename_ by copy to prevent it goes out-of-scope when the lambda is executed later
  auto fn = [this, filename_]() { writter.dlt_pcap_open(dlt, filename_); };
  worker->push_task_blocking(fn);
  is_open.store(true, std::memory_order_relaxed);
}

void dlt_pcap_impl::close()
{
  if (is_open.load(std::memory_order_relaxed)) {
    worker->wait_pending_tasks();
    is_open.store(false, std::memory_order_relaxed); // any further tasks will see it closed
    auto fn = [this]() { writter.dlt_pcap_close(); };
    worker->push_task_blocking(fn);
    worker->wait_pending_tasks(); // make sure dlt_pcap_close is processed
    worker->stop();
  }
}

bool dlt_pcap_impl::is_write_enabled()
{
  return is_open.load(std::memory_order_relaxed);
}

void dlt_pcap_impl::push_pdu(srsran::byte_buffer pdu)
{
  if (!is_write_enabled() || pdu.empty()) {
    return;
  }

  auto fn = [this, pdu]() mutable { write_pdu(std::move(pdu)); };
  if (not worker->push_task(fn)) {
    srslog::fetch_basic_logger("ALL").warning("Dropped {} PCAP PDU. Worker task is full", layer_name);
  }
}

void dlt_pcap_impl::push_pdu(srsran::const_span<uint8_t> pdu)
{
  if (!is_write_enabled() || pdu.empty()) {
    return;
  }

  byte_buffer buffer{pdu};
  auto        fn = [this, buffer]() mutable { write_pdu(std::move(buffer)); };
  if (not worker->push_task(fn)) {
    srslog::fetch_basic_logger("ALL").warning("Dropped {} PCAP PDU. Worker task is full", layer_name);
  }
}

void dlt_pcap_impl::write_pdu(srsran::byte_buffer buf)
{
  if (!is_write_enabled() || buf.empty()) {
    return;
  }

  if (buf.length() > pcap_dlt_max_pdu_len) {
    srslog::fetch_basic_logger("ALL").warning(
        "Dropped {} PCAP PDU. PDU is too big. pdu_len={} max_size={}", layer_name, buf.length(), pcap_dlt_max_pdu_len);
    return;
  }

  span<const uint8_t> pdu = to_span(buf, span<uint8_t>(tmp_mem).first(buf.length()));

  // write packet header
  writter.write_pcap_header(pdu.size_bytes());

  // write PDU payload
  writter.write_pcap_pdu(pdu);
}

} // namespace srsran
