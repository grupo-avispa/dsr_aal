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

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "speechAgent/sound_manager.hpp"

// Functions
SoundManager::SoundManager()
{
  // Initialize variables
}

SoundManager::~SoundManager()
{
  // Free resources
}

// Sound system functions
bool SoundManager::setMasterVolume(const int & percent)
{
  if (DEBUG) {std::cout << LOGTAG << "Setting master volume to " << percent << "%" << std::endl;}
  return runCommand("pactl -- set-sink-volume 0 " + std::to_string(percent) + "%");
}

bool SoundManager::setMasterVolumeUp(const int & percent)
{
  if (DEBUG) {
    std::cout << LOGTAG << "Setting master volume to " << percent << "% higher" << std::endl;
  }
  return runCommand("pactl -- set-sink-volume 0 +" + std::to_string(percent) + "%");
}

bool SoundManager::setMasterVolumeDown(const int & percent)
{
  if (DEBUG) {
    std::cout << LOGTAG << "Setting master volume to " << percent << "% lower" << std::endl;
  }
  return runCommand("pactl -- set-sink-volume 0 -" + std::to_string(percent) + "%");
}

// Multimedia functions
bool SoundManager::playFile(const std::string & file, const double & volumeFactor)
{
  if (file.empty()) {
    std::cout << "WARNING: Could not play an empty file" << std::endl;
  }
  if (DEBUG) {std::cout << LOGTAG << "Playing file \"" << file << "\"" << std::endl;}
  std::string volumeFactorS = std::to_string(volumeFactor);
  std::replace(volumeFactorS.begin(), volumeFactorS.end(), ',', '.');
  return runCommand("play -q -v " + volumeFactorS + " \"" + file + "\" -t alsa");
}

bool SoundManager::playFileAndWait(const std::string & file, const double & volumeFactor)
{
  if (file.empty()) {
    std::cout << "WARNING: Could not play an empty file" << std::endl;
  }
  if (DEBUG) {std::cout << LOGTAG << "Playing file \"" << file << "\"" << std::endl;}
  std::string volumeFactorS = std::to_string(volumeFactor);
  std::replace(volumeFactorS.begin(), volumeFactorS.end(), ',', '.');
  return runCommand("play -q -v " + volumeFactorS + " \"" + file + "\" -t alsa");
}

bool SoundManager::stop()
{
  if (DEBUG) {std::cout << LOGTAG << "Stopping sound" << std::endl;}
  return runCommand("pkill play");
}

// Helpers
bool SoundManager::runCommand(const std::string & command)
{
  if (DEBUG) {std::cout << LOGTAG << "Running command: '" << command << "'" << std::endl;}
  return std::system(command.c_str()) == 0;
}
