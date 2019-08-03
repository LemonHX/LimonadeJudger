#pragma once
#include "headers.hxx"
#define LOG_ERROR(error_code) LOG_F(ERROR, "Error: " #error_code);
#define ERROR_EXIT(error_code)                                                 \
  {                                                                            \
    LOG_ERROR(error_code);                                                     \
    _result->error = error_code;                                               \
    return;                                                                    \
  }

#include "data_structure.hxx"

#define LOG_LEVEL_FATAL 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

struct sub_precess_monitor {
  int pid;
  int timeout;
};


#define SUB_ERROR_EXIT(error_code)                                             \
  {                                                                            \
    LOG_F(ERROR, "Error: System errno: %s; Internal errno: " #error_code,      \
          strerror(errno));                                                    \
    close_file(input_file);                                                    \
    if (output_file == error_file) {                                           \
      close_file(output_file);                                                 \
    } else {                                                                   \
      close_file(output_file);                                                 \
      close_file(error_file);                                                  \
    }                                                                          \
    raise(SIGUSR1);                                                            \
    exit(EXIT_FAILURE);                                                        \
  }

inline void close_file(FILE *fp) {
  if (fp != NULL) {
    fclose(fp);
  }
}

inline auto kill_pid(pid_t pid) -> int { return kill(pid, SIGKILL); }

inline auto timeout_killer(void *sub_precess_monitor) -> void * {
  // this is a new thread, kill the process if timeout
  pid_t pid = ((struct sub_precess_monitor *)sub_precess_monitor)->pid;
  int timeout = ((struct sub_precess_monitor *)sub_precess_monitor)->timeout;
  // On success, pthread_detach() returns 0; on error, it returns an error
  // number.
  if (pthread_detach(pthread_self()) != 0) {
    kill_pid(pid);
    return NULL;
  }
  // usleep can't be used, for time args must < 1000ms
  // this may sleep longer that expected, but we will have a check at the end
  if (sleep((unsigned int)((timeout + 1000) / 1000)) != 0) {
    kill_pid(pid);
    return NULL;
  }
  if (kill_pid(pid) != 0) {
    return NULL;
  }
  return NULL;
}

inline auto redirect_io(struct Config *_config, FILE *input_file,
                        FILE *output_file, FILE *error_file) -> void {

  if (_config->input_path.c_str() != NULL) {
    input_file = fopen(_config->input_path.c_str(), "r");
    if (input_file == NULL) {
      SUB_ERROR_EXIT(DUP2_FAILED);
    }
    if (dup2(fileno(input_file), fileno(stdin)) == -1) {
      // todo log
      SUB_ERROR_EXIT(DUP2_FAILED);
    }
  }

  if (_config->output_path.c_str() != NULL) {
    output_file = fopen(_config->output_path.c_str(), "w");
    if (output_file == NULL) {
      SUB_ERROR_EXIT(DUP2_FAILED);
    }
    // stdout -> file
    if (dup2(fileno(output_file), fileno(stdout)) == -1) {
      SUB_ERROR_EXIT(DUP2_FAILED);
    }
  }

  if (_config->error_path.c_str() != NULL) {
    if (_config->output_path.c_str() != NULL &&
        _config->output_path == _config->error_path) {
      error_file = output_file;
    } else {
      error_file = fopen(_config->error_path.c_str(), "w");
      if (error_file == NULL) {
        // todo log
        SUB_ERROR_EXIT(DUP2_FAILED);
      }
    }
    // stderr -> file
    if (dup2(fileno(error_file), fileno(stderr)) == -1) {
      // todo log
      SUB_ERROR_EXIT(DUP2_FAILED);
    }
  }
}

inline auto set_guid(struct Config *_config, FILE *input_file,
                     FILE *output_file, FILE *error_file) -> void {
  // set gid
  gid_t group_list[] = {_config->gid};
  if (_config->gid != -1 &&
      (setgid(_config->gid) == -1 ||
       setgroups(sizeof(group_list) / sizeof(gid_t), group_list) == -1)) {
    SUB_ERROR_EXIT(SETUID_FAILED);
  }

  // set uid
  if (_config->uid != -1 && setuid(_config->uid) == -1) {
    SUB_ERROR_EXIT(SETUID_FAILED);
  }
}

inline auto set_rlimit(struct Config *_config, FILE *input_file,
                       FILE *output_file, FILE *error_file) -> void {
  if (_config->max_stack != UNLIMITED) {
    struct rlimit max_stack;
    max_stack.rlim_cur = max_stack.rlim_max = (rlim_t)(_config->max_stack);
    if (setrlimit(RLIMIT_STACK, &max_stack) != 0) {
      SUB_ERROR_EXIT(SETRLIMIT_FAILED);
    }
  }

  // set memory limit
  // if memory_limit_check_only == 0, we only check memory usage number, because
  // setrlimit(maxrss) will cause some crash issues
  if (_config->memory_limit_check_only == 0) {
    if (_config->max_memory != UNLIMITED) {
      struct rlimit max_memory;
      max_memory.rlim_cur = max_memory.rlim_max =
          (rlim_t)(_config->max_memory) * 2;
      if (setrlimit(RLIMIT_AS, &max_memory) != 0) {
        SUB_ERROR_EXIT(SETRLIMIT_FAILED);
      }
    }
  }

  // set cpu time limit (in seconds)
  if (_config->max_cpu_time != UNLIMITED) {
    struct rlimit max_cpu_time;
    max_cpu_time.rlim_cur = max_cpu_time.rlim_max =
        (rlim_t)((_config->max_cpu_time + 1000) / 1000);
    if (setrlimit(RLIMIT_CPU, &max_cpu_time) != 0) {
      SUB_ERROR_EXIT(SETRLIMIT_FAILED);
    }
  }

  // set max process number limit
  if (_config->max_process_number != UNLIMITED) {
    struct rlimit max_process_number;
    max_process_number.rlim_cur = max_process_number.rlim_max =
        (rlim_t)_config->max_process_number;
    if (setrlimit(RLIMIT_NPROC, &max_process_number) != 0) {
      SUB_ERROR_EXIT(SETRLIMIT_FAILED);
    }
  }

  // set max output size limit
  if (_config->max_output_size != UNLIMITED) {
    struct rlimit max_output_size;
    max_output_size.rlim_cur = max_output_size.rlim_max =
        (rlim_t)_config->max_output_size;
    if (setrlimit(RLIMIT_FSIZE, &max_output_size) != 0) {
      SUB_ERROR_EXIT(SETRLIMIT_FAILED);
    }
  }
}

inline auto load_seccomp(struct Config *_config, FILE *input_file,
                         FILE *output_file,
                         FILE *error_file) -> void { // load seccomp
  if (_config->seccomp_rule_name.c_str() != NULL) {
    if (_config->seccomp_rule_name == "c_cpp") {
      if (c_cpp_seccomp_rules(_config) != SUCCESS) {
        SUB_ERROR_EXIT(LOAD_SECCOMP_FAILED);
      }
    } else if (_config->seccomp_rule_name == "c_cpp_file_io") {
      if (c_cpp_file_io_seccomp_rules(_config) != SUCCESS) {
        SUB_ERROR_EXIT(LOAD_SECCOMP_FAILED);
      }
    } else if (_config->seccomp_rule_name == "general") {
      if (general_seccomp_rules(_config) != SUCCESS) {
        SUB_ERROR_EXIT(LOAD_SECCOMP_FAILED);
      }
    }
    // other rules
    else {
      // rule does not exist
      SUB_ERROR_EXIT(LOAD_SECCOMP_FAILED);
    }
  }
}