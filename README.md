# 🎵 Frequency Analysis from Captured Audio

> Audio capture and real-time frequency analysis on an ARM Cortex-M microcontroller using the WM8904 codec, I2C/I2S interfaces, and the ARM CMSIS-DSP FFT library.

[![Status](https://img.shields.io/badge/status-active-brightgreen)](https://github.com/OmarAnguiano26/Embebidos_06FFT)
[![Branch](https://img.shields.io/badge/branch-fft-purple)](https://github.com/OmarAnguiano26/Embebidos_06FFT/tree/fft)
[![Platform](https://img.shields.io/badge/platform-SAMV71-orange)](https://github.com/OmarAnguiano26/Embebidos_06FFT)
[![Language](https://img.shields.io/badge/language-C-lightgrey)](https://github.com/OmarAnguiano26/Embebidos_06FFT)
[![DSP](https://img.shields.io/badge/lib-CMSIS--DSP-blue)](https://github.com/ARM-software/CMSIS-DSP)

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Hardware & Interfaces](#-hardware--interfaces)
- [System Architecture](#-system-architecture)
- [Codec Configuration](#-codec-configuration-wm8904)
- [I2S Interface](#-i2s-interface)
- [FFT Implementation](#-fft-implementation)
- [Results](#-results)
- [Conclusions](#-conclusions)
- [Contribute](#-want-to-contribute)
- [Author](#-author)

---

## 📖 Overview

This project captures audio from a **3.5mm jack input** connected to the SAMV71 board via the **WM8904 audio codec**. The captured data is stored in a buffer and processed using the **Fast Fourier Transform (FFT)** to analyze the frequency content of the signal.

FFT is one of the most fundamental algorithms in Digital Signal Processing (DSP). It converts a time-domain signal into the frequency domain, revealing patterns useful in voice recognition, signal detection, and machine learning applications.

> **Course:** Operating Systems Design for Embedded Environments — ITESO A.C., Universidad Jesuita de Guadalajara

---

## ✅ Features

- WM8904 codec configuration via I2C (TWI)
- Audio capture at **8 kHz** sample rate via I2S (SSC peripheral)
- **2048-sample** circular buffer for FFT input
- FFT computation using **ARM CMSIS-DSP** library (1024 output bins)
- Frequency resolution of **~3.9 Hz/bin**
- Tested with three signal types: 1 kHz tone, 2 kHz tone, and human voice
- Results exported via IDE watch and visualized in Excel

---

## 🔧 Hardware & Interfaces

| Component | Description |
|---|---|
| **MCU** | ATSAMV71Q21 (ARM Cortex-M7) |
| **Audio Codec** | Wolfson WM8904 |
| **Audio Input** | 3.5mm jack |
| **Config Interface** | I2C (TWI) — codec register configuration |
| **Data Interface** | I2S (SSC peripheral) — audio data streaming |
| **DSP Library** | ARM CMSIS-DSP (FFT) |
| **Clock** | MCLK at 12.288 MHz supplied to codec |

---

## 🏗 System Architecture

```
[3.5mm Audio Input]
        │
        ▼
  [WM8904 Codec]  ◄──── I2C (TWI): Register Config
        │
        │ I2S (SSC)
        ▼
  [SAMV71 Buffer]  (2048 samples @ 8kHz)
        │
        ▼
  [CMSIS-DSP FFT]  →  1024 frequency bins
        │
        ▼
  [Frequency Analysis]  →  Excel / Serial Console
```

---

## ⚙️ Codec Configuration (WM8904)

The WM8904 is configured via I2C at device address `0x34`. The initialization sequence is:

1. Start MCLK (12.288 MHz) to clock the codec
2. Initialize the I2C (TWI) peripheral
3. Reset codec — write `0x0` to register `0x00`
4. Enable system clock — write `0x7` to R22 (`0x16`)
5. Run the default start sequence:
   - Write `0x100` to Write Sequencer 0 (`0x6C`)
   - Write `0x100` to Write Sequencer 3 (`0x6F`)
   - Wait for completion by reading Write Sequencer 4 (`0x70`)
6. Configure clock rates — write `0xA45F` to Clock Rates 0 (`0x14`)
7. Enable input PGAs — write `0x3` to Power Management 0 (`0x0C`)
8. Enable headphone PGAs — write `0x3` to Power Management 0 (`0x0E`)
9. Enable DAC/ADC — write `0xF` to Power Management 6 (`0x12`)

### I2C Write Operation

```
[START] → [Device Addr 0x34 + W] → [ACK] → [Reg Addr] → [ACK] → [Data Hi] → [Data Lo] → [STOP]
```

### I2C Read Operation

```
[START] → [0x34 + W] → [ACK] → [Reg Addr] → [REPEATED START] → [0x34 + R] → [Data Hi] → [Data Lo] → [STOP]
```

---

## 🔊 I2S Interface

The SSC peripheral is configured as I2S master to stream audio samples from the codec to an internal buffer:

```c
void I2S_Init(void)
{
    PMC_EnablePeripheral(ID_SSC);
    PIO_Configure(SSC_Pins, 6);
    SSC_Configure(SSC, I2S_BITRATE, MASTERCLOCK);
    SSC_ConfigureTransmitter(SSC, SSC_TCMR_CONFIG, SSC_TFMR_CONFIG);
    SSC_ConfigureReceiver(SSC, SSC_RCMR_CONFIG, SSC_RFMR_CONFIG);
    PMC_ConfigurePCK2(1, 0);
    SSC_EnableTransmitter(SSC);
    SSC_EnableReceiver(SSC);
}
```

Data is read from the **RHR register** into the buffer until it is full. The buffer size is set to **2048 samples** — a power-of-2 multiple required by the FFT algorithm.

---

## 📐 FFT Implementation

### Sample Parameters

| Parameter | Value |
|---|---|
| Sample rate | 8,000 Hz |
| FFT size | 2048 samples |
| FFT output bins | 1024 |
| Frequency resolution (Δf) | ~3.906 Hz/bin |

### Frequency Resolution Formula

$$\Delta f = \frac{1}{N \times T_s} = \frac{f_s}{N}$$

For this project:

$$\Delta f = \frac{1}{2048 \times \frac{1}{8000}} = 3.9062 \text{ Hz/bin}$$

Each index in the FFT output corresponds to a frequency step of **3.9062 Hz**, allowing interpretation of the X-axis in the resulting graphs.

### Tested Signals

| Signal | Expected Peak | Frequency Range |
|---|---|---|
| 1 kHz tone | ~1,000 Hz | Narrowband |
| 2 kHz tone | ~2,000 Hz | Narrowband |
| Human voice (male) | 100–900 Hz (fundamental) | 100 Hz – 8 kHz (with harmonics) |

---

## 📊 Results

All three tested signals produced FFT outputs consistent with expectations:

- The **1 kHz tone** showed a sharp, isolated peak near 1,000 Hz.
- The **2 kHz tone** showed a sharp peak near 2,000 Hz.
- The **human voice** showed energy in the fundamental range (100–900 Hz) with harmonic content extending up to ~3.9 kHz — validating the 8 kHz sample rate as sufficient for voice analysis.

Data was exported via the **IDE watch window** (since `stdio` file I/O was unavailable on the target) and plotted in **Excel** using the delta formula to label the frequency axis.

---

## 🔍 Conclusions

This project required integrating multiple hardware modules simultaneously — I2C, I2S, DMA-style buffering, and DSP computation — making it one of the most complex in the specialization.

Key takeaways:

- Deep codec documentation reading is essential; the WM8904 startup sequence must be followed precisely.
- The CMSIS-DSP library greatly simplifies FFT implementation on Cortex-M processors.
- An 8 kHz sample rate is sufficient for male voice frequency analysis.
- The project laid the groundwork for a future real-time FFT implementation, which the SAMV71 hardware is capable of supporting.
- The knowledge gained around DSP libs and audio pipelines is directly applicable to grade-level projects focused on audio signal processing.

---

## 🚀 Want to Contribute?

This project is a solid foundation for real-time audio DSP on embedded systems. If you want to take it further:

**Fork it → Improve it → Share it**

### Ideas for contributions

- ⚡ Implement real-time FFT using DMA and ping-pong buffers
- 🎛 Add configurable sample rate and FFT resolution via CLI
- 📡 Stream FFT results over UART in a plottable format (e.g. CSV)
- 🔌 Port to another codec or MCU family (STM32, NXP i.MX RT)
- 🧪 Add automated test signals using the DAC output for loopback testing

[![Fork this repo](https://img.shields.io/badge/Fork%20this%20repo-%F0%9F%8D%B4%20Contribute-blue?style=for-the-badge)](https://github.com/OmarAnguiano26/Embebidos_06FFT/fork)

---

## 👤 Author

**Omar Alejandro Anguiano Najar**
🔗 [github.com/OmarAnguiano26](https://github.com/OmarAnguiano26)

**Institution:** ITESO A.C., Universidad Jesuita de Guadalajara
**Program:** Embedded Systems Specialization — Operating Systems Design for Embedded Environments
**Date:** December 3, 2024
---

## 📄 License

This project is for academic purposes. 
