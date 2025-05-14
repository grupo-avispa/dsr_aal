// Copyright (c) 2024 Grupo Avispa, DTE, Universidad de Málaga
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

#ifndef ADAPTATIONAGENT__TYPES_HPP_
#define ADAPTATIONAGENT__TYPES_HPP_

#include <string>

enum UseCase { DO_NOTHING, WANDERING, CHARGING, MENU, MUSIC, NEURON_UP, GETME, REMINDER,
  ANNOUNCER, EXPLANATION };

struct personData
{
  std::string identifier;
  std::string commParameters;
  std::string profile;
  std::string activities;
  std::string menu;
  bool neuron;
  bool reminder;
};

#endif  // ADAPTATIONAGENT__TYPES_HPP_
