#pragma once

#include <memory>
#include <optional>

#include "frame/program_interface.h"

namespace frame::opengl::file {

/**
 * @brief Load from a name (something like "Blur").
 * @param name: Program name.
 * @return A unique pointer to a program interface or an error.
 */
std::unique_ptr<ProgramInterface> LoadProgram(const std::string& name);
/**
 * @brief Load from 2 file names one for vertex and one for fragment.
 * @param name: Program name.
 * @param vertex_file: File containing a vertex shader (full path should be
 * provided).
 * @param fragment_file: File containing a fragment shader (full path should be
 * provided).
 * @return A unique pointer to a program interface or an error.
 */
std::unique_ptr<ProgramInterface> LoadProgram(const std::string& name,
                                              const std::string& vertex_file,
                                              const std::string& fragment_file);

}  // namespace frame::opengl::file
