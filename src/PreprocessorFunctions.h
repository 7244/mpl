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
  oplogor,
  opternary
};
PreprocessorParseType IdentifyPreprocessorParse(){
  gt_begin:;

  if(gc() == '\n'){
    if(IsLastExpandDefine()){
      _DeexpandDefine();
      _Deexpand();
      SkipEmptyInLine();
      goto gt_begin;
    }
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
  else if(gc() == '?'){
    ic_unsafe();
    return PreprocessorParseType::opternary;
  }

  errprint_exit("cant identify preprocessor parse %lx %c", gc(), gc());
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

void GetDefineParams(std::map<std::string_view, uintptr_t> &Inputs, bool &va_args){
  /* TOOD do this function without storing string variable */

  va_args = false;

  uintptr_t incount = 0;
  std::string_view in;
  while(1){
    SkipEmptyInLine();
    if(gc() == ')'){
      ic_unsafe();
      break;
    }
    else if(gc() == ','){
      if(!in.size()){
        errprint_exit("zero length define parameter name");
      }
      Inputs[in] = incount++;
      in = {};
      ic_unsafe();
    }
    else if(gc() == '.'){
      ic_unsafe();
      if(gc() != '.'){
        errprint_exit("expected ...");
      }
      ic_unsafe();
      if(gc() != '.'){
        errprint_exit("expected ...");
      }
      ic_unsafe();
      va_args = true;
      SkipEmptyInLine();
      if(gc() != ')'){
        errprint_exit("expected ) after ...");
      }
      ic_unsafe();
      break;
    }
    else{
      in = GetIdentifier();
    }
  }
  if(in.size()){
    Inputs[in] = incount;
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

sint64_t GetPreprocessorNumber(){
  sint64_t v = 0;
  do{
    v = v * 10 + gc() - '0';
    ic_unsafe();
  }while(STR_ischar_digit(gc()));

  uintptr_t literal_u = 0;
  uintptr_t literal_l = 0;
  uintptr_t literal_z = 0;

  while(STR_ischar_char(gc())){
    if(gc() == 'u' || gc() == 'U'){
      if(EXPECT(++literal_u > 1, false)){
        errprint_exit("double integer literal u.");
      }
    }
    else if(gc() == 'l' || gc() == 'L'){
      if(EXPECT(++literal_l > 2, false)){
        errprint_exit("triple integer literal l.");
      }
    }
    else if(gc() == 'z' || gc() == 'Z'){
      if(EXPECT(++literal_z > 1, false)){
        errprint_exit("double integer literal z.");
      }
    }
    else [[unlikely]] {
      errprint_exit("invalid integer literal \"%lc\"", gc());
    }
    ic_unsafe();
  }

  if(literal_l && literal_z){
    errprint_exit("integer literal l and z cant be in same place");
  }

  /* TOOD how to use these literals? */

  return v;
}

void ExpandPreprocessorIdentifier_ddid(uintptr_t ddid){
  auto &dd = DefineDataList[ddid];

  if(!dd.isfunc){
    ExpandTraceDefineAdd(ddid);
    return;
  }

  SkipEmptyInLine();
  if(gc() != '('){
    /* function called without parenthese. need to give 0. */

    if(settings.Wmacro_incorrect_call){
      printwi("Wmacro_incorrect_call");
    }

    *DefineStack.i++ = '0';
    *DefineStack.i++ = '\n';
    _ExpandTraceDefineAdd(0, DefineStack.i - 2, 2);

    return;
  }

  if(dd.va_args){
    __abort();
  }

  uintptr_t pc = 0; /* parameter count */
  ic_unsafe();
  while(1){
    SkipEmptyInLine();
    if(EXPECT(gc() == '\n', false)){
      errprint_exit("endline came too early");
    }
    else if(gc() == ')'){
      break;
    }
    else if(gc() == ','){
      *(std::string_view *)DefineStack.i = std::string_view((const char *)&gc(), 0);
      DefineStack.i += sizeof(std::string_view);
      pc++;
      __abort();
    }
    else{
      auto lp = &gc();
      uintptr_t ScopeIn = 0;
      while(1){
        if(EXPECT(gc() == '\n', false)){
          errprint_exit("endline came too early");;
        }
        else if(gc() == '('){
          ScopeIn++;
        }
        else if(gc() == ')'){
          if(!ScopeIn){
            *(std::string_view *)DefineStack.i = std::string_view(
              (const char *)lp,
              (uintptr_t)&gc() - (uintptr_t)lp
            );
            DefineStack.i += sizeof(std::string_view);
            pc++;
            goto gt_funcend;
          }
          ScopeIn--;
        }
        else if(gc() == ',' && !ScopeIn){
          *(std::string_view *)DefineStack.i = std::string_view(
            (const char *)lp,
            (uintptr_t)&gc() - (uintptr_t)lp
          );
          DefineStack.i += sizeof(std::string_view);
          pc++;
          ic_unsafe();
          break;
        }
        ic_unsafe();
      }
    }
  }

  gt_funcend:;

  if(EXPECT(dd.Inputs.size() - pc > 1, false)){
    errprint_exit("expected %u parameters, got %u parameters.", dd.Inputs.size(), pc);
  }

  while(pc != dd.Inputs.size()){
    *(std::string_view *)DefineStack.i = std::string_view((const char *)&gc(), 0);
    DefineStack.i += sizeof(std::string_view);
    pc++;
  }

  ExpandTraceDefineFunctionAdd(ddid);
}

void ExpandPreprocessorIdentifier(){
  auto SeekConcatenation = [&]() -> bool {
    SkipEmptyInLine();
    if(gc() == '#'){
      if((&gc())[1] == '#'){
        return true;
      }
    }
    return false;
  };

  auto iden = GetIdentifier();

  if(!IsLastExpandDefine()){
    if(!iden.compare("defined")){
      std::string_view defiden;

      SkipEmptyInLine();
      if(gc() == '('){
        ic_unsafe();
        SkipEmptyInLine();
        defiden = GetIdentifier();
        SkipEmptyInLine();
        if(gc() != ')'){
          errprint_exit("expected ) for defined");
        }
        ic_unsafe();
      }
      else{
        defiden = GetIdentifier();
      }

      *DefineStack.i++ = '0' + (DefineDataMap.find(defiden) != DefineDataMap.end());
      *DefineStack.i++ = '\n';
      _ExpandTraceDefineAdd(0, DefineStack.i - 2, 2);

      return;
    }
  }
  else{
    /* TOOOD checking data id if 0 or not can be faster */

    auto &m = DefineDataList[CurrentExpand.DataID].Inputs; /* map */

    if(SeekConcatenation()){
      auto StackStart = DefineStack.i;

      while(1){
        auto mid = m.find(iden);
        if(mid != m.end()){
          auto &param = ((std::string_view *)CurrentExpand.define.pstack)[mid->second];
          __abort();
        }
        else{
          __MemoryCopy(iden.data(), DefineStack.i, iden.size());
          DefineStack.i += iden.size();
        }
        ic_unsafe(); /* second # */
        ic_unsafe(); /* after concatenation */

        SkipEmptyInLine();
        auto iden = GetIdentifier();

        if(!SeekConcatenation()){
          break;
        }
      }
      auto mid = m.find(iden);
      if(mid != m.end()){
        auto &param = ((std::string_view *)CurrentExpand.define.pstack)[mid->second];
        __abort();
      }
      else{
        __MemoryCopy(iden.data(), DefineStack.i, iden.size());
        DefineStack.i += iden.size();
      }

      *DefineStack.i++ = '\n';

      _ExpandTraceDefineAdd(0, StackStart, DefineStack.i - StackStart);
    }
    else{
      auto mid = m.find(iden);
      if(mid != m.end()){
        auto &param = ((std::string_view *)CurrentExpand.define.pstack)[mid->second];
        __abort();
      }
    }
  }

  auto ddmid = DefineDataMap.find(iden);
  if(ddmid == DefineDataMap.end()){
    if(settings.Wmacro_not_defined){
      printwi("warning, Wmacro-not-defined %.*s",
        (uintptr_t)iden.size(), iden.data()
      );
    }

    *DefineStack.i++ = '0';
    *DefineStack.i++ = '\n';
    _ExpandTraceDefineAdd(0, DefineStack.i - 2, 2);

    return;
  }

  ExpandPreprocessorIdentifier_ddid(ddmid->second);
}

void _ParsePreprocessorToCondition(uint8_t *stack){
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
    11 ternary
    12 reserved. used for initial priority
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

    logor,

    ternary
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
      case ops::ternary:
        return 11;
    }
    __unreachable();
  };

  /* last priority */
  uint8_t lp = 12;

  bool LastStackIsVariable = false;

  auto Process = [&]() -> void {
    auto glv = [&]() -> sint64_t {
      stack -= sizeof(sint64_t);
      auto ret = *(sint64_t *)stack;
      return ret;
    };

    if(!LastStackIsVariable){
      printwi("TODO how to describe this error?");
      __abort();
    }

    stack -= sizeof(sint64_t);
    auto rv = *(sint64_t *)stack;

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
          rv = glv() + rv;
          break;
        }
        case ops::minus:{
          rv = glv() - rv;
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
          rv = glv() < rv;
          break;
        }
        case ops::le:{
          rv = glv() <= rv;
          break;
        }
        case ops::gt:{
          rv = glv() > rv;
          break;
        }
        case ops::ge:{
          rv = glv() >= rv;
          break;
        }
        case ops::eq:{
          rv = glv() == rv;
          break;
        }
        case ops::ne:{
          rv = glv() != rv;
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
        case ops::ternary:{
          __unreachable();
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
        _ParsePreprocessorToCondition(stack);
        stack += sizeof(sint64_t);

        LastStackIsVariable = true;

        break;
      }
      case PreprocessorParseType::parentheseclose:{
        goto gt_end;
      }
      case PreprocessorParseType::number:{
        *(sint64_t *)stack = GetPreprocessorNumber();
        stack += sizeof(sint64_t);

        LastStackIsVariable = true;

        break;
      }
      case PreprocessorParseType::Identifier:{
        ExpandPreprocessorIdentifier();
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
        if(LastStackIsVariable){
          AddOP(ops::plus);
        }
        else{
          AddOP(ops::unaryp);
        }
        break;
      }
      case PreprocessorParseType::minus:{
        if(LastStackIsVariable){
          AddOP(ops::minus);
        }
        else{
          AddOP(ops::unarym);
        }
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
      case PreprocessorParseType::opternary:{
        Process();
        stack -= sizeof(sint64_t);
        sint64_t v = *(sint64_t *)stack;
        if(v){
          __abort();
        }
        else{ /* need go to colon */
          while(1){
            if(gc() == '\n'){
              if(EXPECT(!IsLastExpandDefine(), false)){
                errprint_exit("preprocessor ternary operator failed to find :");
              }
              _DeexpandDefine();
              _Deexpand();
            }
            else if(STR_ischar_char(gc()) || gc() == '_'){
              ExpandPreprocessorIdentifier();
            }
            else if(gc() == ':'){
              ic_unsafe();
              break;
            }
            ic_unsafe();
          }
        }
        break;
      }
    }
  }

  gt_end:

  Process();
}

bool ParsePreprocessorToCondition(){
  uint8_t stack[0x1000];
  _ParsePreprocessorToCondition(stack);
  return !!*(sint64_t *)stack;
}

bool GetPreprocessorCondition(uint8_t ConditionType){
  if(ConditionType == 2){
    return ParsePreprocessorToCondition();
  }
  else if(ConditionType == 3){
    return true;
  }

  /* condition is 0 or 1 */

  SkipEmptyInLine();
  auto defiden = GetIdentifier();
  SkipCurrentEmptyLine();
  return DefineDataMap.find(defiden) != DefineDataMap.end() ^ ConditionType;
}

void PreprocessorIf(
  uint8_t Type, /* check PreprocessorScope_t */
  uint8_t ConditionType
){
  gt_begin:;

  if(Type == 0){
    PreprocessorScope.push_back({});
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
    if(Type == 3){
      PreprocessorScope.pop_back();
      return;
    }
    ps.Type = Type;
  }

  if(!PreprocessorScope.back().DidRan){
    auto Condition = GetPreprocessorCondition(ConditionType);
    if(Condition){
      PreprocessorScope.back().DidRan = true;
      return;
    }
  }

  Type = SkipToNextPreprocessorScopeExit();
  if(Type == 1){ /* else if */
    ConditionType = 2;
  }
  else if(Type == 2){ /* else */
    ConditionType = 3;
  }
  goto gt_begin;
}
