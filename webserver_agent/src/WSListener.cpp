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

#include "../include/WSListener.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

DSR_interface my_DSR;
int cont_menu = 0;
MenuSelection menu_selection;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSListener

void WSListener::onPing(const WebSocket& socket, const oatpp::String& message) {
	OATPP_LOGD(TAG, "onPing");
	socket.sendPong(message);
}

void WSListener::onPong(const WebSocket& socket, const oatpp::String& message) {
	OATPP_LOGD(TAG, "onPong");
}

void WSListener::onClose(const WebSocket& socket, v_uint16 code, const oatpp::String& message) {
	OATPP_LOGD(TAG, "onClose code=%d", code);
}

void WSListener::readMessage(const WebSocket& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) {
	bool volume = false;
	OATPP_LOGD(TAG, "Received Message");
	// message transfer finished		
	if(size == 0) { 
		auto wholeMessage = m_messageBuffer.toString();
		m_messageBuffer.setCurrentPosition(0);

		std::string client_message = wholeMessage->c_str();
		OATPP_LOGD(TAG, "onMessage message='%s'", client_message);
		nlohmann::json jmessage = nlohmann::json::parse(client_message);
		std::string action = jmessage["action"].template get<std::string>();
		std::cout << "RECEIVED ACTION: " << action << std::endl;

		// ##### STARTING MENU ##### //
		if(action == "MENU ON"){
			std::string mensajeName = responseJsonName.dump();
			socket.sendOneFrameText( mensajeName);
			std::string mensajeMenuChoices = responseJsonMenuChoices.dump();
			socket.sendOneFrameText( mensajeMenuChoices);
			std::string mensajeSub = responseJsonSub.dump();
			socket.sendOneFrameText( mensajeSub);
		// ##### RECEIVED MENU ##### //
		}else if(action == "MENU FIRST SELECTED"){
			// Convertir el string JSON a una variable JSON
			std::string menu = jmessage["params"].template get<std::string>();
			//nlohmann::json jmenu = nlohmann::json::parse(menu);
			menu_selection.primero = menu;
			// Acceder a los valores dentro del objeto JSON
			std::cout << "Primer plato: " << menu_selection.primero << std::endl;
			responseJsonMenuSelected["number"] = "first";
			std::string mensaje = responseJsonMenuSelected.dump();
			// Create node menu_selection and edge is performing
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			DSR::Node newNode = DSR::Node::create<menu_selection_node_type>("menu_selection");
			if (auto id = my_DSR.G->insert_node(newNode); id.has_value()){
				std::cout << "Inserting node '" << newNode.name() << "' to the graph..." << std::endl;
				auto edge = DSR::Edge::create<is_performing_edge_type>(robot_node.value().id(),  newNode.id());
                if (my_DSR.G->insert_or_assign_edge(edge)) {
                    std::cout << "Inserted edge is_performing for menu_selection node" << std::endl;
                }
			}
			// Sending response to  socket clients
			if(my_socket){
				my_socket->sendOneFrameText(mensaje);
			}
			if(button_socket){
				button_socket->sendOneFrameText(mensaje);
			}
		}else if(action == "MENU SECOND SELECTED"){
			// Convertir el string JSON a una variable JSON
			std::string menu = jmessage["params"].template get<std::string>();
			//nlohmann::json jmenu = nlohmann::json::parse(menu);
			menu_selection.segundo = menu;
			// Acceder a los valores dentro del objeto JSON
			std::cout << "Segundo plato: " << menu_selection.segundo << std::endl;
			responseJsonMenuSelected["number"] = "second";
			std::string mensaje = responseJsonMenuSelected.dump();
			// Sending response to  socket clients
			if(my_socket){
				my_socket->sendOneFrameText(mensaje);
			}
			if(button_socket){
				button_socket->sendOneFrameText(mensaje);
			}
			//Actualizar perfil de usuario con la elección del menu
			nlohmann::json jmenu(menu_selection);
			//Pillar el nodo de la persona a través de la función modify_edge que detecta el nodo enganchado al enlace is_with
			//Mutex
			QMutexLocker locker(mutex);
			std::optional<std::string> person_identifier = my_DSR.G->get_attrib_by_name<identifier_att>(my_DSR.person_node.value());
			std::cout << "Person Node = " << person_identifier.value() << " Menu = " << jmenu.dump(4) << std::endl; 
			//Actualizar sus atributos con las opciones de menu
			if(cont_menu == 0){
				my_DSR.G->add_or_modify_attrib_local<menu1_att>(my_DSR.person_node.value(), jmenu.dump());
				if(my_DSR.G->update_node(my_DSR.person_node.value())){
					std::cout << "MENU UPDATED" << std::endl;
				}else{
					std::cout << "FAIL MENU UPDATED" << std::endl;
				}
			}
			else if(cont_menu == 1){
				my_DSR.G->add_or_modify_attrib_local<menu2_att>(my_DSR.person_node.value(), menu);
				if(my_DSR.G->update_node(my_DSR.person_node.value())){
					std::cout << "MENU UPDATED" << std::endl;
				}else{
					std::cout << "FAIL MENU UPDATED" << std::endl;
				}
			}
			else if(cont_menu == 2){
				my_DSR.G->add_or_modify_attrib_local<menu3_att>(my_DSR.person_node.value(), menu);
				if(my_DSR.G->update_node(my_DSR.person_node.value())){
					std::cout << "MENU UPDATED" << std::endl;
				}else{
					std::cout << "FAIL MENU UPDATED" << std::endl;
				}
			}
			else if(cont_menu == 3){
				my_DSR.G->add_or_modify_attrib_local<menu4_att>(my_DSR.person_node.value(), menu);
				if(my_DSR.G->update_node(my_DSR.person_node.value())){
					std::cout << "MENU UPDATED" << std::endl;
				}else{
					std::cout << "FAIL MENU UPDATED" << std::endl;
				}
			}
			else if(cont_menu == 4){
				my_DSR.G->add_or_modify_attrib_local<menu5_att>(my_DSR.person_node.value(), menu);
				if(my_DSR.G->update_node(my_DSR.person_node.value())){
					std::cout << "MENU UPDATED" << std::endl;
				}else{
					std::cout << "FAIL MENU UPDATED" << std::endl;
				}
			}
			else if(cont_menu == 5){
				my_DSR.G->add_or_modify_attrib_local<menu6_att>(my_DSR.person_node.value(), menu);
				if(my_DSR.G->update_node(my_DSR.person_node.value())){
					std::cout << "MENU UPDATED" << std::endl;
				}else{
					std::cout << "FAIL MENU UPDATED" << std::endl;
				}
			}
			else if(cont_menu == 6){
				my_DSR.G->add_or_modify_attrib_local<menu7_att>(my_DSR.person_node.value(), menu);
				if(my_DSR.G->update_node(my_DSR.person_node.value())){
					std::cout << "MENU UPDATED" << std::endl;
				}else{
					std::cout << "FAIL MENU UPDATED" << std::endl;
				}
			}
			// Delete is_performing edge between robot and show node and create new edge fisnished
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			auto show_node = my_DSR.G->get_node("show");
			if(robot_node.has_value() && show_node.has_value()){
				// Replace edge 'is_performing' to 'finished' between robot and show
				if (my_DSR.G->delete_edge(robot_node.value().id(), show_node.value().id(), "is_performing")) {
					auto edge = DSR::Edge::create<finished_edge_type>(robot_node.value().id(),  show_node.value().id());
					if (my_DSR.G->insert_or_assign_edge(edge)) {
						std::cout << "Finished action " <<  show_node.value().name() << std::endl;
					}
				}else {
				std::cout << "ERROR: Trying to delete edge type is_performing" << std::endl;
				}
			}
			cont_menu++;
			if(cont_menu == 7){
				cont_menu = 0;
			}
		// ##### MENU FINISHED ##### //
		}else if(action == "INTERFACE"){
			std::string client = jmessage["params"].template get<std::string>();
			if(client == "button"){
				std::cout << "Setting ESP-32 button socket" << std::endl;
				button_socket = std::make_shared<oatpp::websocket::WebSocket>(socket.getConnection(), false);
			}else if(client == "html"){
				std::cout << "Setting HTML interface socket" << std::endl;
				my_socket = std::make_shared<oatpp::websocket::WebSocket>(socket.getConnection(), false);
			}
			else{
				std::cout << "Error, client type [" << client << "], must be [button] or [html]"  << std::endl;
			}
		}else if(action == "MENU OFF"){
			responseJsonInterface["interface"] = "default";
			std::string mensaje = responseJsonInterface.dump();
			std::cout << "Sending Interface: " << mensaje << "\nConect value: " << conect << std::endl;
			if(conect){
				socket.sendOneFrameText(mensaje);
				std::cout << "Interface has been sent..." << std::endl;
			}
		// ##### RECEIVED NEURON ##### //
		}else if(action == "NEURON SELECTED"){
			// If accuracy field is set to true increment accuracy counter
			if(jmessage["params"].template get<bool>()){
				accuracy += 1.0f;
			}
			tries += 1.0f;
		// ##### FINISHED NEURON ##### //
		}else if(action == "NEURON OFF"){
			accuracy = accuracy / tries;
			std::cout << "accuracy:"  << accuracy << " // tries: " << tries << std::endl;
			// Get the mutex and set person accuracy value
			QMutexLocker locker(mutex);
			std::optional<std::string> person_identifier = my_DSR.G->get_attrib_by_name<identifier_att>(my_DSR.person_node.value());
			std::cout << "Person Node = " << person_identifier.value() << std::endl; 
			// Update person accuracy value
			my_DSR.G->add_or_modify_attrib_local<accuracy_att>(my_DSR.person_node.value(), accuracy);
			if(my_DSR.G->update_node(my_DSR.person_node.value())){
				std::cout << "ACCURACY OK" << std::endl;
			}else{
				std::cout << "ACCURACY FAIL" << std::endl;
			}
			// Finished show
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			auto show_node = my_DSR.G->get_node("show");
			if(robot_node.has_value() && show_node.has_value()){
				// Replace edge 'is_performing' to 'finished' between robot and show
				if (my_DSR.G->delete_edge(robot_node.value().id(), show_node.value().id(), "is_performing")) {
					auto edge = DSR::Edge::create<finished_edge_type>(robot_node.value().id(),  show_node.value().id());
					if (my_DSR.G->insert_or_assign_edge(edge)) {
						std::cout << "Finished action " <<  show_node.value().name() << std::endl;
					}
				}else {
				std::cout << "ERROR: Trying to delete edge type is_performing" << std::endl;
				}
			}
			// Reset interface
			responseJsonInterface["interface"] = "default";
			std::string mensaje = responseJsonInterface.dump();
			if(conect){
				socket.sendOneFrameText(mensaje);
				std::cout << "Sent default interface ..." << std::endl;
			}
			// Reset control variables
			tries = 0.0;
			accuracy = 0.0;
		// ##### EXPLANATION ##### //
		}else if(action == "BUTTON EXPLAIN"){
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			// Person pressed the button on default interface
			DSR::Node newNode = DSR::Node::create<explanation_node_type>("explanation");
			my_DSR.G->add_or_modify_attrib_local<source_att>(newNode, my_DSR.robot_name_);
			if (auto id = my_DSR.G->insert_node(newNode); id.has_value()){
				std::cout << "Inserting node '" << newNode.name() << "' to the graph..." << std::endl;
			}		
			auto edge = DSR::Edge::create<wants_to_edge_type>(robot_node.value().id(), newNode.id());
			my_DSR.G->add_or_modify_attrib_local<source_att>(edge, my_DSR.robot_name_);
			if (my_DSR.G->insert_or_assign_edge(edge)) {
				std::cout << "Wants_to edge" << std::endl;
			}
		// ##### BRING ME WATER ##### //
		}else if(action == "BUTTON WATER"){
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			// Person pressed the button on default interface
			DSR::Node newNode = DSR::Node::create<bring_water_node_type>("bring_water");
			my_DSR.G->add_or_modify_attrib_local<source_att>(newNode, my_DSR.robot_name_);
			if (auto id = my_DSR.G->insert_node(newNode); id.has_value()){
				std::cout << "Inserting node '" << newNode.name() << "' to the graph..." << std::endl;
			}		
			auto edge = DSR::Edge::create<wants_to_edge_type>(robot_node.value().id(), newNode.id());
			my_DSR.G->add_or_modify_attrib_local<source_att>(edge, my_DSR.robot_name_);
			if (my_DSR.G->insert_or_assign_edge(edge)) {
				std::cout << "Wants_to edge" << std::endl;
			}
		// ##### FINISH BRING ME WATER ##### //
		}else if(action == "BUTTON WATER OFF"){
			// Get person necessity
			QMutexLocker locker(mutex);
			std::string necessity = jmessage["params"].template get<std::string>();
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			// Necesity field is not empty when people press the button on bring water interface
			// Necesity field is empty when people dont press the button and the timeout finish
			if(!necessity.empty()){
				// Update person necessity
				my_DSR.G->add_or_modify_attrib_local<necessity_att>(my_DSR.person_node.value(), necessity);
				my_DSR.G->update_node(my_DSR.person_node.value());
				std::cout << "Updated person node with necessity att: " << necessity << std::endl;
			}
			// Finish bring me water use case
			auto show_node = my_DSR.G->get_node("show");
			if(robot_node.has_value() && show_node.has_value()){
					// Replace edge 'is_performing' to 'finished' between robot and show
					if (my_DSR.G->delete_edge(robot_node.value().id(), show_node.value().id(), "is_performing")) {
						auto edge = DSR::Edge::create<finished_edge_type>(robot_node.value().id(),  show_node.value().id());
						if (my_DSR.G->insert_or_assign_edge(edge)) {
							std::cout << "Finished action " <<  show_node.value().name() << std::endl;
						}
					}else {
					std::cout << "ERROR: Trying to delete edge type is_performing" << std::endl;
					}
			}
		// ##### SET VOLUME ##### //	
		}else if(action == "VOLUME"){
			std::string volume = jmessage["params"].template get<std::string>();
			float vol = std::stof(volume);
			//std::string command = "amixer set Master " + volume + "%";
			//system(command.c_str());
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			DSR::Node newNode = DSR::Node::create<set_volume_node_type>("set_volume");
			my_DSR.G->add_or_modify_attrib_local<source_att>(newNode, my_DSR.robot_name_);
			my_DSR.G->add_or_modify_attrib_local<volume_att>(newNode, vol);
			if (auto id = my_DSR.G->insert_node(newNode); id.has_value()){
				std::cout << "Inserting node '" << newNode.name() << "' to the graph..." << std::endl;
			}		
			auto edge = DSR::Edge::create<wants_to_edge_type>(robot_node.value().id(), newNode.id());
			my_DSR.G->add_or_modify_attrib_local<source_att>(edge, my_DSR.robot_name_);
			if (my_DSR.G->insert_or_assign_edge(edge)) {
				std::cout << "Wants_to edge" << endl;
			}
		// ##### TRACKING ON ##### //
		}else if(action == "BUTTON TRACKING ON"){
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			DSR::Node newNode = DSR::Node::create<track_node_type>("tracking");
			my_DSR.G->add_or_modify_attrib_local<source_att>(newNode, my_DSR.robot_name_);
			if (auto id = my_DSR.G->insert_node(newNode); id.has_value()){
				std::cout << "Inserting node '" << newNode.name() << "' to the graph..." << std::endl;
			}		
			auto edge = DSR::Edge::create<wants_to_edge_type>(robot_node.value().id(), newNode.id());
			my_DSR.G->add_or_modify_attrib_local<source_att>(edge, my_DSR.robot_name_);
			if (my_DSR.G->insert_or_assign_edge(edge)) {
				std::cout << "Wants_to edge" << endl;
			}
		// ##### TRACKING OFF ##### //
		}else if(action == "BUTTON TRACKING OFF"){
			auto robot_node = my_DSR.G->get_node(my_DSR.robot_name_);
			auto tracking_node = my_DSR.G->get_node("tracking");
			if(robot_node.has_value() && tracking_node.has_value()){
				bool repl_edge = DSR::replace_edge<finished_edge_type>(my_DSR.G,robot_node.value().id(),tracking_node.value().id(),"is_performing", my_DSR.robot_name_);
				if(repl_edge){
					std::cout << "Edge replace correctly" << std::endl;
				}
			}
		}
	} else if(size > 0) { // message frame received
		m_messageBuffer.writeSimple(data, size);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSInstanceListener
std::atomic<v_int32> WSInstanceListener::SOCKETS(0);

void WSInstanceListener::onAfterCreate(const oatpp::websocket::WebSocket& socket, const std::shared_ptr<const ParameterMap>& params) {
	// Increse sockets counter and print conection is done
	SOCKETS ++;
	OATPP_LOGD(TAG, "New Incoming Connection. Connection count=%d", SOCKETS.load());
	// Genereate shared pointer from the socket for assync server->client comunication
	socket.setListener(std::make_shared<WSListener>());
	conect = SOCKETS;
	// Initialize battery value on html interface
	auto battery_node = my_DSR.G->get_node("battery");
	if(battery_node.has_value()){
		auto battery_level = my_DSR.G->get_attrib_by_name<battery_percentage_att>(battery_node.value());
		if(battery_level.has_value()){
			responseJsonBattery["bateria"] = battery_level.value(); 
		}
	}
	std::string batt_message = responseJsonBattery.dump();
	socket.sendOneFrameText(batt_message);
	std::string mensajeSub = responseJsonSub.dump();
	socket.sendOneFrameText( mensajeSub);
	OATPP_LOGD(TAG, "Socket connection created and enviroment variables were initialized");
}

void WSInstanceListener::onBeforeDestroy(const oatpp::websocket::WebSocket& socket) {
	SOCKETS --;
	OATPP_LOGD(TAG, "Connection closed. Connection count=%d", SOCKETS.load());
	conect = SOCKETS;
}