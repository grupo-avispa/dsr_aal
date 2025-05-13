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

#include <limits>

#include "nlohmann/json.hpp"

#include "adaptationAgent/adaptation_agent.hpp"
#include "../../../include/dsr_api_ext.hpp"
#include "../../../include/json_messages.hpp"

using json = nlohmann::json;

AdaptationAgent::AdaptationAgent(std::string agent_name, int agent_id, std::string robot_name)
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
    G_.get(), &DSR::DSRGraph::update_node_signal, this, &AdaptationAgent::node_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::update_node_attr_signal, this,
    &AdaptationAgent::node_attributes_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::update_edge_signal, this, &AdaptationAgent::edge_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::update_edge_attr_signal, this,
    &AdaptationAgent::edge_attributes_updated);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::del_edge_signal, this, &AdaptationAgent::edge_deleted);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::del_node_signal_by_node, this, &AdaptationAgent::node_deleted);
  QObject::connect(
    G_.get(), &DSR::DSRGraph::create_edge_signal, this, &AdaptationAgent::edge_created);

  // Initialize the use case
  previous_use_case_ = UseCase::DO_NOTHING;
  current_use_case_ = UseCase::DO_NOTHING;
  use_case_finished_ = false;
}

AdaptationAgent::~AdaptationAgent()
{
  G_.reset();
  logger_->info("Destroying AdaptationAgent");
}

void AdaptationAgent::initializeLogger(std::string log_filepath)
{
  // Initialize logger
  std::string log_folder = log_filepath + std::to_string(std::time(nullptr)) + "/" + agent_name_;
  std::string log_folder_expl =
    log_filepath + std::to_string(std::time(nullptr)) + "/explicability";
  std::string debug_log_file = log_folder + "/debug.log";
  std::string explicability_log_file = log_folder_expl + "/explicability.log";
  console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  debug_file_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(debug_log_file, true);
  logger_ = std::make_unique<spdlog::logger>(
    agent_name_, spdlog::sinks_init_list{console_sink_, debug_file_sink_});
  explicability_file_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
    explicability_log_file, true);
  expl_logger_ = std::make_unique<spdlog::logger>(
    "explicability", spdlog::sinks_init_list{console_sink_, explicability_file_sink_});

  debug_file_sink_->set_level(spdlog::level::debug);
  logger_->set_level(spdlog::level::info);
  explicability_file_sink_->set_level(spdlog::level::info);
  expl_logger_->set_level(spdlog::level::info);

  logger_->info("Initialize adaptation agent");
}

void AdaptationAgent::initializeAdaptation(std::string models)
{
  pref_learning_ = std::make_unique<PreferenceLearning>();
  pref_learning_->loadSessions(models);
  enviroment_data_ = {0, 0, 0, 0};
  timer_.start(100);
}

void AdaptationAgent::compute()
{
  std::vector<int> value_use_cases;
  std::vector<UseCase> use_cases;

  // Execute preference learning
  if (!priorityUseCase(selected_use_case_)) {
    if (!plannedGroupUseCase(selected_use_case_)) {
      // Sólo habilitado con wandering, no quita el recargar o el terapia.
      if (!buttonPushedUseCase(selected_use_case_)) {
        // There isn't a person interacting with the robot
        if (interacting_person_.identifier.empty()) {
          // Elegimos persona y caso de uso
          for (size_t i = 0; i < people_with_robot_.size(); i++) {
            use_cases.push_back(selectedUseCaseForUser(i));
            logger_->info("Use case for user {} is : {}", i, toStr(selectedUseCaseForUser(i)));
            value_use_cases.push_back(evaluate(use_cases.back()));
            logger_->info("Evaluate use case: {}", evaluate(use_cases.back()));
          }
          if (use_cases.size() == 0) {
            selected_use_case_ = UseCase::WANDERING;
          } else {
            auto max_iter = std::max_element(value_use_cases.begin(), value_use_cases.end());
            int max_index = std::distance(value_use_cases.begin(), max_iter);
            selected_use_case_ = use_cases.at(max_index);
            current_person_use_case_ = people_with_robot_[max_index];
            // std::cout << "max_index: " << max_index << "selected_use_case_: " << toStr(
            //   selected_use_case_) << "current_person_use_case_: " <<
            //   current_person_use_case_.identifier << std::endl;
          }
          logger_->info("Selected use case: {}", toStr(selected_use_case_));
        }
      } else {
        logger_->info("Button pushed: water / tracking");
      }
    } else {
      logger_->info("Group planned: musical therapy");
    }
  } else {
    logger_->info("Priority: charging");
  }
  logger_->debug("Selected use case: {}", toStr(selected_use_case_));

  // Activamos persona y caso de uso
  if (selected_use_case_ != current_use_case_) {
    if (selected_use_case_ == UseCase::DO_NOTHING) {
      use_case_finished_ = true;
    }

    logger_->info(
      "Changing use case from {} to {}", toStr(current_use_case_), toStr(selected_use_case_));

    abortCurrentUseCaseInDsr();
    // edge callback
    setNewUseCaseInDsr(toStr(selected_use_case_));
    current_use_case_ = selected_use_case_;
    // curr = sele = do noth, use_case_finish=false
  }
}

void AdaptationAgent::abortCurrentUseCaseInDsr()
{
  logger_->info("Aborting current use case: {}", toStr(current_use_case_));
  // Change the 'use_case' node to 'finished' and delete it.
  // Add 'ABORTED' to result_code attribute.
  if (auto use_case_node = G_->get_node("use_case"); use_case_node.has_value()) {
    if (auto robot_node = G_->get_node(robot_name_); robot_node.has_value()) {
      // Add result_code attribute
      std::string arg = "ABORTED: by adaptation agent";
      G_->add_or_modify_attrib_local<result_code_att>(use_case_node.value(), arg);
      G_->update_node(use_case_node.value());
      // Replace the 'is_performing' edge with a 'finished' edge between robot and use_case
      if (DSR::replace_edge<finished_edge_type>(
          G_, robot_node.value().id(), use_case_node.value().id(), "is_performing", robot_name_))
      {
        expl_logger_->info(
          "Aborting current use case: {} by Adaptation", toStr(current_use_case_));
      }
      // And delete the node
      if (!G_->delete_node("use_case")) {
        logger_->error("The node [{}] couldn't be deleted", use_case_node.value().name());
      }
    }
  }

  // Replace all 'wants_to' edges with 'cancel'.
  // Add 'CANCELED' to result_code attribute.
  logger_->info("Replace all 'wants_to' edges with 'cancel");
  std::vector<DSR::Edge> edges = G_->get_edges_by_type("wants_to");
  for (const auto & edge: edges) {
    auto to_node = G_->get_node(edge.to());
    if (to_node.has_value()) {
      if (to_node.value().name() == "tracking" || to_node.value().type() == "bring_water" ||
        to_node.value().type() == "explanation")
      {
        logger_->debug("Dont cancel [{}]", to_node.value().name());
      } else {
        // Replace the 'wants_to' edge with a 'cancel' edge between robot and action
        if (DSR::replace_edge<cancel_edge_type>(G_, edge.from(), edge.to(), "wants_to", robot_name_)) {
          // Add result_code attribute to the node
          std::string result_code = "CANCELED: by adaptation agent";
          G_->add_or_modify_attrib_local<result_code_att>(to_node.value(), result_code);
          G_->update_node(to_node.value());
          expl_logger_->info(
            "CANCEL [{}] action by Adaptation because aborting currente use case {}",
            to_node.value().name(), toStr(previous_use_case_));
          logger_->info("CANCEL [{}] by adaptation agent", to_node.value().name());
        }
      }
    }
  }

  // Replace all 'is_performing' edges with 'abort'.
  // Add 'ABORTED' to result_code attribute.
  logger_->info("Replace all 'is_performing' edges with 'abort'");
  edges = G_->get_edges_by_type("is_performing");
  for (const auto & edge: edges) {
    auto to_node = G_->get_node(edge.to());
    if (to_node.has_value()) {
      if (to_node.value().name() == "tracking" || to_node.value().type() == "update_bbdd") {
        logger_->debug("Dont abort [{}]", to_node.value().name());
      } else {
        // Replace the 'is_performing' edge with a 'abort' edge between robot and action
        if (DSR::replace_edge<abort_edge_type>(
            G_, edge.from(), edge.to(), "is_performing", robot_name_))
        {
          // Add result_code attribute to the node
          std::string result_code = "ABORTED: by adaptation agent";
          G_->add_or_modify_attrib_local<result_code_att>(to_node.value(), result_code);
          G_->update_node(to_node.value());
          expl_logger_->info(
            "ABORT [{}] action by Adaptation because aborting currente use case {}",
            to_node.value().name(), toStr(previous_use_case_));
          logger_->info("ABORT [{}] by adaptation agent", to_node.value().name());
        }
      }
    }
  }
}

bool AdaptationAgent::setNewUseCaseInDsr(const std::string & new_use_case)
{
  // Add a new 'use_case' node with an 'wants_to' edge
  logger_->info("setNewUseCaseInDSR");
  //if (auto new_node = DSR::add_node_with_edge<use_case_node_type, wants_to_edge_type>(
  //    G_, "use_case", robot_name_))
  //{
  auto robot_node = G_->get_node(robot_name_);
  auto new_node = DSR::Node::create<use_case_node_type>("use_case");
  if (auto id = G_->insert_node(new_node); id.has_value()) {
    auto new_edge = DSR::Edge::create<wants_to_edge_type>(robot_node.value().id(), new_node.id());
    G_->insert_or_assign_edge(new_edge);
    // Modify the attributes of the new node
    G_->add_or_modify_attrib_local<use_case_id_att>(new_node, new_use_case);
    G_->update_node(new_node);
    previous_use_case_ = current_use_case_;
    expl_logger_->info("Starting a new use case: {}", new_use_case);
    logger_->info("New use case is {}", new_use_case);
    if (new_use_case == "water") {
      auto water_node = G_->get_node("bring_water");
      DSR::replace_edge<is_performing_edge_type>(
        G_, robot_node.value().id(), water_node.value().id(), "wants_to", robot_name_);
    } else if (new_use_case == "explanation") {
      auto explanation_node = G_->get_node("explanation");
      DSR::replace_edge<is_performing_edge_type>(
        G_, robot_node.value().id(), explanation_node.value().id(), "wants_to", robot_name_);
    }
  }
  return true;
}

// DSR callbacks
// ----------------------------------------------------------------------------

void AdaptationAgent::node_attributes_updated(
  std::uint64_t id, const std::vector<std::string> & att_names)
{
  auto node = G_->get_node(id);

  // Check if the robot activities have changed
  if (node.has_value() && node.value().name() == robot_name_) {
    // Find the activities attribute
    auto it = std::find_if(
      att_names.begin(), att_names.end(),
      [](const auto & att_name) {
        return att_name == "activities";
      });
    // If the activities attribute has changed,
    if (it != att_names.end()) {
      auto robot_activities = G_->get_attrib_by_name<activities_att>(node.value());
      if (robot_activities.has_value()) {
        robot_agenda_ = robot_activities.value();
        expl_logger_->info("Activities for today are: {}", robot_activities.value());
        logger_->info("Robot Activities changed: {}", robot_activities.value());
      }
    }
  }

  // Check if a person node changed
  if (node.has_value() && node.value().type() == "person") {
    auto person_name = G_->get_attrib_by_name<identifier_att>(node.value());
    auto person_comm = G_->get_attrib_by_name<comm_parameters_att>(node.value());
    auto person_profile = G_->get_attrib_by_name<skills_parameters_att>(node.value());
    auto person_activities = G_->get_attrib_by_name<activities_att>(node.value());
    auto person_menu = G_->get_attrib_by_name<menu1_att>(node.value());
    auto person_neuron = G_->get_attrib_by_name<neuron_att>(node.value());
    auto person_reminder = G_->get_attrib_by_name<reminder_att>(node.value());
    // Check if the person is identified
    if (person_name.has_value()) {
      // If the person is interacting with the robot
      if (person_name.value() == interacting_person_.identifier) {
        if (person_comm.has_value()) {interacting_person_.commParameters = person_comm.value();}
        if (person_profile.has_value()) {interacting_person_.profile = person_profile.value();}
        if (person_activities.has_value()) {
          interacting_person_.activities = person_activities.value();
        }
        if (person_menu.has_value()) {interacting_person_.menu = person_menu.value();}
        if (person_neuron.has_value()) {interacting_person_.neuron = person_neuron.value();}
        if (person_reminder.has_value()) {interacting_person_.reminder = person_reminder.value();}
       // expl_logger_->info("The attributes of {} have changed", interacting_person_.identifier);
      }
      // or the robot is just with the person
      else {
        // Check if the person is already in the list
        auto it = std::find_if(
          people_with_robot_.begin(), people_with_robot_.end(),
          [person_name](const auto & person) {
            return person.identifier == person_name.value();
          });
        // Update the person in the list or add it
        if (it != people_with_robot_.end()) {
          if (person_comm.has_value()) {it->commParameters = person_comm.value();}
          if (person_profile.has_value()) {it->profile = person_profile.value();}
          if (person_activities.has_value()) {it->activities = person_activities.value();}
          if (person_menu.has_value()) {it->menu = person_menu.value();}
          if (person_neuron.has_value()) {it->neuron = person_neuron.value();}
          if (person_reminder.has_value()) {it->reminder = person_reminder.value();}
        } else {
          personData current_person;
          if (person_name.has_value()) {current_person.identifier = person_name.value();}
          if (person_comm.has_value()) {current_person.commParameters = person_comm.value();}
          if (person_profile.has_value()) {current_person.profile = person_profile.value();}
          if (person_activities.has_value()) {
            current_person.activities = person_activities.value();
          }
          if (person_menu.has_value()) {current_person.menu = person_menu.value();}
          if (person_neuron.has_value()) {current_person.neuron = person_neuron.value();}
          if (person_reminder.has_value()) {current_person.reminder = person_reminder.value();}
          people_with_robot_.push_back(current_person);
        }
      }
    }
  }
}

void AdaptationAgent::edge_updated(
  std::uint64_t from, std::uint64_t to, const std::string & type)
{
  // Check if the robot is performing a new use case: robot ---(is_performing)---> use_case
  if (type == "is_performing") {
    auto robot_node = G_->get_node(from);
    auto use_case_node = G_->get_node(to);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      use_case_node.has_value() && use_case_node.value().name() == "use_case")
    {
      // Get the use case name
      auto use_case_name = G_->get_attrib_by_name<use_case_id_att>(use_case_node.value());
      if (use_case_name.has_value()) {
        expl_logger_->info("Performing the use case: {}", use_case_name.value());
      }
    }
  }
  // Check if the robot has finished a use case: robot ---(finished)---> use_case
  else if (type == "finished") {
    auto robot_node = G_->get_node(from);
    auto use_case_node = G_->get_node(to);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      use_case_node.has_value() && use_case_node.value().name() == "use_case")
    {
      auto use_case_id = G_->get_attrib_by_name<use_case_id_att>(use_case_node.value());
      auto result_code = G_->get_attrib_by_name<result_code_att>(use_case_node.value());
      if (use_case_id.has_value() && result_code.has_value()) {
        expl_logger_->info(
          "Finished the use case {} with the result: {}", use_case_id.value(), result_code.value());
      }
      logger_->info("Finished detected for use case: {}", toStr(current_use_case_));
      use_case_finished_ = true;
      if (current_use_case_ != UseCase::DO_NOTHING) {
        selected_use_case_ = UseCase::DO_NOTHING;
      }
      auto water_node = G_->get_node("bring_water");
      if (water_node.has_value()) {
        // Replace the 'is_performing' edge with a 'abort' edge between robot and action
        if (DSR::replace_edge<finished_edge_type>(
            G_, robot_node.value().id(), water_node.value().id(), "is_performing", robot_name_))
        {
          // Add result_code attribute to the node
          // And delete the node
          if (!G_->delete_node("bring_water")) {
            std::cout << "The node [";
            std::cout << water_node.value().name();
            std::cout << "] couldn't be deleted" << std::endl;
          }
        }
      }
      auto explanation_node = G_->get_node("explanation");
      if (explanation_node.has_value()) {
        // Replace the 'is_performing' edge with a 'abort' edge between robot and action
        if (DSR::replace_edge<finished_edge_type>(
            G_, robot_node.value().id(), explanation_node.value().id(), "is_performing", robot_name_))
        {
          // Add result_code attribute to the node
          // And delete the node
          if (!G_->delete_node("explanation")) {
            std::cout << "The node [";
            std::cout << explanation_node.value().name();
            std::cout << "] couldn't be deleted" << std::endl;
          }
        }
      }
      auto tracking_node = G_->get_node("tracking");
      if (tracking_node.has_value()) {
        // Replace the 'is_performing' edge with a 'abort' edge between robot and action
        if (DSR::replace_edge<finished_edge_type>(
            G_, robot_node.value().id(), tracking_node.value().id(), "is_performing", robot_name_))
        {
          // Add result_code attribute to the node
        }
        // And delete the node
        if (!G_->delete_node("tracking")) {
          std::cout << "The node [";
          std::cout << tracking_node.value().name();
          std::cout << "] couldn't be deleted" << std::endl;
        }
      }
    }
  }
  // Check if the robot is interacing with a person: robot ---(interacting)---> person
  else if (type == "interacting") {
    auto robot_node = G_->get_node(from);
    auto person_node = G_->get_node(to);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      person_node.has_value() && person_node.value().type() == "person")
    {
      // Get the attributes of the person node
      auto person_name = G_->get_attrib_by_name<identifier_att>(person_node.value());
      auto person_comm = G_->get_attrib_by_name<comm_parameters_att>(person_node.value());
      auto person_profile = G_->get_attrib_by_name<skills_parameters_att>(person_node.value());
      auto person_activities = G_->get_attrib_by_name<activities_att>(person_node.value());
      auto person_menu = G_->get_attrib_by_name<menu1_att>(person_node.value());
      auto person_neuron = G_->get_attrib_by_name<neuron_att>(person_node.value());
      // Updates person interacting attribute
      if (person_name.has_value()) {interacting_person_.identifier = person_name.value();}
      if (person_comm.has_value()) {interacting_person_.commParameters = person_comm.value();}
      if (person_profile.has_value()) {interacting_person_.profile = person_profile.value();}
      if (person_activities.has_value()) {
        interacting_person_.activities = person_activities.value();
      }
      if (person_menu.has_value()) {interacting_person_.menu = person_menu.value();}
      if (person_neuron.has_value()) {interacting_person_.neuron = person_neuron.value();}
      expl_logger_->info("The person {} is interacting with me", person_name.value());
    }
  }
  // Check if the person is around the robot:  person ---(is_with)---> robot
  else if (type == "is_with") {
  }
}

void AdaptationAgent::edge_deleted(
  std::uint64_t from, std::uint64_t to, const std::string & edge_tag)
{
  // Check if the person interacting with the robot is gone: person ---(!interacting)---> robot
  if (edge_tag == "interacting") {
    auto robot_node = G_->get_node(from);
    auto person_node = G_->get_node(to);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      person_node.has_value() && person_node.value().type() == "person")
    {
      // NOTA: No se puede diferenciar si un nodo persona ha dejado de interactuar con el robot
      // o si los datos son erroneos. En ambos casos, se envía un interactor vacío a adaptationComp
      std::cout << "Robot Interacting with person finished" << std::endl;
      expl_logger_->info(
        "The person {} finish interacting with robot", interacting_person_.identifier);
      interacting_person_ = personData();
    }
  }
  // Check if the person with the robot is gone: robot ---(!is_with)---> person
  else if ("is_with") {
    auto robot_node = G_->get_node(to);
    auto person_node = G_->get_node(from);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      person_node.has_value() && person_node.value().type() == "person")
    {
      // Get the attributes of the person node
      auto person_name = G_->get_attrib_by_name<identifier_att>(person_node.value());
      // Remove the person from the list
      std::cout << "Tamaño inicio = " << people_with_robot_.size() << std::endl;
      people_with_robot_.erase(
        std::remove_if(
          people_with_robot_.begin(), people_with_robot_.end(),
          [person_name](const auto & person) {
            return person.identifier == person_name.value();
          }), people_with_robot_.end());
      logger_->info("Person node {} deleted", person_name.value());
    }
  }
}

void AdaptationAgent::node_deleted(const DSR::Node & node)
{
  if (node.type() == "person") {
    // Get the attributes of the person node
    auto person_name = G_->get_attrib_by_name<identifier_att>(node);
    // Remove the person from the list
    people_with_robot_.erase(
      std::remove_if(
        people_with_robot_.begin(), people_with_robot_.end(),
        [person_name](const auto & person) {
          return person.identifier == person_name.value();
        }), people_with_robot_.end());
    logger_->info("Person node {} deleted", person_name.value());
  }
}

void AdaptationAgent::edge_created(
  std::uint64_t from, std::uint64_t to, const std::string & type)
{
  // Check if the robot is interacing with a person: robot ---(interacting)---> person
  if (type == "interacting") {
    auto robot_node = G_->get_node(from);
    auto person_node = G_->get_node(to);
    if (robot_node.has_value() && robot_node.value().name() == robot_name_ &&
      person_node.has_value() && person_node.value().type() == "person")
    {
      // Get the attributes of the person node
      auto person_name = G_->get_attrib_by_name<identifier_att>(person_node.value());
      auto person_comm = G_->get_attrib_by_name<comm_parameters_att>(person_node.value());
      auto person_profile = G_->get_attrib_by_name<skills_parameters_att>(person_node.value());
      auto person_activities = G_->get_attrib_by_name<activities_att>(person_node.value());
      auto person_menu = G_->get_attrib_by_name<menu1_att>(person_node.value());
      auto person_reminder = G_->get_attrib_by_name<reminder_att>(person_node.value());
      // Updates person interacting attribute
      if (person_name.has_value()) {interacting_person_.identifier = person_name.value();}
      if (person_comm.has_value()) {interacting_person_.commParameters = person_comm.value();}
      if (person_profile.has_value()) {interacting_person_.profile = person_profile.value();}
      if (person_activities.has_value()) {
        interacting_person_.activities = person_activities.value();
      }
      if (person_menu.has_value()) {interacting_person_.menu = person_menu.value();}
      if (person_reminder.has_value()) {interacting_person_.reminder = person_reminder.value();}
    }
  }
  // Check if the person is around the robot:  person ---(is_with)---> robot
  else if (type == "is_with") {
    auto person_node = G_->get_node(from);
    auto robot_node = G_->get_node(to);
    if (person_node.has_value() && person_node.value().type() == "person" &&
      robot_node.has_value() && robot_node.value().name() == robot_name_)
    {
      // Get the attributes of the person node
      auto person_name = G_->get_attrib_by_name<identifier_att>(person_node.value());
      auto person_comm = G_->get_attrib_by_name<comm_parameters_att>(person_node.value());
      auto person_profile = G_->get_attrib_by_name<skills_parameters_att>(person_node.value());
      auto person_activities = G_->get_attrib_by_name<activities_att>(person_node.value());
      auto person_menu = G_->get_attrib_by_name<menu1_att>(person_node.value());
      auto person_neuron = G_->get_attrib_by_name<neuron_att>(person_node.value());
      // Updates person with robot attribute
      personData current_person;
      if (person_name.has_value()) {current_person.identifier = person_name.value();}
      if (person_comm.has_value()) {current_person.commParameters = person_comm.value();}
      if (person_profile.has_value()) {current_person.profile = person_profile.value();}
      if (person_activities.has_value()) {current_person.activities = person_activities.value();}
      if (person_menu.has_value()) {current_person.menu = person_menu.value();}
      if (person_neuron.has_value()) {current_person.neuron = person_neuron.value();}
      // Check if the person is already in the list
      auto it = std::find_if(
        people_with_robot_.begin(), people_with_robot_.end(),
        [current_person](const auto & person) {
          return person.identifier == current_person.identifier;
        });
      // Add the person to the list if it is not already there
      logger_->info(
        "Person created with {} and menu: {}", current_person.identifier, current_person.menu);
      if (it == people_with_robot_.end()) {
        people_with_robot_.push_back(current_person);
      }
    }
  }
}

// Preference Learning methods
// ----------------------------------------------------------------------------

bool AdaptationAgent::priorityUseCase(UseCase & use_case)
{
  bool success = false;
  float battery = 50.0;

  if (auto battery_node = G_->get_node("battery"); battery_node.has_value()) {
    auto battery_level = G_->get_attrib_by_name<battery_percentage_att>(battery_node.value());
    battery = battery_level.value();
  } else {
    //logger_->error("Battery node not found. Using default value: 50.0");
  }

  if (battery < 10.0) {
    use_case = UseCase::CHARGING;
    success = true;
  }

  return success;
}

bool AdaptationAgent::plannedGroupUseCase(UseCase & use_case)
{
  bool terapia_musical = findActivityInAgenda("Terapia Musical", 0);
  if (terapia_musical) {
    use_case = UseCase::MUSIC;
    logger_->info("Terapia musical activada");
  }
  return terapia_musical;
}

bool AdaptationAgent::buttonPushedUseCase(UseCase & use_case)
{
  bool success = false;
  if (auto water_node = G_->get_node("bring_water"); water_node.has_value()) {
    use_case = UseCase::GETME;
    success = true;
  } else if (auto tracking_node = G_->get_node("tracking"); tracking_node.has_value()) {
    use_case = UseCase::ANNOUNCER;
    success = true;
  } else if (auto explanation_node = G_->get_node("explanation"); explanation_node.has_value()) {
    use_case = UseCase::EXPLANATION;
    success = true;
  }
  return success;
}

UseCase AdaptationAgent::selectedUseCaseForUser(int user)
{
  std::vector<int64_t> enviroment_data_aux = updateInputDataUser(user);
  auto priorities = pref_learning_->getPriorities(enviroment_data_aux);
  if (priorities.front() == UseCase::GETME) {             //CUTRE
    priorities.front() = UseCase::WANDERING;
  }
  // We do the +1 because the preference learning doesn't have the DO_NOTHING use case
  return UseCase(priorities.front());
}

int AdaptationAgent::evaluate(UseCase use_case)
{
  int result = 0;
  if (use_case == UseCase::REMINDER) {
    result = 3;
  } else if (use_case == UseCase::NEURON_UP) {
    result = 2;
  } else if (use_case == UseCase::MENU) {
    result = 1;
  }
  return result;
}

bool AdaptationAgent::findActivityInAgenda(std::string activity_name, int pretime)
{
  bool activity = false;
  auto activity_list = getActivityfromJstring(robot_agenda_);
  std::string hour = currentTime(pretime);

  // Find the activity in the list
  auto it = std::find_if(
    activity_list.begin(), activity_list.end(), [&](const auto & act) {
      return act.nombre == activity_name;
    });
  // If the activities attribute has changed,
  if (it != activity_list.end()) {
    int minutosActual = StringTimeToMinutes(hour);
    int minutosInicioActividad = StringTimeToMinutes(it->hora_inicio);
    int minutosFinalActividad = StringTimeToMinutes(it->hora_fin);
    activity = (minutosActual >= minutosInicioActividad) && (minutosActual < minutosFinalActividad);
    logger_->info("Activity {} found from {} to {}", it->nombre, it->hora_inicio, it->hora_fin);
  }
  return activity;
}

bool AdaptationAgent::isPersonBusy(personData person, int pretime)
{
  bool activity = false;
  auto activity_list = getActivityfromJstring(person.activities);
  std::string hour = currentTime(pretime);

  logger_->info("Checking if a person {} is busy at {}", person.identifier, currentTime(pretime));

  for (const auto & act : activity_list) {
    int minutosActual = StringTimeToMinutes(hour);
    int minutosInicioActividad = StringTimeToMinutes(act.hora_inicio);
    int minutosFinalActividad = StringTimeToMinutes(act.hora_fin);
    activity = (minutosActual >= minutosInicioActividad) && (minutosActual < minutosFinalActividad);
    if (activity) {
      logger_->info("Activity {} found from {} to {}", act.nombre, act.hora_inicio, act.hora_fin);
      break;
    }
  }

  return activity;
}

std::vector<int64_t> AdaptationAgent::updateInputDataUser(int user_id)
{
  std::vector<int64_t> enviroment_d_user = {0, 0, 0, 0};
  if (!people_with_robot_.empty()) {
    logger_->info("Updating input data for user: {}", user_id);

    // Menu
    enviroment_d_user[0] = people_with_robot_[user_id].menu.empty() ? 0 : 1;
    // Interaction
    enviroment_d_user[1] = 1;
    // Busy
    enviroment_d_user[2] =
      (isPersonBusy(
        people_with_robot_[user_id],
        15) && people_with_robot_[user_id].reminder ) ? 1 : 0;
    // Cognitive
    enviroment_d_user[3] =
      (findActivityInAgenda("Terapia Cognitiva", 0) &&
      (people_with_robot_[user_id].neuron)) ? 1 : 0;
  }
  return enviroment_d_user;
}

// Helpers
// ----------------------------------------------------------------------------

int AdaptationAgent::StringTimeToMinutes(std::string hour)
{
  return (std::stoi(hour.substr(0, 2)) * 60) + std::stoi(hour.substr(3, 5));
}

UseCase AdaptationAgent::fromStr(std::string use_case_str)
{
  if (use_case_str == "wandering") {
    return UseCase::WANDERING;
  } else if (use_case_str == "charging") {
    return UseCase::CHARGING;
  } else if (use_case_str == "menu") {
    return UseCase::MENU;
  } else if (use_case_str == "music") {
    return UseCase::MUSIC;
  } else if (use_case_str == "neuron") {
    return UseCase::NEURON_UP;
  } else if (use_case_str == "water") {
    return UseCase::GETME;
  } else if (use_case_str == "reminder") {
    return UseCase::REMINDER;
  } else if (use_case_str == "sign") {
    return UseCase::ANNOUNCER;
  } else if (use_case_str == "explanation") {
    return UseCase::EXPLANATION;
  } else {
    return UseCase::DO_NOTHING;
  }
}

std::string AdaptationAgent::toStr(UseCase use_case)
{
  if (use_case == UseCase::WANDERING) {
    return "wandering";
  } else if (use_case == UseCase::CHARGING) {
    return "charging";
  } else if (use_case == UseCase::MENU) {
    return "menu";
  } else if (use_case == UseCase::MUSIC) {
    return "music";
  } else if (use_case == UseCase::NEURON_UP) {
    return "neuron";
  } else if (use_case == UseCase::GETME) {
    return "water";
  } else if (use_case == UseCase::REMINDER) {
    return "reminder";
  } else if (use_case == UseCase::ANNOUNCER) {
    return "tracking";
  } else if (use_case == UseCase::EXPLANATION) {
    return "explanation";
  } else {
    return "do_nothing";
  }
}

std::string AdaptationAgent::currentTime(int premin)
{
  time_t current_time;
  struct tm * now_tm;
  time(&current_time);
  current_time += 60 * premin;
  now_tm = localtime(&current_time);

  std::stringstream ss;
  ss <<
    (now_tm->tm_hour <
  10 ? "0" : "") << now_tm->tm_hour << ":" << (now_tm->tm_min < 10 ? "0" : "") << now_tm->tm_min;

  return ss.str();
}

bool AdaptationAgent::notWaitUseCase(UseCase prior)
{
  return (prior == UseCase::WANDERING) || (prior == UseCase::DO_NOTHING) ||
         (prior == UseCase::CHARGING);
}
