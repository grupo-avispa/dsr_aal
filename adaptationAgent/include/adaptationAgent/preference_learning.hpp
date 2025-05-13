// Copyright (c) 2024 Grupo Avispa, DTE, Universidad de Málaga
// Copyright (c) 2024 Joaquín Ballesteros Gómez
// Copyright (c) 2024 Alberto J. Tudela Roldán
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

#include <map>
#include <string>
#include <vector>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "adaptationAgent/types.hpp"

#ifndef ADAPTATIONCOMP__PREFERENCE_LEARNING_HPP_
#define ADAPTATIONCOMP__PREFERENCE_LEARNING_HPP_

/**
 * @brief Class that manage the preference learning algorithm
 */
class PreferenceLearning
{
public:
  /**
   * @brief Construct a new Preference Learning object
   *
   */
  PreferenceLearning();

  /**
   * @brief Construct a new Preference Learning object
   *
   * @param strValue The path to the folder where the ONNX models are stored.
   */
  PreferenceLearning(const std::string & strValue);

  /**
   * @brief Destroy the Preference Learning object
   *
   */
  ~PreferenceLearning() = default;

  /**
  * @brief Loads the sessions.
  * It creates the Ort::Env object and loads all the ONNX models files
  * from the folderpath class variable.
  *
  * @param folderpath The name of the folder where the ONNX models are stored.
  */
  void loadSessions(std::string folderpath);

  /**
   * @brief Given a vector of integer input_data, it returns a vector of UseCase
   * with the priorities of the use cases.
   *
   * @param input_data The input data in form of vector of integer.
   * @return std::vector<UseCase> The priorities of the use cases.
   */
  std::vector<UseCase> getPriorities(std::vector<int64_t> input_data);

private:
  /**
   * @brief Loads all the ONNX models files from the folderpath class variable and stores
   * the names in the model_names_ class variable.
   *
   * @param folderpath The name of the folder where the ONNX models are stored.
  */
  void loadModelsFiles(std::string folderpath);

  /**
   * @brief It returns, for a given file name like Model_(DEAM,GIWA).onnx,
   * the use case DEAM or GIWA depending on the label.
   *
   * @param filename the name of the ONNX file, it includes the use cases in (USECASE1,USECASE2)
   * @param label The classification for a given input, if it is 1 the USECASE1 is selected,
   * USECASE2 is selected if it is -1
   * @return std::string The first use case if the label is 1, and the second otherwise.
   */
  std::string getStringUseCase(std::string filename, int label);

  /**
   * @brief Given a model session and input vector input_data it returns
   * the classification label (-1 or 1).
   *
   * @param session  Model loaded previusly
   * @param input_data  Input data in form of vector of integer.
   * @return int64_t The classification label, 1 if the first use case of the model is selected,
   * -1 if the second one is selected.
   */
  int64_t evaluate(Ort::Session * session, std::vector<int64_t> input_data);

  const std::map<std::string, UseCase> usecase_strings_ = {
    {"DEAM", UseCase::WANDERING},
    {"RECA", UseCase::CHARGING},
    {"MENU", UseCase::MENU},
    {"TEMU", UseCase::MUSIC},
    {"NEUR", UseCase::NEURON_UP},
    {"GIWA", UseCase::GETME},
    {"ORDA", UseCase::REMINDER}
  };

  Ort::Env env_;
  std::vector<std::string> model_names_;
  std::vector<Ort::Session> sessions_;
};

#endif  // ADAPTATIONCOMP__PREFERENCE_LEARNING_HPP_
