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

#include "../include/DSR_interface.hpp"

std::shared_ptr<oatpp::websocket::WebSocket> my_socket;
std::shared_ptr<oatpp::websocket::WebSocket> button_socket;
int conect=0;
// int use_case = 0; //0=wandering, 1=menu
// int receive = 0;
float accuracy = 0.0;
float tries = 0.0;
nlohmann::json responseJsonBattery{};
nlohmann::json responseJsonName{};
nlohmann::json responseJsonSub{};
nlohmann::json responseJsonInterface{};
nlohmann::json responseJsonMenuChoices{};
nlohmann::json responseJsonMenuSelected{};
nlohmann::json responseJsonQuestion{};
nlohmann::json responseJsonAnswer{};
QMutex *mutex;

DSR_interface::DSR_interface() : QObject() {
	qRegisterMetaType<DSR::Node>("Node");
	qRegisterMetaType<DSR::Edge>("Edge");
	qRegisterMetaType<uint64_t>("uint64_t");
	qRegisterMetaType<std::string>("std::string");
	qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
	qRegisterMetaType<DSR::SignalInfo>("DSR::SignalInfo");
}
DSR_interface::~DSR_interface() = default;

void DSR_interface::initializeDSR(){
        // create graph
		G = std::make_shared<DSR::DSRGraph>(0, agent_name, agent_id, ""); // Init nodes
		std::cout<< __FUNCTION__ << "Graph loaded" << std::endl;  

		//dsr update signals
		QObject::connect(G.get(), &DSR::DSRGraph::update_node_signal, this, &DSR_interface::modify_node_slot);
		QObject::connect(G.get(), &DSR::DSRGraph::update_edge_signal, this, &DSR_interface::modify_edge_slot);
		QObject::connect(G.get(), &DSR::DSRGraph::update_node_attr_signal, this, &DSR_interface::modify_node_attrs_slot);
		QObject::connect(G.get(), &DSR::DSRGraph::update_edge_attr_signal, this, &DSR_interface::modify_edge_attrs_slot);
		QObject::connect(G.get(), &DSR::DSRGraph::del_edge_signal, this, &DSR_interface::del_edge_slot);
		QObject::connect(G.get(), &DSR::DSRGraph::del_node_signal, this, &DSR_interface::del_node_slot);
}

bool DSR_interface::configParamsParser(std::string file_path){
    // Lambda to parse from string to bool
    auto stobool = [] (const std::string &s_condition) {
        return s_condition != "false";
    };
    // Open config file
    std::ifstream configFile(file_path);
    if (!configFile.is_open()) {
        std::cerr << "Couldn't open config file: " << file_path << std::endl;
        return false;
    }
    // Read lines from config file and parse the parameters
    std::string line;
    std::cout << "Configuration parameters:";
    while(std::getline(configFile, line)){
        std::istringstream is_line(line);
        std::string key, value;
        if (std::getline(is_line, key, '=') && std::getline(is_line, value)){
            if(key == "agent_id"){
                agent_id = std::stoi(value);
            }else if(key == "agent_name"){
                agent_name = value;
            }else if(key == "robot_name"){
                robot_name_ = value;
            }else{
                std::cerr << "Error parsing not defined parameter: " << key << std::endl;
                return false;
            }
			std::cout << " " << key << " : " << value;
        }
    }
    std::cout << std::endl << " Finished configuration ...";
    configFile.close();
    return true;
}

void DSR_interface::modify_node_attrs_slot(std::uint64_t id, const std::vector<std::string>& att_names){
	QMutexLocker locker(mutex);
	auto node = G->get_node(id);
	// Node battery has changed
	if (node.has_value() && node.value().name() == "battery"){
		// Find the the attribute that has changed
		for (const auto &att_name : att_names){
			if (auto search = node.value().attrs().find(att_name); search != node.value().attrs().end()){
				// Check if the attribute is battery_percentage and send it to the webServer
				if (search->first == "battery_percentage"){
					float level = std::get<float>(search->second.value());
                    responseJsonBattery["bateria"]=level;
                    std::string mensaje = responseJsonBattery.dump();
                    if(conect){
                        my_socket->sendOneFrameText(mensaje);
                    }
                    std::cout << "Battery level: " << level << "sent to client" << std::endl;
				}
				// Check if the attribute is battery_power_supply_status and send it to the webServer
				if (search->first == "battery_power_supply_status"){
					std::string battery_status = std::get<std::string>(search->second.value());
					static  std::string prev_interface = "default";
					std::cout << "Power supply status: " << battery_status << std::endl;
					std::cout << "Previous interface: " << prev_interface << std::endl;
					std::optional<std::string> interface;
					if (battery_status == "charging"){
						interface = "charging";
					}
					// Send the activation of the interface to the webServer
					if (interface.has_value()){
						prev_interface = interface.value();
                        std::cout << "Interface: " << interface.value() << std::endl;
                        responseJsonInterface["interface"] = interface.value();
                        std::string mensaje = responseJsonInterface.dump();
                        if(conect){
                            my_socket->sendOneFrameText(mensaje);
                        }
					}
				}
			}
		}
	}
	else if (node.has_value() && node.value().name() == "use_case"){
		// Find the the attribute that has changed
		for (const auto &att_name : att_names){
			if (auto search = node.value().attrs().find(att_name); search != node.value().attrs().end()){
				// Check if the attribute is menu_choices and send it to the client
				if (search->first == "menu_choices1"){
					std::string menu_choices1 = std::get<std::string>(search->second.value());
                    std::cout << "Menu choices: " << menu_choices1 << std::endl;
                    MenuChoices parsed_menu_choices1(nlohmann::json::parse(menu_choices1));
					
					auto menu_choices2 = G->get_attrib_by_name<menu_choices2_att>(node.value());
					MenuChoices parsed_menu_choices2(nlohmann::json::parse(menu_choices2.value()));
					auto menu_choices3 = G->get_attrib_by_name<menu_choices3_att>(node.value());
					MenuChoices parsed_menu_choices3(nlohmann::json::parse(menu_choices3.value()));
					auto menu_choices4 = G->get_attrib_by_name<menu_choices4_att>(node.value());
					MenuChoices parsed_menu_choices4(nlohmann::json::parse(menu_choices4.value()));
					auto menu_choices5 = G->get_attrib_by_name<menu_choices5_att>(node.value());
					MenuChoices parsed_menu_choices5(nlohmann::json::parse(menu_choices5.value()));
					auto menu_choices6 = G->get_attrib_by_name<menu_choices6_att>(node.value());
					MenuChoices parsed_menu_choices6(nlohmann::json::parse(menu_choices6.value()));
					auto menu_choices7 = G->get_attrib_by_name<menu_choices7_att>(node.value());
					MenuChoices parsed_menu_choices7(nlohmann::json::parse(menu_choices7.value()));
                    
					std::cout << "Opciones primero: " << parsed_menu_choices1.primero1 << " || " 
                                << parsed_menu_choices1.primero2 << std::endl;
                    responseJsonMenuChoices["Lpri1"] = parsed_menu_choices1.primero1;
                    responseJsonMenuChoices["Lpri2"] = parsed_menu_choices1.primero2;
                    responseJsonMenuChoices["Lseg1"] = parsed_menu_choices1.segundo1;
                    responseJsonMenuChoices["Lseg2"] = parsed_menu_choices1.segundo2;
					responseJsonMenuChoices["Mpri1"] = parsed_menu_choices2.primero1;
                    responseJsonMenuChoices["Mpri2"] = parsed_menu_choices2.primero2;
                    responseJsonMenuChoices["Mseg1"] = parsed_menu_choices2.segundo1;
                    responseJsonMenuChoices["Mseg2"] = parsed_menu_choices2.segundo2;
					responseJsonMenuChoices["Xpri1"] = parsed_menu_choices3.primero1;
                    responseJsonMenuChoices["Xpri2"] = parsed_menu_choices3.primero2;
                    responseJsonMenuChoices["Xseg1"] = parsed_menu_choices3.segundo1;
                    responseJsonMenuChoices["Xseg2"] = parsed_menu_choices3.segundo2;
					responseJsonMenuChoices["Jpri1"] = parsed_menu_choices4.primero1;
                    responseJsonMenuChoices["Jpri2"] = parsed_menu_choices4.primero2;
                    responseJsonMenuChoices["Jseg1"] = parsed_menu_choices4.segundo1;
                    responseJsonMenuChoices["Jseg2"] = parsed_menu_choices4.segundo2;
					responseJsonMenuChoices["Vpri1"] = parsed_menu_choices5.primero1;
                    responseJsonMenuChoices["Vpri2"] = parsed_menu_choices5.primero2;
                    responseJsonMenuChoices["Vseg1"] = parsed_menu_choices5.segundo1;
                    responseJsonMenuChoices["Vseg2"] = parsed_menu_choices5.segundo2;
					responseJsonMenuChoices["Spri1"] = parsed_menu_choices6.primero1;
                    responseJsonMenuChoices["Spri2"] = parsed_menu_choices6.primero2;
                    responseJsonMenuChoices["Sseg1"] = parsed_menu_choices6.segundo1;
                    responseJsonMenuChoices["Sseg2"] = parsed_menu_choices6.segundo2;
					responseJsonMenuChoices["Dpri1"] = parsed_menu_choices7.primero1;
                    responseJsonMenuChoices["Dpri2"] = parsed_menu_choices7.primero2;
                    responseJsonMenuChoices["Dseg1"] = parsed_menu_choices7.segundo1;
                    responseJsonMenuChoices["Dseg2"] = parsed_menu_choices7.segundo2;
                    responseJsonMenuChoices["pos"] = parsed_menu_choices1.postre1;
                    std::string mensaje = responseJsonMenuChoices.dump();
					std::cout << "Pre conect" << std::endl;
                    if(conect){
						std::cout << "Pre Send Menu Choices to interface" << std::endl;
                        my_socket->sendOneFrameText(mensaje);
						std::cout << "Send Menu Choices to interface" << std::endl;
                    }
				}
			}
		}
	}
	else if (node.has_value() && node.value().name() == "explanation"){
		// Find the the attribute that has changed
		for (const auto &att_name : att_names){
			if (auto search = node.value().attrs().find(att_name); search != node.value().attrs().end()){
				// Check if the attribute is battery_percentage and send it to the webServer
				if (search->first == "question"){
					std::string question = std::get<std::string>(search->second.value());
                    responseJsonQuestion["question"]=question;
                    std::string mensaje = responseJsonQuestion.dump();
                    if(conect){
                        my_socket->sendOneFrameText(mensaje);
                    }
                    std::cout << "Question: " << question << "sent to client" << std::endl;
				}
				else if (search->first == "answer"){
					std::string answer = std::get<std::string>(search->second.value());
                    responseJsonAnswer["answer"]=answer;
                    std::string mensaje = responseJsonAnswer.dump();
                    if(conect){
                        my_socket->sendOneFrameText(mensaje);
                    }
                    std::cout << "Answer: " << answer << "sent to client" << std::endl;
				}
			}
		}
	}
}

void DSR_interface::modify_edge_slot(std::uint64_t from, std::uint64_t to,  const std::string &type){
	// Check if the robot is interacting with a person: robot ---(interacting)--> person
	QMutexLocker locker(mutex);
	if (type == "interacting"){
		std::cout << "MODIFY EDGE interacting" << std::endl;
		auto robot_node = G->get_node(from);
		person_node = G->get_node(to);
		if (robot_node.has_value() && robot_node.value().name() == robot_name_
			&& person_node.has_value() && person_node.value().type() == "person"){
			auto person_identifier = G->get_attrib_by_name<identifier_att>(person_node.value());
			std::cout << "The person [" << person_identifier.value() << "] is interacting with the robot" << std::endl;
            responseJsonName["nombre"] = person_identifier.value();
            if(conect){ //Tracking use case
                std::string mensaje = responseJsonName.dump();
                my_socket->sendOneFrameText(mensaje);
            }
			// Updates subtitle flag according to user's profile
			auto comm_parameters = G->get_attrib_by_name<comm_parameters_att>(person_node.value());
			CommParameters comm_param(nlohmann::json::parse(comm_parameters.value()));
			if (comm_param.subtitles == false) {
				use_subtitles_ = false;
			} else {
				use_subtitles_ = true;
			}
			// Update person name to tracking
			auto tracking_node = G->get_node("tracking");
			if(tracking_node.has_value()){
				G->add_or_modify_attrib_local<identifier_att>(tracking_node.value(), person_identifier.value());
				if(G->update_node(tracking_node.value())){
					std::cout << "TRACKING NODE UPDATED" << std::endl;
				}else{
					std::cout << "COULDN'T UPDATE TRACKING NODE" << std::endl;
				}
			}
		}
	}
	// Check if the robot wants to show the screen: robot ---(wants_to)--> show
	else if (type == "wants_to"){
		std::cout << "MODIFY EDGE wants_to" << std::endl;
		auto robot_node = G->get_node(from);
		auto show_node = G->get_node(to);
		if (robot_node.has_value() && robot_node.value().name() == robot_name_
			&& show_node.has_value() && show_node.value().name() == "show"){
			std::cout << "Wants to between robot and show" << std::endl;	
			if (G->delete_edge(robot_node.value().id(), show_node.value().id(), "wants_to")) {
				auto edge = DSR::create_edge_with_priority<is_performing_edge_type>(G, robot_node.value().id(), show_node.value().id(), 0, robot_name_);
				if (G->insert_or_assign_edge(edge)) {
					std::cout << "Insertado edge: is_performing" << std::endl;
					// Get interface to show
					auto interface = G->get_attrib_by_name<interface_att>(show_node.value());
					if (interface.has_value()){
						std::cout << "Selected Interface " << interface.value() << std::endl;
						responseJsonInterface["interface"] = interface.value();
						std::string mensaje = responseJsonInterface.dump();
						if(conect){
							std::cout << "Sendig Interface " << interface.value() << std::endl;
							if(button_socket){
								button_socket->sendOneFrameText(mensaje);
								std::cout << "Interface " << interface.value() << " has been sent to esp32" << std::endl;
							}
							if(my_socket){
								my_socket->sendOneFrameText(mensaje);
								std::cout << "Interface " << interface.value() << " has been sent to web" << std::endl;
							}
						}
						if(interface.value() != "menu1" && interface.value() != "menu2" && interface.value() != "menu3" &&
						interface.value() != "menu4" && interface.value() != "menu5" && interface.value() != "menu6" &&
						interface.value() != "menu7" && interface.value() != "neuron"  && interface.value() != "water"){
							// Finished show
							// Replace edge 'is_performing' to 'finished' between robot and action
							if (G->delete_edge(robot_node.value().id(), show_node.value().id(), "is_performing")) {
								auto edge = DSR::create_edge_with_priority<finished_edge_type>(G, robot_node.value().id(), show_node.value().id(), 0, robot_name_);
								if (G->insert_or_assign_edge(edge)) {
									std::cout << "Finished action " <<  show_node.value().name() << endl;
								}
							}else {
							std::cout << "ERROR: Trying to delete edge type is_performing" << std::endl;
							}
						}
					}
				}
			}
		}
	}
	// Check if the robot cancel to show the screen: robot ---(cancel)--> show
	else if (type == "cancel"){
		std::cout << "MODIFY EDGE cancel" << std::endl;
		auto robot_node = G->get_node(from);
		auto show_node = G->get_node(to);
		if (robot_node.has_value() && robot_node.value().name() == robot_name_
			&& show_node.has_value() && show_node.value().name() == "show"){
			std::cout << "Cancel between robot and show" << std::endl;
			if ( show_node.has_value() ) {	
				// Delete node show
				if (G->delete_node(show_node.value())) {
					std::cout << "Delete node show" << std::endl;
				}
				else{
					std::cout << "Can not delete node show" << std::endl;
				}
			}
			else{
				std::cout << "Can not delete node show because there is no node registered" << std::endl;
			}			
		}
	}
	// Check if the robot abort to show the screen: robot ---(abort)--> show
	else if (type == "abort"){
		std::cout << "MODIFY EDGE abort" << std::endl;
		auto robot_node = G->get_node(from);
		auto show_node = G->get_node(to);
		if (robot_node.has_value() && robot_node.value().name() == robot_name_
			&& show_node.has_value() && show_node.value().name() == "show"){
			std::cout << "Abort between robot and show" << std::endl;	
			if ( show_node.has_value() ) {	
				// Delete node show
				if (G->delete_node(show_node.value())) {
					std::cout << "Delete node show" << std::endl;
				}
				else{
					std::cout << "Can not delete node show" << std::endl;
				}
			}
			else{
				std::cout << "Can not delete node show because there is no node registered" << std::endl;
			}			
		}
	}
	// Check if the robot is beginning to say something, to show subtitles (or not)
	else if (type == "is_performing") {
		std::cout << "MODIFY EDGE is_performing" << std::endl;
		auto robot_node = G->get_node(from);
		auto say_node = G->get_node(to);
		if (robot_node.has_value() && robot_node.value().name() == robot_name_
			&& say_node.has_value() && say_node.value().name() == "say"){
			std::cout << "is_performing between robot and say" << std::endl;
			if (use_subtitles_) {
				std::cout << "using subtitles" << std::endl;
				if(auto text_sub = G->get_attrib_by_name<text_att>(say_node.value()); text_sub.has_value()){
					std::cout << "Subtitles: " << text_sub.value() << std::endl;
					responseJsonSub["subtitulos"] = text_sub.value();
					std::string mensaje = responseJsonSub.dump();
					if(conect){
						my_socket->sendOneFrameText(mensaje);
					}
				}
			}
		}
	}
	// Check if the robot finished saying something, to erase subtitles (or not)
	else if (type == "finished") {
		std::cout << "MODIFY EDGE finished" << std::endl;
		auto robot_node = G->get_node(from);
		auto say_node = G->get_node(to);
		if (robot_node.has_value() && robot_node.value().name() == robot_name_
			&& say_node.has_value() && say_node.value().name() == "say"){
			std::cout << "finished between robot and say" << std::endl;
			if (use_subtitles_) {
				std::cout << "using subtitles" << std::endl;
				std::string text_sub = "";
                responseJsonSub["subtitulos"] = text_sub;
                std::string mensaje = responseJsonSub.dump();
                if(conect){
                    my_socket->sendOneFrameText(mensaje);
                }
			}
		}
	}
}

void DSR_interface::del_edge_slot(std::uint64_t from, std::uint64_t to, const std::string &edge_tag){
	std::cout << "Delete edge of type : " << edge_tag << std::endl;
	// Check if the robot was interacting with a person: robot ---(interacting)--> person
	if (edge_tag == "interacting"){	
		use_subtitles_ = true; // returns to default behaviour (using subtitles) when interaction ends
	}
}