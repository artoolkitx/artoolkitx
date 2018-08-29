/*
 *  mapper.hpp
 *  arx_mapper
 *
 *  Marker mapping using artoolkitX and GTSAM
 *
 *  Copyright 2018 Eden Networks Ltd.
 *
 *  Author(s): Philip Lamb.
 *
 */

#ifndef __MAPPER_H__
#define __MAPPER_H__

// Each variable in the system (poses and landmarks) must be identified with a unique key.
// We will use symbols (X1, X2, L1).
#include <gtsam/inference/Symbol.h>
// Camera observations of landmarks will be stored as Pose3.
#include <gtsam/geometry/Pose3.h>
// Use iSAM2 to solve the structure-from-motion problem incrementally.
#include <gtsam/nonlinear/ISAM2.h>
// iSAM2 requires as input a set set of new factors to be added stored in a factor graph,
// and initial guesses for any new variables used in the added factors
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
// The nonlinear solvers within GTSAM are iterative solvers, meaning they linearize the
// nonlinear functions around an initial linearization point, then solve the linear system
// to update the linearization point. This happens repeatedly until the solver converges
// to a consistent set of variable values. This requires us to specify an initial guess
// for each variable, held in a Values container.
#include <gtsam/nonlinear/Values.h>

// In GTSAM, measurement functions are represented as 'factors'.
// Use Between factors to model the camera's landmark observations.
#include <gtsam/slam/BetweenFactor.h>

// Map is stored as an ARMultiMarkerInfoT structure.
#include <ARX/AR/ar.h>
#include <ARX/AR/arMulti.h>

namespace arx_mapper {
    
    struct Marker {
        int uid;
        ARdouble trans[3][4]; // Marker pose in camera coordinates.
    };

    class Mapper {
    public:
        static int pose_cnt;
        
        Mapper(double relinearize_thresh, int relinearize_skip);
        
        void AddPose(const ARdouble trans[3][4]); // Map pose in camera coordinates.
        void AddFactors(const std::vector<Marker>& markers);

        bool inited() const { return inited_; }
        void Optimize(int num_iterations = 1);
        void Update(ARMultiMarkerInfoT* map) const;
        void AddLandmarks(const std::vector<Marker>& markers);
        void Initialize(int uid, ARdouble width);
        void Clear();
        
    private:
        void AddLandmark(int uid, const gtsam::Pose3& pose);
        
        ARdouble width_;
        bool inited_;
        gtsam::ISAM2Params params_;
        gtsam::ISAM2 isam2_;
        gtsam::NonlinearFactorGraph graph_;
        gtsam::Values initial_estimates_;
        gtsam::Pose3 pose_;
        gtsam::noiseModel::Diagonal::shared_ptr marker_noise_;
        gtsam::noiseModel::Diagonal::shared_ptr small_noise_;
        std::set<int> all_uids_;
    };
    
    gtsam::Pose3 PoseFromARTrans(const ARdouble trans[3][4]);
    void ARTransFromPose(ARdouble trans[3][4], const gtsam::Pose3 &pose);

}  // namespace arx_mapper

#endif // __MAPPER_H__

