/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "../../../support/resource_grid_test_doubles.h"
#include "srsran/phy/upper/signal_processors/srs/formatters.h"
#include "srsran/phy/upper/signal_processors/srs/srs_estimator_configuration.h"
#include "srsran/phy/upper/signal_processors/srs/srs_estimator_factory.h"
#include "fmt/ostream.h"
#include "gtest/gtest.h"

using namespace srsran;

namespace {

// Valid SRS configuration used as a base for the test case.
const srs_estimator_configuration base_config = {{0, 130, 8, 0},
                                                 {srs_resource_configuration::one_two_four_enum(2),
                                                  srs_resource_configuration::one_two_four_enum(1),
                                                  12,
                                                  17,
                                                  647,
                                                  2,
                                                  srs_resource_configuration::comb_size_enum(2),
                                                  1,
                                                  1,
                                                  66,
                                                  1,
                                                  3,
                                                  srs_resource_configuration::group_or_sequence_hopping_enum::neither,
                                                  {}},
                                                 {0}};

struct test_case_t {
  std::function<srs_estimator_configuration()> get_config;
  std::string                                  expr;
};

std::ostream& operator<<(std::ostream& os, const test_case_t& test_case)
{
  fmt::print(os, "{}", test_case.get_config());
  return os;
}

const std::vector<test_case_t> pdsch_processor_validator_test_data = {
    {[] {
       srs_estimator_configuration config = base_config;
       config.resource.comb_size          = srs_resource_configuration::comb_size_enum::two;
       config.resource.comb_offset        = 2;
       return config;
     },
     R"(Invalid SRS resource\.)"},
    {[] {
       srs_estimator_configuration config = base_config;
       config.resource.bandwidth_index    = 1;
       config.resource.freq_hopping       = 0;
       return config;
     },
     R"(Frequency hopping is not supported\.)"},
    {[] {
       srs_estimator_configuration config = base_config;
       config.resource.hopping            = srs_resource_configuration::group_or_sequence_hopping_enum::group_hopping;
       return config;
     },
     R"(No sequence nor group hopping supported\.)"},
    {[] {
       srs_estimator_configuration config = base_config;
       config.ports.clear();
       return config;
     },
     R"(Receive port list is empty\.)"},
};

class srsEstimatorValidatorFixture : public ::testing::TestWithParam<test_case_t>
{
protected:
  void SetUp() override
  {
    std::shared_ptr<low_papr_sequence_generator_factory> sequence_generator_factory =
        create_low_papr_sequence_generator_sw_factory();
    ASSERT_NE(sequence_generator_factory, nullptr);

    std::shared_ptr<srs_estimator_factory> srs_est_factory =
        create_srs_estimator_generic_factory(sequence_generator_factory);
    ASSERT_NE(srs_est_factory, nullptr);

    estimator = srs_est_factory->create();
    ASSERT_NE(estimator, nullptr);

    validator = srs_est_factory->create_validator();
    ASSERT_NE(validator, nullptr);
  }

  std::unique_ptr<srs_estimator>                         estimator = nullptr;
  std::unique_ptr<srs_estimator_configuration_validator> validator = nullptr;
};

TEST_P(srsEstimatorValidatorFixture, srsEstimatorValidatorDeathTest)
{
  // Use thread safe death test.
  ::testing::GTEST_FLAG(death_test_style) = "threadsafe";

  ASSERT_NE(estimator, nullptr);
  ASSERT_NE(validator, nullptr);

  const test_case_t& param = GetParam();

  srs_estimator_configuration config = param.get_config();

  // Make sure the configuration is invalid.
  ASSERT_FALSE(validator->is_valid(config));

  // Prepare resource grid and resource grid mapper spies.
  resource_grid_reader_spy grid(0, 0, 0);

  // Prepare receive data.
  std::vector<uint8_t> data;

  // Process pdsch PDU.
#ifdef ASSERTS_ENABLED
  ASSERT_DEATH({ estimator->estimate(grid, config); }, param.expr);
#endif // ASSERTS_ENABLED
}

// Creates test suite that combines all possible parameters.
INSTANTIATE_TEST_SUITE_P(srsEstimatorValidatorDeathTest,
                         srsEstimatorValidatorFixture,
                         ::testing::ValuesIn(pdsch_processor_validator_test_data));

} // namespace
