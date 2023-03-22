//==============================================================
// Copyright © 2020 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <sycl/sycl.hpp>
#include <array>
#include <dlfcn.h>
#include <iostream>

#include "ExceptionHandler.h"
#include "TestConfigSelector.h"
#include "vector_ops.h"

using namespace sycl;

// number of iterations
const int max_iterations = 10;

// output message for runtime exceptions
#define EXCEPTION_MSG                                                          \
  "    If you are targeting an FPGA hardware, please ensure that an FPGA board is plugged to the system, \n\
        set up correctly and compile with -DFPGA  \n\
    If you are targeting the FPGA emulator, compile with -DFPGA_EMULATOR.\n"

//************************************
// Function description: initialize the array from 0 to array_size-1
//************************************
void initialize_array(IntArray &a) {
  for (size_t i = 0; i < a.size(); i++)
    a[i] = i;
}

//************************************
// Demonstrate summation of arrays both in scalar on CPU and parallel on device
//************************************
int main() {
  // create int array objects with "array_size" to store the input and output
  // data

  IntArray addend_1, addend_2, scalar, parallel;

  // Initialize input arrays with values from 0 to array_size-1
  initialize_array(addend_1);

  initialize_array(addend_2);

  queue devq(testconfig_selector_v, &m_exception_handler);

  void (*vector_add)(queue, const IntArray &, const IntArray &, IntArray &);
  void (*vector_mul)(queue, const IntArray &, const IntArray &, IntArray &);
  void *library_add = dlopen("./vector_add.so", RTLD_NOW);
  if (library_add == NULL) {
    fprintf(stderr, "Unable to open vector_add library: %s\n", dlerror());
    exit(1);
  }

  vector_add = (void (*)(queue, const IntArray &, const IntArray &,
                         IntArray &))dlsym(library_add, "VectorAddInDPCPP");
  if (vector_add == NULL) {
    fprintf(stderr, "Failed to load vector add %s\n", dlerror());
    exit(1);
  }
  void *library_mul = dlopen("./vector_mul.so", RTLD_NOW);
  if (library_mul == NULL) {
    fprintf(stderr, "Unable to open vector_mul library: %s\n", dlerror());
    exit(1);
  }
  vector_mul = (void (*)(queue, const IntArray &, const IntArray &,
                         IntArray &))dlsym(library_mul, "VectorMulInDPCPP");
  if (vector_mul == NULL) {
    fprintf(stderr, "Failed to load vector mul %s\n", dlerror());
    exit(1);
  }

  // std::cout << "Device: " << devq.get_device().get_info<sycl::info::device::name>()
  //          << std::endl;
  for (int iter = 0; iter < max_iterations; iter++) {

    // Compute vector addition in DPC++
    vector_add(devq, addend_1, addend_2, parallel);
    vector_mul(devq, addend_1, addend_2, parallel);

    // Computes the sum of two arrays in scalar for validation
    for (size_t i = 0; i < scalar.size(); i++)
      scalar[i] = (addend_1[i] + addend_2[i]) + (addend_1[i] * addend_2[i]);

    // Verify that the two sum arrays are equal
    for (size_t i = 0; i < parallel.size(); i++) {
      if (parallel[i] != scalar[i]) {
        std::cout << "fail" << std::endl;
        return -1;
      }
    }
  }

  std::cout << "success" << std::endl;

  return 0;
}