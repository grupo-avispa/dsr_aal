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


#include "speechAgent/speech_agent.hpp"
#include "../../../include/dsr_api_ext.hpp"

SpeechAgent::SpeechAgent(std::string agent_name, int agent_id, std::string robot_name)
: agent_name_(agent_name), robot_name_(robot_name)
{
  // Compute
  QObject::connect(&timer_, SIGNAL(timeout()), this, SLOT(compute()));

  // Register types
  qRegisterMetaType<DSR::Node>("Node");
  qRegisterMetaType<DSR::Edge>("Edge");
  qRegisterMetaType<uint64_t>("uint64_t");
  qRegisterMetaType<std::string>("std::string");
  qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
  qRegisterMetaType<DSR::SignalInfo>("DSR::SignalInfo");

  // Create the DSR graph
  G_ = std::make_shared<DSR::DSRGraph>(agent_name, agent_id, "");

  // Add connection signals
  QObject::connect(
    G_.get(), &DSR::DSRGraph::update_node_signal, this, &SpeechAgent::node_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::update_node_attr_signal, this, &SpeechAgent::node_attributes_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::update_edge_signal, this, &SpeechAgent::edge_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::update_edge_attr_signal, this, &SpeechAgent::edge_attributes_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::del_node_signal, this, &SpeechAgent::node_deleted);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::del_edge_signal, this, &SpeechAgent::edge_deleted);
}

SpeechAgent::~SpeechAgent()
{
  G_.reset();
  logger_->info("Destroying SpeechAgent");
}

void SpeechAgent::initializeLogger(std::string log_filepath)
{
  // Initialize logger
  std::string log_folder = log_filepath + std::to_string(std::time(nullptr)) + "/" + agent_name_;
  std::string debug_log_file = log_folder + "/debug.log";
  console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  debug_file_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(debug_log_file, true);
  logger_ = std::make_unique<spdlog::logger>(
    agent_name_, spdlog::sinks_init_list{console_sink_, debug_file_sink_});
  debug_file_sink_->set_level(spdlog::level::debug);
  logger_->set_level(spdlog::level::debug);

  logger_->info("Initialize speech agent");
}

void SpeechAgent::initializeSpeech(std::string sounds_filepath, int volume_factor)
{
  sounds_filepath_ = sounds_filepath;
  volume_factor_ = volume_factor;
  timer_.start(100);
}

void SpeechAgent::compute()
{
  // Play the first action in the list if there is no action being performed
  if (!actions_list_.empty() && !current_action_.has_value()) {
    // Assign the first action of the list to be performed
    current_action_ = actions_list_.front();
    // Delete the action from the list
    actions_list_.erase(actions_list_.begin());
    // Get the name of the action
    auto action_node = G_->get_node(current_action_.value());
    auto act_name = G_->get_name_from_id(current_action_.value());
    // Replace the 'wants_to' edge with a 'is_performing' edge between robot and action
    if (DSR::replace_edge<is_performing_edge_type>(
        G_, robot_name_, act_name.value(), "wants_to", robot_name_))
    {
      // Set the volume
      auto volume = G_->get_attrib_by_name<volume_att>(action_node.value());
      float volume_to_change = volume.has_value() ? volume.value() : 50.0;
      logger_->info("Setting the volume to {}", volume_to_change);
      sound_.setMasterVolume(volume_to_change);
      speech_.configSpeechVolume(volume_to_change);

      // Perform the action
      if (act_name.value() == "say") {
        auto text = G_->get_attrib_by_name<text_att>(action_node.value());
        if (text.has_value()) {
          speech_.sayWithPicoAndWait(text.value());
          setFinishedInDSR();
        } else {
          logger_->error("Error trying to say when the action is say");
        }
      } else if (act_name.value() == "play") {
        auto soundfile = G_->get_attrib_by_name<sound_att>(action_node.value());
        if (soundfile.has_value()) {
          sound_.playFileAndWait(sounds_filepath_ + soundfile.value() + ".wav", volume_factor_);
          setFinishedInDSR();
        } else {
          logger_->error("Error trying to play when the action is play");
        }
      } else {
        logger_->error("Error trying to say or play when the action is something different");
      }
    }
  }
}

bool SpeechAgent::setFinishedInDSR()
{
  bool success = false;
  // Check if the robot is currently performing an action
  if (current_action_.has_value()) {
    auto action = G_->get_name_from_id(current_action_.value());
    // Replace the 'is_performing' edge with a 'finished' edge between robot and current_action
    if (DSR::replace_edge<finished_edge_type>(
        G_, robot_name_, action.value(), "is_performing", robot_name_))
    {
      current_action_.reset();
      success = true;
      logger_->info("Finished action {}", action.value());
    }
  }
  return success;
}

void SpeechAgent::edge_updated(std::uint64_t from, std::uint64_t to, const std::string & type)
{
  // Check if the robot wants to abort or cancel the speech: robot ---(abort)--> say/play
  if (type == "abort" || type == "cancel") {
    auto robot_node = G_->get_node(from);
    auto action_node = G_->get_node(to);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      action_node.has_value() &&
      (action_node.value().name() == "play" || action_node.value().name() == "say") )
    {
      // Remove the node from the list
      actions_list_.erase(
        std::remove(actions_list_.begin(), actions_list_.end(), to), actions_list_.end());
      // Reset current_action
      current_action_.reset();
      // Delete node say/play
      if (G_->delete_node(action_node.value().id())) {
        logger_->info("Delete node {}", action_node.value().name());
      }
      // Stop the components
      sound_.stop();
      speech_.stopMessage();
    }
  }
  // Check if the robot wants to start the speech: robot ---(wants_to)--> say/play
  else if (type == "wants_to") {
    auto robot_node = G_->get_node(from);
    auto action_node = G_->get_node(to);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      action_node.has_value() &&
      (action_node.value().name() == "play" || action_node.value().name() == "say") )
    {
      // Add node to the list if it is not in the list
      if (std::find(actions_list_.begin(), actions_list_.end(), to) == actions_list_.end()) {
        actions_list_.push_back(to);
        logger_->info("New 'wants_to' edge to {} node detected", action_node.value().name());
      }
    } else if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      action_node.has_value() && (action_node.value().name() == "set_volume"))
    {
      if (DSR::replace_edge<is_performing_edge_type>(
          G_, robot_name_, action_node.value().name(), "wants_to", robot_name_))
      {
        auto volume = G_->get_attrib_by_name<volume_att>(action_node.value());
        float volume_to_change = volume.has_value() ? volume.value() : 50.0;
        // Set the volume
        logger_->info("Setting the volume");
        sound_.setMasterVolume(volume_to_change);
        speech_.configSpeechVolume(volume_to_change);

        if (DSR::replace_edge<finished_edge_type>(
            G_, robot_name_, action_node.value().name(), "is_performing", robot_name_))
        {
          logger_->info("Finished setting the volume");
        }
        if (G_->delete_node(action_node.value())) {
          logger_->info("Delete node set_volume");
        }
      }
    }
  }
}
