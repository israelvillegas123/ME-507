# Modeling and Control {#modeling}

## Overview

This page describes the sensing, control logic, and motion control techniques used by the Desk Buddy.

The robot uses VL53L0X Time of Flightsensors to detect nearby objects and desk edges. The STM32 microcontroller processes this information and determines the robot's behavior.

The main goals of the control system are:

* Detect the edge of a desk before falling
* React to user interaction
* Control the wheel motors for movement
* Animate the display and llama ears to express emotion

---

## Distance Measurement

The Desk Buddy uses three VL53L0X Time-of-Flight sensors.

These sensors measure the distance between the robot and nearby objects by timing how long it takes for a laser pulse to reflect back to the sensor.

Distance measurements are reported in millimeters and are continuously updated.

---

## Pet Detection

One of the ToF sensors is used to detect when an hand is close to the robot.

When the measured distance falls below a threshold, the robot enters Pet mode.

In pet mode, the robot displays happy eyes with animated hearts and wiggles its ears.

---

## Edge Detection

Preventing the robot from falling off a desk is the highest priority control task.

When the robot is safely on the desk, the sensors measure a small distance to the desk surface. As the robot approaches the edge, the measured distance increases because the sensor no longer sees the desk under it.

The software uses a simple threshold to detect this condition.

When an edge is detected, the robot enters Falling mode and turns away from the edge.

---

## Encoder Feedback

Each drive motor contains a quadrature encoder that measures wheel rotation.

The encoder generates pulses as the wheel rotates. These pulses are counted by the STM32 using external interrupts.

Encoder feedback gives information about wheel movement and is used for closed loop speed control.

Each encoder has a 12 count per revolution.

The gearmotor has a gear ratio of 150.58:1, meaning the encoder measures motor shaft rotation before the gearbox.

The effective encoder resolution at the wheel is:

Encoder Counts per Wheel Revolution

= 12 × 150.58

= 1807 counts/revolution

This high encoder resolution allows the STM32 to accurately estimate wheel speed and wheel position.


---

## Closed-Loop Motor Control

To improve driving, encoder data can be compared to a desired wheel speed.

The difference between the desired speed and measured speed is the error.

A proportional integral controller can then change the motor PWM duty cycle to reduce this error.

This allows the robot to keep a more consistent speed and compensate for differences between the left and right motors.


