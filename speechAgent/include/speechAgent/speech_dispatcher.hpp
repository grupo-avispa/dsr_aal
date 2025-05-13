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

#ifndef SPEECH_DISPATCHER_HPP_
#define SPEECH_DISPATCHER_HPP_

#include <string>

// Initial implementation to use speech-dispatcher
// TODO: Use proper libraries, not system calls
// TODO: Add more command line options and make it configurable
class SpeechDispatcher
{

public:
  // Functions
  SpeechDispatcher();
  ~SpeechDispatcher();

  // Speech functions
  bool say(const std::string & text);
  bool sayAndWait(const std::string & text);
  bool sayWithPicoAndWait(const std::string & text);                 // TODO FIXME: Temporary fix to make better sound. Put it into other class, it's not related to SpeechDispatcher

  bool stopMessage();
  bool cancel();


  // Config functions
  void configOutModule(const std::string & str);
  void configLanguage(const std::string & str);
  void configVoiceType(const std::string & str);
  void configVoice(const std::string & str);

  bool configSpeechVolume(const int & value);
  bool configSpeechRate(const int & value);
  bool configPitch(const int & value);
  bool configPitchRange(const int & value);

protected:

private:
  // Debug
  static const bool DEBUG = false;                                                  ///< Constant value that allows the compiler to exclude log code.
  static constexpr const char * LOGTAG = "SpeechDispatcher: ";                      ///< Defines a log tag with a different name per class.

  // Constants

  // Variables
  std::string outModule;                                     // Output module
  std::string language;                                      // ISO code
  std::string voiceType;                                     // Preferred voice type (male1, male2, male3, female1, female2, female3, child_male, child_female)
  std::string voice;                                         // Synthesis voice
  int volume = 100;                                          // [-100, 100], spd default 100
  int rate = 0;                                              // [-100, 100], spd default 0
  int pitch = 0;                                             // [-100, 100], spd default 0
  int pitchRange = 0;                                        // [-100, 100], spd default 0

  // Functions
  std::string processConfigOptions();

  // Helper functions
  bool runCommand(const std::string & command);
};

#endif  // SPEECH_DISPATCHER_HPP_
