/*
 *  mapper.cpp
 *  arx_mapper
 *
 *  Marker mapping using artoolkitX and GTSAM
 *
 *  Copyright 2018 Eden Networks Ltd.
 *
 *  Author(s): Philip Lamb.
 *
 */

#include <ARX/AR/config.h>

#if HAVE_GTSAM
#include "mapper.hpp"
#include <gtsam/base/Vector.h>

// In GTSAM, measurement functions are represented as 'factors'.
// Initialize the origin landmark location using a Prior factor.
#include <gtsam/slam/PriorFactor.h>

 // From artoolkitX (millimetres) to GTSAM (metres).
#ifdef ARDOUBLE_IS_FLOAT
#  define SCALEF 1000.0f
#else
#  define SCALEF 1000
#endif

namespace arx_mapper {
    
    using namespace gtsam;
    
    int Mapper::pose_cnt = 0;
    
    Mapper::Mapper(double relinearize_thresh, int relinearize_skip) :
        inited_(false),
        params_(ISAM2GaussNewtonParams(), relinearize_thresh, relinearize_skip),
        isam2_(params_),
        marker_noise_(noiseModel::Diagonal::Sigmas((Vector(6) << Vector3::Constant(0.20), Vector3::Constant(0.1)).finished())), // 20cm std on x,y,z, 0.1 rad (5.73 deg) on roll,pitch,yaw.
        small_noise_(noiseModel::Diagonal::Sigmas((Vector(6) << Vector3::Constant(0.10), Vector3::Constant(0.05)).finished())) {  // 10cm std on x,y,z 0.05 rad (2.86 deg) on roll,pitch,yaw.
    }
    
    void Mapper::AddPose(const ARdouble trans[3][4]) {
        pose_cnt++;
        pose_ = PoseFromARTrans(trans).inverse();
        initial_estimates_.insert(Symbol('x', pose_cnt), pose_);
    }
    
    void Mapper::AddFactors(const std::vector<Marker>& markers) {
        Symbol x_i('x', pose_cnt);
        for (const Marker& marker : markers) {
            graph_.push_back(BetweenFactor<Pose3>(x_i, Symbol('l', marker.uid), PoseFromARTrans(marker.trans), marker_noise_));
        }
    }
    
    void Mapper::Initialize(int uid, ARdouble width) {
        
        if (pose_cnt != 1) {
            ARLOGe("Incorrect initial pose.\n");
            return;
        }
        width_ = width;
        
        AddLandmark(uid, Pose3());
        
        // A very strong prior on first pose and landmark
        ARLOGi("Add pose prior on pose %d.\n", pose_cnt);
        graph_.push_back(PriorFactor<Pose3>(Symbol('x', pose_cnt), pose_, small_noise_));
        ARLOGi("Add landmark prior on uid %d.\n", uid);
        graph_.push_back(PriorFactor<Pose3>(Symbol('l', uid), Pose3(), small_noise_));

        inited_ = true;
    }
    
    // private
    void Mapper::AddLandmark(int uid, const Pose3 &pose) {
        initial_estimates_.insert(Symbol('l', uid), pose);
        all_uids_.insert(uid);
    }
    
    void Mapper::AddLandmarks(const std::vector<Marker>& markers) {
        for (const Marker& marker : markers) {
            // Only add landmark if it's not already added
            if (all_uids_.find(marker.uid) == all_uids_.end()) {
                const Pose3 &w_T_c = pose_;
                const Pose3 c_T_t = PoseFromARTrans(marker.trans);
                const Pose3 w_T_t = w_T_c.compose(c_T_t);
                AddLandmark(marker.uid, w_T_t);
            }
        }
    }
    
    void Mapper::Optimize(int num_iterations) {
        // Each call to iSAM2 update(*) performs one iteration of the iterative nonlinear solver.
        // If accuracy is desired at the expense of time, update(*) can be called additional times
        // to perform multiple optimizer iterations every step.
        isam2_.update(graph_, initial_estimates_);
        if (num_iterations > 1) {
            for (int i = 1; i < num_iterations; ++i) {
                isam2_.update();
            }
        }
    }
    
    void Mapper::Update(ARMultiMarkerInfoT* map) const {
        
        Values results = isam2_.calculateEstimate();
        //results.print("Current estimate: ");
        
        // Update the current camera pose.
        ARTransFromPose(map->trans, (results.at<Pose3>(Symbol('x', pose_cnt))).inverse());
        
        // Update the current map.
        for (const int uid : all_uids_) {
            ARdouble mTrans[3][4];
            ARTransFromPose(mTrans, results.at<Pose3>(Symbol('l', uid)));
            arMultiAddOrUpdateSubmarker(map, uid, AR_MULTI_PATTERN_TYPE_MATRIX, width_, mTrans, 0);
        }
    }
    
    void Mapper::Clear() {
        graph_.resize(0);
        initial_estimates_.clear();
    }
    
    Pose3 PoseFromARTrans(const ARdouble trans[3][4]) {
        Point3 t(trans[0][3] / SCALEF, trans[1][3] / SCALEF, trans[2][3] / SCALEF);
        Rot3 R(trans[0][0], trans[0][1], trans[0][2],
               trans[1][0], trans[1][1], trans[1][2],
               trans[2][0], trans[2][1], trans[2][2]);
        return Pose3(R, t);
    }
    void ARTransFromPose(ARdouble trans[3][4], const gtsam::Pose3 &pose) {
        const Point3 &t = pose.translation();
        const Matrix3 &R = pose.rotation().matrix();
        trans[0][0] = (ARdouble)R(0, 0); trans[0][1] = (ARdouble)R(0, 1); trans[0][2] = (ARdouble)R(0, 2); trans[0][3] = (ARdouble)t(0) * SCALEF;
        trans[1][0] = (ARdouble)R(1, 0); trans[1][1] = (ARdouble)R(1, 1); trans[1][2] = (ARdouble)R(1, 2); trans[1][3] = (ARdouble)t(1) * SCALEF;
        trans[2][0] = (ARdouble)R(2, 0); trans[2][1] = (ARdouble)R(2, 1); trans[2][2] = (ARdouble)R(2, 2); trans[2][3] = (ARdouble)t(2) * SCALEF;
    }
    
}  // namespace arx_mapper

#endif // HAVE_GTSAM
