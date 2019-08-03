#include "headers.hxx"
#include "exector.hxx"

inline void split(const std::string &s, std::vector<std::string> &sv,
                  const char delim = ' ') {
  sv.clear();
  std::istringstream iss(s);
  std::string temp;

  while (std::getline(iss, temp, delim)) {
    sv.emplace_back(std::move(temp));
  }

  return;
}
inline auto get_files_from_dir_name(std::string cate_dir)
    -> std::vector<std::string> {
  DIR *dir;
  struct dirent *ptr;
  auto files = std::vector<std::string>();
  if ((dir = opendir(cate_dir.c_str())) == NULL) {
    perror("Open dir error...");
    exit(1);
  }

  while ((ptr = readdir(dir)) != NULL) {
    auto tmp_split = std::vector<std::string>();
    if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
      continue;
    else if (ptr->d_type == 8) {
      split(std::string(ptr->d_name), tmp_split, '.');
      files.push_back(tmp_split[0]);
    } else if (ptr->d_type == 4) {
      LOG_F(ERROR, "nested dir not allowed");
    }
  }
  closedir(dir);
  sort(files.begin(), files.end());
  return files;
}

auto main(int argc, char *argv[]) -> int {
  auto ins_dir = "./ins/";
  auto uouts_dir = "./uouts/";
  auto outs_dir = "./outs/";
  auto ress_dir = "./ress/";

  auto _config = new Config();
  auto _result = new Result();
  loguru::init(argc, argv);
  loguru::add_file(_config->log_path.c_str(), loguru::Append,
                   loguru::Verbosity_MAX);
  auto ins = get_files_from_dir_name(ins_dir);
  #pragma omp parallel for
  for (unsigned int i = 0; i < ins.size(); ++i) {
    printf("%d\n",i);
    _config->input_path = ins_dir + ins[i] + ".in";
    _config->output_path = outs_dir + ins[i] + ".out";
    _config->unified_output_path = uouts_dir + ins[i] + ".out";
    LOG_F(INFO, "using file in:%s ; out:%s ; uout:%s",
          _config->input_path.c_str(), _config->output_path.c_str(),
          _config->unified_output_path.c_str());
    exec_with_restriction(_config, _result);
    FILE *out_res = fopen((ress_dir + ins[i] + ".res").c_str(), "w");
    fprintf(out_res,
            "{\n"
            "    \"cpu_time\": %d,\n"
            "    \"real_time\": %d,\n"
            "    \"memory\": %ld,\n"
            "    \"signal\": %d,\n"
            "    \"exit_code\": %d,\n"
            "    \"error\": %d,\n"
            "    \"result\": %d\n"
            "}",
            _result->cpu_time, _result->real_time, _result->memory,
            _result->signal, _result->exit_code, _result->error,
            _result->result);
#ifdef PRINTRES
    printf("{\n"
           "    \"cpu_time\": %d,\n"
           "    \"real_time\": %d,\n"
           "    \"memory\": %ld,\n"
           "    \"signal\": %d,\n"
           "    \"exit_code\": %d,\n"
           "    \"error\": %d,\n"
           "    \"result\": %d\n"
           "}",
           _result->cpu_time, _result->real_time, _result->memory,
           _result->signal, _result->exit_code, _result->error,
           _result->result);
#endif
    close_file(out_res);
  }
}
