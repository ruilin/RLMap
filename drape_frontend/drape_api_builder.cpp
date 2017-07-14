#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/colored_symbol_shape.hpp"
#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/batcher.hpp"

#include "indexer/feature_decl.hpp"

#include "base/string_utils.hpp"

namespace
{

void BuildText(string const & str, dp::FontDecl const & font, m2::PointD const & position,
               m2::PointD const & center, ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher)
{
  gui::StaticLabel::LabelResult result;
  gui::StaticLabel::CacheStaticText(str, "\n", dp::LeftTop, font, textures, result);
  glsl::vec2 const pt = glsl::ToVec2(df::MapShape::ConvertToLocal(position, center, df::kShapeCoordScalar));
  for (gui::StaticLabel::Vertex & v : result.m_buffer)
    v.m_position = glsl::vec3(pt, 0.0f);

  dp::AttributeProvider provider(1 /* streamCount */, static_cast<uint32_t>(result.m_buffer.size()));
  provider.InitStream(0 /* streamIndex */, gui::StaticLabel::Vertex::GetBindingInfo(),
                      make_ref(result.m_buffer.data()));

  batcher.InsertListOfStrip(result.m_state, make_ref(&provider), dp::Batcher::VertexPerQuad);
}

} // namespace

namespace df
{

void DrapeApiBuilder::BuildLines(DrapeApi::TLines const & lines, ref_ptr<dp::TextureManager> textures,
                                 vector<drape_ptr<DrapeApiRenderProperty>> & properties)
{
  properties.reserve(lines.size());

  uint32_t constexpr kMaxSize = 5000;
  uint32_t constexpr kFontSize = 14;
  FeatureID fakeFeature;

  for (auto const & line : lines)
  {
    string id = line.first;
    DrapeApiLineData const & data = line.second;
    m2::RectD rect;
    for (m2::PointD p : data.m_points)
      rect.Add(p);

    dp::Batcher batcher(kMaxSize, kMaxSize);
    auto property = make_unique_dp<DrapeApiRenderProperty>();
    property->m_center = rect.Center();
    {
      dp::SessionGuard guard(batcher, [&property, id](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
      {
        property->m_id = id;
        property->m_buckets.push_back(make_pair(state, move(b)));
      });

      m2::SharedSpline spline(data.m_points);
      LineViewParams lvp;
      lvp.m_tileCenter = property->m_center;
      lvp.m_depth = 0.0f;
      lvp.m_minVisibleScale = 1;
      lvp.m_cap = dp::RoundCap;
      lvp.m_color = data.m_color;
      lvp.m_width = data.m_width;
      lvp.m_join = dp::RoundJoin;
      LineShape(spline, lvp).Draw(make_ref(&batcher), textures);

      if (data.m_showPoints)
      {
        ColoredSymbolViewParams cvp;
        cvp.m_tileCenter = property->m_center;
        cvp.m_depth = 0.0f;
        cvp.m_minVisibleScale = 1;
        cvp.m_shape = ColoredSymbolViewParams::Shape::Circle;
        cvp.m_color = data.m_color;
        cvp.m_radiusInPixels = data.m_width * 2.0f;
        for (m2::PointD const & pt : data.m_points)
        {
          ColoredSymbolShape(m2::PointF(pt), cvp, TileKey(), 0 /* textIndex */,
                             false /* need overlay */).Draw(make_ref(&batcher), textures);
        }
      }

      if (data.m_markPoints || data.m_showId)
      {
        dp::FontDecl font(data.m_color, kFontSize);
        size_t index = 0;
        for (m2::PointD const & pt : data.m_points)
        {
          if (index > 0 && !data.m_markPoints) break;

          string s;
          if (data.m_markPoints)
            s = strings::to_string(index) + ((data.m_showId && index == 0) ? (" (" + id + ")") : "");
          else
            s = id;

          BuildText(s, font, pt, property->m_center, textures, batcher);
          index++;
        }
      }
    }

    if (!property->m_buckets.empty())
      properties.push_back(move(property));
  }
}

} // namespace df
