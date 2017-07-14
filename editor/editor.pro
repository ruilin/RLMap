# Editor specific things.

TARGET = editor
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
  changeset_wrapper.cpp \
  config_loader.cpp \
  editor_config.cpp \
  editor_notes.cpp \
  editor_storage.cpp \
  opening_hours_ui.cpp \
  osm_auth.cpp \
  osm_feature_matcher.cpp \
  server_api.cpp \
  ui2oh.cpp \
  user_stats.cpp \
  xml_feature.cpp \

HEADERS += \
  changeset_wrapper.hpp \
  config_loader.hpp \
  editor_config.hpp \
  editor_notes.hpp \
  editor_storage.hpp \
  opening_hours_ui.hpp \
  osm_auth.hpp \
  osm_feature_matcher.hpp \
  server_api.hpp \
  ui2oh.hpp \
  user_stats.hpp \
  xml_feature.hpp \
  yes_no_unknown.hpp \
