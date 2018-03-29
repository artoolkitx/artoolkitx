//
//  binary_hierarchical_clustering.h
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

#include "kmedoids.h"

#include <unordered_map>
#include <queue>

namespace vision {
    
    // Forward declartion
    template<int NUM_BYTES_PER_FEATURE>
    class Node;
    
    /**
     * The nodes in the tree are sorted as they are visited when a QUERY is done. This class
     * represents an entry in a priority queue to revisit certains nodes in a back-trace.
     */
    template<int NUM_BYTES_PER_FEATURE>
    class PriorityQueueItem {
    public:
        
        typedef Node<NUM_BYTES_PER_FEATURE> node_t;
        
        PriorityQueueItem()
        : mNode(NULL)
        , mDistance(0) {}
        PriorityQueueItem(const node_t* node, unsigned int dist)
        : mNode(node), mDistance(dist) {}
        ~PriorityQueueItem() {}
    
        /**
         * Get pointer to node.
         */
        inline const node_t* node() const { return mNode; }
        
        /**
         * Distance to cluster center.
         */
        inline unsigned int dist() const { return mDistance; }
        
        /**
         * Operator for sorting the queue. Smallest item is always the first.
         */
        bool operator<(const PriorityQueueItem& item) const {
            return mDistance > item.mDistance;
        }
        
    private:
    
        // Pointer to the node
        const node_t* mNode;
    
        // Distance from cluster center
        unsigned int mDistance;
    
    }; // PriorityQueueItem
    
    /**
     * Represents a node in the tree.
     */
    template<int NUM_BYTES_PER_FEATURE>
    class Node {
    public:
        
        typedef int node_id_t;
        typedef Node<NUM_BYTES_PER_FEATURE> node_t;
        typedef PriorityQueueItem<NUM_BYTES_PER_FEATURE> queue_item_t;
        typedef std::priority_queue<queue_item_t> queue_t;
        
        Node(node_id_t id);
        Node(node_id_t id, const unsigned char* center);
        ~Node() {
            for(size_t i = 0; i < mChildren.size(); i++) {
                delete mChildren[i];
            }
        }
        
        /**
         * @return Node id
         */
        inline node_id_t id() const { return mId; }
        
        /**
         * Set/Get leaf flag
         */
        inline void leaf(bool b) { mLeaf = b; }
        inline bool leaf() const { return mLeaf; }
        
        /**
         * @return Get children
         */
        inline std::vector<node_t*>& children() { return mChildren; }
        inline const std::vector<node_t*>& children() const { return mChildren; }
        
        /**
         * @return Get the reverse index
         */
        inline std::vector<int>& reverseIndex() { return mReverseIndex; }
        inline const std::vector<int>& reverseIndex() const { return mReverseIndex; }
        
        /**
         * Get a queue of all the children nodes sorted by distance from node center.
         */
        inline void nearest(std::vector<const node_t*>& nodes,
                            queue_t& queue,
                            const unsigned char* feature) const {
            unsigned int mind = std::numeric_limits<unsigned int>::max();
            int mini = -1;
            
            // Compute the distance to each cluster center
            std::vector<queue_item_t> v(mChildren.size());
            for(size_t i = 0; i < v.size(); i++) {
                unsigned int d = HammingDistance<NUM_BYTES_PER_FEATURE>(mChildren[i]->mCenter, feature);
                v[i] = queue_item_t(mChildren[i], d);
                if(d < mind) {
                    mind = d;
                    mini = (int)i;
                }
            }
            ASSERT(mini != -1, "Minimum index not set");
            
            // Store the closest child
            nodes.push_back(mChildren[mini]);
            
            // Any nodes that are the SAME distance as the minimum node are added
            // to the output vector, otherwise it's pushed onto the queue.
            for(size_t i = 0; i < v.size(); i++) {
                if(i == mini) {
                    continue;
                } else if(v[i].dist() == v[mini].dist()) {
                    nodes.push_back(mChildren[i]);
                } else {
                    queue.push(v[i]);
                }
            }
        }
        
    private:
        
        // ID of the node
        node_id_t mId;
        
        // Feature center
        unsigned char mCenter[NUM_BYTES_PER_FEATURE];
        
        // True if a leaf node
        bool mLeaf;
        
        // Child nodes
        std::vector<node_t*> mChildren;
        
        // Index of the features at this node
        std::vector<int> mReverseIndex;
        
    }; // Node

    template<int NUM_BYTES_PER_FEATURE>
    Node<NUM_BYTES_PER_FEATURE>::Node(node_id_t id)
    : mId(id)
    , mLeaf(true) {
        ZeroVector(mCenter, NUM_BYTES_PER_FEATURE);
    }
            
    template<int NUM_BYTES_PER_FEATURE>
    Node<NUM_BYTES_PER_FEATURE>::Node(node_id_t id, const unsigned char* center)
    : mId(id)
    , mLeaf(true) {
        CopyVector(mCenter, center, NUM_BYTES_PER_FEATURE);
    }
    
    /**
     * Implements hierarchical clustering for binary features. This can
     * be used for fast nearest neighbor search.
     */
    template<int NUM_BYTES_PER_FEATURE>
    class BinaryHierarchicalClustering {
    public:
        
        typedef Node<NUM_BYTES_PER_FEATURE> node_t;
        typedef std::unique_ptr<node_t> node_ptr_t;
        typedef BinarykMedoids<NUM_BYTES_PER_FEATURE> kmedoids_t;
        typedef std::unordered_map<int, std::vector<int> > cluster_map_t;
        
        typedef PriorityQueueItem<NUM_BYTES_PER_FEATURE> queue_item_t;
        typedef std::priority_queue<queue_item_t> queue_t;
        
        BinaryHierarchicalClustering();
        ~BinaryHierarchicalClustering() {}
        
        /**
         * Build the tree.
         */
        void build(const unsigned char* features, int num_features);
        
        /**
         * Query the tree for a reverse index.
         */
        int query(const unsigned char* feature) const;
        
        /**
         * @return Reverse index after a QUERY.
         */
        inline const std::vector<int>& reverseIndex() const { return mQueryReverseIndex; }

        /**
         * Set/Get number of hypotheses
         */
        inline void setNumHypotheses(int n) { mBinarykMedoids.setNumHypotheses(n); }
        inline int numHypotheses() const { return mBinarykMedoids.numHypotheses(); }
        
        /**
         * Set/Get number of center.
         */
        inline void setNumCenters(int k) { mBinarykMedoids.setk(k); }
        inline int numCenters() const { return mBinarykMedoids.k(); }
        
        /**
         * Set/Get max nodes to pop from queue.
         */
        inline void setMaxNodesToPop(int n) { mMaxNodesToPop = n; }
        inline int maxNodesPerPop() const { return mMaxNodesToPop; }
        
        /**
         * Set/Get minimum number of features per node.
         */
        inline void setMinFeaturesPerNode(int n) { mMinFeaturePerNode = n; }
        inline int minFeaturesPerNode() const { return mMinFeaturePerNode; }
        
    private:
        
        // Random number seed
        int mRandSeed;
        
        // Counter for node id's
        int mNextNodeId;
        
        // Root node
        node_ptr_t mRoot;
        
        // Clustering algorithm
        kmedoids_t mBinarykMedoids;
        
        // Reverse index for query
        mutable std::vector<int> mQueryReverseIndex;
        
        // Node queue
        mutable queue_t mQueue;
        
        // Number of nodes popped off the priority queue
        mutable int mNumNodesPopped;
        
        // Maximum nodes to pop off the priority queue
        int mMaxNodesToPop;
        
        // Minimum number of feature at a node
        int mMinFeaturePerNode;
        
        /**
         * Get the next node id
         */
        inline int nextNodeId() {
            return mNextNodeId++;
        }
        
        /**
         * Private build function with a set of indices.
         */
        void build(const unsigned char* features, int num_features, const int* indices, int num_indices);
        
        /**
         * Recursive function to build the tree.
         */
        void build(node_t* node, const unsigned char* features, int num_features, const int* indices, int num_indices);
        
        /**
         * Recursive function query function.
         */
        void query(queue_t& queue, const node_t* node, const unsigned char* feature) const;
        
    }; // BinaryHierarchicalClustering

    template<int NUM_BYTES_PER_FEATURE>
    BinaryHierarchicalClustering<NUM_BYTES_PER_FEATURE>::BinaryHierarchicalClustering()
    : mRandSeed(1234)
    , mNextNodeId(0)
    , mBinarykMedoids(mRandSeed)
    , mNumNodesPopped(0)
    , mMaxNodesToPop(0)
    , mMinFeaturePerNode(16) {
        mBinarykMedoids.setk(8);
        mBinarykMedoids.setNumHypotheses(1);
    }
    
    template<int NUM_BYTES_PER_FEATURE>
    void BinaryHierarchicalClustering<NUM_BYTES_PER_FEATURE>::build(const unsigned char* features, int num_features) {
        std::vector<int> indices(num_features);
        for(size_t i = 0; i < indices.size(); i++) {
            indices[i] = (int)i;
        }
        build(features, num_features, &indices[0], (int)indices.size());
    }
    
    template<int NUM_BYTES_PER_FEATURE>
    void BinaryHierarchicalClustering<NUM_BYTES_PER_FEATURE>::build(const unsigned char* features, int num_features, const int* indices, int num_indices) {
        mRoot.reset(new node_t(nextNodeId()));
        mRoot->leaf(false);
        build(mRoot.get(), features, num_features, indices, num_indices);
    }
    
    template<int NUM_BYTES_PER_FEATURE>
    void BinaryHierarchicalClustering<NUM_BYTES_PER_FEATURE>::build(node_t* node, const unsigned char* features, int num_features, const int* indices, int num_indices) {
        // Check if there are enough features to cluster.
        // If not, then assign all features to the same cluster.
        if(num_indices <= max2(mBinarykMedoids.k(), mMinFeaturePerNode)) {
            node->leaf(true);
            node->reverseIndex().resize(num_indices);
            for(int i = 0; i < num_indices; i++) {
                node->reverseIndex()[i] = indices[i];
            }
        } else {
            cluster_map_t cluster_map;
            
            // Perform clustering
            mBinarykMedoids.assign(features, num_features, indices, num_indices);
            
            // Get a list of features for each cluster center
            const std::vector<int>& assignment = mBinarykMedoids.assignment();
            ASSERT(assignment.size() == num_indices, "Assignment size wrong");
            for(size_t i = 0; i < assignment.size(); i++) {
                ASSERT(assignment[i] != -1, "Assignment is invalid");
                ASSERT(assignment[i] < num_indices, "Assignment out of range");
                ASSERT(indices[assignment[i]] < num_features, "Assignment out of range");
                
                cluster_map[indices[assignment[i]]].push_back(indices[i]);
            }

            // If there is only 1 cluster then make this node a leaf
            if(cluster_map.size() == 1) {
                node->leaf(true);
                node->reverseIndex().resize(num_indices);
                for(int i = 0; i < num_indices; i++) {
                    node->reverseIndex()[i] = indices[i];
                }
                return;
            }
            
            // Create a new node for each cluster center
            node->children().reserve(cluster_map.size());
            for(cluster_map_t::const_iterator it = cluster_map.begin();
                it != cluster_map.end();
                it++) {
                ASSERT(it->second.size() != 0, "Cluster must have atleaset 1 feature");
                
                node_t* new_node = new node_t(nextNodeId(),
                                              &features[it->first*NUM_BYTES_PER_FEATURE]);
                new_node->leaf(false);
                
                // Make the new node a child of the input node
                node->children().push_back(new_node);
                
                // Recursively build the tree
                const std::vector<int>& v = it->second;
                build(new_node, features, num_features, &v[0], (int)v.size());
            }
        }
    }
    
    template<int NUM_BYTES_PER_FEATURE>
    int BinaryHierarchicalClustering<NUM_BYTES_PER_FEATURE>::query(const unsigned char* feature) const {
        ASSERT(mRoot.get(), "Root cannot be NULL");
        
        mNumNodesPopped = 0;
        mQueryReverseIndex.clear();
        
        while(!mQueue.empty()) {
            mQueue.pop();
        }
        
        query(mQueue, mRoot.get(), feature);
        
        return (int)mQueryReverseIndex.size();
    }
    
    template<int NUM_BYTES_PER_FEATURE>
    void BinaryHierarchicalClustering<NUM_BYTES_PER_FEATURE>::query(queue_t& queue,
                                                                    const node_t* node,
                                                                    const unsigned char* feature) const {
        if(node->leaf()) {
            // Insert all the leaf indices into the query index
            mQueryReverseIndex.insert(mQueryReverseIndex.end(),
                                      node->reverseIndex().begin(),
                                      node->reverseIndex().end());
            return;
        } else {
            std::vector<const node_t*> nodes;
            node->nearest(nodes, queue, feature);
            for(size_t i = 0; i < nodes.size(); i++) {
                query(queue, nodes[i], feature);
            }
            
            // Pop a node from the queue
            if(mNumNodesPopped < mMaxNodesToPop && !queue.empty()) {
                const node_t* q = queue.top().node();
                queue.pop();
                mNumNodesPopped++;
                query(queue, q, feature);
            }
        }
    }
    
} // vision