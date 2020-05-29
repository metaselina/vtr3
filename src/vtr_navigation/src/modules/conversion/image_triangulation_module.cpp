#include <asrl/navigation/modules/conversion/image_triangulation_module.h>

#include <asrl/vision/TypeHelpers.hpp>
#include <asrl/vision/geometry/geometry_tools.hpp>
// #include <asrl/navigation/Visualize.hpp>

namespace asrl {
namespace navigation {

#if 0
void ImageTriangulationModule::setConfig(std::shared_ptr<Config> &config) {
  config_ = config;
}
#endif

/// @brief Given two frames and matches detects the inliers that fit
///        the given model, and provides an initial guess at transform T_q_m.
void ImageTriangulationModule::run(QueryCache &qdata, MapCache &mdata,
                                   const std::shared_ptr<const Graph> &) {
  // check if the required data is in this cache
  if (!qdata.rig_features.is_valid() || !qdata.rig_calibrations.is_valid()) {
    return;
  }
  // Inputs, frames, calibration
  const auto &features = *qdata.rig_features;
  const auto &calibrations = *qdata.rig_calibrations;

  // Outputs, candidate_landmarks
  auto &candidate_landmarks = qdata.candidate_landmarks.fallback();

  // Iterate through the rigs
  auto feature_itr = features.begin();
  auto calibration_itr = calibrations.begin();

  // Go through each rig
  for (; feature_itr != features.end() && calibration_itr != calibrations.end();
       ++feature_itr, ++calibration_itr) {
    // add an empty set of rig landmarks for this rig
    candidate_landmarks->emplace_back(vision::RigLandmarks());
    auto &rig_landmarks = candidate_landmarks->back();
    rig_landmarks.name = feature_itr->name;

    // get the minimum approximate disparity to correspond with a maximum depth
    // get the maximum approximate disparity to correspond with a minimum depth
    // this assumes the stereo rig is a 'standard' horizontally parallel type
    float d_min = 0;
    float d_max = std::numeric_limits<float>::max();
    if (calibration_itr->extrinsics.size() > 1) {
      double baseline = calibration_itr->extrinsics.back().r_ba_ina()[0];
      double f = calibration_itr->intrinsics.front()(0, 0);
      d_min = f * baseline / config_->max_triangulation_depth;
      d_max = f * baseline / config_->min_triangulation_depth;
    }

    for (const auto &channel : feature_itr->channels) {
      // add an empty set of channel landmarks to this rig
      rig_landmarks.channels.emplace_back(vision::ChannelLandmarks());
      auto &landmarks = rig_landmarks.channels.back();
      landmarks.name = channel.name;

      // if this channel has no features, then go to the next
      if (channel.cameras.size() == 0) {
        continue;
      }

      auto num_cameras = channel.cameras.size();
      auto num_keypoints = channel.cameras[0].keypoints.size();

      // copy the descriptor info from the feature.
      landmarks.appearance.descriptors = channel.cameras[0].descriptors.clone();
      landmarks.appearance.feat_infos = channel.cameras[0].feat_infos;
      landmarks.appearance.feat_type = channel.cameras[0].feat_type;
      landmarks.appearance.name = channel.cameras[0].name;
      // technically this shouldn't be allowed as landmark appearance shouldn't
      // contain keypoints for now it is utilised in mono RANSAC instead of the
      // observations
      landmarks.appearance.keypoints = channel.cameras[0].keypoints;

      // Iterate through the observations of the landmark from each camera and
      // triangulate.

      landmarks.points = vision::Points3_t::Zero(3, num_keypoints);
      landmarks.covariances.resize(9, num_keypoints);
      landmarks.valid.resize(num_keypoints, false);

      for (uint32_t keypoint_idx = 0; keypoint_idx < num_keypoints;
           ++keypoint_idx) {
        // check if we meet the disparity limit for this keypoint
        float d = std::abs(
            channel.cameras[0].keypoints[keypoint_idx].pt.x -
            channel.cameras[num_cameras - 1].keypoints[keypoint_idx].pt.x);
        if (d < d_min) continue;
        if (d > d_max) continue;

        // Extract the keypoints and their info (precision) for every camera
        std::vector<cv::Point2f> keypoints;
        keypoints.reserve(num_cameras);
        vision::FeatureInfos feat_infos;
        feat_infos.reserve(num_cameras);
        for (uint32_t camera_idx = 0; camera_idx < num_cameras; ++camera_idx) {
          keypoints.emplace_back(
              channel.cameras[camera_idx].keypoints[keypoint_idx].pt);
          feat_infos.emplace_back(
              channel.cameras[camera_idx].feat_infos[keypoint_idx]);
        }

        // if there is more than one camera, triangulate. Mono landmarks will
        // get initialized elsewhere
        if (calibration_itr->extrinsics.size() > 1) {
          // set up the landmarks from the rig observations (with covariance)
          landmarks.points.col(keypoint_idx) = vision::triangulateFromRig(
              *calibration_itr, keypoints, feat_infos,
              &landmarks.covariances(0, keypoint_idx));
          landmarks.valid.at(keypoint_idx) = true;
        }
      }
    }
  }
}

#if 0
/// @brief Visualization implementation
void ImageTriangulationModule::visualizeImpl(
    QueryCache &qdata, MapCache &, const std::shared_ptr<const Graph> &,
    std::mutex &vis_mtx) {
  // check if visualization is enabled
  if (config_->visualize_features)
    visualize::showFeatures(vis_mtx, qdata, " features");

  // check if visualization is enabled
  if (config_->visualize_stereo_features)
    visualize::showStereoMatches(vis_mtx, qdata, " stereo features");
}
#endif

}  // namespace navigation
}  // namespace asrl
