#include <catch2/catch_all.hpp>
#include "Util/ProgramOptions.hpp"
#include "Hardware/Adversaries.hpp"

class Test {
 public:
  Test(double maximum_leakage, uint64_t number_of_propagations,
       uint64_t number_of_standard_probes, uint64_t number_of_probe_extensions,
       uint64_t number_of_probing_sets)
      : maximum_leakage_(maximum_leakage),
        number_of_propagations_(number_of_propagations),
        number_of_standard_probes_(number_of_standard_probes),
        number_of_probe_extensions_(number_of_probe_extensions),
        number_of_probing_sets_(number_of_probing_sets) {}

  bool Require(bool leakage_expected, uint64_t number_of_propagations,
               uint64_t number_of_standard_probes,
               uint64_t number_of_probe_extensions,
               uint64_t number_of_probing_sets) {
    if (leakage_expected && maximum_leakage_ < 5.0) {
      return false;
    } else if (!leakage_expected && maximum_leakage_ >= 5.0) {
      return false;
    } else if (number_of_propagations != number_of_propagations_) {
      return false;
    } else if (number_of_standard_probes != number_of_standard_probes_) {
      return false;
    } else if (number_of_probe_extensions != number_of_probe_extensions_) {
      return false;
    } else if (number_of_probing_sets != number_of_probing_sets_) {
      return false;
    } else {
      return true;
    }
  }

 private:
  double maximum_leakage_;
  uint64_t number_of_propagations_;
  uint64_t number_of_standard_probes_;
  uint64_t number_of_probe_extensions_;
  uint64_t number_of_probing_sets_;
};

Test TestLmdplAndd1(Settings& settings) {
  const std::string library_file_name = "library.json";
  const std::string library_name = "NANG45";
  const std::string design_file_name = "tests/full/lmdpl_and_d1/lmdpl_and_d1.v";
  const std::string topmodule_name = "lmdpl_and_d1";
  const std::string result_folder_name = "tests/full/lmdpl_and_d1";

  boost::json::object library_file = ValidateJson(library_file_name);
  Library library(library_file, library_name);
  CircuitStruct circuit(design_file_name, topmodule_name, settings, library);
  std::vector<SharedData> shared_data(settings.GetNumberOfThreads(),
                                      SharedData(circuit, settings));
  Simulation simulation(circuit, settings);
  simulation.topmodule_name_ = topmodule_name;
  simulation.result_folder_name_ = result_folder_name;

  if (settings.side_channel_analysis.relaxed_model) {
    Hardware::Adversaries<Hardware::RelaxedProbe> adversary(
        library, circuit, settings, simulation);
    double maximum_leakage =
        adversary.EvaluateRobustProbingSecurity(shared_data);
    return Test(maximum_leakage, adversary.GetNumberOfPropagations(),
                adversary.GetNumberOfStandardProbes(),
                adversary.GetNumberOfExtendedProbes(),
                adversary.GetNumberOfProbingSets());
  } else {
    Hardware::Adversaries<Hardware::RobustProbe> adversary(
        library, circuit, settings, simulation);
    double maximum_leakage =
        adversary.EvaluateRobustProbingSecurity(shared_data);
    return Test(maximum_leakage, adversary.GetNumberOfPropagations(),
                adversary.GetNumberOfStandardProbes(),
                adversary.GetNumberOfExtendedProbes(),
                adversary.GetNumberOfProbingSets());
  }
}

TEST_CASE("Test full verification (lmdpl_and_d1)", "[lmdpl_and_d1]") {
  const std::string config_file_name = "tests/full/lmdpl_and_d1/lmdpl_and_d1.json";
  boost::json::object config_file = ValidateJson(config_file_name);
  Settings settings(config_file, true);

  SECTION("Robust Probing Model with no minimization") {
    settings.SetMinimization(Minimization::none);
    settings.SetRelaxedModel(false);
    Test test = TestLmdplAndd1(settings);

    // We expect leakage in the first order.
    REQUIRE(test.Require(true, 41, 123, 48, 123));
  }

  SECTION("Robust Probing Model with trivial minimization") {
    settings.SetMinimization(Minimization::trivial);
    settings.SetRelaxedModel(false);
    Test test = TestLmdplAndd1(settings);

    // We expect leakage in the first order.
    REQUIRE(test.Require(true, 11, 33, 48, 12));
  }

  SECTION("Robust Probing Model with aggressive minimization") {
    settings.SetMinimization(Minimization::aggressive);
    settings.SetRelaxedModel(false);
    Test test = TestLmdplAndd1(settings);

    // We expect leakage in the first order.
    REQUIRE(test.Require(true, 11, 33, 48, 6));
  }

  SECTION("Robust but Relaxed Probing Model with no minimization") {
    settings.SetMinimization(Minimization::none);
    settings.SetRelaxedModel(true);
    settings.SetTestOrder(GENERATE(1, 2));

    Test test = TestLmdplAndd1(settings);

    if (settings.GetTestOrder() == 1) {
      // We expect no leakage in the first order.
      REQUIRE(test.Require(false, 144, 120, 144, 120));
    } else {
      // But we expect leakage in the second order.
      REQUIRE(test.Require(true, 144, 120, 144, 2340));
    }
  }

  SECTION("Robust but Relaxed Probing Model with trivial minimization") {
    settings.SetMinimization(Minimization::trivial);
    settings.SetRelaxedModel(true);
    settings.SetTestOrder(GENERATE(1, 2));

    Test test = TestLmdplAndd1(settings);

    if (settings.GetTestOrder() == 1) {
      // We expect no leakage in the first order.
      REQUIRE(test.Require(false, 144, 30, 144, 30));
    } else {
      // But we expect leakage in the second order.
      REQUIRE(test.Require(true, 144, 30, 144, 135));
    }
  }

  SECTION("Robust but Relaxed Probing Model with aggressive minimization") {
    settings.SetMinimization(Minimization::aggressive);
    settings.SetRelaxedModel(true);
    settings.SetTestOrder(GENERATE(1, 2));

    Test test = TestLmdplAndd1(settings);

    if (settings.GetTestOrder() == 1) {
      // We expect no leakage in the first order.
      REQUIRE(test.Require(false, 144, 30, 144, 30));
    } else {
      // But we expect leakage in the second order.
      REQUIRE(test.Require(true, 144, 30, 144, 135));
    }
  }
}
