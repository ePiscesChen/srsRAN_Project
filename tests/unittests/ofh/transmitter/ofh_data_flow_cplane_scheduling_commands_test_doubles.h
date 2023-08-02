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

#include "../../../../lib/ofh/transmitter/ofh_data_flow_cplane_scheduling_commands.h"

namespace srsran {
namespace ofh {
namespace testing {

/// Spy Control-Plane scheduling commands data flow.
class data_flow_cplane_scheduling_commands_spy : public data_flow_cplane_scheduling_commands
{
public:
  struct spy_info {
    unsigned          eaxc      = -1;
    data_direction    direction = data_direction::downlink;
    slot_point        slot;
    filter_index_type filter_type;
  };

  void enqueue_section_type_1_message(const data_flow_cplane_type_1_context& context) override
  {
    has_enqueue_section_type_1_message_method_been_called = true;
    info.slot                                             = context.slot;
    info.eaxc                                             = context.eaxc;
    info.direction                                        = context.direction;
    info.filter_type                                      = context.filter_type;
  }

  void enqueue_section_type_3_prach_message(const struct data_flow_cplane_scheduling_prach_context& context) override
  {
    has_enqueue_section_type_3_message_method_been_called = true;
    info.slot                                             = context.slot;
    info.eaxc                                             = context.eaxc;
    info.direction                                        = data_direction::uplink;
    info.filter_type                                      = context.filter_type;
  }

  /// Returns true if the method enqueue section type 1 message has been called, otherwise false.
  bool has_enqueue_section_type_1_method_been_called() const
  {
    return has_enqueue_section_type_1_message_method_been_called;
  }

  /// Returns true if the method enqueue section type 3 message has been called, otherwise false.
  bool has_enqueue_section_type_3_method_been_called() const
  {
    return has_enqueue_section_type_3_message_method_been_called;
  }

  /// Returns the configured eAxC.
  spy_info get_spy_info() const { return info; }

private:
  bool     has_enqueue_section_type_1_message_method_been_called = false;
  bool     has_enqueue_section_type_3_message_method_been_called = false;
  spy_info info;
};

} // namespace testing
} // namespace ofh
} // namespace srsran