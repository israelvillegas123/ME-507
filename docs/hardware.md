# Hardware {#hardware}

## Hardware Overview

The Desk Buddy is built around an STM32F411 microcontroller and a custom PCB that connects all of the sensors, motors, and user interface components. The robot uses sensors to detect the edge of a desk, while motors and servos allow it to move and interact with the user.

---

## Main Controller

### STM32F411CEU6

The STM32F411CEU6 is the main microcontroller used in the project. It acts as the brain of the robot and is responsible for reading sensor data, controlling the motors and servos, updating the display, and making decisions about how the robot should behave.

---

## Sensors

### Time of Flight Sensors

Three VL53L0X Time of Flight distance sensors are mounted on the front of the robot. These sensors measure the distance to nearby objects and help the robot from falling off the edge. The VL53L0X sensors communicate with the STM32 microcontroller using the I2C interface and provide accurate distance measurements in a compact package.

---

## Actuators

### DC Motors with Encoders

The robot moves using two DC gear motors. Each motor includes an encoder that measures wheel rotation. The encoder feedback allows the software to monitor wheel speed and improve motor control.

### Servo Motors

Two servo motors are used to move the robot's arms. These arms are used to give the robot personality and make it feel more interactive.

---

## User Interface

### OLED Display

A SSD1306 OLED display is mounted on the front of the robot and is used to show facial expressions and animations. The display communicates with the STM32 microcontroller over an I2C interface and serves as the primary visual interface for the robot.

---

## Bill of Materials

| Component                  | Qty   | Purpose                                    |
| -------------------------- | ----- | ------------------------------------------ |
| STM32F411CEU6              | 1     | Main microcontroller                       |
| DC Gear Motor with Encoder | 2     | Drive the robot and provide speed feedback |
| DRV8833 Motor Driver       | 1     | Control the DC motors                      |
| Servo Motor                | 2     | Move the robot arms                        |
| VL53L0X ToF Sensor         | 3     | Distance measurement and edge detection    |
| SSD1306 I2C OLED Display   | 1     | Display faces and status information       |
| 7.4 V LiPo Battery         | 1     | Power source                               |

