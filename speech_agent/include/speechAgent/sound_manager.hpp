// Copyright (c) 2022 Grupo Avispa, DTE, Universidad de Málaga
// Copyright (c) 2022 Alejandro Cruces
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SOUND_MANAGER_HPP_
#define SOUND_MANAGER_HPP_

#include <string>


// Initial implementation to managee sound settings and play sound
// TODO: Use proper libraries, not system calls
class SoundManager
{

public:
  // Functions
  SoundManager();
  ~SoundManager();


  // Sound system functions
  bool setMasterVolume(const int & percent);
  bool setMasterVolumeUp(const int & percent);
  bool setMasterVolumeDown(const int & percent);


  // Multimedia functions
  bool playFile(const std::string & file, const double & volumeFactor = 1);
  bool playFileAndWait(const std::string & file, const double & volumeFactor = 1);

  bool stop();

protected:

private:
  // Debug
  static const bool DEBUG = false;                                      ///< Constant value that allows the compiler to exclude log code.
  static constexpr const char * LOGTAG = "SoundManager: ";              ///< Defines a log tag with a different name per class.

  // Constants

  // Variables

  // Functions

  // Helper functions
  bool runCommand(const std::string & command);
};

#endif  // SOUND_MANAGER_HPP_
