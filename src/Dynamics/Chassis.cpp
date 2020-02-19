//
// Created by qiayuan on 2/13/20.
//
#include "Dynamics/Chassis.h"
#include "Dynamics/spatial.h"
#include "Math/orientation_tools.h"

using namespace ori;
using namespace spatial;

/*!
 * Build a FloatingBaseModel of the chassis
 */
template<typename T>
FloatingBaseModel<T> Chassis<T>::buildModel() {
  FloatingBaseModel<T> model;

  // we assume the Chassis's body can be modeled as a uniformly distributed box.
  Vec3<T> bodyDims(_bodyLength, _bodyWidth, _bodyHeight);
  model.addBase(_bodyInertia);
  // add contact for the chassis's body
  model.addGroundContactBoxPoints(5, bodyDims);
  const int baseID = 5;
  int bodyID = baseID;

  T sideSign[4] = {-1, 1, 1, -1};

  Mat3<T> I3 = Mat3<T>::Identity();

  // loop over 4 legs
  for (int wheelID = 0; wheelID < 4; wheelID++) {

    // Suspe Joint
    bodyID++;
    Mat6<T> xtreeSuspe =
        createSXform(coordinateRotation<T>(CoordinateAxis::Z, T(M_PI)),
                     withLegSigns<T>(_suspeLocation, wheelID));
    Mat6<T> xtreeSuspeRotor =
        createSXform(coordinateRotation<T>(CoordinateAxis::Z, T(M_PI)),
                     withLegSigns<T>(_suspeRotorLocation, wheelID));
    if (sideSign[wheelID] < 0) {
      model.addBody(_suspeInertia.flipAlongAxis(CoordinateAxis::Y),
                    _suspeRotorInertia.flipAlongAxis(CoordinateAxis::Y),
                    6., baseID, JointType::Revolute,
                    CoordinateAxis::Y, xtreeSuspe, xtreeSuspeRotor);
    } else {
      model.addBody(_suspeInertia, _suspeRotorInertia, 6., baseID,
                    JointType::Revolute, CoordinateAxis::Y, xtreeSuspe,
                    xtreeSuspeRotor);
    }

    // Wheel Joint
    bodyID++;
    Mat6<T> xtreeKnee = createSXform(I3, withLegSigns<T>(_wheelLocation, wheelID));
    Mat6<T> xtreeKneeRotor = createSXform(I3, withLegSigns<T>(_wheelRotorLocation, wheelID));
    if (sideSign[wheelID] < 0) {
      model.addBody(_wheelInertia.flipAlongAxis(CoordinateAxis::Y),
                    _wheelRotorInertia.flipAlongAxis(CoordinateAxis::Y),
                    _wheelGearRatio, bodyID - 1, JointType::Revolute,
                    CoordinateAxis::Y, xtreeKnee, xtreeKneeRotor);
    } else {
      model.addBody(_wheelInertia, _wheelRotorInertia, _wheelGearRatio, bodyID - 1,
                    JointType::Revolute, CoordinateAxis::Y, xtreeKnee,
                    xtreeKneeRotor);
    }

    //add "foot" of wheel
    for (int i = 0; i < 16; ++i) {    //Mecanum wheel of robomaster has 16 rotor
      double angle = 2. * M_PI / 16. * i;
      model.addGroundContactPoint
          (bodyID,
           Vec3<T>(_wheelRadius * sin(angle), 0, _wheelRadius * cos(angle)),
           true);
      //Add two more contact point for higher accuracy
      model.addGroundContactPoint
          (bodyID,
           Vec3<T>(_wheelRadius * sin(angle + M_PI / 0.0111), 0, _wheelRadius * cos(angle + M_PI / 0.0111)),
           true);
      model.addGroundContactPoint
          (bodyID,
           Vec3<T>(_wheelRadius * sin(angle - M_PI / 0.0111), 0, _wheelRadius * cos(angle - M_PI / 0.0111)),
           true);
    }
  }

  Vec3<T> g(0, 0, -9.81);
  model.setGravity(g);

  return model;
}

/*!
 * Flip signs of elements of a vector V depending on which leg it belongs to
 */
template<typename T, typename T2>
Vec3<T> withLegSigns(const Eigen::MatrixBase<T2> &v, int wheelID) {
  static_assert(T2::ColsAtCompileTime == 1 && T2::RowsAtCompileTime == 3,
                "Must have 3x1 matrix");
  switch (wheelID) {
    case 0:return Vec3<T>(v[0], -v[1], v[2]);
    case 1:return Vec3<T>(v[0], v[1], v[2]);
    case 2:return Vec3<T>(-v[0], v[1], v[2]);
    case 3:return Vec3<T>(-v[0], -v[1], v[2]);
    default:throw std::runtime_error("Invalid wheel id!");
  }
}

template<typename T>
std::vector<ActuatorModel<T>> Chassis<T>::buildActuatorModels() {
  std::vector<ActuatorModel<T>> models;
  models.emplace_back(_wheelGearRatio, _motorKT, _motorR, _batteryV,
                      _jointDamping, _jointDryFriction, _motorTauMax);

  return models;
}

template
class Chassis<double>;
template
class Chassis<float>;
