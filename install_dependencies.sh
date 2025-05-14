#!/bin/bash

# ----------------------------------------------------
#     INSTALL ROBOCOMP COMPONENTS FOR CLARA ROBOTS
# ----------------------------------------------------

# Variables
SCRIPT_DIR=$(pwd)

# Colors
RED="\e[31m"
GREEN="\e[32m"
YELLOW="\e[33m"
BLUE="\e[34m"
MAGENTA="\e[35m"
CYAN="\e[36m"
BOLDRED="\e[1;31m"
BOLDGREEN="\e[1;32m"
ITALICRED="\e[3;31m"
ITALICGREEN="\e[3;32m"
ENDCOLOR="\e[0m"

# Functions
confirm() {
  echo -e -n "$1 ? (y/n) "
  read ans
  case "$ans" in
  y | Y | yes | YES | Yes) return 0 ;;
  *) return 1 ;;
  esac
}

clone_pull() {
  DIRECTORY=$(basename "$1" .git)
  if [ -d "$DIRECTORY" ]; then
    cd "$DIRECTORY"
    if [[ -z $2 ]]; then
      set "$1" "master"
    fi
    git pull origin "$2"
    cd ../
  else
    git clone "$1"
  fi
}

printError() {
  echo -e "${RED}$1${ENDCOLOR}"
  exit 1
}

# --------------------------------------------------------------------------------------------------------------------------------
echo -e "${BOLDGREEN}----------------------------------------------------------------${ENDCOLOR}"
echo -e "${BOLDGREEN}     SCRIPT TO INSTALL ROBOCOMP COMPONENTS FOR CLARA ROBOTS     ${ENDCOLOR}"
echo -e "${BOLDGREEN}----------------------------------------------------------------${ENDCOLOR}"

# Update and upgrade the system
echo -e "${CYAN}- First, we will update and upgrade the system.${ENDCOLOR}"
DEBIAN_FRONTEND=noninteractive apt -y update && DEBIAN_FRONTEND=noninteractive apt -y upgrade

# Install dependencies
echo -e "${CYAN}- First, we will install some dependencies.${ENDCOLOR}"
DEBIAN_FRONTEND=noninteractive apt install -y curl git cmake make build-essential libboost-all-dev || printError "- The dependencies could not be installed. Please, check the log and try again."
DEBIAN_FRONTEND=noninteractive apt install -y nlohmann-json3-dev libeigen3-dev libttspico-utils speech-dispatcher sox libwebsockets-dev libzmq3-dev libncurses-dev libspdlog-dev moreutils || printError "- The dependencies could not be installed. Please, check the log and try again."
DEBIAN_FRONTED=noninteractive apt install -y libopenscenegraph-dev libtinyxml2-dev qtbase5-dev || printError "- The dependencies could not be installed. Please, check the log and try again."

# Install Oat++
echo -e "${CYAN}- Next, we will install Oat++.${ENDCOLOR}"
if [ ! -d "/usr/local/include/oatpp" ]; then
  echo -e "${YELLOW}- Oat++ is not installed. Installing ...${ENDCOLOR}"
  # Install Oat++
  cd /tmp
  git clone https://github.com/oatpp/oatpp.git
  cd oatpp
  mkdir -p build && cd build
  cmake .. && make -j$(nproc) && sudo make install
fi

# Install ONNX runtime
echo -e "${CYAN}- Next, we will install ONNX runtime.${ENDCOLOR}"
if [ ! -d "/usr/local/include/onnxruntime" ]; then
  echo -e "${YELLOW}- ONNX runtime is not installed. Installing ...${ENDCOLOR}"
  # Install ONNX runtime
  cd /tmp
  curl -sSL https://github.com/microsoft/onnxruntime/releases/download/v1.17.0/onnxruntime-linux-x64-1.17.0.tgz -o onnxruntime-linux-x64-1.17.0.tgz || printError "- The ONNX runtime could not be downloaded. Please, check the log and try again."
  tar -xvzf onnxruntime-linux-x64-1.17.0.tgz && rm onnxruntime-linux-x64-1.17.0.tgz
  cp -R onnxruntime-linux-x64-1.17.0/include /usr/local/include/onnxruntime
  cp -R onnxruntime-linux-x64-1.17.0/lib /usr/local
  rm -R onnxruntime-linux-x64-1.17.0
  echo -e "${GREEN}- ONNX runtime have been installed.${ENDCOLOR}"
else
  echo -e "${GREEN}- ONNX runtime is already installed.${ENDCOLOR}"
fi
