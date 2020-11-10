#include <vtr_testing/module_offline.hpp>

#include <tf2_ros/transform_listener.h>

#include <vtr_navigation/types.hpp>

/// \brief Privileged edge mask. Used to create a subgraph on privileged edges
using PrivilegedEvaluator = pose_graph::eval::Mask::Privileged<pose_graph::RCGraph>::Caching;
using PrivilegedEvaluatorPtr = PrivilegedEvaluator::Ptr;


class ModuleLoc : public ModuleOffline {
 public:
  ModuleLoc(const std::shared_ptr<rclcpp::Node> node, fs::path &results_dir)
      : ModuleOffline(node, results_dir) {

    initializePipeline();
  }

 private:
  void initializePipeline() final {
// Get the path that we should repeat
    vtr::navigation::BasicTactic::VertexId::Vector sequence;
    sequence.reserve(graph_->numberOfVertices());
    // Extract the privileged sub graph from the full graph.
    PrivilegedEvaluatorPtr evaluator(new PrivilegedEvaluator());
    evaluator->setGraph(graph_.get());
    auto privileged_path = graph_->getSubgraph(0ul, evaluator);
    for (auto it = privileged_path->begin(0ul); it != privileged_path->end();
         ++it) {
      LOG(INFO) << it->v()->id();
      sequence.push_back(it->v()->id());
    }

    // normally the state machine would add a run when a goal is started. We
    // spoof that here.
    tactic_->addRun();

    // Initialize localization
    tactic_->setPath(sequence);

    // Create a Metric Localization pipeline.
    tactic_->setPipeline(mission_planning::PipelineType::MetricLocalization);

    // get the co-ordinate frame names
    control_frame_ =
        node_->declare_parameter<std::string>("control_frame", "base_link");
    sensor_frame_ =
        node_->declare_parameter<std::string>("sensor_frame", "front_xb3");

    // Extract the Vehicle->Sensor transformation.
    rclcpp::Clock::SharedPtr clock =
        std::make_shared<rclcpp::Clock>(RCL_SYSTEM_TIME);
    tf2_ros::Buffer tf_buffer{clock};
    tf2_ros::TransformListener tf_listener{tf_buffer};
    auto tf_sensor_vehicle =
        tf_buffer.lookupTransform(sensor_frame_, control_frame_,
                                  tf2::TimePoint(), tf2::durationFromSec(5));
    T_sensor_vehicle_ = fromStampedTransformation(tf_sensor_vehicle);
    T_sensor_vehicle_.setZeroCovariance();
    tactic_->setTSensorVehicle(T_sensor_vehicle_);

    // whether or not to save the graph
    save_graph_ = node_->declare_parameter<bool>("save_graph", false);

  }

  bool save_graph_;
};