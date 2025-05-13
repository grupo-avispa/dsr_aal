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

#ifndef WEBSERVERAGENT_HPP
#define WEBSERVERAGENT_HPP

// OAT++
#include "./WSListener.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp-websocket/ConnectionHandler.hpp"

// UTILS
#include "nlohmann/json.hpp"
#include "../../../include/json_messages.hpp"

#include "./DSR_init.hpp"


void initializeJson();

class webServerAgent{
    public:
	webServerAgent();
	~webServerAgent();
	//######################//
	//    OATPP COMPONETS	//
	//######################//
	/**
	 *  Create ConnectionProvider component which listens on the port
	 */
	OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([] {
	return oatpp::network::tcp::server::ConnectionProvider::createShared({"0.0.0.0", 8085, oatpp::network::Address::IP_4});
	}());

	/**
	 *  Create Router component
	 */
	OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
	return oatpp::web::server::HttpRouter::createShared();
	}());

	/**
	 *  Create http ConnectionHandler
	 */
	OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, httpConnectionHandler)("http" /* qualifier */, [] {
	OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component
	return oatpp::web::server::HttpConnectionHandler::createShared(router);
	}());

	/**
	 *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
	 */
	OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)([] {
	return oatpp::parser::json::mapping::ObjectMapper::createShared();
	}());

	/**
	 *  Create websocket connection handler
	 */
	OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler)("websocket" /* qualifier */, [] {
	auto connectionHandler = oatpp::websocket::ConnectionHandler::createShared();
	connectionHandler->setSocketInstanceListener(std::make_shared<WSInstanceListener>());
	return connectionHandler;
	}());
};

#endif // WEBSERVERAGENT_HPP