#pragma once
#include <fstream>
#include <iostream>

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "sub.hxx"
#include "utils.hxx"

auto exec_with_restriction(struct Config *_config, struct Result *_result) -> void {
  // check whether current user is root
  uid_t uid = getuid();
  if (uid != 0) {
    ERROR_EXIT(ROOT_REQUIRED);
  }

  // check args
  if ((_config->max_cpu_time < 1 && _config->max_cpu_time != UNLIMITED) ||
      (_config->max_real_time < 1 && _config->max_real_time != UNLIMITED) ||
      (_config->max_stack < 1) ||
      (_config->max_memory < 1 && _config->max_memory != UNLIMITED) ||
      (_config->max_process_number < 1 &&
       _config->max_process_number != UNLIMITED) ||
      (_config->max_output_size < 1 && _config->max_output_size != UNLIMITED)) {
    ERROR_EXIT(INVALID_CONFIG);
  }

  // record current time
  struct timeval start, end;
  gettimeofday(&start, NULL);

  pid_t sub_pid = fork();

  // pid < 0 shows clone failed
  if (sub_pid < 0) {

    ERROR_EXIT(FORK_FAILED);

  } else if (sub_pid == 0) {
    // when is forked run the sub_process for limitting the resource and execute
    // the programme
    sub_process(_config);

  } else if (sub_pid > 0) {
    // create new thread to watch the sub process
    pthread_t tid = 0;
    if (_config->max_real_time != UNLIMITED) {
      struct sub_precess_monitor monitor;

      monitor.timeout = _config->max_real_time;
      monitor.pid = sub_pid;
      if (pthread_create(&tid, NULL, timeout_killer, (void *)(&monitor)) != 0) {
        kill_pid(sub_pid);
        ERROR_EXIT(PTHREAD_FAILED);
      }
    }

    int status;
    struct rusage resource_usage;

    // wait for sub process to finish
    // success something | failed -1
    if (wait4(sub_pid, &status, WSTOPPED, &resource_usage) == -1) {
      kill_pid(sub_pid);
      ERROR_EXIT(WAIT_FAILED);
    }
    // get end time
    gettimeofday(&end, NULL);
    _result->real_time = (int)(end.tv_sec * 1000 + end.tv_usec / 1000 -
                               start.tv_sec * 1000 - start.tv_usec / 1000);

    // process exited, we may need to cancel timeout killer thread
    if (_config->max_real_time != UNLIMITED) {
      if (pthread_cancel(tid) != 0) {
        // todo logging
      };
    }

    if (WIFSIGNALED(status) != 0) {
      _result->signal = WTERMSIG(status);
    }

    if (_result->signal == SIGUSR1) {
      _result->result = SYSTEM_ERROR;
    } else {
      _result->exit_code = WEXITSTATUS(status);
      _result->cpu_time = (int)(resource_usage.ru_utime.tv_sec * 1000 +
                                resource_usage.ru_utime.tv_usec / 1000);
      _result->memory = resource_usage.ru_maxrss * 1024;

      if (_result->exit_code != 0) {
        _result->result = RUNTIME_ERROR;
      }

      if (_result->signal == SIGSEGV) {
        // if real mem is bigger than config
        if (_config->max_memory != UNLIMITED &&
            _result->memory > _config->max_memory) {
          _result->result = MEMORY_LIMIT_EXCEEDED;
        } else {
          _result->result = RUNTIME_ERROR;
        }
      } else {
        if (_result->signal != 0) {
          _result->result = RUNTIME_ERROR;
        }
        // if real mem is bigger than config
        if (_config->max_memory != UNLIMITED &&
            _result->memory > _config->max_memory) {
          _result->result = MEMORY_LIMIT_EXCEEDED;
        }
        // if real time is bigger than config
        if (_config->max_real_time != UNLIMITED &&
            _result->real_time > _config->max_real_time) {
          _result->result = REAL_TIME_LIMIT_EXCEEDED;
        }
        // if real cpu time is bigger than config
        if (_config->max_cpu_time != UNLIMITED &&
            _result->cpu_time > _config->max_cpu_time) {
          _result->result = CPU_TIME_LIMIT_EXCEEDED;
        }
      }
    }
    std::ifstream our_out(_config->output_path);
    std::ifstream uni_out(_config->unified_output_path);
    std::string ot((std::istreambuf_iterator<char>(our_out)),
                   (std::istreambuf_iterator<char>()));
    std::string ut((std::istreambuf_iterator<char>(uni_out)),
                   (std::istreambuf_iterator<char>()));

    printf("%s\n====\n%s\n", ot.c_str(), ut.c_str());
    if (ot == ut) {
      _result->result = ACCEPTED;
    }
  }
}
