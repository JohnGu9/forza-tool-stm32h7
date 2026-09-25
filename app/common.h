#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct {
  void *context;
  void (*callback)(void *context);
} EventTask_t;

void invoke_tasks(EventTask_t *list[], size_t length);

int32_t add_task(EventTask_t *task, EventTask_t *list[], size_t length);

int32_t remove_task(EventTask_t *task, EventTask_t *list[], size_t length);

#define STRUCT_PACK __attribute__((__packed__))

typedef struct {
  int32_t IsRaceOn;
  uint32_t TimestampMS;

  // Engine RPM values
  float EngineMaxRpm;
  float EngineIdleRpm;
  float CurrentEngineRpm;

  // In the car's local space; X = right, Y = up, Z = forward
  float AccelerationX;
  float AccelerationY;
  float AccelerationZ;

  // In the car's local space; X = right, Y = up, Z = forward
  float VelocityX;
  float VelocityY;
  float VelocityZ;

  // Angular velocity in the car's local space (rad/s); X = pitch, Y = yaw, Z =
  // roll
  float AngularVelocityX;
  float AngularVelocityY;
  float AngularVelocityZ;

  // Car orientation (radians)
  float Yaw;
  float Pitch;
  float Roll;

  // Suspension travel normalized: 0.0f = max stretch; 1.0 = max compression
  float NormalizedSuspensionTravelFrontLeft;
  float NormalizedSuspensionTravelFrontRight;
  float NormalizedSuspensionTravelRearLeft;
  float NormalizedSuspensionTravelRearRight;

  // Tire normalized slip ratio, = 0 means 100% grip and |ratio| > 1.0 means
  // loss of grip.
  float TireSlipRatioFrontLeft;
  float TireSlipRatioFrontRight;
  float TireSlipRatioRearLeft;
  float TireSlipRatioRearRight;

  // Wheel rotation speed radians/sec.
  float WheelRotationSpeedFrontLeft;
  float WheelRotationSpeedFrontRight;
  float WheelRotationSpeedRearLeft;
  float WheelRotationSpeedRearRight;

  // = 1 when wheel is on rumble strip, = 0 when off.
  int32_t WheelOnRumbleStripFrontLeft;
  int32_t WheelOnRumbleStripFrontRight;
  int32_t WheelOnRumbleStripRearLeft;
  int32_t WheelOnRumbleStripRearRight;

  // = 1 when wheel is in a puddle, = 0 when not.
  int32_t WheelInPuddleFrontLeft;
  int32_t WheelInPuddleFrontRight;
  int32_t WheelInPuddleRearLeft;
  int32_t WheelInPuddleRearRight;

  // Non-dimensional surface rumble values passed to controller force feedback
  float SurfaceRumbleFrontLeft;
  float SurfaceRumbleFrontRight;
  float SurfaceRumbleRearLeft;
  float SurfaceRumbleRearRight;

  // Tire normalized slip angle, = 0 means 100% grip and |angle| > 1.0 means
  // loss of grip.
  float TireSlipAngleFrontLeft;
  float TireSlipAngleFrontRight;
  float TireSlipAngleRearLeft;
  float TireSlipAngleRearRight;

  // Tire normalized combined slip, = 0 means 100% grip and |slip| > 1.0 means
  // loss of grip.
  float TireCombinedSlipFrontLeft;
  float TireCombinedSlipFrontRight;
  float TireCombinedSlipRearLeft;
  float TireCombinedSlipRearRight;

  // Actual suspension travel in meters
  float SuspensionTravelMetersFrontLeft;
  float SuspensionTravelMetersFrontRight;
  float SuspensionTravelMetersRearLeft;
  float SuspensionTravelMetersRearRight;

  // Unique ID of the car make/model
  int32_t CarOrdinal;
  // Between 0 (D -- worst cars) and 7 (X class -- best cars) inclusive
  int32_t CarClass;
  // Between 100 (worst car) and 999 (best car) inclusive
  int32_t CarPerformanceIndex;
  // 0 = FWD, 1 = RWD, 2 = AWD
  int32_t DrivetrainType;
  // Number of cylinders in the engine
  int32_t NumCylinders;
} STRUCT_PACK SledData_t;

typedef struct {
  // Position in world space (meters)
  float PositionX;
  float PositionY;
  float PositionZ;

  // Speed in meters per second
  float Speed;
  // Power in watts
  float Power;
  // Torque in newton-meters
  float Torque;

  // Tire temperature
  float TireTempFrontLeft;
  float TireTempFrontRight;
  float TireTempRearLeft;
  float TireTempRearRight;

  // Turbo/supercharger boost (PSI above atmospheric)
  float Boost;
  // Fuel level (0.0 = empty, 1.0 = full)
  float Fuel;
  // Total distance traveled (meters)
  float DistanceTraveled;

  // Lap times (seconds); 0.0 if not applicable
  float BestLap;
  float LastLap;
  float CurrentLap;
  // Total race time (seconds since driving started)
  float CurrentRaceTime;

  // Number of laps completed
  uint16_t LapNumber;
  // Current race position
  uint8_t RacePosition;

  // Player inputs (0 to 255)
  uint8_t Accel;
  uint8_t Brake;
  uint8_t Clutch;
  uint8_t HandBrake;

  // Current gear
  uint8_t Gear;
  // Steering input (-127 = full left, 0 = center, 127 = full right)
  uint8_t Steer;
  // Normalized driving line position (-127 to 127)
  uint8_t NormalizedDrivingLine;
  // Normalized AI braking difference (-127 to 127)
  uint8_t NormalizedAIBrakeDifference;
} STRUCT_PACK DashData_t;

typedef struct {
  SledData_t sled;

  // Car group identifier
  uint32_t CarGroup;
  // Velocity loss from smashable object collision (m/s)
  float SmashableVelDiff;
  // Mass of recently hit smashable object (kg)
  float SmashableMass;

  DashData_t dash;
} STRUCT_PACK Fh4Data_t;

typedef struct {
  SledData_t sled;

  DashData_t dash;
} STRUCT_PACK Fm7Data_t;

typedef struct {
  SledData_t sled;

  DashData_t dash;

  float TireWearFrontLeft;
  float TireWearFrontRight;
  float TireWearRearLeft;
  float TireWearRearRight;

  // ID for track
  int32_t TrackOrdinal;
} STRUCT_PACK Fm8Data_t;

const SledData_t *get_sled_data(const uint8_t *buffer, const size_t length);
const DashData_t *get_dash_data(const uint8_t *buffer, const size_t length);

#ifdef __cplusplus
}
#endif

#endif
