// Copyright (c) 2024 Grupo Avispa, DTE, Universidad de Málaga
// Copyright (c) 2024 Óscar Pons Fernandez
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
#ifndef DSR_interface_hpp
#define DSR_interface_hpp

// C++
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <sstream>
#include <fstream>

// DSR
#include "dsr/api/dsr_api.h"
#include "dsr/gui/dsr_gui.h"
#include "../../../include/dsr_api_ext.hpp"

// OATPP
#include "oatpp-websocket/WebSocket.hpp"

// UTILS
#include "../../../include/json_messages.hpp"

extern std::shared_ptr<oatpp::websocket::WebSocket> my_socket;
extern std::shared_ptr<oatpp::websocket::WebSocket> button_socket;
extern int conect;
// extern int use_case; //0=wandering, 1=menu
// extern int receive;
extern float accuracy;
extern float tries;
extern nlohmann::json responseJsonBattery;
extern nlohmann::json responseJsonName;
extern nlohmann::json responseJsonSub;
extern nlohmann::json responseJsonInterface;
extern nlohmann::json responseJsonMenuChoices;
extern nlohmann::json responseJsonMenuSelected;
extern nlohmann::json responseJsonQuestion;
extern nlohmann::json responseJsonAnswer;
extern QMutex *mutex;

class DSR_interface: public QObject{
	Q_OBJECT
    public:
	// DSR graph
	std::shared_ptr<DSR::DSRGraph> G;

	//DSR params
	std::string agent_name;
	int agent_id;

	// Action being currently performed
	std::optional<DSR::Node> person_node;
	// Id of the person node that is "interacting" the robot
	bool use_subtitles_;
	std::string robot_name_;

	DSR_interface();
    virtual ~DSR_interface();
    //######################//
	//     DSR FUNCTIONS	//
	//######################//
	void initializeDSR();
	bool configParamsParser(std::string file_path);
	void modify_node_slot(std::uint64_t, const std::string &type){};
	void modify_node_attrs_slot(std::uint64_t id, const std::vector<std::string>& att_names);
	void modify_edge_slot(std::uint64_t from, std::uint64_t to,  const std::string &type);
	void modify_edge_attrs_slot(std::uint64_t from, std::uint64_t to, const std::string &type, const std::vector<std::string>& att_names){};
	void del_edge_slot(std::uint64_t from, std::uint64_t to, const std::string &edge_tag);
	void del_node_slot(std::uint64_t from){};
	private:
};
#endif // DSR_interface_hpp