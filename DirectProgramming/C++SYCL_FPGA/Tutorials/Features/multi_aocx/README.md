# Using the flows test 'multi_aocx'

This FPGA tutorial demonstrates the flows in multi_aocx

| Optimized for                     | Description
|:---                               |:---
| OS                                | Linux* Ubuntu* 18.04/20.04 <br> RHEL*/CentOS* 8 <br> SUSE* 15 <br> Windows* 10
| Hardware                          | Intel&reg; Programmable Acceleration Card (PAC) with Intel Arria&reg; 10 GX FPGA <br> Intel&reg; FPGA Programmable Acceleration Card (PAC) D5005 (with Intel Stratix&reg; 10 SX) <br> Intel&reg; FPGA 3rd party / custom platforms with oneAPI support <br> **Note**: Intel&reg; FPGA PAC hardware is only compatible with Ubuntu 18.04*
| Software                          | Intel® oneAPI DPC++/C++ Compiler
| What you will learn               | How to use the `multi_aocx` flow tests to split kernels into multiple different images to split up compile time.
| Time to complete                  | 30 minutes

> **Note**: Even though the Intel DPC++/C++ OneAPI compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> For using the simulator flow, one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Intel® FPGA Edition
> - Questa*-Intel® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Intel® Quartus® Prime Pro Edition must be installed and accessible through your PATH.

## Purpose

`multi_aocx` could be used to split up your design into multiple images thus shorten the compilt time. Runtime automatically chooses between the images when user calls a specific function. It is useful for large designs.

- `device_link` could be used to parallelize the individual compiles when users don't need to restructure their code. 
- `-fsycl-device-code-split=[per_kernel|per_source]` flag could be used when a design doesn't fit a single FPGA, so user must split it into multiple smaller images.
- `dynamic_link` could be used to load all libraries at runtime therefore generates smaller binary.
- `dynamic_load` could be used to dynamically load only the libaries that's needed instead of all.

### Simple Code Example

`device_link`:
```
icpx ... vector_mul.cpp -fsycl-link=image -o mul.a
icpx ... vector_add.cpp -fsycl-link=image -o add.a
icpx ... main.cpp mul.a add.a -o main.exe
```
`-fsycl-device-code-split=[per_kernel|per_source]`:
```
icpx ... simple.cpp -o simple.exe -fsycl-device-code-split=per_kernel
icpx ... -reuse-exe=simple.exe -Xsv
```
`dynamic_link`:
```
icpx ... -fPIC -c vector_mul.cpp -o mul.o
icpx ... -fPIC -shared mul.o -o mul.so
icpx ... -fPIC -c vector_add.cpp -o add.o
icpx ... -fPIC -shared add.o -o add.so
icpx ... main.cpp mul.so add.so -o main.exe
```
`dynamic_load`:
```
icpx ... -fPIC -c vector_mul.cpp -o mul.o
icpx ... -fPIC -shared mul.o -o mul.so
icpx ... -fPIC -c vector_add.cpp -o add.o
icpx ... -fPIC -shared add.o -o add.so
icpx ... main.cpp -o main.exe
```

### Using the multi-aocx flows

`device_link`:
- Only cpp (aocx) file that changed needs to be recompiled
- Change to one cpp file requires recompile of executable
- All aocxs are embedded in single executable binary, therefore not scalable
- Recommended for small designs with few aocxs

`-fsycl-device-code-split=[per_kernel|per_source]`:
- Split a large design into smaller images
  - `per_kernel` - a separate device code module is created for each SYCL kernel
  - `per_source` - a separate device code module is created for each source
- Users don't need to restructure their code 
- All aocxs need to be recompiled if any code or target changed

`dynamic_link`:
- Only libraries that changed needs to be recompiled
- Loads all libraries at runtime therefore generates a smaller binary
- All libraries are loaded to memory therefore not scalable
- Recommended for smaller designs with few aocxs

`dynamic_load`:
- Like dynamic_link BUT Load the shared lib with dlopen in host code to access
- Only loaded libraries are in memory
- Recommended for designs with many aocxs


## Key Concepts



## Building the `multi_aocx` Tutorial

> **Note**: If you have not already done so, set up your CLI
> environment by sourcing  the `setvars` script located in
> the root of your oneAPI installation.
>
> Linux*:
>
> - For system wide installations: `. /opt/intel/oneapi/setvars.sh`
> - For private installations: `. ~/intel/oneapi/setvars.sh`
>
> Windows*:
>
> - `C:\Program Files(x86)\Intel\oneAPI\setvars.bat`
>
>For more information on environment variables, see **Use the setvars Script** for [Linux or macOS](https://www.intel.com/content/www/us/en/develop/documentation/oneapi-programming-guide/top/oneapi-development-environment-setup/use-the-setvars-script-with-linux-or-macos.html), or [Windows](https://www.intel.com/content/www/us/en/develop/documentation/oneapi-programming-guide/top/oneapi-development-environment-setup/use-the-setvars-script-with-windows.html).

### Running Samples in Intel&reg; DevCloud

If you are running a sample in the Intel&reg; DevCloud, remember that you must specify the type of compute node and whether to run in batch or interactive mode:

- Compiles to FPGA are supported only on `fpga_compile` nodes.
- Executing programs on FPGA hardware is supported only on `fpga_runtime` nodes of the appropriate type, such as `fpga_runtime:arria10` or `fpga_runtime:stratix10`.

On the login nodes, you cannot compile or execute programs on FPGA hardware. For more information, see the Intel&reg; oneAPI Base Toolkit Get Started Guide ([https://devcloud.intel.com/oneapi/documentation/base-toolkit/](https://devcloud.intel.com/oneapi/documentation/base-toolkit/)).

When compiling for FPGA hardware, increase the job timeout to 12h.

### Using Visual Studio Code*  (Optional)

You can use Visual Studio Code (VS Code) extensions to set your environment, create launch configurations,
and browse and download samples.

The basic steps to build and run a sample using VS Code include:

- Download a sample using the extension **Code Sample Browser for Intel&reg; oneAPI Toolkits**.
- Configure the oneAPI environment with the extension **Environment Configurator for Intel&reg; oneAPI Toolkits**.
- Open a Terminal in VS Code (**Terminal>New Terminal**).
- Run the sample in the VS Code terminal using the instructions below.

To learn more about the extensions and how to configure the oneAPI environment, see the
[Using Visual Studio Code with Intel&reg; oneAPI Toolkits User Guide](https://software.intel.com/content/www/us/en/develop/documentation/using-vs-code-with-intel-oneapi/top.html).

### On a Linux* System

1. Install the design in `build` directory from the design directory by running `cmake`:

   ```bash
   mkdir build
   cd build
   ```

   If you are compiling for the Intel&reg; PAC with Intel Arria&reg; 10 GX FPGA, run `cmake` using the command:

   ```bash
   cmake ..
   ```

   Alternatively, to compile for the Intel&reg; FPGA PAC D5005 (with Intel Stratix&reg; 10 SX), run `cmake` using the command:

   ```bash
   cmake .. -DFPGA_DEVICE=intel_s10sx_pac:pac_s10
   ```

   You can also compile for a custom FPGA platform. Ensure that the board support package is installed on your system. Then run `cmake` using the command:

   ```bash
   cmake .. -DFPGA_DEVICE=<board-support-package>:<board-variant>
   ```

2. Compile the design using the generated `Makefile`. The following four build targets are provided that match the recommended development flow:

   - Compile and run for emulation (fast compile time, targets emulates an FPGA device) using:

     ```bash
     make fpga_emu
     ```

   - Generate HTML optimization reports using:

     ```bash
     make report
     ```

   - Compile for simulation (fast compile time, targets simulated FPGA device)

     ```bash
     make fpga_sim
     ```

   - Compile and run on FPGA hardware (longer compile time, targets an FPGA device) using:

     ```bash
     make fpga
     ```

3. (Optional) As the earlier hardware compile can take several hours to complete, FPGA precompiled binaries (compatible with Ubuntu 18.04) can be downloaded [here](https://iotdk.intel.com/fpga-precompiled-binaries/latest/multi_aocx.fpga.tar.gz).

### On a Windows* System

1. Generate the `Makefile` by running `cmake`.

   ```
   mkdir build
   cd build
   ```

   To compile for the Intel&reg; PAC with Intel Arria&reg; 10 GX FPGA, run `cmake` using the command:

    ```
    cmake -G "NMake Makefiles" ..
   ```

   Alternatively, to compile for the Intel&reg; FPGA PAC D5005 (with Intel Stratix&reg; 10 SX), run `cmake` using the command:

   ```
   cmake -G "NMake Makefiles" .. -DFPGA_DEVICE=intel_s10sx_pac:pac_s10
   ```

   You can also compile for a custom FPGA platform. Ensure that the board support package is installed on your system. Then run `cmake` using the command:

   ```
   cmake -G "NMake Makefiles" .. -DFPGA_DEVICE=<board-support-package>:<board-variant>
   ```

2. Compile the design through the generated `Makefile`. The following build targets are provided, matching the recommended development flow:

   - Compile for emulation (fast compile time, targets emulated FPGA device):

     ```
     nmake fpga_emu
     ```

   - Generate the optimization report:

     ```
     nmake report
     ```

   - Compile for simulation (fast compile time, targets simulated FPGA device, reduced problem size):

     ```
     nmake fpga_sim
     ```

   - Compile for FPGA hardware (longer compile time, targets FPGA device):

     ```
     nmake fpga
     ```

> **Note**: The Intel&reg; PAC with Intel Arria&reg; 10 GX FPGA and Intel&reg; FPGA PAC D5005 (with Intel Stratix&reg; 10 SX) do not yet support Windows*. Compiling to FPGA hardware on Windows* requires a third-party or custom Board Support Package (BSP) with Windows* support.

> **Note**: If you encounter any issues with long paths when compiling under Windows*, you might have to create your `build` directory in a shorter path, for example `c:\samples\build`.  You can then run `cmake` from that directory, and provide `cmake` with the full path to your sample directory.

### In Third-Party Integrated Development Environments (IDEs)

You can compile and run this tutorial in the Eclipse*IDE (in Linux*) and the Visual Studio*IDE (in Windows*).
For instructions, refer to [FPGA Workflows on Third-Party IDEs for Intel&reg; oneAPI Toolkits](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-oneapi-dpcpp-fpga-workflow-on-ide.html).

## Examining the Reports

Locate the pair of `report.html` files in either:

- **Report-only compile**:  `multi_aocx_report.prj`
- **FPGA hardware compile**: `multi_aocx.prj`

Scroll down on the Summary page of the report and expand the section titled **Compile Estimated Kernel Resource Utilization Summary**. Observe how the kernel `ConstructFromACFixed` consumes fewer resources than the kernel named `ConstructFromFloat`. Similarly, observe how the kernel named `CalculateWithACFixed` consumes fewer FPGA resources than `CalculateWithFloat`.

## Running the Sample

1. Run the sample on the FPGA emulator (the kernel executes on the CPU):

   ```bash
   ./multi_aocx.fpga_emu    (Linux)
   multi_aocx.fpga_emu.exe  (Windows)
   ```

2. Run the sample of the FPGA simulator device

   ```bash
   ./multi_aocx.fpga_sim        (Linux)
   multi_aocx.fpga_sim.exe      (Windows)
   ```

3. Run the sample on the FPGA device

   ```bash
   ./multi_aocx.fpga             (Linux)
   multi_aocx.fpga.exe           (Windows)
   ```

### Example of Output on Emulator

```txt
1. Testing Constructing multi_aocx from float or multi_aocx:
Constructed from float:         3.6416015625
Constructed from multi_aocx:      3.6416015625

2. Testing calculation with float or multi_aocx math functions:
MAX DIFF (quantum) for multi_aocx<10, 3, true>:   0.0078125
MAX DIFF for float:                             9.53674e-07

Input 0:                        -0.80799192
result(fixed point):            1
difference(fixed point):        0
result(float):                  1
difference(float):              0

Input 1:                        -2.099829
result(fixed point):            0.9921875
difference(fixed point):        0.0078125
result(float):                  0.99999994
difference(float):              5.9604645e-08

Input 2:                        -0.74206626
result(fixed point):            1
difference(fixed point):        0
result(float):                  1
difference(float):              0

Input 3:                        -2.3321707
result(fixed point):            1
difference(fixed point):        0
result(float):                  1
difference(float):              0

Input 4:                        1.1432415
result(fixed point):            0.9921875
difference(fixed point):        0.0078125
result(float):                  0.99999994
difference(float):              5.9604645e-08

PASSED: all kernel results are correct.
```

### Discussion of Results

You can obtain a smaller hardware footprint for your kernel by ensuring that the `multi_aocx` numbers are constructed from `float` or `double` numbers outside the kernel. Additionally, by using the `multi_aocx` types and math library functions, you can use an even smaller fixed point format to trade-off even more accuracy for a more resource efficient design if your application requirements allow for it.

## License

Code samples are licensed under the MIT license. See [License.txt](https://github.com/oneapi-src/oneAPI-samples/blob/master/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](https://github.com/oneapi-src/oneAPI-samples/blob/master/third-party-programs.txt).
