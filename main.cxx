#include "loguru/loguru.hpp"
#include "loguru/loguru.cpp"
#include "exector.hxx"

auto main(int argc, char *argv[])->int {
  auto _config = new Config();
  auto _result = new Result();
  loguru::init(argc,argv);
  loguru::add_file(_config->log_path.c_str(), loguru::Append, loguru::Verbosity_MAX);
  exec_with_restriction(_config, _result);

  printf("{\n"
         "    \"cpu_time\": %d,\n"
         "    \"real_time\": %d,\n"
         "    \"memory\": %ld,\n"
         "    \"signal\": %d,\n"
         "    \"exit_code\": %d,\n"
         "    \"error\": %d,\n"
         "    \"result\": %d\n"
         "}",
         _result->cpu_time, _result->real_time, _result->memory, _result->signal,
         _result->exit_code, _result->error, _result->result);
}
