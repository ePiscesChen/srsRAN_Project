/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "upper_phy_rx_results_notifier_wrapper.h"
#include "srsran/support/error_handling.h"

using namespace srsran;

void upper_phy_rx_results_notifier_wrapper::on_new_prach_results(const ul_prach_results& result)
{
  report_fatal_error_if_not(rx_results_notifier, "Invalid results notifier object.");
  rx_results_notifier->on_new_prach_results(result);
}

void upper_phy_rx_results_notifier_wrapper::on_new_pusch_results_control(const ul_pusch_results_control& result)
{
  report_fatal_error_if_not(rx_results_notifier, "Invalid results notifier object.");
  rx_results_notifier->on_new_pusch_results_control(result);
}

void upper_phy_rx_results_notifier_wrapper::on_new_pusch_results_data(const ul_pusch_results_data& result)
{
  report_fatal_error_if_not(rx_results_notifier, "Invalid results notifier object.");
  rx_results_notifier->on_new_pusch_results_data(result);
}

void upper_phy_rx_results_notifier_wrapper::on_new_pucch_results(const ul_pucch_results& result)
{
  report_fatal_error_if_not(rx_results_notifier, "Invalid results notifier object.");
  rx_results_notifier->on_new_pucch_results(result);
}

void upper_phy_rx_results_notifier_wrapper::on_new_srs_results(const ul_srs_results& result)
{
  report_fatal_error_if_not(rx_results_notifier, "Results notifier already configured.");
  rx_results_notifier->on_new_srs_results(result);
}

void upper_phy_rx_results_notifier_wrapper::connect(upper_phy_rx_results_notifier& n)
{
  report_fatal_error_if_not(!rx_results_notifier, "Results notifier already configured.");
  rx_results_notifier = &n;
}
