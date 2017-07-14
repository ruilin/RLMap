#pragma once

#include "drape_frontend/kinetic_scroller.hpp"
#include "drape_frontend/navigator.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"

#include "base/timer.hpp"

#include "std/array.hpp"
#include "std/bitset.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/list.hpp"

namespace df
{

int const kDoNotChangeZoom = -1;
double const kDoNotAutoZoom = -1.0;

using TAnimationCreator = function<drape_ptr<Animation>(ref_ptr<Animation>)>;

class UserEvent
{
public:
  enum class EventType
  {
    Touch,
    Scale,
    SetCenter,
    SetRect,
    SetAnyRect,
    Resize,
    Rotate,
    FollowAndRotate,
    AutoPerspective,
    VisibleViewport
  };

  virtual ~UserEvent() {}
  virtual EventType GetType() const = 0;
};

struct Touch
{
  m2::PointF m_location = m2::PointF::Zero();
  int64_t m_id = -1; // if id == -1 then touch is invalid
  float m_force = 0.0; // relative force of touch [0.0 - 1.0]
};

class TouchEvent : public UserEvent
{
public:
  TouchEvent()
    : m_type(TOUCH_CANCEL)
    , m_timeStamp(my::Timer::LocalTime())
    , m_pointersMask(0xFFFF)
  {
  }

  enum ETouchType
  {
    TOUCH_DOWN,
    TOUCH_MOVE,
    TOUCH_UP,
    TOUCH_CANCEL
  };

  static uint8_t const INVALID_MASKED_POINTER;

  EventType GetType() const override { return UserEvent::EventType::Touch; }

  ETouchType GetTouchType() const { return m_type; }
  void SetTouchType(ETouchType touchType) { m_type = touchType; }

  double GetTimeStamp() const { return m_timeStamp; }
  void SetTimeStamp(double timeStamp) { m_timeStamp = timeStamp; }

  array<Touch, 2> const & GetTouches() const { return m_touches; }

  Touch const & GetFirstTouch() const { return m_touches[0]; }
  Touch const & GetSecondTouch() const { return m_touches[1]; }

  void SetFirstTouch(Touch const & touch);
  void SetSecondTouch(Touch const & touch);

  void PrepareTouches(array<Touch, 2> const & previousToches);

  /// Methods for work with current touches
  /// For example : user put down one finger. We will have one touch in m_touches
  /// and GetFirstMaskedPointer return index of this pointer in m_touches (0 in this case)
  /// Then user puts down the second finger. m_touches will have 2 valid elements, but new finger only one.
  /// In this case GetFirstMaskedPointer returns index of new pointer.
  /// If user put down both fingers simultaneously, then GetFirst and GetSecond
  /// will return valid not equal INVALID_MASKED_POINTER
  void SetFirstMaskedPointer(uint8_t firstMask);
  uint8_t GetFirstMaskedPointer() const;
  void SetSecondMaskedPointer(uint8_t secondMask);
  uint8_t GetSecondMaskedPointer() const;
  size_t GetMaskedCount();

private:
  void Swap();

  ETouchType m_type;
  array<Touch, 2> m_touches; // array of all touches
  double m_timeStamp; // seconds
  uint16_t m_pointersMask;
};

class ScaleEvent : public UserEvent
{
public:
  ScaleEvent(double factor, m2::PointD const & pxPoint, bool isAnim)
    : m_factor(factor)
    , m_pxPoint(pxPoint)
    , m_isAnim(isAnim)
  {
  }

  EventType GetType() const override { return UserEvent::EventType::Scale; }

  double GetFactor() const { return m_factor; }
  m2::PointD const & GetPxPoint() const { return m_pxPoint; }
  bool IsAnim() const { return m_isAnim; }

private:
  double m_factor;
  m2::PointD m_pxPoint;
  bool m_isAnim;
};

class SetCenterEvent : public UserEvent
{
public:
  SetCenterEvent(m2::PointD const & center, int zoom, bool isAnim,
                 TAnimationCreator const & parallelAnimCreator = nullptr)
    : m_center(center)
    , m_zoom(zoom)
    , m_isAnim(isAnim)
    , m_parallelAnimCreator(parallelAnimCreator)
  {
  }

  EventType GetType() const override { return UserEvent::EventType::SetCenter; }

  m2::PointD const & GetCenter() const { return m_center; }
  int GetZoom() const { return m_zoom; }
  bool IsAnim() const { return m_isAnim; }
  TAnimationCreator const & GetParallelAnimCreator() const { return m_parallelAnimCreator; }

private:
  m2::PointD m_center; // center point in mercator
  int m_zoom; // if zoom == -1, then zoom level will'n change
  bool m_isAnim;
  TAnimationCreator m_parallelAnimCreator;
};

class SetRectEvent : public UserEvent
{
public:
  SetRectEvent(m2::RectD const & rect, bool rotate, int zoom, bool isAnim,
               TAnimationCreator const & parallelAnimCreator = nullptr)
    : m_rect(rect)
    , m_applyRotation(rotate)
    , m_zoom(zoom)
    , m_isAnim(isAnim)
    , m_parallelAnimCreator(parallelAnimCreator)
  {
  }

  EventType GetType() const override { return UserEvent::EventType::SetRect; }

  m2::RectD const & GetRect() const { return m_rect; }
  bool GetApplyRotation() const { return m_applyRotation; }
  int GetZoom() const { return m_zoom; }
  bool IsAnim() const { return m_isAnim; }
  TAnimationCreator const & GetParallelAnimCreator() const { return m_parallelAnimCreator; }

private:
  m2::RectD m_rect; // destination mercator rect
  bool m_applyRotation; // if true, current rotation will be apply to m_rect
  int m_zoom; // if zoom == -1, then zoom level will'n change
  bool m_isAnim;
  TAnimationCreator m_parallelAnimCreator;
};

class SetAnyRectEvent : public UserEvent
{
public:
  SetAnyRectEvent(m2::AnyRectD const & rect, bool isAnim)
    : m_rect(rect)
    , m_isAnim(isAnim)
  {}

  EventType GetType() const override { return UserEvent::EventType::SetAnyRect; }

  m2::AnyRectD const & GetRect() const { return m_rect; }
  bool IsAnim() const { return m_isAnim; }

private:
  m2::AnyRectD m_rect;  // destination mercator rect
  bool m_isAnim;
};

class FollowAndRotateEvent : public UserEvent
{
public:
  FollowAndRotateEvent(m2::PointD const & userPos, m2::PointD const & pixelZero,
                       double azimuth, double autoScale, TAnimationCreator const & parallelAnimCreator = nullptr)
    : m_userPos(userPos)
    , m_pixelZero(pixelZero)
    , m_azimuth(azimuth)
    , m_preferredZoomLevel(kDoNotChangeZoom)
    , m_autoScale(autoScale)
    , m_isAutoScale(true)
    , m_isAnim(true)
    , m_parallelAnimCreator(parallelAnimCreator)
  {
  }

  FollowAndRotateEvent(m2::PointD const & userPos, m2::PointD const & pixelZero,
                       double azimuth, int preferredZoomLevel,
                       bool isAnim, TAnimationCreator const & parallelAnimCreator = nullptr)
    : m_userPos(userPos)
    , m_pixelZero(pixelZero)
    , m_azimuth(azimuth)
    , m_preferredZoomLevel(preferredZoomLevel)
    , m_autoScale(kDoNotAutoZoom)
    , m_isAutoScale(false)
    , m_isAnim(isAnim)
    , m_parallelAnimCreator(parallelAnimCreator)
  {}

  EventType GetType() const override { return UserEvent::EventType::FollowAndRotate; }

  m2::PointD const & GetUserPos() const { return m_userPos; }
  m2::PointD const & GetPixelZero() const { return m_pixelZero; }
  double GetAzimuth() const { return m_azimuth; }
  int GetPreferredZoomLelel() const { return m_preferredZoomLevel; }
  double GetAutoScale() const { return m_autoScale; }
  bool IsAutoScale() const { return m_isAutoScale; }
  bool IsAnim() const { return m_isAnim; }
  TAnimationCreator const & GetParallelAnimCreator() const { return m_parallelAnimCreator; }

private:
  m2::PointD m_userPos;
  m2::PointD m_pixelZero;
  double m_azimuth;
  int m_preferredZoomLevel;
  double m_autoScale;
  bool m_isAutoScale;
  bool m_isAnim;
  TAnimationCreator m_parallelAnimCreator;
};

class SetAutoPerspectiveEvent : public UserEvent
{
public:
  SetAutoPerspectiveEvent(bool isAutoPerspective)
    : m_isAutoPerspective(isAutoPerspective)
  {}

  EventType GetType() const override { return UserEvent::EventType::AutoPerspective; }

  bool IsAutoPerspective() const { return m_isAutoPerspective; }

private:
  bool m_isAutoPerspective;
};

class RotateEvent : public UserEvent
{
public:
  RotateEvent(double targetAzimut, TAnimationCreator const & parallelAnimCreator = nullptr)
    : m_targetAzimut(targetAzimut)
    , m_parallelAnimCreator(parallelAnimCreator)
  {}

  EventType GetType() const override { return UserEvent::EventType::Rotate; }

  double GetTargetAzimuth() const { return m_targetAzimut; }
  TAnimationCreator const & GetParallelAnimCreator() const { return m_parallelAnimCreator; }

private:
  double m_targetAzimut;
  TAnimationCreator m_parallelAnimCreator;
};

class ResizeEvent : public UserEvent
{
public:
  ResizeEvent(uint32_t w, uint32_t h) : m_width(w), m_height(h) {}

  EventType GetType() const override { return UserEvent::EventType::Resize; }

  uint32_t GetWidth() const { return m_width; }
  uint32_t GetHeight() const { return m_height; }

private:
  uint32_t m_width;
  uint32_t m_height;
};

class SetVisibleViewportEvent : public UserEvent
{
public:
  SetVisibleViewportEvent(m2::RectD const & rect)
    : m_rect(rect)
  {}

  EventType GetType() const override { return UserEvent::EventType::VisibleViewport; }

  m2::RectD const & GetRect() const { return m_rect; }

private:
  m2::RectD m_rect;
};

class UserEventStream
{
public:
  class Listener
  {
  public:
    virtual ~Listener() {}

    virtual void OnTap(m2::PointD const & pt, bool isLong) = 0;
    virtual void OnForceTap(m2::PointD const & pt) = 0;
    virtual void OnDoubleTap(m2::PointD const & pt) = 0;
    virtual void OnTwoFingersTap() = 0;
    virtual bool OnSingleTouchFiltrate(m2::PointD const & pt, TouchEvent::ETouchType type) = 0;
    virtual void OnDragStarted() = 0;
    virtual void OnDragEnded(m2::PointD const & distance) = 0;

    virtual void OnScaleStarted() = 0;
    virtual void OnRotated() = 0;
    virtual void CorrectScalePoint(m2::PointD & pt) const = 0;
    virtual void CorrectGlobalScalePoint(m2::PointD & pt) const = 0;
    virtual void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const = 0;
    virtual void OnScaleEnded() = 0;
    virtual void OnAnimatedScaleEnded() = 0;

    virtual void OnTouchMapAction() = 0;

    virtual bool OnNewVisibleViewport(m2::RectD const & oldViewport, m2::RectD const & newViewport, m2::PointD & gOffset) = 0;
  };

  UserEventStream();
  void AddEvent(drape_ptr<UserEvent> && event);
  ScreenBase const & ProcessEvents(bool & modelViewChanged, bool & viewportChanged);
  ScreenBase const & GetCurrentScreen() const;
  m2::RectD const & GetVisibleViewport() const;

  void GetTargetScreen(ScreenBase & screen);
  m2::AnyRectD GetTargetRect();

  bool IsInUserAction() const;
  bool IsWaitingForActionCompletion() const;

  void SetListener(ref_ptr<Listener> listener) { m_listener = listener; }

  void SetKineticScrollEnabled(bool enabled);

#ifdef DEBUG
  static char const * BEGIN_DRAG;
  static char const * DRAG;
  static char const * END_DRAG;
  static char const * BEGIN_SCALE;
  static char const * SCALE;
  static char const * END_SCALE;
  static char const * BEGIN_TAP_DETECTOR;
  static char const * LONG_TAP_DETECTED;
  static char const * SHORT_TAP_DETECTED;
  static char const * CANCEL_TAP_DETECTOR;
  static char const * TRY_FILTER;
  static char const * END_FILTER;
  static char const * CANCEL_FILTER;
  static char const * TWO_FINGERS_TAP;
  static char const * BEGIN_DOUBLE_TAP_AND_HOLD;
  static char const * DOUBLE_TAP_AND_HOLD;
  static char const * END_DOUBLE_TAP_AND_HOLD;

  using TTestBridge = function<void (char const * action)>;
  void SetTestBridge(TTestBridge const & fn) { m_testFn = fn; }
#endif

private:
  bool OnSetScale(ref_ptr<ScaleEvent> scaleEvent);
  bool OnSetAnyRect(ref_ptr<SetAnyRectEvent> anyRectEvent);
  bool OnSetRect(ref_ptr<SetRectEvent> rectEvent);
  bool OnSetCenter(ref_ptr<SetCenterEvent> centerEvent);
  bool OnRotate(ref_ptr<RotateEvent> rotateEvent);

  bool SetAngle(double azimuth, TAnimationCreator const & parallelAnimCreator = nullptr);
  bool SetRect(m2::RectD rect, int zoom, bool applyRotation, bool isAnim,
               TAnimationCreator const & parallelAnimCreator = nullptr);
  bool SetRect(m2::AnyRectD const & rect, bool isAnim,
               TAnimationCreator const & parallelAnimCreator = nullptr);

  bool SetScreen(ScreenBase const & screen, bool isAnim,
                 TAnimationCreator const & parallelAnimCreator = nullptr);
  bool SetFollowAndRotate(m2::PointD const & userPos, m2::PointD const & pixelPos,
                          double azimuth, int preferredZoomLevel, double autoScale,
                          bool isAnim, bool isAutoScale,
                          TAnimationCreator const & parallelAnimCreator = nullptr);
  void SetAutoPerspective(bool isAutoPerspective);
  void CheckAutoRotate();

  m2::AnyRectD GetCurrentRect() const;

  bool ProcessTouch(TouchEvent const & touch);

  bool TouchDown(array<Touch, 2> const & touches);
  bool TouchMove(array<Touch, 2> const & touches, double timestamp);
  bool TouchCancel(array<Touch, 2> const & touches);
  bool TouchUp(array<Touch, 2> const & touches);
  void UpdateTouches(array<Touch, 2> const & touches);

  void BeginDrag(Touch const & t, double timestamp);
  void Drag(Touch const & t, double timestamp);
  // EndDrag returns false in case of kinetic moving after dragging has begun.
  bool EndDrag(Touch const & t, bool cancelled);

  void BeginScale(Touch const & t1, Touch const & t2);
  void Scale(Touch const & t1, Touch const & t2);
  void EndScale(Touch const & t1, Touch const & t2);

  void BeginTapDetector();
  void DetectShortTap(Touch const & touch);
  void DetectLongTap(Touch const & touch);
  bool DetectDoubleTap(Touch const & touch);
  void PerformDoubleTap(Touch const & touch);
  bool DetectForceTap(Touch const & touch);
  void EndTapDetector(Touch const & touch);
  void CancelTapDetector();
  
  void StartDoubleTapAndHold(Touch const & touch);
  void UpdateDoubleTapAndHold(Touch const & touch);
  void EndDoubleTapAndHold(Touch const & touch);

  void BeginTwoFingersTap(Touch const & t1, Touch const & t2);
  void EndTwoFingersTap();

  bool TryBeginFilter(Touch const & t);
  void EndFilter(Touch const & t);
  void CancelFilter(Touch const & t);

  void ApplyAnimations();
  void ResetAnimations(Animation::Type animType, bool rewind = true, bool finishAll = false);
  void ResetAnimations(Animation::Type animType, string const & customType, bool rewind = true, bool finishAll = false);
  void ResetMapPlaneAnimations();
  bool InterruptFollowAnimations(bool force);

  bool CheckDrag(array<Touch, 2> const & touches, double threshold) const;

  using TEventsList = list<drape_ptr<UserEvent>>;
  TEventsList m_events;
  mutable mutex m_lock;

  m2::RectD m_visibleViewport;

  Navigator m_navigator;
  my::Timer m_touchTimer;
  enum ERecognitionState
  {
    STATE_EMPTY,
    STATE_FILTER,
    STATE_TAP_DETECTION,
    STATE_WAIT_DOUBLE_TAP,
    STATE_TAP_TWO_FINGERS,
    STATE_WAIT_DOUBLE_TAP_HOLD,
    STATE_DOUBLE_TAP_HOLD,
    STATE_DRAG,
    STATE_SCALE
  } m_state;

  array<Touch, 2> m_touches;

  AnimationSystem & m_animationSystem;

  bool m_modelViewChanged = false;

  unique_ptr<UserEvent> m_pendingEvent;

  ref_ptr<Listener> m_listener;

#ifdef DEBUG
  TTestBridge m_testFn;
#endif
  m2::PointD m_startDragOrg;
  array<m2::PointF, 2> m_twoFingersTouches;
  m2::PointD m_startDoubleTapAndHold;

  KineticScroller m_scroller;
  my::Timer m_kineticTimer;
  bool m_kineticScrollEnabled = true;
};

}