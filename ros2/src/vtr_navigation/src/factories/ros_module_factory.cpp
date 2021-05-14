#include <vtr_navigation/factories/module_factory.hpp>
#include <vtr_navigation/factories/ros_module_factory.hpp>
#include <vtr_navigation/modules.hpp>

namespace vtr {
namespace navigation {

ROSModuleFactory::mod_ptr ROSModuleFactory::make_str(
    const std::string &type_str) const {
  // Right now, just call the default factory...
  auto new_module = ModuleFactory(type_str).make();
  if (!new_module) return new_module;
  configureModule(new_module, type_str);
  return new_module;
}

void ROSModuleFactory::configureModule(std::shared_ptr<BaseModule> &new_module,
                                       const std::string &type_str) const {
  if (isType<ConversionExtractionModule>(type_str))
    configureConversionExtractor(new_module);
  else if (isType<ImageTriangulationModule>(type_str))
    configureImageTriangulator(new_module);
  else if (isType<LandmarkRecallModule>(type_str))
    configureLandmarkRecallModule(new_module);
  else if (isType<ASRLStereoMatcherModule>(type_str))
    configureASRLStereoMatcher(new_module);
  else if (isType<StereoRansacModule>(type_str))
    configureStereoRANSAC(new_module);
  else if (isType<KeyframeOptimizationModule>(type_str))
    configureKeyframeOptimization(new_module);
  else if (isType<SimpleVertexTestModule>(type_str))
    configureSimpleVertexCreationTestModule(new_module);
  else if (isType<WindowedRecallModule>(type_str))
    configureWindowedRecallModule(new_module);
  else if (isType<WindowOptimizationModule>(type_str))
    configureWindowOptimization(new_module);
  else if (isType<LandmarkMigrationModule>(type_str))
    configureLandmarkMigration(new_module);
  else if (isType<SubMapExtractionModule>(type_str))
    configureSubMapExtraction(new_module);
  else if (isType<MelMatcherModule>(type_str))
    configureMelMatcher(new_module);
#if false
  else if (isType<ResultsModule>(type_str))
    configureResults(new_module);
#endif
  else if (isType<CollaborativeLandmarksModule>(type_str))
    configureCollabLandmarks(new_module);
  else if (isType<ExperienceTriageModule>(type_str))
    configureExperienceTriage(new_module);
  else if (isType<RandomExperiencesModule>(type_str))
    configureRandomExperiences(new_module);
  else if (isType<TodRecognitionModule>(type_str))
    configureTodRecog(new_module);
  else if (isType<MelRecognitionModule>(type_str))
    configureMelRecog(new_module);
  else
    throw std::runtime_error("Cannot configure requested module.");
#if false
  /*
  if (isType<NoopModule>(type_str)) {
  } else if (isType<StereoRansacModule>(type_str)) {
    configureStereoRANSAC(new_module);
  } else if (isType<MonoRansacModule>(type_str)) {
    configureMonoRANSAC(new_module);
  } else if (isType<InitMonoRansacModule>(type_str)) {
    configureInitMonoRANSAC(new_module);
  } else if (isType<OpenCVStereoMatcherModule>(type_str)) {
    configureOpenCVStereoMatcher(new_module);
  } else if (isType<ASRLMonoMatcherModule>(type_str)) {
    configureASRLMonoMatcher(new_module);
  } else if (isType<ASRLStereoMatcherModule>(type_str)) {
    configureASRLStereoMatcher(new_module);
  } else if (isType<KeyframeOptimizationModule>(type_str)) {
    configureKeyframeOptimization(new_module);
  } else if (isType<FeatureExtractionModule>(type_str)) {
    configureFeatureExtractor(new_module);
  } else if (isType<ImageConversionModule>(type_str)) {
    configureImageConverter(new_module);
  } else if (isType<ConversionExtractionModule>(type_str)) {
    configureConversionExtractor(new_module);
  } else if (isType<CVStereoBMModule>(type_str)) {
    configureCVStereoBM(new_module);
  } else if (isType<CVGpuStereoBMModule>(type_str)) {
    configureCVGpuStereoBM(new_module);
  } else if (isType<ElasModule>(type_str)) {
    configureElas(new_module);
  } else if (isType<CVReprojectorModule>(type_str)) {
    configureCVReprojector(new_module);
  } else if (isType<CVGpuReprojectorModule>(type_str)) {
    configureCVGpuReprojector(new_module);
  } else if (isType<ImageTriangulationModule>(type_str)) {
    configureImageTriangulator(new_module);
  } else if (isType<MonoTriangulationModule>(type_str)) {
    configureMonoTriangulator(new_module);
  } else if (isType<SequentialTriangulationModule>(type_str)) {
    configureSequentialTriangulator(new_module);
  } else if (isType<SimpleVertexTestModule>(type_str)) {
    configureSimpleVertexCreationTestModule(new_module);
  } else if (isType<LancasterVertexTestModule>(type_str)) {
    configureLancasterVertexCreationTestModule(new_module);
  } else if (isType<GimbalVertexTestModule>(type_str)) {
    configureGimbalVertexCreationTestModule(new_module);
  } else if (isType<LandmarkRecallModule>(type_str)) {
    configureLandmarkRecallModule(new_module);
  } else if (isType<WindowedRecallModule>(type_str)) {
    configureWindowedRecallModule(new_module);
  } else if (isType<WindowOptimizationModule>(type_str)) {
    configureWindowOptimization(new_module);
  } else if (isType<SubMapExtractionModule>(type_str)) {
    configureSubMapExtraction(new_module);
  } else if (isType<LandmarkMigrationModule>(type_str)) {
    configureLandmarkMigration(new_module);
  } else if (isType<MelMatcherModule>(type_str)) {
    configureMelMatcher(new_module);
  } else if (isType<MelRecognitionModule>(type_str)) {
    configureMelRecog(new_module);
  } else if (isType<TodRecognitionModule>(type_str)) {
    configureTodRecog(new_module);
  } else if (isType<CollaborativeLandmarksModule>(type_str)) {
    configureCollabLandmarks(new_module);
  } else if (isType<RandomExperiencesModule>(type_str)) {
    configureRandomExperiences(new_module);
  } else if (isType<ExperienceTriageModule>(type_str)) {
    configureExperienceTriage(new_module);
  } else if (isType<ResultsModule>(type_str)) {
    configureResults(new_module);
  } else if (isType<QuickVORosPublisherModule>(type_str)) {
    configureQuickVORosPublisher(new_module);
  } else if (isType<RefinedVORosPublisherModule>(type_str)) {
    configureRefinedVORosPublisher(new_module);
  } else if (isType<LocalizationRosPublisherModule>(type_str)) {
    configureLocalizationRosPublisher(new_module);
  } else if (isType<MonoPlanarScalingModule>(type_str)) {
    configureMonoScaling(new_module);
  } else if (isType<UnderfootSeparateModule>(type_str)) {
    configureUnderfootSeparate(new_module);
  } else if (isType<UnderfootAggregateModule>(type_str)) {
    configureUnderfootAggregate(new_module);
  } else if (isType<LookaheadPatchGenerationModule>(type_str)) {
    configureLookaheadPatchGeneration(new_module);
  } else if (isType<CDMaxMinModule>(type_str)) {
    configureCDMaxMin(new_module);
  } else if (isType<CDMinMaxModule>(type_str)) {
    configureCDMinMax(new_module);
  } else if (isType<CDGmmModule>(type_str)) {
    configureCDGmm(new_module);
  } else if (isType<CDGpcModule>(type_str)) {
    configureCDGpc(new_module);
  } else if (isType<TrainingModule>(type_str)) {
    configureTraining(new_module);
  }
  */
#endif
}

void ROSModuleFactory::configureORBDetector(
    vision::ORBConfiguration &config) const {
  // clang-format off
  config.use_STAR_detector_  = node_->declare_parameter<bool>(param_prefix_ + ".extractor.orb.use_STAR_detector", true);
  config.use_GPU_descriptors_  = node_->declare_parameter<bool>(param_prefix_ + ".extractor.orb.use_GPU_descriptors", false);
  config.STAR_maxSize_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.STAR_maxSize", 5);
  config.STAR_responseThreshold_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.STAR_responseThreshold", 10);
  config.STAR_lineThresholdProjected_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.STAR_lineThresholdProjected", 10);
  config.STAR_lineThresholdBinarized_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.STAR_lineThresholdBinarized", 8);
  config.STAR_suppressNonmaxSize_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.STAR_suppressNonmaxSize", 5);
  config.num_detector_features_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.num_detector_features", 7000);
  config.num_binned_features_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.num_binned_features", 800);
  config.scaleFactor_  = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.scaleFactor", 1.2);
  config.nlevels_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.nlevels", 8);
  config.edgeThreshold_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.edgeThreshold", 31);
  config.firstLevel_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.firstLevel", 0);
  config.WTA_K_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.WTA_K", 2);
  config.upright_  = node_->declare_parameter<bool>(param_prefix_ + ".extractor.orb.upright_flag", false);
  config.num_threads_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.num_threads", 8);

  auto scoreType = node_->declare_parameter<std::string>(param_prefix_ + ".extractor.orb.scoreType", "HARRIS");
  if (scoreType == "HARRIS")
    config.scoreType_ = cv::ORB::HARRIS_SCORE;
  else if (scoreType == "FAST")
    config.scoreType_ = cv::ORB::FAST_SCORE;

  config.patchSize_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.patchSize", 64); // \todo: 64 gives an error in cuda::ORB, max 59
  config.fastThreshold_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.fastThreshold", 20);
  config.x_bins_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.x_bins", 3);
  config.y_bins_  = node_->declare_parameter<int>(param_prefix_ + ".extractor.orb.y_bins", 2);
  config.stereo_matcher_config_.descriptor_match_thresh_ = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.matcher.descriptor_match_thresh", 0.55);
  config.stereo_matcher_config_.stereo_descriptor_match_thresh_ = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.matcher.stereo_descriptor_match_thresh", 0.55);
  config.stereo_matcher_config_.stereo_y_tolerance_ = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.matcher.stereo_y_tolerance", 1.0f);
  config.stereo_matcher_config_.stereo_x_tolerance_min_  = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.matcher.stereo_x_tolerance_min", 0);
  config.stereo_matcher_config_.stereo_x_tolerance_max_  = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.matcher.stereo_x_tolerance_max", 16);
  config.stereo_matcher_config_.check_octave_  = node_->declare_parameter<bool>(param_prefix_ + ".extractor.orb.matcher.check_octave", true);
  config.stereo_matcher_config_.check_response_  = node_->declare_parameter<bool>(param_prefix_ + ".extractor.orb.matcher.check_response", true);
  config.stereo_matcher_config_.min_response_ratio_  = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.matcher.min_response_ratio", 0.1);
  config.stereo_matcher_config_.scale_x_tolerance_by_y_  = node_->declare_parameter<bool>(param_prefix_ + ".extractor.orb.matcher.scale_x_tolerance_by_y", true);
  config.stereo_matcher_config_.x_tolerance_scale_ = node_->declare_parameter<double>(param_prefix_ + ".extractor.orb.matcher.x_tolerance_scale", 768);
  // clang-format on
}

#if GPUSURF_ENABLED
void ROSModuleFactory::configureSURFDetector(
    asrl::GpuSurfConfiguration &config) const {
  // clang-format off
  config.threshold = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.threshold", 1e-7);
  config.upright_flag = node_->declare_parameter<bool>(param_prefix_ + ".extractor.surf.upright_flag", true);
#ifdef DETERMINISTIC_VTR
  LOG_IF(config.upright_flag, WARNING) << "SURF upright flag set to FALSE in deterministic mode.";
  config.upright_flag = false;
#endif
  config.nOctaves = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.nOctaves", 4);
  config.nIntervals = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.nIntervals", 4);
  config.initialScale = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.initialScale", 1.5);
  config.edgeScale = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.edgeScale", 1.5);
  config.l1 = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.l1", 3.f / 1.5f);
  config.l2 = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.l2", 5.f / 1.5f);
  config.l3 = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.l3", 3.f / 1.5f);
  config.l4 = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.l4", 1.f / 1.5f);
  config.initialStep = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.initialStep", 1);
  config.targetFeatures = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.targetFeatures", 800);
  config.detector_threads_x = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.detector_threads_x", 16);
  config.detector_threads_y = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.detector_threads_y", 4);
  config.nonmax_threads_x = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.nonmax_threads_x", 16);
  config.nonmax_threads_y = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.nonmax_threads_y", 16);
  config.regions_horizontal = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.regions_horizontal", 8);
  config.regions_vertical = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.regions_vertical", 6);
  config.regions_target = node_->declare_parameter<int>(param_prefix_ + ".extractor.surf.regions_target", 800);
  // clang-format on
}

void ROSModuleFactory::configureSURFStereoDetector(
    asrl::GpuSurfStereoConfiguration &config) const {
  // clang-format off
  config.stereoDisparityMinimum = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.stereoDisparityMinimum", 0.0);
  config.stereoDisparityMaximum = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.stereoDisparityMaximum", 120.0);
  config.stereoCorrelationThreshold = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.stereoCorrelationThreshold", 0.79);
  config.stereoYTolerance = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.stereoYTolerance", 1.0);
  config.stereoScaleTolerance = node_->declare_parameter<double>(param_prefix_ + ".extractor.surf.stereoScaleTolerance", 0.8);
  // clang-format on
}
#endif

void ROSModuleFactory::configureConversionExtractor(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<ConversionExtractionModule::Config> config;
  /// config.reset(new ConversionExtractionModule::Config());
  auto config = std::make_shared<ConversionExtractionModule::Config>();
  config->conversions = node_->declare_parameter<decltype(config->conversions)>(param_prefix_ + ".conversions", config->conversions);
  config->color_constant_weights = node_->declare_parameter<decltype(config->color_constant_weights)>(param_prefix_ + ".color_constant.weights", config->color_constant_weights);
  config->color_constant_histogram_equalization = node_->declare_parameter<decltype(config->color_constant_histogram_equalization)>(param_prefix_ + ".color_constant.histogram_equalization", config->color_constant_histogram_equalization);
  config->feature_type = node_->declare_parameter<decltype(config->feature_type)>(param_prefix_ + ".extractor.type", config->feature_type);
  config->visualize_raw_features = node_->declare_parameter<decltype(config->visualize_raw_features)>(param_prefix_ + ".extractor.visualize_raw_features", config->visualize_raw_features);
  // clang-format on
  // configure the detector
  if (config->feature_type == "OPENCV_ORB") {
    configureORBDetector(config->opencv_orb_params);
  } else if (config->feature_type == "ASRL_GPU_SURF") {
#if GPUSURF_ENABLED
    configureSURFDetector(config->gpu_surf_params);
    configureSURFStereoDetector(config->gpu_surf_stereo_params);
    config->gpu_surf_stereo_params.threshold = config->gpu_surf_params.threshold;
    config->gpu_surf_stereo_params.upright_flag = config->gpu_surf_params.upright_flag;
    config->gpu_surf_stereo_params.initialScale = config->gpu_surf_params.initialScale;
    config->gpu_surf_stereo_params.edgeScale = config->gpu_surf_params.edgeScale;
    config->gpu_surf_stereo_params.detector_threads_x = config->gpu_surf_params.detector_threads_x;
    config->gpu_surf_stereo_params.detector_threads_y = config->gpu_surf_params.detector_threads_y;
    config->gpu_surf_stereo_params.regions_horizontal = config->gpu_surf_params.regions_horizontal;
    config->gpu_surf_stereo_params.regions_vertical = config->gpu_surf_params.regions_vertical;
    config->gpu_surf_stereo_params.regions_target = config->gpu_surf_params.regions_target;
#else
    throw std::runtime_error(
        "ROSModuleFactory::configureFeatureExtractor: GPU SURF isn't enabled!");
#endif
  } else {
    throw std::runtime_error(
        "Couldn't determine feature type when building ConversionExtraction "
        "Module!");
  }

  std::dynamic_pointer_cast<ConversionExtractionModule>(new_module)
      ->setConfig(config);
}
#if false
#if 0
void ROSModuleFactory::configureFeatureExtractor(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<FeatureExtractionModule::Config> config;
  config.reset(new FeatureExtractionModule::Config());

  nh_->param<std::string>(param_prefix_ + "extractor/type",
                          config->feature_type, "ASRL_GPU_SURF");
  nh_->param<bool>(param_prefix_ + "extractor/visualize_raw_features",
                   config->visualize_raw_features, false);
  nh_->getParam(param_prefix_ + "extractor/channels", config->channels);

  // configure the detector
  if (config->feature_type == "OPENCV_ORB") {
    configureORBDetector(config->opencv_orb_params);
  } else if (config->feature_type == "ASRL_GPU_SURF") {
#if GPUSURF_ENABLED
    configureSURFDetector(config->gpu_surf_params);
    configureSURFStereoDetector(config->gpu_surf_stereo_params);
#else
    throw std::runtime_error(
        "ROSModuleFactory::configureFeatureExtractor: GPU SURF isn't enabled!");
#endif
  } else {
    throw std::runtime_error(
        "Couldn't determine feature type when building ConversionExtraction "
        "Module!");
  }

  std::dynamic_pointer_cast<FeatureExtractionModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureImageConverter(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<ImageConversionModule::Config> config;
  config.reset(new ImageConversionModule::Config());

  nh_->getParam(param_prefix_ + "/conversions", config->conversions);
  nh_->getParam(param_prefix_ + "/color_constant/weights",
                config->color_constant_weights);
  nh_->getParam(param_prefix_ + "/color_constant/histogram_equalization",
                config->color_constant_histogram_equalization);
  std::dynamic_pointer_cast<ImageConversionModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureCVStereoBM(
    std::shared_ptr<BaseModule> &new_module) const {
  CVStereoBMModule::Config config;

  nh_->getParam(param_prefix_ + "/max_fps", config.max_fps);
  nh_->getParam(param_prefix_ + "/visualize", config.visualize);

  std::dynamic_pointer_cast<CVStereoBMModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureCVStereoSgbm(
    std::shared_ptr<BaseModule> &new_module) const {
  CVStereoSgbmModule::Config config;

  nh_->getParam(param_prefix_ + "/max_fps", config.max_fps);
  nh_->getParam(param_prefix_ + "/visualize", config.visualize);

  config.cv_stereo_sgbm_params =
      std::make_shared<asrl::vision::CVStereoSGBM::Params>();
  nh_->getParam(param_prefix_ + "/minDisparity",
                config.cv_stereo_sgbm_params->minDisparity);
  nh_->getParam(param_prefix_ + "/numberOfDisparities",
                config.cv_stereo_sgbm_params->numberOfDisparities);
  nh_->getParam(param_prefix_ + "/SADWindowSize",
                config.cv_stereo_sgbm_params->SADWindowSize);
  nh_->getParam(param_prefix_ + "/preFilterCap",
                config.cv_stereo_sgbm_params->preFilterCap);
  nh_->getParam(param_prefix_ + "/uniquenessRatio",
                config.cv_stereo_sgbm_params->uniquenessRatio);
  nh_->getParam(param_prefix_ + "/P1", config.cv_stereo_sgbm_params->P1);
  nh_->getParam(param_prefix_ + "/P2", config.cv_stereo_sgbm_params->P2);
  nh_->getParam(param_prefix_ + "/speckleWindowSize",
                config.cv_stereo_sgbm_params->speckleWindowSize);
  nh_->getParam(param_prefix_ + "/speckleRange",
                config.cv_stereo_sgbm_params->speckleRange);
  nh_->getParam(param_prefix_ + "/disp12MaxDiff",
                config.cv_stereo_sgbm_params->disp12MaxDiff);
  nh_->getParam(param_prefix_ + "/fullDP",
                config.cv_stereo_sgbm_params->fullDP);
  nh_->getParam(param_prefix_ + "/use_left_right_consistency",
                config.cv_stereo_sgbm_params->use_left_right_consistency);
  nh_->getParam(param_prefix_ + "/left_right_tolerance",
                config.cv_stereo_sgbm_params->left_right_tolerance);

  std::dynamic_pointer_cast<CVStereoSgbmModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureCVGpuStereoBM(
    std::shared_ptr<BaseModule> &new_module) const {
  CVGpuStereoBMModule::Config config;

  nh_->getParam(param_prefix_ + "/max_fps", config.max_fps);
  nh_->getParam(param_prefix_ + "/visualize", config.visualize);

#if defined(HAVE_OPENCV_CUDASTEREO) || defined(HAVE_OPENCV_GPU)
  config.gpu_bm_params = std::make_shared<asrl::vision::CVGpuStereoBM::Params>();
  nh_->getParam(param_prefix_ + "/preset", config.gpu_bm_params->preset);
  nh_->getParam(param_prefix_ + "/ndisp", config.gpu_bm_params->ndisp);
  nh_->getParam(param_prefix_ + "/winSize", config.gpu_bm_params->winSize);
  nh_->getParam(param_prefix_ + "/avergeTexThreshold",
                config.gpu_bm_params->avergeTexThreshold);
  nh_->getParam(param_prefix_ + "/use_left_right_consistency",
                config.gpu_bm_params->use_left_right_consistency);
  nh_->getParam(param_prefix_ + "/left_right_tolerance",
                config.gpu_bm_params->left_right_tolerance);

  nh_->getParam(param_prefix_ + "/postfilters/gpu_bilateral_filter/k",
                config.gpu_bilateral_filter_config.k);
  nh_->getParam(param_prefix_ + "/postfilters/gpu_bilateral_filter/sigma_color",
                config.gpu_bilateral_filter_config.sigma_color);
  nh_->getParam(
      param_prefix_ + "/postfilters/gpu_bilateral_filter/sigma_spatial",
      config.gpu_bilateral_filter_config.sigma_spatial);
#endif

  std::dynamic_pointer_cast<CVGpuStereoBMModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureElas(
    std::shared_ptr<BaseModule> &new_module) const {
  ElasModule::Config config;

  nh_->getParam(param_prefix_ + "/max_fps", config.max_fps);
  nh_->getParam(param_prefix_ + "/visualize", config.visualize);
#if ELAS_ENABLED
  nh_->getParam(param_prefix_ + "/disp_min", config.elas_params.disp_min);
  nh_->getParam(param_prefix_ + "/disp_max", config.elas_params.disp_max);
  nh_->getParam(param_prefix_ + "/support_threshold",
                config.elas_params.support_threshold);
  nh_->getParam(param_prefix_ + "/support_texture",
                config.elas_params.support_texture);
  nh_->getParam(param_prefix_ + "/candidate_stepsize",
                config.elas_params.candidate_stepsize);
  nh_->getParam(param_prefix_ + "/incon_window_size",
                config.elas_params.incon_window_size);
  nh_->getParam(param_prefix_ + "/incon_threshold",
                config.elas_params.incon_threshold);
  nh_->getParam(param_prefix_ + "/incon_min_support",
                config.elas_params.incon_min_support);
  nh_->getParam(param_prefix_ + "/add_corners", config.elas_params.add_corners);
  nh_->getParam(param_prefix_ + "/grid_size", config.elas_params.grid_size);
  nh_->getParam(param_prefix_ + "/beta", config.elas_params.beta);
  nh_->getParam(param_prefix_ + "/gamma", config.elas_params.gamma);
  nh_->getParam(param_prefix_ + "/sigma", config.elas_params.sigma);
  nh_->getParam(param_prefix_ + "/sradius", config.elas_params.sradius);
  nh_->getParam(param_prefix_ + "/match_texture",
                config.elas_params.match_texture);
  nh_->getParam(param_prefix_ + "/lr_threshold",
                config.elas_params.lr_threshold);
  nh_->getParam(param_prefix_ + "/speckle_sim_threshold",
                config.elas_params.speckle_sim_threshold);
  nh_->getParam(param_prefix_ + "/speckle_size",
                config.elas_params.speckle_size);
  nh_->getParam(param_prefix_ + "/ipol_gap_width",
                config.elas_params.ipol_gap_width);
  nh_->getParam(param_prefix_ + "/filter_median",
                config.elas_params.filter_median);
  nh_->getParam(param_prefix_ + "/filter_adaptive_mean",
                config.elas_params.filter_adaptive_mean);
  nh_->getParam(param_prefix_ + "/postprocess_only_left",
                config.elas_params.postprocess_only_left);
  nh_->getParam(param_prefix_ + "/subsampling", config.elas_params.subsampling);
#endif
  std::dynamic_pointer_cast<ElasModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureCVReprojector(
    std::shared_ptr<BaseModule> &new_module) const {
  CVReprojectorModule::Config config;

  nh_->getParam(param_prefix_ + "/max_depth", config.max_depth);
  nh_->getParam(param_prefix_ + "/sample_rate", config.sample_rate);

  std::dynamic_pointer_cast<CVReprojectorModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureCVGpuReprojector(
    std::shared_ptr<BaseModule> &new_module) const {
  CVGpuReprojectorModule::Config config;

  nh_->getParam(param_prefix_ + "/max_depth", config.max_depth);
  nh_->getParam(param_prefix_ + "/sample_rate", config.sample_rate);

  std::dynamic_pointer_cast<CVGpuReprojectorModule>(new_module)
      ->setConfig(config);
}
#endif
#endif
void ROSModuleFactory::configureImageTriangulator(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<ImageTriangulationModule::Config> config;
  /// config.reset(new ImageTriangulationModule::Config());
  auto config = std::make_shared<ImageTriangulationModule::Config>();
  config->visualize_features = node_->declare_parameter<decltype(config->visualize_features)>( param_prefix_ + ".visualize_features", config->visualize_features);
  config->visualize_stereo_features = node_->declare_parameter<decltype(config->visualize_stereo_features)>( param_prefix_ + ".visualize_stereo_features", config->visualize_stereo_features);
  config->min_triangulation_depth = node_->declare_parameter<decltype(config->min_triangulation_depth)>( param_prefix_ + ".min_triangulation_depth", config->min_triangulation_depth);
  config->max_triangulation_depth = node_->declare_parameter<decltype(config->max_triangulation_depth)>( param_prefix_ + ".max_triangulation_depth", config->max_triangulation_depth);
  std::dynamic_pointer_cast<ImageTriangulationModule>(new_module)->setConfig(config);
  // clang-format on
}
#if false
#if 0
void ROSModuleFactory::configureMonoTriangulator(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<MonoTriangulationModule::Config> config;
  config.reset(new MonoTriangulationModule::Config());

  nh_->getParam(param_prefix_ + "visualize_features",
                config->visualize_features);
  nh_->getParam(param_prefix_ + "min_new_points", config->min_new_points);
  nh_->getParam(param_prefix_ + "min_new_points_init",
                config->min_new_points_init);
  nh_->getParam(param_prefix_ + "min_depth", config->min_depth);
  nh_->getParam(param_prefix_ + "max_depth", config->max_depth);
  nh_->getParam(param_prefix_ + "plane_distance", config->plane_distance);
  nh_->getParam(param_prefix_ + "max_reprojection_error",
                config->max_reprojection_error);

  std::dynamic_pointer_cast<MonoTriangulationModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureSequentialTriangulator(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<SequentialTriangulationModule::Config> config;
  config.reset(new SequentialTriangulationModule::Config());

  nh_->getParam(param_prefix_ + "visualize_features",
                config->visualize_features);
  nh_->getParam(param_prefix_ + "min_new_points", config->min_new_points);
  nh_->getParam(param_prefix_ + "min_depth", config->min_depth);
  nh_->getParam(param_prefix_ + "max_depth", config->max_depth);
  nh_->getParam(param_prefix_ + "max_reprojection_error",
                config->max_reprojection_error);

  std::dynamic_pointer_cast<SequentialTriangulationModule>(new_module)
      ->setConfig(config);
}
#endif
#endif
void ROSModuleFactory::configureSteam(
    std::shared_ptr<SteamModule::Config> &config) const {
  // clang-format off
  config->solver_type = node_->declare_parameter<decltype(config->solver_type)>(param_prefix_ + ".solver_type", config->solver_type);
  config->loss_function = node_->declare_parameter<decltype(config->loss_function)>(param_prefix_ + ".loss_function", config->loss_function);
  config->verbose = node_->declare_parameter<decltype(config->verbose)>(param_prefix_ + ".verbose", config->verbose);
  config->use_T_q_m_prior = node_->declare_parameter<decltype(config->use_T_q_m_prior)>(param_prefix_ + ".use_T_q_m_prior", config->use_T_q_m_prior);

  config->iterations = node_->declare_parameter<decltype(config->iterations)>(param_prefix_ + ".iterations", config->iterations);
  config->absoluteCostThreshold = node_->declare_parameter<decltype(config->absoluteCostThreshold)>(param_prefix_ + ".absoluteCostThreshold", config->absoluteCostThreshold);
  config->absoluteCostChangeThreshold = node_->declare_parameter<decltype(config->absoluteCostChangeThreshold)>(param_prefix_ + ".absoluteCostChangeThreshold", config->absoluteCostChangeThreshold);
  config->relativeCostChangeThreshold = node_->declare_parameter<decltype(config->relativeCostChangeThreshold)>(param_prefix_ + ".relativeCostChangeThreshold", config->relativeCostChangeThreshold);

  config->ratioThresholdShrink = node_->declare_parameter<decltype(config->ratioThresholdShrink)>(param_prefix_ + ".ratioThresholdShrink", config->ratioThresholdShrink);
  config->ratioThresholdGrow = node_->declare_parameter<decltype(config->ratioThresholdGrow)>(param_prefix_ + ".ratioThresholdGrow", config->ratioThresholdGrow);
  config->shrinkCoeff = node_->declare_parameter<decltype(config->shrinkCoeff)>(param_prefix_ + ".shrinkCoeff", config->shrinkCoeff);
  config->growCoeff = node_->declare_parameter<decltype(config->growCoeff)>(param_prefix_ + ".growCoeff", config->growCoeff);
  config->maxShrinkSteps = node_->declare_parameter<decltype(config->maxShrinkSteps)>(param_prefix_ + ".maxShrinkSteps", config->maxShrinkSteps);
  config->backtrackMultiplier = node_->declare_parameter<decltype(config->backtrackMultiplier)>(param_prefix_ + ".backtrackMultiplier", config->backtrackMultiplier);
  config->maxBacktrackSteps = node_->declare_parameter<decltype(config->maxBacktrackSteps)>(param_prefix_ + ".maxBacktrackSteps", config->maxBacktrackSteps);

  // validity checking
  config->perform_planarity_check = node_->declare_parameter<decltype(config->perform_planarity_check)>(param_prefix_ + ".perform_planarity_check", config->perform_planarity_check);
  config->plane_distance = node_->declare_parameter<decltype(config->plane_distance)>(param_prefix_ + ".plane_distance", config->plane_distance);
  config->min_point_depth = node_->declare_parameter<decltype(config->min_point_depth)>(param_prefix_ + ".min_point_depth", config->min_point_depth);
  config->max_point_depth = node_->declare_parameter<decltype(config->max_point_depth)>(param_prefix_ + ".max_point_depth", config->max_point_depth);

  // trajectory stuff.
  config->save_trajectory = node_->declare_parameter<decltype(config->save_trajectory)>(param_prefix_ + ".save_trajectory", config->save_trajectory);
  config->trajectory_smoothing = node_->declare_parameter<decltype(config->trajectory_smoothing)>(param_prefix_ + ".trajectory_smoothing", config->trajectory_smoothing);
  config->lin_acc_std_dev_x = node_->declare_parameter<decltype(config->lin_acc_std_dev_x)>(param_prefix_ + ".lin_acc_std_dev_x", config->lin_acc_std_dev_x);
  config->lin_acc_std_dev_y = node_->declare_parameter<decltype(config->lin_acc_std_dev_y)>(param_prefix_ + ".lin_acc_std_dev_y", config->lin_acc_std_dev_y);
  config->lin_acc_std_dev_z = node_->declare_parameter<decltype(config->lin_acc_std_dev_z)>(param_prefix_ + ".lin_acc_std_dev_z", config->lin_acc_std_dev_z);
  config->ang_acc_std_dev_x = node_->declare_parameter<decltype(config->ang_acc_std_dev_x)>(param_prefix_ + ".ang_acc_std_dev_x", config->ang_acc_std_dev_x);
  config->ang_acc_std_dev_y = node_->declare_parameter<decltype(config->ang_acc_std_dev_y)>(param_prefix_ + ".ang_acc_std_dev_y", config->ang_acc_std_dev_y);
  config->ang_acc_std_dev_z = node_->declare_parameter<decltype(config->ang_acc_std_dev_z)>(param_prefix_ + ".ang_acc_std_dev_z", config->ang_acc_std_dev_z);
  config->disable_solver = node_->declare_parameter<decltype(config->disable_solver)>(param_prefix_ + ".disable_solver", config->disable_solver);
  // velocity prior
  config->velocity_prior = node_->declare_parameter<decltype(config->velocity_prior)>(param_prefix_ + ".velocity_prior", config->velocity_prior);
  config->lin_vel_mean_x = node_->declare_parameter<decltype(config->lin_vel_mean_x)>(param_prefix_ + ".lin_vel_mean_x", config->lin_vel_mean_x);
  config->lin_vel_mean_y = node_->declare_parameter<decltype(config->lin_vel_mean_y)>(param_prefix_ + ".lin_vel_mean_y", config->lin_vel_mean_y);
  config->lin_vel_mean_z = node_->declare_parameter<decltype(config->lin_vel_mean_z)>(param_prefix_ + ".lin_vel_mean_z", config->lin_vel_mean_z);
  config->ang_vel_mean_x = node_->declare_parameter<decltype(config->ang_vel_mean_x)>(param_prefix_ + ".ang_vel_mean_x", config->ang_vel_mean_x);
  config->ang_vel_mean_y = node_->declare_parameter<decltype(config->ang_vel_mean_y)>(param_prefix_ + ".ang_vel_mean_y", config->ang_vel_mean_y);
  config->ang_vel_mean_z = node_->declare_parameter<decltype(config->ang_vel_mean_z)>(param_prefix_ + ".ang_vel_mean_z", config->ang_vel_mean_z);

  config->lin_vel_std_dev_x = node_->declare_parameter<decltype(config->lin_vel_std_dev_x)>(param_prefix_ + ".lin_vel_std_dev_x", config->lin_vel_std_dev_x);
  config->lin_vel_std_dev_y = node_->declare_parameter<decltype(config->lin_vel_std_dev_y)>(param_prefix_ + ".lin_vel_std_dev_y", config->lin_vel_std_dev_y);
  config->lin_vel_std_dev_z = node_->declare_parameter<decltype(config->lin_vel_std_dev_z)>(param_prefix_ + ".lin_vel_std_dev_z", config->lin_vel_std_dev_z);
  config->ang_vel_std_dev_x = node_->declare_parameter<decltype(config->ang_vel_std_dev_x)>(param_prefix_ + ".ang_vel_std_dev_x", config->ang_vel_std_dev_x);
  config->ang_vel_std_dev_y = node_->declare_parameter<decltype(config->ang_vel_std_dev_y)>(param_prefix_ + ".ang_vel_std_dev_y", config->ang_vel_std_dev_y);
  config->ang_vel_std_dev_z = node_->declare_parameter<decltype(config->ang_vel_std_dev_z)>(param_prefix_ + ".ang_vel_std_dev_z", config->ang_vel_std_dev_z);
  // clang-format on
}

void ROSModuleFactory::configureKeyframeOptimization(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<KeyframeOptimizationModule::Config> config;
  /// config.reset(new KeyframeOptimizationModule::Config());
  auto config = std::make_shared<KeyframeOptimizationModule::Config>();

  // Base Config
  auto base_config = std::dynamic_pointer_cast<SteamModule::Config>(config);
  configureSteam(base_config);

  config->pose_prior_enable = node_->declare_parameter<decltype(config->pose_prior_enable)>(param_prefix_ + ".pose_prior_enable", config->pose_prior_enable);
  config->depth_prior_enable = node_->declare_parameter<decltype(config->depth_prior_enable)>(param_prefix_ + ".depth_prior_enable", config->depth_prior_enable);
  config->depth_prior_weight = node_->declare_parameter<decltype(config->depth_prior_weight)>(param_prefix_ + ".depth_prior_weight", config->depth_prior_weight);
  /// config->max_point_depth = node_->declare_parameter<decltype(config->max_point_depth)>(param_prefix_ + ".max_point_depth", config->max_point_depth);
  config->use_migrated_points = node_->declare_parameter<decltype(config->use_migrated_points)>(param_prefix_ + ".use_migrated_points", config->use_migrated_points);

  std::dynamic_pointer_cast<KeyframeOptimizationModule>(new_module)->setConfig(config);
  // clang-format on
}

void ROSModuleFactory::configureWindowOptimization(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<WindowOptimizationModule::Config> config;
  /// config.reset(new WindowOptimizationModule::Config());
  auto config = std::make_shared<WindowOptimizationModule::Config>();

  // Base Config
  auto base_config = std::dynamic_pointer_cast<SteamModule::Config>(config);
  configureSteam(base_config);

  config->depth_prior_enable = node_->declare_parameter<decltype(config->depth_prior_enable)>(param_prefix_ + ".depth_prior_enable", config->depth_prior_enable);
  config->depth_prior_weight = node_->declare_parameter<decltype(config->depth_prior_weight)>(param_prefix_ + ".depth_prior_weight", config->depth_prior_weight);
  // config->max_point_depth = node_->declare_parameter<decltype(config->max_point_depth)>(param_prefix_ + ".max_point_depth", config->max_point_depth);

  std::dynamic_pointer_cast<WindowOptimizationModule>(new_module)->setConfig(config);
}

#if 0
void ROSModuleFactory::configureOpenCVStereoMatcher(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<OpenCVStereoMatcherModule::Config> config;
  config.reset(new OpenCVStereoMatcherModule::Config());

  nh_->getParam(param_prefix_ + "nn_match_ratio", config->nn_match_ratio);
  nh_->getParam(param_prefix_ + "forward_matching_pixel_thresh",
                config->forward_matching_pixel_thresh);
  nh_->getParam(param_prefix_ + "max_point_depth", config->max_point_depth);
  nh_->getParam(param_prefix_ + "descriptor_thresh", config->descriptor_thresh);

  std::dynamic_pointer_cast<OpenCVStereoMatcherModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureASRLMonoMatcher(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<ASRLMonoMatcherModule::Config> config;
  config.reset(new ASRLMonoMatcherModule::Config());

  nh_->getParam(param_prefix_ + "check_laplacian_bit",
                config->check_laplacian_bit);
  nh_->getParam(param_prefix_ + "check_octave", config->check_octave);
  nh_->getParam(param_prefix_ + "check_response", config->check_response);
  nh_->getParam(param_prefix_ + "min_response_ratio",
                config->min_response_ratio);
  nh_->getParam(param_prefix_ + "matching_pixel_thresh",
                config->matching_pixel_thresh);
  nh_->getParam(param_prefix_ + "tight_matching_pixel_thresh",
                config->tight_matching_pixel_thresh);
  nh_->getParam(param_prefix_ + "tight_matching_x_sigma",
                config->tight_matching_x_sigma);
  nh_->getParam(param_prefix_ + "tight_matching_y_sigma",
                config->tight_matching_y_sigma);
  nh_->getParam(param_prefix_ + "tight_matching_phi_sigma",
                config->tight_matching_phi_sigma);
  nh_->getParam(param_prefix_ + "tight_matching_theta_sigma",
                config->tight_matching_theta_sigma);
  nh_->getParam(param_prefix_ + "tight_matching_psi_sigma",
                config->tight_matching_psi_sigma);
  nh_->getParam(param_prefix_ + "use_pixel_variance",
                config->use_pixel_variance);
  std::string prediction_method;
  nh_->getParam(param_prefix_ + "prediction_method", prediction_method);
  if (!prediction_method.compare("se3"))
    config->prediction_method = ASRLMonoMatcherModule::se3;
  else if (!prediction_method.compare("none"))
    config->prediction_method = ASRLMonoMatcherModule::none;
  else
    config->prediction_method = ASRLMonoMatcherModule::none;
  nh_->getParam(param_prefix_ + "max_point_depth", config->max_point_depth);
  nh_->getParam(param_prefix_ + "descriptor_thresh_cpu",
                config->descriptor_thresh_cpu);
  nh_->getParam(param_prefix_ + "descriptor_thresh_gpu",
                config->descriptor_thresh_gpu);
  nh_->getParam(param_prefix_ + "parallel_threads", config->parallel_threads);
  nh_->getParam(param_prefix_ + "visualize_feature_matches",
                config->visualize_feature_matches);
  nh_->getParam(param_prefix_ + "min_matches", config->min_matches);
  nh_->getParam(param_prefix_ + "min_window_size", config->min_window_size);
  nh_->getParam(param_prefix_ + "max_window_size", config->max_window_size);
  nh_->getParam(param_prefix_ + "match_on_gpu", config->match_on_gpu);
  nh_->getParam(param_prefix_ + "match_gpu_tight_knn_match_num",
                config->match_gpu_tight_knn_match_num);
  nh_->getParam(param_prefix_ + "match_gpu_loose_knn_match_num",
                config->match_gpu_loose_knn_match_num);
  std::dynamic_pointer_cast<ASRLMonoMatcherModule>(new_module)
      ->setConfig(config);
}
#endif

void ROSModuleFactory::configureASRLStereoMatcher(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<ASRLStereoMatcherModule::Config> config;
  /// config.reset(new ASRLStereoMatcherModule::Config());
  auto config = std::make_shared<ASRLStereoMatcherModule::Config>();
  config->check_laplacian_bit = node_->declare_parameter<decltype(config->check_laplacian_bit)>(param_prefix_ + ".check_laplacian_bit", config->check_laplacian_bit);
  config->check_octave = node_->declare_parameter<decltype(config->check_octave)>(param_prefix_ + ".check_octave", config->check_octave);
  config->check_response = node_->declare_parameter<decltype(config->check_response)>(param_prefix_ + ".check_response", config->check_response);
  config->min_response_ratio = node_->declare_parameter<decltype(config->min_response_ratio)>(param_prefix_ + ".min_response_ratio", config->min_response_ratio);
  config->matching_pixel_thresh = node_->declare_parameter<decltype(config->matching_pixel_thresh)>(param_prefix_ + ".matching_pixel_thresh", config->matching_pixel_thresh);
  config->tight_matching_pixel_thresh = node_->declare_parameter<decltype(config->tight_matching_pixel_thresh)>(param_prefix_ + ".tight_matching_pixel_thresh", config->tight_matching_pixel_thresh);
  config->tight_matching_x_sigma = node_->declare_parameter<decltype(config->tight_matching_x_sigma)>(param_prefix_ + ".tight_matching_x_sigma", config->tight_matching_x_sigma);
  config->tight_matching_y_sigma = node_->declare_parameter<decltype(config->tight_matching_y_sigma)>(param_prefix_ + ".tight_matching_y_sigma", config->tight_matching_y_sigma);
  config->tight_matching_theta_sigma = node_->declare_parameter<decltype(config->tight_matching_theta_sigma)>(param_prefix_ + ".tight_matching_theta_sigma", config->tight_matching_theta_sigma);
  config->use_pixel_variance = node_->declare_parameter<decltype(config->use_pixel_variance)>(param_prefix_ + ".use_pixel_variance", config->use_pixel_variance);

  auto prediction_method = node_->declare_parameter<std::string>(param_prefix_ + ".prediction_method", "");
  if (!prediction_method.compare("se3"))
    config->prediction_method = ASRLStereoMatcherModule::se3;
  else if (!prediction_method.compare("none"))
    config->prediction_method = ASRLStereoMatcherModule::none;
  else
    config->prediction_method = ASRLStereoMatcherModule::none;

  config->max_point_depth = node_->declare_parameter<decltype(config->max_point_depth)>(param_prefix_ + ".max_point_depth", config->max_point_depth);
  config->descriptor_thresh = node_->declare_parameter<decltype(config->descriptor_thresh)>(param_prefix_ + ".descriptor_thresh", config->descriptor_thresh);
  config->parallel_threads = node_->declare_parameter<decltype(config->parallel_threads)>(param_prefix_ + ".parallel_threads", config->parallel_threads);
#ifdef DETERMINISTIC_VTR
  LOG_IF(config->parallel_threads>1, WARNING) << "ASRL stereo matcher number of threads set to 1 in deterministic mode.";
  config->parallel_threads = 1;
#endif
  config->visualize_feature_matches = node_->declare_parameter<decltype(config->visualize_feature_matches)>(param_prefix_ + ".visualize_feature_matches", config->visualize_feature_matches);

  std::dynamic_pointer_cast<ASRLStereoMatcherModule>(new_module)->setConfig(config);
  // clang-format on
}

void ROSModuleFactory::configureRANSAC(
    std::shared_ptr<RansacModule::Config> &config) const {
  // clang-format off
  // Base RANSAC
  config->enable = node_->declare_parameter<decltype(config->enable)>(param_prefix_ + ".enable", config->enable);
  config->iterations = node_->declare_parameter<decltype(config->iterations)>(param_prefix_ + ".iterations", config->iterations);
  config->flavor = node_->declare_parameter<decltype(config->flavor)>(param_prefix_ + ".flavor", config->flavor);
  config->sigma = node_->declare_parameter<decltype(config->sigma)>(param_prefix_ + ".sigma", config->sigma);
  config->threshold = node_->declare_parameter<decltype(config->threshold)>(param_prefix_ + ".threshold", config->threshold);
  config->early_stop_ratio = node_->declare_parameter<decltype(config->early_stop_ratio)>(param_prefix_ + ".early_stop_ratio", config->early_stop_ratio);
  config->early_stop_min_inliers = node_->declare_parameter<decltype(config->early_stop_min_inliers)>(param_prefix_ + ".early_stop_min_inliers", config->early_stop_min_inliers);
  config->visualize_ransac_inliers = node_->declare_parameter<decltype(config->visualize_ransac_inliers)>(param_prefix_ + ".visualize_ransac_inliers", config->visualize_ransac_inliers);
  config->use_migrated_points = node_->declare_parameter<decltype(config->use_migrated_points)>(param_prefix_ + ".use_migrated_points", config->use_migrated_points);
  config->min_inliers = node_->declare_parameter<decltype(config->min_inliers)>(param_prefix_ + ".min_inliers", config->min_inliers);
  config->enable_local_opt = node_->declare_parameter<decltype(config->enable_local_opt)>(param_prefix_ + ".enable_local_opt", config->enable_local_opt);
  config->num_threads = node_->declare_parameter<decltype(config->num_threads)>(param_prefix_ + ".num_threads", config->num_threads);
#ifdef DETERMINISTIC_VTR
  LOG_IF(config->num_threads!=1, WARNING) << "RANSAC number of threads set to 1 in deterministic mode.";
  config->num_threads = 1;
#endif
  // clang-format on
  // sanity check
  if (config->num_threads < 1) config->num_threads = 1;
}

void ROSModuleFactory::configureStereoRANSAC(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<StereoRansacModule::Config> config;
  /// config.reset(new StereoRansacModule::Config());
  auto config = std::make_shared<StereoRansacModule::Config>();

  // Base Config
  auto base_config = std::dynamic_pointer_cast<RansacModule::Config>(config);
  configureRANSAC(base_config);

  // Stereo RANSAC Config
  config->mask_depth = node_->declare_parameter<decltype(config->mask_depth)>(param_prefix_ + ".mask_depth", config->mask_depth);
  config->mask_depth_inlier_count = node_->declare_parameter<decltype(config->mask_depth_inlier_count)>(param_prefix_ + ".mask_depth_inlier_count", config->mask_depth_inlier_count);
  /// config->visualize_ransac_inliers = node_->declare_parameter<decltype(config->visualize_ransac_inliers)>(param_prefix_ + ".visualize_ransac_inliers", config->visualize_ransac_inliers);
  config->use_covariance = node_->declare_parameter<decltype(config->use_covariance)>(param_prefix_ + ".use_covariance", config->use_covariance);

  std::dynamic_pointer_cast<StereoRansacModule>(new_module)->setConfig(config);
  // clang-format on
}

#if 0
void ROSModuleFactory::configureInitMonoRANSAC(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<InitMonoRansacModule::Config> config;
  config.reset(new InitMonoRansacModule::Config());

  // Init Mono RANSAC Config
  nh_->getParam(param_prefix_ + "ransac_prob", config->ransac_prob);
  nh_->getParam(param_prefix_ + "ransac_thresh", config->ransac_thresh);
  nh_->getParam(param_prefix_ + "min_inliers", config->min_inliers);
  nh_->getParam(param_prefix_ + "min_inlier_ratio", config->min_inlier_ratio);
  nh_->getParam(param_prefix_ + "visualize_ransac_inliers",
                config->visualize_ransac_inliers);

  std::dynamic_pointer_cast<InitMonoRansacModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureMonoRANSAC(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<MonoRansacModule::Config> config;
  config.reset(new MonoRansacModule::Config());

  // Base Config
  auto base_config = std::dynamic_pointer_cast<RansacModule::Config>(config);
  configureRANSAC(base_config);

  // Mono RANSAC Config
  nh_->getParam(param_prefix_ + "mask_inlier_count", config->mask_inlier_count);
  nh_->getParam(param_prefix_ + "visualize_ransac_inliers",
                config->visualize_ransac_inliers);

  std::dynamic_pointer_cast<MonoRansacModule>(new_module)->setConfig(config);
}
#endif

void ROSModuleFactory::configureSimpleVertexCreationTestModule(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<SimpleVertexTestModule::Config> config;
  /// config.reset(new SimpleVertexTestModule::Config());
  auto config = std::make_shared<SimpleVertexTestModule::Config>();
  config->min_creation_distance = node_->declare_parameter<decltype(config->min_creation_distance)>(param_prefix_ + ".min_creation_distance", config->min_creation_distance);
  config->max_creation_distance = node_->declare_parameter<decltype(config->max_creation_distance)>(param_prefix_ + ".max_creation_distance", config->max_creation_distance);
  config->min_distance = node_->declare_parameter<decltype(config->min_distance)>(param_prefix_ + ".min_distance", config->min_distance);
  config->rotation_threshold_min = node_->declare_parameter<decltype(config->rotation_threshold_min)>(param_prefix_ + ".rotation_threshold_min", config->rotation_threshold_min);
  config->rotation_threshold_max = node_->declare_parameter<decltype(config->rotation_threshold_max)>(param_prefix_ + ".rotation_threshold_max", config->rotation_threshold_max);
  config->match_threshold_min_count = node_->declare_parameter<decltype(config->match_threshold_min_count)>(param_prefix_ + ".match_threshold_min_count", config->match_threshold_min_count);
  config->match_threshold_fail_count = node_->declare_parameter<decltype(config->match_threshold_fail_count)>(param_prefix_ + ".match_threshold_fail_count", config->match_threshold_fail_count);
  std::dynamic_pointer_cast<SimpleVertexTestModule>(new_module)->setConfig(config);
  // clang-format on
}

#if 0
void ROSModuleFactory::configureLancasterVertexCreationTestModule(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<LancasterVertexTestModule::Config> config;
  config.reset(new LancasterVertexTestModule::Config());

  nh_->getParam(param_prefix_ + "min_creation_distance",
                config->min_creation_distance);
  nh_->getParam(param_prefix_ + "max_creation_distance",
                config->max_creation_distance);
  nh_->getParam(param_prefix_ + "min_distance", config->min_distance);
  nh_->getParam(param_prefix_ + "rotation_threshold_min",
                config->rotation_threshold_min);
  nh_->getParam(param_prefix_ + "rotation_threshold_max",
                config->rotation_threshold_max);
  nh_->getParam(param_prefix_ + "match_threshold_min_count",
                config->match_threshold_min_count);
  nh_->getParam(param_prefix_ + "match_threshold_fail_count",
                config->match_threshold_fail_count);
  nh_->getParam(param_prefix_ + "tri_threshold_min_count",
                config->tri_threshold_min_count);
  nh_->getParam(param_prefix_ + "tri_threshold_fail_count",
                config->tri_threshold_fail_count);
  nh_->getParam(param_prefix_ + "tri_threshold_init_min_count",
                config->tri_threshold_init_min_count);
  nh_->getParam(param_prefix_ + "tri_threshold_init_fail_count",
                config->tri_threshold_init_fail_count);

  std::dynamic_pointer_cast<LancasterVertexTestModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureGimbalVertexCreationTestModule(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<GimbalVertexTestModule::Config> config;
  config.reset(new GimbalVertexTestModule::Config());

  nh_->getParam(param_prefix_ + "min_creation_distance",
                config->min_creation_distance);
  nh_->getParam(param_prefix_ + "max_creation_distance",
                config->max_creation_distance);
  nh_->getParam(param_prefix_ + "min_distance", config->min_distance);
  nh_->getParam(param_prefix_ + "rotation_threshold_min",
                config->rotation_threshold_min);
  nh_->getParam(param_prefix_ + "rotation_threshold_max",
                config->rotation_threshold_max);
  nh_->getParam(param_prefix_ + "match_threshold_min_count",
                config->match_threshold_min_count);
  nh_->getParam(param_prefix_ + "match_threshold_fail_count",
                config->match_threshold_fail_count);
  nh_->getParam(param_prefix_ + "tri_threshold_min_count",
                config->tri_threshold_min_count);
  nh_->getParam(param_prefix_ + "tri_threshold_fail_count",
                config->tri_threshold_fail_count);
  nh_->getParam(param_prefix_ + "tri_threshold_init_min_count",
                config->tri_threshold_init_min_count);
  nh_->getParam(param_prefix_ + "tri_threshold_init_fail_count",
                config->tri_threshold_init_fail_count);

  std::dynamic_pointer_cast<GimbalVertexTestModule>(new_module)
      ->setConfig(config);
}
#endif

void ROSModuleFactory::configureLandmarkRecallModule(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<LandmarkRecallModule::Config> config;
  /// config.reset(new LandmarkRecallModule::Config());
  auto config = std::make_shared<LandmarkRecallModule::Config>();
  config->landmark_source = node_->declare_parameter<decltype(config->landmark_source)>( param_prefix_ + ".landmark_source", config->landmark_source);
  config->landmark_matches = node_->declare_parameter<decltype(config->landmark_matches)>( param_prefix_ + ".landmark_matches", config->landmark_matches);
  std::dynamic_pointer_cast<LandmarkRecallModule>(new_module)->setConfig(config);
  // clang-format on
}

void ROSModuleFactory::configureWindowedRecallModule(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  /// std::shared_ptr<WindowedRecallModule::Config> config;
  /// config.reset(new WindowedRecallModule::Config());
  auto config = std::make_shared<WindowedRecallModule::Config>();

  config->window_size = node_->declare_parameter<decltype(config->window_size)>(param_prefix_ + ".window_size", config->window_size);

  std::dynamic_pointer_cast<WindowedRecallModule>(new_module)->setConfig(config);
  // clang-format on
}

void ROSModuleFactory::configureSubMapExtraction(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<SubMapExtractionModule::Config> config;
  config.reset(new SubMapExtractionModule::Config());

  config->sigma_scale = node_->declare_parameter<decltype(config->sigma_scale)>(param_prefix_ + ".sigma_scale", config->sigma_scale);
  config->temporal_min_depth = node_->declare_parameter<decltype(config->temporal_min_depth)>(param_prefix_ + ".temporal_min_depth", config->temporal_min_depth);
  config->temporal_max_depth = node_->declare_parameter<decltype(config->temporal_max_depth)>(param_prefix_ + ".temporal_max_depth", config->temporal_max_depth);
  config->search_spatially = node_->declare_parameter<decltype(config->search_spatially)>(param_prefix_ + ".search_spatially", config->search_spatially);
  config->angle_weight = node_->declare_parameter<decltype(config->angle_weight)>(param_prefix_ + ".angle_weight", config->angle_weight);

  std::dynamic_pointer_cast<SubMapExtractionModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureLandmarkMigration(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<LandmarkMigrationModule::Config> config;
  config.reset(new LandmarkMigrationModule::Config());

  std::dynamic_pointer_cast<LandmarkMigrationModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureMelMatcher(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<MelMatcherModule::Config> config;
  config.reset(new MelMatcherModule::Config());

  config->target_match_count = node_->declare_parameter<decltype(config->target_match_count)>(param_prefix_ + ".target_match_count", config->target_match_count);
  config->min_match_count = node_->declare_parameter<decltype(config->min_match_count)>(param_prefix_ + ".min_match_count", config->min_match_count);
  config->time_allowance = node_->declare_parameter<decltype(config->time_allowance)>(param_prefix_ + ".time_allowance", config->time_allowance);
  config->matching_pixel_thresh = node_->declare_parameter<decltype(config->matching_pixel_thresh)>(param_prefix_ + ".matching_pixel_thresh", config->matching_pixel_thresh);
  config->tight_matching_pixel_thresh = node_->declare_parameter<decltype(config->tight_matching_pixel_thresh)>(param_prefix_ + ".tight_matching_pixel_thresh", config->tight_matching_pixel_thresh);
  config->tight_matching_x_sigma = node_->declare_parameter<decltype(config->tight_matching_x_sigma)>(param_prefix_ + ".tight_matching_x_sigma", config->tight_matching_x_sigma);
  config->tight_matching_y_sigma = node_->declare_parameter<decltype(config->tight_matching_y_sigma)>(param_prefix_ + ".tight_matching_y_sigma", config->tight_matching_y_sigma);
  config->tight_matching_theta_sigma = node_->declare_parameter<decltype(config->tight_matching_theta_sigma)>(param_prefix_ + ".tight_matching_theta_sigma", config->tight_matching_theta_sigma);
  config->min_response_ratio = node_->declare_parameter<decltype(config->min_response_ratio)>(param_prefix_ + ".min_response_ratio", config->min_response_ratio);
  config->descriptor_thresh_cpu = node_->declare_parameter<decltype(config->descriptor_thresh_cpu)>(param_prefix_ + ".descriptor_thresh_cpu", config->descriptor_thresh_cpu);
  config->descriptor_thresh_gpu = node_->declare_parameter<decltype(config->descriptor_thresh_gpu)>(param_prefix_ + ".descriptor_thresh_gpu", config->descriptor_thresh_gpu);
  config->min_track_length = node_->declare_parameter<decltype(config->min_track_length)>(param_prefix_ + ".min_track_length", config->min_track_length);
  config->max_landmark_depth = node_->declare_parameter<decltype(config->max_landmark_depth)>(param_prefix_ + ".max_landmark_depth", config->max_landmark_depth);
  config->max_depth_diff = node_->declare_parameter<decltype(config->max_depth_diff)>(param_prefix_ + ".max_depth_diff", config->max_depth_diff);
  config->visualize = node_->declare_parameter<decltype(config->visualize)>(param_prefix_ + ".visualize", config->visualize);
  config->screen_matched_landmarks = node_->declare_parameter<decltype(config->screen_matched_landmarks)>(param_prefix_ + ".screen_matched_landmarks", config->screen_matched_landmarks);
  config->parallel_threads = node_->declare_parameter<decltype(config->parallel_threads)>(param_prefix_ + ".parallel_threads", config->parallel_threads);
#ifdef DETERMINISTIC_VTR
  LOG_IF(config->parallel_threads>1, WARNING) << "MEL matcher number of threads set to 1 in deterministic mode.";
  config->parallel_threads = 1;
#endif
  config->match_on_gpu = node_->declare_parameter<decltype(config->match_on_gpu)>(param_prefix_ + ".match_on_gpu", config->match_on_gpu);
  config->match_gpu_knn_match_num = node_->declare_parameter<decltype(config->match_gpu_knn_match_num)>(param_prefix_ + ".match_gpu_knn_match_num", config->match_gpu_knn_match_num);

  std::dynamic_pointer_cast<MelMatcherModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureMelRecog(
    std::shared_ptr<BaseModule> &new_module) const {
  // clang-format off
  auto config = std::make_shared<MelRecognitionModule::Config>();

  config->temporal_depth = node_->declare_parameter<decltype(config->temporal_depth)>( param_prefix_ + ".temporal_depth", config->temporal_depth);
  config->verbose = node_->declare_parameter<decltype(config->verbose)>( param_prefix_ + ".verbose", config->verbose);
  config->sliding_window = node_->declare_parameter<decltype(config->sliding_window)>( param_prefix_ + ".sliding_window", config->sliding_window);
  config->cluster_size = node_->declare_parameter<decltype(config->cluster_size)>( param_prefix_ + ".cluster_size", config->cluster_size);
  config->compare_octave = node_->declare_parameter<decltype(config->compare_octave)>( param_prefix_ + ".compare_octave", config->compare_octave);
  config->compare_laplacian = node_->declare_parameter<decltype(config->compare_laplacian)>( param_prefix_ + ".compare_laplacian", config->compare_laplacian);
  config->num_desired_experiences = node_->declare_parameter<decltype(config->num_desired_experiences)>( param_prefix_ + ".num_desired_experiences", config->num_desired_experiences);
  config->in_the_loop = node_->declare_parameter<decltype(config->in_the_loop)>( param_prefix_ + ".in_the_loop", config->in_the_loop);

  std::dynamic_pointer_cast<MelRecognitionModule>(new_module)->setConfig(config);
  // clang-format on
}

void ROSModuleFactory::configureTodRecog(
    std::shared_ptr<BaseModule> &new_module) const {
  TodRecognitionModule::Config config;

  config.verbose = node_->declare_parameter<decltype(config.verbose)>(param_prefix_ + ".verbose", config.verbose);
  config.num_exp = node_->declare_parameter<decltype(config.num_exp)>(param_prefix_ + ".num_desired_experiences", config.num_exp);
  config.in_the_loop = node_->declare_parameter<decltype(config.in_the_loop)>(param_prefix_ + ".in_the_loop", config.in_the_loop);
  config.time_of_day_weight = node_->declare_parameter<decltype(config.time_of_day_weight)>(param_prefix_ + ".time_of_day_weight", config.time_of_day_weight);
  config.total_time_weight = node_->declare_parameter<decltype(config.total_time_weight)>(param_prefix_ + ".total_time_weight", config.total_time_weight);

  std::dynamic_pointer_cast<TodRecognitionModule>(new_module)->setConfig(std::move(config));
}

void ROSModuleFactory::configureCollabLandmarks(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<CollaborativeLandmarksModule::Config> config;
  config.reset(new CollaborativeLandmarksModule::Config());

  config->verbose = node_->declare_parameter<decltype(config->verbose)>(param_prefix_ + ".verbose", config->verbose);
  config->num_exp = node_->declare_parameter<decltype(config->num_exp)>(param_prefix_ + ".num_desired_experiences", config->num_exp);
  config->in_the_loop = node_->declare_parameter<decltype(config->in_the_loop)>(param_prefix_ + ".in_the_loop", config->in_the_loop);
  config->similarity_decay = node_->declare_parameter<decltype(config->similarity_decay)>(param_prefix_ + ".similarity_decay", config->similarity_decay);
  config->prediction_decay = node_->declare_parameter<decltype(config->prediction_decay)>(param_prefix_ + ".prediction_decay", config->prediction_decay);
  config->recommend_landmarks = node_->declare_parameter<decltype(config->recommend_landmarks)>(param_prefix_ + ".recommend_landmarks", config->recommend_landmarks);

  std::dynamic_pointer_cast<CollaborativeLandmarksModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureRandomExperiences(
    std::shared_ptr<BaseModule> &new_module) const {
  RandomExperiencesModule::Config config;

  config.verbose = node_->declare_parameter<decltype(config.verbose)>(param_prefix_ + ".verbose", config.verbose);
  config.in_the_loop = node_->declare_parameter<decltype(config.in_the_loop)>(param_prefix_ + ".in_the_loop", config.in_the_loop);
  config.num_exp = node_->declare_parameter<decltype(config.num_exp)>(param_prefix_ + ".num_desired_experiences", config.num_exp);

  std::dynamic_pointer_cast<RandomExperiencesModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureExperienceTriage(
    std::shared_ptr<BaseModule> &new_module) const {
  ExperienceTriageModule::Config config;

  config.verbose = node_->declare_parameter<decltype(config.verbose)>(param_prefix_ + ".verbose", config.verbose);
  config.always_privileged = node_->declare_parameter<decltype(config.always_privileged)>(param_prefix_ + ".always_privileged", config.always_privileged);
  config.in_the_loop = node_->declare_parameter<decltype(config.in_the_loop)>(param_prefix_ + ".in_the_loop", config.in_the_loop);

  std::dynamic_pointer_cast<ExperienceTriageModule>(new_module)->setConfig(config);
}

#if false
void ROSModuleFactory::configureResults(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<ResultsModule::Config> config;
  config.reset(new ResultsModule::Config());

  nh_->getParam(param_prefix_ + "directory", config->directory);

  std::dynamic_pointer_cast<ResultsModule>(new_module)->setConfig(config);
}

#if 0
void ROSModuleFactory::configureMonoScaling(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<MonoPlanarScalingModule::Config> config;
  config.reset(new MonoPlanarScalingModule::Config());

  // Mono Scaling Module Config
  nh_->getParam(param_prefix_ + "base_height", config->base_height);
  nh_->getParam(param_prefix_ + "init_distance", config->init_distance);
  nh_->getParam(param_prefix_ + "use_position_as_lla",
                config->use_position_as_lla);

  std::dynamic_pointer_cast<MonoPlanarScalingModule>(new_module)
      ->setConfig(config);
}
void ROSModuleFactory::configureUnderfootSeparate(
    std::shared_ptr<BaseModule> &new_module) const {
  UnderfootSeparateModule::Config config;

  config.node_handle = const_cast<ros::NodeHandle *>(nh_);
  config.param_prefix = param_prefix_;

  nh_->getParam(param_prefix_ + "/underfoot_distance",
                config.underfoot_distance);
  nh_->getParam(param_prefix_ + "/occlusion_distance",
                config.occlusion_distance);
  nh_->getParam(param_prefix_ + "/max_vertices", config.max_vertices);
  nh_->getParam(param_prefix_ + "/underfoot_set_skip",
                config.underfoot_set_skip);
  nh_->getParam(param_prefix_ + "/obstacle_button_idx",
                config.obstacle_button_idx);
  nh_->getParam(param_prefix_ + "/verbose", config.verbose);

  config.patch_gen_params =
      std::make_shared<terrain_assessment::PatchGenParams>();
  nh_->getParam(param_prefix_ + "/patch_gen_params/num_cells",
                config.patch_gen_params->num_cells);
  nh_->getParam(param_prefix_ + "/patch_gen_params/cell_sizes",
                config.patch_gen_params->cell_sizes);
  nh_->getParam(param_prefix_ + "/patch_gen_params/points_per_cell",
                config.patch_gen_params->points_per_cell);
  nh_->getParam(param_prefix_ + "/patch_gen_params/imagelike_pointcloud",
                config.patch_gen_params->imagelike_pointcloud);

  std::dynamic_pointer_cast<UnderfootSeparateModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureUnderfootAggregate(
    std::shared_ptr<BaseModule> &new_module) const {
  UnderfootAggregateModule::Config config;

  config.node_handle = const_cast<ros::NodeHandle *>(nh_);
  config.param_prefix = param_prefix_;

  nh_->getParam(param_prefix_ + "/underfoot_distance",
                config.underfoot_distance);
  nh_->getParam(param_prefix_ + "/occlusion_distance",
                config.occlusion_distance);
  nh_->getParam(param_prefix_ + "/max_vertices", config.max_vertices);
  nh_->getParam(param_prefix_ + "/underfoot_set_skip",
                config.underfoot_set_skip);
  nh_->getParam(param_prefix_ + "/obstacle_button_idx",
                config.obstacle_button_idx);
  nh_->getParam(param_prefix_ + "/verbose", config.verbose);

  config.patch_gen_params =
      std::make_shared<terrain_assessment::PatchGenParams>();
  nh_->getParam(param_prefix_ + "/patch_gen_params/num_cells",
                config.patch_gen_params->num_cells);
  nh_->getParam(param_prefix_ + "/patch_gen_params/cell_sizes",
                config.patch_gen_params->cell_sizes);
  nh_->getParam(param_prefix_ + "/patch_gen_params/points_per_cell",
                config.patch_gen_params->points_per_cell);
  nh_->getParam(param_prefix_ + "/patch_gen_params/imagelike_pointcloud",
                config.patch_gen_params->imagelike_pointcloud);

  std::dynamic_pointer_cast<UnderfootAggregateModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureQuickVORosPublisher(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<QuickVORosPublisherModule::Config> config;
  config.reset(new QuickVORosPublisherModule::Config());

  nh_->getParam(param_prefix_ + "use_position_as_lla",
                config->use_position_as_lla);
  nh_->getParam(param_prefix_ + "line_strip_scale", config->line_strip_scale);
  nh_->getParam(param_prefix_ + "robot_base_frame", config->robot_base_frame);

  std::dynamic_pointer_cast<QuickVORosPublisherModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureRefinedVORosPublisher(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<RefinedVORosPublisherModule::Config> config;
  config.reset(new RefinedVORosPublisherModule::Config());

  nh_->getParam(param_prefix_ + "use_position_as_lla",
                config->use_position_as_lla);
  nh_->getParam(param_prefix_ + "line_strip_scale", config->line_strip_scale);

  std::dynamic_pointer_cast<RefinedVORosPublisherModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureLocalizationRosPublisher(
    std::shared_ptr<BaseModule> &new_module) const {
  std::shared_ptr<LocalizationRosPublisherModule::Config> config;
  config.reset(new LocalizationRosPublisherModule::Config());

  nh_->getParam(param_prefix_ + "use_position_as_lla",
                config->use_position_as_lla);
  nh_->getParam(param_prefix_ + "line_strip_scale", config->line_strip_scale);

  std::dynamic_pointer_cast<LocalizationRosPublisherModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureLookaheadPatchGeneration(
    std::shared_ptr<BaseModule> &new_module) const {
  LookaheadPatchGenerationModule::Config config;

  config.node_handle = const_cast<ros::NodeHandle *>(nh_);
  config.param_prefix = param_prefix_;

  nh_->getParam(param_prefix_ + "/max_fps", config.max_fps);
  nh_->getParam(param_prefix_ + "/lookahead_distance",
                config.lookahead_distance);
  nh_->getParam(param_prefix_ + "/occlusion_distance",
                config.occlusion_distance);
  nh_->getParam(param_prefix_ + "/max_vertices", config.max_lookahead_vertices);
  nh_->getParam(param_prefix_ + "/lookahead_set_skip",
                config.lookahead_set_skip);
  nh_->getParam(param_prefix_ + "/save_to_disk", config.save_to_disk);
  nh_->getParam(param_prefix_ + "/verbose", config.verbose);

  config.patch_gen_params =
      std::make_shared<terrain_assessment::PatchGenParams>();
  nh_->getParam(param_prefix_ + "/patch_gen_params/num_cells",
                config.patch_gen_params->num_cells);
  nh_->getParam(param_prefix_ + "/patch_gen_params/cell_sizes",
                config.patch_gen_params->cell_sizes);
  nh_->getParam(param_prefix_ + "/patch_gen_params/points_per_cell",
                config.patch_gen_params->points_per_cell);
  nh_->getParam(param_prefix_ + "/patch_gen_params/imagelike_pointcloud",
                config.patch_gen_params->imagelike_pointcloud);

  nh_->getParam(param_prefix_ + "/matched_feature_mask/enabled",
                config.matched_feature_mask_config.enabled);
  nh_->getParam(param_prefix_ + "/matched_feature_mask/surf_base_window",
                config.matched_feature_mask_config.surf_base_window);

  std::dynamic_pointer_cast<LookaheadPatchGenerationModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureTraining(
    std::shared_ptr<BaseModule> &new_module) const {
  TrainingModule::Config config;

  nh_->getParam(param_prefix_ + "/training_distance", config.training_distance);
  nh_->getParam(param_prefix_ + "/max_vertices", config.max_training_vertices);
  nh_->getParam(param_prefix_ + "/training_runs", config.n_training_runs);
  nh_->getParam(param_prefix_ + "/min_cell_fraction", config.min_cell_fraction);

  std::dynamic_pointer_cast<TrainingModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureGpcTraining(
    std::shared_ptr<BaseModule> &new_module) const {
  GpcTrainingModule::Config config;

  std::dynamic_pointer_cast<GpcTrainingModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureChangeDetection(
    std::shared_ptr<BaseModule> &new_module) const {
  ChangeDetectionModule::Config config;

  config.node_handle = const_cast<ros::NodeHandle *>(nh_);
  config.param_prefix = param_prefix_;

  nh_->getParam(param_prefix_ + "/max_fps", config.max_fps);
  nh_->getParam(param_prefix_ + "/max_diff", config.max_label);
  nh_->getParam(param_prefix_ + "/n_consec_inf", config.n_consec_inf);
  nh_->getParam(param_prefix_ + "/slow_distance", config.slow_distance);
  nh_->getParam(param_prefix_ + "/stop_distance", config.stop_distance);

  nh_->getParam(param_prefix_ + "/lookahead/lookahead_distance",
                config.lookahead_distance);
  nh_->getParam(param_prefix_ + "/lookahead/occlusion_distance",
                config.occlusion_distance);
  nh_->getParam(param_prefix_ + "/lookahead/max_vertices",
                config.max_lookahead_vertices);
  nh_->getParam(param_prefix_ + "/lookahead/lookahead_set_skip",
                config.lookahead_set_skip);

  nh_->getParam(param_prefix_ + "/training/training_distance",
                config.training_distance);
  nh_->getParam(param_prefix_ + "/training/max_vertices",
                config.max_training_vertices);
  nh_->getParam(param_prefix_ + "/training/training_runs",
                config.n_training_runs);
  nh_->getParam(param_prefix_ + "/training/min_cell_fraction",
                config.min_cell_fraction);

  std::string smoothing_type;
  nh_->getParam(param_prefix_ + "/smoothing_type", smoothing_type);
  if (smoothing_type == "none") {
    config.smoothing_config.smoothing_type =
        terrain_assessment::SmoothingType::None;
  } else if (smoothing_type == "median") {
    config.smoothing_config.smoothing_type =
        terrain_assessment::SmoothingType::Median;
  } else if (smoothing_type == "connected_component") {
    config.smoothing_config.smoothing_type =
        terrain_assessment::SmoothingType::ConnectedComponent;
  }
  nh_->param<int>(param_prefix_ + "/nth_best", config.smoothing_config.nth_best,
                  0);
  nh_->param<int>(param_prefix_ + "/n_components",
                  config.smoothing_config.n_components, 0);

  config.patch_gen_params =
      std::make_shared<terrain_assessment::PatchGenParams>();
  nh_->getParam(param_prefix_ + "/patch_gen_params/num_cells",
                config.patch_gen_params->num_cells);
  nh_->getParam(param_prefix_ + "/patch_gen_params/cell_sizes",
                config.patch_gen_params->cell_sizes);
  nh_->getParam(param_prefix_ + "/patch_gen_params/points_per_cell",
                config.patch_gen_params->points_per_cell);
  nh_->getParam(param_prefix_ + "/patch_gen_params/imagelike_pointcloud",
                config.patch_gen_params->imagelike_pointcloud);

  std::dynamic_pointer_cast<ChangeDetectionModule>(new_module)
      ->setConfig(config);
}

void ROSModuleFactory::configureCDMaxMin(
    std::shared_ptr<BaseModule> &new_module) const {
  configureChangeDetection(new_module);
}

void ROSModuleFactory::configureCDMinMax(
    std::shared_ptr<BaseModule> &new_module) const {
  configureChangeDetection(new_module);
}

void ROSModuleFactory::configureCDGmm(
    std::shared_ptr<BaseModule> &new_module) const {
  configureChangeDetection(new_module);

  CDGmmModule::Config config;

  config.dpgmm_config = std::make_shared<terrain_assessment::DPGmm::Config>();
  nh_->getParam(param_prefix_ + "/gmm/k", config.dpgmm_config->base_config->K);
  nh_->getParam(param_prefix_ + "/gmm/max_iter", config.dpgmm_config->max_iter);
  nh_->getParam(param_prefix_ + "/gmm/tol", config.dpgmm_config->tol);

  std::dynamic_pointer_cast<CDGmmModule>(new_module)->setConfig(config);
}

void ROSModuleFactory::configureCDGpc(
    std::shared_ptr<BaseModule> &new_module) const {
  configureChangeDetection(new_module);

  CDGpcModule::Config config;

  nh_->getParam(param_prefix_ + "/gp/mean", config.mean);
  nh_->getParam(param_prefix_ + "/gp/l", config.l);
  nh_->getParam(param_prefix_ + "/gp/sf", config.sf);

  std::dynamic_pointer_cast<CDGpcModule>(new_module)->setConfig(config);
}
#endif
#endif
}  // namespace navigation
}  // namespace vtr