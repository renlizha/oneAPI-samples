#ifndef VECTOR_OPS_H
#define VECTOR_OPS_H

#include <sycl/sycl.hpp>
#include <array>

using namespace sycl;
// ARRAY type & data size for use in this example
constexpr size_t array_size = 10000;
typedef std::array<int, array_size> IntArray;

// Convience data access definitions
constexpr access::mode dp_read = access::mode::read;
constexpr access::mode dp_write = access::mode::write;
constexpr access::mode dp_read_write = access::mode::read_write;

extern "C" void VectorAddInDPCPP(queue q, const IntArray &addend_1,
                                 const IntArray &addend_2,
                                 IntArray &sum_parallel);

extern "C" void VectorMulInDPCPP(queue q, const IntArray &addend_1,
                                 const IntArray &addend_2,
                                 IntArray &sum_parallel);

#endif