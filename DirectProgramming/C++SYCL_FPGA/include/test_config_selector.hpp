#pragma once
#include <sycl/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>
#include <string>

// We should remove the older device selectors since most of our code is not
// backwards-compatible with them
#if SYCL_EXT_INTEL_FPGA_DEVICE_SELECTOR < 2

namespace fpga_tools {
  
class select_by_string : public sycl::default_selector {
public:
  select_by_string(std::string s) : target_name(s) {}
  virtual int operator()(const sycl::device &device) const {
    std::string name = device.get_info<sycl::info::device::name>();
    if (name.find(target_name) != std::string::npos) {
      // The returned value represents a priority, this number is chosen to be
      // large to ensure high priority
      return 10000;
    }
    return -1;
  }

private:
  std::string target_name;
};

class testconfig_selector : public sycl::default_selector {
private:
#ifdef FPGA_EMULATOR
  sycl::ext::intel::fpga_emulator_selector my_selector;
#elif FPGA_SIMULATOR
  std::string simulator_device_string =
      "SimulatorDevice : Multi-process Simulator (aclmsim0)";
  select_by_string my_selector = select_by_string{simulator_device_string};
#else
  sycl::ext::intel::fpga_selector my_selector;
#endif
public:
  virtual int operator()(const sycl::device &device) const {
    return my_selector.operator()(device);
  }
};
#elif SYCL_EXT_INTEL_FPGA_DEVICE_SELECTOR < 3

// 20220928 and later
#ifdef FPGA_EMULATOR
using testconfig_selector = sycl::ext::intel::fpga_emulator_selector;
#elif FPGA_SIMULATOR
using testconfig_selector = sycl::ext::intel::fpga_simulator_selector;
#else
using testconfig_selector = sycl::ext::intel::fpga_selector;
#endif

#else // SYCL_EXT_INTEL_FPGA_DEVICE_SELECTOR >= 3
inline int testconfig_selector_v(const sycl::device &device) {
#ifdef FPGA_EMULATOR
  return sycl::ext::intel::fpga_emulator_selector_v(device);
#elif FPGA_SIMULATOR
  return sycl::ext::intel::fpga_simulator_selector_v(device);
#else
  return sycl::ext::intel::fpga_selector_v(device);
#endif
}

} // namespace fpga_tools

#endif // SYCL_EXT_INTEL_FPGA_DEVICE_SELECTOR

