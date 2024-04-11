/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsran/phy/upper/signal_processors/srs/srs_estimator_factory.h"
#include "logging_srs_estimator_decorator.h"
#include "srs_estimator_generic_impl.h"
#include "srs_validator_generic_impl.h"

using namespace srsran;

namespace {

class srs_estimator_factory_generic : public srs_estimator_factory
{
public:
  srs_estimator_factory_generic(std::shared_ptr<low_papr_sequence_generator_factory> sequence_generator_factory_) :
    sequence_generator_factory(std::move(sequence_generator_factory_))
  {
    srsran_assert(sequence_generator_factory, "Invalid sequence generator factory.");
  }

  std::unique_ptr<srs_estimator> create() override
  {
    srs_estimator_generic_impl::dependencies deps;
    deps.sequence_generator = sequence_generator_factory->create();

    return std::make_unique<srs_estimator_generic_impl>(std::move(deps));
  }

  std::unique_ptr<srs_estimator_configuration_validator> create_validator() override
  {
    return std::make_unique<srs_validator_generic_impl>();
  }

private:
  std::shared_ptr<low_papr_sequence_generator_factory> sequence_generator_factory;
};

} // namespace

std::shared_ptr<srs_estimator_factory> srsran::create_srs_estimator_generic_factory(
    std::shared_ptr<low_papr_sequence_generator_factory> sequence_generator_factory)
{
  return std::make_shared<srs_estimator_factory_generic>(std::move(sequence_generator_factory));
}

std::unique_ptr<srs_estimator> srs_estimator_factory::create(srslog::basic_logger& logger)
{
  return std::make_unique<logging_srs_estimator_decorator>(logger, create());
}
