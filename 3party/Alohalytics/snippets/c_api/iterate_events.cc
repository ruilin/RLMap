/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2016 Maps.Me

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#define EXPORT extern "C"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <set>
#include <string>

#include "../../src/event_base.h"
#include "../../src/processor.h"


const size_t kUidLength = 32;

// Public struct used in Python.
struct UserInfo
{
  char os;
  float lat;
  float lon;
  char raw_uid[kUidLength];

  UserInfo()
  : os(0), lat(0), lon(0)
  {
    std::memset(raw_uid, 0, sizeof(raw_uid));
  }

  /*
  Helper function for setting up os type as a num for use in Python.
  NOTE: C char will be converted into python string - so we do not want that.
  */
  void setOSType(char osType)
  {
    switch (osType)
    {
      case 'A':
      {
        this->os = 1;
        break;
      }
      case 'I':
      {
        this->os = 2;
        break;
      }
    };
  }

  std::string compressUid(std::string uid) noexcept(false)
  {
    if (uid.size() < kUidLength)
    {
      throw std::logic_error("Unidentified UID format");
    }
    
    uid.erase(0, 2);
    // TODO: think about just using fixed positions instead of more general attempt with searching.
    uid.erase(std::remove(uid.begin(), uid.end(), '-'), uid.end());

    if (uid.size() != kUidLength)
    {
      throw std::logic_error("Unidentified UID format");
    }
  
    return uid;
  }


  // Throw away useless characters and convert to char array (fixed size - no null-termination needed).
  void setUid(std::string const & uid) noexcept(false)
  {
      std::string cUid = this->compressUid(uid);
      std::memcpy(this->raw_uid, cUid.c_str(), sizeof(char) * cUid.size());
  }

  // Helper function for setting up geo coords.
  void setGeo(alohalytics::Location const & location)
  {
    if (location.HasLatLon())
    {
      this->lat = static_cast<float>(location.latitude_deg_);
      this->lon = static_cast<float>(location.longitude_deg_);
    }
  }
};

// Public struct used in Python.
struct EventTime
{
  uint64_t client_creation;
  uint64_t server_upload;

  EventTime(uint64_t client_creation, uint64_t server_upload)
  : client_creation(client_creation), server_upload(server_upload)
  {
  }
};

// Can't use std::function here. Possibly because of ctypes (Python) limitations.
using TCallback = void (char const *, EventTime const &, UserInfo const &, char const **, int);

/*
Helper function that flattens KeyPairs events to a 1-dim plain lists
processed on the Python side as lists of pairs while converting them to dicts.
*/
std::vector<char const *> getPairs(std::map<std::string, std::string> const & eventPairs)
{
  std::vector<char const *> pairs(eventPairs.size() * 2);

  for (auto const & kp : eventPairs)
  {
    pairs.emplace_back(kp.first.c_str());
    pairs.emplace_back(kp.second.c_str());
  }

  return pairs;
}

/*
This is public function used in Python as a main loop.
NOTE: We need a C interface, so prepare yourself for C-style structures in the params of
this function (called from a Python code with a ctypes wrapper) and
callback function (actual Python function - also a ctypes wrapper).
*/
EXPORT void Iterate(TCallback callback, char const ** eventNames, int numEventNames, int eventsLimit)
{
  std::set<std::string> processKeys(eventNames, eventNames + numEventNames);

  alohalytics::Processor(
      [&callback, &processKeys](AlohalyticsIdServerEvent const * idEvent, AlohalyticsKeyEvent const * event)
      {
        if (!processKeys.empty() && !processKeys.count(event->key))
          return;

        char const * key = event->key.c_str();

        EventTime const eventTime(event->timestamp, idEvent->server_timestamp);

        UserInfo userInfo;
        userInfo.setOSType(idEvent->id[0]);
        userInfo.setUid(idEvent->id);

        AlohalyticsKeyValueLocationEvent const * kvle = dynamic_cast<AlohalyticsKeyValueLocationEvent const *>(event);
        if (kvle)
        {
          char const * kValues[1] = {kvle->value.c_str()};

          userInfo.setGeo(kvle->location);

          callback(key, eventTime, userInfo, kValues, 1 /* length of kValues */);
          return;
        }

        AlohalyticsKeyLocationEvent const * kle = dynamic_cast<AlohalyticsKeyLocationEvent const *>(event);
        if (kle)
        {
          userInfo.setGeo(kle->location);

          callback(key, eventTime, userInfo, NULL, 0); // no data in event so data list and its size are NULL and 0
          return;
        }

        AlohalyticsKeyPairsLocationEvent const * kple = dynamic_cast<AlohalyticsKeyPairsLocationEvent const *>(event);
        if (kple)
        {
          std::vector<char const *> pairs = getPairs(kple->pairs);

          userInfo.setGeo(kple->location);

          callback(key, eventTime, userInfo, pairs.data(), pairs.size());
          return;
        }

        AlohalyticsKeyValueEvent const * kve = dynamic_cast<AlohalyticsKeyValueEvent const *>(event);
        if (kve)
        {
          char const * kValues[1] = {kve->value.c_str()};

          callback(key, eventTime, userInfo, kValues, 1 /* length of kValues */);
          return;
        }

        AlohalyticsKeyPairsEvent const * kpe = dynamic_cast<AlohalyticsKeyPairsEvent const *>(event);
        if (kpe)
        {
          std::vector<char const *> pairs = getPairs(kpe->pairs);

          callback(key, eventTime, userInfo, pairs.data(), pairs.size());
          return;
        }

        AlohalyticsKeyEvent const * ke = dynamic_cast<AlohalyticsKeyEvent const *>(event);
        if (ke)
        {
          callback(key, eventTime, userInfo, NULL, 0); // no data in event so data list and its size are NULL and 0
          return;
        }

      }, std::cin, eventsLimit);
}
