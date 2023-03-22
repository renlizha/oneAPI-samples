set(SOURCE_FILE_ADD ../src/vector_add.cpp)
set(SOURCE_FILE_MUL ../src/vector_mul.cpp)
set(SOURCE_FILE_MAIN ../src/main.cpp)
set(TARGET_NAME main)
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
set(EMULATOR_COMPILE_FLAGS "-fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_EMULATOR -Wall ${WIN_FLAG}")
set(EMULATOR_LINK_FLAGS "-fsycl -fintelfpga ${AC_TYPES_FLAG}")
set(SIMULATOR_COMPILE_FLAGS "-fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_SIMULATOR -Wall ${WIN_FLAG}")
set(SIMULATOR_LINK_FLAGS "-fsycl -fintelfpga ${AC_TYPES_FLAG} -Xssimulation -Xsghdl -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS}")
set(HARDWARE_COMPILE_FLAGS "-fsycl -fintelfpga ${AC_TYPES_FLAG} -Wall ${WIN_FLAG}")
set(HARDWARE_LINK_FLAGS "-fsycl -fintelfpga ${AC_TYPES_FLAG} -Xshardware -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS}")
set(LINK_FLAGS_IMAGE "-fsycl-link=image")
# use cmake -D USER_HARDWARE_FLAGS=<flags> to set extra flags for FPGA backend compilation

###############################################################################
### FPGA Emulator
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_EMULATOR -o vector_add.o -c vector_add.cpp
#    [link] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_EMULATOR -fsycl-link=image vector_add.o -o add.a
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_EMULATOR -o vector_mul.o -c vector_mul.cpp
#    [link] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_EMULATOR -fsycl-link=image vector_mul.o -o mul.a
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_EMULATOR -o main.cpp.o -c main.cpp
#    [link]    icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} main.cpp.o mul.a add.a -o main.fpga_emu
add_library(library_add STATIC ${SOURCE_FILE_ADD})
add_library(library_mul STATIC ${SOURCE_FILE_MUL})
set_target_properties(library_add PROPERTIES COMPILE_FLAGS "${EMULATOR_COMPILE_FLAGS}")
set_target_properties(library_add PROPERTIES LINK_FLAGS "${EMULATOR_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
set_target_properties(library_mul PROPERTIES COMPILE_FLAGS "${EMULATOR_COMPILE_FLAGS}")
set_target_properties(library_mul PROPERTIES LINK_FLAGS "${EMULATOR_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
add_executable(${EMULATOR_TARGET} ${SOURCE_FILE_MAIN})
target_include_directories(${EMULATOR_TARGET} PRIVATE ../../../../include)
set_target_properties(${EMULATOR_TARGET} PROPERTIES COMPILE_FLAGS "${EMULATOR_COMPILE_FLAGS}")
set_target_properties(${EMULATOR_TARGET} PROPERTIES LINK_FLAGS "${EMULATOR_LINK_FLAGS}")
target_link_libraries(${EMULATOR_TARGET} library_add library_mul)
add_custom_target(fpga_emu DEPENDS ${EMULATOR_TARGET})

###############################################################################
### Generate Report
###############################################################################
# To compile manually:
#   icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=early main.cpp mul.a add.a -o ac_fixed_report.a
set(FPGA_EARLY_IMAGE ${TARGET_NAME}_report.a)
# The compile output is not an executable, but an intermediate compilation result unique to SYCL.
add_library(library_add STATIC ${SOURCE_FILE_ADD})
add_library(library_mul STATIC ${SOURCE_FILE_MUL})
set_target_properties(library_add PROPERTIES COMPILE_FLAGS "${EMULATOR_COMPILE_FLAGS}")
set_target_properties(library_add PROPERTIES LINK_FLAGS "${EMULATOR_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
set_target_properties(library_mul PROPERTIES COMPILE_FLAGS "${EMULATOR_COMPILE_FLAGS}")
set_target_properties(library_mul PROPERTIES LINK_FLAGS "${EMULATOR_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
add_executable(${FPGA_EARLY_IMAGE} ${SOURCE_FILE_MAIN})
target_include_directories(${FPGA_EARLY_IMAGE} PRIVATE ../../../../include)
add_custom_target(report DEPENDS ${FPGA_EARLY_IMAGE})
set_target_properties(${FPGA_EARLY_IMAGE} PROPERTIES COMPILE_FLAGS "${HARDWARE_COMPILE_FLAGS}")
set_target_properties(${FPGA_EARLY_IMAGE} PROPERTIES LINK_FLAGS "${HARDWARE_LINK_FLAGS} -fsycl-link=early")
# fsycl-link=early stops the compiler after RTL generation, before invoking Quartus®
target_link_libraries(${EMULATOR_TARGET} library_add library_mul)

###############################################################################
### FPGA Simulator
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_SIMULATOR -o vector_add.o -c vector_add.cpp
#    [link] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_add.o -o add.a
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_SIMULATOR -o vector_mul.o -c vector_mul.cpp
#    [link] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_mul.o -o mul.a
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -DFPGA_SIMULATOR -o main.cpp.o -c main.cpp
#    [link]    icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> main.cpp.o mul.a add.a -o main.fpga_emu
add_library(library_add STATIC ${SOURCE_FILE_ADD})
add_library(library_mul STATIC ${SOURCE_FILE_MUL})
set_target_properties(library_add PROPERTIES COMPILE_FLAGS "${SIMULATOR_COMPILE_FLAGS}")
set_target_properties(library_add PROPERTIES LINK_FLAGS "${SIMULATOR_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
set_target_properties(library_mul PROPERTIES COMPILE_FLAGS "${SIMULATOR_COMPILE_FLAGS}")
set_target_properties(library_mul PROPERTIES LINK_FLAGS "${SIMULATOR_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
add_executable(${SIMULATOR_TARGET} ${SOURCE_FILE_MAIN})
target_include_directories(${SIMULATOR_TARGET} PRIVATE ../../../../include)
set_target_properties(${SIMULATOR_TARGET} PROPERTIES COMPILE_FLAGS "${SIMULATOR_COMPILE_FLAGS}")
set_target_properties(${SIMULATOR_TARGET} PROPERTIES LINK_FLAGS "${SIMULATOR_LINK_FLAGS} -reuse-exe=${CMAKE_BINARY_DIR}/${SIMULATOR_TARGET}")
# The -reuse-exe flag enables rapid recompilation of host-only code changes.
target_link_libraries(${SIMULATOR_TARGET} library_add library_mul)
add_custom_target(fpga_sim DEPENDS ${SIMULATOR_TARGET})


###############################################################################
### FPGA Hardware
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -o vector_add.o -c vector_add.cpp
#    [link] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_add.o -o add.a
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -o vector_mul.o -c vector_mul.cpp
#    [link] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_mul.o -o mul.a
#    [compile] icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -o main.cpp.o -c main.cpp
#    [link]    icpx -fsycl -fintelfpga ${AC_TYPES_FLAG} -Xshardware -Xstarget=<FPGA_DEVICE> main.cpp.o mul.a add.a -o main.fpga_emu
add_library(library_add STATIC ${SOURCE_FILE_ADD})
add_library(library_mul STATIC ${SOURCE_FILE_MUL})
set_target_properties(library_add PROPERTIES COMPILE_FLAGS "${HARDWARE_COMPILE_FLAGS}")
set_target_properties(library_add PROPERTIES LINK_FLAGS "${HARDWARE_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
set_target_properties(library_mul PROPERTIES COMPILE_FLAGS "${HARDWARE_COMPILE_FLAGS}")
set_target_properties(library_mul PROPERTIES LINK_FLAGS "${HARDWARE_LINK_FLAGS} {LINK_FLAGS_IMAGE}")
add_executable(${FPGA_TARGET} ${SOURCE_FILE_MAIN})
target_include_directories(${FPGA_TARGET} PRIVATE ../../../../include)
set_target_properties(${FPGA_TARGET} PROPERTIES COMPILE_FLAGS "${HARDWARE_COMPILE_FLAGS}")
set_target_properties(${FPGA_TARGET} PROPERTIES LINK_FLAGS "${HARDWARE_LINK_FLAGS} -reuse-exe=${CMAKE_BINARY_DIR}/${FPGA_TARGET}")
# The -reuse-exe flag enables rapid recompilation of host-only code changes.
target_link_libraries(${FPGA_TARGET} library_add library_mul)
add_custom_target(fpga DEPENDS ${FPGA_TARGET})
