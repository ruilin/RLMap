#pragma once

#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace routing
{
class SegmentedRoute final
{
public:
  class Step final
  {
  public:
    Step() = default;
    Step(Segment const & segment, m2::PointD const & point) : m_segment(segment), m_point(point) {}

    Segment const & GetSegment() const { return m_segment; }
    m2::PointD const & GetPoint() const { return m_point; }

  private:
    Segment const m_segment;
    // The front point of segment
    m2::PointD const m_point = m2::PointD::Zero();
  };

  SegmentedRoute(m2::PointD const & start, m2::PointD const & finish);

  void AddStep(Segment const & segment, m2::PointD const & point)
  {
    m_steps.emplace_back(segment, point);
  }

  double CalcDistance(m2::PointD const & point) const;

  m2::PointD const & GetStart() const { return m_start; }
  m2::PointD const & GetFinish() const { return m_finish; }
  std::vector<Step> const & GetSteps() const { return m_steps; }
  bool IsEmpty() const { return m_steps.empty(); }

private:
  m2::PointD const m_start;
  m2::PointD const m_finish;
  std::vector<Step> m_steps;
};
}  // namespace routing
