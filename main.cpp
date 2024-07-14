#ifndef set_WriteToFile
  #define set_WriteToFile 1
#endif

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

void _print(sint32_t fd, const char *format, ...){
  IO_fd_t fd_stdout;
  IO_fd_set(&fd_stdout, fd);
  va_list argv;
  va_start(argv, format);
  IO_vprint(&fd_stdout, format, argv);
  va_end(argv);
}
#define printstderr(...) _print(FD_ERR, ##__VA_ARGS__)
#define printstdout(...) _print(FD_OUT, ##__VA_ARGS__)

struct pile_t{
  struct{
    bool Wmacro_redefined = true;

    /* no any compiler gives error or warning about it. but i want. */
    bool Wmacro_not_defined = false;

    bool Wmacro_incorrect_call = true;
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

  #define BLL_set_prefix DefineDataList
  #define BLL_set_Link 0
  #define BLL_set_Recycle 1
  #define BLL_set_IntegerNR 1
  #define BLL_set_CPP_ConstructDestruct 1
  #define BLL_set_CPP_Node_ConstructDestruct 1
  #define BLL_set_CPP_CopyAtPointerChange 1
  #define BLL_set_AreWeInsideStruct 1
  #define BLL_set_NodeData \
    bool isfunc; \
    std::map<std::string_view, uintptr_t> Inputs; \
    bool va_args; \
    const uint8_t *op; /* output pointer */ \
    const uint8_t *os; /* output size */
  #define BLL_set_type_node uintptr_t
  #include <BLL/BLL.h>
  DefineDataList_t DefineDataList;
  struct DefineDataMap_t : std::map<std::string_view, uintptr_t>{
    void erase(auto i){
      auto o = operator[](i);
      auto &pile = *OFFSETLESS(this, pile_t, DefineDataMap);
      pile.DefineDataList.Recycle(o);
      std::map<std::string_view, uintptr_t>::erase(i);
    }
  }DefineDataMap;

  struct PreprocessorScope_t{
    /* 0 #if, 1 #elif, 2 #else */
    uint8_t Type = 0;

    bool DidRan = false;
  };
  std::vector<PreprocessorScope_t> PreprocessorScope;

  struct DefineStack_t{
    uint8_t *i;

    DefineStack_t(){
      /* TOOD use mmap and reserve after pointer */
      /* so we can get crash for sure instead of corruption */

      i = (uint8_t *)A_resize(NULL, 0x4000);
    }
    ~DefineStack_t(){
      A_resize(i, 0);
    }
  }DefineStack;

  struct expandtrace_data_t{
    /* we do little bit trick here */
    /* DataID can point file or define so */
    /* so if it has biggest bit set, it means file. if not, its define. */
    uintptr_t DataID;

    union{
      struct{
        bool Relative;
        uintptr_t PathSize;
      }file;
      struct{
        uint8_t *pstack;
      }define;
    };

    /* they are same as *DataList[DataID] */
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
    return FileDataList[CurrentExpand.DataID].pragma_once;
  }

  void print_ExpandTrace(){
    /* for stack heap sync */
    ExpandTrace[ExpandTrace.Usage() - 1] = CurrentExpand;

    auto rp = &RelativePaths.back();
    uintptr_t rpsize = rp->size();
    for(uintptr_t i = ExpandTrace.Usage(); i-- > 1;){
      auto &et = ExpandTrace[i];
      if(et.DataID & (uintptr_t)1 << sizeof(uintptr_t) * 8 - 1){
        uintptr_t FileDataID = et.DataID ^ (uintptr_t)1 << sizeof(uintptr_t) * 8 - 1;

        auto &f = FileDataList[FileDataID];
        printstderr(
          "%.*s%.*s:%u\n",
          rpsize,
          rp->c_str(),
          (uintptr_t)f.FileName.size(),
          f.FileName.data(),
          (uintptr_t)et.i - (uintptr_t)f.p
        );
        if(et.file.Relative){
          rpsize -= et.file.PathSize;
        }
        else{
          rp--;
          rpsize = rp->size();
        }
      }
      else{
        if(et.DataID == 0){
          printstderr("inside stack or flat define\n");
        }
        else{
          auto &dd = DefineDataList[et.DataID];
          printstderr("inside function define(");
          uintptr_t pi = 0;
          while(pi != dd.Inputs.size()){
            for(auto const &it : dd.Inputs){
              if(it.second == pi){
                printstderr("%.*s", (uintptr_t)it.first.size(), it.first.data());
                pi++;
                if(pi != dd.Inputs.size()){
                  printstderr(",");
                }
                break;
              }
            }
          }
          printstderr(")\n");
        }
      }
    }
  }
  /* print with info */
  #define printwi(format, ...) \
    printstderr(format "\n", ##__VA_ARGS__); \
    print_ExpandTrace(); \
    printstderr("\n");
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

  void _ExpandTraceFileSet(
    uintptr_t FileDataID,
    bool Relative,
    uintptr_t PathSize
  ){
    CurrentExpand.DataID = FileDataID;
    CurrentExpand.DataID |= (uintptr_t)1 << sizeof(uintptr_t) * 8 - 1;

    CurrentExpand.file.Relative = Relative;
    CurrentExpand.file.PathSize = PathSize;

    auto &fd = FileDataList[FileDataID];
    CurrentExpand.s = &fd.p[fd.s];
    CurrentExpand.i = fd.p;
  }

  void ExpandTraceFileAdd(
    uintptr_t FileDataID,
    bool Relative,
    uintptr_t PathSize
  ){
    ExpandTrace[ExpandTrace.Usage() - 1] = CurrentExpand;

    ExpandTrace.inc();

    _ExpandTraceFileSet(
      FileDataID,
      Relative,
      PathSize
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
      if(EXPECT(i == DefaultInclude.size(), false)){
        errprint_exit("failed to find non relative include \"%s\"", PathName.c_str());
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

      auto p = (uint8_t *)malloc(file_input_size + 1); /* + 1 for endline in end */
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

    ExpandTraceFileAdd(FileDataID, Relative, 0);
  }

  void _ExpandTraceDefineAdd(uintptr_t DataID, const uint8_t *i, uintptr_t size){
    ExpandTrace[ExpandTrace.Usage() - 1] = CurrentExpand;

    ExpandTrace.inc();

    CurrentExpand.DataID = DataID;

    *(uintptr_t *)&CurrentExpand.s = size;
    CurrentExpand.i = i;
  }
  void ExpandTraceDefineAdd(uintptr_t DefineDataID){
    auto &dd = DefineDataList[DefineDataID];
    _ExpandTraceDefineAdd(0, dd.op, 0);
  }
  void ExpandTraceDefineFunctionAdd(uintptr_t DefineDataID){
    auto &dd = DefineDataList[DefineDataID];
    _ExpandTraceDefineAdd(DefineDataID, dd.op, dd.Inputs.size() * sizeof(std::string_view));
    CurrentExpand.define.pstack = DefineStack.i - dd.Inputs.size() * sizeof(std::string_view);
  }

  void _DeexpandFile(){
    if(CurrentExpand.file.Relative){
      auto &rp = RelativePaths.back();
      rp = rp.substr(0, rp.size() - CurrentExpand.file.PathSize);
    }
    else{
      RelativePaths.pop_back();
    }
  }
  void _DeexpandDefine(){
    DefineStack.i -= *(uintptr_t *)&CurrentExpand.s;
  }

  void _Deexpand(){
    ExpandTrace.dec();
    CurrentExpand = ExpandTrace[ExpandTrace.Usage() - 1];
  }

  bool IsLastExpandFile(){
    return !!(CurrentExpand.DataID & (uintptr_t)1 << sizeof(uintptr_t) * 8 - 1);
  }
  bool IsLastExpandDefine(){
    return !IsLastExpandFile();
  }

  void Deexpand(){
    if(IsLastExpandFile()){
      _DeexpandFile();
    }
    else{
      _DeexpandDefine();
    }

    _Deexpand();
  }

  /* get char */
  const uint8_t &gc(){
    return *CurrentExpand.i;
  }

  void ic_unsafe(){
    #if set_WriteToFile
      printstdout("%c", *CurrentExpand.i);
    #endif
    ++CurrentExpand.i;
  }
  void ic_endline(){
    if(IsLastExpandDefine()){
      _DeexpandDefine();
      _Deexpand();
      return;
    }
    else{
      CurrentExpand.i++;
      while(CurrentExpand.i == CurrentExpand.s){
        Deexpand();
      }
    }
  }
  void _ic(){
    #if set_WriteToFile
      printstdout("%c", *CurrentExpand.i);
    #endif
    if(gc() == '\n'){
      ic_endline();
    }
    else{
      CurrentExpand.i++;
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
      printstderr("-h == prints this and exits program.\n");
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
    printstderr("undefined parameter `%s`. -h for help\n", argv[ParameterAt]);
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
          printstderr("parameter should have a -\n");
          return 0;
        }
      }
    }
    if(ParameterAt < argc){
      CallParameter(ParameterAt, argc, argv);
    }
  }

  if(pile.filename.size() == 0){
    printstderr("need something to process\n");
    return 0;
  }

  /* ddid 0 is for stack defines */
  pile.DefineDataList.NewNode();

  pile.FileDataList[pile.FileDataList.NewNode()] = {
    .s = 6,
    .p = (uint8_t *)"\n#eof\n"
  };
  pile._ExpandTraceFileSet(0, true, 0);
  pile.ExpandTrace.inc();

  pile.RelativePaths.push_back({});

  pile.ExpandFile(true, pile.filename);

  return pile.Compile();
}
