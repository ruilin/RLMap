/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>
#include <sstream>

#if defined(__OBJC__)
#include <Foundation/Foundation.h>
#elif defined(ANDROID)
#include <android/log.h>
#else
#include <iostream>
#endif

// Pretty-printing for std::pair.
template <typename T, typename U>
std::ostream & operator<<(std::ostream & out, const std::pair<T, U> & p) {
  out << p.first << "=" << p.second;
  return out;
}

namespace alohalytics {

class Logger {
  std::ostringstream out_;

 public:
  Logger() = default;

  Logger(const char * file, int line) { out_ << file << ':' << line << ": "; }

  ~Logger() {
#if defined(__OBJC__)
    // NSLog prints time automatically.
    NSLog(@"Alohalytics: %s", out_.str().c_str());
#elif defined(ANDROID)
    // Android also prints time automatically.
    __android_log_print(ANDROID_LOG_INFO, "Alohalytics", "%s", out_.str().c_str());
#else
    char buf[100] = "";
    const time_t now = time(nullptr);
    (void)::strftime(buf, sizeof(buf), "%d/%b/%Y:%H:%M:%S ", ::localtime(&now));
    std::cout << buf << "Alohalytics: " << out_.str() << std::endl;
#endif
  }

  template <typename T, typename... ARGS>
  void Log(const T & arg1, const ARGS &... others) {
    Log(arg1);
    out_ << ' ';
    Log(others...);
  }

  void Log() {}

  template <typename T>
  void Log(const T & t) {
    out_ << t;
  }

  // String specialization to avoid printing every character as a container's element.
  void Log(const std::string & t) {
    if (t.empty()) {
      out_ << "<EMPTY_STRING>";
    } else {
      out_ << t;
    }
  }

  // Specialization to work with nullptr
  void Log(const char * t) {
    if (t) {
      out_ << t;
    } else {
      out_ << "<NULL>";
    }
  }

  // Pretty-printing for containers.
  template <template <typename, typename...> class ContainerType, typename ValueType, typename... Args>
  void Log(const ContainerType<ValueType, Args...> & c) {
    out_ << '{';
    size_t index = 0;
    const size_t count = c.size();
    for (const auto & v : c) {
      out_ << v;
      if (++index != count) {
        out_ << ',';
      }
    }
    out_ << '}';
  }
};

}  // namespace alohalytics

#define ATRACE(...) alohalytics::Logger(__FILE__, __LINE__).Log(__VA_ARGS__)
#define ALOG(...) alohalytics::Logger().Log(__VA_ARGS__)

#endif  // #ifndef LOGGER_H
