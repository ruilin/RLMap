#include "shader_def.hpp"

#include "base/assert.hpp"

#include <unordered_map>
#include <utility>

namespace gpu
{
static char const TEXT_OUTLINED_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoord; \n\
  attribute vec2 a_outlineColorTexCoord; \n\
  attribute vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_isOutlinePass; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  #endif \n\
  varying vec2 v_maskTexCoord; \n\
  const float BaseDepthShift = -10.0; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float isOutline = step(0.5, u_isOutlinePass); \n\
    float notOutline = 1.0 - isOutline; \n\
    float depthShift = BaseDepthShift * isOutline; \n\
    vec4 pos = (vec4(a_position.xyz, 1) + vec4(0.0, 0.0, depthShift, 0.0)) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    vec2 colorTexCoord = a_colorTexCoord * notOutline + a_outlineColorTexCoord * isOutline; \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const GLES3_TEXT_OUTLINED_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoord; \n\
  in vec2 a_outlineColorTexCoord; \n\
  in vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_isOutlinePass; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoord; \n\
  #endif \n\
  out vec2 v_maskTexCoord; \n\
  const float BaseDepthShift = -10.0; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float isOutline = step(0.5, u_isOutlinePass); \n\
    float notOutline = 1.0 - isOutline; \n\
    float depthShift = BaseDepthShift * isOutline; \n\
    vec4 pos = (vec4(a_position.xyz, 1) + vec4(0.0, 0.0, depthShift, 0.0)) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    vec2 colorTexCoord = a_colorTexCoord * notOutline + a_outlineColorTexCoord * isOutline; \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const CIRCLE_POINT_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_normal; \n\
  attribute vec3 a_position; \n\
  attribute vec4 a_color; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec3 v_radius; \n\
  varying vec4 v_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec3 radius = a_normal * a_position.z; \n\
    vec4 pos = vec4(a_position.xy, 0, 1) * modelView; \n\
    vec4 shiftedPos = vec4(radius.xy, 0, 0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_radius = radius; \n\
    v_color = a_color; \n\
  } \n\
";

static char const GLES3_CIRCLE_POINT_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_normal; \n\
  in vec3 a_position; \n\
  in vec4 a_color; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec3 v_radius; \n\
  out vec4 v_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec3 radius = a_normal * a_position.z; \n\
    vec4 pos = vec4(a_position.xy, 0, 1) * modelView; \n\
    vec4 shiftedPos = vec4(radius.xy, 0, 0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_radius = radius; \n\
    v_color = a_color; \n\
  } \n\
";

static char const TEXT_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoord; \n\
  attribute vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  #endif \n\
  varying vec2 v_maskTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
   \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = a_colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const GLES3_TEXT_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoord; \n\
  in vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoord; \n\
  #endif \n\
  out vec2 v_maskTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
   \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = a_colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const AREA3D_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec3 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  varying vec2 v_colorTexCoords; \n\
  varying float v_intensity; \n\
  const vec4 lightDir = vec4(1.0, 0.0, 3.0, 0.0); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1.0) * modelView; \n\
   \n\
    vec4 normal = vec4(a_position + a_normal, 1.0) * modelView; \n\
    normal.xyw = (normal * projection).xyw; \n\
    normal.z = normal.z * zScale; \n\
    pos.xyw = (pos * projection).xyw; \n\
    pos.z = a_position.z * zScale; \n\
   \n\
    v_intensity = max(0.0, -dot(normalize(lightDir), normalize(normal - pos))); \n\
    gl_Position = pivotTransform * pos; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_AREA3D_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec3 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  out vec2 v_colorTexCoords; \n\
  out float v_intensity; \n\
  const vec4 lightDir = vec4(1.0, 0.0, 3.0, 0.0); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1.0) * modelView; \n\
   \n\
    vec4 normal = vec4(a_position + a_normal, 1.0) * modelView; \n\
    normal.xyw = (normal * projection).xyw; \n\
    normal.z = normal.z * zScale; \n\
    pos.xyw = (pos * projection).xyw; \n\
    pos.z = a_position.z * zScale; \n\
   \n\
    v_intensity = max(0.0, -dot(normalize(lightDir), normalize(normal - pos))); \n\
    gl_Position = pivotTransform * pos; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const MY_POSITION_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform vec3 u_position; \n\
  uniform float u_azimut; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float sinV = sin(u_azimut); \n\
    float cosV = cos(u_azimut); \n\
    mat4 rotation; \n\
    rotation[0] = vec4(cosV, sinV, 0.0, 0.0); \n\
    rotation[1] = vec4(-sinV, cosV, 0.0, 0.0); \n\
    rotation[2] = vec4(0.0, 0.0, 1.0, 0.0); \n\
    rotation[3] = vec4(0.0, 0.0, 0.0, 1.0); \n\
    vec4 pos = vec4(u_position, 1.0) * modelView; \n\
    vec4 normal = vec4(a_normal, 0, 0); \n\
    vec4 shiftedPos = normal * rotation + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_MY_POSITION_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform vec3 u_position; \n\
  uniform float u_azimut; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float sinV = sin(u_azimut); \n\
    float cosV = cos(u_azimut); \n\
    mat4 rotation; \n\
    rotation[0] = vec4(cosV, sinV, 0.0, 0.0); \n\
    rotation[1] = vec4(-sinV, cosV, 0.0, 0.0); \n\
    rotation[2] = vec4(0.0, 0.0, 1.0, 0.0); \n\
    rotation[3] = vec4(0.0, 0.0, 0.0, 1.0); \n\
    vec4 pos = vec4(u_position, 1.0) * modelView; \n\
    vec4 normal = vec4(a_normal, 0, 0); \n\
    vec4 shiftedPos = normal * rotation + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const ARROW3D_OUTLINE_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying float v_intensity; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 resColor = vec4(u_color.rgb, u_color.a * smoothstep(0.7, 1.0, v_intensity)); \n\
    gl_FragColor = samsungGoogleNexusWorkaround(resColor); \n\
  } \n\
";

static char const GLES3_ARROW3D_OUTLINE_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in float v_intensity; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 resColor = vec4(u_color.rgb, u_color.a * smoothstep(0.7, 1.0, v_intensity)); \n\
    v_FragColor = samsungGoogleNexusWorkaround(resColor); \n\
  } \n\
";

static char const SMAA_FINAL_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_blendingWeightTex; \n\
  uniform vec4 u_framebufferMetrics; \n\
  varying vec2 v_colorTexCoords; \n\
  varying vec4 v_offset; \n\
  #ifdef GLES3 \n\
    #define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0) \n\
  #else \n\
    #define SMAASampleLevelZero(tex, coord) texture2D(tex, coord) \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 a; \n\
    a.x = texture2D(u_blendingWeightTex, v_offset.xy).a; // Right \n\
    a.y = texture2D(u_blendingWeightTex, v_offset.zw).g; // Top \n\
    a.wz = texture2D(u_blendingWeightTex, v_colorTexCoords).xz; // Bottom / Left \n\
    if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) < 1e-5) \n\
    { \n\
      gl_FragColor = texture2D(u_colorTex, v_colorTexCoords); \n\
    } \n\
    else \n\
    { \n\
      vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w); \n\
      vec2 blendingWeight = a.yw; \n\
      if (max(a.x, a.z) > max(a.y, a.w)) \n\
      { \n\
        blendingOffset = vec4(a.x, 0.0, a.z, 0.0); \n\
        blendingWeight = a.xz; \n\
      } \n\
      blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0)); \n\
      vec4 bc = blendingOffset * vec4(u_framebufferMetrics.xy, -u_framebufferMetrics.xy); \n\
      bc += v_colorTexCoords.xyxy; \n\
      vec4 color = blendingWeight.x * SMAASampleLevelZero(u_colorTex, bc.xy); \n\
      color += blendingWeight.y * SMAASampleLevelZero(u_colorTex, bc.zw); \n\
      gl_FragColor = color; \n\
    } \n\
  } \n\
";

static char const GLES3_SMAA_FINAL_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_blendingWeightTex; \n\
  uniform vec4 u_framebufferMetrics; \n\
  in vec2 v_colorTexCoords; \n\
  in vec4 v_offset; \n\
  #ifdef GLES3 \n\
    #define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0) \n\
  #else \n\
    #define SMAASampleLevelZero(tex, coord) texture(tex, coord) \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 a; \n\
    a.x = texture(u_blendingWeightTex, v_offset.xy).a; // Right \n\
    a.y = texture(u_blendingWeightTex, v_offset.zw).g; // Top \n\
    a.wz = texture(u_blendingWeightTex, v_colorTexCoords).xz; // Bottom / Left \n\
    if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) < 1e-5) \n\
    { \n\
      v_FragColor = texture(u_colorTex, v_colorTexCoords); \n\
    } \n\
    else \n\
    { \n\
      vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w); \n\
      vec2 blendingWeight = a.yw; \n\
      if (max(a.x, a.z) > max(a.y, a.w)) \n\
      { \n\
        blendingOffset = vec4(a.x, 0.0, a.z, 0.0); \n\
        blendingWeight = a.xz; \n\
      } \n\
      blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0)); \n\
      vec4 bc = blendingOffset * vec4(u_framebufferMetrics.xy, -u_framebufferMetrics.xy); \n\
      bc += v_colorTexCoords.xyxy; \n\
      vec4 color = blendingWeight.x * SMAASampleLevelZero(u_colorTex, bc.xy); \n\
      color += blendingWeight.y * SMAASampleLevelZero(u_colorTex, bc.zw); \n\
      v_FragColor = color; \n\
    } \n\
  } \n\
";

static char const TEXT_OUTLINED_GUI_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoord; \n\
  attribute vec2 a_outlineColorTexCoord; \n\
  attribute vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform float u_isOutlinePass; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  #endif \n\
  varying vec2 v_maskTexCoord; \n\
  const float kBaseDepthShift = -10.0; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float isOutline = step(0.5, u_isOutlinePass); \n\
    float depthShift = kBaseDepthShift * isOutline; \n\
   \n\
    vec4 pos = (vec4(a_position.xyz, 1.0) + vec4(0.0, 0.0, depthShift, 0.0)) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos; \n\
    gl_Position = shiftedPos * projection; \n\
    vec2 colorTexCoord = mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const GLES3_TEXT_OUTLINED_GUI_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoord; \n\
  in vec2 a_outlineColorTexCoord; \n\
  in vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform float u_isOutlinePass; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoord; \n\
  #endif \n\
  out vec2 v_maskTexCoord; \n\
  const float kBaseDepthShift = -10.0; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float isOutline = step(0.5, u_isOutlinePass); \n\
    float depthShift = kBaseDepthShift * isOutline; \n\
   \n\
    vec4 pos = (vec4(a_position.xyz, 1.0) + vec4(0.0, 0.0, depthShift, 0.0)) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos; \n\
    gl_Position = shiftedPos * projection; \n\
    vec2 colorTexCoord = mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const TEXTURING_GUI_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_position; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    gl_Position = vec4(a_position, 0, 1) * modelView * projection; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_TEXTURING_GUI_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_position; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    gl_Position = vec4(a_position, 0, 1) * modelView * projection; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const TRAFFIC_LINE_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec2 a_colorTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec2 v_colorTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    v_colorTexCoord = a_colorTexCoord; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const GLES3_TRAFFIC_LINE_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec2 a_colorTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec2 v_colorTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    v_colorTexCoord = a_colorTexCoord; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const TEXT_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec2 v_maskTexCoord; \n\
  #ifdef ENABLE_VTF \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  uniform vec2 u_contrastGamma; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 glyphColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 glyphColor = texture2D(u_colorTex, v_colorTexCoord); \n\
  #endif \n\
  #ifdef GLES3 \n\
    float dist = texture2D(u_maskTex, v_maskTexCoord).r; \n\
  #else \n\
    float dist = texture2D(u_maskTex, v_maskTexCoord).a; \n\
  #endif \n\
    float alpha = smoothstep(u_contrastGamma.x - u_contrastGamma.y, u_contrastGamma.x + u_contrastGamma.y, dist) * u_opacity; \n\
    glyphColor.a *= alpha; \n\
    gl_FragColor = glyphColor; \n\
  } \n\
";

static char const GLES3_TEXT_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 v_maskTexCoord; \n\
  #ifdef ENABLE_VTF \n\
  in " LOW_P " vec4 v_color; \n\
  #else \n\
  in vec2 v_colorTexCoord; \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  uniform vec2 u_contrastGamma; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 glyphColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 glyphColor = texture(u_colorTex, v_colorTexCoord); \n\
  #endif \n\
  #ifdef GLES3 \n\
    float dist = texture(u_maskTex, v_maskTexCoord).r; \n\
  #else \n\
    float dist = texture(u_maskTex, v_maskTexCoord).a; \n\
  #endif \n\
    float alpha = smoothstep(u_contrastGamma.x - u_contrastGamma.y, u_contrastGamma.x + u_contrastGamma.y, dist) * u_opacity; \n\
    glyphColor.a *= alpha; \n\
    v_FragColor = glyphColor; \n\
  } \n\
";

static char const SMAA_EDGES_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_pos; \n\
  attribute vec2 a_tcoord; \n\
  uniform vec4 u_framebufferMetrics; \n\
  varying vec2 v_colorTexCoords; \n\
  varying vec4 v_offset0; \n\
  varying vec4 v_offset1; \n\
  varying vec4 v_offset2; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_colorTexCoords = a_tcoord; \n\
    v_offset0 = u_framebufferMetrics.xyxy * vec4(-1.0, 0.0, 0.0, -1.0) + a_tcoord.xyxy; \n\
    v_offset1 = u_framebufferMetrics.xyxy * vec4( 1.0, 0.0, 0.0,  1.0) + a_tcoord.xyxy; \n\
    v_offset2 = u_framebufferMetrics.xyxy * vec4(-2.0, 0.0, 0.0, -2.0) + a_tcoord.xyxy; \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

static char const GLES3_SMAA_EDGES_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_pos; \n\
  in vec2 a_tcoord; \n\
  uniform vec4 u_framebufferMetrics; \n\
  out vec2 v_colorTexCoords; \n\
  out vec4 v_offset0; \n\
  out vec4 v_offset1; \n\
  out vec4 v_offset2; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_colorTexCoords = a_tcoord; \n\
    v_offset0 = u_framebufferMetrics.xyxy * vec4(-1.0, 0.0, 0.0, -1.0) + a_tcoord.xyxy; \n\
    v_offset1 = u_framebufferMetrics.xyxy * vec4( 1.0, 0.0, 0.0,  1.0) + a_tcoord.xyxy; \n\
    v_offset2 = u_framebufferMetrics.xyxy * vec4(-2.0, 0.0, 0.0, -2.0) + a_tcoord.xyxy; \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

static char const ROUTE_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec3 a_length; \n\
  attribute vec4 a_color; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform vec4 u_routeParams; \n\
  varying vec3 v_length; \n\
  varying vec4 v_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float normalLen = length(a_normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    vec2 len = vec2(a_length.x, a_length.z); \n\
    if (u_routeParams.x != 0.0 && normalLen != 0.0) \n\
    { \n\
      vec2 norm = a_normal * u_routeParams.x; \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm, \n\
                                                      modelView, length(norm)); \n\
      if (u_routeParams.y != 0.0) \n\
        len = vec2(a_length.x + a_length.y * u_routeParams.y, a_length.z); \n\
    } \n\
    v_length = vec3(len, u_routeParams.z); \n\
    v_color = a_color; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const GLES3_ROUTE_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec2 a_normal; \n\
  in vec3 a_length; \n\
  in vec4 a_color; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform vec4 u_routeParams; \n\
  out vec3 v_length; \n\
  out vec4 v_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float normalLen = length(a_normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    vec2 len = vec2(a_length.x, a_length.z); \n\
    if (u_routeParams.x != 0.0 && normalLen != 0.0) \n\
    { \n\
      vec2 norm = a_normal * u_routeParams.x; \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm, \n\
                                                      modelView, length(norm)); \n\
      if (u_routeParams.y != 0.0) \n\
        len = vec2(a_length.x + a_length.y * u_routeParams.y, a_length.z); \n\
    } \n\
    v_length = vec3(len, u_routeParams.z); \n\
    v_color = a_color; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const ARROW3D_SHADOW_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_pos; \n\
  uniform mat4 u_transform; \n\
  varying float v_intensity; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 position = u_transform * vec4(a_pos.x, a_pos.y, 0.0, 1.0); \n\
    v_intensity = a_pos.w; \n\
    gl_Position = position; \n\
  } \n\
";

static char const GLES3_ARROW3D_SHADOW_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_pos; \n\
  uniform mat4 u_transform; \n\
  out float v_intensity; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 position = u_transform * vec4(a_pos.x, a_pos.y, 0.0, 1.0); \n\
    v_intensity = a_pos.w; \n\
    gl_Position = position; \n\
  } \n\
";

static char const DISCARDED_TEXTURING_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords); \n\
    finalColor.a *= u_opacity; \n\
    if (finalColor.a < 0.01) \n\
      discard; \n\
    gl_FragColor = finalColor; \n\
  } \n\
";

static char const GLES3_DISCARDED_TEXTURING_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  in vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 finalColor = texture(u_colorTex, v_colorTexCoords); \n\
    finalColor.a *= u_opacity; \n\
    if (finalColor.a < 0.01) \n\
      discard; \n\
    v_FragColor = finalColor; \n\
  } \n\
";

static char const DEBUG_RECT_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_position; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    gl_Position = vec4(a_position, 0, 1); \n\
  } \n\
";

static char const GLES3_DEBUG_RECT_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_position; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    gl_Position = vec4(a_position, 0, 1); \n\
  } \n\
";

static char const TEXTURING_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0, 0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_TEXTURING_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0, 0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const COLORED_SYMBOL_BILLBOARD_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec4 a_normal; \n\
  attribute vec4 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec4 v_normal; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal.xy + a_colorTexCoords.zw, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, 0.0, offset.xy); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoords.xy); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords.xy; \n\
  #endif \n\
    v_normal = a_normal; \n\
  } \n\
";

static char const GLES3_COLORED_SYMBOL_BILLBOARD_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec4 a_normal; \n\
  in vec4 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec4 v_normal; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal.xy + a_colorTexCoords.zw, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, 0.0, offset.xy); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoords.xy); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords.xy; \n\
  #endif \n\
    v_normal = a_normal; \n\
  } \n\
";

static char const COLORED_SYMBOL_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec4 a_normal; \n\
  attribute vec4 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec4 v_normal; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 p = vec4(a_position, 1) * modelView; \n\
    vec4 pos = vec4(a_normal.xy + a_colorTexCoords.zw, 0, 0) + p; \n\
    gl_Position = applyPivotTransform(pos * projection, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoords.xy); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords.xy; \n\
  #endif \n\
    v_normal = a_normal; \n\
  } \n\
";

static char const GLES3_COLORED_SYMBOL_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec4 a_normal; \n\
  in vec4 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec4 v_normal; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 p = vec4(a_position, 1) * modelView; \n\
    vec4 pos = vec4(a_normal.xy + a_colorTexCoords.zw, 0, 0) + p; \n\
    gl_Position = applyPivotTransform(pos * projection, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoords.xy); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords.xy; \n\
  #endif \n\
    v_normal = a_normal; \n\
  } \n\
";

static char const SMAA_EDGES_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  varying vec2 v_colorTexCoords; \n\
  varying vec4 v_offset0; \n\
  varying vec4 v_offset1; \n\
  varying vec4 v_offset2; \n\
  #define SMAA_THRESHOLD 0.05 \n\
  const vec2 kThreshold = vec2(SMAA_THRESHOLD, SMAA_THRESHOLD); \n\
  #define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0 \n\
  const vec3 kWeights = vec3(0.2126, 0.7152, 0.0722); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    float L = dot(texture2D(u_colorTex, v_colorTexCoords).rgb, kWeights); \n\
    float Lleft = dot(texture2D(u_colorTex, v_offset0.xy).rgb, kWeights); \n\
    float Ltop = dot(texture2D(u_colorTex, v_offset0.zw).rgb, kWeights); \n\
    vec4 delta; \n\
    delta.xy = abs(L - vec2(Lleft, Ltop)); \n\
    vec2 edges = step(kThreshold, delta.xy); \n\
    if (dot(edges, vec2(1.0, 1.0)) == 0.0) \n\
        discard; \n\
    float Lright = dot(texture2D(u_colorTex, v_offset1.xy).rgb, kWeights); \n\
    float Lbottom  = dot(texture2D(u_colorTex, v_offset1.zw).rgb, kWeights); \n\
    delta.zw = abs(L - vec2(Lright, Lbottom)); \n\
    vec2 maxDelta = max(delta.xy, delta.zw); \n\
    float Lleftleft = dot(texture2D(u_colorTex, v_offset2.xy).rgb, kWeights); \n\
    float Ltoptop = dot(texture2D(u_colorTex, v_offset2.zw).rgb, kWeights); \n\
    delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop)); \n\
    maxDelta = max(maxDelta.xy, delta.zw); \n\
    float finalDelta = max(maxDelta.x, maxDelta.y); \n\
    edges *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy); \n\
    gl_FragColor = vec4(edges, 0.0, 1.0); \n\
  } \n\
";

static char const GLES3_SMAA_EDGES_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  in vec2 v_colorTexCoords; \n\
  in vec4 v_offset0; \n\
  in vec4 v_offset1; \n\
  in vec4 v_offset2; \n\
  #define SMAA_THRESHOLD 0.05 \n\
  const vec2 kThreshold = vec2(SMAA_THRESHOLD, SMAA_THRESHOLD); \n\
  #define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0 \n\
  const vec3 kWeights = vec3(0.2126, 0.7152, 0.0722); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    float L = dot(texture(u_colorTex, v_colorTexCoords).rgb, kWeights); \n\
    float Lleft = dot(texture(u_colorTex, v_offset0.xy).rgb, kWeights); \n\
    float Ltop = dot(texture(u_colorTex, v_offset0.zw).rgb, kWeights); \n\
    vec4 delta; \n\
    delta.xy = abs(L - vec2(Lleft, Ltop)); \n\
    vec2 edges = step(kThreshold, delta.xy); \n\
    if (dot(edges, vec2(1.0, 1.0)) == 0.0) \n\
        discard; \n\
    float Lright = dot(texture(u_colorTex, v_offset1.xy).rgb, kWeights); \n\
    float Lbottom  = dot(texture(u_colorTex, v_offset1.zw).rgb, kWeights); \n\
    delta.zw = abs(L - vec2(Lright, Lbottom)); \n\
    vec2 maxDelta = max(delta.xy, delta.zw); \n\
    float Lleftleft = dot(texture(u_colorTex, v_offset2.xy).rgb, kWeights); \n\
    float Ltoptop = dot(texture(u_colorTex, v_offset2.zw).rgb, kWeights); \n\
    delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop)); \n\
    maxDelta = max(maxDelta.xy, delta.zw); \n\
    float finalDelta = max(maxDelta.x, maxDelta.y); \n\
    edges *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy); \n\
    v_FragColor = vec4(edges, 0.0, 1.0); \n\
  } \n\
";

static char const TEXTURING_BILLBOARD_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_TEXTURING_BILLBOARD_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const ARROW3D_SHADOW_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying float v_intensity; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 resColor = vec4(u_color.rgb, u_color.a * v_intensity); \n\
    gl_FragColor = samsungGoogleNexusWorkaround(resColor); \n\
  } \n\
";

static char const GLES3_ARROW3D_SHADOW_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in float v_intensity; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 resColor = vec4(u_color.rgb, u_color.a * v_intensity); \n\
    v_FragColor = samsungGoogleNexusWorkaround(resColor); \n\
  } \n\
";

static char const POSITION_ACCURACY3D_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform vec3 u_position; \n\
  uniform float u_accuracy; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 position = vec4(u_position.xy, 0.0, 1.0) * modelView; \n\
    vec4 normal = vec4(normalize(a_normal) * u_accuracy, 0.0, 0.0); \n\
    position = (position + normal) * projection; \n\
    gl_Position = applyPivotTransform(position, pivotTransform, u_position.z * zScale); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_POSITION_ACCURACY3D_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform vec3 u_position; \n\
  uniform float u_accuracy; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 position = vec4(u_position.xy, 0.0, 1.0) * modelView; \n\
    vec4 normal = vec4(normalize(a_normal) * u_accuracy, 0.0, 0.0); \n\
    position = (position + normal) * projection; \n\
    gl_Position = applyPivotTransform(position, pivotTransform, u_position.z * zScale); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const TRAFFIC_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec2 v_colorTexCoord; \n\
  varying vec2 v_maskTexCoord; \n\
  varying float v_halfLength; \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  uniform float u_outline; \n\
  uniform vec3 u_lightArrowColor; \n\
  uniform vec3 u_darkArrowColor; \n\
  uniform vec3 u_outlineColor; \n\
  const float kAntialiasingThreshold = 0.92; \n\
  const float kOutlineThreshold1 = 0.8; \n\
  const float kOutlineThreshold2 = 0.5; \n\
  const float kMaskOpacity = 0.7; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 color = texture2D(u_colorTex, v_colorTexCoord); \n\
    float alphaCode = color.a; \n\
    vec4 mask = texture2D(u_maskTex, v_maskTexCoord); \n\
    color.a = u_opacity * (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_halfLength))); \n\
    color.rgb = mix(color.rgb, mask.rgb * mix(u_lightArrowColor, u_darkArrowColor, step(alphaCode, 0.6)), mask.a * kMaskOpacity); \n\
    if (u_outline > 0.0) \n\
    { \n\
      color.rgb = mix(color.rgb, u_outlineColor, step(kOutlineThreshold1, abs(v_halfLength))); \n\
      color.rgb = mix(color.rgb, u_outlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_halfLength))); \n\
    } \n\
    gl_FragColor = color; \n\
  } \n\
";

static char const GLES3_TRAFFIC_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 v_colorTexCoord; \n\
  in vec2 v_maskTexCoord; \n\
  in float v_halfLength; \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  uniform float u_outline; \n\
  uniform vec3 u_lightArrowColor; \n\
  uniform vec3 u_darkArrowColor; \n\
  uniform vec3 u_outlineColor; \n\
  const float kAntialiasingThreshold = 0.92; \n\
  const float kOutlineThreshold1 = 0.8; \n\
  const float kOutlineThreshold2 = 0.5; \n\
  const float kMaskOpacity = 0.7; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 color = texture(u_colorTex, v_colorTexCoord); \n\
    float alphaCode = color.a; \n\
    vec4 mask = texture(u_maskTex, v_maskTexCoord); \n\
    color.a = u_opacity * (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_halfLength))); \n\
    color.rgb = mix(color.rgb, mask.rgb * mix(u_lightArrowColor, u_darkArrowColor, step(alphaCode, 0.6)), mask.a * kMaskOpacity); \n\
    if (u_outline > 0.0) \n\
    { \n\
      color.rgb = mix(color.rgb, u_outlineColor, step(kOutlineThreshold1, abs(v_halfLength))); \n\
      color.rgb = mix(color.rgb, u_outlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_halfLength))); \n\
    } \n\
    v_FragColor = color; \n\
  } \n\
";

static char const MASKED_TEXTURING_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  varying vec2 v_colorTexCoords; \n\
  varying vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords) * texture2D(u_maskTex, v_maskTexCoords); \n\
    finalColor.a *= u_opacity; \n\
    gl_FragColor = finalColor; \n\
  } \n\
";

static char const GLES3_MASKED_TEXTURING_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  in vec2 v_colorTexCoords; \n\
  in vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 finalColor = texture(u_colorTex, v_colorTexCoords) * texture(u_maskTex, v_maskTexCoords); \n\
    finalColor.a *= u_opacity; \n\
    v_FragColor = finalColor; \n\
  } \n\
";

static char const SCREEN_QUAD_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_pos; \n\
  attribute vec2 a_tcoord; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_colorTexCoords = a_tcoord; \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

static char const GLES3_SCREEN_QUAD_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_pos; \n\
  in vec2 a_tcoord; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_colorTexCoords = a_tcoord; \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

static char const DASHED_LINE_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec3 a_normal; \n\
  attribute vec2 a_colorTexCoord; \n\
  attribute vec4 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec2 v_colorTexCoord; \n\
  varying vec2 v_maskTexCoord; \n\
  varying vec2 v_halfLength; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal.xy; \n\
    float halfWidth = length(normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (halfWidth != 0.0) \n\
    { \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + normal, \n\
                                                      modelView, halfWidth); \n\
    } \n\
    float uOffset = min(length(vec4(kShapeCoordScalar, 0, 0, 0) * modelView) * a_maskTexCoord.x, 1.0); \n\
    v_colorTexCoord = a_colorTexCoord; \n\
    v_maskTexCoord = vec2(a_maskTexCoord.y + uOffset * a_maskTexCoord.z, a_maskTexCoord.w); \n\
    v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z)); \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const GLES3_DASHED_LINE_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec3 a_normal; \n\
  in vec2 a_colorTexCoord; \n\
  in vec4 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec2 v_colorTexCoord; \n\
  out vec2 v_maskTexCoord; \n\
  out vec2 v_halfLength; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal.xy; \n\
    float halfWidth = length(normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (halfWidth != 0.0) \n\
    { \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + normal, \n\
                                                      modelView, halfWidth); \n\
    } \n\
    float uOffset = min(length(vec4(kShapeCoordScalar, 0, 0, 0) * modelView) * a_maskTexCoord.x, 1.0); \n\
    v_colorTexCoord = a_colorTexCoord; \n\
    v_maskTexCoord = vec2(a_maskTexCoord.y + uOffset * a_maskTexCoord.z, a_maskTexCoord.w); \n\
    v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z)); \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const ROUTE_ARROW_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_arrowHalfWidth; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float normalLen = length(a_normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (normalLen != 0.0) \n\
    { \n\
      vec2 norm = a_normal * u_arrowHalfWidth; \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm, \n\
                                                      modelView, length(norm)); \n\
    } \n\
    v_colorTexCoords = a_colorTexCoords; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const GLES3_ROUTE_ARROW_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_arrowHalfWidth; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float normalLen = length(a_normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (normalLen != 0.0) \n\
    { \n\
      vec2 norm = a_normal * u_arrowHalfWidth; \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm, \n\
                                                      modelView, length(norm)); \n\
    } \n\
    v_colorTexCoords = a_colorTexCoords; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const ROUTE_DASH_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec3 v_length; \n\
  varying vec4 v_color; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  uniform vec2 u_pattern; \n\
  const float kAntialiasingThreshold = 0.92; \n\
  float alphaFromPattern(float curLen, float dashLen, float gapLen) \n\
  { \n\
    float len = dashLen + gapLen; \n\
    float offset = fract(curLen / len) * len; \n\
    return step(offset, dashLen); \n\
  } \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 color = u_color + v_color; \n\
    if (v_length.x < v_length.z) \n\
    { \n\
      color.a = 0.0; \n\
    } \n\
    else \n\
    { \n\
      color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y))) * \n\
                 alphaFromPattern(v_length.x, u_pattern.x, u_pattern.y); \n\
    } \n\
    gl_FragColor = samsungGoogleNexusWorkaround(color); \n\
  } \n\
";

static char const GLES3_ROUTE_DASH_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 v_length; \n\
  in vec4 v_color; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  uniform vec2 u_pattern; \n\
  const float kAntialiasingThreshold = 0.92; \n\
  float alphaFromPattern(float curLen, float dashLen, float gapLen) \n\
  { \n\
    float len = dashLen + gapLen; \n\
    float offset = fract(curLen / len) * len; \n\
    return step(offset, dashLen); \n\
  } \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 color = u_color + v_color; \n\
    if (v_length.x < v_length.z) \n\
    { \n\
      color.a = 0.0; \n\
    } \n\
    else \n\
    { \n\
      color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y))) * \n\
                 alphaFromPattern(v_length.x, u_pattern.x, u_pattern.y); \n\
    } \n\
    v_FragColor = samsungGoogleNexusWorkaround(color); \n\
  } \n\
";

static char const AREA3D_OUTLINE_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1.0) * modelView; \n\
    pos.xyw = (pos * projection).xyw; \n\
    pos.z = a_position.z * zScale; \n\
    gl_Position = pivotTransform * pos; \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
  } \n\
";

static char const GLES3_AREA3D_OUTLINE_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1.0) * modelView; \n\
    pos.xyw = (pos * projection).xyw; \n\
    pos.z = a_position.z * zScale; \n\
    gl_Position = pivotTransform * pos; \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
  } \n\
";

static char const HATCHING_AREA_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec2 a_colorTexCoords; \n\
  attribute vec2 a_maskTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  varying vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1) * modelView * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
    v_maskTexCoords = a_maskTexCoords; \n\
  } \n\
";

static char const GLES3_HATCHING_AREA_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec2 a_colorTexCoords; \n\
  in vec2 a_maskTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoords; \n\
  #endif \n\
  out vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1) * modelView * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
    v_maskTexCoords = a_maskTexCoords; \n\
  } \n\
";

static char const TRAFFIC_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec4 a_normal; \n\
  attribute vec4 a_colorTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform vec4 u_trafficParams; \n\
  varying vec2 v_colorTexCoord; \n\
  varying vec2 v_maskTexCoord; \n\
  varying float v_halfLength; \n\
  const float kArrowVSize = 0.25; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal.xy; \n\
    float halfWidth = length(normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (halfWidth != 0.0) \n\
    { \n\
      vec2 norm = normal * u_trafficParams.x; \n\
      if (a_normal.z < 0.0) \n\
        norm = normal * u_trafficParams.y; \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm, \n\
                                                      modelView, length(norm)); \n\
    } \n\
    float uOffset = length(vec4(kShapeCoordScalar, 0, 0, 0) * modelView) * a_normal.w; \n\
    v_colorTexCoord = a_colorTexCoord.xy; \n\
    float v = mix(a_colorTexCoord.z, a_colorTexCoord.z + kArrowVSize, 0.5 * a_normal.z + 0.5); \n\
    v_maskTexCoord = vec2(uOffset * u_trafficParams.z, v) * u_trafficParams.w; \n\
    v_maskTexCoord.x *= step(a_colorTexCoord.w, v_maskTexCoord.x); \n\
    v_halfLength = a_normal.z; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const GLES3_TRAFFIC_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec4 a_normal; \n\
  in vec4 a_colorTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform vec4 u_trafficParams; \n\
  out vec2 v_colorTexCoord; \n\
  out vec2 v_maskTexCoord; \n\
  out float v_halfLength; \n\
  const float kArrowVSize = 0.25; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal.xy; \n\
    float halfWidth = length(normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (halfWidth != 0.0) \n\
    { \n\
      vec2 norm = normal * u_trafficParams.x; \n\
      if (a_normal.z < 0.0) \n\
        norm = normal * u_trafficParams.y; \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm, \n\
                                                      modelView, length(norm)); \n\
    } \n\
    float uOffset = length(vec4(kShapeCoordScalar, 0, 0, 0) * modelView) * a_normal.w; \n\
    v_colorTexCoord = a_colorTexCoord.xy; \n\
    float v = mix(a_colorTexCoord.z, a_colorTexCoord.z + kArrowVSize, 0.5 * a_normal.z + 0.5); \n\
    v_maskTexCoord = vec2(uOffset * u_trafficParams.z, v) * u_trafficParams.w; \n\
    v_maskTexCoord.x *= step(a_colorTexCoord.w, v_maskTexCoord.x); \n\
    v_halfLength = a_normal.z; \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const MASKED_TEXTURING_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  attribute vec2 a_maskTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec2 v_colorTexCoords; \n\
  varying vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0, 0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
    v_maskTexCoords = a_maskTexCoords; \n\
  } \n\
";

static char const GLES3_MASKED_TEXTURING_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  in vec2 a_maskTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec2 v_colorTexCoords; \n\
  out vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    vec4 shiftedPos = vec4(a_normal, 0, 0) + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
    v_maskTexCoords = a_maskTexCoords; \n\
  } \n\
";

static char const PATH_SYMBOL_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    float normalLen = length(a_normal); \n\
    vec4 n = normalize(vec4(a_position.xy + a_normal * kShapeCoordScalar, 0, 0) * modelView); \n\
    vec4 norm = n * normalLen; \n\
    vec4 shiftedPos = norm + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_PATH_SYMBOL_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position.xyz, 1) * modelView; \n\
    float normalLen = length(a_normal); \n\
    vec4 n = normalize(vec4(a_position.xy + a_normal * kShapeCoordScalar, 0, 0) * modelView); \n\
    vec4 norm = n * normalLen; \n\
    vec4 shiftedPos = norm + pos; \n\
    gl_Position = applyPivotTransform(shiftedPos * projection, pivotTransform, 0.0); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const USER_MARK_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  attribute float a_animate; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_interpolationT; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal; \n\
    if (a_animate > 0.0) \n\
      normal = u_interpolationT * normal; \n\
    vec4 p = vec4(a_position, 1) * modelView; \n\
    vec4 pos = vec4(normal, 0, 0) + p; \n\
    vec4 projectedPivot = p * projection; \n\
    gl_Position = applyPivotTransform(pos * projection, pivotTransform, 0.0); \n\
    gl_Position.z = projectedPivot.y / projectedPivot.w * 0.5 + 0.5; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_USER_MARK_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  in float a_animate; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_interpolationT; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal; \n\
    if (a_animate > 0.0) \n\
      normal = u_interpolationT * normal; \n\
    vec4 p = vec4(a_position, 1) * modelView; \n\
    vec4 pos = vec4(normal, 0, 0) + p; \n\
    vec4 projectedPivot = p * projection; \n\
    gl_Position = applyPivotTransform(pos * projection, pivotTransform, 0.0); \n\
    gl_Position.z = projectedPivot.y / projectedPivot.w * 0.5 + 0.5; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const TEXT_BILLBOARD_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoord; \n\
  attribute vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_isOutlinePass; \n\
  uniform float zScale; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  #endif \n\
  varying vec2 v_maskTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = a_colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const GLES3_TEXT_BILLBOARD_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoord; \n\
  in vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_isOutlinePass; \n\
  uniform float zScale; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoord; \n\
  #endif \n\
  out vec2 v_maskTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = a_colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const USER_MARK_BILLBOARD_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  attribute float a_animate; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_interpolationT; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal; \n\
    if (a_animate > 0.0) \n\
      normal = u_interpolationT * normal; \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(normal, 0.0, 0.0) * projection; \n\
    vec4 projectedPivot = pivot * projection; \n\
    gl_Position = applyBillboardPivotTransform(projectedPivot, pivotTransform, 0.0, offset.xy); \n\
    gl_Position.z = projectedPivot.y / projectedPivot.w * 0.5 + 0.5; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_USER_MARK_BILLBOARD_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  in float a_animate; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_interpolationT; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal; \n\
    if (a_animate > 0.0) \n\
      normal = u_interpolationT * normal; \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(normal, 0.0, 0.0) * projection; \n\
    vec4 projectedPivot = pivot * projection; \n\
    gl_Position = applyBillboardPivotTransform(projectedPivot, pivotTransform, 0.0, offset.xy); \n\
    gl_Position.z = projectedPivot.y / projectedPivot.w * 0.5 + 0.5; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const MASKED_TEXTURING_BILLBOARD_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  attribute vec2 a_maskTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  varying vec2 v_colorTexCoords; \n\
  varying vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
    v_maskTexCoords = a_maskTexCoords; \n\
  } \n\
";

static char const GLES3_MASKED_TEXTURING_BILLBOARD_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  in vec2 a_maskTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float zScale; \n\
  out vec2 v_colorTexCoords; \n\
  out vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pivot = vec4(a_position.xyz, 1.0) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
    v_colorTexCoords = a_colorTexCoords; \n\
    v_maskTexCoords = a_maskTexCoords; \n\
  } \n\
";

static char const LINE_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec2 v_halfLength; \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  varying vec2 v_colorTexCoord; \n\
  #endif \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 color = v_color; \n\
  #else \n\
    " LOW_P " vec4 color = texture2D(u_colorTex, v_colorTexCoord); \n\
  #endif \n\
    color.a *= u_opacity; \n\
   \n\
    float currentW = abs(v_halfLength.x); \n\
    float diff = v_halfLength.y - currentW; \n\
    color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0)); \n\
   \n\
    gl_FragColor = color; \n\
  } \n\
";

static char const GLES3_LINE_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 v_halfLength; \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  in " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  in vec2 v_colorTexCoord; \n\
  #endif \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 color = v_color; \n\
  #else \n\
    " LOW_P " vec4 color = texture(u_colorTex, v_colorTexCoord); \n\
  #endif \n\
    color.a *= u_opacity; \n\
   \n\
    float currentW = abs(v_halfLength.x); \n\
    float diff = v_halfLength.y - currentW; \n\
    color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0)); \n\
   \n\
    v_FragColor = color; \n\
  } \n\
";

static char const ARROW3D_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec2 v_intensity; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    float alpha = smoothstep(0.8, 1.0, v_intensity.y); \n\
    vec4 resColor = vec4((v_intensity.x * 0.5 + 0.5) * u_color.rgb, u_color.a * alpha); \n\
    gl_FragColor = samsungGoogleNexusWorkaround(resColor); \n\
  } \n\
";

static char const GLES3_ARROW3D_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 v_intensity; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    float alpha = smoothstep(0.8, 1.0, v_intensity.y); \n\
    vec4 resColor = vec4((v_intensity.x * 0.5 + 0.5) * u_color.rgb, u_color.a * alpha); \n\
    v_FragColor = samsungGoogleNexusWorkaround(resColor); \n\
  } \n\
";

static char const CIRCLE_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec3 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  varying vec3 v_radius; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 p = vec4(a_position, 1) * modelView; \n\
    vec4 pos = vec4(a_normal.xy, 0, 0) + p; \n\
    gl_Position = applyPivotTransform(pos * projection, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
    v_radius = a_normal; \n\
  } \n\
";

static char const GLES3_CIRCLE_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec3 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  out vec3 v_radius; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 p = vec4(a_position, 1) * modelView; \n\
    vec4 pos = vec4(a_normal.xy, 0, 0) + p; \n\
    gl_Position = applyPivotTransform(pos * projection, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
    v_radius = a_normal; \n\
  } \n\
";

static char const CIRCLE_POINT_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform float u_opacity; \n\
  varying vec3 v_radius; \n\
  varying vec4 v_color; \n\
  const float kAntialiasingScalar = 0.9; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    float d = dot(v_radius.xy, v_radius.xy); \n\
    vec4 finalColor = v_color; \n\
   \n\
    float aaRadius = v_radius.z * kAntialiasingScalar; \n\
    float stepValue = smoothstep(aaRadius * aaRadius, v_radius.z * v_radius.z, d); \n\
    finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue); \n\
    gl_FragColor = samsungGoogleNexusWorkaround(finalColor); \n\
  } \n\
";

static char const GLES3_CIRCLE_POINT_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform float u_opacity; \n\
  in vec3 v_radius; \n\
  in vec4 v_color; \n\
  const float kAntialiasingScalar = 0.9; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    float d = dot(v_radius.xy, v_radius.xy); \n\
    vec4 finalColor = v_color; \n\
   \n\
    float aaRadius = v_radius.z * kAntialiasingScalar; \n\
    float stepValue = smoothstep(aaRadius * aaRadius, v_radius.z * v_radius.z, d); \n\
    finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue); \n\
    v_FragColor = samsungGoogleNexusWorkaround(finalColor); \n\
  } \n\
";

static char const SOLID_COLOR_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 finalColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    finalColor.a *= u_opacity; \n\
    gl_FragColor = finalColor; \n\
  } \n\
";

static char const GLES3_SOLID_COLOR_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  in " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  in vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 finalColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 finalColor = texture(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    finalColor.a *= u_opacity; \n\
    v_FragColor = finalColor; \n\
  } \n\
";

static char const COLORED_SYMBOL_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  varying vec4 v_normal; \n\
  #ifdef ENABLE_VTF \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 color = v_color; \n\
  #else \n\
    " LOW_P " vec4 color = texture2D(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    float r1 = (v_normal.z - aaPixelsCount) * (v_normal.z - aaPixelsCount); \n\
    float r2 = v_normal.x * v_normal.x + v_normal.y * v_normal.y; \n\
    float r3 = v_normal.z * v_normal.z; \n\
    float alpha = mix(step(r3, r2), smoothstep(r1, r3, r2), v_normal.w); \n\
    " LOW_P " vec4 finalColor = color; \n\
    finalColor.a = finalColor.a * u_opacity * (1.0 - alpha); \n\
    if (finalColor.a == 0.0) \n\
      discard; \n\
    gl_FragColor = finalColor; \n\
  } \n\
";

static char const GLES3_COLORED_SYMBOL_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  in vec4 v_normal; \n\
  #ifdef ENABLE_VTF \n\
  in " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  in vec2 v_colorTexCoords; \n\
  #endif \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 color = v_color; \n\
  #else \n\
    " LOW_P " vec4 color = texture(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    float r1 = (v_normal.z - aaPixelsCount) * (v_normal.z - aaPixelsCount); \n\
    float r2 = v_normal.x * v_normal.x + v_normal.y * v_normal.y; \n\
    float r3 = v_normal.z * v_normal.z; \n\
    float alpha = mix(step(r3, r2), smoothstep(r1, r3, r2), v_normal.w); \n\
    " LOW_P " vec4 finalColor = color; \n\
    finalColor.a = finalColor.a * u_opacity * (1.0 - alpha); \n\
    if (finalColor.a == 0.0) \n\
      discard; \n\
    v_FragColor = finalColor; \n\
  } \n\
";

static char const CIRCLE_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  varying vec3 v_radius; \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 finalColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    float smallRadius = v_radius.z - aaPixelsCount; \n\
    float stepValue = smoothstep(smallRadius * smallRadius, v_radius.z * v_radius.z, \n\
                                 v_radius.x * v_radius.x + v_radius.y * v_radius.y); \n\
    finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue); \n\
    gl_FragColor = finalColor; \n\
  } \n\
";

static char const GLES3_CIRCLE_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  in " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  in vec2 v_colorTexCoords; \n\
  #endif \n\
  in vec3 v_radius; \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 finalColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 finalColor = texture(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    float smallRadius = v_radius.z - aaPixelsCount; \n\
    float stepValue = smoothstep(smallRadius * smallRadius, v_radius.z * v_radius.z, \n\
                                 v_radius.x * v_radius.x + v_radius.y * v_radius.y); \n\
    finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue); \n\
    v_FragColor = finalColor; \n\
  } \n\
";

static char const SMAA_FINAL_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_pos; \n\
  attribute vec2 a_tcoord; \n\
  uniform vec4 u_framebufferMetrics; \n\
  varying vec2 v_colorTexCoords; \n\
  varying vec4 v_offset; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_colorTexCoords = a_tcoord; \n\
    v_offset = u_framebufferMetrics.xyxy * vec4(1.0, 0.0, 0.0, 1.0) + a_tcoord.xyxy; \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

static char const GLES3_SMAA_FINAL_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_pos; \n\
  in vec2 a_tcoord; \n\
  uniform vec4 u_framebufferMetrics; \n\
  out vec2 v_colorTexCoords; \n\
  out vec4 v_offset; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_colorTexCoords = a_tcoord; \n\
    v_offset = u_framebufferMetrics.xyxy * vec4(1.0, 0.0, 0.0, 1.0) + a_tcoord.xyxy; \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

static char const LINE_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec3 a_normal; \n\
  attribute vec2 a_colorTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  #endif \n\
  varying vec2 v_halfLength; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal.xy; \n\
    float halfWidth = length(normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (halfWidth != 0.0) \n\
    { \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + normal, \n\
                                                      modelView, halfWidth); \n\
    } \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = a_colorTexCoord; \n\
  #endif \n\
    v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z)); \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const GLES3_LINE_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec3 a_normal; \n\
  in vec2 a_colorTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoord; \n\
  #endif \n\
  out vec2 v_halfLength; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec2 normal = a_normal.xy; \n\
    float halfWidth = length(normal); \n\
    vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy; \n\
    if (halfWidth != 0.0) \n\
    { \n\
      transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + normal, \n\
                                                      modelView, halfWidth); \n\
    } \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = a_colorTexCoord; \n\
  #endif \n\
    v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z)); \n\
    vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  } \n\
";

static char const ARROW3D_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_pos; \n\
  attribute vec3 a_normal; \n\
  uniform mat4 u_transform; \n\
  varying vec2 v_intensity; \n\
  const vec3 lightDir = vec3(0.316, 0.0, 0.948); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 position = u_transform * vec4(a_pos.xyz, 1.0); \n\
    v_intensity = vec2(max(0.0, -dot(lightDir, a_normal)), a_pos.w); \n\
    gl_Position = position; \n\
  } \n\
";

static char const GLES3_ARROW3D_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_pos; \n\
  in vec3 a_normal; \n\
  uniform mat4 u_transform; \n\
  out vec2 v_intensity; \n\
  const vec3 lightDir = vec3(0.316, 0.0, 0.948); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 position = u_transform * vec4(a_pos.xyz, 1.0); \n\
    v_intensity = vec2(max(0.0, -dot(lightDir, a_normal)), a_pos.w); \n\
    gl_Position = position; \n\
  } \n\
";

static char const AREA_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec3 a_position; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1) * modelView * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
  } \n\
";

static char const GLES3_AREA_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 a_position; \n\
  in vec2 a_colorTexCoords; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoords; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    vec4 pos = vec4(a_position, 1) * modelView * projection; \n\
    gl_Position = applyPivotTransform(pos, pivotTransform, 0.0); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, a_colorTexCoords); \n\
  #else \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  #endif \n\
  } \n\
";

static char const TEXTURING_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords); \n\
    finalColor.a *= u_opacity; \n\
    gl_FragColor = finalColor; \n\
  } \n\
";

static char const GLES3_TEXTURING_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  in vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 finalColor = texture(u_colorTex, v_colorTexCoords); \n\
    finalColor.a *= u_opacity; \n\
    v_FragColor = finalColor; \n\
  } \n\
";

static char const RULER_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoords; \n\
  uniform vec2 u_position; \n\
  uniform float u_length; \n\
  uniform mat4 projection; \n\
  varying vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    gl_Position = vec4(u_position + a_position + u_length * a_normal, 0, 1) * projection; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const GLES3_RULER_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoords; \n\
  uniform vec2 u_position; \n\
  uniform float u_length; \n\
  uniform mat4 projection; \n\
  out vec2 v_colorTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    gl_Position = vec4(u_position + a_position + u_length * a_normal, 0, 1) * projection; \n\
    v_colorTexCoords = a_colorTexCoords; \n\
  } \n\
";

static char const SMAA_BLENDING_WEIGHT_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_smaaArea; \n\
  uniform sampler2D u_smaaSearch; \n\
  uniform vec4 u_framebufferMetrics; \n\
  varying vec4 v_coords; \n\
  varying vec4 v_offset0; \n\
  varying vec4 v_offset1; \n\
  varying vec4 v_offset2; \n\
  #define SMAA_SEARCHTEX_SIZE vec2(66.0, 33.0) \n\
  #define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 16.0) \n\
  #define SMAA_AREATEX_MAX_DISTANCE 16.0 \n\
  #define SMAA_AREATEX_PIXEL_SIZE (vec2(1.0 / 256.0, 1.0 / 1024.0)) \n\
  #ifdef GLES3 \n\
    #define SMAALoopBegin(condition) while (condition) { \n\
    #define SMAALoopEnd } \n\
    #define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0) \n\
    #define SMAASampleLevelZeroOffset(tex, coord, offset) textureLodOffset(tex, coord, 0.0, offset) \n\
    #define SMAARound(v) round((v)) \n\
    #define SMAAOffset(x,y) ivec2(x,y) \n\
  #else \n\
    #define SMAA_MAX_SEARCH_STEPS 8 \n\
    #define SMAALoopBegin(condition) for (int i = 0; i < SMAA_MAX_SEARCH_STEPS; i++) { if (!(condition)) break; \n\
    #define SMAALoopEnd } \n\
    #define SMAASampleLevelZero(tex, coord) texture2D(tex, coord) \n\
    #define SMAASampleLevelZeroOffset(tex, coord, offset) texture2D(tex, coord + vec2(offset) * u_framebufferMetrics.xy) \n\
    #define SMAARound(v) floor((v) + 0.5) \n\
    #define SMAAOffset(x,y) vec2(x,y) \n\
  #endif \n\
  const vec2 kAreaTexMaxDistance = vec2(SMAA_AREATEX_MAX_DISTANCE, SMAA_AREATEX_MAX_DISTANCE); \n\
  const float kActivationThreshold = 0.8281; \n\
  float SMAASearchLength(vec2 e, float offset) \n\
  { \n\
    vec2 scale = SMAA_SEARCHTEX_SIZE * vec2(0.5, -1.0); \n\
    vec2 bias = SMAA_SEARCHTEX_SIZE * vec2(offset, 1.0); \n\
    scale += vec2(-1.0,  1.0); \n\
    bias += vec2( 0.5, -0.5); \n\
    scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE; \n\
    bias *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE; \n\
  #ifdef GLES3 \n\
    return SMAASampleLevelZero(u_smaaSearch, scale * e + bias).r; \n\
  #else \n\
    return SMAASampleLevelZero(u_smaaSearch, scale * e + bias).a; \n\
  #endif \n\
  } \n\
  float SMAASearchXLeft(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(0.0, 1.0); \n\
    SMAALoopBegin(texcoord.x > end && e.g > kActivationThreshold && e.r == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(-2.0, 0.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e, 0.0); \n\
    return u_framebufferMetrics.x * offset + texcoord.x; \n\
  } \n\
  float SMAASearchXRight(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(0.0, 1.0); \n\
    SMAALoopBegin(texcoord.x < end && e.g > kActivationThreshold && e.r == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(2.0, 0.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e, 0.5); \n\
    return -u_framebufferMetrics.x * offset + texcoord.x; \n\
  } \n\
  float SMAASearchYUp(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(1.0, 0.0); \n\
    SMAALoopBegin(texcoord.y > end && e.r > kActivationThreshold && e.g == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(0.0, -2.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e.gr, 0.0); \n\
    return u_framebufferMetrics.y * offset + texcoord.y; \n\
  } \n\
  float SMAASearchYDown(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(1.0, 0.0); \n\
    SMAALoopBegin(texcoord.y < end && e.r > kActivationThreshold && e.g == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(0.0, 2.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e.gr, 0.5); \n\
    return -u_framebufferMetrics.y * offset + texcoord.y; \n\
  } \n\
  vec2 SMAAArea(vec2 dist, float e1, float e2) \n\
  { \n\
    vec2 texcoord = kAreaTexMaxDistance * SMAARound(4.0 * vec2(e1, e2)) + dist; \n\
    texcoord = SMAA_AREATEX_PIXEL_SIZE * (texcoord + 0.5); \n\
    return SMAASampleLevelZero(u_smaaArea, texcoord).rg; \n\
  } \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 weights = vec4(0.0, 0.0, 0.0, 0.0); \n\
    vec2 e = texture2D(u_colorTex, v_coords.xy).rg; \n\
    if (e.g > 0.0) // Edge at north \n\
    { \n\
      vec2 d; \n\
      vec3 coords; \n\
      coords.x = SMAASearchXLeft(v_offset0.xy, v_offset2.x); \n\
      coords.y = v_offset1.y; \n\
      d.x = coords.x; \n\
      float e1 = SMAASampleLevelZero(u_colorTex, coords.xy).r; \n\
      coords.z = SMAASearchXRight(v_offset0.zw, v_offset2.y); \n\
      d.y = coords.z; \n\
      d = abs(SMAARound(u_framebufferMetrics.zz * d - v_coords.zz)); \n\
      vec2 sqrt_d = sqrt(d); \n\
      float e2 = SMAASampleLevelZeroOffset(u_colorTex, coords.zy, SMAAOffset(1, 0)).r; \n\
      weights.rg = SMAAArea(sqrt_d, e1, e2); \n\
    } \n\
    if (e.r > 0.0) // Edge at west \n\
    { \n\
      vec2 d; \n\
      vec3 coords; \n\
      coords.y = SMAASearchYUp(v_offset1.xy, v_offset2.z); \n\
      coords.x = v_offset0.x; \n\
      d.x = coords.y; \n\
      float e1 = SMAASampleLevelZero(u_colorTex, coords.xy).g; \n\
      coords.z = SMAASearchYDown(v_offset1.zw, v_offset2.w); \n\
      d.y = coords.z; \n\
      d = abs(SMAARound(u_framebufferMetrics.ww * d - v_coords.ww)); \n\
      vec2 sqrt_d = sqrt(d); \n\
      float e2 = SMAASampleLevelZeroOffset(u_colorTex, coords.xz, SMAAOffset(0, 1)).g; \n\
      weights.ba = SMAAArea(sqrt_d, e1, e2); \n\
    } \n\
    gl_FragColor = weights; \n\
  } \n\
";

static char const GLES3_SMAA_BLENDING_WEIGHT_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_smaaArea; \n\
  uniform sampler2D u_smaaSearch; \n\
  uniform vec4 u_framebufferMetrics; \n\
  in vec4 v_coords; \n\
  in vec4 v_offset0; \n\
  in vec4 v_offset1; \n\
  in vec4 v_offset2; \n\
  #define SMAA_SEARCHTEX_SIZE vec2(66.0, 33.0) \n\
  #define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 16.0) \n\
  #define SMAA_AREATEX_MAX_DISTANCE 16.0 \n\
  #define SMAA_AREATEX_PIXEL_SIZE (vec2(1.0 / 256.0, 1.0 / 1024.0)) \n\
  #ifdef GLES3 \n\
    #define SMAALoopBegin(condition) while (condition) { \n\
    #define SMAALoopEnd } \n\
    #define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0) \n\
    #define SMAASampleLevelZeroOffset(tex, coord, offset) textureLodOffset(tex, coord, 0.0, offset) \n\
    #define SMAARound(v) round((v)) \n\
    #define SMAAOffset(x,y) ivec2(x,y) \n\
  #else \n\
    #define SMAA_MAX_SEARCH_STEPS 8 \n\
    #define SMAALoopBegin(condition) for (int i = 0; i < SMAA_MAX_SEARCH_STEPS; i++) { if (!(condition)) break; \n\
    #define SMAALoopEnd } \n\
    #define SMAASampleLevelZero(tex, coord) texture(tex, coord) \n\
    #define SMAASampleLevelZeroOffset(tex, coord, offset) texture(tex, coord + vec2(offset) * u_framebufferMetrics.xy) \n\
    #define SMAARound(v) floor((v) + 0.5) \n\
    #define SMAAOffset(x,y) vec2(x,y) \n\
  #endif \n\
  const vec2 kAreaTexMaxDistance = vec2(SMAA_AREATEX_MAX_DISTANCE, SMAA_AREATEX_MAX_DISTANCE); \n\
  const float kActivationThreshold = 0.8281; \n\
  float SMAASearchLength(vec2 e, float offset) \n\
  { \n\
    vec2 scale = SMAA_SEARCHTEX_SIZE * vec2(0.5, -1.0); \n\
    vec2 bias = SMAA_SEARCHTEX_SIZE * vec2(offset, 1.0); \n\
    scale += vec2(-1.0,  1.0); \n\
    bias += vec2( 0.5, -0.5); \n\
    scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE; \n\
    bias *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE; \n\
  #ifdef GLES3 \n\
    return SMAASampleLevelZero(u_smaaSearch, scale * e + bias).r; \n\
  #else \n\
    return SMAASampleLevelZero(u_smaaSearch, scale * e + bias).a; \n\
  #endif \n\
  } \n\
  float SMAASearchXLeft(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(0.0, 1.0); \n\
    SMAALoopBegin(texcoord.x > end && e.g > kActivationThreshold && e.r == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(-2.0, 0.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e, 0.0); \n\
    return u_framebufferMetrics.x * offset + texcoord.x; \n\
  } \n\
  float SMAASearchXRight(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(0.0, 1.0); \n\
    SMAALoopBegin(texcoord.x < end && e.g > kActivationThreshold && e.r == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(2.0, 0.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e, 0.5); \n\
    return -u_framebufferMetrics.x * offset + texcoord.x; \n\
  } \n\
  float SMAASearchYUp(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(1.0, 0.0); \n\
    SMAALoopBegin(texcoord.y > end && e.r > kActivationThreshold && e.g == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(0.0, -2.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e.gr, 0.0); \n\
    return u_framebufferMetrics.y * offset + texcoord.y; \n\
  } \n\
  float SMAASearchYDown(vec2 texcoord, float end) \n\
  { \n\
    vec2 e = vec2(1.0, 0.0); \n\
    SMAALoopBegin(texcoord.y < end && e.r > kActivationThreshold && e.g == 0.0) \n\
      e = SMAASampleLevelZero(u_colorTex, texcoord).rg; \n\
      texcoord = vec2(0.0, 2.0) * u_framebufferMetrics.xy + texcoord; \n\
    SMAALoopEnd \n\
    float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e.gr, 0.5); \n\
    return -u_framebufferMetrics.y * offset + texcoord.y; \n\
  } \n\
  vec2 SMAAArea(vec2 dist, float e1, float e2) \n\
  { \n\
    vec2 texcoord = kAreaTexMaxDistance * SMAARound(4.0 * vec2(e1, e2)) + dist; \n\
    texcoord = SMAA_AREATEX_PIXEL_SIZE * (texcoord + 0.5); \n\
    return SMAASampleLevelZero(u_smaaArea, texcoord).rg; \n\
  } \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 weights = vec4(0.0, 0.0, 0.0, 0.0); \n\
    vec2 e = texture(u_colorTex, v_coords.xy).rg; \n\
    if (e.g > 0.0) // Edge at north \n\
    { \n\
      vec2 d; \n\
      vec3 coords; \n\
      coords.x = SMAASearchXLeft(v_offset0.xy, v_offset2.x); \n\
      coords.y = v_offset1.y; \n\
      d.x = coords.x; \n\
      float e1 = SMAASampleLevelZero(u_colorTex, coords.xy).r; \n\
      coords.z = SMAASearchXRight(v_offset0.zw, v_offset2.y); \n\
      d.y = coords.z; \n\
      d = abs(SMAARound(u_framebufferMetrics.zz * d - v_coords.zz)); \n\
      vec2 sqrt_d = sqrt(d); \n\
      float e2 = SMAASampleLevelZeroOffset(u_colorTex, coords.zy, SMAAOffset(1, 0)).r; \n\
      weights.rg = SMAAArea(sqrt_d, e1, e2); \n\
    } \n\
    if (e.r > 0.0) // Edge at west \n\
    { \n\
      vec2 d; \n\
      vec3 coords; \n\
      coords.y = SMAASearchYUp(v_offset1.xy, v_offset2.z); \n\
      coords.x = v_offset0.x; \n\
      d.x = coords.y; \n\
      float e1 = SMAASampleLevelZero(u_colorTex, coords.xy).g; \n\
      coords.z = SMAASearchYDown(v_offset1.zw, v_offset2.w); \n\
      d.y = coords.z; \n\
      d = abs(SMAARound(u_framebufferMetrics.ww * d - v_coords.ww)); \n\
      vec2 sqrt_d = sqrt(d); \n\
      float e2 = SMAASampleLevelZeroOffset(u_colorTex, coords.xz, SMAAOffset(0, 1)).g; \n\
      weights.ba = SMAAArea(sqrt_d, e1, e2); \n\
    } \n\
    v_FragColor = weights; \n\
  } \n\
";

static char const HATCHING_AREA_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  varying vec2 v_colorTexCoords; \n\
  #endif \n\
  uniform sampler2D u_maskTex; \n\
  varying vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 color = v_color; \n\
  #else \n\
    " LOW_P " vec4 color = texture2D(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    color *= texture2D(u_maskTex, v_maskTexCoords); \n\
    color.a *= u_opacity; \n\
    gl_FragColor = color; \n\
  } \n\
";

static char const GLES3_HATCHING_AREA_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform float u_opacity; \n\
  #ifdef ENABLE_VTF \n\
  in " LOW_P " vec4 v_color; \n\
  #else \n\
  uniform sampler2D u_colorTex; \n\
  in vec2 v_colorTexCoords; \n\
  #endif \n\
  uniform sampler2D u_maskTex; \n\
  in vec2 v_maskTexCoords; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 color = v_color; \n\
  #else \n\
    " LOW_P " vec4 color = texture(u_colorTex, v_colorTexCoords); \n\
  #endif \n\
    color *= texture(u_maskTex, v_maskTexCoords); \n\
    color.a *= u_opacity; \n\
    v_FragColor = color; \n\
  } \n\
";

static char const ROUTE_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec3 v_length; \n\
  varying vec4 v_color; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  uniform vec4 u_outlineColor; \n\
  uniform vec4 u_routeParams; \n\
  const float kAntialiasingThreshold = 0.92; \n\
  const float kOutlineThreshold1 = 0.81; \n\
  const float kOutlineThreshold2 = 0.71; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0); \n\
    if (v_length.x >= v_length.z) \n\
    { \n\
      color = mix(mix(u_color, vec4(v_color.rgb, 1.0), v_color.a), u_color, step(u_routeParams.w, 0.0)); \n\
      color = mix(color, u_outlineColor, step(kOutlineThreshold1, abs(v_length.y))); \n\
      color = mix(color, u_outlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_length.y))); \n\
      color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y))); \n\
    } \n\
    gl_FragColor = samsungGoogleNexusWorkaround(color); \n\
  } \n\
";

static char const GLES3_ROUTE_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec3 v_length; \n\
  in vec4 v_color; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  uniform vec4 u_outlineColor; \n\
  uniform vec4 u_routeParams; \n\
  const float kAntialiasingThreshold = 0.92; \n\
  const float kOutlineThreshold1 = 0.81; \n\
  const float kOutlineThreshold2 = 0.71; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0); \n\
    if (v_length.x >= v_length.z) \n\
    { \n\
      color = mix(mix(u_color, vec4(v_color.rgb, 1.0), v_color.a), u_color, step(u_routeParams.w, 0.0)); \n\
      color = mix(color, u_outlineColor, step(kOutlineThreshold1, abs(v_length.y))); \n\
      color = mix(color, u_outlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_length.y))); \n\
      color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y))); \n\
    } \n\
    v_FragColor = samsungGoogleNexusWorkaround(color); \n\
  } \n\
";

static char const TEXT_FIXED_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec2 v_maskTexCoord; \n\
  #ifdef ENABLE_VTF \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  uniform vec2 u_contrastGamma; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 glyphColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 glyphColor = texture2D(u_colorTex, v_colorTexCoord); \n\
  #endif \n\
  #ifdef GLES3 \n\
    float alpha = texture2D(u_maskTex, v_maskTexCoord).r; \n\
  #else \n\
    float alpha = texture2D(u_maskTex, v_maskTexCoord).a; \n\
  #endif \n\
    glyphColor.a *= u_opacity * alpha; \n\
    gl_FragColor = glyphColor; \n\
  } \n\
";

static char const GLES3_TEXT_FIXED_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 v_maskTexCoord; \n\
  #ifdef ENABLE_VTF \n\
  in " LOW_P " vec4 v_color; \n\
  #else \n\
  in vec2 v_colorTexCoord; \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  uniform vec2 u_contrastGamma; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
  #ifdef ENABLE_VTF \n\
    " LOW_P " vec4 glyphColor = v_color; \n\
  #else \n\
    " LOW_P " vec4 glyphColor = texture(u_colorTex, v_colorTexCoord); \n\
  #endif \n\
  #ifdef GLES3 \n\
    float alpha = texture(u_maskTex, v_maskTexCoord).r; \n\
  #else \n\
    float alpha = texture(u_maskTex, v_maskTexCoord).a; \n\
  #endif \n\
    glyphColor.a *= u_opacity * alpha; \n\
    v_FragColor = glyphColor; \n\
  } \n\
";

static char const DEBUG_RECT_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    gl_FragColor = samsungGoogleNexusWorkaround(u_color); \n\
  } \n\
";

static char const GLES3_DEBUG_RECT_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform vec4 u_color; \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
  uniform sampler2D u_colorTex; \n\
  #endif \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    v_FragColor = samsungGoogleNexusWorkaround(u_color); \n\
  } \n\
";

static char const TEXT_OUTLINED_BILLBOARD_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec4 a_position; \n\
  attribute vec2 a_normal; \n\
  attribute vec2 a_colorTexCoord; \n\
  attribute vec2 a_outlineColorTexCoord; \n\
  attribute vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_isOutlinePass; \n\
  uniform float zScale; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  varying " LOW_P " vec4 v_color; \n\
  #else \n\
  varying vec2 v_colorTexCoord; \n\
  #endif \n\
  varying vec2 v_maskTexCoord; \n\
  const float kBaseDepthShift = -10.0; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float isOutline = step(0.5, u_isOutlinePass); \n\
    float depthShift = kBaseDepthShift * isOutline; \n\
   \n\
    vec4 pivot = (vec4(a_position.xyz, 1.0) + vec4(0.0, 0.0, depthShift, 0.0)) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
    vec2 colorTexCoord = mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture2D(u_colorTex, colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const GLES3_TEXT_OUTLINED_BILLBOARD_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec4 a_position; \n\
  in vec2 a_normal; \n\
  in vec2 a_colorTexCoord; \n\
  in vec2 a_outlineColorTexCoord; \n\
  in vec2 a_maskTexCoord; \n\
  uniform mat4 modelView; \n\
  uniform mat4 projection; \n\
  uniform mat4 pivotTransform; \n\
  uniform float u_isOutlinePass; \n\
  uniform float zScale; \n\
  #ifdef ENABLE_VTF \n\
  uniform sampler2D u_colorTex; \n\
  out " LOW_P " vec4 v_color; \n\
  #else \n\
  out vec2 v_colorTexCoord; \n\
  #endif \n\
  out vec2 v_maskTexCoord; \n\
  const float kBaseDepthShift = -10.0; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    float isOutline = step(0.5, u_isOutlinePass); \n\
    float depthShift = kBaseDepthShift * isOutline; \n\
   \n\
    vec4 pivot = (vec4(a_position.xyz, 1.0) + vec4(0.0, 0.0, depthShift, 0.0)) * modelView; \n\
    vec4 offset = vec4(a_normal, 0.0, 0.0) * projection; \n\
    gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy); \n\
    vec2 colorTexCoord = mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline); \n\
  #ifdef ENABLE_VTF \n\
    v_color = texture(u_colorTex, colorTexCoord); \n\
  #else \n\
    v_colorTexCoord = colorTexCoord; \n\
  #endif \n\
    v_maskTexCoord = a_maskTexCoord; \n\
  } \n\
";

static char const TRAFFIC_LINE_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  varying vec2 v_colorTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 color = texture2D(u_colorTex, v_colorTexCoord); \n\
    gl_FragColor = vec4(color.rgb, u_opacity); \n\
  } \n\
";

static char const GLES3_TRAFFIC_LINE_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  in vec2 v_colorTexCoord; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 color = texture(u_colorTex, v_colorTexCoord); \n\
    v_FragColor = vec4(color.rgb, u_opacity); \n\
  } \n\
";

static char const DASHED_LINE_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  varying vec2 v_colorTexCoord; \n\
  varying vec2 v_halfLength; \n\
  varying vec2 v_maskTexCoord; \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 color = texture2D(u_colorTex, v_colorTexCoord); \n\
  #ifdef GLES3 \n\
    float mask = texture2D(u_maskTex, v_maskTexCoord).r; \n\
  #else \n\
    float mask = texture2D(u_maskTex, v_maskTexCoord).a; \n\
  #endif \n\
    color.a = color.a * mask * u_opacity; \n\
   \n\
    float currentW = abs(v_halfLength.x); \n\
    float diff = v_halfLength.y - currentW; \n\
    color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0)); \n\
    gl_FragColor = color; \n\
  } \n\
";

static char const GLES3_DASHED_LINE_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 v_colorTexCoord; \n\
  in vec2 v_halfLength; \n\
  in vec2 v_maskTexCoord; \n\
  uniform sampler2D u_colorTex; \n\
  uniform sampler2D u_maskTex; \n\
  uniform float u_opacity; \n\
  const float aaPixelsCount = 2.5; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 color = texture(u_colorTex, v_colorTexCoord); \n\
  #ifdef GLES3 \n\
    float mask = texture(u_maskTex, v_maskTexCoord).r; \n\
  #else \n\
    float mask = texture(u_maskTex, v_maskTexCoord).a; \n\
  #endif \n\
    color.a = color.a * mask * u_opacity; \n\
   \n\
    float currentW = abs(v_halfLength.x); \n\
    float diff = v_halfLength.y - currentW; \n\
    color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0)); \n\
    v_FragColor = color; \n\
  } \n\
";

static char const TEXTURING3D_FSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  varying vec2 v_colorTexCoords; \n\
  varying float v_intensity; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  void main() \n\
  { \n\
    vec4 finalColor = vec4(texture2D(u_colorTex, v_colorTexCoords).rgb, u_opacity); \n\
    gl_FragColor = vec4((v_intensity * 0.2 + 0.8) * finalColor.rgb, finalColor.a); \n\
  } \n\
";

static char const GLES3_TEXTURING3D_FSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  uniform sampler2D u_colorTex; \n\
  uniform float u_opacity; \n\
  in vec2 v_colorTexCoords; \n\
  in float v_intensity; \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 samsungGoogleNexusWorkaround(vec4 color) \n\
  { \n\
  #ifdef SAMSUNG_GOOGLE_NEXUS \n\
    const float kFakeColorScalar = 0.0; \n\
    return color + texture(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar; \n\
  #else \n\
    return color; \n\
  #endif \n\
  } \n\
  out vec4 v_FragColor; \n\
  void main() \n\
  { \n\
    vec4 finalColor = vec4(texture(u_colorTex, v_colorTexCoords).rgb, u_opacity); \n\
    v_FragColor = vec4((v_intensity * 0.2 + 0.8) * finalColor.rgb, finalColor.a); \n\
  } \n\
";

static char const SMAA_BLENDING_WEIGHT_VSH[] = " \
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  attribute vec2 a_pos; \n\
  attribute vec2 a_tcoord; \n\
  uniform vec4 u_framebufferMetrics; \n\
  varying vec4 v_coords; \n\
  varying vec4 v_offset0; \n\
  varying vec4 v_offset1; \n\
  varying vec4 v_offset2; \n\
  #define SMAA_MAX_SEARCH_STEPS 8.0 \n\
  const vec4 kMaxSearchSteps = vec4(-2.0 * SMAA_MAX_SEARCH_STEPS, 2.0 * SMAA_MAX_SEARCH_STEPS, \n\
                                    -2.0 * SMAA_MAX_SEARCH_STEPS, 2.0  * SMAA_MAX_SEARCH_STEPS); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_coords = vec4(a_tcoord, a_tcoord * u_framebufferMetrics.zw); \n\
    v_offset0 = u_framebufferMetrics.xyxy * vec4(-0.25, -0.125, 1.25, -0.125) + a_tcoord.xyxy; \n\
    v_offset1 = u_framebufferMetrics.xyxy * vec4(-0.125, -0.25, -0.125, 1.25) + a_tcoord.xyxy; \n\
    v_offset2 = u_framebufferMetrics.xxyy * kMaxSearchSteps + vec4(v_offset0.xz, v_offset1.yw); \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

static char const GLES3_SMAA_BLENDING_WEIGHT_VSH[] = " \
  " SHADER_VERSION " \n\
  #ifdef GL_ES \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
      #define MAXPREC " HIGH_P " \n\
    #else \n\
      #define MAXPREC " MEDIUM_P " \n\
    #endif \n\
    precision MAXPREC float; \n\
  #endif \n\
  in vec2 a_pos; \n\
  in vec2 a_tcoord; \n\
  uniform vec4 u_framebufferMetrics; \n\
  out vec4 v_coords; \n\
  out vec4 v_offset0; \n\
  out vec4 v_offset1; \n\
  out vec4 v_offset2; \n\
  #define SMAA_MAX_SEARCH_STEPS 8.0 \n\
  const vec4 kMaxSearchSteps = vec4(-2.0 * SMAA_MAX_SEARCH_STEPS, 2.0 * SMAA_MAX_SEARCH_STEPS, \n\
                                    -2.0 * SMAA_MAX_SEARCH_STEPS, 2.0  * SMAA_MAX_SEARCH_STEPS); \n\
  const float kShapeCoordScalar = 1000.0; \n\
  vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ) \n\
  { \n\
    vec4 transformedPivot = pivot; \n\
    float w = transformedPivot.w; \n\
    transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw; \n\
    transformedPivot.z *= transformedPivot.w / w; \n\
    return transformedPivot; \n\
  } \n\
  vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset) \n\
  { \n\
    float logicZ = pivot.z / pivot.w; \n\
    vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w); \n\
    vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0); \n\
    return vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0); \n\
  } \n\
  vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth) \n\
  { \n\
    vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy; \n\
    return originalAxisPos + normalize(p - originalAxisPos) * halfWidth; \n\
  } \n\
  void main() \n\
  { \n\
    v_coords = vec4(a_tcoord, a_tcoord * u_framebufferMetrics.zw); \n\
    v_offset0 = u_framebufferMetrics.xyxy * vec4(-0.25, -0.125, 1.25, -0.125) + a_tcoord.xyxy; \n\
    v_offset1 = u_framebufferMetrics.xyxy * vec4(-0.125, -0.25, -0.125, 1.25) + a_tcoord.xyxy; \n\
    v_offset2 = u_framebufferMetrics.xxyy * kMaxSearchSteps + vec4(v_offset0.xz, v_offset1.yw); \n\
    gl_Position = vec4(a_pos, 0.0, 1.0); \n\
  } \n\
";

#define TEXT_OUTLINED_VSH_INDEX 13
#define CIRCLE_POINT_VSH_INDEX 8
#define TEXT_VSH_INDEX 38
#define AREA3D_VSH_INDEX 18
#define MY_POSITION_VSH_INDEX 39
#define ARROW3D_OUTLINE_FSH_INDEX 2
#define SMAA_FINAL_FSH_INDEX 11
#define TEXT_OUTLINED_GUI_VSH_INDEX 14
#define TEXTURING_GUI_VSH_INDEX 9
#define TRAFFIC_LINE_VSH_INDEX 52
#define TEXT_FSH_INDEX 57
#define SMAA_EDGES_VSH_INDEX 1
#define ROUTE_VSH_INDEX 60
#define ARROW3D_SHADOW_VSH_INDEX 17
#define DISCARDED_TEXTURING_FSH_INDEX 35
#define DEBUG_RECT_VSH_INDEX 32
#define TEXTURING_VSH_INDEX 45
#define COLORED_SYMBOL_BILLBOARD_VSH_INDEX 29
#define COLORED_SYMBOL_VSH_INDEX 26
#define SMAA_EDGES_FSH_INDEX 25
#define TEXTURING_BILLBOARD_VSH_INDEX 36
#define ARROW3D_SHADOW_FSH_INDEX 10
#define POSITION_ACCURACY3D_VSH_INDEX 59
#define TRAFFIC_FSH_INDEX 19
#define MASKED_TEXTURING_FSH_INDEX 40
#define SCREEN_QUAD_VSH_INDEX 6
#define DASHED_LINE_VSH_INDEX 47
#define ROUTE_ARROW_VSH_INDEX 33
#define ROUTE_DASH_FSH_INDEX 42
#define AREA3D_OUTLINE_VSH_INDEX 4
#define HATCHING_AREA_VSH_INDEX 16
#define TRAFFIC_VSH_INDEX 12
#define MASKED_TEXTURING_VSH_INDEX 58
#define PATH_SYMBOL_VSH_INDEX 15
#define USER_MARK_VSH_INDEX 27
#define TEXT_BILLBOARD_VSH_INDEX 44
#define USER_MARK_BILLBOARD_VSH_INDEX 22
#define MASKED_TEXTURING_BILLBOARD_VSH_INDEX 5
#define LINE_FSH_INDEX 31
#define ARROW3D_FSH_INDEX 48
#define CIRCLE_VSH_INDEX 24
#define CIRCLE_POINT_FSH_INDEX 28
#define SOLID_COLOR_FSH_INDEX 20
#define COLORED_SYMBOL_FSH_INDEX 7
#define CIRCLE_FSH_INDEX 0
#define SMAA_FINAL_VSH_INDEX 21
#define LINE_VSH_INDEX 54
#define ARROW3D_VSH_INDEX 43
#define AREA_VSH_INDEX 41
#define TEXTURING_FSH_INDEX 49
#define RULER_VSH_INDEX 30
#define SMAA_BLENDING_WEIGHT_FSH_INDEX 56
#define HATCHING_AREA_FSH_INDEX 23
#define ROUTE_FSH_INDEX 37
#define TEXT_FIXED_FSH_INDEX 53
#define DEBUG_RECT_FSH_INDEX 55
#define TEXT_OUTLINED_BILLBOARD_VSH_INDEX 3
#define TRAFFIC_LINE_FSH_INDEX 46
#define DASHED_LINE_FSH_INDEX 50
#define TEXTURING3D_FSH_INDEX 51
#define SMAA_BLENDING_WEIGHT_VSH_INDEX 34

int const TEXTURING_PROGRAM = 0;
int const MASKED_TEXTURING_PROGRAM = 1;
int const COLORED_SYMBOL_PROGRAM = 2;
int const TEXT_OUTLINED_PROGRAM = 3;
int const TEXT_PROGRAM = 4;
int const TEXT_FIXED_PROGRAM = 5;
int const TEXT_OUTLINED_GUI_PROGRAM = 6;
int const AREA_PROGRAM = 7;
int const AREA_OUTLINE_PROGRAM = 8;
int const AREA_3D_PROGRAM = 9;
int const AREA_3D_OUTLINE_PROGRAM = 10;
int const LINE_PROGRAM = 11;
int const CAP_JOIN_PROGRAM = 12;
int const DASHED_LINE_PROGRAM = 13;
int const PATH_SYMBOL_LINE = 14;
int const HATCHING_AREA_PROGRAM = 15;
int const TEXTURING_GUI_PROGRAM = 16;
int const RULER_PROGRAM = 17;
int const ACCURACY_PROGRAM = 18;
int const MY_POSITION_PROGRAM = 19;
int const BOOKMARK_PROGRAM = 20;
int const ROUTE_PROGRAM = 21;
int const ROUTE_DASH_PROGRAM = 22;
int const ROUTE_ARROW_PROGRAM = 23;
int const CIRCLE_POINT_PROGRAM = 24;
int const DEBUG_RECT_PROGRAM = 25;
int const SCREEN_QUAD_PROGRAM = 26;
int const ARROW_3D_PROGRAM = 27;
int const ARROW_3D_SHADOW_PROGRAM = 28;
int const ARROW_3D_OUTLINE_PROGRAM = 29;
int const COLORED_SYMBOL_BILLBOARD_PROGRAM = 30;
int const TEXTURING_BILLBOARD_PROGRAM = 31;
int const MASKED_TEXTURING_BILLBOARD_PROGRAM = 32;
int const TEXT_OUTLINED_BILLBOARD_PROGRAM = 33;
int const TEXT_BILLBOARD_PROGRAM = 34;
int const TEXT_FIXED_BILLBOARD_PROGRAM = 35;
int const BOOKMARK_BILLBOARD_PROGRAM = 36;
int const TRAFFIC_PROGRAM = 37;
int const TRAFFIC_LINE_PROGRAM = 38;
int const SMAA_EDGES_PROGRAM = 39;
int const SMAA_BLENDING_WEIGHT_PROGRAM = 40;
int const SMAA_FINAL_PROGRAM = 41;

#if !defined(COMPILER_TESTS)
namespace
{
void InitGpuProgramsLib(dp::ApiVersion apiVersion, std::map<int, GpuProgramInfo> & gpuIndex)
{
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
    gpuIndex.insert(std::make_pair(0, GpuProgramInfo(TEXTURING_VSH_INDEX, TEXTURING_FSH_INDEX, TEXTURING_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(1, GpuProgramInfo(MASKED_TEXTURING_VSH_INDEX, MASKED_TEXTURING_FSH_INDEX, MASKED_TEXTURING_VSH, MASKED_TEXTURING_FSH, 2)));
    gpuIndex.insert(std::make_pair(2, GpuProgramInfo(COLORED_SYMBOL_VSH_INDEX, COLORED_SYMBOL_FSH_INDEX, COLORED_SYMBOL_VSH, COLORED_SYMBOL_FSH, 1)));
    gpuIndex.insert(std::make_pair(3, GpuProgramInfo(TEXT_OUTLINED_VSH_INDEX, TEXT_FSH_INDEX, TEXT_OUTLINED_VSH, TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(4, GpuProgramInfo(TEXT_VSH_INDEX, TEXT_FSH_INDEX, TEXT_VSH, TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(5, GpuProgramInfo(TEXT_VSH_INDEX, TEXT_FIXED_FSH_INDEX, TEXT_VSH, TEXT_FIXED_FSH, 2)));
    gpuIndex.insert(std::make_pair(6, GpuProgramInfo(TEXT_OUTLINED_GUI_VSH_INDEX, TEXT_FSH_INDEX, TEXT_OUTLINED_GUI_VSH, TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(7, GpuProgramInfo(AREA_VSH_INDEX, SOLID_COLOR_FSH_INDEX, AREA_VSH, SOLID_COLOR_FSH, 1)));
    gpuIndex.insert(std::make_pair(8, GpuProgramInfo(AREA_VSH_INDEX, SOLID_COLOR_FSH_INDEX, AREA_VSH, SOLID_COLOR_FSH, 1)));
    gpuIndex.insert(std::make_pair(9, GpuProgramInfo(AREA3D_VSH_INDEX, TEXTURING3D_FSH_INDEX, AREA3D_VSH, TEXTURING3D_FSH, 1)));
    gpuIndex.insert(std::make_pair(10, GpuProgramInfo(AREA3D_OUTLINE_VSH_INDEX, SOLID_COLOR_FSH_INDEX, AREA3D_OUTLINE_VSH, SOLID_COLOR_FSH, 1)));
    gpuIndex.insert(std::make_pair(11, GpuProgramInfo(LINE_VSH_INDEX, LINE_FSH_INDEX, LINE_VSH, LINE_FSH, 1)));
    gpuIndex.insert(std::make_pair(12, GpuProgramInfo(CIRCLE_VSH_INDEX, CIRCLE_FSH_INDEX, CIRCLE_VSH, CIRCLE_FSH, 1)));
    gpuIndex.insert(std::make_pair(13, GpuProgramInfo(DASHED_LINE_VSH_INDEX, DASHED_LINE_FSH_INDEX, DASHED_LINE_VSH, DASHED_LINE_FSH, 2)));
    gpuIndex.insert(std::make_pair(14, GpuProgramInfo(PATH_SYMBOL_VSH_INDEX, TEXTURING_FSH_INDEX, PATH_SYMBOL_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(15, GpuProgramInfo(HATCHING_AREA_VSH_INDEX, HATCHING_AREA_FSH_INDEX, HATCHING_AREA_VSH, HATCHING_AREA_FSH, 2)));
    gpuIndex.insert(std::make_pair(16, GpuProgramInfo(TEXTURING_GUI_VSH_INDEX, TEXTURING_FSH_INDEX, TEXTURING_GUI_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(17, GpuProgramInfo(RULER_VSH_INDEX, TEXTURING_FSH_INDEX, RULER_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(18, GpuProgramInfo(POSITION_ACCURACY3D_VSH_INDEX, TEXTURING_FSH_INDEX, POSITION_ACCURACY3D_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(19, GpuProgramInfo(MY_POSITION_VSH_INDEX, TEXTURING_FSH_INDEX, MY_POSITION_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(20, GpuProgramInfo(USER_MARK_VSH_INDEX, DISCARDED_TEXTURING_FSH_INDEX, USER_MARK_VSH, DISCARDED_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(21, GpuProgramInfo(ROUTE_VSH_INDEX, ROUTE_FSH_INDEX, ROUTE_VSH, ROUTE_FSH, 0)));
    gpuIndex.insert(std::make_pair(22, GpuProgramInfo(ROUTE_VSH_INDEX, ROUTE_DASH_FSH_INDEX, ROUTE_VSH, ROUTE_DASH_FSH, 0)));
    gpuIndex.insert(std::make_pair(23, GpuProgramInfo(ROUTE_ARROW_VSH_INDEX, DISCARDED_TEXTURING_FSH_INDEX, ROUTE_ARROW_VSH, DISCARDED_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(24, GpuProgramInfo(CIRCLE_POINT_VSH_INDEX, CIRCLE_POINT_FSH_INDEX, CIRCLE_POINT_VSH, CIRCLE_POINT_FSH, 0)));
    gpuIndex.insert(std::make_pair(25, GpuProgramInfo(DEBUG_RECT_VSH_INDEX, DEBUG_RECT_FSH_INDEX, DEBUG_RECT_VSH, DEBUG_RECT_FSH, 0)));
    gpuIndex.insert(std::make_pair(26, GpuProgramInfo(SCREEN_QUAD_VSH_INDEX, TEXTURING_FSH_INDEX, SCREEN_QUAD_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(27, GpuProgramInfo(ARROW3D_VSH_INDEX, ARROW3D_FSH_INDEX, ARROW3D_VSH, ARROW3D_FSH, 0)));
    gpuIndex.insert(std::make_pair(28, GpuProgramInfo(ARROW3D_SHADOW_VSH_INDEX, ARROW3D_SHADOW_FSH_INDEX, ARROW3D_SHADOW_VSH, ARROW3D_SHADOW_FSH, 0)));
    gpuIndex.insert(std::make_pair(29, GpuProgramInfo(ARROW3D_SHADOW_VSH_INDEX, ARROW3D_OUTLINE_FSH_INDEX, ARROW3D_SHADOW_VSH, ARROW3D_OUTLINE_FSH, 0)));
    gpuIndex.insert(std::make_pair(30, GpuProgramInfo(COLORED_SYMBOL_BILLBOARD_VSH_INDEX, COLORED_SYMBOL_FSH_INDEX, COLORED_SYMBOL_BILLBOARD_VSH, COLORED_SYMBOL_FSH, 1)));
    gpuIndex.insert(std::make_pair(31, GpuProgramInfo(TEXTURING_BILLBOARD_VSH_INDEX, TEXTURING_FSH_INDEX, TEXTURING_BILLBOARD_VSH, TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(32, GpuProgramInfo(MASKED_TEXTURING_BILLBOARD_VSH_INDEX, MASKED_TEXTURING_FSH_INDEX, MASKED_TEXTURING_BILLBOARD_VSH, MASKED_TEXTURING_FSH, 2)));
    gpuIndex.insert(std::make_pair(33, GpuProgramInfo(TEXT_OUTLINED_BILLBOARD_VSH_INDEX, TEXT_FSH_INDEX, TEXT_OUTLINED_BILLBOARD_VSH, TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(34, GpuProgramInfo(TEXT_BILLBOARD_VSH_INDEX, TEXT_FSH_INDEX, TEXT_BILLBOARD_VSH, TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(35, GpuProgramInfo(TEXT_BILLBOARD_VSH_INDEX, TEXT_FIXED_FSH_INDEX, TEXT_BILLBOARD_VSH, TEXT_FIXED_FSH, 2)));
    gpuIndex.insert(std::make_pair(36, GpuProgramInfo(USER_MARK_BILLBOARD_VSH_INDEX, DISCARDED_TEXTURING_FSH_INDEX, USER_MARK_BILLBOARD_VSH, DISCARDED_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(37, GpuProgramInfo(TRAFFIC_VSH_INDEX, TRAFFIC_FSH_INDEX, TRAFFIC_VSH, TRAFFIC_FSH, 2)));
    gpuIndex.insert(std::make_pair(38, GpuProgramInfo(TRAFFIC_LINE_VSH_INDEX, TRAFFIC_LINE_FSH_INDEX, TRAFFIC_LINE_VSH, TRAFFIC_LINE_FSH, 1)));
    gpuIndex.insert(std::make_pair(39, GpuProgramInfo(SMAA_EDGES_VSH_INDEX, SMAA_EDGES_FSH_INDEX, SMAA_EDGES_VSH, SMAA_EDGES_FSH, 1)));
    gpuIndex.insert(std::make_pair(40, GpuProgramInfo(SMAA_BLENDING_WEIGHT_VSH_INDEX, SMAA_BLENDING_WEIGHT_FSH_INDEX, SMAA_BLENDING_WEIGHT_VSH, SMAA_BLENDING_WEIGHT_FSH, 3)));
    gpuIndex.insert(std::make_pair(41, GpuProgramInfo(SMAA_FINAL_VSH_INDEX, SMAA_FINAL_FSH_INDEX, SMAA_FINAL_VSH, SMAA_FINAL_FSH, 2)));
  }
  else if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    gpuIndex.insert(std::make_pair(0, GpuProgramInfo(TEXTURING_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_TEXTURING_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(1, GpuProgramInfo(MASKED_TEXTURING_VSH_INDEX, MASKED_TEXTURING_FSH_INDEX, GLES3_MASKED_TEXTURING_VSH, GLES3_MASKED_TEXTURING_FSH, 2)));
    gpuIndex.insert(std::make_pair(2, GpuProgramInfo(COLORED_SYMBOL_VSH_INDEX, COLORED_SYMBOL_FSH_INDEX, GLES3_COLORED_SYMBOL_VSH, GLES3_COLORED_SYMBOL_FSH, 1)));
    gpuIndex.insert(std::make_pair(3, GpuProgramInfo(TEXT_OUTLINED_VSH_INDEX, TEXT_FSH_INDEX, GLES3_TEXT_OUTLINED_VSH, GLES3_TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(4, GpuProgramInfo(TEXT_VSH_INDEX, TEXT_FSH_INDEX, GLES3_TEXT_VSH, GLES3_TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(5, GpuProgramInfo(TEXT_VSH_INDEX, TEXT_FIXED_FSH_INDEX, GLES3_TEXT_VSH, GLES3_TEXT_FIXED_FSH, 2)));
    gpuIndex.insert(std::make_pair(6, GpuProgramInfo(TEXT_OUTLINED_GUI_VSH_INDEX, TEXT_FSH_INDEX, GLES3_TEXT_OUTLINED_GUI_VSH, GLES3_TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(7, GpuProgramInfo(AREA_VSH_INDEX, SOLID_COLOR_FSH_INDEX, GLES3_AREA_VSH, GLES3_SOLID_COLOR_FSH, 1)));
    gpuIndex.insert(std::make_pair(8, GpuProgramInfo(AREA_VSH_INDEX, SOLID_COLOR_FSH_INDEX, GLES3_AREA_VSH, GLES3_SOLID_COLOR_FSH, 1)));
    gpuIndex.insert(std::make_pair(9, GpuProgramInfo(AREA3D_VSH_INDEX, TEXTURING3D_FSH_INDEX, GLES3_AREA3D_VSH, GLES3_TEXTURING3D_FSH, 1)));
    gpuIndex.insert(std::make_pair(10, GpuProgramInfo(AREA3D_OUTLINE_VSH_INDEX, SOLID_COLOR_FSH_INDEX, GLES3_AREA3D_OUTLINE_VSH, GLES3_SOLID_COLOR_FSH, 1)));
    gpuIndex.insert(std::make_pair(11, GpuProgramInfo(LINE_VSH_INDEX, LINE_FSH_INDEX, GLES3_LINE_VSH, GLES3_LINE_FSH, 1)));
    gpuIndex.insert(std::make_pair(12, GpuProgramInfo(CIRCLE_VSH_INDEX, CIRCLE_FSH_INDEX, GLES3_CIRCLE_VSH, GLES3_CIRCLE_FSH, 1)));
    gpuIndex.insert(std::make_pair(13, GpuProgramInfo(DASHED_LINE_VSH_INDEX, DASHED_LINE_FSH_INDEX, GLES3_DASHED_LINE_VSH, GLES3_DASHED_LINE_FSH, 2)));
    gpuIndex.insert(std::make_pair(14, GpuProgramInfo(PATH_SYMBOL_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_PATH_SYMBOL_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(15, GpuProgramInfo(HATCHING_AREA_VSH_INDEX, HATCHING_AREA_FSH_INDEX, GLES3_HATCHING_AREA_VSH, GLES3_HATCHING_AREA_FSH, 2)));
    gpuIndex.insert(std::make_pair(16, GpuProgramInfo(TEXTURING_GUI_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_TEXTURING_GUI_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(17, GpuProgramInfo(RULER_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_RULER_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(18, GpuProgramInfo(POSITION_ACCURACY3D_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_POSITION_ACCURACY3D_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(19, GpuProgramInfo(MY_POSITION_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_MY_POSITION_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(20, GpuProgramInfo(USER_MARK_VSH_INDEX, DISCARDED_TEXTURING_FSH_INDEX, GLES3_USER_MARK_VSH, GLES3_DISCARDED_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(21, GpuProgramInfo(ROUTE_VSH_INDEX, ROUTE_FSH_INDEX, GLES3_ROUTE_VSH, GLES3_ROUTE_FSH, 0)));
    gpuIndex.insert(std::make_pair(22, GpuProgramInfo(ROUTE_VSH_INDEX, ROUTE_DASH_FSH_INDEX, GLES3_ROUTE_VSH, GLES3_ROUTE_DASH_FSH, 0)));
    gpuIndex.insert(std::make_pair(23, GpuProgramInfo(ROUTE_ARROW_VSH_INDEX, DISCARDED_TEXTURING_FSH_INDEX, GLES3_ROUTE_ARROW_VSH, GLES3_DISCARDED_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(24, GpuProgramInfo(CIRCLE_POINT_VSH_INDEX, CIRCLE_POINT_FSH_INDEX, GLES3_CIRCLE_POINT_VSH, GLES3_CIRCLE_POINT_FSH, 0)));
    gpuIndex.insert(std::make_pair(25, GpuProgramInfo(DEBUG_RECT_VSH_INDEX, DEBUG_RECT_FSH_INDEX, GLES3_DEBUG_RECT_VSH, GLES3_DEBUG_RECT_FSH, 0)));
    gpuIndex.insert(std::make_pair(26, GpuProgramInfo(SCREEN_QUAD_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_SCREEN_QUAD_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(27, GpuProgramInfo(ARROW3D_VSH_INDEX, ARROW3D_FSH_INDEX, GLES3_ARROW3D_VSH, GLES3_ARROW3D_FSH, 0)));
    gpuIndex.insert(std::make_pair(28, GpuProgramInfo(ARROW3D_SHADOW_VSH_INDEX, ARROW3D_SHADOW_FSH_INDEX, GLES3_ARROW3D_SHADOW_VSH, GLES3_ARROW3D_SHADOW_FSH, 0)));
    gpuIndex.insert(std::make_pair(29, GpuProgramInfo(ARROW3D_SHADOW_VSH_INDEX, ARROW3D_OUTLINE_FSH_INDEX, GLES3_ARROW3D_SHADOW_VSH, GLES3_ARROW3D_OUTLINE_FSH, 0)));
    gpuIndex.insert(std::make_pair(30, GpuProgramInfo(COLORED_SYMBOL_BILLBOARD_VSH_INDEX, COLORED_SYMBOL_FSH_INDEX, GLES3_COLORED_SYMBOL_BILLBOARD_VSH, GLES3_COLORED_SYMBOL_FSH, 1)));
    gpuIndex.insert(std::make_pair(31, GpuProgramInfo(TEXTURING_BILLBOARD_VSH_INDEX, TEXTURING_FSH_INDEX, GLES3_TEXTURING_BILLBOARD_VSH, GLES3_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(32, GpuProgramInfo(MASKED_TEXTURING_BILLBOARD_VSH_INDEX, MASKED_TEXTURING_FSH_INDEX, GLES3_MASKED_TEXTURING_BILLBOARD_VSH, GLES3_MASKED_TEXTURING_FSH, 2)));
    gpuIndex.insert(std::make_pair(33, GpuProgramInfo(TEXT_OUTLINED_BILLBOARD_VSH_INDEX, TEXT_FSH_INDEX, GLES3_TEXT_OUTLINED_BILLBOARD_VSH, GLES3_TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(34, GpuProgramInfo(TEXT_BILLBOARD_VSH_INDEX, TEXT_FSH_INDEX, GLES3_TEXT_BILLBOARD_VSH, GLES3_TEXT_FSH, 2)));
    gpuIndex.insert(std::make_pair(35, GpuProgramInfo(TEXT_BILLBOARD_VSH_INDEX, TEXT_FIXED_FSH_INDEX, GLES3_TEXT_BILLBOARD_VSH, GLES3_TEXT_FIXED_FSH, 2)));
    gpuIndex.insert(std::make_pair(36, GpuProgramInfo(USER_MARK_BILLBOARD_VSH_INDEX, DISCARDED_TEXTURING_FSH_INDEX, GLES3_USER_MARK_BILLBOARD_VSH, GLES3_DISCARDED_TEXTURING_FSH, 1)));
    gpuIndex.insert(std::make_pair(37, GpuProgramInfo(TRAFFIC_VSH_INDEX, TRAFFIC_FSH_INDEX, GLES3_TRAFFIC_VSH, GLES3_TRAFFIC_FSH, 2)));
    gpuIndex.insert(std::make_pair(38, GpuProgramInfo(TRAFFIC_LINE_VSH_INDEX, TRAFFIC_LINE_FSH_INDEX, GLES3_TRAFFIC_LINE_VSH, GLES3_TRAFFIC_LINE_FSH, 1)));
    gpuIndex.insert(std::make_pair(39, GpuProgramInfo(SMAA_EDGES_VSH_INDEX, SMAA_EDGES_FSH_INDEX, GLES3_SMAA_EDGES_VSH, GLES3_SMAA_EDGES_FSH, 1)));
    gpuIndex.insert(std::make_pair(40, GpuProgramInfo(SMAA_BLENDING_WEIGHT_VSH_INDEX, SMAA_BLENDING_WEIGHT_FSH_INDEX, GLES3_SMAA_BLENDING_WEIGHT_VSH, GLES3_SMAA_BLENDING_WEIGHT_FSH, 3)));
    gpuIndex.insert(std::make_pair(41, GpuProgramInfo(SMAA_FINAL_VSH_INDEX, SMAA_FINAL_FSH_INDEX, GLES3_SMAA_FINAL_VSH, GLES3_SMAA_FINAL_FSH, 2)));
  }
}
}  // namespace

ShaderMapper::ShaderMapper(dp::ApiVersion apiVersion)
{ gpu::InitGpuProgramsLib(apiVersion, m_mapping); }
GpuProgramInfo const & ShaderMapper::GetProgramInfo(int program) const
{
  auto it = m_mapping.find(program);
  ASSERT(it != m_mapping.end(), ());
  return it->second;
}
#endif

#if defined(COMPILER_TESTS)
ShadersEnum GetVertexShaders(dp::ApiVersion apiVersion)
{
  ShadersEnum vertexEnum;
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
    vertexEnum.push_back(std::make_pair("SMAA_EDGES_VSH", std::string(SMAA_EDGES_VSH)));
    vertexEnum.push_back(std::make_pair("TEXT_OUTLINED_BILLBOARD_VSH", std::string(TEXT_OUTLINED_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("AREA3D_OUTLINE_VSH", std::string(AREA3D_OUTLINE_VSH)));
    vertexEnum.push_back(std::make_pair("MASKED_TEXTURING_BILLBOARD_VSH", std::string(MASKED_TEXTURING_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("SCREEN_QUAD_VSH", std::string(SCREEN_QUAD_VSH)));
    vertexEnum.push_back(std::make_pair("CIRCLE_POINT_VSH", std::string(CIRCLE_POINT_VSH)));
    vertexEnum.push_back(std::make_pair("TEXTURING_GUI_VSH", std::string(TEXTURING_GUI_VSH)));
    vertexEnum.push_back(std::make_pair("TRAFFIC_VSH", std::string(TRAFFIC_VSH)));
    vertexEnum.push_back(std::make_pair("TEXT_OUTLINED_VSH", std::string(TEXT_OUTLINED_VSH)));
    vertexEnum.push_back(std::make_pair("TEXT_OUTLINED_GUI_VSH", std::string(TEXT_OUTLINED_GUI_VSH)));
    vertexEnum.push_back(std::make_pair("PATH_SYMBOL_VSH", std::string(PATH_SYMBOL_VSH)));
    vertexEnum.push_back(std::make_pair("HATCHING_AREA_VSH", std::string(HATCHING_AREA_VSH)));
    vertexEnum.push_back(std::make_pair("ARROW3D_SHADOW_VSH", std::string(ARROW3D_SHADOW_VSH)));
    vertexEnum.push_back(std::make_pair("AREA3D_VSH", std::string(AREA3D_VSH)));
    vertexEnum.push_back(std::make_pair("SMAA_FINAL_VSH", std::string(SMAA_FINAL_VSH)));
    vertexEnum.push_back(std::make_pair("USER_MARK_BILLBOARD_VSH", std::string(USER_MARK_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("CIRCLE_VSH", std::string(CIRCLE_VSH)));
    vertexEnum.push_back(std::make_pair("COLORED_SYMBOL_VSH", std::string(COLORED_SYMBOL_VSH)));
    vertexEnum.push_back(std::make_pair("USER_MARK_VSH", std::string(USER_MARK_VSH)));
    vertexEnum.push_back(std::make_pair("COLORED_SYMBOL_BILLBOARD_VSH", std::string(COLORED_SYMBOL_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("RULER_VSH", std::string(RULER_VSH)));
    vertexEnum.push_back(std::make_pair("DEBUG_RECT_VSH", std::string(DEBUG_RECT_VSH)));
    vertexEnum.push_back(std::make_pair("ROUTE_ARROW_VSH", std::string(ROUTE_ARROW_VSH)));
    vertexEnum.push_back(std::make_pair("SMAA_BLENDING_WEIGHT_VSH", std::string(SMAA_BLENDING_WEIGHT_VSH)));
    vertexEnum.push_back(std::make_pair("TEXTURING_BILLBOARD_VSH", std::string(TEXTURING_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("TEXT_VSH", std::string(TEXT_VSH)));
    vertexEnum.push_back(std::make_pair("MY_POSITION_VSH", std::string(MY_POSITION_VSH)));
    vertexEnum.push_back(std::make_pair("AREA_VSH", std::string(AREA_VSH)));
    vertexEnum.push_back(std::make_pair("ARROW3D_VSH", std::string(ARROW3D_VSH)));
    vertexEnum.push_back(std::make_pair("TEXT_BILLBOARD_VSH", std::string(TEXT_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("TEXTURING_VSH", std::string(TEXTURING_VSH)));
    vertexEnum.push_back(std::make_pair("DASHED_LINE_VSH", std::string(DASHED_LINE_VSH)));
    vertexEnum.push_back(std::make_pair("TRAFFIC_LINE_VSH", std::string(TRAFFIC_LINE_VSH)));
    vertexEnum.push_back(std::make_pair("LINE_VSH", std::string(LINE_VSH)));
    vertexEnum.push_back(std::make_pair("MASKED_TEXTURING_VSH", std::string(MASKED_TEXTURING_VSH)));
    vertexEnum.push_back(std::make_pair("POSITION_ACCURACY3D_VSH", std::string(POSITION_ACCURACY3D_VSH)));
    vertexEnum.push_back(std::make_pair("ROUTE_VSH", std::string(ROUTE_VSH)));
  }
  else if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    vertexEnum.push_back(std::make_pair("GLES3_SMAA_EDGES_VSH", std::string(GLES3_SMAA_EDGES_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXT_OUTLINED_BILLBOARD_VSH", std::string(GLES3_TEXT_OUTLINED_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_AREA3D_OUTLINE_VSH", std::string(GLES3_AREA3D_OUTLINE_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_MASKED_TEXTURING_BILLBOARD_VSH", std::string(GLES3_MASKED_TEXTURING_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_SCREEN_QUAD_VSH", std::string(GLES3_SCREEN_QUAD_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_CIRCLE_POINT_VSH", std::string(GLES3_CIRCLE_POINT_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXTURING_GUI_VSH", std::string(GLES3_TEXTURING_GUI_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TRAFFIC_VSH", std::string(GLES3_TRAFFIC_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXT_OUTLINED_VSH", std::string(GLES3_TEXT_OUTLINED_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXT_OUTLINED_GUI_VSH", std::string(GLES3_TEXT_OUTLINED_GUI_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_PATH_SYMBOL_VSH", std::string(GLES3_PATH_SYMBOL_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_HATCHING_AREA_VSH", std::string(GLES3_HATCHING_AREA_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_ARROW3D_SHADOW_VSH", std::string(GLES3_ARROW3D_SHADOW_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_AREA3D_VSH", std::string(GLES3_AREA3D_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_SMAA_FINAL_VSH", std::string(GLES3_SMAA_FINAL_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_USER_MARK_BILLBOARD_VSH", std::string(GLES3_USER_MARK_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_CIRCLE_VSH", std::string(GLES3_CIRCLE_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_COLORED_SYMBOL_VSH", std::string(GLES3_COLORED_SYMBOL_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_USER_MARK_VSH", std::string(GLES3_USER_MARK_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_COLORED_SYMBOL_BILLBOARD_VSH", std::string(GLES3_COLORED_SYMBOL_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_RULER_VSH", std::string(GLES3_RULER_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_DEBUG_RECT_VSH", std::string(GLES3_DEBUG_RECT_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_ROUTE_ARROW_VSH", std::string(GLES3_ROUTE_ARROW_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_SMAA_BLENDING_WEIGHT_VSH", std::string(GLES3_SMAA_BLENDING_WEIGHT_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXTURING_BILLBOARD_VSH", std::string(GLES3_TEXTURING_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXT_VSH", std::string(GLES3_TEXT_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_MY_POSITION_VSH", std::string(GLES3_MY_POSITION_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_AREA_VSH", std::string(GLES3_AREA_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_ARROW3D_VSH", std::string(GLES3_ARROW3D_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXT_BILLBOARD_VSH", std::string(GLES3_TEXT_BILLBOARD_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TEXTURING_VSH", std::string(GLES3_TEXTURING_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_DASHED_LINE_VSH", std::string(GLES3_DASHED_LINE_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_TRAFFIC_LINE_VSH", std::string(GLES3_TRAFFIC_LINE_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_LINE_VSH", std::string(GLES3_LINE_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_MASKED_TEXTURING_VSH", std::string(GLES3_MASKED_TEXTURING_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_POSITION_ACCURACY3D_VSH", std::string(GLES3_POSITION_ACCURACY3D_VSH)));
    vertexEnum.push_back(std::make_pair("GLES3_ROUTE_VSH", std::string(GLES3_ROUTE_VSH)));
  }
  return vertexEnum;
}
ShadersEnum GetFragmentShaders(dp::ApiVersion apiVersion)
{
  ShadersEnum fragmentEnum;
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
    fragmentEnum.push_back(std::make_pair("CIRCLE_FSH", std::string(CIRCLE_FSH)));
    fragmentEnum.push_back(std::make_pair("ARROW3D_OUTLINE_FSH", std::string(ARROW3D_OUTLINE_FSH)));
    fragmentEnum.push_back(std::make_pair("COLORED_SYMBOL_FSH", std::string(COLORED_SYMBOL_FSH)));
    fragmentEnum.push_back(std::make_pair("ARROW3D_SHADOW_FSH", std::string(ARROW3D_SHADOW_FSH)));
    fragmentEnum.push_back(std::make_pair("SMAA_FINAL_FSH", std::string(SMAA_FINAL_FSH)));
    fragmentEnum.push_back(std::make_pair("TRAFFIC_FSH", std::string(TRAFFIC_FSH)));
    fragmentEnum.push_back(std::make_pair("SOLID_COLOR_FSH", std::string(SOLID_COLOR_FSH)));
    fragmentEnum.push_back(std::make_pair("HATCHING_AREA_FSH", std::string(HATCHING_AREA_FSH)));
    fragmentEnum.push_back(std::make_pair("SMAA_EDGES_FSH", std::string(SMAA_EDGES_FSH)));
    fragmentEnum.push_back(std::make_pair("CIRCLE_POINT_FSH", std::string(CIRCLE_POINT_FSH)));
    fragmentEnum.push_back(std::make_pair("LINE_FSH", std::string(LINE_FSH)));
    fragmentEnum.push_back(std::make_pair("DISCARDED_TEXTURING_FSH", std::string(DISCARDED_TEXTURING_FSH)));
    fragmentEnum.push_back(std::make_pair("ROUTE_FSH", std::string(ROUTE_FSH)));
    fragmentEnum.push_back(std::make_pair("MASKED_TEXTURING_FSH", std::string(MASKED_TEXTURING_FSH)));
    fragmentEnum.push_back(std::make_pair("ROUTE_DASH_FSH", std::string(ROUTE_DASH_FSH)));
    fragmentEnum.push_back(std::make_pair("TRAFFIC_LINE_FSH", std::string(TRAFFIC_LINE_FSH)));
    fragmentEnum.push_back(std::make_pair("ARROW3D_FSH", std::string(ARROW3D_FSH)));
    fragmentEnum.push_back(std::make_pair("TEXTURING_FSH", std::string(TEXTURING_FSH)));
    fragmentEnum.push_back(std::make_pair("DASHED_LINE_FSH", std::string(DASHED_LINE_FSH)));
    fragmentEnum.push_back(std::make_pair("TEXTURING3D_FSH", std::string(TEXTURING3D_FSH)));
    fragmentEnum.push_back(std::make_pair("TEXT_FIXED_FSH", std::string(TEXT_FIXED_FSH)));
    fragmentEnum.push_back(std::make_pair("DEBUG_RECT_FSH", std::string(DEBUG_RECT_FSH)));
    fragmentEnum.push_back(std::make_pair("SMAA_BLENDING_WEIGHT_FSH", std::string(SMAA_BLENDING_WEIGHT_FSH)));
    fragmentEnum.push_back(std::make_pair("TEXT_FSH", std::string(TEXT_FSH)));
  }
  else if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    fragmentEnum.push_back(std::make_pair("GLES3_CIRCLE_FSH", std::string(GLES3_CIRCLE_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_ARROW3D_OUTLINE_FSH", std::string(GLES3_ARROW3D_OUTLINE_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_COLORED_SYMBOL_FSH", std::string(GLES3_COLORED_SYMBOL_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_ARROW3D_SHADOW_FSH", std::string(GLES3_ARROW3D_SHADOW_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_SMAA_FINAL_FSH", std::string(GLES3_SMAA_FINAL_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_TRAFFIC_FSH", std::string(GLES3_TRAFFIC_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_SOLID_COLOR_FSH", std::string(GLES3_SOLID_COLOR_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_HATCHING_AREA_FSH", std::string(GLES3_HATCHING_AREA_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_SMAA_EDGES_FSH", std::string(GLES3_SMAA_EDGES_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_CIRCLE_POINT_FSH", std::string(GLES3_CIRCLE_POINT_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_LINE_FSH", std::string(GLES3_LINE_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_DISCARDED_TEXTURING_FSH", std::string(GLES3_DISCARDED_TEXTURING_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_ROUTE_FSH", std::string(GLES3_ROUTE_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_MASKED_TEXTURING_FSH", std::string(GLES3_MASKED_TEXTURING_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_ROUTE_DASH_FSH", std::string(GLES3_ROUTE_DASH_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_TRAFFIC_LINE_FSH", std::string(GLES3_TRAFFIC_LINE_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_ARROW3D_FSH", std::string(GLES3_ARROW3D_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_TEXTURING_FSH", std::string(GLES3_TEXTURING_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_DASHED_LINE_FSH", std::string(GLES3_DASHED_LINE_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_TEXTURING3D_FSH", std::string(GLES3_TEXTURING3D_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_TEXT_FIXED_FSH", std::string(GLES3_TEXT_FIXED_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_DEBUG_RECT_FSH", std::string(GLES3_DEBUG_RECT_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_SMAA_BLENDING_WEIGHT_FSH", std::string(GLES3_SMAA_BLENDING_WEIGHT_FSH)));
    fragmentEnum.push_back(std::make_pair("GLES3_TEXT_FSH", std::string(GLES3_TEXT_FSH)));
  }
  return fragmentEnum;
}
#endif
}  // namespace gpu
