/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "tractor.h"

using namespace tractor;

static const unsigned int DEFAULT_LINE_COUNT = 81;

/**
 * Creates a new grid mesh.
 *
 * @param lineCount The number of lines in the grid. (Rows or columns). Should be odd.
 *
 * @return A new grid mesh or nullptr if there was an error.
 */
std::shared_ptr<Mesh> createGridMesh(unsigned int lineCount = DEFAULT_LINE_COUNT);

/**
 * Creates a model that contains a new grid mesh.
 *
 * @param lineCount The number of lines in the grid. (Rows or columns). Should be odd.
 *
 * @return A new model containing a grid mesh or nullptr if there was an error.
 */
Model* createGridModel(unsigned int lineCount = DEFAULT_LINE_COUNT);
