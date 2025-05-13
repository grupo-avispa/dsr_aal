// Copyright (c) 2024 Grupo Avispa, DTE, Universidad de Málaga
// Copyright (c) 2024 Joaquín Ballesteros Gómez
// Copyright (c) 2024 Alberto J. Tudela Roldán
// Copyright (c) 2024 José Galeas Merchán
// Copyright (c) 2024 Juan Pedro Bandera Rubio
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

#ifndef ADAPTATIONAGENT__ADAPTATION_AGENT_HPP_
#define ADAPTATIONAGENT__ADAPTATION_AGENT_HPP_

#include <mutex>
#include <string>

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

#include "adaptationAgent/types.hpp"
#include "adaptationAgent/preference_learning.hpp"


class AdaptationAgent : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief Construct a new AdaptationAgent object
   *
   * @param agent_name The name of the agent.
   * @param agent_id The ID of the agent.
   * @param robot_name The name of the robot.
   */
  AdaptationAgent(std::string agent_name, int agent_id, std::string robot_name);

  /**
   * @brief Destroy the AdaptationAgent object
   */
  ~AdaptationAgent();

  /**
   * @brief Initialize the logger.
   *
   * @param log_filepath The path to the log file.
   */
  void initializeLogger(std::string log_filepath);

  /**
   * @brief Initialize the preference learning.
   *
   * @param models The path to the models.
   */
  void initializeAdaptation(std::string models);

public slots:
  /**
   * @brief Launch the adaptation agent.
   */
  void compute();

  // DSR callbacks
  void node_updated(std::uint64_t /*id*/, const std::string & /*type*/) {}
  void node_attributes_updated(uint64_t id, const std::vector<std::string> & att_names);
  void edge_updated(std::uint64_t from, std::uint64_t to, const std::string & type);
  void edge_attributes_updated(
    std::uint64_t /*from*/, std::uint64_t /*to*/,
    const std::string & /*type*/, const std::vector<std::string> & /*att_names*/) {}
  void node_deleted(const DSR::Node & node);
  void edge_deleted(std::uint64_t from, std::uint64_t to, const std::string & edge_tag);
  void edge_created(std::uint64_t from, std::uint64_t to, const std::string & type);

private:
  /**
   * @brief Abort the current use case in the DSR.
   * - Change all 'wants_to' edges connecting action nodes to 'cancel'
   * - Change all 'is_performing' edges connecting action nodes to 'abort'
   */
  void abortCurrentUseCaseInDsr();

  /**
   * @brief Set the new use case in the DSR.
   *
   * @param new_use_case The new use case.
   * @return bool If the new use case was set.
   */
  bool setNewUseCaseInDsr(const std::string & new_use_case);

  /**
   * @brief Set the new use case with priority.
   *
   * @param use_case The new use case.
   * @return bool If the new use case was set.
   */
  bool priorityUseCase(UseCase & use_case);

  /**
   * @brief Set the new use case for a group.
   *
   * @param use_case The new use case.
   * @return bool If the new use case was set.
   */
  bool plannedGroupUseCase(UseCase & use_case);

  /**
   * @brief Set the new use case if a button is pushed.
   *
   * @param use_case The new use case.
   * @return bool If the new use case was set.
   */
  bool buttonPushedUseCase(UseCase & use_case);

  /**
   * @brief Set the new use case if a person is detected.
   *
   * @param use_case The new use case.
   * @return bool If the new use case was set.
   */
  UseCase selectedUseCaseForUser(int user);

  /**
   * @brief Evaluate the use case.
   *
   * @param use_case The use case.
   * @return int The evaluation of the use case.
   */
  int evaluate(UseCase use_case);

  /**
   * @brief Find the activity in the agenda.
   *
   * @param activity_name The name of the activity.
   * @param pretime The previous time.
   * @return bool If the activity was found.
   */
  bool findActivityInAgenda(std::string activity_name, int pretime);

  /**
   * @brief Check if a person is busy.
   *
   * @param person The person data.
   * @param pretime The previous time.
   * @return bool If the person is busy.
   */
  bool isPersonBusy(personData person, int pretime = 10);

  /**
   * @brief Update the input data for the user.
   *
   * @param user_id The ID of the user.
   * @return std::vector<int64_t> The updated input data.
   */
  std::vector<int64_t> updateInputDataUser(int user_id);

  // Helpers
  int StringTimeToMinutes(std::string hour);
  UseCase fromStr(std::string use_case_str);
  std::string toStr(UseCase use_case);
  std::string currentTime(int premin);
  bool notWaitUseCase(UseCase prior);

  // DSR graph
  std::shared_ptr<DSR::DSRGraph> G_;
  std::string agent_name_;
  std::string robot_name_;

  // Log related variables
  std::string log_filepath_;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
  std::shared_ptr<spdlog::sinks::basic_file_sink_mt> debug_file_sink_;
  std::unique_ptr<spdlog::logger> logger_;
  std::shared_ptr<spdlog::sinks::basic_file_sink_mt> explicability_file_sink_;
  std::unique_ptr<spdlog::logger> expl_logger_;

  QTimer timer_;

  // Use case flow control variables
  UseCase selected_use_case_;
  UseCase previous_use_case_;
  UseCase current_use_case_;
  bool use_case_finished_;

  // Preference learning variables
  const std::map<std::string, int> enviroment_input_ = {
    {"battery", 0},
    {"menu", 1},
    {"interaction", 2},
    {"busy", 3},
    {"neuron_up", 4},
    {"music", 5}
  };
  std::vector<int64_t> enviroment_data_;
  std::unique_ptr<PreferenceLearning> pref_learning_;

  // Current interacting person
  personData interacting_person_;
  // Current person using the robot
  personData current_person_use_case_;
  // Current people with the robot
  std::vector<personData> people_with_robot_;
  // Current robot agenda
  std::string robot_agenda_;
};

#endif  // ADAPTATIONAGENT__ADAPTATION_AGENT_HPP_
