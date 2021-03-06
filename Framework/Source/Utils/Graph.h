/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once

namespace Falcor
{
    template<typename NodeType, typename EdgeType, typename EdgeHashType = std::hash<EdgeType>>
    class Graph
    {
    public:
        using SharedPtr = std::shared_ptr<Graph>;
        using SharedConstPtr = std::shared_ptr<const Graph>;

        static SharedPtr create()
        {
            return SharedPtr(new Graph());
        }

        void gotoStart() { mCurrentNode = 0; }

        bool isEdgeExists(const EdgeType& e) const 
        {
            return (getEdgeIt(e) != mGraph[mCurrentNode].edges.end());
        }

        bool walk(const EdgeType& e)
        {
            if (isEdgeExists(e))
            {
                mCurrentNode = getEdgeIt(e)->second;
                return true;
            }
            else
            {
                uint32_t newIndex = (uint32_t)mGraph.size();
                mGraph[mCurrentNode].edges[e] = newIndex;
                mGraph.push_back(Node());
                mCurrentNode = newIndex;
                return false;
            }
        }
        
        const NodeType& getCurrentNode() const
        {
            return mGraph[mCurrentNode].data;
        }

        void setCurrentNodeData(const NodeType& data)
        {
            mGraph[mCurrentNode].data = data;
        }
    private:
        Graph() : mGraph(1) {}

        using edge_map = std::unordered_map<EdgeType, uint32_t, EdgeHashType>;
        
        const auto getEdgeIt(const EdgeType& e) const
        {
            const Node& n = mGraph[mCurrentNode];
            return n.edges.find(e);
        }

        struct Node
        {
            NodeType data = { 0 };
            edge_map edges;
        };

        std::vector<Node> mGraph;
        uint32_t mCurrentNode = 0;
    };
}