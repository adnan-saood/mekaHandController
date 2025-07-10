| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- |

# Meka Robot Hand Controller (ESP32S3 Version)


## Project Description – Humanoid Hand Control System

We are designing a real-time, embedded control system for a **humanoid robotic hand**. The system will consist of five **Series Elastic Actuators (SEAs)**, each capable of closed-loop control based on high-frequency sensor feedback. The project will be implemented using an **object-oriented structure** on top of **FreeRTOS**, enabling modularity, deterministic behavior, and high performance. Communication with a host PC will be performed through a **USB HID interface**.

---

### System Architecture Overview

#### Actuator Configuration

* **SEATypeA** (4 actuators)

  * **Motor drive**: Brushed DC motor with H-bridge control (PWM and Direction)
  * **Sensors**:

    * `MA3Encoder`: PWM-based absolute shaft encoder
    * `LoadPotentiometer`: Analog sensor (spring deflection) read via external SPI ADC

* **SEATypeB** (1 actuator)

  * **Motor drive**: Brushed DC motor with separate PWM and Direction signals
  * **Sensors**:

    * `MA3Encoder`
    * `LoadPotentiometer` (via SPI ADC)

Each actuator will be represented by a class that encapsulates sensing, filtering, actuation, and safety.

---

### Control Strategies

Two control modes will be supported, selectable per actuator:

1. **Feed-Forward PID Position Control (`PIDFFController`)**

   * Classic PID with velocity/acceleration feedforward
   * Configurable stiffness by tuning gains dynamically
   * Used for precise position control and compliant behavior

2. **Force Impedance Control (`ImpedanceController`)**

   * Uses load-side deflection to estimate force
   * Applies a dynamic spring-damper model:

     $$
     \tau = K(x_d - x) + D(\dot{x}_d - \dot{x})
     $$
   * Suitable for interaction tasks and adaptive compliance

Each controller will be defined as a pluggable module, usable through a common interface.

---

### Peripherals and Interfaces

#### Communication

* **`USBCom`**: USB HID interface

  * Bi-directional command and telemetry exchange
  * No drivers required on host side (cross-platform)
  * Polling rate target: 1 kHz (Full-Speed USB)

#### Sensor Inputs

* **SPI**: External ADC for analog potentiometers (one per actuator)
* **Input Capture Timers**: For decoding PWM from `MA3Encoder`
* **I2C**: IMU sensor for global hand orientation or motion feedback

#### Actuator Outputs

* **PWM Timers**: Motor control (H-Bridge or PWM/Dir)
* **GPIO**: Direction pins, enable lines, status LEDs

#### Additional Recommended Peripherals

* DMA for SPI and USB buffers
* Flash or EEPROM for persistent configuration (e.g., calibration, gain values)
* Watchdog timer for safety and recovery
* System heartbeat indicators (LED/GPIO or software counter)

---

### FreeRTOS Task Architecture

| Task Name     | Frequency    | Purpose                                                    |
| ------------- | ------------ | ---------------------------------------------------------- |
| `ControlTask` | 1 kHz        | Runs selected controller for each SEA (PIDFF or Impedance) |
| `SensorTask`  | 1 kHz        | Reads sensors, updates actuator state                      |
| `USBComTask`  | \~1 kHz      | Communicates with host PC (commands, telemetry, logs)      |
| `LoggingTask` | Configurable | Manages periodic logging of sensor and actuator data       |
| `SafetyTask`  | 100–500 Hz   | Monitors for faults (timeouts, range errors, watchdogs)    |

All actuator control and communication will be deterministic and coordinated through these tasks. Task prioritization and execution timing will be tuned to guarantee responsiveness and reliability.

---

### Software Structure and Modularity

* **`SEA` class**: Represents each actuator (type A or B)
* **`Encoder`, `Potentiometer`, `IMU`, `MotorDriver` classes**: Sensor and actuator abstractions
* **`PIDFFController`, `ImpedanceController`**: Interchangeable control modules
* **`USBCom`**: Interface for real-time command/telemetry over HID
* **`Logger`**: For system state, debugging, and post-analysis
* **`ConfigurationManager`**: Handles runtime and persistent settings

All components will follow SOLID design principles, with clear separation between hardware abstraction, control logic, and communication layers.


# N2 Table 


## Component Descriptions

### **Actuators**

Each actuator corresponds to a joint in the humanoid hand, driven by a DC motor through a Series Elastic Actuator (SEA) structure.

* **ThumbAbd** – Thumb abduction/adduction joint
* **ThumbFlex** – Thumb flexion/extension joint
* **IndexFlex** – Index finger flexion/extension joint
* **MiddleFlex** – Middle finger flexion/extension joint
* **PinkyFlex** – Pinky finger flexion/extension joint

These actuators each include:

* A DC motor with PWM or PWM+DIR interface
* An MA3 absolute encoder (PWM output) on the motor shaft
* A load-side potentiometer read via external SPI ADC

---

### **Drivers & Hardware Interfaces**

* **PWMDrv** – Generates PWM signals to drive motor H-bridges (and DIR for special actuators).
* **SPIDrv** – Manages communication with external ADC via SPI to acquire analog signals.
* **ADCDrv** – Abstract interface for retrieving values from the external SPI ADC (e.g., load potentiometers).
* **ICDrv** – Input capture peripheral interface to read PWM signals from MA3 encoders.
* **I2CDrv** – Communicates with I2C devices, specifically the IMU.
* **USBCom** – USB HID communication with host PC. Used for:

  * Streaming sensor data
  * Receiving commands
  * Sending logs and diagnostics
  * Parameter/configuration updates

---

### **Sensors**

* **MA3\_**\* – Shaft encoder for each actuator, provides high-resolution position feedback via PWM.
* **Pot\_**\* – Analog potentiometer per actuator for measuring elastic load displacement (via SPI ADC).
* **IMU** – Inertial Measurement Unit connected over I2C. Used for global hand orientation, inertial compensation, or movement context.

---

### **FreeRTOS Tasks**

* **CtrlTask** – Main control loop. Implements:

  * Feedforward PID (PIDFF) with stiffness control
  * Force impedance control
  * Generates motor commands
* **SensorTask** – Handles all sensor data acquisition:

  * Reads MA3 via IC
  * Acquires potentiometer data via SPI ADC
  * Requests IMU readings via I2C
* **USBTask** – Manages USB HID communication

  * Handles inbound commands
  * Sends real-time data streams or debug info
* **LogTask** – Gathers system logs, task-level diagnostics, optionally transmits them over USB
* **SafetyTask** – Monitors system health, sensor sanity checks, watchdog timeouts, and manages fault states

---

### **Utilities**

* **Logger** – Provides structured logging capability to memory buffers or USB output, with support for timestamps and tags.
* **ConfigMgr** – Configuration manager:

  * Receives new parameters via `USBCom`
  * Validates and applies control gains, thresholds, limits
  * Optionally persists settings in non-volatile memory

| From \ To   | CtrlTask               | SensorTask                 | USBTask                     | LogTask                  | SafetyTask              | PWMDrv             | SPIDrv                 | ICDrv                 | I2CDrv               | USBCom                 | ConfigMgr                  |
|-------------|------------------------|----------------------------|-----------------------------|--------------------------|-------------------------|--------------------|------------------------|------------------------|----------------------|--------------------------|-----------------------------|
| CtrlTask    | (self)                 | receives sensor data       |                             | sends control logs       | receives safety status  | sends motor cmds   | reads load ADC         | reads encoder pulses   | reads IMU data       |                          | receives config commands    |
| SensorTask  | sends sensor data      | (self)                     | sends data to USBTask       | sends sensor logs        | sends safety signals    |                    | reads sensor ADCs      | captures MA3 encoder   | reads IMU            |                          |                             |
| USBTask     |                        |                            | (self)                      | forwards host logs       |                         |                    |                        |                        |                      | communicates with host   | receives and sends configs  |
| LogTask     | logs control outputs   | logs sensor readings       | logs USB events             | (self)                   | logs safety alerts      |                    |                        |                        |                      |                          |                             |
| SafetyTask  | sends stop signals     | monitors sensor limits     | sends emergency msg         | logs safety data         | (self)                  |                    |                        |                        |                      |                          |                             |
| PWMDrv      | drives motors          |                            |                             |                          |                         | (self)             |                        |                        |                      |                          |                             |
| SPIDrv      | reads load ADC         | reads pressure ADCs        |                             |                          |                         |                    | (self)                |                        |                      |                          |                             |
| ICDrv       | reads encoder pulses   | sends encoder positions    |                             |                          |                         |                    |                        | (self)                |                      |                          |                             |
| I2CDrv      | reads IMU              |                             |                             |                          |                         |                    |                        |                        | (self)               |                          |                             |
| USBCom      |                        |                            |                             |                          |                         |                    |                        |                        |                      | (self)                  | sends commands to config     |
| ConfigMgr   | sends params to Ctrl   |                            |                             |                          |                         |                    |                        |                        |                      |                          | (self)                      |
