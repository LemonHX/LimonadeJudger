#pragma once
#include "headers.hxx"
#include "rules/c_cpp.cxx"
#include "rules/c_cpp_file_io.cxx"
#include "rules/general.cxx"
#include "utils.hxx"

inline auto sub_process(struct Config *_config) -> void {
  FILE *input_file = NULL, *output_file = NULL, *error_file = NULL;

  set_rlimit(_config, input_file, output_file, error_file);
  redirect_io(_config, input_file, output_file, error_file);
  set_guid(_config, input_file, output_file, error_file);
  load_seccomp(_config, input_file, output_file, error_file);

  execve(_config->exe_path.c_str(), _config->args, _config->env);
  SUB_ERROR_EXIT(EXECVE_FAILED);
}
