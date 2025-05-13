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

// C++
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <sstream>
#include <fstream>
#include <signal.h>

// OAT++
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/web/server/HttpRouter.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp-websocket/ConnectionHandler.hpp"
#include "oatpp/network/Server.hpp"
#include "../include/WSListener.hpp"
#include "../include/MyController.hpp"

// DSR
#include "dsr/api/dsr_api.h"
#include "dsr/gui/dsr_gui.h"
#include "../../../include/dsr_api_ext.hpp"
#include "../include/DSR_init.hpp"

// UTILS
#include "../include/webServerAgent.hpp"

std::thread oatppThread;

namespace MyOatppFunctions {
    bool server_running = false;
    std::mutex server_op_mutex;
    std::atomic_bool server_should_continue;

    /**
     * You can't run two of those threads in one application concurrently in this setup. Especially with the AppComponents inside the
     * Threads scope. If you want to run multiple threads with multiple servers you either have to manage the components
     * by yourself and do not rely on the OATPP_COMPONENT mechanism or have one process-global AppComponent.
     * Further you have to make sure you don't have multiple ServerConnectionProvider listening to the same port.
     */
    void StartOatppServer() {
        std::lock_guard<std::mutex> lock(server_op_mutex);

        /* Check if server is already running, if so, do nothing. */
        if (server_running) {
            return;
        }

        /* Signal that the server is running */
        server_running = true;

        /* Tell the server it should run */
        server_should_continue.store(true);

        oatppThread = std::thread([] {
            /* Register components in scope of thread WARNING: COMPONENTS ONLY VALID WHILE THREAD IS RUNNING! */
            webServerAgent components;

            /* Get router component */
            OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
            /* Create MyController and add all of its endpoints to router */
            router->addController(std::make_shared<MyController>());

            /* Get connection handler component */
            //OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
            OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, "http");

            /* Get connection provider component */
            OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

            /* Create server which takes provided TCP connections and passes them to HTTP connection handler */
            oatpp::network::Server server(connectionProvider, connectionHandler);

            /* Run server, let it check a lambda-function if it should continue to run
            * Return true to keep the server up, return false to stop it.
            * Treat this function like a ISR: Don't do anything heavy in it! Just check some flags or at max some very
            * lightweight logic.
            * The performance of your REST-API depends on this function returning as fast as possible! */
            std::function<bool()> condition = [](){
            return server_should_continue.load();
            };

            /* Print info about server port */
            OATPP_LOGI("MyApp", "Server running on port %s", connectionProvider->getProperty("port").getData());

            server.run(condition);

            /* Server has shut down, so we dont want to connect any new connections */
            connectionProvider->stop();

            /* Now stop the connection handler and wait until all running connections are served */
            connectionHandler->stop();
        });
    }

    void StopOatppServer() {
        std::lock_guard<std::mutex> lock(server_op_mutex);

        /* Tell server to stop */
        server_should_continue.store(false);

        /* Wait for the server to stop */
        if (oatppThread.joinable()) {
            oatppThread.join();
        }
    }
}

void initializeJson(){
	responseJsonName["type"] = "name";
	responseJsonName["nombre"] = "";
	responseJsonBattery["type"] = "battery";
    responseJsonBattery["bateria"] = 99; // Set bateria to the variable value
	responseJsonSub["type"] = "subtitles";
    responseJsonSub["subtitulos"] = "";
	responseJsonInterface["type"] = "interface";
	responseJsonInterface["interface"] = "";
	responseJsonMenuChoices["type"] = "menu_choices";
	responseJsonMenuChoices["Lpri1"] = "";
	responseJsonMenuChoices["Lpri2"] = "";
	responseJsonMenuChoices["Lseg1"] = "";
	responseJsonMenuChoices["Lseg2"] = "";
    responseJsonMenuChoices["Mpri1"] = "";
	responseJsonMenuChoices["Mpri2"] = "";
	responseJsonMenuChoices["Mseg1"] = "";
	responseJsonMenuChoices["Mseg2"] = "";
    responseJsonMenuChoices["Xpri1"] = "";
	responseJsonMenuChoices["Xpri2"] = "";
	responseJsonMenuChoices["Xseg1"] = "";
	responseJsonMenuChoices["Xseg2"] = "";
    responseJsonMenuChoices["Jpri1"] = "";
	responseJsonMenuChoices["Jpri2"] = "";
	responseJsonMenuChoices["Jseg1"] = "";
	responseJsonMenuChoices["Jseg2"] = "";
    responseJsonMenuChoices["Vpri1"] = "";
	responseJsonMenuChoices["Vpri2"] = "";
	responseJsonMenuChoices["Vseg1"] = "";
	responseJsonMenuChoices["Vseg2"] = "";
    responseJsonMenuChoices["Spri1"] = "";
	responseJsonMenuChoices["Spri2"] = "";
	responseJsonMenuChoices["Sseg1"] = "";
	responseJsonMenuChoices["Sseg2"] = "";
    responseJsonMenuChoices["Dpri1"] = "";
	responseJsonMenuChoices["Dpri2"] = "";
	responseJsonMenuChoices["Dseg1"] = "";
	responseJsonMenuChoices["Dseg2"] = "";
	responseJsonMenuChoices["pos"] = "";
    responseJsonMenuSelected["type"] = "option_selected";
    responseJsonMenuSelected["number"] = "";
    responseJsonQuestion["type"] = "question";
    responseJsonQuestion["question"] = "";
    responseJsonAnswer["type"] = "answer";
    responseJsonAnswer["answer"] = "";
}

webServerAgent::webServerAgent(){}
webServerAgent::~webServerAgent(){}

void signalHandler(int signum){
    std::cout << std::endl << "Interrupt signal (" << signum << ") received." << std::endl;
    my_socket.reset();
    button_socket.reset();
    MyOatppFunctions::StopOatppServer();
    oatpp::base::Environment::destroy();
    my_DSR.G.reset();
    exit(signum); 

}

int main(int argc, char* argv[]){
    QCoreApplication app(argc, argv);
    if(argc != 2){
        std::cerr << "Error calling the executable, try: <./build/webServerAgent-exe path_to_this_directory/etc/config.txt>" << std::endl;
    }
    if(!my_DSR.configParamsParser(argv[1])){
        return -1;
    }
    initializeJson();
    my_DSR.initializeDSR();
    oatpp::base::Environment::init();
    MyOatppFunctions::StartOatppServer();
    signal (SIGINT,signalHandler);
    auto res = app.exec();
    return 0;
}