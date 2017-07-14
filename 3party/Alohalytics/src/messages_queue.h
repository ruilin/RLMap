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

// This queue stores incoming messages in the memory on a separate thread as a continuous
// block of bytes. If storage directory is set, it stores everything in a file (on a separate thread too).
// When TMaxFileSizeInBytes limit is hit, file is "archived" (see TFileArchiver in constructor)
// and a new file is created instead.
// Destructor gracefully processes all commands left in the queue.

#ifndef MESSAGES_QUEUE_H
#define MESSAGES_QUEUE_H

#include <cerrno>              // errno
#include <condition_variable>  // condition_variable
#include <cstdio>              // rename, remove
#include <ctime>               // time, gmtime
#include <fstream>             // ofstream
#include <functional>          // bind, function
#include <limits>              // numeric_limits
#include <list>                // list
#include <memory>              // unique_ptr
#include <mutex>               // mutex
#include <string>              // string
#include <thread>              // thread

#include "file_manager.h"
#include "logger.h"

namespace alohalytics {

// Archiver function, should process first parameter's file, remove it and store result in the second parameter.
typedef std::function<void(const std::string & file_to_archive, const std::string & archived_file)> TFileArchiver;
// Processor should return true if file was processed successfully.
// If file_name_in_content is true, then second parameter is a full path to a file instead of a buffer.
typedef std::function<bool(bool file_name_in_content, const std::string & content)> TArchivedFilesProcessor;
enum class ProcessingResult { EProcessedSuccessfully, EProcessingError, ENothingToProcess };
typedef std::function<void(ProcessingResult)> TFileProcessingFinishedCallback;

// Default name for "active" file where we store messages.
constexpr char kCurrentFileName[] = "alohalytics_messages";
constexpr char kArchivedFilesExtension[] = ".archived";

// TMaxFileSizeInBytes is a size limit (before gzip) when we archive "current" file and create a new one for appending.
// Optimal size is the one which (gzipped) can be POSTed to the server as one HTTP request.
template <std::streamoff TMaxFileSizeInBytes>
class MessagesQueue final {
 public:
  static constexpr std::streamoff kMaxFileSizeInBytes = TMaxFileSizeInBytes;
  // Default archiving simply renames original file without any additional processing.
  static void ArchiveFileByRenamingIt(const std::string & original_file, const std::string & out_archive) {
    if (std::rename(original_file.c_str(), out_archive.c_str())) {
      ALOG("ERROR: Rename", original_file, "to", out_archive, "has failed with error", std::to_string(errno));
    }
  }

  // Pass custom processing function here, e.g. append IDs, gzip everything before archiving file etc.
  MessagesQueue(TFileArchiver file_archiver = &ArchiveFileByRenamingIt) : file_archiver_(file_archiver) {}

  ~MessagesQueue() {
    {
      std::lock_guard<std::mutex> lock(commands_mutex_);
      worker_thread_should_exit_ = true;
      commands_condition_variable_.notify_all();
    }
    worker_thread_.join();
  }

  // Sets working directory (and flushes in-memory messages into the file).
  // Executed on the WorkerThread.
  void SetStorageDirectory(std::string directory) {
    FileManager::AppendDirectorySlash(directory);
    std::lock_guard<std::mutex> lock(commands_mutex_);
    commands_queue_.push_back(std::bind(&MessagesQueue::ProcessInitializeStorageCommand, this, directory));
    commands_condition_variable_.notify_all();
  }
  // Stores message into a file archive (if SetStorageDirectory was called with a valid directory),
  // otherwise stores messages in-memory.
  // Executed on the WorkerThread.
  void PushMessage(const std::string & message) {
    {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      messages_buffer_.append(message);
    }
    std::lock_guard<std::mutex> lock(commands_mutex_);
    commands_queue_.push_back(std::bind(&MessagesQueue::ProcessMessageCommand, this));
    commands_condition_variable_.notify_all();
  }

  // Processor should return true if file was successfully processed (e.g. uploaded to a server, etc.).
  // File is deleted if processor has returned true.
  // Processing stops if processor returns false.
  // Optional callback is called when all files are processed.
  // This call automatically archives current file before processing, but zero-sized "current" file is never archived.
  // Executed on the WorkerThread.
  void ProcessArchivedFiles(TArchivedFilesProcessor processor,
                            TFileProcessingFinishedCallback callback = TFileProcessingFinishedCallback()) {
    std::lock_guard<std::mutex> lock(commands_mutex_);
    commands_queue_.push_back(std::bind(&MessagesQueue::ProcessArchivedFilesCommand, this, processor, callback));
    commands_condition_variable_.notify_all();
  }

  // This may be needed for correct logrotate utility support on *nix systems.
  void LogrotateCurrentFile() {
    std::lock_guard<std::mutex> lock(commands_mutex_);
    commands_queue_.push_back(std::bind(&MessagesQueue::ProcessLogrotateCurrentFileCommand, this));
    commands_condition_variable_.notify_all();
  }

 private:
  // Returns full path to unique, non-existing file in the given directory.
  // Uses default extension for easier archives scan later.
  static std::string GenerateFullFilePathForArchive(const std::string & directory) {
    std::time_t now = std::time(nullptr);
    std::string generated;
    do {
      generated = directory + kCurrentFileName + "-" + std::to_string(now) + kArchivedFilesExtension;
      ++now;
    } while (std::ifstream(generated).good());
    return generated;
  }

  // current_file_ is single-threaded.
  void ArchiveCurrentFile() {
    if (current_file_) {
      current_file_.reset(nullptr);
      const std::string current_file_path = storage_directory_ + kCurrentFileName;
      file_archiver_(current_file_path.c_str(), GenerateFullFilePathForArchive(storage_directory_).c_str());
      current_file_.reset(new std::ofstream(current_file_path, std::ios_base::app | std::ios_base::binary));
    }
  }

  void StoreMessages(std::string const & messages) {
    if (current_file_) {
      // TODO(AlexZ): Consider using write instead of operator<<.
      *current_file_ << messages << std::flush;
      if (current_file_->fail()) {
        ALOG("ERROR: Write to", storage_directory_ + kCurrentFileName, "has failed.");
      } else if (current_file_->tellp() >= kMaxFileSizeInBytes) {
        ArchiveCurrentFile();
      }
    } else {
      inmemory_storage_.append(messages);
    }
  }

  void ProcessInitializeStorageCommand(const std::string & directory) {
    current_file_.reset(nullptr);
    std::unique_ptr<std::ofstream> new_current_file(
        new std::ofstream(directory + kCurrentFileName, std::ios_base::app | std::ios_base::binary));
    if (new_current_file->fail()) {
      // If file could not be created, fall back to the in-memory storage.
      storage_directory_.clear();
      ALOG("ERROR: Could not create file", directory + kCurrentFileName);
    } else {
      storage_directory_ = directory;
      current_file_ = std::move(new_current_file);
      // Also check if there are any messages in the memory storage, and save them to file.
      if (!inmemory_storage_.empty()) {
        StoreMessages(inmemory_storage_);
        inmemory_storage_.clear();
      }
    }
  }

  void ProcessMessageCommand() {
    std::string messages_buffer_copy;
    {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      if (!messages_buffer_.empty()) {
        messages_buffer_copy.swap(messages_buffer_);
      }
    }
    if (!messages_buffer_copy.empty()) {
      StoreMessages(messages_buffer_copy);
    }
  }

  // If there is no file storage directory set, it should also process messages from the memory buffer.
  void ProcessArchivedFilesCommand(TArchivedFilesProcessor processor, TFileProcessingFinishedCallback callback) {
    ProcessingResult result = ProcessingResult::ENothingToProcess;
    // Process in-memory messages, if any.
    if (!inmemory_storage_.empty()) {
      if (processor(false /* in-memory buffer */, inmemory_storage_)) {
        inmemory_storage_.clear();
        result = ProcessingResult::EProcessedSuccessfully;
      } else {
        result = ProcessingResult::EProcessingError;
      }
      // If in-memory storage is used, then file storage directory was not set and we can't process files.
      // So here we notify callback and return.
      // TODO(AlexZ): Do we need to use in-memory storage if storage is initialized but full/not accessible?
      if (callback) {
        callback(result);
      }
      return;
    }
    if (current_file_ && current_file_->tellp() > 0) {
      ArchiveCurrentFile();
    }
    FileManager::ForEachFileInDir(storage_directory_, [&processor, &result](const std::string & full_path_to_file) {
      // Ignore non-archived files.
      const auto pos = full_path_to_file.rfind(kArchivedFilesExtension);
      if (pos == std::string::npos || pos + sizeof(kArchivedFilesExtension) - 1 != full_path_to_file.size()) {
        return true;
      }
      // Ignore and delete zero-size files.
      try {
        if (0 == FileManager::GetFileSize(full_path_to_file)) {
          std::remove(full_path_to_file.c_str());
          return true;
        }
      } catch (const std::exception &) {
        // File is absent or is a directory.
        return true;
      }
      // Process archived files.
      if (processor(true /* true here means that second parameter is file path */, full_path_to_file)) {
        result = ProcessingResult::EProcessedSuccessfully;
        // Also delete successfully processed archive.
        std::remove(full_path_to_file.c_str());
        return true;
      } else {
        result = ProcessingResult::EProcessingError;
        // Stop processing archives on error.
        return false;
      }
    });
    if (callback) {
      callback(result);
    }
  }

  void ProcessLogrotateCurrentFileCommand() {
    // Here we simply reopen the file. It should be already moved by logrotate.
    current_file_.reset(nullptr);
    current_file_.reset(
        new std::ofstream(storage_directory_ + kCurrentFileName, std::ios_base::app | std::ios_base::binary));
    if (current_file_->fail()) {
      ALOG("ERROR: Could not reopen", storage_directory_ + kCurrentFileName);
    }
  }

  void WorkerThread() {
    TCommand command_to_execute;
    while (true) {
      {
        std::unique_lock<std::mutex> lock(commands_mutex_);
        commands_condition_variable_.wait(lock,
                                          [this] { return !commands_queue_.empty() || worker_thread_should_exit_; });
        if (worker_thread_should_exit_) {
          // Gracefully finish all commands left in the queue and exit.
          for (auto & command : commands_queue_) {
            command();
          }
          commands_queue_.clear();
          return;
        }
        command_to_execute = commands_queue_.front();
        commands_queue_.pop_front();
      }
      command_to_execute();
    }
  }

 private:
  TFileArchiver file_archiver_;
  // Synchronized buffer to pass messages between threads.
  std::string messages_buffer_;
  // Directory with a slash at the end, where we store "current" file and archived files.
  std::string storage_directory_;
  // Used as an in-memory storage if storage_dir_ was not set.
  std::string inmemory_storage_;
  typedef std::function<void()> TCommand;
  std::list<TCommand> commands_queue_;

  // Should be guarded by commands_mutex_.
  bool worker_thread_should_exit_ = false;
  std::mutex messages_mutex_;
  std::mutex commands_mutex_;
  std::condition_variable commands_condition_variable_;
  // Only WorkerThread accesses this variable.
  std::unique_ptr<std::ofstream> current_file_;
  // Should be the last member of the class to initialize after all other members.
  std::thread worker_thread_ = std::thread(&MessagesQueue::WorkerThread, this);
};

typedef MessagesQueue<1024 * 100> THundredKilobytesFileQueue;
// TODO(AlexZ): Remove unnecessary file size checks from this specialization.
typedef MessagesQueue<std::numeric_limits<std::streamoff>::max()> TUnlimitedFileQueue;

}  // namespace alohalytics

#endif  // MESSAGES_QUEUE_H
