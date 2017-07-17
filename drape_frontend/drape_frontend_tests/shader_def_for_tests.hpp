#pragma once

#include "drape/drape_global.hpp"
#include "drape/gpu_program_info.hpp"

#include "std/target_os.hpp"

#include <map>
#include <string>
#include <vector>

namespace gpu
{
#if defined(OMIM_OS_DESKTOP) && !defined(COMPILER_TESTS)
  #define LOW_P "" 
  #define MEDIUM_P "" 
  #define HIGH_P "" 
  #define MAXPREC_P "" 
  #define SHADER_VERSION "#version 150 core \n"
#else
  #define LOW_P "lowp"
  #define MEDIUM_P "mediump"
  #define HIGH_P "highp"
  #define MAXPREC_P "MAXPREC"
  #define SHADER_VERSION "#version 300 es \n"
#endif

extern int const TEXTURING_PROGRAM;
extern int const MASKED_TEXTURING_PROGRAM;
extern int const COLORED_SYMBOL_PROGRAM;
extern int const TEXT_OUTLINED_PROGRAM;
extern int const TEXT_PROGRAM;
extern int const TEXT_FIXED_PROGRAM;
extern int const TEXT_OUTLINED_GUI_PROGRAM;
extern int const AREA_PROGRAM;
extern int const AREA_OUTLINE_PROGRAM;
extern int const AREA_3D_PROGRAM;
extern int const AREA_3D_OUTLINE_PROGRAM;
extern int const LINE_PROGRAM;
extern int const CAP_JOIN_PROGRAM;
extern int const DASHED_LINE_PROGRAM;
extern int const PATH_SYMBOL_LINE;
extern int const HATCHING_AREA_PROGRAM;
extern int const TEXTURING_GUI_PROGRAM;
extern int const RULER_PROGRAM;
extern int const ACCURACY_PROGRAM;
extern int const MY_POSITION_PROGRAM;
extern int const BOOKMARK_PROGRAM;
extern int const ROUTE_PROGRAM;
extern int const ROUTE_DASH_PROGRAM;
extern int const ROUTE_ARROW_PROGRAM;
extern int const CIRCLE_POINT_PROGRAM;
extern int const DEBUG_RECT_PROGRAM;
extern int const SCREEN_QUAD_PROGRAM;
extern int const ARROW_3D_PROGRAM;
extern int const ARROW_3D_SHADOW_PROGRAM;
extern int const ARROW_3D_OUTLINE_PROGRAM;
extern int const COLORED_SYMBOL_BILLBOARD_PROGRAM;
extern int const TEXTURING_BILLBOARD_PROGRAM;
extern int const MASKED_TEXTURING_BILLBOARD_PROGRAM;
extern int const TEXT_OUTLINED_BILLBOARD_PROGRAM;
extern int const TEXT_BILLBOARD_PROGRAM;
extern int const TEXT_FIXED_BILLBOARD_PROGRAM;
extern int const BOOKMARK_BILLBOARD_PROGRAM;
extern int const TRAFFIC_PROGRAM;
extern int const TRAFFIC_LINE_PROGRAM;
extern int const SMAA_EDGES_PROGRAM;
extern int const SMAA_BLENDING_WEIGHT_PROGRAM;
extern int const SMAA_FINAL_PROGRAM;

#if !defined(COMPILER_TESTS)
class ShaderMapper : public GpuProgramGetter
{
public:
  ShaderMapper(dp::ApiVersion apiVersion);
  GpuProgramInfo const & GetProgramInfo(int program) const override;
private:
  std::map<int, gpu::GpuProgramInfo> m_mapping;
};
#endif

#if defined(COMPILER_TESTS)
using ShadersEnum = std::vector<std::pair<std::string, std::string>>;
extern ShadersEnum GetVertexShaders(dp::ApiVersion apiVersion);
extern ShadersEnum GetFragmentShaders(dp::ApiVersion apiVersion);
#endif
} // namespace gpu
