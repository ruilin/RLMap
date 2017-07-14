#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "std/vector.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"

namespace routing
{
/// Single graph node representation for routing task
struct FeatureGraphNode
{
  PhantomNode node;
  OsrmMappingTypes::FtSeg segment;
  m2::PointD segmentPoint;
  Index::MwmId mwmId;

  /*!
  * \brief fill FeatureGraphNode with values.
  * \param nodeId osrm node idetifier.
  * \param isStartNode true if this node will first in the path.
  * \param mwmName @nodeId refers node on the graph of this map.
  */
  FeatureGraphNode(NodeID const nodeId, bool const isStartNode, Index::MwmId const & id);

  FeatureGraphNode(NodeID const nodeId, NodeID const reverseNodeId, bool const isStartNode,
                   Index::MwmId const & id);

  /// \brief Invalid graph node constructor
  FeatureGraphNode();
};

/*!
 * \brief The RawPathData struct is our representation of OSRM PathData struct.
 * I use it for excluding dependency from OSRM. Contains OSRM node ID and it's weight.
 */
struct RawPathData
{
  NodeID node;
  EdgeWeight segmentWeight;  // Time in tenths of a second to pass |node|.

  RawPathData() : node(SPECIAL_NODEID), segmentWeight(INVALID_EDGE_WEIGHT) {}

  RawPathData(NodeID node, EdgeWeight segmentWeight)
      : node(node), segmentWeight(segmentWeight)
  {
  }
};

//@todo (dragunov) make proper name
using TRoutingNodes = vector<FeatureGraphNode>;
using TRawDataFacade = OsrmRawDataFacade<QueryEdge::EdgeData>;

/*!
   * \brief FindWeightsMatrix Find weights matrix from sources to targets. WARNING it finds only
 * weights, not pathes.
   * \param sources Sources graph nodes vector. Each source is the representation of a start OSRM
 * node.
   * \param targets Targets graph nodes vector. Each target is the representation of a finish OSRM
 * node.
   * \param facade Osrm data facade reference.
   * \param packed Result vector with weights. Source nodes are rows.
   * cost(source1 -> target1) cost(source1 -> target2) cost(source2 -> target1) cost(source2 ->
 * target2)
   */
void FindWeightsMatrix(TRoutingNodes const & sources, TRoutingNodes const & targets,
                       TRawDataFacade & facade, vector<EdgeWeight> & result);
}  // namespace routing
