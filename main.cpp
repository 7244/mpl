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
  struct{
    bool Wmacro_redefined = true;

    /* no any compiler gives error or warning about it. but i want. */
    bool Wmacro_not_defined = false;
  }settings;

  std::string filename;

  #define BLL_set_prefix FileDataList
  #define BLL_set_Link 0
  #define BLL_set_Recycle 0
  #define BLL_set_IntegerNR 1
  #define BLL_set_CPP_ConstructDestruct 1
  #define BLL_set_CPP_Node_ConstructDestruct 1
  #define BLL_set_CPP_CopyAtPointerChange 1
  #define BLL_set_AreWeInsideStruct 1
  #define BLL_set_NodeData \
    bool pragma_once = false; \
    uintptr_t s; \
    uint8_t *p; \
    std::string FileName;
  #define BLL_set_type_node uintptr_t
  #include <BLL/BLL.h>
  FileDataList_t FileDataList;
  std::map<std::string, uintptr_t> FileDataMap; /* points FileDataList */

  std::vector<std::string> RelativePaths;

  struct Define_t{
    bool isfunc;
    std::vector<std::string> Inputs;
    bool va_args;
    std::string Output;
  };
  std::map<std::string_view, Define_t> DefineMap;

  struct PreprocessorScope_t{
    /* 0 #if, 1 #elif, 2 #else */
    uint8_t Type = 0;

    bool DidRan = 0;
  };
  std::vector<PreprocessorScope_t> PreprocessorScope;

  struct expandtrace_data_t{
    bool Relative;

    uintptr_t PathSize;

    uintptr_t FileDataID;

    /* they are same as FileDataList[FileDataID] */
    /* they are here for performance reasons */
    const uint8_t *s;
    const uint8_t *i;
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
    return FileDataList[CurrentExpand.FileDataID].pragma_once;
  }

  void print_ExpandTrace(){
    /* for stack heap sync */
    ExpandTrace[ExpandTrace.Usage() - 1] = CurrentExpand;

    auto rp = &RelativePaths.back();
    uintptr_t rpsize = rp->size();
    for(uintptr_t i = ExpandTrace.Usage(); i-- > 1;){
      auto &et = ExpandTrace[i];
      auto &f = FileDataList[et.FileDataID];
      print(
        "%.*s%.*s:%u\n",
        rpsize,
        rp->c_str(),
        (uintptr_t)f.FileName.size(),
        f.FileName.data(),
        (uintptr_t)et.i - (uintptr_t)f.p
      );
      if(et.Relative){
        rpsize -= et.PathSize;
      }
      else{
        rp--;
        rpsize = rp->size();
      }
    }
  }
  /* print with info */
  #define printwi(format, ...) \
    print(format "\n", ##__VA_ARGS__); \
    print_ExpandTrace(); \
    print("\n");
  #define errprint_exit(...) \
    printwi(__VA_ARGS__) \
    PR_exit(1); \
    __unreachable();

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
    uintptr_t FileDataID
  ){
    CurrentExpand.Relative = Relative;
    CurrentExpand.PathSize = PathSize;
    CurrentExpand.FileDataID = FileDataID;

    auto &fd = FileDataList[FileDataID];
    CurrentExpand.s = &fd.p[fd.s];
    CurrentExpand.i = fd.p;
  }

  void ExpandTraceAdd(
    bool Relative,
    uintptr_t PathSize,
    uintptr_t FileDataID
  ){
    ExpandTrace[ExpandTrace.Usage() - 1] = CurrentExpand;

    ExpandTrace.inc();

    _ExpandTraceSet(
      Relative,
      PathSize,
      FileDataID
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

      RelativePaths.back().append(DefaultInclude[i]);
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

      FileName = std::string((const char *)&PathName[i + 1], PathName.size() - i - 1);
      if(i > 0){
        RelativePaths.back().append(PathName, 0, i + 1);
      }
    }

    std::string abspath = RelativePaths.back() + FileName;

    uintptr_t FileDataID;

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

      auto p = (uint8_t *)malloc(file_input_size + 1); /* + 1 for endline in end*/
      uintptr_t p_size = 0;
      {
        auto tp = (uint8_t *)malloc(file_input_size);

        if(FS_file_read(
          &file_input,
          tp,
          file_input_size
        ) != file_input_size){
          __abort();
        }

        FS_file_close(&file_input);

        if(file_input_size){
          #include "SimplifyFile1.h"
        }

        free(tp);
      }

      if(!p_size || p[p_size - 1] != '\n'){
        p[p_size++] = '\n';
      }
      p = (uint8_t *)realloc(p, p_size);

      FileDataList[FileDataList.NewNode()] = {
        .s = p_size,
        .p = p,
        .FileName = FileName
      };
      FileDataID = FileDataList.Usage() - 1;

      FileDataMap[abspath] = FileDataID;
    }
    else{
      FileDataID = it->second;
    }

    ExpandTraceAdd(Relative, 0, FileDataID);
  }
  void DeexpandFile(){
    if(CurrentExpand.Relative){
      auto &rp = RelativePaths.back();
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
    ++CurrentExpand.i;
  }
  void _ic(){
    CurrentExpand.i++;

    while(CurrentExpand.i == CurrentExpand.s){
      DeexpandFile();
    }
  }
  void ic(){
    _ic();
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

  pile.FileDataList[pile.FileDataList.NewNode()] = {
    .s = 6,
    .p = (uint8_t *)"\n#eof\n"
  };
  pile._ExpandTraceSet(true, 0, 0);
  pile.ExpandTrace.inc();

  pile.RelativePaths.push_back({});

  pile.ExpandFile(true, pile.filename);

  return pile.Compile();
}
