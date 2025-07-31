### Automation Lab, Sungkyunkwan University

# AutoSDV: ZCU (Zone Contrl Unit)

This is the repository of ZCU part of AutoSDV.

"AutoSDV: An Open 1/5-Scale Software-Defined Vehicle Platform for Experimentation and Research"

## 1. Overview
- camera_module: Receives camera frame data from the ECU and publishes it to the HPC
- control_module:Subscribes to control signals from the HPC and converts them to CAN format, sending them to the CAN bus.
- status_module: Receives actuator status values from the ECU via CAN and publishes them.

## 2. Setup

### 2.1 Hardware Requirements
- **ZCU Board**: [S32G3-VNP-RDB3](https://www.nxp.com/design/design-center/development-boards-and-designs/S32G-VNP-RDB3).


### 2.3 Software Requirements
- **DDS**: [FastDDS](https://github.com/eProsima/Fast-DDS).


### 2.4 Installationn
TBD
