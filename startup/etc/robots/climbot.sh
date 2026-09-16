#! /bin/nsh

# Climbot actuator allocation defaults.
param set-default CA_THR_F 1.0
param set-default CA_THR_R 1.0
param set-default CA_YAW_F 1.0
param set-default CA_YAW_R 0.0
param set-default CA_ARM_SW 1

# Drive motors use Damiao velocity mode.
param set-default CAN_M0_PROTO 1
param set-default CAN_M0_TYPE 2
param set-default CAN_M0_MODE 3
param set-default CAN_M1_PROTO 1
param set-default CAN_M1_TYPE 2
param set-default CAN_M1_MODE 3

# DM4310 steering motors use position-velocity mode.
param set-default CAN_S0_PROTO 1
param set-default CAN_S0_TYPE 1
param set-default CAN_S0_MODE 2
param set-default CAN_S0_PFBMAX 12.5
param set-default CAN_S1_PROTO 1
param set-default CAN_S1_TYPE 1
param set-default CAN_S1_MODE 2
param set-default CAN_S1_PFBMAX 12.5

control_allocator start -c climbot
can_output start -d /dev/can0
