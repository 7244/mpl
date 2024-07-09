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


enum class WordType : uint8_t{
  PreprocessorStart,
  Identifier
};
WordType IdentifyWordAndSkip(){
  if(gc() == '#'){

    ic_unsafe();
    SkipEmptyInLine();

    return WordType::PreprocessorStart;
  }
  else{
    printwi("cant identify %c", gc());
    __abort();
  }

  __unreachable();
}

enum class PreprocessorParseType : uint8_t{
  endline,
  parentheseopen,
  parentheseclose,
  number,
  Identifier,
  oplognot,
  opbinot,
  opmul,
  opdiv,
  opmod,
  plus,
  minus,
  oplshift,
  oprshift,
  oplt,
  ople,
  opgt,
  opge,
  opeq,
  opne,
  opbiand,
  opbixor,
  opbior,
  oplogand,
  oplogor
};
PreprocessorParseType IdentifyPreprocessorParse(){
  if(gc() == '\n'){
    ic();
    return PreprocessorParseType::endline;
  }
  else if(gc() == '('){
    ic_unsafe();
    return PreprocessorParseType::parentheseopen;
  }
  else if(gc() == ')'){
    ic_unsafe();
    return PreprocessorParseType::parentheseclose;
  }
  else if(STR_ischar_digit(gc())){
    return PreprocessorParseType::number;
  }
  else if(STR_ischar_char(gc()) || gc() == '_'){
    return PreprocessorParseType::Identifier;
  }
  else if(gc() == '+'){
    ic_unsafe();
    return PreprocessorParseType::plus;
  }
  else if(gc() == '-'){
    ic_unsafe();
    return PreprocessorParseType::minus;
  }
  else if(gc() == '!'){
    ic_unsafe();
    if(gc() == '='){
      ic_unsafe();
      return PreprocessorParseType::opne;
    }
    return PreprocessorParseType::oplognot;
  }
  else if(gc() == '~'){
    ic_unsafe();
    return PreprocessorParseType::opbinot;
  }
  else if(gc() == '*'){
    ic_unsafe();
    return PreprocessorParseType::opmul;
  }
  else if(gc() == '/'){
    ic_unsafe();
    return PreprocessorParseType::opdiv;
  }
  else if(gc() == '%'){
    ic_unsafe();
    return PreprocessorParseType::opmod;
  }
  else if(gc() == '<'){
    ic_unsafe();
    if(gc() == '<'){
      ic_unsafe();
      return PreprocessorParseType::oplshift;
    }
    else if(gc() == '='){
      ic_unsafe();
      return PreprocessorParseType::ople;
    }
    return PreprocessorParseType::oplt;
  }
  else if(gc() == '>'){
    ic_unsafe();
    if(gc() == '>'){
      ic_unsafe();
      return PreprocessorParseType::oprshift;
    }
    else if(gc() == '='){
      ic_unsafe();
      return PreprocessorParseType::opge;
    }
    return PreprocessorParseType::opgt;
  }
  else if(gc() == '='){
    ic_unsafe();
    if(gc() != '='){
      __abort();
    }
    ic_unsafe();
    return PreprocessorParseType::opeq;
  }
  else if(gc() == '&'){
    ic_unsafe();
    if(gc() == '&'){
      ic_unsafe();
      return PreprocessorParseType::oplogand;
    }
    return PreprocessorParseType::opbiand;
  }
  else if(gc() == '^'){
    ic_unsafe();
    return PreprocessorParseType::opbixor;
  }
  else if(gc() == '|'){
    ic_unsafe();
    if(gc() == '|'){
      ic_unsafe();
      return PreprocessorParseType::oplogor;
    }
    return PreprocessorParseType::opbior;
  }
  else{
    printwi("cant identify preprocessor parse %c", gc());
    __abort();
  }

  __unreachable();
}

/* TOOD no need to use r. can use expand i pointer */
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

sint64_t _GetConditionFromPreprocessor(uint8_t *stack){
  auto StackStart = stack;

  /*
    defined will be solved immediately
    0 unary+ unary- ! ~
    1 * / %
    2 + -
    3 << >>
    4 < <= > >=
    5 == !=
    6 &
    7 ^
    8 |
    9 &&
    10 ||
    11 reserved. used for initial priority
  */

  enum class ops : uint8_t{
    unaryp,
    unarym,
    lognot,
    binot,

    mul,
    div,
    mod,

    plus,
    minus,

    lshift,
    rshift,

    lt,
    le,
    gt,
    ge,

    eq,
    ne,

    biand,

    bixor,

    bior,

    logand,

    logor
  };

  /* get priority */
  auto gp = [&](ops op) -> uint8_t {
    switch(op){
      case ops::unaryp:
      case ops::unarym:
      case ops::lognot:
      case ops::binot:
        return 0;
      case ops::mul:
      case ops::div:
      case ops::mod:
        return 1;
      case ops::plus:
      case ops::minus:
        return 2;
      case ops::lshift:
      case ops::rshift:
        return 3;
      case ops::lt:
      case ops::le:
      case ops::gt:
      case ops::ge:
        return 4;
      case ops::eq:
      case ops::ne:
        return 5;
      case ops::biand:
        return 6;
      case ops::bixor:
        return 7;
      case ops::bior:
        return 8;
      case ops::logand:
        return 9;
      case ops::logor:
        return 10;
    }
    __unreachable();
  };

  /* last priority */
  uint8_t lp = 11;

  bool LastStackIsVariable = false;

  auto Process = [&]() -> void {
    auto glv = [&]() -> sint64_t {
      auto ret = *(sint64_t *)stack;
      stack -= sizeof(sint64_t);
      return ret;
    };

    if(!LastStackIsVariable){
      printwi("TODO how to describe this error?");
      __abort();
    }

    auto rv = *(sint64_t *)stack;
    stack -= sizeof(sint64_t);

    while(stack != StackStart){
      ops op = (ops)*--stack;

      switch(op){
        case ops::unaryp:{
          rv = +rv;
          break;
        }
        case ops::unarym:{
          rv = -rv;
          break;
        }
        case ops::lognot:{
          rv = !rv;
          break;
        }
        case ops::binot:{
          rv = ~rv;
          break;
        }
        case ops::mul:{
          __abort();
          break;
        }
        case ops::div:{
          __abort();
          break;
        }
        case ops::mod:{
          __abort();
          break;
        }
        case ops::plus:{
          __abort();
          break;
        }
        case ops::minus:{
          __abort();
          break;
        }
        case ops::lshift:{
          __abort();
          break;
        }
        case ops::rshift:{
          __abort();
          break;
        }
        case ops::lt:{
          __abort();
          break;
        }
        case ops::le:{
          __abort();
          break;
        }
        case ops::gt:{
          __abort();
          break;
        }
        case ops::ge:{
          __abort();
          break;
        }
        case ops::eq:{
          __abort();
          break;
        }
        case ops::ne:{
          __abort();
          break;
        }
        case ops::biand:{
          __abort();
          break;
        }
        case ops::bixor:{
          __abort();
          break;
        }
        case ops::bior:{
          __abort();
          break;
        }
        case ops::logand:{
          rv = glv() && rv;
          break;
        }
        case ops::logor:{
          rv = glv() || rv;
          break;
        }
      }
    }

    *(sint64_t *)stack = rv;
    stack += sizeof(sint64_t);
  };

  auto AddOP = [&](ops new_op) -> void {

    auto p = gp(new_op);
    if(p >= lp){
      Process();
    }

    *stack++ = (uint8_t)new_op;
    lp = p;
    LastStackIsVariable = false;
  };

  while(1){
    SkipEmptyInLine();
    auto t = IdentifyPreprocessorParse();
    switch(t){
      case PreprocessorParseType::endline:{
        goto gt_end;
      }
      case PreprocessorParseType::parentheseopen:{
        _GetConditionFromPreprocessor(stack);
        stack += sizeof(sint64_t);

        LastStackIsVariable = true;

        break;
      }
      case PreprocessorParseType::parentheseclose:{
        goto gt_end;
      }
      case PreprocessorParseType::number:{
        __abort();
        break;
      }
      case PreprocessorParseType::Identifier:{
        auto iden = GetIdentifier();
        if(!iden.compare("defined")){
          std::string_view defiden;

          SkipEmptyInLine();
          if(gc() == '('){
            ic_unsafe();
            SkipEmptyInLine();
            defiden = GetIdentifier();
            SkipEmptyInLine();
            if(gc() != ')'){
              printwi("expected ) for defined");
            }
            ic_unsafe();
          }
          else{
            defiden = GetIdentifier();
          }

          *(sint64_t *)stack = DefineMap.find(defiden) != DefineMap.end();
          stack += sizeof(sint64_t);

          LastStackIsVariable = true;
        }
        else{
          auto d = DefineMap.find(iden);
          if(d == DefineMap.end()){
            if(settings.Wmacro_not_defined){
              printwi("warning, Wmacro-not-defined %.*s",
                (uintptr_t)iden.size(), iden.data()
              );
            }

            *(sint64_t *)stack = 0;
            stack += sizeof(sint64_t);

            LastStackIsVariable = true;
          }
          else{
            __abort();
          }
        }
        break;
      }
      case PreprocessorParseType::oplognot:{
        AddOP(ops::lognot);
        break;
      }
      case PreprocessorParseType::opbinot:{
        AddOP(ops::binot);
        break;
      }
      case PreprocessorParseType::opmul:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::mul);
        break;
      }
      case PreprocessorParseType::opdiv:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::div);
        break;
      }
      case PreprocessorParseType::opmod:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::mod);
        break;
      }
      case PreprocessorParseType::plus:{
        __abort();
        break;
      }
      case PreprocessorParseType::minus:{
        __abort();
        break;
      }
      case PreprocessorParseType::oplshift:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::lshift);
        break;
      }
      case PreprocessorParseType::oprshift:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::rshift);
        break;
      }
      case PreprocessorParseType::oplt:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::lt);
        break;
      }
      case PreprocessorParseType::ople:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::le);
        break;
      }
      case PreprocessorParseType::opgt:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::gt);
        break;
      }
      case PreprocessorParseType::opge:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::ge);
        break;
      }
      case PreprocessorParseType::opeq:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::eq);
        break;
      }
      case PreprocessorParseType::opne:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::ne);
        break;
      }
      case PreprocessorParseType::opbiand:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::biand);
        break;
      }
      case PreprocessorParseType::opbixor:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::bixor);
        break;
      }
      case PreprocessorParseType::opbior:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::bior);
        break;
      }
      case PreprocessorParseType::oplogand:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::logand);
        break;
      }
      case PreprocessorParseType::oplogor:{
        if(!LastStackIsVariable){ __abort(); }
        AddOP(ops::logor);
        break;
      }
    }
  }

  gt_end:

  Process();

  return *(sint64_t *)stack;
}

bool GetConditionFromPreprocessor(){
  uint8_t stack[0x1000];
  return !!_GetConditionFromPreprocessor(stack);
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
        auto str = ReadLineAsBeautyString();
        printwi("#error %s", str.c_str());
        PR_exit(1);
      }
      else if(!Identifier.compare("warning")){
        auto str = ReadLineAsBeautyString();
        printwi("#warning %s", str.c_str());
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
      else if(!Identifier.compare("if")){
        PreprocessorIf(0, GetConditionFromPreprocessor());
      }
      else if(!Identifier.compare("else")){
        if(!PreprocessorScope.size()){
          /* else from nowhere? */
          __abort();
        }
        if(PreprocessorScope.back().Condition == false){
          /* else from nowhere? */
          __abort();
        }
        SkipCurrentEmptyLine(); /* lets ignore */
      }
      else if(!Identifier.compare("endif")){
        if(!PreprocessorScope.size()){
          /* endif from nowhere? */
          __abort();
        }
        if(PreprocessorScope.back().Condition == false){
          /* endif from nowhere? */
          __abort();
        }
        SkipCurrentEmptyLine(); /* lets ignore */
      }
      else if(!Identifier.compare("undef")){
        SkipEmptyInLine();
        auto defiden = GetIdentifier();
        SkipCurrentEmptyLine();
        if(DefineMap.find(defiden) == DefineMap.end()){
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
