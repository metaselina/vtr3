#pragma once

#include <chrono>

#include <rclcpp/rclcpp.hpp>

#include <vtr_lgmath_extensions/conversions.hpp>
#include <vtr_messages/msg/graph_path.hpp>
#include <vtr_messages/msg/rig_images.hpp>
#include <vtr_messages/msg/robot_status.hpp>
#include <std_msgs/msg/int32.hpp>
#include <vtr_messages/srv/get_rig_calibration.hpp>
#include <vtr_messages/srv/set_graph.hpp>
#include <vtr_messages/srv/trigger.hpp>
#include <vtr_mission_planning/ros_callbacks.hpp>
#include <vtr_mission_planning/ros_mission_server.hpp>
#include <vtr_mission_planning/state_machine.hpp>
#include <vtr_navigation/factories/ros_tactic_factory.hpp>
#include <vtr_navigation/publisher_interface.hpp>
#include <vtr_navigation/types.hpp>
#include <vtr_path_planning/simple_planner.hpp>
#include <vtr_vision/messages/bridge.hpp>

#if false
#include <tf/transform_listener.h>

#include <asrl/pose_graph/index/RCGraph.hpp>
#endif
#if 0
#include <actionlib/client/simple_action_client.h>
#include <geometry_msgs/Transform.h>
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/Joy.h>
#include <sensor_msgs/NavSatFix.h>

#include <tf/transform_broadcaster.h>
#include <visualization_msgs/MarkerArray.h>
// For new path tracker callback
#include <std_msgs/UInt8.h>

#include <asrl/navigation/Caches.hpp>
#include <asrl/navigation/assemblies/RosAssemblyBuilder.hpp>
#include <asrl/navigation/tactics/TacticConfig.hpp>

#include <asrl__messages/FollowPathAction.h>
#include <asrl__messages/Path.h>
#include <asrl__messages/RobotStatus.h>
#include <asrl__messages/TrackingStatus.h>

#include <babelfish_robochunk_robochunk_sensor_msgs/RigImages.h>
#include <asrl/common/rosutil/transformation_utilities.hpp>
#include <asrl/messages/lgmath_conversions.hpp>
#include <asrl/planning/TimeDeltaPlanner.hpp>
#include <asrl/pose_graph/path/LocalizationChain.hpp>
#endif

using namespace std::chrono_literals;

namespace vtr {
namespace navigation {
#if false
class BasicTactic;
#endif
#if 0
class ParallelTactic;
#endif

using GetRigCalibration = vtr_messages::srv::GetRigCalibration;
using Trigger = vtr_messages::srv::Trigger;
using SetGraph = vtr_messages::srv::SetGraph;
using RigImages = vtr_messages::msg::RigImages;
using RigCalibration = vtr_messages::msg::RigCalibration;
using GpggaMsg = nmea_msgs::msg::Gpgga;
using PlannerPtr = vtr::path_planning::PlanningInterface::Ptr;

class Navigator : public PublisherInterface {
 public:
#if 0
  using RigCalibrationPtr = std::shared_ptr<vision::RigCalibration> ;
  using ImuCalibrationPtr = std::shared_ptr<vision::IMUCalibration> ;
  using RigImagesPtr = std::shared_ptr<robochunk::sensor_msgs::RigImages> ;
  using RCGraph = asrl::pose_graph::RCGraph ;
  using TransformType = asrl::pose_graph::RCGraph::TransformType ;
  using QueryCachePtr = std::shared_ptr<asrl::navigation::QueryCache> ;
#endif
  using RobotMsg = vtr_messages::msg::RobotStatus;
  using PathMsg = vtr_messages::msg::GraphPath;
#if 0
  using TrackingStatus = asrl__messages::TrackingStatus;

  using FollowPathAction = asrl__messages::FollowPathAction;
  using FollowPathResultConstPtr = asrl__messages::FollowPathResultConstPtr;
  using ActionClientType = actionlib::SimpleActionClient<FollowPathAction>;

  PTR_TYPEDEFS(Navigator);
#endif

  Navigator(const std::shared_ptr<rclcpp::Node> node)
      : node_(node) /*, pathClient_("/path_tracker/action_server", true),
                   tracking_(false), speed_(0)*/
  {
    // Set busy flag to true while node initializes.
    busy_ = true;

    // Lock just in case we get a really early data message before things have
    // settled
    std::lock_guard<std::mutex> lck(queue_lock_);

    busy_service_ = node_->create_service<Trigger>(
        "busy", std::bind(&Navigator::_busyCallback, this,
                          std::placeholders::_1, std::placeholders::_2));

    directory_change_ = node_->create_service<SetGraph>(
        "set_graph", std::bind(&Navigator::_setGraphCallback, this,
                               std::placeholders::_1, std::placeholders::_2));

    robot_publisher_ = node_->create_publisher<RobotMsg>("robot", 5);

    frames_on_vo_publisher_ = node_->create_publisher<std_msgs::msg::Int32>("frames_on_vo", 5);
#if 0
    plannerChange_ = nh_.advertiseService("in/reset_planner", &Navigator::_reloadPlannerCallback, this);
    statusPublisher_ = nh_.advertise<TrackingStatus>("out/tracker_status", 5, true);
    gimbalPublisher_ = nh_.advertise<geometry_msgs::TransformStamped>("out/gimbal", 1, true);
#endif

    // generate a tactic from the factory
    ROSTacticFactory tactic_factory(node_, "tactic");
    tactic_ = tactic_factory.makeVerified();
    // get a pointer to the graph
    graph_ = tactic_->poseGraph();

    // initialize a pipeline
    _initializePipeline();

    // by default, make the buffer small so that we drop frames, but some
    // playback tools require frames to be buffered
    auto subscriber_buffer_len =
        node_->declare_parameter<int>("navigator/subscriber_buffer_len", 1);
#if 0
    // check if the sensor frame is not static
    nh_.param<bool>("navigator/nonstatic_sensor_frame", nonstatic_sensor_frame_, false);
#endif
    // subscribers
    rigimages_subscription_ = node_->create_subscription<RigImages>(
        "xb3_images", subscriber_buffer_len,
        std::bind(&Navigator::_imageCallback, this, std::placeholders::_1));
    gpspos_subscription_ = node_->create_subscription<GpggaMsg>(
        "gpgga", 10, std::bind(&Navigator::_gpggaCallback, this, std::placeholders::_1));
    following_path_publisher_ =
        node_->create_publisher<PathMsg>("out/following_path", 1);
#if 0
    navsatfix_subscriber_ = nh_.subscribe("/in/navsatfix",subscriber_buffer_len,&Navigator::NavSatFixCallback,this);
    wheelodom_subscriber_ = nh_.subscribe("/in/odom",subscriber_buffer_len,&Navigator::OdomCallback,this);
    joy_subscriber_ = nh_.subscribe("in/joy", 1, &Navigator::JoyCallback, this);
    imu_subscriber_ = nh_.subscribe("in/imu", 1, &Navigator::ImuCallback, this);
    path_tracker_subscriber_ = nh_.subscribe("/path_done_status", 1, &Navigator::_pathDoneCallback, this);
    gimbal_subscriber_ = nh_.subscribe("in/gimbal", 1, &Navigator::GimbalCallback, this);

    // Set up the action server.
    loc_stream.open("/home/asrl/loc_data.csv");

    path_pub_ = nh_.advertise<visualization_msgs::MarkerArray>("out/path_viz", 1, true);
#endif

    // service clients
    rig_calibration_client_ =
        node_->create_client<GetRigCalibration>("xb3_calibration");

    // Set busy flag to false once node is initialized.
    busy_ = false;

    // we don't have any images in the queue yet
    image_in_queue_ = false;

    // set quit flag false
    quit_ = false;

    // launch the processing thread
    process_thread_ = std::thread(&Navigator::process, this);
  }

  ~Navigator() {
#if false
    loc_stream.close();
#endif
    halt(true, true);
  }

  /** \brief Update robot messages for the UI */
  // void publishRobot(const pose_graph::LocalizationChain &chain,
  // VertexId currentVertex) override;

  /** \brief Update robot messages for the UI */
  void publishRobot(const Localization& persistentLoc, uint64_t pathSeq = 0,
                    const Localization& targetLoc = Localization()) override;

  /** \brief Delete ALL the things!  ...but cleanly */
  bool halt(bool force = false, bool save = true);

#if 0
  /** \brief Set the calibration for the stereo camera */
  inline void setCalibration(RigCalibrationPtr &calibration) { rig_calibration_ = calibration; }

  /** \brief Get the tactic being used */
  inline const asrl::navigation::BasicTactic & tactic() { return *tactic_; }
#endif
  /** \brief Set the path followed by the path tracker */
  void publishPath(const pose_graph::LocalizationChain& chain) override;

  /** \brief Set the localization chain info for the safety monitor */
  void publishVoFrames(int keyframes_on_vo) override;

  /** \brief Clear the path followed by the path tracker */
  void clearPath() override;
#if 0
  /** \brief Update localization messages for the path tracker */
  virtual void updateLocalization(const TransformType &T_leaf_trunk,
                                  const TransformType &T_root_trunk,
                                  const TransformType &T_leaf_trunk_sensor,
                                  uint64_t stamp);

  /** \brief Publish T_0_q in tf for rviz. */
  virtual void publishT_0_q(QueryCachePtr q_data);

  /** \brief Publish T_0_q in tf for rviz. */
  virtual void publishPathViz(QueryCachePtr q_data, pose_graph::LocalizationChain &loc_chain);
#endif

 private:
  /** \brief Initial pipeline setup */
  void _initializePipeline();

  /**
   * \brief ROS callback to check if the node is currently processing an image.
   */
  /// bool busyCallback(std_srvs::Trigger::Request& /*request*/,
  ///                   std_srvs::Trigger::Response& response);
  void _busyCallback(std::shared_ptr<Trigger::Request> request,
                     std::shared_ptr<Trigger::Response> response);

  /** \brief ROS callback to change the graph directory */
  /// bool _setGraphCallback(SetGraphRequest& request,
  ///                                   SetGraphResponse& response)
  void _setGraphCallback(std::shared_ptr<SetGraph::Request> request,
                         std::shared_ptr<SetGraph::Response> response);

#if 0
  /// @brief ROS callback when the path tracker is finished
  void _pathCallback(const actionlib::SimpleClientGoalState& state,
                     const FollowPathResultConstPtr& result);

  /// @brief ROS callback when the path tracker is finished. Used for the new path tracker. Temporary.
  void _pathDoneCallback(const std_msgs::UInt8 status_msg);

  /// @brief ROS callback to reload path planner parameters
  bool _reloadPlannerCallback(std_srvs::Trigger::Request& request, std_srvs::TriggerResponse& response);
#endif
  /** \brief Pulls the path planner constants from ROS and builds the planner */
  void _buildPlanner();

  /** \brief ROS callback to accept rig images */
  void _imageCallback(const RigImages::SharedPtr msg);

  /** \brief ROS callback to accept GPGGA (GNSS position) msgs */
  void _gpggaCallback(GpggaMsg::SharedPtr msg);

  /** \brief Fetch image calibration data from a service */
  void _fetchCalibration();
#if 0
  RigCalibrationPtr fetchCalibration();
  vision::RigImages copyImages(const babelfish_robochunk_robochunk_sensor_msgs::RigImages &img);

  /// @brief ROS callback to accept NavSatFix messages
  void NavSatFixCallback(const sensor_msgs::NavSatFix &fix);

  /// @brief ROS callback to accept imu messages
  void ImuCallback(const ::sensor_msgs::ImuPtr& imu_msg);
  ImuCalibrationPtr fetchImuCalibration();

  /// @brief ROS callback to accept Odom messages
  void OdomCallback(const nav_msgs::Odometry &odom_data);

  /// @brief ROS callback to accept joy messages
  void JoyCallback(const ::sensor_msgs::JoyPtr& joy_msg);

  /// @brief ROS callback to accept joy messages
  void GimbalCallback(const ::geometry_msgs::PoseStampedPtr& gimbal_msg);
#endif
  /// @brief the queue processing function
  void process(void);
#if 0
  std::ofstream loc_stream;

  /// @brief Broadcasts the localization transform data for the path tracker.
  tf::TransformBroadcaster localizationBroadcaster_;

  /// @brief ROS-listeners for vehicle-sensor transforms
  tf::TransformListener tf_listener_;
#endif
  // ros::Publisher robotPublisher_;
  rclcpp::Publisher<RobotMsg>::SharedPtr robot_publisher_;

  /** \brief Publishes how long localization pipeline has gone without localizing */
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr frames_on_vo_publisher_;
#if 0
  ros::ServiceServer plannerChange_;

  /// @brief NavSatFix Subscriber
  ros::Subscriber navsatfix_subscriber_;

  /// @brief WheelOdom Subscriber
  ros::Subscriber wheelodom_subscriber_;

  /// @brief Stereo Camera Calibration
  ros::ServiceClient calibration_client_;

  /// @brief Joy Subscriber
  ros::Subscriber joy_subscriber_;

  /// @brief Imu Subscriber
  ros::Subscriber imu_subscriber_;

  /// @brief PathCallback subscriber. For new path tracker.  Temporary
  ros::Subscriber path_tracker_subscriber_;

  /// @brief Gimbal Subscriber
  ros::Subscriber gimbal_subscriber_;
#endif
  /** \brief Publisher to send the path tracker new following paths. */
  // ros::Publisher followingPathPublisher_;
  rclcpp::Publisher<PathMsg>::SharedPtr following_path_publisher_;
#if 0
  /// @brief Publisher to send status updates from the navigator
  ros::Publisher statusPublisher_;

  /// @brief Publisher to send the T_leaf_trunk transform in the sensor frame to a gimbal controller
  ros::Publisher gimbalPublisher_;

  /// @brief TF broadcaster for rviz
  tf::TransformBroadcaster T_0_q_broadcaster_;

  /// @brief path viz publisher for rviz
  ros::Publisher path_pub_;

  /// @brief Calibration for the IMU
  ImuCalibrationPtr imu_calibration_;

  /// @brief The latest joystick message
  ::sensor_msgs::JoyPtr joy_msg_;

  /// @brief The latest gimbal message
  ::geometry_msgs::PoseStampedPtr gimbal_msg_;

  /// @brief The name of the co-ordinate frame of the IMU
  std::string imu_frame_;

  ActionClientType pathClient_;
  bool tracking_;
  double speed_;

  /// @brief Monocular configurations need a position message before an image
  bool wait_for_pos_msg_;

  /// @brief If the sensor frame is non-static, we need to request it each time
  bool nonstatic_sensor_frame_;

  uint64_t image_recieve_time_ns ;
  RunId last_run_viz_ = -1;
#endif

  PlannerPtr planner_;

  /** \brief ROS-handle for communication */
  const std::shared_ptr<rclcpp::Node> node_;

  /** \brief ROS communications */
  rclcpp::Service<Trigger>::SharedPtr busy_service_;

  rclcpp::Service<SetGraph>::SharedPtr directory_change_;
  /** \brief Rig Images Subscriber */
  rclcpp::Subscription<RigImages>::SharedPtr rigimages_subscription_;
  rclcpp::Client<GetRigCalibration>::SharedPtr rig_calibration_client_;

  /** \brief GNSS Position Subscriber */
  rclcpp::Subscription<GpggaMsg>::SharedPtr gpspos_subscription_;

  /**
   * \brief Set true when pipeline is processing, and read by robochunk sensor
   * driver via service to reduce AFAP playback speed
   */
  std::atomic<bool> busy_;

  /** \brief a flag to let the process() thread know when to quit */
  std::atomic<bool> quit_;

  /** \brief the queue processing thread */
  std::thread process_thread_;
  /** \brief a notifier for when jobs are on the queue */
  std::condition_variable process_;

  /** \brief Tactic used for image processing */
  std::shared_ptr<BasicTactic> tactic_;
  /** \brief Pose graph */
  std::shared_ptr<pose_graph::RCGraph> graph_;

  /** \brief The name of the co-ordinate frame of the sensor */
  std::string sensor_frame_;
  /** \brief The name of the co-ordinate frame for the controller */
  std::string control_frame_;
  /** \brief Transformation between the sensor and vehicle */
  EdgeTransform T_sensor_vehicle_;

  /** \brief Calibration for the stereo rig */
  std::shared_ptr<vision::RigCalibration> rig_calibration_;
  /** \brief the image queue */
  std::queue<QueryCachePtr> queue_;
  /** \brief a lock to coordinate adding/removing jobs from the queue */
  std::mutex queue_lock_;
  std::atomic<bool> image_in_queue_;
  /** \brief Drop image messages if there is already one in the queue */
  bool drop_frames_;

  /** \brief Mission-specific members */
  mission_planning::state::StateMachine::Ptr state_machine_;
  mission_planning::RosCallbacks::Ptr graphCallbacks_;
  std::unique_ptr<mission_planning::RosMissionServer> mission_server_;
};

}  // namespace navigation
}  // namespace vtr
