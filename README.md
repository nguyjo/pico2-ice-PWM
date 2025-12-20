# pico2-ice-Joystick

Firmware that computes PWM levels in the RP2350 and outputs PWM signals from the ice40 FPGA.

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
