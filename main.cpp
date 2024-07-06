#include <WITCH/WITCH.h>
#include <WITCH/IO/IO.h>
#include <WITCH/IO/print.h>
#include <WITCH/FS/FS.h>
#include <WITCH/STR/common/common.h>
#include <WITCH/PR/PR.h>
#include <WITCH/A/A.h>

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
  struct StringSize_t{
    uint32_t s; /* TOOD hardcoded type */
    uint8_t *p;

    ~StringSize_t(){
      if(s){
        A_resize(p, 0);
      }
    }
  };

  void AddVoidToString(std::string &str, uintptr_t s, const void *v){
    auto p = (const uint8_t *)v;
    auto stop = &p[s];
    while(p != stop){
      str += *p++;
    }
  }

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

  struct PreprocessorScope_t{
    /* 0 #if, 1 #elif, 2 #else */
    uint8_t Type = 0;

    bool Condition;
  };
  std::vector<PreprocessorScope_t> PreprocessorScope;

  struct expandtrace_data_t{
    bool Relative;

    uintptr_t PathSize;

    StringSize_t FileName;

    uintptr_t FileDataVectorID;

    /* they are same as FileDataVector[FileDataVectorID] */
    /* they are here for performance reasons */
    const uint8_t *s;
    const uint8_t *i;

    uintptr_t LineIndex;
  };
  expandtrace_data_t CurrentExpand;
  #define BLL_set_prefix ExpandTrace
  #define BLL_set_Link 0
  #define BLL_set_Recycle 0
  #define BLL_set_IntegerNR 1
  #define BLL_set_CPP_ConstructDestruct 1
  #define BLL_set_CPP_Node_ConstructDestruct 1
  #define BLL_set_AreWeInsideStruct 1
  #define BLL_set_NodeDataType expandtrace_data_t
  #define BLL_set_type_node uintptr_t
  #include <BLL/BLL.h>
  ExpandTrace_t ExpandTrace;

  bool &GetPragmaOnce(){
    return FileDataVector[CurrentExpand.FileDataVectorID].pragma_once;
  }

  /* print with info */
  #define printwi(format, ...) \
    print( \
      format " %.*s:%lu\n", \
      ##__VA_ARGS__, \
      (uintptr_t)CurrentExpand.FileName.s, \
      CurrentExpand.FileName.p, \
      (uint32_t)CurrentExpand.LineIndex \
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

  void _ExpandTraceSet(
    bool Relative,
    uintptr_t PathSize,
    uintptr_t FileNameSize,
    uint8_t *FileName,
    uintptr_t FileDataVectorID
  ){
    CurrentExpand.Relative = Relative;
    CurrentExpand.PathSize = PathSize;
    CurrentExpand.FileName.s = FileNameSize;
    CurrentExpand.FileName.p = FileName;
    CurrentExpand.FileDataVectorID = FileDataVectorID;

    auto &fd = FileDataVector[FileDataVectorID];
    CurrentExpand.s = &fd.p[fd.s];
    CurrentExpand.i = fd.p;

    CurrentExpand.LineIndex = 0;
  }

  void ExpandTraceAdd(
    bool Relative,
    uintptr_t PathSize,
    uintptr_t FileNameSize,
    uint8_t *FileName,
    uintptr_t FileDataVectorID
  ){
    ExpandTrace[ExpandTrace.Usage() - 1] = CurrentExpand;

    ExpandTrace.inc();

    _ExpandTraceSet(
      Relative,
      PathSize,
      FileNameSize,
      FileName,
      FileDataVectorID
    );
  }

  void ExpandFile(bool Relative, const std::string &PathName){
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

    uintptr_t FileNameSize;
    uint8_t *FileName;

    {
      sintptr_t i = PathName.size();
      while(i--){
        if(PathName[i] == '/'){
          break;
        }
      }

      /* TODO check if slash has something before itself */

      FileNameSize = PathName.size() - i - 1;
      FileName = (uint8_t *)A_resize(NULL, FileNameSize);
      __MemoryCopy(&PathName[i + 1], FileName, FileNameSize);
      if(i > 0){
        RelativePaths[RelativePaths.size() - 1].append(PathName, 0, i + 1);
      }
    }

    std::string abspath = RelativePaths[RelativePaths.size() - 1];
    AddVoidToString(abspath, FileNameSize, FileName);

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
        .s = (uintptr_t)file_input_size + 1,
        .p = (uint8_t *)malloc(file_input_size + 1)
      });
      FileDataVectorID = FileDataVector.size() - 1;

      if(FS_file_read(
        &file_input,
        FileDataVector[FileDataVectorID].p,
        file_input_size
      ) != file_input_size){
        __abort();
      }

      FileDataVector[FileDataVectorID].p[file_input_size] = '\n';

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

    ExpandTraceAdd(Relative, 0, FileNameSize, FileName, FileDataVectorID);
  }
  void DeexpandFile(){
    if(CurrentExpand.Relative){
      auto &rp = RelativePaths[RelativePaths.size() - 1];
      rp = rp.substr(0, rp.size() - CurrentExpand.PathSize);
    }
    else if(CurrentExpand.Relative){
      RelativePaths.pop_back();
    }

    ExpandTrace.dec();
    CurrentExpand = ExpandTrace[ExpandTrace.Usage() - 1];
  }

  /* get char */
  const uint8_t &gc(){
    return *CurrentExpand.i;
  }

  void ic_unsafe(){
    CurrentExpand.i++;

    while(EXPECT(gc() == '\\', false)){
      CurrentExpand.i++;
      if(gc() != '\n'){
        return;
      }
      CurrentExpand.i++;
    }
  }
  void _ic(){
    CurrentExpand.i++;

    while(CurrentExpand.i == CurrentExpand.s){
      DeexpandFile();
    }
  }
  /* ignore basic stuff */
  void ibs(){
    uint8_t c;
    while((c = gc()) == '\r' || c == '\\'){
      _ic();
      if(c == '\\'){
        if(gc() != '\n'){
          return;
        }
        _ic();
      }
    }
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
    .s = 6,
    .p = (uint8_t *)"\n#eof\n"
  });
  pile._ExpandTraceSet(true, 0, 0, NULL, 0);
  pile.ExpandTrace.inc();

  pile.RelativePaths.push_back({});

  pile.ExpandFile(true, pile.filename);

  pile.ibs();

  return pile.Compile();
}
