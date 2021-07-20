#pragma once

#include <steam.hpp>

namespace steam {

//////////////////////////////////////////////////////////////////////////////////////////////
/// \brief The distance between two points living in their respective frame is
///        used as our error function.
//////////////////////////////////////////////////////////////////////////////////////////////
class PGRVertexPinPriorErrorEval : public ErrorEvaluator<2, 6>::type {
 public:
  /// Convenience typedefs
  using Ptr = boost::shared_ptr<PGRVertexPinPriorErrorEval>;
  using ConstPtr = boost::shared_ptr<const PGRVertexPinPriorErrorEval>;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Constructor
  /// \param T_rq      Transformation matrix from query to reference.
  /// \param reference A point in the 'reference' frame expressed in cartesian
  ///                  coordinates.
  /// \param query     A point in the 'query' frame expressed in cartesian
  ///                  coordinates.
  //////////////////////////////////////////////////////////////////////////////////////////////
  PGRVertexPinPriorErrorEval(const se3::TransformEvaluator::ConstPtr &T_rv,
                             const Eigen::Vector2d &meas);

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Returns whether or not an evaluator contains unlocked state
  ///        variables
  //////////////////////////////////////////////////////////////////////////////////////////////
  bool isActive() const override;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Evaluate the 2-d measurement error (x, y)
  //////////////////////////////////////////////////////////////////////////////////////////////
  Eigen::Matrix<double, 2, 1> evaluate() const override;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Evaluate the 2-d measurement error (x, y) and Jacobians
  //////////////////////////////////////////////////////////////////////////////////////////////
  virtual Eigen::Matrix<double, 2, 1> evaluate(
      const Eigen::Matrix<double, 2, 2> &lhs,
      std::vector<Jacobian<2, 6>> *jacs) const;

 private:
  se3::TransformEvaluator::ConstPtr T_rv_;

  Eigen::Matrix<double, 2, 4> D_ = Eigen::Matrix<double, 2, 4>::Zero();

  Eigen::Vector4d meas_;
  Eigen::Vector4d origin_;
};

}  // namespace steam
