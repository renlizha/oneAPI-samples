set(SOURCE_FILE_ADD vector_add.cpp)
set(SOURCE_FILE_MUL vector_mul.cpp)
set(SOURCE_FILE_MAIN main.cpp)
set(TARGET_NAME device_link)
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
set(EMULATOR_LINK_FLAGS "-fsycl -fintelfpga")
set(SIMULATOR_COMPILE_FLAGS "-fsycl -fintelfpga -DFPGA_SIMULATOR")
set(SIMULATOR_LINK_FLAGS "-fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS}")
set(HARDWARE_COMPILE_FLAGS "-fsycl -fintelfpga")
set(HARDWARE_LINK_FLAGS "-fsycl -fintelfpga -Xshardware -Xstarget=${FPGA_DEVICE} ${USER_HARDWARE_FLAGS}")
set(LINK_FLAGS_IMAGE "-fsycl-link=image")
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
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga -DFPGA_EMULATOR -o vector_add.o -c vector_add.cpp
#    [link] icpx -fsycl -fintelfpga -DFPGA_EMULATOR -fsycl-link=image vector_add.o -o add.a
#    [compile] icpx -fsycl -fintelfpga -DFPGA_EMULATOR -o vector_mul.o -c vector_mul.cpp
#    [link] icpx -fsycl -fintelfpga -DFPGA_EMULATOR -fsycl-link=image vector_mul.o -o mul.a
#    [compile] icpx -fsycl -fintelfpga -DFPGA_EMULATOR -o main.cpp.o -c main.cpp
#    [link]    icpx -fsycl -fintelfpga main.cpp.o mul.a add.a -o main.fpga_emu
add_custom_target(fpga_emu DEPENDS ${EMULATOR_TARGET})
set(MAIN_OBJ_EMU main_emu.o)
set(ADD_IMAGE_EMU add_emu.a)
set(MUL_IMAGE_EMU mul_emu.a)

add_custom_command(OUTPUT ${MAIN_OBJ_EMU}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} -o ${CMAKE_BINARY_DIR}/${MAIN_OBJ_EMU} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN})

add_custom_command(OUTPUT ${ADD_IMAGE_EMU}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${CMAKE_BINARY_DIR}/${ADD_IMAGE_EMU} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_IMAGE_EMU}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${EMULATOR_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${CMAKE_BINARY_DIR}/${MUL_IMAGE_EMU} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${EMULATOR_TARGET}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} -fsycl -fintelfpga ${CMAKE_BINARY_DIR}/${MAIN_OBJ_EMU} ${CMAKE_BINARY_DIR}/${ADD_IMAGE_EMU} ${CMAKE_BINARY_DIR}/${MUL_IMAGE_EMU} -o ${CMAKE_BINARY_DIR}/${EMULATOR_TARGET} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MAIN_OBJ_EMU} ${ADD_IMAGE_EMU} ${MUL_IMAGE_EMU})

###############################################################################
### Generate Report
###############################################################################
# To compile manually:
#   icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=early main.cpp mul.a add.a -o device_link_report.a
#   fsycl-link=early stops the compiler after RTL generation, before invoking Quartus®
set(FPGA_EARLY_IMAGE ${TARGET_NAME}_report.a)
# The compile output is not an executable, but an intermediate compilation result unique to SYCL.
add_custom_target(report DEPENDS ${FPGA_EARLY_IMAGE})
set(MAIN_REPORT main_report.o)
set(ADD_IMAGE_REPORT add_report.a)
set(MUL_IMAGE_REPORT mul_report.a)

add_custom_command(OUTPUT ${MAIN_REPORT}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} -o ${CMAKE_BINARY_DIR}/${MAIN_REPORT} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN})

add_custom_command(OUTPUT ${ADD_IMAGE_REPORT}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${CMAKE_BINARY_DIR}/${ADD_IMAGE_REPORT} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_IMAGE_REPORT}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${CMAKE_BINARY_DIR}/${MUL_IMAGE_REPORT} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${FPGA_EARLY_IMAGE}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -fsycl -fintelfpga -fsycl-link=early ${CMAKE_BINARY_DIR}/${MAIN_REPORT} ${CMAKE_BINARY_DIR}/${ADD_IMAGE_REPORT} ${CMAKE_BINARY_DIR}/${MUL_IMAGE_REPORT} -o ${CMAKE_BINARY_DIR}/${FPGA_EARLY_IMAGE} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MAIN_REPORT} ${ADD_IMAGE_REPORT} ${MUL_IMAGE_REPORT})

###############################################################################
### FPGA Simulator
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga -DFPGA_SIMULATOR -o vector_add.o -c vector_add.cpp
#    [link] icpx -fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_add.o -o add.a
#    [compile] icpx -fsycl -fintelfpga -DFPGA_SIMULATOR -o vector_mul.o -c vector_mul.cpp
#    [link] icpx -fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_mul.o -o mul.a
#    [compile] icpx -fsycl -fintelfpga -DFPGA_SIMULATOR -o main.cpp.o -c main.cpp
#    [link]    icpx -fsycl -fintelfpga -Xssimulation -Xsghdl -Xstarget=<FPGA_DEVICE> main.cpp.o mul.a add.a -o main.fpga_emu
add_custom_target(fpga_sim DEPENDS ${SIMULATOR_TARGET})
set(MAIN_OBJ_SIM main_sim.o)
set(ADD_IMAGE_SIM add_sim.a)
set(MUL_IMAGE_SIM mul_sim.a)

add_custom_command(OUTPUT ${MAIN_OBJ_SIM}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${SIMULATOR_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} -o ${CMAKE_BINARY_DIR}/${MAIN_OBJ_SIM} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN})

add_custom_command(OUTPUT ${ADD_IMAGE_SIM}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${SIMULATOR_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${CMAKE_BINARY_DIR}/${ADD_IMAGE_SIM} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_IMAGE_SIM}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${SIMULATOR_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${CMAKE_BINARY_DIR}/${MUL_IMAGE_SIM} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${SIMULATOR_TARGET}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -fsycl -fintelfpga ${CMAKE_BINARY_DIR}/${MAIN_OBJ_SIM} ${CMAKE_BINARY_DIR}/${ADD_IMAGE_SIM} ${CMAKE_BINARY_DIR}/${MUL_IMAGE_SIM} -o ${CMAKE_BINARY_DIR}/${SIMULATOR_TARGET} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MAIN_OBJ_SIM} ${ADD_IMAGE_SIM} ${MUL_IMAGE_SIM})


###############################################################################
### FPGA Hardware
###############################################################################
# CMake executes:
#    [compile] icpx -fsycl -fintelfpga -o vector_add.o -c vector_add.cpp
#    [link] icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_add.o -o add.a
#    [compile] icpx -fsycl -fintelfpga -o vector_mul.o -c vector_mul.cpp
#    [link] icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> -fsycl-link=image vector_mul.o -o mul.a
#    [compile] icpx -fsycl -fintelfpga -o main.cpp.o -c main.cpp
#    [link]    icpx -fsycl -fintelfpga -Xshardware -Xstarget=<FPGA_DEVICE> main.cpp.o mul.a add.a -o main.fpga_emu
add_custom_target(fpga DEPENDS ${FPGA_TARGET})
set(MAIN_OBJ main.o)
set(ADD_IMAGE add.a)
set(MUL_IMAGE mul.a)

add_custom_command(OUTPUT ${MAIN_OBJ}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_COMPILE_FLAGS_LIST} -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MAIN} -o ${CMAKE_BINARY_DIR}/${MAIN_OBJ} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MAIN})

add_custom_command(OUTPUT ${ADD_IMAGE}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_ADD} -o ${CMAKE_BINARY_DIR}/${ADD_IMAGE} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_ADD})

add_custom_command(OUTPUT ${MUL_IMAGE}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_LIST} ${HARDWARE_LINK_FLAGS_LIST} ${LINK_FLAGS_IMAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE_MUL} -o ${CMAKE_BINARY_DIR}/${MUL_IMAGE} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${SOURCE_FILE_MUL})

add_custom_command(OUTPUT ${FPGA_TARGET}
                COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -fsycl -fintelfpga ${CMAKE_BINARY_DIR}/${MAIN_OBJ} ${CMAKE_BINARY_DIR}/${ADD_IMAGE} ${CMAKE_BINARY_DIR}/${MUL_IMAGE} -o ${CMAKE_BINARY_DIR}/${FPGA_TARGET} -I${CMAKE_CURRENT_LIST_DIR}/../../../../../include
                DEPENDS ${MAIN_OBJ} ${ADD_IMAGE} ${MUL_IMAGE})
