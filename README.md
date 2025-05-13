# dsr_aal

![License](https://img.shields.io/github/license/grupo-avispa/dsr_aal)

## Overview

``dsr_aal`` is a package that provides a set of DSR agents for different use cases and environments. These agents are designed to enhance the robot's capabilities and enable it to perform various tasks in different environments. The package includes the following agents:

* [adaptationAgent]: DSR agent that changes the behavior of the robot depending on the context.
* [mqtt_agent]: DSR agent that connects the robot to an MQTT broker, allowing it to publish and subscribe to topics.
* [wasp_dsr_planner]: DSR agent that loads a Behavior Tree engine and plans the use case and send it to/from the DSR.
* [webServerAgent]: DSR agent that start the webserver inside the robot.
* [speechAgent]: DSR agent that tells the robot what and when to speak.

## Installation

### Building from Source

#### Building

To build from source, clone the latest version from this repository and compile the package using the following command:
```bash
cd $HOME
git clone --recurse-submodules https://github.com/grupo-avispa/dsr_aal.git
cd dsr_aal
mkdir -p build && cd build
cmake .. && make -j4 && sudo make install
```

[adaptationAgent]: /adaptationAgent
[mqtt_agent]: /mqtt_agent
[speechAgent]: /speechAgent
[wasp_dsr_planner]: /wasp_dsr_planner
[webServerAgent]: /webServerAgent