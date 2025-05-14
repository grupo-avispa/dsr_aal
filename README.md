# dsr_aal

![License](https://img.shields.io/github/license/grupo-avispa/dsr_aal)
<!-- [![Build](https://github.com/grupo-avispa/dsr_aal/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/grupo-avispa/dsr_aal/actions/workflows/build.yml) -->


## Overview

``dsr_aal`` is a package that provides a set of DSR agents for different use cases and environments. These agents are designed to enhance the robot's capabilities and enable it to perform various tasks in different environments. The package includes the following agents:

* [adaptation_agent]: DSR agent that changes the behavior of the robot depending on the context.
* [mqtt_dsr_agent]: DSR agent that connects the robot to an MQTT broker, allowing it to publish and subscribe to topics.
* [speech_agent]: DSR agent that tells the robot what and when to speak.
* [wasp_dsr_planner]: DSR agent that loads a Behavior Tree engine and plans the use case and send it to/from the DSR.
* [webserver_agent]: DSR agent that start the webserver inside the robot.


## Installation

### Building from Source

#### Dependencies

- [Cortex](https://github.com/grupo-avispa/cortex) (Library for Deep State Representation)
- [Oatpp](https://github.com/oatpp/oatpp) (HTTP server and client)
- [BehaviorTree.CPP](https://www.behaviortree.dev) (Behavior Tree library)
- [ONNXRuntime](https://onnxruntime.ai/) (ONNX Runtime for running machine learning models)

#### Building

To build from source, clone the latest version from this repository and compile the package using the following command:
```bash
cd $HOME
git clone --recurse-submodules https://github.com/grupo-avispa/dsr_aal.git
cd dsr_aal
./install_dependencies.sh
```

Then build the package using the following command:
```bash
cd dsr_aal
mkdir -p build && cd build
cmake .. && make -j4 && sudo make install
```

[adaptation_agent]: /adaptation_agent
[mqtt_dsr_agent]: /mqtt_dsr_agent
[speech_agent]: /speech_agent
[wasp_dsr_planner]: /wasp_dsr_planner
[webserver_agent]: /webserver_agent