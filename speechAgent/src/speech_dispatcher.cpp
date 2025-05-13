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

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "speechAgent/speech_dispatcher.hpp"

// TODO: Check wav+filter:
//		text="Ya me marcho, muchas gracias por escucharme, que tengan un buen día y disfruten de los eventos. Nos vemos pronto"
//		pico2wave -l "es-ES" -w pico.wav "$text" && play -qV0 pico.wav treble 24 gain -l -6


// Functions
SpeechDispatcher::SpeechDispatcher()
{
  // Initialize variables
  outModule = "pico";
  language = "en";
}

SpeechDispatcher::~SpeechDispatcher()
{
  // Free resources
  cancel();
}

// Speech functions
bool SpeechDispatcher::say(const std::string & text)
{
  if (DEBUG) {std::cout << LOGTAG << "Saying \"" << text << "\"" << std::endl;}
  const std::string command = "spd-say " + processConfigOptions() + " \"" + text + "\"";
  std::cout << "JP: config to say: " << command << std::endl;
  return runCommand(command);
}
bool SpeechDispatcher::sayAndWait(const std::string & text)
{
  if (DEBUG) {
    std::cout << LOGTAG << "Waiting until \"" << text << "\" is spoken or discarded" << std::endl;
  }
  const std::string command = "spd-say " + processConfigOptions() + " -w \"" + text + "\"";
  return runCommand(command);
}
bool SpeechDispatcher::sayWithPicoAndWait(const std::string & text)              // TODO FIXME: Temporary fix to make better sound. Put it into other class, it's not related to SpeechDispatcher
{
  if (DEBUG) {std::cout << LOGTAG << "Saying \"" << text << "\"" << std::endl;}

  const std::string filename = "/tmp/picotmp.wav";
//	const std::string command = "pico2wave -l es-ES -w \""+filename+"\" \""+text+"\" && play -qV2 \""+filename+"\" treble 18";

  int pico_volume = (int)(volume / 2);     // pico volume goes from -50 to 50
  std::string volume_s = std::to_string(pico_volume);
  const std::string command = "pico2wave -l es-ES -w \"" + filename + "\" \"" + text +
    "\" && play -qV2 \"" + filename + "\" treble 18 gain -l " + volume_s;
  if (DEBUG) {std::cout << "Command to say: " << command << std::endl;}
  return runCommand(command);
}

bool SpeechDispatcher::stopMessage()
{
  if (DEBUG) {std::cout << LOGTAG << "Stop saying current message" << std::endl;}
  return runCommand("spd-say -S");
}

bool SpeechDispatcher::cancel()
{
  if (DEBUG) {std::cout << LOGTAG << "Stop saying all messages" << std::endl;}
  return runCommand("spd-say -C");
}

// Config functions
void SpeechDispatcher::configOutModule(const std::string & str)
{
  if (DEBUG) {std::cout << LOGTAG << "Setting output module to " << str << std::endl;}
  outModule = str;
}

void SpeechDispatcher::configLanguage(const std::string & str)
{
  if (DEBUG) {std::cout << LOGTAG << "Setting language to " << str << std::endl;}
  language = str;
}

void SpeechDispatcher::configVoiceType(const std::string & str)
{
  if (DEBUG) {std::cout << LOGTAG << "Setting voice type to " << str << std::endl;}
  voiceType = str;
}

void SpeechDispatcher::configVoice(const std::string & str)
{
  if (DEBUG) {std::cout << LOGTAG << "Setting voice to " << str << std::endl;}
  voice = str;
}

bool SpeechDispatcher::configSpeechVolume(const int & value)
{
  if ((value < -100) || (value > 100)) {
    std::cout << "WARNING: Ignoring speech volume out of range [-100,100]: " << value << std::endl;
    return false;
  }
  if (DEBUG) {std::cout << LOGTAG << "Setting speech volume to " << value << std::endl;}
  volume = value;
  return true;
}

bool SpeechDispatcher::configSpeechRate(const int & value)
{
  if ((value < -100) || (value > 100)) {
    std::cout << "WARNING: Ignoring speech rate out of range [-100,100]: " << value << std::endl;
    return false;
  }
  if (DEBUG) {std::cout << LOGTAG << "Setting speech rate to " << value << std::endl;}
  rate = value;
  return true;
}

bool SpeechDispatcher::configPitch(const int & value)
{
  if ((value < -100) || (value > 100)) {
    std::cout << "WARNING: Ignoring pitch out of range [-100,100]: " << value << std::endl;
    return false;
  }
  if (DEBUG) {std::cout << LOGTAG << "Setting pitch to " << value << std::endl;}
  pitch = value;
  return true;
}

bool SpeechDispatcher::configPitchRange(const int & value)
{
  if ((value < -100) || (value > 100)) {
    std::cout << "WARNING: Ignoring pitch-range out of range [-100,100]: " << value << std::endl;
    return false;
  }
  if (DEBUG) {std::cout << LOGTAG << "Setting pitch range to " << value << std::endl;}
  pitchRange = value;
  return true;
}

std::string SpeechDispatcher::processConfigOptions()
{
  std::ostringstream out;
  if (!outModule.empty()) {out << " -o " << outModule;}
  if (!language.empty()) {out << " -l " << language;}
  if (!voiceType.empty()) {out << " -t " << voiceType;}
  if (!voice.empty()) {out << " -y " << voice;}
  if (volume != 100) {out << " -i " << volume;}
  if (rate != 0) {out << " -r " << rate;}
  if (pitch != 0) {out << " -p " << pitch;}
  if (pitchRange != 0) {out << " -R " << pitchRange;}
  return out.str();
}

// Helpers
bool SpeechDispatcher::runCommand(const std::string & command)
{
  if (DEBUG) {std::cout << LOGTAG << "Running command: '" << command << "'" << std::endl;}
  return std::system(command.c_str()) == 0;
}
