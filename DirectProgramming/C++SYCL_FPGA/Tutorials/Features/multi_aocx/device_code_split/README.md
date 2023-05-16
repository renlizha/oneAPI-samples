# Using the flows test 'device_code_split'

This FPGA tutorial demonstrates the -fsycl-device-code-split=per_source|per_kernel

| Optimized for                     | Description
|:---                               |:---
| OS                                | Linux* Ubuntu* 18.04/20.04 <br> RHEL*/CentOS* 8 <br> SUSE* 15 <br> Windows* 10
| Hardware                          | Intel&reg; Programmable Acceleration Card (PAC) with Intel Arria&reg; 10 GX FPGA <br> Intel&reg; FPGA Programmable Acceleration Card (PAC) D5005 (with Intel Stratix&reg; 10 SX) <br> Intel&reg; FPGA 3rd party / custom platforms with oneAPI support <br> **Note**: Intel&reg; FPGA PAC hardware is only compatible with Ubuntu 18.04*
| Software                          | Intel® oneAPI DPC++/C++ Compiler
| What you will learn               | How to use the `-fsycl-device-code-split=per_source|per_kernel` to create multiple aocx.
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
The purpose of this tutorial is to describe the `-fsycl-device-code-split` flag and to show how it can be leveraged to break down the compilation of several kernels into smaller individual compiles.  

The tutorial will also describe the `-reuse-exe=<exe>` flag and show how it can enable rapid recompilation of host-only code changes.

### Using the flag

`-fsycl-device-code-split=[per_kernel|per_source]` specifies how the device code in the input set of source files will be structured for compilation.

- It splits a large design into smaller images:
  - `per_kernel` - a separate device code module is created for each SYCL kernel.
  - `per_source` - a separate device code module is created for each source.
- Users do not need to manually restructure their code or source files to get smaller hardware image files. This flag allows the compiler to do so automatically. 
- When using the `-fsycl-device-code-split` flag, all kernels will be recompiled if any code or the target is changed unless you use the `-reuse-exe` flag (described below) to specify to the compiler which hardware images can be reused.
- The total compile time will likely be longer, but the main perk is if your image is getting too large to fit on the FPGA, this can be a nice way to split them.

`-reuse-exe=<exe>` re-uses the device binary embedded within the previously compiled SYCL binary <exe>.

- It speeds up the FPGA compilation step if the device code corresponding to `<exe>` is unchanged.
- It can minimize or avoid long Quartus compile times for FPGA targets.
- This only works when the device code is unchanged.

using the flags together:
```
icpx ... simple.cpp -o simple.exe -fsycl-device-code-split=per_kernel -reuse-exe=simple.exe
```
If `simple.exe` does not exist, `-reuse-exe` is ignored and the FPGA device image is regenerated. This will always be the case the first time a project is compiled.

## Key Concepts
- Using the `-fsycl-device-code-split=[per_kernel|per_source]` flag allows the compiler to split a larger design into smaller hardware images automatically, saving users' time from manual restructure.
- If the device code and options affecting the device have not changed since the previous compilation, passing the `-reuse-exe=<exe_name>` flag instructs the compiler to extract the compiled FPGA binary from the existing executable and package it into the new executable, saving the device compilation time.

## Building the `device_code_split` Tutorial

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

3. (Optional) As the earlier hardware compile can take several hours to complete, FPGA precompiled binaries (compatible with Ubuntu 18.04) can be downloaded [here](https://iotdk.intel.com/fpga-precompiled-binaries/latest/device_code_split.fpga.tar.gz).

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

- **Report-only compile**:  `device_code_split_report.prj`
- **FPGA hardware compile**: `device_code_split.prj`

Scroll down on the Summary page of the report and expand the section titled 

## Running the Sample

1. Run the sample on the FPGA emulator (the kernel executes on the CPU):

   ```bash
   ./device_code_split.fpga_emu    (Linux)
   device_code_split.fpga_emu.exe  (Windows)
   ```

2. Run the sample of the FPGA simulator device

   ```bash
   ./device_code_split.fpga_sim        (Linux)
   device_code_split.fpga_sim.exe      (Windows)
   ```

3. Run the sample on the FPGA device

   ```bash
   ./device_code_split.fpga             (Linux)
   device_code_split.fpga.exe           (Windows)
   ```

### Example of Output on Emulator


### Discussion of Results


## License

Code samples are licensed under the MIT license. See [License.txt](https://github.com/oneapi-src/oneAPI-samples/blob/master/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](https://github.com/oneapi-src/oneAPI-samples/blob/master/third-party-programs.txt).
