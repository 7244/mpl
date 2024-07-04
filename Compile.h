struct WordType{
  using t = uint8_t;
  enum : t{
    PreprocessorStart
  };
};

/* tab or space */
bool ischar_ts(){
  return gc() == ' ' || gc() == '\t';
}

void SkipEmptyInLine(){
  while(1){
    if(!ischar_ts()){
      break;
    }

    ic();
  }
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
        else{
          __abort();
        }

        while(gc() != '\n'){
          Define.Output += gc();
          ic();
        }

        DefineMap[defiden] = Define;
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
