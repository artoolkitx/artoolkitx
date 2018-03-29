//
//  robust_homography.h
//  artoolkitX
//
//  This file is part of artoolkitX.
//
//  artoolkitX is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  artoolkitX is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#pragma once

#include "homography_solver.h"
#include <math/rand.h>
#include <math/homography.h>
#include <math/cholesky_linear_solvers.h>
#include <math/robustifiers.h>
#include <utils/partial_sort.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Eigen>
#include <unsupported/Eigen/MatrixFunctions>

namespace vision {
    
#define HOMOGRAPHY_DEFAULT_CAUCHY_SCALE         0.01
#define HOMOGRAPHY_DEFAULT_NUM_HYPOTHESES       1024
#define HOMOGRAPHY_DEFAULT_MAX_TRIALS           1064
#define HOMOGRAPHY_DEFAULT_CHUNK_SIZE           50
    
    /**
     * Compute the Cauchy reprojection cost for H*p-q.
     */
    template<typename T>
    inline T CauchyProjectiveReprojectionCost(const T H[9], const T p[2], const T q[2], T one_over_scale2) {
        T pp[2];
        T f[2];
        
        MultiplyPointHomographyInhomogenous(pp[0], pp[1], H, p[0], p[1]);
        
        f[0] = pp[0] - q[0];
        f[1] = pp[1] - q[1];
        
        return CauchyCost(f, one_over_scale2);
    }
    
    /**
     * Compute the Cauchy reprojection cost for H*p_i-q_i.
     */
    template<typename T>
    inline T CauchyProjectiveReprojectionCost(const T H[9],
                                              const T* p,
                                              const T* q,
                                              int num_points,
                                              T one_over_scale2) {
        int i;
        T total_cost;
        
        total_cost = 0;
        for(i = 0; i < num_points; i++, p+=2, q+=2) {
            total_cost += CauchyProjectiveReprojectionCost(H, p, q, one_over_scale2);
        }
        
        return total_cost;
    }
    
    /**
     * Robustly solve for the homography given a set of correspondences. 
     */
    template<typename T>
    bool PreemptiveRobustHomography(T H[9],
                                    const T* p,
                                    const T* q,
                                    int num_points,
                                    const T* test_points,
                                    int num_test_points,
                                    std::vector<T> &hyp /* 9*max_num_hypotheses */,
                                    std::vector<int> &tmp_i /* num_points */,
                                    std::vector< std::pair<T, int> > &hyp_costs /* max_num_hypotheses */,
                                    T scale = HOMOGRAPHY_DEFAULT_CAUCHY_SCALE,
                                    int max_num_hypotheses = HOMOGRAPHY_DEFAULT_NUM_HYPOTHESES,
                                    int max_trials = HOMOGRAPHY_DEFAULT_MAX_TRIALS,
                                    int chunk_size = HOMOGRAPHY_DEFAULT_CHUNK_SIZE) {
        int* hyp_perm;
        T one_over_scale2;
        T min_cost;
        int num_hypotheses, num_hypotheses_remaining, min_index;
        int cur_chunk_size, this_chunk_end;
        int trial;
        int seed;
        int sample_size = 4;
        
        ASSERT(hyp.size() >= 9*max_num_hypotheses, "hyp vector should be of size 9*max_num_hypotheses");
        ASSERT(tmp_i.size() >= num_points, "tmp_i vector should be of size num_points");
        ASSERT(hyp_costs.size() >= max_num_hypotheses, "hyp_costs vector should be of size max_num_hypotheses");
        
        // We need at least SAMPLE_SIZE points to sample from
        if(num_points < sample_size) {
            return false;
        }
        
        seed = 1234;
        
        hyp_perm = &tmp_i[0];

        one_over_scale2 = 1/sqr(scale);
        chunk_size = min2(chunk_size, num_points);
        
        // Fill arrays from [0)
        SequentialVector(hyp_perm, num_points, 0);

        // Shuffle the indices
        ArrayShuffle(hyp_perm, num_points, num_points, seed);

        // Compute a set of hypotheses
        for(trial = 0, num_hypotheses = 0;
            trial < max_trials && num_hypotheses < max_num_hypotheses;
            trial++) {
            
            // Shuffle the first SAMPLE_SIZE indices
            ArrayShuffle(hyp_perm, num_points, sample_size, seed);
            
            // Check if the four points are geometrically valid
            if(!Homography4PointsGeometricallyConsistent(&p[hyp_perm[0]<<1],
                                                         &p[hyp_perm[1]<<1],
                                                         &p[hyp_perm[2]<<1],
                                                         &p[hyp_perm[3]<<1],
                                                         &q[hyp_perm[0]<<1],
                                                         &q[hyp_perm[1]<<1],
                                                         &q[hyp_perm[2]<<1],
                                                         &q[hyp_perm[3]<<1])) {
                continue;
            }
            
            // Compute the homography
            if(!SolveHomography4Points(&hyp[num_hypotheses*9],
                                       &p[hyp_perm[0]<<1],
                                       &p[hyp_perm[1]<<1],
                                       &p[hyp_perm[2]<<1],
                                       &p[hyp_perm[3]<<1],
                                       &q[hyp_perm[0]<<1],
                                       &q[hyp_perm[1]<<1],
                                       &q[hyp_perm[2]<<1],
                                       &q[hyp_perm[3]<<1])) {
                continue;
            }
            
            // Check the test points
            if(num_test_points > 0) {
                if(!HomographyPointsGeometricallyConsistent(&hyp[num_hypotheses*9], test_points, num_test_points)) {
                    continue;
                }
            }
            
            num_hypotheses++;
        }
        
        // We fail if no hypotheses could be computed
        if(num_hypotheses == 0) {
            return false;
        }
        
        // Initialize the hypotheses costs
        for(int i = 0; i < num_hypotheses; i++) {
            hyp_costs[i].first = 0;
            hyp_costs[i].second = i;
        }
        
        num_hypotheses_remaining = num_hypotheses;
        cur_chunk_size = chunk_size;
        
        for(int i = 0;
            i < num_points && num_hypotheses_remaining > 2;
            i+=cur_chunk_size) {
            
            // Size of the current chunk
            cur_chunk_size = min2(chunk_size, num_points-i);
            
            // End of the current chunk
            this_chunk_end = i+cur_chunk_size;
            
            // Score each of the remaining hypotheses
            for(int j = 0; j < num_hypotheses_remaining; j++) {
                const T* H_cur = &hyp[hyp_costs[j].second*9];
                for(int k = i; k < this_chunk_end; k++) {
                    hyp_costs[j].first += CauchyProjectiveReprojectionCost(H_cur,
                                                                           &p[hyp_perm[k]<<1],
                                                                           &q[hyp_perm[k]<<1],
                                                                           one_over_scale2);
                }
            }
            
            // Cut out half of the hypotheses
            FastMedian(&hyp_costs[0], num_hypotheses_remaining);
            num_hypotheses_remaining = num_hypotheses_remaining>>1;
        }
        
        // Find the best hypothesis
        min_index = hyp_costs[0].second;
        min_cost = hyp_costs[0].first;
        for(int i = 1; i < num_hypotheses_remaining; i++) {
            if(hyp_costs[i].first < min_cost ) {
                min_cost = hyp_costs[i].first;
                min_index = hyp_costs[i].second;
            }
        }
        
        // Move the best hypothesis
        CopyVector9(H, &hyp[min_index*9]);
        NormalizeHomography(H);
        
        return true;
    }
    
    /**
     * Compute the Lie Basis Jacobian for the homography.
     *
     * @param[out] J 2x8 jacobian
     * @param[out] f error
     * @param[in] pp pp=H*p
     * @param[in] p point to linearize
     * @param[in] q
     */
    template<typename T>
    inline void HomographyLieJacobian(T J[16], T f[2], const T pp[2], const T p[2], const T q[2]) {
        // [ 1, 0, y,  0,  x,   -x,  -x^2, -x*y]
        // [ 0, 1,  0, x, -y, -2*y, -x*y,  -y^2]
        
        const T& x = p[0];
        const T& y = p[1];
        
        J[0] = 1;
        J[1] = 0;
        J[2] = y;
        J[3] = 0;
        J[4] = x;
        J[5] = -x;
        J[6] = -x*x;
        J[7] = -x*y;
        J[8] = 0;
        J[9] = 1;
        J[10] = 0;
        J[11] = x;
        J[12] = -y;
        J[13] = -2*y;
        J[14] = -x*y;
        J[15] = -y*y;
        
        f[0] = pp[0]-q[0];
        f[1] = pp[1]-q[1];
    }
    
    /**
     * Compute the Cauchy Derivative
     *
     * xp = sqrt(log(1+(x^+y^2)*one_over_scale2))*(x/sqrt(x^2+y^2))
     * yp = sqrt(log(1+(x^+y^2)*one_over_scale2))*(y/sqrt(x^2+y^2))
     *
     * d(xp)/x = d(xp)/f * df/dx
     * d(yp)/y = d(yp)/f * df/dy
     *
     * where f = log(1+(x^2+y^2)*one_over_scale2)/(x^2+y^2)
     *
     * d(xp)/f = x/(2*sqrt(f))
     * d(yp)/f = y/(2*sqrt(f))
     */
    template<typename T>
    inline void CauchyDerivative(T J_r[4], T fp[2], const T f[2], T one_over_scale2) {
        T dqdf[2];
        T dfdp[2];
        
        const T& x = f[0];
        const T& y = f[1];
        T x2 = x*x;
        T y2 = y*y;
        T r2 = x2+y2;
        
        bool fu_at_zero = false;
        
        if(r2 <= 0) {
            fu_at_zero = true;
        }
        else {
            T one_over_r2 = 1/r2;
            T t = 1+r2*one_over_scale2;
            T one_over_r2_times_t = 1/(r2*t);
            T fu = std::log(t)*one_over_r2;
            
            if(fu <= 0) {
                fu_at_zero = true;
            }
            else {
                T sqrt_fu = std::sqrt(fu);
                T fu_times_one_over_r2 = fu*one_over_r2;
                T one_over_denom = 1./(2.*sqrt_fu);
                
                // dqdf
                dqdf[0] = x*one_over_denom;
                dqdf[1] = y*one_over_denom;
                
                // dfdp
                dfdp[0] = 2*(one_over_scale2*x*one_over_r2_times_t - (x*fu_times_one_over_r2));
                dfdp[1] = 2*(one_over_scale2*y*one_over_r2_times_t - (y*fu_times_one_over_r2));
                
                // J_r
                J_r[0] = dqdf[0]*dfdp[0]+sqrt_fu;    J_r[1] = dqdf[0]*dfdp[1];
                J_r[2] = J_r[1];                     J_r[3] = dqdf[1]*dfdp[1]+sqrt_fu;
                
                // error
                fp[0] = sqrt_fu*f[0];
                fp[1] = sqrt_fu*f[1];
            }
        }
        
        if(fu_at_zero) {
            fp[0] = 0;
            fp[1] = 0;
            
            J_r[0] = std::sqrt(one_over_scale2);
            J_r[1] = 0;
            J_r[2] = 0;
            J_r[3] = J_r[0];
        }
    }
    
    /**
     * Compute the Homography Jacobian of the Post Multiplied Update equation.
     *
     * H_delta*H0
     */
    template<typename T>
    inline void RobustHomographyLieJacobianPostMultiply(T Jp[16],
                                                        T fp[2],
                                                        const T H[9],
                                                        const T p[2],
                                                        const T q[2],
                                                        T one_over_scale2) {
        T pp[2];
        T f[2];
        T J[16];
        T J_r[4];
        
        // pp=H*p
        MultiplyPointHomographyInhomogenous(pp[0], pp[1], H, p[0], p[1]);
        // Linearize at the point "pp"
        HomographyLieJacobian(J, f, pp, pp, q);
        CauchyDerivative(J_r, fp, f, one_over_scale2);
        
        // Jp = Jr*J
        Jp[0]  = J_r[0]*J[0];
        Jp[1]  =               J_r[1]*J[9];
        Jp[2]  = J_r[0]*J[2];
        Jp[3]  =               J_r[1]*J[11];
        Jp[4]  = J_r[0]*J[4] + J_r[1]*J[12];
        Jp[5]  = J_r[0]*J[5] + J_r[1]*J[13];
        Jp[6]  = J_r[0]*J[6] + J_r[1]*J[14];
        Jp[7]  = J_r[0]*J[7] + J_r[1]*J[15];
        Jp[8]  = J_r[2]*J[0];
        Jp[9]  =               J_r[3]*J[9];
        Jp[10] = J_r[2]*J[2];
        Jp[11] =               J_r[3]*J[11];
        Jp[12] = J_r[2]*J[4] + J_r[3]*J[12];
        Jp[13] = J_r[2]*J[5] + J_r[3]*J[13];
        Jp[14] = J_r[2]*J[6] + J_r[3]*J[14];
        Jp[15] = J_r[2]*J[7] + J_r[3]*J[15];
    }
    
    /**
     * Compute the normal equations for the post multiplied homography update.
     */
    template<typename T>
    void ComputeHomographyNormalEquationsPostMultiply(T JtJ[64],
                                                      T Jtr[8],
                                                      const T H[9],
                                                      const T* p,
                                                      const T* q,
                                                      int num_points,
                                                      T one_over_scale2) {
        T J[16];
        T fp[2];
        
        ZeroVector(JtJ, 64);
        ZeroVector(Jtr, 8);
        
        for(int i = 0; i < num_points; i++, p+=2, q+=2) {
            // Jacobian
            RobustHomographyLieJacobianPostMultiply(J, fp, H, p, q, one_over_scale2);
            
            // JtJ += J'*J
            MultiplyAndAccumulateAtA(JtJ, J, 2, 8);
            
            // Jtr += J'*r
            MultiplyAndAccumulateAtx(Jtr, J, 2, 8, fp);
        }
        
        SymmetricExtendUpperToLower(JtJ, 8);
        ScaleVector8<T>(Jtr, Jtr, -1);
    }
    
    /**
	 * Regularize the 8x8 system with Levenberg-Marquardt.
	 *
	 * JtJ+diag(lambda*diag(JtJ))
	 */
	template<typename T>
	inline void RegularizeLevenbergMarquardt8x8(T JtJ[64], const T in_JtJ[64], T lambda) {
		JtJ[0]  = in_JtJ[0]  + (lambda*in_JtJ[0]);
		JtJ[9]  = in_JtJ[9]  + (lambda*in_JtJ[9]);
		JtJ[18] = in_JtJ[18] + (lambda*in_JtJ[18]);
		JtJ[27] = in_JtJ[27] + (lambda*in_JtJ[27]);
		JtJ[36] = in_JtJ[36] + (lambda*in_JtJ[36]);
		JtJ[45] = in_JtJ[45] + (lambda*in_JtJ[45]);
		JtJ[54] = in_JtJ[54] + (lambda*in_JtJ[54]);
		JtJ[63] = in_JtJ[63] + (lambda*in_JtJ[63]);
	}
    
    /**
	 * This is the Lie Basis summation.
     * "Homography-based 2d visual tracking and servoing", S. Benhimane, E. Malis
	 */
	template<typename T>
	inline void LieAlgebraSum(T A[9], const T x[8]) {
		A[0] = x[4];	A[1] = x[2];		A[2] = x[0];
		A[3] = x[3];	A[4] = -x[4]-x[5];	A[5] = x[1];
		A[6] = x[6];	A[7] = x[7];		A[8] = x[5];
	}
    
    /**
	 * Compute the incremental update from the Lie Algebra Basis
	 */
	template<typename T>
	inline void IncrementalHomographyFromLieWeights(T H[9], const T x[8]) {
		LieAlgebraSum(H, x);
		
		// TODO: remove Eigen
		Eigen::Matrix<T, 3, 3> eigenMat;
		CopyVector9<T>(eigenMat.data(), H);
		Eigen::Matrix<T, 3, 3> matExp;
		matExp = eigenMat.exp();
		CopyVector9(H, matExp.data());
	}
    
    /**
	 * Update homography with projective motion
     *
     * Hp = (I+H)*H0
     *
     * @param[out] Hp
     * @param[in] H
     * @param[in] motion Lie weights
	 */
	template<typename T>
	inline void UpdateProjectiveMotionPostMultiply(T Hp[9], const T H[9], const T x0[8]) {
        T tmp[9];
		T H_delta[9];
		
        CopyVector9(tmp, H);
		IncrementalHomographyFromLieWeights(H_delta, x0);
		Multiply3x3_3x3(Hp, H_delta, tmp);
	}
    
    /**
     * Minimizes the Cauchy Robustified error between points H*p and q in the functional f(Hp-q).
     *
     * @param[in/out] H input homography should be the initial estimate
     * @param[in] p array of points in [x1,y1,x2,y2,...,xk,yk] format
     * @param[in] q array of points in [x1,y1,x2,y2,...,xk,yk] format
     * @param[in] type motion type
     * @param[in] scale Cauchy scale
     * @param[in] max_iterations max number of iterations
     * @param[in] max_stops max number of non-converging iterations before stopping
     * @parms[in] improvement min amount of improvement on each iteration causing stops to be incremented
     */
    template<typename T>
    bool PolishHomography(T H[9],
                           const T* p,
                           const T* q,
                           int num_points,
                           T scale = HOMOGRAPHY_DEFAULT_CAUCHY_SCALE,
                           int max_iterations = 500,
                           int max_stops = 20,
                           T improvement = 0.0001)
    {
        T one_over_scale2;
        T JtJ[64];
        T reg_JtJ[64];
        T Jtr[8];
        T delta[8];
        T cost, last_cost;
        bool update;
        T lambda;
        T Hp[9];
        int stops;
        int num_params;
        
        stops = 0;
        lambda = 0.01;
        update = true;
        one_over_scale2 = 1/(scale*scale);
        
        num_params = 8;
        
        last_cost = CauchyProjectiveReprojectionCost(H, p, q, num_points, one_over_scale2);
        
        int i;
        for(i = 0; i < max_iterations && stops < max_stops; i++) {
            if(update) {
                ComputeHomographyNormalEquationsPostMultiply(JtJ, Jtr, H, p, q, num_points, one_over_scale2);
                CopyVector(reg_JtJ, JtJ, num_params*num_params);
            }
            
            //
            // Solve the linear system
            //
            RegularizeLevenbergMarquardt8x8(reg_JtJ, JtJ, lambda);
            if(!SolvePositiveDefiniteSystem<T, 8>(delta, reg_JtJ, Jtr, 0)) {
                return false;
            }
            UpdateProjectiveMotionPostMultiply(Hp, H, delta);
			
            //
            // Compute robust error function
            //
            cost = CauchyProjectiveReprojectionCost(Hp, p, q, num_points, one_over_scale2);
            
			if(cost < last_cost) {
				CopyVector9(H, Hp);
				stops = (last_cost - cost) < improvement ? stops + 1 : 0;
				last_cost = cost;
                lambda = max2<T>(lambda*0.1, 0.000001);
				update = true;
			}
			else {
                lambda = min2<T>(lambda*10, 100000);
				stops++;
				update = false;
			}
        }
        
        return true;
    }
    
    /**
     * Robust homography estimation.
     */
    template<typename T>
    class RobustHomography {
    public:
        
        RobustHomography(T cauchyScale          = HOMOGRAPHY_DEFAULT_CAUCHY_SCALE,
                         int maxNumHypotheses   = HOMOGRAPHY_DEFAULT_NUM_HYPOTHESES,
                         int maxTrials          = HOMOGRAPHY_DEFAULT_MAX_TRIALS,
                         int chunkSize          = HOMOGRAPHY_DEFAULT_CHUNK_SIZE);
        ~RobustHomography() {}
        
        /**
         * Initalize the RANSAC parameters.
         */
        void init(T cauchyScale,
                  int maxNumHypotheses,
                  int maxTrials,
                  int chunkSize);
        
        /**
         * Find the homography from a set of 2D correspondences.
         */
        bool find(float H[9], const T* p, const T* q, int num_points);
        bool find(float H[9], const T* p, const T* q, int num_points, const T* test_points, int num_test_points);
        
    private:
        
        // Temporary memory for RANSAC
        std::vector<T> mHyp;
        std::vector<int> mTmpi;
        std::vector< std::pair<T, int> > mHypCosts;
        
        // RANSAC params
        T mCauchyScale;
        int mMaxNumHypotheses;
        int mMaxTrials;
        int mChunkSize;
        
    }; // RobustHomography
    
    template<typename T>
    RobustHomography<T>::RobustHomography(T cauchyScale,
                                          int maxNumHypotheses,
                                          int maxTrials,
                                          int chunkSize) {
        init(cauchyScale, maxNumHypotheses, maxTrials, chunkSize);
    }
    
    template<typename T>
    void RobustHomography<T>::init(T cauchyScale,
                                   int maxNumHypotheses,
                                   int maxTrials,
                                   int chunkSize) {
        mHyp.resize(9*maxNumHypotheses);
        mHypCosts.resize(maxNumHypotheses);
        
        mCauchyScale = cauchyScale;
        mMaxNumHypotheses = maxNumHypotheses;
        mMaxTrials = maxTrials;
        mChunkSize = chunkSize;
    }
    
    template<typename T>
    bool RobustHomography<T>::find(float H[9], const T* p, const T* q, int num_points) {
        mTmpi.resize(num_points);
        if(!PreemptiveRobustHomography<T>(H,
                                          p,
                                          q,
                                          num_points,
                                          0,
                                          0,
                                          mHyp,
                                          mTmpi,
                                          mHypCosts,
                                          mCauchyScale,
                                          mMaxNumHypotheses,
                                          mMaxTrials,
                                          mChunkSize)) {
            return false;
        }
        
        return PolishHomography(H, p, q, num_points);
    }
    
    template<typename T>
    bool RobustHomography<T>::find(float H[9], const T* p, const T* q, int num_points, const T* test_points, int num_test_points) {
        mTmpi.resize(num_points);
        return PreemptiveRobustHomography<T>(H,
                                             p,
                                             q,
                                             num_points,
                                             test_points,
                                             num_test_points,
                                             mHyp,
                                             mTmpi,
                                             mHypCosts,
                                             mCauchyScale,
                                             mMaxNumHypotheses,
                                             mMaxTrials,
                                             mChunkSize);
    }
    
} // vision
