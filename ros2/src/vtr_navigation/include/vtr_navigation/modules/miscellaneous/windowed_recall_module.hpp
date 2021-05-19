#pragma once

#include <vtr_navigation/modules/base_module.hpp>
#include <vtr_vision/messages/bridge.hpp>

namespace vtr {
namespace navigation {

/**
 * \brief A module that retrieves landmarks from multiple graph vertices and
 * store them into map cache.
 */
class WindowedRecallModule : public BaseModule {
 public:
  /**
   * \brief Static module identifier.
   * \todo change this to static_name
   */
  static constexpr auto type_str_ = "windowed_recall";

  /** \brief Collection of config parameters */
  struct Config {
    int window_size;
    bool tdcp_enable;
    Eigen::Matrix<double, 6, 6> default_T_0g_cov;
  };

  WindowedRecallModule(std::string name = type_str_) : BaseModule{name} {};
  ~WindowedRecallModule() = default;

  /**
   * \brief Given a window size, and a start vertex, recall all of the
   * landmarks and observations within the window, and set up a chain of poses
   * in a single coordinate frame.
   */
  virtual void run(QueryCache &qdata, MapCache &mdata,
                   const std::shared_ptr<const Graph> &graph);

  /** \brief Does nothing? */
  virtual void updateGraph(QueryCache &, MapCache &,
                           const std::shared_ptr<Graph> &, VertexId);

  /** \brief Sets the module's configuration. */
  void setConfig(std::shared_ptr<Config> &config) { config_ = config; }

 private:
  /**
   * \brief Loads a specific vertex's landmarks and observations into the
   * landmark and pose map.
   * \param[in,out] lm_map A map containing all currently observed landmarks
   with observations.
   * \param[in,out] poses A map containing poses associated with each vertex.
   * \param transforms TODO
   * \param current_vertex The current vertex
   * \param rig_name TODO
   * \param graph The pose graph.
   */
  void loadVertexData(LandmarkMap &lm_map, SteamPoseMap &poses,
                      SensorVehicleTransformMap &transforms,
                      const pose_graph::RCVertex::Ptr &current_vertex,
                      const std::string &rig_name,
                      const std::shared_ptr<const Graph> &graph);

  /**
   * \brief Loads a all of the landmarks and observations for a specific
   * vertex's channel.
   * \param[in,out] lm_map A map containing all currently observed landmarks
   with observations.
   * \param[in,out] poses A map containing poses associated with each vertex.
   * \param transforms TODO
   * \param current_vertex The current vertex
   * \param channel_obs TODO
   * \param rig_name TODO
   * \param graph The pose graph.
   */
  void loadLandmarksAndObs(
      LandmarkMap &lm_map, SteamPoseMap &poses,
      SensorVehicleTransformMap &transforms,
      const pose_graph::RCVertex::Ptr &current_vertex,
      const vtr_messages::msg::ChannelObservations &channel_obs,
      const std::string &rig_name, const std::shared_ptr<const Graph> &graph);

  /**
   * \brief Given a set of vertices, computes poses for each vertex in a single
   * global coordinate frame.
   * \param[in,out] poses A map containing poses associated with each vertex.
   * \param graph The pose graph.
   */
  void computePoses(SteamPoseMap &poses,
                    const std::shared_ptr<const Graph> &graph);

  void getTimesandVelocities(SteamPoseMap &poses,
                             const std::shared_ptr<const Graph> &graph);

  /**
   * \brief Loads the sensor transform from robochunk via a vertex ID
   * \param vid The Vertex ID of the vertex we need to load the transform from.
   * \param transforms The map of vertex ID to T_s_v's
   * \param rig_name the name of the current rig
   * \param graph A pointer to the pose graph.
   */
  void loadSensorTransform(const VertexId &vid,
                           SensorVehicleTransformMap &transforms,
                           const std::string &rig_name,
                           const Graph::ConstPtr &graph);

  /**
   * \brief a map that keeps track of the pointers into the vertex landmark
   * messages.
   */
  std::map<VertexId, std::shared_ptr<vtr_messages::msg::RigLandmarks>>
      vertex_landmarks_;

  /** \brief Module configuration. */
  std::shared_ptr<Config> config_;
};

}  // namespace navigation
}  // namespace vtr
