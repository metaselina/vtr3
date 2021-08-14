#pragma once

#include <memory>
#include <unordered_set>

#include <vtr_lidar/cloud/cloud.h>
#include <vtr_lidar/nanoflann/nanoflann.hpp>

// KDTree type definition
using KDTree_Params = nanoflann::KDTreeSingleIndexAdaptorParams;
using PointXYZ_KDTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, PointCloud>, PointCloud, 3>;
using PointXYZ_Dynamic_KDTree = nanoflann::KDTreeSingleIndexDynamicAdaptor<
    nanoflann::L2_Simple_Adaptor<float, PointCloud>, PointCloud, 3>;

namespace {
// Simple utility function to combine hashtables
template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (hash_combine(seed, rest), ...);
}

}  // namespace

namespace vtr {
namespace lidar {

struct VoxKey {
  VoxKey(int x0 = 0, int y0 = 0, int z0 = 0) : x(x0), y(y0), z(z0) {}

  bool operator==(const VoxKey& other) const {
    return (x == other.x && y == other.y && z == other.z);
  }

  int x, y, z;
};

inline VoxKey operator+(const VoxKey A, const VoxKey B) {
  return VoxKey(A.x + B.x, A.y + B.y, A.z + B.z);
}

inline VoxKey operator-(const VoxKey A, const VoxKey B) {
  return VoxKey(A.x - B.x, A.y - B.y, A.z - B.z);
}

struct PixKey {
  PixKey(int x0 = 0, int y0 = 0) : x(x0), y(y0) {}

  bool operator==(const PixKey& other) const {
    return (x == other.x && y == other.y);
  }

  int x, y;
};

inline PixKey operator+(const PixKey A, const PixKey B) {
  return PixKey(A.x + B.x, A.y + B.y);
}

inline PixKey operator-(const PixKey A, const PixKey B) {
  return PixKey(A.x - B.x, A.y - B.y);
}

}  // namespace lidar
}  // namespace vtr

// Specialization of std:hash function
namespace std {
using namespace vtr::lidar;

template <>
struct hash<VoxKey> {
  std::size_t operator()(const VoxKey& k) const {
    std::size_t ret = 0;
    hash_combine(ret, k.x, k.y, k.z);
    return ret;
  }
};

template <>
struct hash<PixKey> {
  std::size_t operator()(const PixKey& k) const {
    std::size_t ret = 0;
    hash_combine(ret, k.x, k.y);
    return ret;
  }
};
}  // namespace std

namespace vtr {
namespace lidar {

class PointMap {
 public:
  /** \brief Constructors */
  PointMap(const float dl = 1.0f)
      : dl_(dl), tree(3, cloud, KDTree_Params(10 /* max leaf */)) {}

  /** \brief Size of the map (number of point/voxel in the map) */
  size_t size() { return cloud.pts.size(); }

  /** \brief Update map with a set of new points including movabilities. */
  void update(const std::vector<PointXYZ>& points,
              const std::vector<PointXYZ>& normals,
              const std::vector<float>& scores,
              const std::vector<std::pair<int, int>>& movabilities);

 private:
  VoxKey getKey(const PointXYZ& p) const {
    // Position of point in sample map
    PointXYZ p_pos = p / dl_;
    VoxKey k((int)floor(p_pos.x), (int)floor(p_pos.y), (int)floor(p_pos.z));
    return k;
  }

  void updateCapacity(size_t num_pts) {
    // Reserve new space if needed
    if (samples.empty()) samples.reserve(10 * num_pts);
    if (cloud.pts.capacity() < cloud.pts.size() + num_pts) {
      cloud.pts.reserve(cloud.pts.capacity() + num_pts);
      normals.reserve(normals.capacity() + num_pts);
      scores.reserve(scores.capacity() + num_pts);
      observations.reserve(observations.capacity() + num_pts);
      movabilities.reserve(movabilities.capacity() + num_pts);
    }
  }

  /** \brief Initialize a voxel centroid */
  void initSample(const VoxKey& k, const PointXYZ& p, const PointXYZ& n,
                  const float& s, const std::pair<int, int>& m) {
    // We place a new key in the hashmap
    samples.emplace(k, cloud.pts.size());

    // We add new voxel data but initiate only the centroid
    cloud.pts.push_back(p);
    normals.push_back(n);
    scores.push_back(s);
    observations.push_back(1);
    movabilities.push_back(m);
  }

  // Update of voxel centroid
  void updateSample(const size_t idx, const PointXYZ&, const PointXYZ& n,
                    const float& s, const std::pair<int, int>& m) {
    // Update number of observations of this point
    observations[idx]++;
    // Update normal if we have a clear view of it and closer distance (see
    // computation of score)
    if (s > scores[idx]) {
      scores[idx] = s;
      normals[idx] = n;
    }
    // Update movability if we have more observations of this point
    if (m.second > movabilities[idx].second) {
      movabilities[idx].first = m.first;
      movabilities[idx].second = m.second;
    }
  }

  void updateLimits(const VoxKey& k) {
    if (k.x < min_vox_.x) min_vox_.x = k.x;
    if (k.y < min_vox_.y) min_vox_.y = k.y;
    if (k.z < min_vox_.z) min_vox_.z = k.z;

    if (k.x > max_vox_.x) max_vox_.x = k.x;
    if (k.y > max_vox_.y) max_vox_.y = k.y;
    if (k.z > max_vox_.z) max_vox_.z = k.z;
  }

 private:
  // Voxel size
  float dl_ = 1.0f;

  // Map limits
  VoxKey max_vox_ =
      VoxKey(std::numeric_limits<int>::min(), std::numeric_limits<int>::min(),
             std::numeric_limits<int>::min());
  VoxKey min_vox_ =
      VoxKey(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
             std::numeric_limits<int>::max());

 public:
  // Containers for the data
  PointCloud cloud;
  std::vector<PointXYZ> normals;
  std::vector<float> scores;
  std::vector<int> observations;
  std::vector<std::pair<int, int>> movabilities;  // dynamic obs, total obs

  // Sparse hashmap that contain voxels (each voxel data is in the contiguous
  // vector containers)
  std::unordered_map<VoxKey, size_t> samples;

  // KDTree for neighbors query
  PointXYZ_Dynamic_KDTree tree;

  int number_of_updates = 0;

  friend class PointMapMigrator;
};

class PointMapMigrator {
 public:
  /** \brief Constructors \todo also need to get the transformation!*/
  PointMapMigrator(const Eigen::Matrix4d& T_on, const PointMap& old_map,
                   PointMap& new_map)
      : C_on_(T_on.block<3, 3>(0, 0).cast<float>()),
        r_no_ino_(T_on.block<3, 1>(0, 3).cast<float>()),
        old_map_(old_map),
        new_map_(new_map) {}

  /** \brief Update map with a set of new points in new map frame */
  void update(const std::vector<PointXYZ>& points,
              const std::vector<PointXYZ>& normals,
              const std::vector<float>& scores,
              const std::vector<std::pair<int, int>>& movabilities);

 private:
  const Eigen::Matrix3f C_on_;
  const Eigen::Vector3f r_no_ino_;
  const PointMap& old_map_;
  PointMap& new_map_;
};

class PointMapBase {
 public:
  PointMapBase(const float dl)
      : dl_(dl), tree(3, cloud, KDTree_Params(10 /* max leaf */)) {}

  /** \brief Size of the map (number of point/voxel in the map) */
  size_t size() { return cloud.pts.size(); }

 protected:
  VoxKey getKey(const PointXYZ& p) const {
    // Position of point in sample map
    PointXYZ p_pos = p / dl_;
    VoxKey k((int)floor(p_pos.x), (int)floor(p_pos.y), (int)floor(p_pos.z));
    return k;
  }

  virtual void updateCapacity(size_t num_pts) {
    // Reserve new space if needed
    if (samples.empty()) samples.reserve(10 * num_pts);
    if (cloud.pts.capacity() < cloud.pts.size() + num_pts) {
      cloud.pts.reserve(cloud.pts.capacity() + num_pts);
      normals.reserve(normals.capacity() + num_pts);
      normal_scores.reserve(normal_scores.capacity() + num_pts);
    }
  }

  /** \brief Initialize a voxel centroid */
  virtual void initSample(const VoxKey& k, const PointXYZ& p, const PointXYZ& n,
                          const float& s) {
    // We place a new key in the hashmap
    samples.emplace(k, cloud.pts.size());

    // We add new voxel data but initiate only the centroid
    cloud.pts.push_back(p);
    normals.push_back(n);
    normal_scores.push_back(s);
  }

  // Update of voxel centroid
  virtual void updateSample(const size_t idx, const PointXYZ&,
                            const PointXYZ& n, const float& s) {
    // Update normal if we have a clear view of it and closer distance (see
    // computation of score)
    if (s > normal_scores[idx]) {
      normals[idx] = n;
      normal_scores[idx] = s;
    }
  }

 private:
  /** \brief Voxel size */
  float dl_;

 public:
  // Containers for the data
  PointCloud cloud;
  std::vector<PointXYZ> normals;
  std::vector<float> normal_scores;

  /** \brief Sparse hashmap that contain voxels and map to point indices */
  std::unordered_map<VoxKey, size_t> samples;

  /** \brief KDTree for neighbors query */
  PointXYZ_Dynamic_KDTree tree;
};

/** \brief Point cloud map used for odometry and estimating movabilities. */
class IncrementalPointMap : public PointMapBase {
 public:
  IncrementalPointMap(const float dl) : PointMapBase(dl) {}

  /** \brief Update map with a set of new points including movabilities. */
  void update(const std::vector<PointXYZ>& points,
              const std::vector<PointXYZ>& normals,
              const std::vector<float>& normal_scores,
              const std::vector<std::pair<int, int>>& movabilities);

 protected:
  void updateCapacity(size_t num_pts) {
    // Reserve new space if needed
    PointMapBase::updateCapacity(num_pts);
    if (movabilities.capacity() < movabilities.size() + num_pts)
      movabilities.reserve(movabilities.capacity() + num_pts);
  }

  /** \brief Initialize a voxel centroid */
  void initSample(const VoxKey& k, const PointXYZ& p, const PointXYZ& n,
                  const float& s, const std::pair<int, int>& m) {
    PointMapBase::initSample(k, p, n, s);
    movabilities.push_back(m);
  }

  // Update of voxel centroid
  void updateSample(const size_t idx, const PointXYZ& p, const PointXYZ& n,
                    const float& s, const std::pair<int, int>& m) {
    PointMapBase::updateSample(idx, p, n, s);
    // Update movability if we have more observations of this point
    if (m.second > movabilities[idx].second) {
      movabilities[idx].first = m.first;
      movabilities[idx].second = m.second;
    }
  }

 public:
  /** \brief Number of time this map has been updated */
  int number_of_updates = 0;

  // Containers for the data
  std::vector<std::pair<int, int>> movabilities;  // dynamic obs, total obs

  friend class IncrementalPointMapMigrator;
};

class IncrementalPointMapMigrator {
 public:
  /** \brief Constructors \todo also need to get the transformation!*/
  IncrementalPointMapMigrator(const Eigen::Matrix4d& T_on,
                              const IncrementalPointMap& old_map,
                              IncrementalPointMap& new_map)
      : C_on_(T_on.block<3, 3>(0, 0).cast<float>()),
        r_no_ino_(T_on.block<3, 1>(0, 3).cast<float>()),
        old_map_(old_map),
        new_map_(new_map) {}

  /** \brief Update map with a set of new points in new map frame */
  void update(const std::vector<PointXYZ>& points,
              const std::vector<PointXYZ>& normals,
              const std::vector<float>& scores,
              const std::vector<std::pair<int, int>>& movabilities);

 private:
  const Eigen::Matrix3f C_on_;
  const Eigen::Vector3f r_no_ino_;
  const IncrementalPointMap& old_map_;
  IncrementalPointMap& new_map_;
};

/** \brief Point cloud map that merges maps from within a single experience. */
class SingleExpPointMap : public PointMapBase {
 public:
  SingleExpPointMap(const float dl) : PointMapBase(dl) {}

  /** \brief Update map with a set of new points including movabilities. */
  void update(const std::vector<PointXYZ>& points,
              const std::vector<PointXYZ>& normals,
              const std::vector<float>& normal_scores,
              const std::vector<std::pair<int, int>>& movabilities);

  void buildKDTree() {
    if (tree_built_) throw std::runtime_error{"KDTree has already been built!"};
    this->tree.addPoints(0, this->cloud.pts.size() - 1);
    tree_built_ = true;
  }

 private:
  bool tree_built_ = false;
};

/** \brief Point cloud map that merges maps from multiple experiences. */
class MultiExpPointMap : public PointMapBase {
 public:
  MultiExpPointMap(const float dl) : PointMapBase(dl) {}

  /** \brief Update map with a set of single experience maps. */
  void update(
      const std::unordered_map<uint32_t /* RunIdType */,
                               std::shared_ptr<vtr::lidar::SingleExpPointMap>>&
          single_exp_maps);

  void buildKDTree() {
    if (tree_built_) throw std::runtime_error{"KDTree has already been built!"};
    this->tree.addPoints(0, this->cloud.pts.size() - 1);
    tree_built_ = true;
  }

 protected:
  void updateCapacity(size_t num_pts) {
    // Reserve new space if needed
    PointMapBase::updateCapacity(num_pts);
    if (observations.capacity() < observations.size() + num_pts) {
      observations.reserve(observations.capacity() + num_pts);
      experiences.reserve(experiences.capacity() + num_pts);
    }
  }

  /** \brief Initialize a voxel centroid */
  void initSample(const VoxKey& k, const PointXYZ& p, const PointXYZ& n,
                  const float& s, const uint32_t& e) {
    PointMapBase::initSample(k, p, n, s);
    observations.push_back(1);
    single_exp_obs_updated_.insert(k);
    experiences.push_back(e);
  }

  // Update of voxel centroid
  void updateSample(const VoxKey& k, const size_t idx, const PointXYZ& p,
                    const PointXYZ& n, const float& s, const uint32_t& e) {
    PointMapBase::updateSample(idx, p, n, s);
    // Update number of observations of this point (only if not already updated
    // for this experience)
    if (single_exp_obs_updated_.count(k) == 0) {
      observations[idx]++;
      single_exp_obs_updated_.insert(k);
    }
    // Choose experience with smaller number
    if (e < experiences[idx]) experiences[idx] = e;
  }

 private:
  bool tree_built_ = false;

  std::unordered_set<VoxKey> single_exp_obs_updated_;

 public:
  /** \brief Number of experiences in this map */
  int number_of_experiences = 0;

  // Containers for the data
  std::vector<int> observations;      /// number of observations of the point
  std::vector<uint32_t> experiences;  /// which experience is this point from
};

}  // namespace lidar
}  // namespace vtr