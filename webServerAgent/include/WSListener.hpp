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

#ifndef WSListener_hpp
#define WSListener_hpp

#include "oatpp-websocket/ConnectionHandler.hpp"
#include "oatpp-websocket/WebSocket.hpp"
#include "../../../include/json_messages.hpp"

// DSR
#include "dsr/api/dsr_api.h"
#include "dsr/gui/dsr_gui.h"
#include "../../../include/dsr_api_ext.hpp"
#include "./DSR_init.hpp"

extern int conect;
extern int use_case; //0=wandering, 1=menu
extern int receive;
extern nlohmann::json responseJsonBattery;
extern nlohmann::json responseJsonName;
extern nlohmann::json responseJsonSub;
extern nlohmann::json responseJsonInterface;
extern nlohmann::json responseJsonMenuChoices;
extern std::string message;

/**
 * WebSocket listener listens on incoming WebSocket events.
 */
class WSListener : public oatpp::websocket::WebSocket::Listener {
private:
	static constexpr const char* TAG = "Server_WSListener";
private:
	/**
	 * Buffer for messages. Needed for multi-frame messages.
	 */
	oatpp::data::stream::BufferOutputStream m_messageBuffer;
public:

	/**
	 * Called on "ping" frame.
	 */
	void onPing(const WebSocket& socket, const oatpp::String& message) override;

	/**
	 * Called on "pong" frame
	 */
	void onPong(const WebSocket& socket, const oatpp::String& message) override;

	/**
	 * Called on "close" frame
	 */
	void onClose(const WebSocket& socket, v_uint16 code, const oatpp::String& message) override;

	/**
	 * Called on each message frame. After the last message will be called once-again with size == 0 to designate end of the message.
	 */
	void readMessage(const WebSocket& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) override;

};

/**
 * Listener on new WebSocket connections.
 */
class WSInstanceListener : public oatpp::websocket::ConnectionHandler::SocketInstanceListener {
private:
	static constexpr const char* TAG = "Server_WSInstanceListener";
public:
	/**
	 * Counter for connected clients.
	 */
	static std::atomic<v_int32> SOCKETS;
public:

	/**
	 *  Called when socket is created
	 */
	void onAfterCreate(const oatpp::websocket::WebSocket& socket, const std::shared_ptr<const ParameterMap>& params) override;

	/**
	 *  Called before socket instance is destroyed.
	 */
	void onBeforeDestroy(const oatpp::websocket::WebSocket& socket) override;

};

#endif // WSListener_hpp
