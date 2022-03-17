// Copyright 2019 The Bazel Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BUILD_BAZEL_RULES_SWIFT_TOOLS_WORKER_SWIFT_RUNNER_H_
#define BUILD_BAZEL_RULES_SWIFT_TOOLS_WORKER_SWIFT_RUNNER_H_

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "tools/common/bazel_substitutions.h"
#include "tools/common/temp_file.h"

// Handles spawning the Swift compiler driver, making any required substitutions
// of the command line arguments (for example, Bazel's magic Xcode placeholder
// strings).
//
// The first argument in the list passed to the spawner should be the Swift
// tool that should be invoked (for example, "swiftc"). This spawner also
// recognizes special arguments of the form `-Xwrapped-swift=<arg>`. Arguments
// of this form are consumed entirely by this wrapper and are not passed down to
// the Swift tool (however, they may add normal arguments that will be passed).
//
// The following spawner-specific arguments are supported:
//
// -Xwrapped-swift=-debug-prefix-pwd-is-dot
//     When specified, the Swift compiler will be directed to remap the current
//     directory's path to the string "." in debug info. This remapping must be
//     applied here because we do not know the current working directory at
//     analysis time when the argument list is constructed.
//
// -Xwrapped-swift=-ephemeral-module-cache
//     When specified, the spawner will create a new temporary directory, pass
//     that to the Swift compiler using `-module-cache-path`, and then delete
//     the directory afterwards. This should resolve issues where the module
//     cache state is not refreshed correctly in all situations, which
//     sometimes results in hard-to-diagnose crashes in `swiftc`.
class SwiftRunner {
 public:
  // Create a new spawner that launches a Swift tool with the given arguments.
  // The first argument is assumed to be that tool. If force_response_file is
  // true, then the remaining arguments will be unconditionally written into a
  // response file instead of being passed on the command line.
  SwiftRunner(const std::vector<std::string> &args,
              bool force_response_file = false);

  // Run the Swift compiler, redirecting stderr to the specified stream. If
  // stdout_to_stderr is true, then stdout is also redirected to that stream.
  int Run(std::ostream &stderr_stream, bool stdout_to_stderr = false);

 private:
  // Processes an argument that looks like it might be a response file (i.e., it
  // begins with '@') and returns true if the argument(s) passed to the consumer
  // were different than "arg").
  //
  // If the argument is not actually a response file (i.e., it begins with '@'
  // but the file cannot be read), then it is passed directly to the consumer
  // and this method returns false. Otherwise, if the response file could be
  // read, this method's behavior depends on a few factors:
  //
  // - If the spawner is forcing response files, then the arguments in this
  //   response file are read and processed and sent directly to the consumer.
  //   In other words, they will be rewritten into that new response file
  //   directly, rather than being kept in their own separate response file.
  //   This is because there is no reason to maintain the original and multiple
  //   response files at this stage of processing. In this case, the function
  //   returns true.
  //
  // - If the spawner is not forcing response files, then the arguments in this
  //   response file are read and processed. If none of the arguments changed,
  //   then this function passes the original response file argument to the
  //   consumer and returns false. If some arguments did change, then they are
  //   written to a new response file, a response file argument pointing to that
  //   file is passed to the consumer, and the method returns true.
  bool ProcessPossibleResponseFile(
      absl::string_view arg, std::function<void(absl::string_view)> consumer);

  // Applies substitutions for a single argument and passes the new arguments
  // (or the original, if no substitution was needed) to the consumer. Returns
  // true if any substitutions were made (that is, if the arguments passed to
  // the consumer were anything different than "arg").
  //
  // This method has file system side effects, creating temporary files and
  // directories as needed for a particular substitution.
  bool ProcessArgument(absl::string_view arg,
                       std::function<void(absl::string_view)> consumer);

  // Applies substitutions to the given command line arguments, returning the
  // results in a new vector.
  std::vector<std::string> ProcessArguments(
      const std::vector<std::string> &args);

  // A mapping of Bazel placeholder strings to the actual paths that should be
  // substituted for them. Supports Xcode resolution on Apple OSes.
  bazel_rules_swift::BazelPlaceholderSubstitutions
      bazel_placeholder_substitutions_;

  // The arguments, post-substitution, passed to the spawner.
  std::vector<std::string> args_;

  // Temporary files (e.g., rewritten response files) that should be cleaned up
  // after the driver has terminated.
  std::vector<std::unique_ptr<TempFile>> temp_files_;

  // Temporary directories (e.g., ephemeral module cache) that should be cleaned
  // up after the driver has terminated.
  std::vector<std::unique_ptr<TempDirectory>> temp_directories_;

  // Arguments will be unconditionally written into a response file and passed
  // to the tool that way.
  bool force_response_file_;

  // The path to the generated header rewriter tool, if one is being used for
  // this compilation.
  std::string generated_header_rewriter_path_;
};

#endif  // BUILD_BAZEL_RULES_SWIFT_TOOLS_WORKER_SWIFT_RUNNER_H_
