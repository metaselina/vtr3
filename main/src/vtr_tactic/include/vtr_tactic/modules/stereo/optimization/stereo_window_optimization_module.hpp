#pragma once

// LGMath and Steam
#include <lgmath.hpp>
#include <steam.hpp>

#include <cpo_interfaces/msg/tdcp.hpp>

#include <vtr_tactic/modules/base_module.hpp>
#include <vtr_tactic/modules/stereo/optimization/steam_module.hpp>

using TdcpMsg = cpo_interfaces::msg::TDCP;

namespace vtr {
namespace tactic {
namespace stereo {

/** \brief A module that runs STEAM on multiple graph vertices. */
class StereoWindowOptimizationModule : public SteamModule {
 public:
  /** \brief Static module identifier. */
  static constexpr auto static_name = "stereo_window_optimization";

  /** \brief Collection of config parameters */
  struct Config : SteamModule::Config {
    bool depth_prior_enable;
    double depth_prior_weight;
    int min_tdcp_terms;
    double tdcp_cov;
    double stereo_cov_multiplier;
  };

  StereoWindowOptimizationModule(const std::string &name = static_name)
      : SteamModule(name) {}

  void configFromROS(const rclcpp::Node::SharedPtr &node,
                     const std::string param_prefix) override;

 protected:
  /** \brief Update the graph with optimized transforms */
  virtual void updateGraphImpl(QueryCache &, MapCache &mdata,
                               const Graph::Ptr &graph, VertexId);

  /** \brief Given two frames, builds a sensor specific optimization problem. */
  virtual std::shared_ptr<steam::OptimizationProblem>
  generateOptimizationProblem(QueryCache &qdata, MapCache &mdata,
                              const std::shared_ptr<const Graph> &graph);

  virtual void updateCaches(QueryCache &qdata, MapCache &);

 private:
#if false
  /**
   * \brief samples and saves the optimized trajectory and stores it in the
   * latest vertex.
   */
  void saveTrajectory(QueryCache &qdata, MapCache &mdata,
                      const std::shared_ptr<Graph> &graph);
#endif
  /**
   * \brief Initializes the problem based on an initial condition.
   * The initial guess at the transformation between the query frame and the map
   * frame.
   */
  void resetProblem();

  /**
   * \brief Adds a depth cost associated with this landmark to the depth cost
   * terms.
   * \param landmark The landmark in question.
   */
  void addDepthCost(steam::se3::LandmarkStateVar::Ptr landmark);

  /**
   * \brief Adds a TDCP cost associated with this carrier phase measurement to
   * the TDCP cost terms.
   * \param msg The TDCP pseudo-measurement.
   * \param T_0g_statevar Extra state variable required for TDCP, global pose.
   */
  void addTdcpCost(const TdcpMsg::SharedPtr &msg,
                   const steam::se3::TransformEvaluator::ConstPtr &T_0g,
                   const steam::se3::TransformEvaluator::ConstPtr &T_0i);

  /**
   * \brief Verifies the input data being used in the optimization problem,
   * namely, the inlier matches and initial estimate.
   * \param qdata The query data.
   * \param mdata The map data.
   */
  virtual bool verifyInputData(QueryCache &qdata, MapCache &mdata);

  /**
   * \brief Verifies the output data generated byt the optimization problem
   * \param qdata The query data.
   * \param mdata The map data.
   */
  virtual bool verifyOutputData(QueryCache &qdata, MapCache &mdata);

  /**
   * \brief performs sanity checks on the landmark
   * \param point The landmark.
   * \param qdata The query data.*
   * \return true if the landmark meets all checks, false otherwise.
   */
  bool isLandmarkValid(const Eigen::Vector3d &point, QueryCache &qdata);

  /** \brief the cost terms associated with landmark observations. */
  steam::ParallelizedCostTermCollection::Ptr vision_cost_terms_;

  /** \brief the cost terms associated with white-noise-on-acceleration motion prior */
  steam::ParallelizedCostTermCollection::Ptr smoothing_cost_terms_;

  /** \brief The cost terms associated with landmark depth. */
  steam::ParallelizedCostTermCollection::Ptr depth_cost_terms_;

  /** \brief The cost terms associated with carrier phase measurements (TDCP) */
  steam::ParallelizedCostTermCollection::Ptr tdcp_cost_terms_;

  /** \brief The cost for prior on global orientation needed for TDCP  */
  steam::ParallelizedCostTermCollection::Ptr global_prior_cost_term_;

  /** \brief The loss function used for the depth cost. */
  steam::LossFunctionBase::Ptr sharedDepthLossFunc_;

  /** \brief the loss function associated with observation cost. */
  steam::LossFunctionBase::Ptr sharedLossFunc_;

  /** \brief The loss function used for the TDCP cost. */
  steam::LossFunctionBase::Ptr sharedTdcpLossFunc_;

  /** \brief The steam problem. */
  std::shared_ptr<steam::OptimizationProblem> problem_;

  /** \brief Module configuration. */
  std::shared_ptr<Config> window_config_;

  /** \brief Stores absolute orientation for use with GPS terms */
  steam::se3::TransformStateVar::Ptr T_0g_statevar_;

  std::vector<std::vector<double>> ypr_estimates_;   // temporary
};

}  // namespace stereo
}  // namespace tactic
}  // namespace vtr