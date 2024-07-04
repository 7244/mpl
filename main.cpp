#include <WITCH/WITCH.h>
#include <WITCH/IO/IO.h>
#include <WITCH/IO/print.h>
#include <WITCH/FS/FS.h>
#include <WITCH/STR/common/common.h>
#include <WITCH/PR/PR.h>

/* realpath */
#include <stdlib.h>

#include <string>
#include <string_view>
#include <vector>
#include <map>

void print(const char *format, ...){
  IO_fd_t fd_stdout;
  IO_fd_set(&fd_stdout, FD_ERR);
  va_list argv;
  va_start(argv, format);
  IO_vprint(&fd_stdout, format, argv);
  va_end(argv);
}

struct pile_t{
  struct{
    bool Wmacro_redefined = true;
  }settings;

  std::string filename;

  struct RawData_t{
    bool pragma_once = false;

    uintptr_t s;
    uint8_t *p;
  };
  std::vector<RawData_t> FileDataVector;
  std::map<std::string, uintptr_t> FileDataMap; /* points FileDataVector */

  std::vector<std::string> RelativePaths;

  struct Define_t{
    bool isfunc;
    std::vector<std::string> Inputs;
    bool va_args;
    std::string Output;
  };
  std::map<std::string, Define_t> DefineMap;

  struct expandtrace_t{
    bool Relative;

    uintptr_t PathSize;
    std::string_view FileName;

    uintptr_t FileDataVectorID;

    uintptr_t i = 0;
    uintptr_t LineIndex = 0;
  };
  std::vector<expandtrace_t> expandtrace;

  bool &GetPragmaOnce(){
    return FileDataVector[expandtrace[expandtrace.size() - 1].FileDataVectorID].pragma_once;
  }

  /* print with info */
  #define printwi(format, ...) \
    print( \
      format " %.*s:%lu\n", \
      ##__VA_ARGS__, \
      (uintptr_t)expandtrace[expandtrace.size() - 1].FileName.size(), \
      &expandtrace[expandtrace.size() - 1].FileName[0], \
      (uint32_t)expandtrace[expandtrace.size() - 1].LineIndex \
    );

  std::string realpath(std::string p){
    auto v = ::realpath(p.c_str(), NULL);
    std::string r = v;
    free(v);
    return r;
  }

  std::vector<std::string> DefaultInclude = {
    "/usr/include/",
    "/usr/local/include/"
  };

  void ExpandFile(bool Relative, std::string PathName){
    if(!Relative){
      RelativePaths.push_back({});
      uintptr_t i = 0;
      for(; i < DefaultInclude.size(); i++){
        if(IO_IsPathExists((DefaultInclude[i] + PathName).c_str())){
          break;
        }
      }
      if(i == DefaultInclude.size()){
        printwi("failed to find non relative include");
      }

      RelativePaths[RelativePaths.size() - 1].append(DefaultInclude[i]);
    }

    std::string FileName;

    {
      sintptr_t i = PathName.size();
      while(i--){
        if(PathName[i] == '/'){
          break;
        }
      }

      /* TODO check if slash has something before itself */

      FileName = PathName.substr(i + 1, PathName.size() - i - 1);
      if(i > 0){
        RelativePaths[RelativePaths.size() - 1].append(PathName, 0, i + 1);
      }
    }

    std::string abspath = RelativePaths[RelativePaths.size() - 1] + FileName;

    uintptr_t FileDataVectorID;

    auto it = FileDataMap.find(abspath);
    if(it == FileDataMap.end()){
      FS_file_t file_input;
      if(FS_file_open(
        abspath.c_str(),
        &file_input,
        O_RDONLY
      ) != 0){
        printwi("failed to open file %s", abspath.c_str());
        PR_exit(1);
      }

      IO_fd_t fd_input;
      FS_file_getfd(&file_input, &fd_input);

      IO_stat_t st;
      IO_fstat(&fd_input, &st);

      auto file_input_size = IO_stat_GetSizeInBytes(&st);

      FileDataVector.push_back({
        .s = (uintptr_t)file_input_size,
        .p = (uint8_t *)malloc(file_input_size)
      });
      FileDataVectorID = FileDataVector.size() - 1;

      if(FS_file_read(
        &file_input,
        FileDataVector[FileDataVectorID].p,
        file_input_size
      ) != file_input_size){
        __abort();
      }

      FS_file_close(&file_input);

      FileDataMap[abspath] = FileDataVectorID;
    }
    else{
      #if 0 // i hate cpp
      FileDataVectorID = FileDataMap[it];
      #else
      FileDataVectorID = FileDataMap[abspath];
      #endif
    }

    expandtrace.push_back({
      .Relative = Relative,
      .PathSize = 0,
      .FileName = FileName,
      .FileDataVectorID = FileDataVectorID
    });
  }
  void DeexpandFile(){
    auto &et = expandtrace[expandtrace.size() - 1];

    if(et.Relative){
      auto &rp = RelativePaths[RelativePaths.size() - 1];
      rp = rp.substr(0, rp.size() - et.PathSize);
    }
    else if(et.Relative){
      RelativePaths.pop_back();
    }

    expandtrace.pop_back();
  }

  /* get char */
  auto gc(){
    auto &et = expandtrace[expandtrace.size() - 1];
    auto fdvid = et.FileDataVectorID;
    return FileDataVector[fdvid].p[et.i];
  }
  /* iterate char */
  void _ic(){
    auto &et = expandtrace[expandtrace.size() - 1];
    et.i++;

    auto fdvid = et.FileDataVectorID;

    while(expandtrace[expandtrace.size() - 1].i == FileDataVector[fdvid].s){
      DeexpandFile();
    }
  }
  /* ignore basic stuff */
  auto ibs(){
    if(gc() != '\\'){
      return;
    }
    else if(gc() == '\r'){
      _ic();
    }

    _ic();
    if(gc() == '\n'){
      _ic();
      ibs();
      return;
    }
    else if(gc() != '\r'){
      __abort(); /* TOOD add printwi */
    }

    _ic();
    if(gc() != '\n'){
      __abort(); /* TOOD add printwi */
    }

    _ic();

    ibs();
  }
  void ic(){
    _ic();
    ibs();
  }

  #include "Compile.h"
}pile;

struct ProgramParameter_t{
  std::string name;
  std::function<void(uint32_t, const char **)> func;
};
ProgramParameter_t ProgramParameters[] = {
  {
    "h",
    [](uint32_t argCount, const char **arg){
      print("-h == prints this and exits program.\n");
      PR_exit(0);
    }
  },
  {
    "Wmacro-redefined",
    [](uint32_t argCount, const char **arg){
      if(argCount != 0){
        __abort();
      }
      pile.settings.Wmacro_redefined = true;
    }
  },
  {
    "Wno-macro-redefined",
    [](uint32_t argCount, const char **arg){
      if(argCount != 0){
        __abort();
      }
      pile.settings.Wmacro_redefined = false;
    }
  },
  {
    "i",
    [](uint32_t argCount, const char **arg){
      if(argCount != 1){
        __abort();
      }
      pile.filename = std::string(arg[0]);
    }
  },
  {
    "I",
    [](uint32_t argCount, const char **arg){
      if(argCount != 1){
        __abort();
      }
      pile.DefaultInclude.push_back(std::string(arg[0]));
    }
  }
};
constexpr uint32_t ProgramParameterCount = sizeof(ProgramParameters) / sizeof(ProgramParameters[0]);

void CallParameter(uint32_t ParameterAt, uint32_t iarg, const char **argv){
  uint32_t is = 0;
  for(; is < ProgramParameterCount; is++){
    if(std::string(&argv[ParameterAt][1]) == ProgramParameters[is].name){
      break;
    }
  }
  if(is == ProgramParameterCount){
    print("undefined parameter `%s`. -h for help\n", argv[ParameterAt]);
    PR_exit(0);
  }
  ProgramParameters[is].func(iarg - ParameterAt - 1, &argv[ParameterAt + 1]);
}

int main(int argc, const char **argv){
  {
    uint32_t ParameterAt = 1;
    for(uint32_t iarg = 1; iarg < argc; iarg++){
      if(argv[iarg][0] == '-'){
        if(ParameterAt == iarg){
          continue;
        }
        CallParameter(ParameterAt, iarg, argv);
        ParameterAt = iarg;
      }
      else{
        if(ParameterAt == iarg){
          print("parameter should have a -\n");
          return 0;
        }
      }
    }
    if(ParameterAt < argc){
      CallParameter(ParameterAt, argc, argv);
    }
  }

  if(pile.filename.size() == 0){
    print("need something to process\n");
    return 0;
  }

  pile.FileDataVector.push_back({
    .s = 5,
    .p = (uint8_t *)"\n#eof"
  });
  pile.expandtrace.push_back({
    .Relative = true,
    .PathSize = 0,
    .FileName = "",
    .FileDataVectorID = 0
  });

  pile.RelativePaths.push_back({});

  pile.ExpandFile(true, pile.filename);

  pile.ibs();

  return pile.Compile();
}
