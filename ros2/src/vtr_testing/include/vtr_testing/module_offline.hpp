//#include <chrono>
#include <filesystem>
//
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>      //todo: sort out includes between VO/Offline
//#include <tf2_ros/transform_listener.h>
//#include <rclcpp/rclcpp.hpp>
//
#include <vtr_common/timing/simple_timer.hpp>
#include <vtr_navigation/factories/ros_tactic_factory.hpp>
#include <vtr_vision/messages/bridge.hpp>
//#include <vtr_vision/types.hpp>

///// #include <babelfish_robochunk_robochunk_sensor_msgs/RigImages.h>
#include <vtr_messages/msg/rig_images.hpp>
//#include <vtr_messages/msg/time_stamp.hpp>

//#include <vtr_storage/data_stream_reader.hpp>
#if false
#include <robochunk_msgs/XB3CalibrationResponse.pb.h>
#include <vtr_navigation/tactics/basic_tactic.h>
#include <vtr_navigation/tactics/parallel_tactic.h>
#endif

namespace fs = std::filesystem;
using namespace vtr;

/** StereoNavigationNode::fromStampedTransformation */
lgmath::se3::Transformation fromStampedTransformation(
    geometry_msgs::msg::TransformStamped const &t_base_child) {
  // converts to a tf2::Transform first.
  tf2::Stamped<tf2::Transform> tf2_base_child;
  tf2::fromMsg(t_base_child, tf2_base_child);

  Eigen::Matrix4d T;
  T.setIdentity();
  tf2::Matrix3x3 C_bc(tf2_base_child.getRotation());
  for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 3; ++col) T(row, col) = C_bc[row][col];
  T(0, 3) = tf2_base_child.getOrigin().x();
  T(1, 3) = tf2_base_child.getOrigin().y();
  T(2, 3) = tf2_base_child.getOrigin().z();
  /// LOG(DEBUG) << T << std::endl;
  return lgmath::se3::Transformation(T);
}

class ModuleOffline {
 public:
  ModuleOffline(const std::shared_ptr<rclcpp::Node> node, fs::path &results_dir)
      : node_(node) {
    fs::create_directories(results_dir);
    outstream_.open(results_dir / "results.csv");
    outstream_ << "timestamp,map vertex (run),map vertex (vertex),r\n";

    navigation::ROSTacticFactory tactic_factory{node_, "tactic"};
    tactic_ = tactic_factory.makeVerified();
    graph_ = tactic_->poseGraph();
  }

  ~ModuleOffline() { saveGraph(); }

  void setCalibration(std::shared_ptr<vision::RigCalibration> calibration) {
    rig_calibration_ = calibration;
  }

  void processImageData(
      std::shared_ptr<vtr_messages::msg::RigImages> rig_images,
      const vtr_messages::msg::TimeStamp &stamp) {
    navigation::QueryCachePtr query_data{new navigation::QueryCache};

    // add the timestamp
    *query_data->stamp = stamp;

    // add the rig names
    auto &rig_names = query_data->rig_names.fallback();
    rig_names->push_back(sensor_frame_);

    // add the images to the cache
    rig_images->name = sensor_frame_; // \todo (yuchen) should not be set here
    auto &images = query_data->rig_images.fallback();
    images->emplace_back(messages::copyImages(*rig_images.get()));

    // add calibration data
    auto &calibration_list = query_data->rig_calibrations.fallback();
    calibration_list->emplace_back(*rig_calibration_.get());

    tactic_->runPipeline(query_data);
#if false
    if (dynamic_cast<navigation::ParallelTactic *>(tactic_.get())) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
#endif

    idx++;
    if (idx % 100 == 0) {
      double elapsed = timer.elapsedMs();
      double avg_speed = elapsed / 100;
      LOG(INFO) << "proc. image: node: " << node_->get_name() << "  "
                << "vtx: " << tactic_->currentVertexID() << "  "
                << "rate: " << 1000 / avg_speed << " hz";
      timer.reset();
    }
  }

  void saveGraph() {
    navigation::EdgeTransform T_curr(true);
    auto path_itr = graph_->beginDfs(navigation::VertexId(0, 0));
    for (; path_itr != graph_->end(); ++path_itr) {
      T_curr = T_curr * path_itr->T();
      if (path_itr->from().isValid()) LOG(INFO) << path_itr->e()->id();
      outstream_ << (path_itr->v()->keyFrameTime().nanoseconds_since_epoch) /
          1e9
                 << "," << path_itr->v()->id().majorId() << ","
                 << path_itr->v()->id().minorId();
      // flatten r vector to save
      auto tmp = T_curr.r_ab_inb();
      auto T_flat = std::vector<double>(tmp.data(), tmp.data() + 3);
      for (auto v : T_flat) outstream_ << "," << v;
      outstream_ << "\n";
    }
    outstream_.close();
    graph_->save();
  }

 protected:
  virtual void initializePipeline() = 0;

  const std::shared_ptr<rclcpp::Node> node_;
  std::ofstream outstream_;

  int idx = 0;
  common::timing::SimpleTimer timer;

#if false
  /// @brief TF listener to get the camera->robot transformation.
  tf::TransformListener tf_listener_;
#endif

  std::string sensor_frame_;
  std::string control_frame_;
  lgmath::se3::Transformation T_sensor_vehicle_;

  // Pipeline / tactic
  std::shared_ptr<navigation::BasicTactic> tactic_;
  std::shared_ptr<pose_graph::RCGraph> graph_;

  // add the rig images to the query data.
  std::shared_ptr<vision::RigCalibration> rig_calibration_;
};