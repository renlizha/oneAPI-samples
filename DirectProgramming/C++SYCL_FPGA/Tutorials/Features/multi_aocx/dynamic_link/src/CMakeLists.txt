set(SOURCE_FILE_ADD vector_add.cpp)
set(SOURCE_FILE_MUL vector_mul.cpp)
set(SOURCE_FILE_MAIN main.cpp)
set(TARGET_NAME dynamic_link)
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
set(EMULATOR_COMPILE_FLAGS "-fsycl -fPIC -fintelfpga -DFPGA_EMULATOR")
set(EMULATOR_LINK_FLAGS "-fsycl -fPIC -fintelfpga")
set(SIMULATOR_COMPILE_FLAGS "-fsycl -fPIC -fintelfpga")
set(SIMULATOR_LINK_FLAGS "-fsycl -fPIC -fintelfpga -Xssimulation -Xsghdl -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS}")
set(HARDWARE_COMPILE_FLAGS "-fsycl -fPIC -fintelfpga")
set(HARDWARE_LINK_FLAGS "-fsycl -fPIC -fintelfpga -Xshardware -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS}")
# use cmake -D USER_HARDWARE_FLAGS=<flags> to set extra flags for FPGA backend compilation

set(CMAKE_CXX_FLAGS_LIST "${CMAKE_CXX_FLAGS}")
separate_arguments(CMAKE_CXX_FLAGS_LIST)
set(EMULATOR_COMPILE_FLAGS_LIST "${EMULATOR_COMPILE_FLAGS}")
separate_arguments(EMULATOR_COMPILE_FLAGS_LIST)
set(EMULATOR_LINK_FLAGS_LIST "${EMULATOR_LINK_FLAGS}")
separate_arguments(EMULATOR_LINK_FLAGS_LIST)
set(SIMULATOR_COMPILE_FLAGS_LIST "${SIMULATOR_COMPILE_FLAGS}")
separate_arguments(SIMULATOR_COMPILE_FLAGS_LIST)
set(SIMULATOR_LINK_FLAGS_LIST "${SIMULATOR_LINK_FLAGS}")
separate_arguments(SIMULATOR_LINK_FLAGS_LIST)
set(HARDWARE_COMPILE_FLAGS_LIST "${HARDWARE_COMPILE_FLAGS}")
separate_arguments(HARDWARE_COMPILE_FLAGS_LIST)
set(HARDWARE_LINK_FLAGS_LIST "${HARDWARE_LINK_FLAGS}")
separate_arguments(HARDWARE_LINK_FLAGS_LIST)

###############################################################################
### FPGA Emulator
###############################################################################
# To compile manually:
#    icpx -fsycl -fintelfpga -fPIC -o vector_add.o -c vector_add.cpp
#    icpx -fsycl -fintelfpga -fPIC -shared vector_add.o -o add.so
#    icpx -fsycl -fintelfpga -fPIC -o vector_mul.o -c vector_mul.cpp
#    icpx -fsycl -fintelfpga -fPIC -shared vector_mul.o -o mul.so
#    icpx -fsycl main.cpp.o mul.so add.so -o main.fpga_emu
add_custom_target(fpga_emu DEPENDS ${EMULATOR_TARGET})
set(ADD_OBJ_EMU add_emu.o)
set(MUL_OBJ_EMU mul_emu.o)
set(ADD_SHARED_EMU add_emu.so)
set(MUL_SHARED_EMU mul_emu.so)

add_custom_command(OUTPUT ${ADD_OBJ_EMU}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${ADD_OBJ_EMU} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_OBJ_EMU}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${MUL_OBJ_EMU} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${ADD_SHARED_EMU}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -shared ${ADD_OBJ_EMU} -o ${CMAKE_BINARY_DIR}/${ADD_SHARED_EMU} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${ADD_OBJ_EMU})

add_custom_command(OUTPUT ${MUL_SHARED_EMU}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -shared ${MUL_OBJ_EMU} -o ${CMAKE_BINARY_DIR}/${MUL_SHARED_EMU} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MUL_OBJ_EMU})

add_custom_command(OUTPUT ${EMULATOR_TARGET}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} -fsycl -DFPGA_EMULATOR ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} ${CMAKE_BINARY_DIR}/${ADD_SHARED_EMU} ${CMAKE_BINARY_DIR}/${MUL_SHARED_EMU} -o ${CMAKE_BINARY_DIR}/${EMULATOR_TARGET} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN} ${ADD_SHARED_EMU} ${MUL_SHARED_EMU})

###############################################################################
### Generate Report
###############################################################################
# To compile manually:
#    icpx -fsycl -fintelfpga -fPIC -o vector_add.o -c vector_add.cpp
#    icpx -fsycl -fintelfpga -fPIC -shared vector_add.o -o add.so
#    icpx -fsycl -fintelfpga -fPIC -o vector_mul.o -c vector_mul.cpp
#    icpx -fsycl -fintelfpga -fPIC -shared vector_mul.o -o mul.so
#   icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=early main.cpp mul.so add.so -o dynamic_link_report.a
#   fsycl-link=early stops the compiler after RTL generation, before invoking Quartus®
set(FPGA_EARLY_IMAGE ${TARGET_NAME}_report.a)
# The compile output is not an executable, but an intermediate compilation result unique to SYCL.
add_custom_target(report DEPENDS ${FPGA_EARLY_IMAGE})
set(ADD_OBJ_REPORT add_report.o)
set(MUL_OBJ_REPORT mul_report.o)
set(ADD_SHARED_REPORT add_report.so)
set(MUL_SHARED_REPORT mul_report.so)

add_custom_command(OUTPUT ${ADD_OBJ_REPORT}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${ADD_OBJ_REPORT} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_OBJ_REPORT}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${MUL_OBJ_REPORT} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${ADD_SHARED_REPORT}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -shared ${ADD_OBJ_REPORT} -o ${CMAKE_BINARY_DIR}/${ADD_SHARED_REPORT} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${ADD_OBJ_REPORT})

add_custom_command(OUTPUT ${MUL_SHARED_REPORT}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} -shared ${MUL_OBJ_REPORT} -o ${CMAKE_BINARY_DIR}/${MUL_SHARED_REPORT} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MUL_OBJ_REPORT})

add_custom_command(OUTPUT ${FPGA_EARLY_IMAGE}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} -fsycl -DFPGA_EMULATOR ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} ${CMAKE_BINARY_DIR}/${ADD_SHARED_REPORT} ${CMAKE_BINARY_DIR}/${MUL_SHARED_REPORT} -o ${CMAKE_BINARY_DIR}/${FPGA_EARLY_IMAGE} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN} ${ADD_SHARED_REPORT} ${MUL_SHARED_REPORT})

###############################################################################
### FPGA Simulator
###############################################################################
# To compile manually:
#    icpx -fsycl -fintelfpga -DFPGA_SIMULATOR -fPIC -o vector_add.o -c vector_add.cpp
#    icpx -fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> -fPIC -shared vector_add.o -o add.so
#    icpx -fsycl -fintelfpga -DFPGA_SIMULATOR fPIC -o vector_mul.o -c vector_mul.cpp
#    icpx -fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> -fPIC -shared vector_mul.o -o mul.so
#    icpx -fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> main.cpp mul.so add.so -o main.fpga_sim
add_custom_target(fpga_sim DEPENDS ${SIMULATOR_TARGET})
set(ADD_OBJ_SIM add_sim.o)
set(MUL_OBJ_SIM mul_sim.o)
set(ADD_SHARED_SIM add_sim.so)
set(MUL_SHARED_SIM mul_sim.so)

add_custom_command(OUTPUT ${ADD_OBJ_SIM}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${SIMULATOR_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${ADD_OBJ_SIM} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_OBJ_SIM}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${SIMULATOR_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${MUL_OBJ_SIM} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${ADD_SHARED_SIM}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${SIMULATOR_LINK_FLAGS_LIST} -shared ${ADD_OBJ_SIM} -o ${CMAKE_BINARY_DIR}/${ADD_SHARED_SIM} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${ADD_OBJ_SIM})

add_custom_command(OUTPUT ${MUL_SHARED_SIM}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${SIMULATOR_LINK_FLAGS_LIST} -shared ${MUL_OBJ_SIM} -o ${CMAKE_BINARY_DIR}/${MUL_SHARED_SIM} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MUL_OBJ_SIM})

add_custom_command(OUTPUT ${SIMULATOR_TARGET}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} -fsycl -DFPGA_SIMULATOR ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} ${CMAKE_BINARY_DIR}/${ADD_SHARED_SIM} ${CMAKE_BINARY_DIR}/${MUL_SHARED_SIM} -o ${CMAKE_BINARY_DIR}/${SIMULATOR_TARGET} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN} ${ADD_SHARED_SIM} ${MUL_SHARED_SIM})


###############################################################################
### FPGA Hardware
###############################################################################
# To compile manually:
#    icpx -fsycl -fintelfpga -fPIC -o vector_add.o -c vector_add.cpp
#    icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> -fPIC -shared vector_add.o -o add.so
#    icpx -fsycl -fintelfpga -fPIC -o vector_mul.o -c vector_mul.cpp
#    icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> -fPIC -shared vector_mul.o -o mul.so
#    icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> main.cpp mul.so add.so -o main.fpga
add_custom_target(fpga DEPENDS ${FPGA_TARGET})
set(ADD_OBJ add.o)
set(MUL_OBJ mul.o)
set(ADD_SHARED add.so)
set(MUL_SHARED mul.so)

add_custom_command(OUTPUT ${ADD_OBJ}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${ADD_OBJ} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_OBJ}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${MUL_OBJ} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${ADD_SHARED}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_LINK_FLAGS_LIST} -shared ${ADD_OBJ} -o ${CMAKE_BINARY_DIR}/${ADD_SHARED} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${ADD_OBJ})

add_custom_command(OUTPUT ${MUL_SHARED}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_LINK_FLAGS_LIST} -shared ${MUL_OBJ} -o ${CMAKE_BINARY_DIR}/${MUL_SHARED} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MUL_OBJ})

add_custom_command(OUTPUT ${FPGA_TARGET}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} -fsycl ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} ${CMAKE_BINARY_DIR}/${ADD_SHARED} ${CMAKE_BINARY_DIR}/${MUL_SHARED} -o ${CMAKE_BINARY_DIR}/${FPGA_TARGET} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN} ${ADD_SHARED} ${MUL_SHARED})
