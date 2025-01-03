#ifndef set_LineInformationPlus
  #define set_LineInformationPlus 1
#endif

#ifndef set_StoreTrace
  #define set_StoreTrace 1
#endif

#ifndef set_WriteToFile
  #define set_WriteToFile 0
#endif

#ifndef set_support_c99
  #define set_support_c99 1
#endif
#ifndef set_forcerules_c99
  #define set_forcerules_c99 1
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
/* for process main's parameters */
#include <functional>

#ifndef SWAP
  #define SWAP(p0, p1) std::swap(p0, p1)
#endif

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

  /* INFO rdata stands for raw data, sdata stands for simplified data */
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
    const uint8_t *rdatabegin; \
    const uint8_t *rdataend; \
    const uint8_t *sdatabegin; \
    const uint8_t *sdataend; \
    std::string FullPath; \
    uintptr_t FileNameSize;
  #define BLL_set_type_node uintptr_t
  #include <BLL/BLL.h>
  FileDataList_t FileDataList;
  std::map<std::string, uintptr_t> FileDataMap; /* points FileDataList */

  #if set_StoreTrace
    /* used for store expand trace */
    #define BLL_set_prefix DataLink
    #define BLL_set_Link 0
    #define BLL_set_CPP_ConstructDestruct 1
    #define BLL_set_AreWeInsideStruct 1
    #define BLL_set_NodeData \
      DataLink_NodeReference_t dlid; \
      uintptr_t DataID; \
      const uint8_t *Offset;
    #define BLL_set_type_node uintptr_t
    #include <BLL/BLL.h>
    DataLink_t DataLink;
  #endif

  std::string RelativePaths;
  std::vector<uintptr_t> RelativePathsSize;

  struct DefineDataType_t{
    bool isfunc;
    std::map<std::string_view, uintptr_t> Inputs;
    bool va_args;
    const uint8_t *op; /* output pointer */
    const uint8_t *os; /* output size */
    #if set_StoreTrace
      uintptr_t TraceCount;
      DataLink_t::nr_t DataLinkID;
    #endif

    void destructor(pile_t &pile){
      DataLink_t::nr_t dlid = DataLinkID;
      for(auto tc = TraceCount; tc--;){
        auto t = pile.DataLink[dlid].dlid;
        pile.DataLink.Recycle(dlid);
        dlid = t;
      }
    }
  };
  #define BLL_set_prefix DefineDataList
  #define BLL_set_Link 0
  #define BLL_set_Recycle 1
  #define BLL_set_IntegerNR 1
  #define BLL_set_CPP_ConstructDestruct 1
  #define BLL_set_CPP_Node_ConstructDestruct 1
  #define BLL_set_CPP_CopyAtPointerChange 1
  #define BLL_set_AreWeInsideStruct 1
  #define BLL_set_NodeDataType DefineDataType_t
  #define BLL_set_type_node uintptr_t
  #include <BLL/BLL.h>
  DefineDataList_t DefineDataList;
  struct DefineDataMap_t : std::map<std::string_view, uintptr_t>{
    void erase(auto i){
      auto o = operator[](i);
      auto &pile = *OFFSETLESS(this, pile_t, DefineDataMap);
      pile.DefineDataList[o].destructor(pile);
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
  #define BLL_set_AreWeInsideStruct 1
  #define BLL_set_NodeDataType expandtrace_data_t
  #define BLL_set_type_node uintptr_t
  #include <BLL/BLL.h>
  ExpandTrace_t ExpandTrace;

  bool &GetPragmaOnce(){
    return FileDataList[CurrentExpand.DataID].pragma_once;
  }

  void GetInfoFromFile(
    const uint8_t * const coffset, /* current offset */
    uintptr_t FileDataID,
    uintptr_t &Line
  ){
    auto &f = FileDataList[FileDataID];

    const uint8_t *rdata = f.rdatabegin;
    const uint8_t *rdataend = f.rdataend;
    const uint8_t *sdatabegin = f.sdatabegin;

    #define set_SimplifyFile_Info
    #include "SimplifyFile1.h"

    Line += set_LineInformationPlus;
  }

  /* syncs stack and heap expand */
  void ExpandSync(){
    ExpandTrace[ExpandTrace.Usage() - 1] = CurrentExpand;
  }

  void print_TraceDataID(uintptr_t DataID, const uint8_t * const Offset){
    if(DataID & (uintptr_t)1 << sizeof(uintptr_t) * 8 - 1){
      uintptr_t FileDataID = DataID ^ (uintptr_t)1 << sizeof(uintptr_t) * 8 - 1;
      auto &f = FileDataList[FileDataID];

      uintptr_t LineIndex;
      GetInfoFromFile(Offset, FileDataID, LineIndex);

      printstderr(
        "In file included from %s:%u\n",
        f.FullPath.c_str(),
        LineIndex
      );
    }
    else{
      if(DataID == 0){
        printstderr("inside stack or flat define\n");
      }
      else{
        auto &dd = DefineDataList[DataID];
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
  void print_TraceDataLink(uintptr_t TraceCount, DataLink_t::nr_t dlid){
    while(TraceCount--){
      print_TraceDataID(DataLink[dlid].DataID, DataLink[dlid].Offset);
      dlid = DataLink[dlid].dlid;
    }
  }
  void print_ExpandTrace(){
    ExpandSync();

    for(uintptr_t i = ExpandTrace.Usage(); i-- > 1;){
      auto &et = ExpandTrace[i];
      print_TraceDataID(
        et.DataID,
        et.i
      );
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

  struct TypeData_t{
    uintptr_t refc = 0;
  };
  #define BLL_set_prefix TypeList
  #define BLL_set_Link 0
  #define BLL_set_Recycle 1
  #define BLL_set_IntegerNR 1
  #define BLL_set_CPP_ConstructDestruct 1
  #define BLL_set_CPP_Node_ConstructDestruct 1
  #define BLL_set_AreWeInsideStruct 1
  #define BLL_set_NodeDataType TypeData_t
  #define BLL_set_type_node uintptr_t
  #include <BLL/BLL.h>
  TypeList_t TypeList;

  struct Identifier_t{
    enum class Type_t : uintptr_t{
      Type,
      Function,
      Label,
      Variable
    };
    Type_t Type;
    union{
      TypeList_t::nr_t TypeID;
    };
  };
  std::map<std::string, Identifier_t> IdentifierMap;
  std::map<std::string, TypeList_t::nr_t> StructIdentifierMap;

  Identifier_t *IdentifierMapGet(auto p){
    auto it = IdentifierMap.find(std::string(p));
    if(it == IdentifierMap.end()){
      return NULL;
    }
    return &it->second;
  }

  void TypeReference(TypeList_t::nr_t id){
    TypeList[id].refc++;
  }
  void TypeDereference(TypeList_t::nr_t id){
    TypeList[id].refc--;
    if(TypeList[id].refc == 0){
      __abort();
    }
  }

  enum class ScopeEnum : uintptr_t{
    Global,
    Type
  };
  struct ScopeData_t{
    ScopeEnum se;
    union{
      TypeList_t::nr_t TypeID;
    };

    enum class UnitEnum : uintptr_t{
      Typedef,
      Type,
      UnknownIdentifier
    };
    struct UnitData_Type_t{
      TypeList_t::nr_t tid; /* type id */
    };
    struct UnitData_UnknownIdentifier_t{
      std::string str;
    };
    struct UnitData_t{
      UnitEnum ue;
      union{ /* ugly because c++ doesnt support unions properly */
        uint8_t u;
        uint8_t n0[sizeof(UnitData_Type_t)];
        uint8_t n1[sizeof(UnitData_UnknownIdentifier_t)];
      };
    };
    UnitData_t UnitArray[8];
    uintptr_t UnitIndex = 0;

    void constructor(ScopeEnum se){
      new (this) ScopeData_t;
      this->se = se;
    }
    void destructor(){
      this->~ScopeData_t();
    }
  };
  struct ScopeStack_t{
    ScopeData_t *i;

    ScopeStack_t(){
      /* TOOD use mmap and reserve after pointer */
      /* so we can get crash for sure instead of corruption */

      i = (ScopeData_t *)A_resize(NULL, 0x100 * sizeof(ScopeData_t));

      i->constructor(ScopeEnum::Global);
    }
    ~ScopeStack_t(){
      A_resize(i, 0);
    }
  }ScopeStack;
  void ScopeIn(ScopeEnum se){
    ScopeStack.i++;
    ScopeStack.i->constructor(se);
  }
  void ScopeOut(){
    ScopeStack.i->destructor();
    ScopeStack.i--;
  }
  void *ScopeUnitData(uintptr_t i){
    return (void *)&ScopeStack.i->UnitArray[i].u;
  }
  void *ScopeUnitData(){
    return ScopeUnitData(ScopeStack.i->UnitIndex - 1);
  }
  void ScopeUnitInc(ScopeData_t::UnitEnum ue){
    auto &s = *ScopeStack.i;
    if(s.UnitIndex == sizeof(s.UnitArray) / sizeof(s.UnitArray[0])){
      __abort();
    }

    if(ue == ScopeData_t::UnitEnum::UnknownIdentifier){
      new (&s.UnitArray[s.UnitIndex].u) ScopeData_t::UnitData_UnknownIdentifier_t;
    }

    s.UnitArray[s.UnitIndex].ue = ue;

    s.UnitIndex++;
  }
  void ScopeClearUnits(){
    auto &s = *ScopeStack.i;
    auto ui = s.UnitIndex;
    while(ui--){
      if(s.UnitArray[ui].ue == ScopeData_t::UnitEnum::Type){
        TypeDereference(((ScopeData_t::UnitData_Type_t *)&s.UnitArray[ui].u)->tid);
      }
      if(s.UnitArray[ui].ue == ScopeData_t::UnitEnum::UnknownIdentifier){
        ((ScopeData_t::UnitData_UnknownIdentifier_t *)&s.UnitArray[ui].u)->~UnitData_UnknownIdentifier_t();
      }
    }
  }
  void ScopeResetUnits(){
    auto &s = *ScopeStack.i;
    ScopeClearUnits();
    s.UnitIndex = 0;
  }
  void ScopeUnitDone(){
    auto &s = *ScopeStack.i;
    do{
      #include "ScopeUnitDone.h"
    }while(0);
    ScopeResetUnits();
  }

  void IdentifierPointType(auto in, TypeList_t::nr_t TypeID){
    if(ScopeStack.i->se == ScopeEnum::Global){
      TypeReference(TypeID);
      IdentifierMap[in] = {
        .Type = Identifier_t::Type_t::Type,
        .TypeID = TypeID
      };
    }
    else{
      __abort();
    }
  }

  void IWantToDefineType(TypeList_t::nr_t TypeID){
    __abort();
  }

  TypeList_t::nr_t DeclareType(){
    return TypeList.NewNode();
  }

  TypeList_t::nr_t StructIdentifierGetType(auto in){
    auto find = StructIdentifierMap.find(std::string(in));
    if(find != StructIdentifierMap.end()){
      return find->second;
    }
    auto r = DeclareType();
    StructIdentifierMap[std::string(in)] = r;
    return r;
  }

  enum class SpecialTypeEnum : uintptr_t{
    _void,
    _uint64_t,
    _sint64_t,
    _uint32_t,
    _sint32_t,
    _uint16_t,
    _sint16_t,
    _uint8_t,
    _sint8_t,
    _bool,
    #if SYSTEM_BIT == 64
      _uintptr_t = _uint64_t,
      _sintptr_t = _sint64_t
    #elif SYSTEM_BIT == 32
      _uintptr_t = _uint32_t,
      _sintptr_t = _sint32_t
    #else
      #error ?
    #endif
  };
  struct TypeInit_t{
    TypeInit_t(){
      auto pile = OFFSETLESS(this, pile_t, TypeInit);

      #define d(name) \
        pile->IdentifierPointType(STR(name), pile->TypeList.NewNode());

      d(void)
      d(uint64_t)
      d(sint64_t)
      d(uint32_t)
      d(sint32_t)
      d(uint16_t)
      d(sint16_t)
      d(uint8_t)
      d(sint8_t)
      d(bool)

      pile->IdentifierPointType("_uintptr_t", (TypeList_t::nr_t)SpecialTypeEnum::_uintptr_t);
      pile->IdentifierPointType("_sintptr_t", (TypeList_t::nr_t)SpecialTypeEnum::_sintptr_t);

      #undef d
    }
  }TypeInit;

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
    CurrentExpand.s = fd.sdataend;
    CurrentExpand.i = fd.sdatabegin;
  }

  void ExpandTraceFileAdd(
    uintptr_t FileDataID,
    bool Relative,
    uintptr_t PathSize
  ){
    ExpandSync();

    ExpandTrace.inc();

    _ExpandTraceFileSet(
      FileDataID,
      Relative,
      PathSize
    );
  }

  void ExpandFile(bool Relative, const std::string &PathName){
    if(!Relative){
      /* check for placebo headers */
      if(!PathName.compare("stddef.h")){
        return;
      }
      else if(!PathName.compare("stdarg.h")){
        return;
      }

      uintptr_t i = 0;
      for(; i < DefaultInclude.size(); i++){
        if(IO_IsPathExists((DefaultInclude[i] + PathName).c_str())){
          break;
        }
      }
      if(EXPECT(i == DefaultInclude.size(), false)){
        errprint_exit("failed to find non relative include \"%s\"", PathName.c_str());
      }

      RelativePaths.append(DefaultInclude[i]);
      RelativePathsSize.push_back(DefaultInclude[i].size());
    }

    uintptr_t FileNameSize;
    {
      sintptr_t i = PathName.size();
      while(i--){
        if(PathName[i] == '/'){
          break;
        }
      }

      /* TODO check if slash has something before itself */

      FileNameSize = PathName.size() - i - 1;
      if(i > 0){
        RelativePaths.append(PathName, 0, i + 1);
        RelativePathsSize.back() += i + 1;
      }
    }

    auto abspath =
      std::string(
        RelativePaths.begin() + RelativePaths.size() - RelativePathsSize.back(),
        RelativePaths.end()
      ) + std::string(PathName.end() - FileNameSize, PathName.end())
    ;

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
      if(file_input_size > (uintptr_t)-1){
        /* file size is bigger than what we can map to memory */
        __abort();
      }

      auto rdatabegin = (uint8_t *)IO_mmap(
        NULL,
        file_input_size,
        PROT_READ,
        MAP_SHARED,
        fd_input.fd, 
        0
      );
      uint8_t *rdataend = &rdatabegin[file_input_size];
      if((sintptr_t)rdatabegin < 0 && (sintptr_t)rdatabegin > -2048){
        __abort();
      }

      FS_file_close(&file_input);

      auto sdatabegin = A_resize(NULL, file_input_size + 1); /* + 1 for endline in end */
      uint8_t *sdataend;

      {
        auto rdata = rdatabegin;
        auto sdata = sdatabegin;
        #include "SimplifyFile1.h"
        sdataend = sdata;
      }

      if(sdatabegin == sdataend || *(sdataend - 1) != '\n'){
        *sdataend++ = '\n';
      }

      uintptr_t sdataSize = sdataend - sdatabegin;

      /* sdatabegin can be changed with resize so lets dont use sdataend */
      sdatabegin = A_resize(sdatabegin, sdataSize);

      FileDataList[FileDataList.NewNode()] = {
        .rdatabegin = rdatabegin,
        .rdataend = rdataend,
        .sdatabegin = sdatabegin,
        .sdataend = &sdatabegin[sdataSize],
        .FullPath = abspath,
        .FileNameSize = FileNameSize
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
    ExpandSync();

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
    auto &rp = RelativePaths;
    if(CurrentExpand.file.Relative){
      rp = rp.substr(0, rp.size() - CurrentExpand.file.PathSize);
    }
    else{
      rp = rp.substr(0, rp.size() - RelativePathsSize.back());
      RelativePathsSize.pop_back();
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
    #if set_WriteToFile
      printstdout("%c", '\n');
    #endif
    if(IsLastExpandDefine()){
      _DeexpandDefine();
      _Deexpand();
      return;
    }
    else{
      CurrentExpand.i++;
      while(CurrentExpand.i == CurrentExpand.s){
        Deexpand(); /* TOOD we already know its a file. why not call deexpand file? */
      }
    }
  }
  void _ic(){
    if(gc() == '\n'){
      ic_endline();
    }
    else{
      #if set_WriteToFile
        printstdout("%c", *CurrentExpand.i);
      #endif
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

  const uint8_t eof_string[5] = {'#','e','o','f','\n'};
  {
    auto &f = pile.FileDataList[pile.FileDataList.NewNode()];

    f.rdatabegin = eof_string;
    f.rdataend = eof_string + sizeof(eof_string);
    f.sdatabegin = eof_string;
    f.sdataend = eof_string + sizeof(eof_string);
    f.FullPath = "";
    f.FileNameSize = 0;
  }
  pile._ExpandTraceFileSet(0, true, 0);
  pile.ExpandTrace.inc();

  pile.RelativePaths += '\n';
  pile.RelativePathsSize.push_back(0);

  pile.ExpandFile(true, pile.filename);

  return pile.Compile();
}
