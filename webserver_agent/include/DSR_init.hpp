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

#ifndef DSR_init_hpp
#define DSR_init_hpp
#include "DSR_interface.hpp"

// It is mandatory to create this object here as an extern because both 
// webServerAgent and WSListener need acces to the graph whlile the graph must be initialized on
// webServerAgent who also needs the WSListener include. This mutual inclusion might case a circular
// dependency in the program

extern DSR_interface my_DSR;

#endif // DSR_init_hpp