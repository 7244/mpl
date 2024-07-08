struct WordType{
  using t = uint8_t;
  enum : t{
    PreprocessorStart
  };
};

/* tab or space */
bool ischar_ts(const uint8_t &c){
  return c == ' ' || c == '\t';
}
bool ischar_ts(){
  return ischar_ts(gc());
}

void SkipEmptyInLine(){
  while(1){
    if(!ischar_ts()){
      break;
    }

    ic_unsafe();
  }
}

/* beauty string is just string that doesnt have spaces in begin and end */
/* can be used as side effect without using return value */
std::string ReadLineAsBeautyString(){
  std::string r;

  SkipEmptyInLine();

  while(gc() != '\n'){
    r += gc();
    ic();
  }

  ic();

  auto l = r.size();
  while(l--){
    if(!ischar_ts(r[l])){
      break;
    }
    r.pop_back();
  }

  return r;
}

WordType::t IdentifyWordAndSkip(){
  if(gc() == '#'){

    ic();
    SkipEmptyInLine();

    return WordType::PreprocessorStart;
  }
  else{
    printwi("cant identify %c", gc());
    __abort();
  }

  __unreachable();
}

uintptr_t _GetIdentifier(){
  uintptr_t r = 1;

  if(EXPECT(!STR_ischar_char(gc()) && gc() != '_', false)){
    printwi("Identifier starts with not character.");
    __abort();
  }

  ic_unsafe();
  while(STR_ischar_char(gc()) || STR_ischar_digit(gc()) || gc() == '_'){
    ic_unsafe();
    r++;
  }

  return r;
}

std::string_view GetIdentifier(){
  auto p = &gc();
  return std::string_view((const char *)p, _GetIdentifier());
}

/* include strings cant have escape or anything fancy */
bool GetIncludeString(bool &Relative, std::string &Name){
  uint8_t StopAt;

  if(gc() == '\"'){
    Relative = true;
    StopAt = '\"';
  }
  else if(gc() == '<'){
    Relative = false;
    StopAt = '>';
  }
  else{
    return false;
  }

  ic();
  while(1){
    if(gc() == StopAt){
      ic();
      break;
    }
    Name.push_back(gc());
    ic();
  }

  return true;
}

struct IncludePath_t{
  bool Relative;
  std::string Name;
};
IncludePath_t GetIncludePath(){
  IncludePath_t r;

  if(GetIncludeString(r.Relative, r.Name));
  else{
    /* TODO parse macro if it is macro */
    __abort();
  }

  return r;
}

void GetDefineParams(std::vector<std::string> &Inputs, bool &va_args){
  std::string in;
  while(1){
    SkipEmptyInLine();
    if(gc() == ')'){
      ic();
      break;
    }
    else if(gc() == ','){
      if(!in.size()){
        printwi("zero length parameter name");
        PR_exit(1);
      }
      Inputs.push_back(in);
      in = {};
      ic();
    }
    else if(gc() == '.'){
      ic();
      if(gc() != '.'){
        printwi("expected ...");
        PR_exit(1);
      }
      ic();
      if(gc() != '.'){
        printwi("expected ...");
        PR_exit(1);
      }
      ic();
      va_args = true;
      while(ischar_ts()){
        ic();
      }
      if(gc() != ')'){
        printwi("expected ) after ...");
        PR_exit(1);
      }
      ic();
      break;
    }
    else{
      in = GetIdentifier();
    }
  }
  if(in.size()){
    Inputs.push_back(in);
  }
}

void SkipCurrentEmptyLine(){
  while(1){
    if(ischar_ts()){
      
    }
    else if(gc() == '\n'){
      ic();
      break;
    }
    else{
      printwi("this line supposed to be empty after something");
      __abort();
    }
    ic();
  }
}

void SkipCurrentLine(){
  while(1){
    if(gc() == '\n'){
      ic();
      break;
    }
    ic_unsafe();
  }
}

uint8_t SkipToNextPreprocessorScopeExit(){
  std::vector<uint8_t> Scopes;

  while(1){
    SkipEmptyInLine();
    if(gc() == '#'){
      ic_unsafe();
      SkipEmptyInLine();

      auto Identifier = GetIdentifier();

      do{
        uint8_t t =
          1 * !Identifier.compare("ifdef") |
          1 * !Identifier.compare("ifndef") |
          1 * !Identifier.compare("if") |
          2 * !Identifier.compare("elif") |
          3 * !Identifier.compare("else") |
          4 * !Identifier.compare("endif")
        ;
        if(t == 0){
          break;
        }
        if(t == 1){
          Scopes.push_back(0);
          break;
        }
        --t;
        if(!Scopes.size()){
          return t;
        }
        auto &s = Scopes.back();
        if(t == 3){
          Scopes.pop_back();
          break;
        }
        if(EXPECT(s == 2 && t <= s, false)){
          printwi("double #else or #elif after #else");
          __abort();
        }
        s = t;
      }while(0);

      SkipCurrentLine();
    }
    else if(gc() == '\n'){
      ic();
    }
    else{
      SkipCurrentLine();
    }
  }
}

bool GetConditionFromPreprocessor(){
  __abort();
  return true;
}

void PreprocessorIf(
  uint8_t Type, /* check PreprocessorScope_t */
  bool Condition
){
  gt_begin:;

  if(Type == 0){
    PreprocessorScope.push_back({.Condition = Condition});
  }
  else{
    if(!PreprocessorScope.size()){
      printwi("preprocessor scope if type requires past if.");
      __abort();
    }
    auto &ps = PreprocessorScope[PreprocessorScope.size() - 1];
    if(ps.Type == 2 && Type <= ps.Type){
      printwi("double #else or #elif after #else");
      __abort();
    }
    ps.Type = Type;
  }

  if(!Condition){
    Type = SkipToNextPreprocessorScopeExit();
    if(Type == 3){
      return;
    }
    else if(Type == 2){
      Condition = true;
    }
    else{ /* 1 */
      Condition = GetConditionFromPreprocessor();
    }
    goto gt_begin;
  }
}

bool Compile(){
  while(1){

    if(STR_ischar_blank(gc())){
      ic();
      continue;
    }

    auto wt = IdentifyWordAndSkip();

    if(wt == WordType::PreprocessorStart){
      auto Identifier = GetIdentifier();
      if(!Identifier.compare("eof")){
        return 0;
      }
      else if(!Identifier.compare("error")){
        auto errstr = ReadLineAsBeautyString();
        printwi("#error %s", errstr.c_str());
        PR_exit(1);
      }
      else if(!Identifier.compare("include")){
        SkipEmptyInLine();
        auto ip = GetIncludePath();
        ExpandFile(ip.Relative, ip.Name);
      }
      else if(!Identifier.compare("pragma")){
        SkipEmptyInLine();
        auto piden = GetIdentifier();
        if(!piden.compare("once")){
          if(!GetPragmaOnce()){
            GetPragmaOnce() = true;
          }
          else{
            DeexpandFile();
          }
        }
        else{
          __abort();
        }
      }
      else if(!Identifier.compare("define")){
        SkipEmptyInLine();
        auto defiden = GetIdentifier();

        auto did = DefineMap.find(defiden);
        if(did != DefineMap.end()){
          if(settings.Wmacro_redefined){
            printwi("macro is redefined");
          }
          DefineMap.erase(defiden);
        }

        Define_t Define;
        if(gc() == '('){
          Define.isfunc = true;
          ic();
          GetDefineParams(Define.Inputs, Define.va_args);
        }
        else if(ischar_ts()){
          Define.isfunc = false;
          ic();
        }
        else if(gc() != '\n'){
          __abort();
        }

        Define.Output = ReadLineAsBeautyString();

        DefineMap[defiden] = Define;
      }
      else if(!Identifier.compare("ifdef")){
        SkipEmptyInLine();
        auto defiden = GetIdentifier();
        SkipCurrentEmptyLine();
        PreprocessorIf(0, DefineMap.find(defiden) != DefineMap.end());
      }
      else if(!Identifier.compare("ifndef")){
        SkipEmptyInLine();
        auto defiden = GetIdentifier();
        SkipCurrentEmptyLine();
        PreprocessorIf(0, DefineMap.find(defiden) == DefineMap.end());
      }
      else if(!Identifier.compare("undef")){
        SkipEmptyInLine();
        auto defiden = GetIdentifier();
        SkipCurrentEmptyLine();
        if(DefineMap.find(defiden) == DefineMap.end()){
          /* no any compiler gives error or warning about it. but i want. */
          if(settings.Wmacro_not_defined){
            printwi("warning, Wmacro-not-defined %.*s",
              (uintptr_t)defiden.size(), defiden.data()
            );
          }
        }
        else{
          DefineMap.erase(defiden);
        }
      }
      else{
        printwi("unknown preprocessor identifier. %.*s",
          (uintptr_t)Identifier.size(), Identifier.data()
        );
        __abort();
      }
    }
    else{
      __abort();
    }
  }

  return 0;
}
