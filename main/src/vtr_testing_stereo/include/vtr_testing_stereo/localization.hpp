#pragma once

#include <vtr_testing_stereo/offline.hpp>

#include <vtr_common/timing/time_utils.hpp>
#include <vtr_common/utils/filesystem.hpp>
#include <vtr_logging/logging_init.hpp>
#include <vtr_navigation/navigator.hpp>
#include <vtr_pose_graph/evaluator/common.hpp>

using namespace vtr::common;
using namespace vtr::logging;
using namespace vtr::navigation;
using namespace vtr::pose_graph;

using LocEvaluator = eval::Mask::Privileged<RCGraph>::Caching;

class LocalizationNavigator : public OfflineNavigator {
 public:
  LocalizationNavigator(const rclcpp::Node::SharedPtr node,
                        std::string output_dir)
      : OfflineNavigator(node, output_dir) {
    // Get the path that we should repeat
    VertexId::Vector sequence;
    sequence.reserve(graph_->numberOfVertices());
    LOG(INFO) << "Number of Vertices: " << graph_->numberOfVertices();
    // Extract the privileged sub graph from the full graph.
    LocEvaluator::Ptr evaluator(new LocEvaluator());
    evaluator->setGraph(graph_.get());
    auto privileged_path = graph_->getSubgraph(0ul, evaluator);
    for (auto it = privileged_path->begin(0ul); it != privileged_path->end();
         ++it) {
      LOG(INFO) << it->v()->id();
      sequence.push_back(it->v()->id());
    }

    tactic_->setPath(sequence);

    // normally the state machine would add a run when a goal is started. We
    // spoof that here.
    tactic_->addRun();

    // Create a branch pipeline.
    tactic_->setPipeline(tactic::PipelineMode::Following);

    // setup output of results to CSV
    std::stringstream ss;
    ss << "results_run_" << std::setfill('0') << std::setw(6)
       << (graph_->numberOfRuns() - 1);
    auto run_results_dir = fs::path(fs::path(output_dir_) / ss.str());
    fs::create_directories(run_results_dir);
    std::string loc_results_filename =
        node_->declare_parameter<std::string>("loc_results_filename", "loc.csv");
    loc_outstream_.open(run_results_dir / loc_results_filename);
    loc_outstream_ << "timestamp,query run,query vertex,map run,map vertex,success,r,,,T_q_m\n";
  }

  ~LocalizationNavigator() { saveLoc(); }

 private:

  void saveLoc() {
    lgmath::se3::TransformationWithCovariance T_q_m(true);

    // get vertices from latest run
    auto root_vid = navigation::VertexId(graph_->numberOfRuns() - 1, 0);
    TemporalEvaluator::Ptr evaluator(new TemporalEvaluator());
    evaluator->setGraph((void *)graph_.get());
    auto path_itr = graph_->beginDfs(root_vid, 0, evaluator);

    for (; path_itr != graph_->end(); ++path_itr) {
      auto &v = path_itr->v();
      LOG(INFO) << "v " << v->id();
      v->load("results_localization");
      LOG(INFO) << "LOADED " << v->id();

      auto loc_msg =
          v->retrieveKeyframeData<vtr_messages::msg::LocalizationStatus>(
              "results_localization",
              true);
      LOG(INFO) << "RETRIEVED " << v->id();

      if (loc_msg != nullptr && loc_msg->success) {
        uint64 q_id_64 = loc_msg->query_id;
        uint q_id_minor = (uint)q_id_64;
        uint q_id_major = (uint)(q_id_64 >> 32);

        uint64 m_id_64 = loc_msg->map_id;
        uint m_id_minor = (uint)m_id_64;
        uint m_id_major = (uint)(m_id_64 >> 32);

        LOG(INFO) << q_id_major << "-" << q_id_minor << ", " << m_id_major
                  << "-" << m_id_minor;

        loc_msg->t_query_map >> T_q_m;

        std::streamsize prec = loc_outstream_.precision();
        loc_outstream_
            << std::setprecision(21)
            << (path_itr->v()->keyFrameTime().nanoseconds_since_epoch) / 1e9
            << std::setprecision(prec) << "," << q_id_major << "," << q_id_minor
            << "," << m_id_major << "," << m_id_minor << ","
            << loc_msg->success;

        // flatten r vector to save
        auto tmp = T_q_m.r_ba_ina();
        auto r_flat = std::vector<double>(tmp.data(), tmp.data() + 3);
        for (auto element : r_flat) loc_outstream_ << "," << element;

        // also save whole T_q_m matrix
        auto tmp2 = T_q_m.matrix();
        auto T_flat = std::vector<double>(tmp2.data(), tmp2.data() + 16);
        for (auto element : T_flat) loc_outstream_ << "," << element;

        loc_outstream_ << "\n";
      }
    }
    loc_outstream_.close();
  }

  /** \brief Stream to save position from localization to csv */
  std::ofstream loc_outstream_;
};