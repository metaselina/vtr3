#pragma once

#include <pcl_conversions/pcl_conversions.h>

#include <vtr_lidar/pointmap/pointmap.hpp>
#include <vtr_lidar/ray_tracing.hpp>
#include <vtr_tactic/modules/base_module.hpp>

// visualization
#include <sensor_msgs/msg/point_cloud2.hpp>
using PointCloudMsg = sensor_msgs::msg::PointCloud2;

namespace vtr {
namespace tactic {
namespace lidar {

/** \brief Preprocess raw pointcloud points and compute normals */
class MapMaintenanceModule : public BaseModule {
 public:
  /** \brief Static module identifier. */
  static constexpr auto static_name = "lidar.map_maintenance";

  /** \brief Collection of config parameters */
  struct Config {
    float map_voxel_size = 0.03;
    // dynamic objects remocal
    float horizontal_resolution = 0.001;
    float vertical_resolution = 0.001;
    int min_num_observations = 0;
    int max_num_observations = 20;

    bool visualize = false;
  };

  MapMaintenanceModule(const std::string &name = static_name)
      : BaseModule{name}, config_(std::make_shared<Config>()){};

  void configFromROS(const rclcpp::Node::SharedPtr &node,
                     const std::string param_prefix) override;

 private:
  void runImpl(QueryCache &qdata, MapCache &mdata,
               const Graph::ConstPtr &graph) override;

  /** \brief Visualization */
  void visualizeImpl(QueryCache &, MapCache &, const Graph::ConstPtr &,
                     std::mutex &) override;

  /** \brief Module configuration. */
  std::shared_ptr<Config> config_;

  /** \brief for visualization only */
  rclcpp::Publisher<PointCloudMsg>::SharedPtr pc_pub_;
  rclcpp::Publisher<PointCloudMsg>::SharedPtr map_pub_;
  rclcpp::Publisher<PointCloudMsg>::SharedPtr movability_map_pub_;
};

}  // namespace lidar
}  // namespace tactic
}  // namespace vtr