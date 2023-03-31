#include "vector_ops.h"

class SimpleVadd;
using namespace sycl;

extern "C" {
void VectorAddInDPCPP(queue q, const IntArray &addend_1,
                      const IntArray &addend_2, IntArray &sum_parallel) {

  // create the range object for the arrays managed by the buffer
  range<1> num_items{array_size};

  // create buffers that hold the data shared between the host and the devices.
  //    1st parameter: pointer of the data;
  //    2nd parameter: size of the data
  // the buffer destructor is responsible to copy the data back to host when it
  // goes out of scope.
  buffer<int, 1> addend_1_buf(addend_1.data(), num_items);
  buffer<int, 1> addend_2_buf(addend_2.data(), num_items);
  buffer<int, 1> sum_buf(sum_parallel.data(), num_items);

  // submit a command group to the queue by a lambda function that
  // contains the data access permission and device computation (kernel)
  q.submit([&](handler &h) {
    // create an accessor for each buffer with access permission: read, write or
    // read/write the accessor is the only mean to access the memory in the
    // buffer.
    auto addend_1_accessor = addend_1_buf.get_access<dp_read>(h);
    auto addend_2_accessor = addend_2_buf.get_access<dp_read>(h);

    // the sum_accessor is used to store (with write permision) the sum data
    auto sum_accessor = sum_buf.get_access<dp_write>(h);

    // Use parallel_for to run array addition in parallel on device. This
    // executes the kernel.
    //    1st parameter is the number of work items to use
    //    2nd parameter is the kernel, a lambda that specifies what to do per
    //    work item. the parameter of the lambda is the work item id of the
    //    current item.
    // DPC++ supports unnamed lambda kernel by default.
    h.parallel_for<class SimpleVadd>(num_items, [=](id<1> i) {
      sum_accessor[i] = addend_1_accessor[i] + addend_2_accessor[i];
    });
  });

  // q.submit() is an asynchronously call. DPC++ runtime enqueues and runs the
  // kernel asynchronously. at the end of the DPC++ scope the buffer's data is
  // copied back to the host.
}
}
