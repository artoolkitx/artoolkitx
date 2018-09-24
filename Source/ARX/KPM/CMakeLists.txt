set(PUBLIC_HEADERS
    include/ARX/KPM/kpm.h
    include/ARX/KPM/kpmType.h
)

set(SOURCE
	kpmFopen.h
    kpmPrivate.h
	FreakMatcher/detectors/DoG_scale_invariant_detector.h
	FreakMatcher/detectors/gaussian_scale_space_pyramid.h
	FreakMatcher/detectors/gradients.h
	FreakMatcher/detectors/harris-inline.h
	FreakMatcher/detectors/harris.h
	FreakMatcher/detectors/interpolate.h
	FreakMatcher/detectors/orientation_assignment.h
	FreakMatcher/detectors/pyramid-inline.h
	FreakMatcher/detectors/pyramid.h
	FreakMatcher/facade/visual_database_facade.h
	FreakMatcher/framework/date_time.h
	FreakMatcher/framework/error.h
	FreakMatcher/framework/exception.h
	FreakMatcher/framework/image.h
	FreakMatcher/framework/image_utils.h
	FreakMatcher/framework/logger.h
	FreakMatcher/framework/timers.h
	FreakMatcher/homography_estimation/homography_solver.h
	FreakMatcher/homography_estimation/robust_homography.h
	FreakMatcher/matchers/binary_hierarchical_clustering.h
	FreakMatcher/matchers/feature_matcher-inline.h
	FreakMatcher/matchers/feature_matcher.h
	FreakMatcher/matchers/feature_point.h
	FreakMatcher/matchers/feature_store.h
	FreakMatcher/matchers/freak.h
	FreakMatcher/matchers/freak84-inline.h
	FreakMatcher/matchers/hough_similarity_voting.h
	FreakMatcher/matchers/keyframe.h
	FreakMatcher/matchers/kmedoids.h
	FreakMatcher/matchers/matcher_types.h
	FreakMatcher/matchers/visual_database-inline.h
	FreakMatcher/matchers/visual_database.h
	FreakMatcher/matchers/visual_database_types.h
	FreakMatcher/math/cholesky.h
	FreakMatcher/math/cholesky_linear_solvers.h
	FreakMatcher/math/geometry.h
	FreakMatcher/math/hamming.h
	FreakMatcher/math/homography.h
	FreakMatcher/math/indexing.h
	FreakMatcher/math/linear_algebra.h
	FreakMatcher/math/linear_solvers.h
	FreakMatcher/math/math_utils.h
	FreakMatcher/math/matrix.h
	FreakMatcher/math/polynomial.h
	FreakMatcher/math/quaternion.h
	FreakMatcher/math/rand.h
	FreakMatcher/math/robustifiers.h
	FreakMatcher/utils/feature_drawing.h
	FreakMatcher/utils/partial_sort.h
	FreakMatcher/utils/point.h
	kpmHandle.cpp
	kpmRefDataSet.cpp
	kpmMatching.cpp
	kpmResult.cpp
	kpmUtil.cpp
	kpmFopen.c
	FreakMatcher/detectors/DoG_scale_invariant_detector.cpp
	FreakMatcher/detectors/gaussian_scale_space_pyramid.cpp
	FreakMatcher/detectors/gradients.cpp
	FreakMatcher/detectors/harris.cpp
	FreakMatcher/detectors/orientation_assignment.cpp
	FreakMatcher/detectors/pyramid.cpp
	FreakMatcher/facade/visual_database_facade.cpp
	FreakMatcher/matchers/hough_similarity_voting.cpp
	FreakMatcher/matchers/freak.cpp
	FreakMatcher/framework/date_time.cpp
	FreakMatcher/framework/image.cpp
	FreakMatcher/framework/logger.cpp
	FreakMatcher/framework/timers.cpp
)

add_library(KPM STATIC
    ${PUBLIC_HEADERS} ${SOURCE}
)

target_include_directories(KPM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/FreakMatcher
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ARUtil/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../AR/include
    PRIVATE ${JPEG_INCLUDE_DIR}
)

target_link_libraries(KPM
    INTERFACE ${LIBS}
    PRIVATE ${JPEG_LIBRARIES}
)

# Pass on headers to parent.
string(REGEX REPLACE "([^;]+)" "KPM/\\1" hprefixed "${PUBLIC_HEADERS}")
set(FRAMEWORK_HEADERS
   ${FRAMEWORK_HEADERS}
   ${hprefixed}
   PARENT_SCOPE
)

