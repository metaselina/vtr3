#pragma once

#include <pcl_conversions/pcl_conversions.h>

#include <vtr_tactic/modules/base_module.hpp>

// temp
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <vtr_messages_lidar/msg/movability.hpp>
#include <vtr_messages_lidar/msg/point_map.hpp>
using PointCloudMsg = sensor_msgs::msg::PointCloud2;
using PointXYZMsg = vtr_messages_lidar::msg::PointXYZ;
using MovabilityMsg = vtr_messages_lidar::msg::Movability;
using PointMapMsg = vtr_messages_lidar::msg::PointMap;

namespace vtr {
namespace tactic {
namespace lidar {

/** \brief */
class MapRecallModule : public BaseModule {
 public:
  /** \brief Static module identifier. */
  static constexpr auto static_name = "lidar.map_recall";

  /** \brief Config parameters. */
  struct Config {
    float map_voxel_size = 0.2;
    bool visualize = false;
  };

  MapRecallModule(const std::string &name = static_name)
      : BaseModule{name}, config_(std::make_shared<Config>()) {}

  void configFromROS(const rclcpp::Node::SharedPtr &node,
                     const std::string param_prefix) override;

 private:
  void runImpl(QueryCache &qdata, const Graph::ConstPtr &graph) override;

  void visualizeImpl(QueryCache &, const Graph::ConstPtr &,
                     std::mutex &) override;

  /** \brief Module configuration. */
  std::shared_ptr<Config> config_;

  /** \brief for visualization only */
  rclcpp::Publisher<PointCloudMsg>::SharedPtr map_pub_;
};

}  // namespace lidar
}  // namespace tactic
}  // namespace vtr