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

#include <iostream>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "adaptationAgent/preference_learning.hpp"

PreferenceLearning::PreferenceLearning()
{
}

void PreferenceLearning::loadSessions(std::string folderpath)
{
  env_ = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "test");

  loadModelsFiles(folderpath);

  // Create the session options if needed
  Ort::SessionOptions session_options;

  // Create the sessions
  for (const auto & modelName : model_names_) {
    //Ort::Session session(env, model_path.c_str(), session_options);
    sessions_.push_back(Ort::Session(env_, (folderpath + modelName).c_str(), session_options));
  }
}

void PreferenceLearning::loadModelsFiles(std::string folderpath)
{
  // Load all the model files from the folder
  for (const auto & entry : std::filesystem::directory_iterator(folderpath)) {
    if (entry.is_regular_file()) {
      model_names_.push_back(entry.path().filename().string());
    }
  }

  // Print the model names
  std::cout << "Model names:" << std::endl;
  for (const auto & name : model_names_) {
    std::cout << name << ", ";
  }
  std::cout << std::endl;
}

std::string PreferenceLearning::getStringUseCase(std::string filename, int label)
{
  size_t start = filename.find('(');
  size_t middle = filename.find(',');
  size_t end = filename.find(')');

  std::string substr = filename.substr(start + 1, middle - start - 1);
  if (label < 0) {
    substr = filename.substr(middle + 1, end - middle - 1);
  }
  return substr;
}

int64_t PreferenceLearning::evaluate(Ort::Session * session, std::vector<int64_t> input_data)
{
  // Get the input and output names
  std::vector<const char *> input_names;
  std::vector<const char *> output_names;

  input_names.push_back("X");
  output_names.push_back("output_label");
  output_names.push_back("output_probability");

  // Define the shape of the input tensor [1, 6]
  std::vector<int64_t> input_shape = {1, 4};

  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
  // Create an Ort::Value object to contain the input data
  Ort::Value input_tensor = Ort::Value::CreateTensor<int64_t>(
    memory_info, input_data.data(), input_data.size(),
    input_shape.data(), input_shape.size());

  // Execute the inference
  std::vector<Ort::Value> output_tensors = session->Run(
    Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, 1,
    output_names.data(), output_names.size());

  auto output_data = output_tensors[0].GetTensorMutableData<int64_t>();
  return output_data[0];
}


std::vector<UseCase> PreferenceLearning::getPriorities(std::vector<int64_t> input_data)
{
  // Print the input data
  /* std::cout << "Input data ";
  for (size_t i = 0; i < input_data.size(); ++i) {
    std::cout << input_data[i];
    if (i != input_data.size() - 1) {
      std::cout << " ";
    }
  }
  std::cout << std::endl;
  */

  // Create a map to store the name of the use case and the number of times it is selected
  std::map<std::string, int> name_to_id;
  for (size_t i = 0; i < sessions_.size(); ++i) {
    auto output_data = evaluate(&(sessions_[i]), input_data);
    std::string aux = getStringUseCase(model_names_[i], output_data);
    name_to_id[aux]++;
  }

  // Print the map
  /* std::cout << "Mapa de nombres a IDs:" << std::endl;
  for (const auto & pair : name_to_id) {
     std::cout << pair.first << " -> " << pair.second << std::endl;
  }*/

  // Copy the elements of the map to a vector of pairs (key, value)
  std::vector<std::pair<std::string, int>> vec(name_to_id.begin(), name_to_id.end());

  // Sort the vector according to the value of the integer in decreasing order
  std::sort(
    vec.begin(), vec.end(), [](const auto & a, const auto & b) {
      return a.second > b.second;
    });

  // Print the sorted vector of priorities
  std::vector<UseCase> priorities;
  // std::cout << "Priorities:" << std::endl;
  for (const auto & pair : vec) {
    // std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
    auto it = usecase_strings_.find(pair.first);
    if (it != usecase_strings_.end()) {
      priorities.push_back(it->second);
    }
  }
  return priorities;
}