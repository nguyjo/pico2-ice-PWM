# pico2-ice-Joystick

Firmware that implements a Joystick-like Laser Diode Motion Controller.

It needs a local [oss-cad-suite](https://github.com/YosysHQ/oss-cad-suite-build) toolchain installed,
or at least `yosys`, `nextpnr-ice40`, `icepack` and `python` in the execution path.

---

## The pico-ice-sdk and pico-sdk repositories are used as submodules in this project.
### pico-ice-sdk

[Doc](http://pico-ice.tinyvision.ai/)
| [Hardware](https://github.com/tinyvision-ai-inc/pico-ice)
| [SDK](https://github.com/tinyvision-ai-inc/pico-ice-sdk)
| [Schematic](https://raw.githubusercontent.com/tinyvision-ai-inc/pico-ice/main/Board/Rev3/pico-ice.pdf)
| [Assembly](https://htmlpreview.github.io/?https://github.com/tinyvision-ai-inc/pico-ice/blob/main/Board/Rev3/bom/ibom.html)
| [Discord](https://discord.gg/t2CzbAYeD2)

### pico-sdk
| [SDK](https://github.com/raspberrypi/pico-sdk)

---

## To quickly get started:
Provided you have the compilers setup.
For riscv, you may add `-DPICO_GCC_TRIPLE=riscv64-unknown-elf` to the cmake command if your 'riscv64' compiler supports rp2350/rv32.

- clone the repository
- run `git submodule update --init --recursive`
- `mkdir build && cd build`
- `cmake -DPICO_BOARD=pico_ice ..` or `cmake -DPICO_BOARD=pico2_ice ..` or `cmake -DPICO_BOARD=pico2_ice -DPICO_PLATFORM=rp2350-riscv ..
    - If you are using Ninja on Windows, try `cmake -G "Ninja" -DPICO_BOARD=pico2_ice ..`
- `make -j8`
- Copy the generated uf2 to the board after setting it to flashing mode

# Project Abstract: FPGA-based Laser Motion Controller

**Authors:** Andrei Vasilev & Joseph Nguyen (Walla Walla University, ENGR 433)

This project explores the design and implementation of a hybrid MCU-FPGA control system using the **pico2-ice** development board (RP2350 + iCE40UP5K). The system functions as a 2-degree-of-freedom laser turret that mimics the orientation of a handheld controller in real-time.

By offloading precise timing tasks to the FPGA, the system achieves jitter-free servo motion while allowing the microcontroller to focus on high-speed sensor fusion.

### System Architecture
The design utilizes a "Brain and Muscle" architecture:

*   **The Brain (RP2350):** Interfaces with an **LSM6DSOX IMU** via I2C. It reads raw accelerometer and gyroscope data, applies a complementary filter to calculate stable Roll and Pitch angles, and maps these physical angles to a normalized byte protocol (0–180°).
*   **The Protocol (SPI):** Data is transmitted via a custom SPI packet structure from the MCU (Master) to the FPGA (Slave), sending coordinate updates at 50Hz.
*   **The Muscle (iCE40 FPGA):** A custom Verilog design receives the SPI packets, performs hardware-level arithmetic to convert angles into clock ticks, and drives parallel 16-bit PWM generators. This ensures that servo jitter is eliminated, regardless of the CPU load on the RP2350.

### Key Features
*   **Sensor Fusion:** Implementation of a complementary filter to mitigate accelerometer noise and gyroscope drift.
*   **Hardware Acceleration:** FPGA-based PWM generation running at 24MHz effective logic speed.
*   **Custom Verilog SPI Slave:** A robust 16-bit SPI receiver with asynchronous resets and clock domain crossing logic.
*   **Safety Interlocks:** Hardware-level constraints in Verilog prevent the servos from being driven beyond their physical mechanical limits (0.6ms – 2.4ms pulse width).

### Hardware Bill of Materials
*   **Controller:** pico2-ice (RP2350 + iCE40UP5K)
*   **Sensors:** Adafruit LSM6DSOX + LIS3MDL Precision 9 DoF IMU
*   **Actuators:** 2x Standard Servo Motors (Pan/Tilt Mount)
*   **Output:** 5mW Red Laser Diode (Driven via NPN Transistor circuit)
