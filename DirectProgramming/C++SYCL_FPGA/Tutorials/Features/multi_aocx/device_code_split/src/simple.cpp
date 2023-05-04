#include <sycl/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>
#include <iostream>

using namespace sycl;
constexpr access::mode sycl_read = access::mode::read;
constexpr access::mode sycl_write = access::mode::write;

class SimpleAdd;
class SimpleMul;

void kernelAdd(const float a, const float b, float &c) {

  {
#if FPGA_SIMULATOR
    auto selector = sycl::ext::intel::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
    auto selector = sycl::ext::intel::fpga_selector_v;
#else // #if FPGA_EMULATOR
    auto selector = sycl::ext::intel::fpga_emulator_selector_v;
#endif

    queue q(selector);
    buffer<float, 1> bufferA(&a, 1);
    buffer<float, 1> bufferB(&b, 1);
    buffer<float, 1> bufferC(&c, 1);

    q.submit([&](handler &h) {
      auto const accessorA = bufferA.template get_access<sycl_read>(h);
      auto accessorB = bufferB.template get_access<sycl_read>(h);
      auto accessorC = bufferC.template get_access<sycl_write>(h);
      h.single_task<SimpleAdd>(
          [=]() { accessorC[0] = accessorA[0] + accessorB[0]; });
    });

    q.wait();
  }
}

void kernelMul(const float a, const float b, float &c) {

  {
#if FPGA_SIMULATOR
    auto selector = sycl::ext::intel::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
    auto selector = sycl::ext::intel::fpga_selector_v;
#else // #if FPGA_EMULATOR
    auto selector = sycl::ext::intel::fpga_emulator_selector_v;
#endif

    queue q(selector);
    buffer<float, 1> bufferA(&a, 1);
    buffer<float, 1> bufferB(&b, 1);
    buffer<float, 1> bufferC(&c, 1);

    q.submit([&](handler &h) {
      auto const accessorA = bufferA.template get_access<sycl_read>(h);
      auto accessorB = bufferB.template get_access<sycl_read>(h);
      auto accessorC = bufferC.template get_access<sycl_write>(h);
      h.single_task<SimpleMul>(
          [=]() { accessorC[0] = accessorA[0] * accessorB[0]; });
    });

    q.wait();
  }
}

int main() {
  float r1 = 0;
  float r2 = 0;
  kernelAdd(0.1f, 0.2f, r1);
  kernelMul(0.3f, 0.4f, r2);

  if (r1 != 0.1f + 0.2f) {
    std::cout << "RESULT: The results are incorrect !\n";
    return 1;
  }
  if (r2 != 0.3f * 0.4f) {
    std::cout << "RESULT: The results are incorrect !\n";
    return 1;
  }
  std::cout << "RESULT: The results are correct!\n";
  return 0;
}
