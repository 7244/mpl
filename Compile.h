struct WordType{
  using t = uint8_t;
  enum : t{
    PreprocessorStart
  };
};

/* tab or space */
bool ischar_ts(uint8_t c){
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

    ic();
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
    __abort();
  }

  __unreachable();
}

std::string GetIdentifier(){
  std::string r;

  if(!STR_ischar_char(gc()) && gc() != '_'){
    printwi("Identifier starts with not character.");
    __abort();
  }

  r.push_back(gc());

  ic();
  while(1){
    if(!STR_ischar_digit(gc()) && !STR_ischar_char(gc()) && gc() != '_'){
      break;
    }

    r.push_back(gc());

    ic();
  }

  return r;
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
    ic();
    if(gc() == '\n'){
      ic();
      break;
    }
  }
}

uint8_t SkipToNextPreprocessorScopeExit(){
  std::vector<uint8_t> Scopes;

  while(1){
    SkipEmptyInLine();
    if(gc() == '#'){
      ic();
      SkipEmptyInLine();

      auto Identifier = GetIdentifier();

      if(
        !Identifier.compare("ifdef") ||
        !Identifier.compare("ifndef") ||
        !Identifier.compare("if")
      ){
        Scopes.push_back(0);
      }
      else do{
        uint8_t t =
          1 * !Identifier.compare("elif") |
          2 * !Identifier.compare("else") |
          3 * !Identifier.compare("endif")
        ;
        if(t == 0){
          break;
        }
        if(!Scopes.size()){
          return t;
        }
        auto &s = Scopes[Scopes.size() - 1];
        if(t == 3){
          Scopes.pop_back();
          break;
        }
        if(s == 2 && t <= s){
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
        __abort();
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
      else{
        print("what is this? %s\n", Identifier.c_str());
        __abort();
      }
    }
    else{
      __abort();
    }
  }

  return 0;
}
