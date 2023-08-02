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

#include "ofh_data_flow_cplane_scheduling_commands.h"
#include "srsran/support/executors/task_executor.h"
#include <atomic>
#include <memory>

namespace srsran {
namespace ofh {

/// Task dispatcher entry.
struct data_flow_cplane_downlink_task_dispatcher_entry {
  data_flow_cplane_downlink_task_dispatcher_entry(
      std::unique_ptr<data_flow_cplane_scheduling_commands> data_flow_cplane_,
      task_executor&                                        executor_) :
    data_flow_cplane(std::move(data_flow_cplane_)), executor(executor_)
  {
    srsran_assert(data_flow_cplane, "Invalid data flow");
  }

  std::unique_ptr<data_flow_cplane_scheduling_commands> data_flow_cplane;
  task_executor&                                        executor;
};

/// Open Fronthaul downlink Control-Plane task dispatcher implementation.
class data_flow_cplane_downlink_task_dispatcher : public data_flow_cplane_scheduling_commands
{
public:
  data_flow_cplane_downlink_task_dispatcher(std::vector<data_flow_cplane_downlink_task_dispatcher_entry>&& config_) :
    dispatchers(std::move(config_))
  {
  }

  // See interface for documentation.
  void enqueue_section_type_1_message(const data_flow_cplane_type_1_context& context) override
  {
    unsigned                                         index            = last_used++ % dispatchers.size();
    data_flow_cplane_downlink_task_dispatcher_entry& dispatcher       = dispatchers[index];
    data_flow_cplane_scheduling_commands&            data_flow_cplane = *dispatcher.data_flow_cplane;

    dispatcher.executor.execute(
        [&data_flow_cplane, context]() { data_flow_cplane.enqueue_section_type_1_message(context); });
  }

  // See interface for documentation.
  void enqueue_section_type_3_prach_message(const data_flow_cplane_scheduling_prach_context& context) override
  {
    unsigned                                         index            = last_used++ % dispatchers.size();
    data_flow_cplane_downlink_task_dispatcher_entry& dispatcher       = dispatchers[index];
    data_flow_cplane_scheduling_commands&            data_flow_cplane = *dispatcher.data_flow_cplane;

    dispatcher.executor.execute(
        [&data_flow_cplane, context]() { data_flow_cplane.enqueue_section_type_3_prach_message(context); });
  }

private:
  std::atomic<unsigned>                                        last_used = {0};
  std::vector<data_flow_cplane_downlink_task_dispatcher_entry> dispatchers;
};

} // namespace ofh
} // namespace srsran
