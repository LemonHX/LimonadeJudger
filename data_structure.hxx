#pragma once
#include "headers.hxx"
#define ARGS_MAX_NUMBER 256
#define ENV_MAX_NUMBER 256
#define UNLIMITED -1
/**
 * Error codes
 */
enum {
  SUCCESS = 0,
  INVALID_CONFIG = -1,
  FORK_FAILED = -2,
  PTHREAD_FAILED = -3,
  WAIT_FAILED = -4,
  ROOT_REQUIRED = -5,
  LOAD_SECCOMP_FAILED = -6,
  SETRLIMIT_FAILED = -7,
  DUP2_FAILED = -8,
  SETUID_FAILED = -9,
  EXECVE_FAILED = -10,
  SPJ_ERROR = -11
};

/**
 * Result codes
 */
enum {
  WRONG_ANSWER = -1,
  ACCEPTED = 0,
  CPU_TIME_LIMIT_EXCEEDED = 1,
  REAL_TIME_LIMIT_EXCEEDED = 2,
  MEMORY_LIMIT_EXCEEDED = 3,
  RUNTIME_ERROR = 4,
  SYSTEM_ERROR = 5
};

struct Config {
public:
  int max_cpu_time;
  int max_real_time;
  long max_memory;
  long max_stack;
  int max_process_number;
  long max_output_size;
  int memory_limit_check_only;
  std::string exe_path;
  std::string input_path;
  std::string output_path;
  std::string unified_output_path;
  std::string error_path;
  char *args[ARGS_MAX_NUMBER];
  char *env[ENV_MAX_NUMBER];
  std::string log_path;
  std::string seccomp_rule_name;
  uid_t uid;
  gid_t gid;

  /**
   * default config using unlimited resources and using genenral seccomp rule
   */
  inline Config() {
    max_cpu_time = UNLIMITED;
    max_real_time = UNLIMITED;
    max_memory = UNLIMITED;
    max_stack = 1024000000;
    max_process_number = 1;
    max_output_size = UNLIMITED;
    memory_limit_check_only = UNLIMITED;
    exe_path = "./a.out";
    input_path = "./in";
    output_path = "./out";
    unified_output_path = "./uout";
    error_path = "./error";
    log_path = "./log";
    seccomp_rule_name = "general";
    uid = 0;
    gid = 0;
  }
};
/**
 * will be britty printed with json format
 */
struct Result {
public:
  int cpu_time;
  int real_time;
  long memory;
  int signal;
  int exit_code;
  int error;
  int result;
  inline Result() {
    result = WRONG_ANSWER;
    error = SUCCESS;
    cpu_time = real_time = signal = exit_code = memory = 0;
  }
};
