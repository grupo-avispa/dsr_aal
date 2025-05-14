// Copyright (c) 2024 Grupo Avispa, DTE, Universidad de Málaga
// Copyright (c) 2024 Juan Pedro Bandera Rubio
// Copyright (c) 2024 Alberto J. Tudela Roldán
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

#ifndef SPEECHAGENT__SPEECH_AGENT_HPP_
#define SPEECHAGENT__SPEECH_AGENT_HPP_

#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Qt
#include <QObject>

// SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
// #include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/async.h"

// DSR
#include "dsr/api/dsr_api.h"
#include "dsr/gui/dsr_gui.h"

#include "speechAgent/sound_manager.hpp"
#include "speechAgent/speech_dispatcher.hpp"

class SpeechAgent : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief Construct a new SpeechAgent object
   *
   * @param agent_name The name of the agent.
   * @param agent_id The ID of the agent.
   * @param robot_name The name of the robot.
   */
  SpeechAgent(std::string agent_name, int agent_id, std::string robot_name);

  /**
   * @brief Destroy the SpeechAgent object
   */
  ~SpeechAgent();

  /**
   * @brief Initialize the logger.
   *
   * @param log_filepath The path to the log file.
   */
  void initializeLogger(std::string log_filepath);

  /**
   * @brief Initialize the speech agent.
   *
   * @param sounds_filepath The path to the sounds file.
   * @param volume_factor The volume factor.
   */
  void initializeSpeech(std::string sounds_filepath, int volume_factor);

public slots:
  /**
   * @brief Launch the speech agent.
   */
  void compute();

  // DSR callbacks
  void node_updated(std::uint64_t /*id*/, const std::string & /*type*/) {}
  void node_attributes_updated(uint64_t /*id*/, const std::vector<std::string> & /*att_names*/) {}
  void edge_updated(std::uint64_t from, std::uint64_t to, const std::string & type);
  void edge_attributes_updated(
    std::uint64_t /*from*/, std::uint64_t /*to*/,
    const std::string & /*type*/, const std::vector<std::string> & /*att_names*/) {}
  void node_deleted(std::uint64_t /*id*/) {}
  void edge_deleted(std::uint64_t /*from*/, std::uint64_t /*to*/, const std::string & /*edge_tag*/) {}

private:
  /**
   * @brief Set finished in the DSR.
   *
   * @return true If the event was set.
   */
  bool setFinishedInDSR();

  // DSR graph
  std::shared_ptr<DSR::DSRGraph> G_;
  std::string agent_name_;
  std::string robot_name_;

  // Logger
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
  std::shared_ptr<spdlog::sinks::basic_file_sink_mt> debug_file_sink_;
  std::unique_ptr<spdlog::logger> logger_;

  // Speed related
  SpeechDispatcher speech_;
  SoundManager sound_;
  std::string sounds_filepath_;
  int volume_factor_;

  QTimer timer_;

  // Vector containing all 'say' and 'play' actions for the agent
  std::vector<uint64_t> actions_list_;
  // Action being currently performed
  std::optional<uint64_t> current_action_;
  // Id of the person node that is "interacting" the robot
  std::optional<uint64_t> person_node_id_;
};

#endif  // SPEECHAGENT__SPEECH_AGENT_HPP_
