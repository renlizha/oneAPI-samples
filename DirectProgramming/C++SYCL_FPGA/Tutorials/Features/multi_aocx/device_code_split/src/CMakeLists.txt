set(SOURCE_FILE simple.cpp)
set(TARGET_NAME device_code_split)
set(EMULATOR_TARGET ${TARGET_NAME}.fpga_emu)
set(SIMULATOR_TARGET ${TARGET_NAME}.fpga_sim)
set(FPGA_TARGET ${TARGET_NAME}.fpga)

# FPGA board selection
if(NOT DEFINED FPGA_DEVICE)
    set(FPGA_DEVICE "intel_a10gx_pac:pac_a10")
    message(STATUS "FPGA_DEVICE was not specified.\
                    \nConfiguring the design to run on the default FPGA board ${FPGA_DEVICE} (Intel(R) PAC with Intel Arria(R) 10 GX FPGA). \
                    \nPlease refer to the README for information on board selection.")
else()
    message(STATUS "Configuring the design to run on FPGA board ${FPGA_DEVICE}")
endif()

# A SYCL ahead-of-time (AoT) compile processes the device code in two stages.
# 1. The "compile" stage compiles the device code to an intermediate representation (SPIR-V).
# 2. The "link" stage invokes the compiler's FPGA backend before linking.
#    For this reason, FPGA backend flags must be passed as link flags in CMake.
set(EMULATOR_COMPILE_FLAGS "-fsycl -fintelfpga -DFPGA_EMULATOR")
# -fsycl-device-code-split=per_kernel creates separate aocxs based on kernels
set(EMULATOR_LINK_FLAGS "-fsycl -fintelfpga -fsycl-device-code-split=per_kernel")
set(SIMULATOR_COMPILE_FLAGS "-fsycl -fintelfpga -DFPGA_SIMULATOR")
set(SIMULATOR_LINK_FLAGS "-fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS} -fsycl-device-code-split=per_kernel")
set(HARDWARE_COMPILE_FLAGS "-fsycl -fintelfpga")
set(HARDWARE_LINK_FLAGS "-fsycl -fintelfpga -Xshardware -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS}  -fsycl-device-code-split=per_kernel")
# use cmake -D USER_HARDWARE_FLAGS=<flags> to set extra flags for FPGA backend compilation

###############################################################################
### FPGA Emulator
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga -DFPGA_EMULATOR -o simple.cpp.o simple.cpp
#    [link]    icpx -fsycl -fintelfpga  -fsycl-device-code-split=per_kernel simple.cpp.o -o simple.fpga_emu
add_executable(${EMULATOR_TARGET} ${SOURCE_FILE})
target_include_directories(${EMULATOR_TARGET} PRIVATE ../../../../../include)
set_target_properties(${EMULATOR_TARGET} PROPERTIES COMPILE_FLAGS "${EMULATOR_COMPILE_FLAGS}")
set_target_properties(${EMULATOR_TARGET} PROPERTIES LINK_FLAGS "${EMULATOR_LINK_FLAGS}")
add_custom_target(fpga_emu DEPENDS ${EMULATOR_TARGET})

###############################################################################
### Generate Report
###############################################################################
# To compile manually:
#   icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=early  -fsycl-device-code-split=per_kernel simple.cpp -o simple_split_report.a
set(FPGA_EARLY_IMAGE ${TARGET_NAME}_report.a)
# The compile output is not an executable, but an intermediate compilation result unique to SYCL.
add_executable(${FPGA_EARLY_IMAGE} ${SOURCE_FILE})
target_include_directories(${FPGA_EARLY_IMAGE} PRIVATE ../../../../../include)
add_custom_target(report DEPENDS ${FPGA_EARLY_IMAGE})
set_target_properties(${FPGA_EARLY_IMAGE} PROPERTIES COMPILE_FLAGS "${HARDWARE_COMPILE_FLAGS}")
set_target_properties(${FPGA_EARLY_IMAGE} PROPERTIES LINK_FLAGS "${HARDWARE_LINK_FLAGS} -fsycl-link=early")
# fsycl-link=early stops the compiler after RTL generation, before invoking Quartus®

###############################################################################
### FPGA Simulator
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga -DFPGA_SIMULATOR -o simple.cpp.o simple.cpp
#    [link]    icpx -fsycl -fintelfpga -fsycl-device-code-split=per_kernel simple.cpp.o -o simple.fpga_simu
add_executable(${SIMULATOR_TARGET} ${SOURCE_FILE})
target_include_directories(${SIMULATOR_TARGET} PRIVATE ../../../../../include)
set_target_properties(${SIMULATOR_TARGET} PROPERTIES COMPILE_FLAGS "${SIMULATOR_COMPILE_FLAGS}")
set_target_properties(${SIMULATOR_TARGET} PROPERTIES LINK_FLAGS "${SIMULATOR_LINK_FLAGS} -reuse-exe=${CMAKE_BINARY_DIR}/${EMULATOR_TARGET}")
# The -reuse-exe flag enables rapid recompilation of host-only code changes.
add_custom_target(fpga_sim DEPENDS ${SIMULATOR_TARGET})


###############################################################################
### FPGA Hardware
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga -o simple.cpp.o simple.cpp
#    [link]    icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE>  -fsycl-device-code-split=per_kernel simple.cpp.o -o simple.fpga
add_executable(${FPGA_TARGET} ${SOURCE_FILE})
target_include_directories(${FPGA_TARGET} PRIVATE ../../../../../include)
set_target_properties(${FPGA_TARGET} PROPERTIES COMPILE_FLAGS "${HARDWARE_COMPILE_FLAGS}")
set_target_properties(${FPGA_TARGET} PROPERTIES LINK_FLAGS "${HARDWARE_LINK_FLAGS} -reuse-exe=${CMAKE_BINARY_DIR}/${EMULATOR_TARGET}")
# The -reuse-exe flag enables rapid recompilation of host-only code changes.
add_custom_target(fpga DEPENDS ${FPGA_TARGET})
