/**
 * Copyright (c) 2016  Zubax Robotics OU  <info@zubax.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *    following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "common.hpp"
#include "parameters.hpp"
#include "running_task.hpp"
#include "hw_test/report.hpp"
#include "motor_id/task.hpp"
#include <math/math.hpp>
#include <cstdint>
#include <utility>


namespace foc
{
/**
 * Must be invoked in the first order, exactly once.
 * This function may block for a few seconds.
 * All other API functions are non-blocking, except stated otherwise.
 */
void init(const Parameters& params);

/**
 * Allows to change configuration parameters at runtime.
 * The new parameters will take effect on next state switch (e.g. motor start/stop, identification, etc).
 */
void setParameters(const Parameters& params);
Parameters getParameters();

/**
 * Subset of @ref getParameters().
 */
MotorParameters getMotorParameters();

/**
 * See @ref beginHardwareTest().
 */
hw_test::Report getHardwareTestReport();

/**
 * Begins the asynchronous process of motor identification.
 * See @ref MotorIdentificationMode.
 * Completion of the process can be detected by means of monitoring the current state of the controller, see @ref State.
 * The identified parameters can be read via @ref getMotorParameters().
 */
void beginMotorIdentification(motor_id::Mode mode);

/**
 * Begins the asynchronous process of hardware testing.
 * Completion of the process can be detected by means of monitoring the current state of the controller, see @ref State.
 * The result of the test can be obtained via @ref getLastHardwareTestReport().
 * A side effect of the test is that the HW driver will be recalibrated.
 */
void beginHardwareTest();

/**
 * State of the control logic.
 * Some of the functions may be unavailable in certain states.
 * TODO: At the moment, the control logic does not use this state representation.
 *       There is a wrapper that converts real states into this representation.
 *       Probably this should be simplified.
 */
enum class State
{
    /**
     * The control logic is doing nothing and is ready to accept commands.
     * If the motor stalled or another error occurred, an error code will be set.
     */
    Idle,

    /**
     * Motor identification is in progress, commands cannot be accepted.
     * @ref beginMotorIdentification().
     */
    MotorIdentification,

    /**
     * Hardware testing is underway.
     * Possible outcomes:
     *  - Test passed                                   -> Idle
     *  - Test failed (hardware problems detected)      -> Fault
     * @ref beginHardwareTest().
     */
    HardwareTesting,

    /**
     * The motor is starting, or some pre-start procedures are underway. This is a transient state.
     * Possible outcomes:
     *  - Started successfully  -> Running
     *  - Failed to start       -> Idle or Fault
     */
    Spinup,

    /**
     * The motor is running. Next state is normally Idle.
     */
    Running,

    /**
     * The controller has encountered a serious error and will not start the motor until the error is reset.
     * In order to reset the error, call @ref stop(), or set a zero setpoint (which is equivalent to calling stop()).
     */
    Fault
};

inline const char* stateToString(const State s)
{
    switch (s)
    {
    case State::Idle:                return "Idle";
    case State::MotorIdentification: return "MotorID";
    case State::HardwareTesting:     return "HWTest";
    case State::Spinup:              return "Spinup";
    case State::Running:             return "Running";
    case State::Fault:               return "Fault";
    }

    return "BADSTATE";
}

/**
 * @ref State.
 */
State getState();

/**
 * Assigns new setpoint; the units depend on the selected control mode.
 * The value of zero stops the motor and clears the fault state, which is equivalent to calling @ref stop().
 * Negative values indicate reverse rotation.
 *
 * @param control_mode          See @ref ControlMode.
 * @param value                 Value depending on the control mode.
 * @param request_ttl           After this timeout (in seconds) the motor will be stopped automatically.
 */
void setSetpoint(ControlMode control_mode,
                 Const value,
                 Const request_ttl);

/**
 * Stops the motor normally if it is running.
 * Clears the fault state if the motor is not running.
 */
inline void stop()
{
    setSetpoint(ControlMode(0), 0.0F, 0.0F);
}

/**
 * Returns the instant motor current in Amperes.
 */
Scalar getInstantCurrentFiltered();

/**
 * Returns the instant relative power in percent of the maximum.
 */
Scalar getInstantDemandFactorFiltered();

/**
 * Returns the current mechanical angular velocity of the rotor in RPM.
 */
Scalar getInstantMechanicalRPM();

/**
 * Returns the number of all outstanding errors at the moment.
 * The exact semantics is yet to be defined; refer to the code to learn more.
 */
std::uint32_t getErrorCount();

/**
 * Generate sound using the motor windings.
 * The request MAY be ignored if the controller is in not in the Idle state.
 * Units are SI (Hertz, seconds).
 */
void beep(Const frequency,
          Const duration);

/**
 * Prints the current status information into stdout.
 * This command is mostly useful for debugging, diagnostics and tuning.
 * The function makes blocking calls to printf() and may moderately disturb IRQ processing due to critical sections.
 */
void printStatusInfo();

/**
 * This command is intended for use with CLI plotting tool.
 * Refer to the project tools directory for more info.
 */
void plotRealTimeValues();

/**
 * Named debug values.
 * This is suitable for e.g. reporting via UAVCAN.
 * Key length is guaranteed to never exceed 3 ASCII characters (3 bytes).
 */
using DebugKeyValueType = std::pair<os::heapless::String<3>, Scalar>;
constexpr unsigned NumDebugKeyValuePairs = 4;

std::array<DebugKeyValueType, NumDebugKeyValuePairs> getDebugKeyValuePairs();

}
