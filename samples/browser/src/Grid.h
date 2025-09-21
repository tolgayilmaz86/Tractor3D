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
