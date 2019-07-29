#include <stdbool.h>
#include "./c_cpp.cxx"
#include "../data_structure.hxx"


int c_cpp_file_io_seccomp_rules(struct Config *_config) {
    return _c_cpp_seccomp_rules(_config, true);
}
