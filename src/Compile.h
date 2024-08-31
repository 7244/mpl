bool ischar_empty(const uint8_t &c){
  return c == ' ';
}
bool ischar_empty(){
  return ischar_empty(gc());
}

void SkipEmptyInLine(){
  while(1){
    if(!ischar_empty()){
      break;
    }

    ic_unsafe();
  }
}

/* automaticly expands identifier too */
void SkipTillCode(){
  gt_begin:

  while(1){
    if(gc() == '\n'){
      ic_endline();
    }
    else if(ischar_empty()){
      ic_unsafe();
    }
    else{
      if(STR_ischar_char(gc()) || gc() == '_'){
        auto t = CurrentExpand.i + 1;
        while(STR_ischar_char(*t) || STR_ischar_digit(*t) || *t == '_'){
          t++;
        }
        auto iden = std::string_view(
          (const char *)CurrentExpand.i,
          (uintptr_t)t - (uintptr_t)CurrentExpand.i
        );
        auto ddmid = DefineDataMap.find(iden);
        if(ddmid != DefineDataMap.end()){
          auto ddid = ddmid->second;
          auto &d = DefineDataList[ddid];
          if(d.isfunc){
            /* need to search parenthese */
            __abort();
          }
          CurrentExpand.i = t;
          ExpandPreprocessorIdentifier_ddid(ddmid->second);
          goto gt_begin;
        }
      }
      break;
    }
  }
}

/* beautifully just means no spaces in begin and end */
/* TOOOD this function looks like mess. but can someone code faster than this? */
void ReadLineBeautifully0(const uint8_t **p, const uint8_t **s){
  SkipEmptyInLine();
  *p = &gc();
  *s = &gc();
  while(gc() != '\n'){
    while(!ischar_empty()){
      if(gc() == '\n'){
        *s = &gc();
        return;
      }
      ic_unsafe();
    }
    *s = &gc();
    SkipEmptyInLine();
  }
}

/* TOOD no need to use r. can use expand i pointer */
uintptr_t _GetIdentifier(){
  uintptr_t r = 1;

  if(EXPECT(!STR_ischar_char(gc()) && gc() != '_', false)){
    errprint_exit("Identifier starts with not character %lx %c", gc(), gc());
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

void SkipCurrentEmptyLine(){
  while(gc() != '\n'){
    if(EXPECT(!ischar_empty(), false)){
      errprint_exit("this line supposed to be empty after something");
    }
    ic_unsafe();
  }
  ic_endline();
}

void SkipCurrentLine(){
  while(1){
    if(gc() == '\n'){
      ic_endline();
      break;
    }
    ic_unsafe();
  }
}

#include "PreprocessorFunctions.h"

/* note that this function can change scope */
uintptr_t SimplifyType(){
  #if set_support_c99
    uintptr_t TypeID;

    uintptr_t _void = 0;
    uintptr_t _unsigned = 0;
    uintptr_t _signed = 0;
    uintptr_t _char = 0;
    uintptr_t _short = 0;
    uintptr_t _int = 0;
    uintptr_t _long = 0;
    uintptr_t _float = 0;
    uintptr_t _double = 0;

    uintptr_t _bool = 0;

    auto CheckCTypeLogic = [&]() -> void {
      uintptr_t cor = _char | _short | _int | _long | _float | _double;

      if(EXPECT(_unsigned && _signed, false)){
        errprint_exit("unsigned and signed is together");
      }

      if(EXPECT(_void && (_unsigned | _signed | _bool | cor), false)){
        errprint_exit("weird stuff with void");
      }

      if(EXPECT(_bool && (_unsigned | _signed | _void | cor), false)){
        errprint_exit("weird stuff with bool");
      }

      if(EXPECT((_unsigned | _signed) && (_float | _double), false)){
        errprint_exit("signs cant be specified with floating points");
      }

      if(EXPECT(_char && _int, false)){
        errprint_exit("char and int cant be together");
      }

      if(EXPECT(_char + _short + !!_long > 1, false)){
        errprint_exit("too many different types");
      }

      if(_void){
        TypeID = (uintptr_t)SpecialTypeEnum::_void;
      }
      else if(_unsigned){
        if(_char){
          TypeID = (uintptr_t)SpecialTypeEnum::_uint8_t;
        }
        else if(_short){
          TypeID = (uintptr_t)SpecialTypeEnum::_uint16_t;
        }
        else if(_long == 1){
          TypeID = (uintptr_t)SpecialTypeEnum::_uintptr_t;
        }
        else if(_long == 2){
          TypeID = (uintptr_t)SpecialTypeEnum::_uint64_t;
        }
        else{
          #if SYSTEM_BIT >= 32
            TypeID = (uintptr_t)SpecialTypeEnum::_uint32_t;
          #elif SYSTEM_BIT == 16
            TypeID =(uintptr_t)SpecialTypeEnum::_uint16_t;
          #else
            #error ?
          #endif
        }
      }
      else if(_signed){
        gt_signed:

        if(_char){
          TypeID = (uintptr_t)SpecialTypeEnum::_sint8_t;
        }
        else if(_short){
          TypeID = (uintptr_t)SpecialTypeEnum::_sint16_t;
        }
        else if(_long == 1){
          TypeID = (uintptr_t)SpecialTypeEnum::_sintptr_t;
        }
        else if(_long == 2){
          TypeID = (uintptr_t)SpecialTypeEnum::_sint64_t;
        }
        else{
          #if SYSTEM_BIT >= 32
            TypeID = (uintptr_t)SpecialTypeEnum::_sint32_t;
          #elif SYSTEM_BIT == 16
            TypeID = (uintptr_t)SpecialTypeEnum::_sint16_t;
          #else
            #error ?
          #endif
        }
      }
      else if(_char | _short | _int | _long){
        goto gt_signed;
      }
      else{
        if(EXPECT(cor | _void | _unsigned | _signed | _bool, false)){
          errprint_exit("type logic is failed");
        }
        TypeID = (uintptr_t)-1;
      }
    };

    while(1){
      if(STR_ischar_char(gc()) || gc() == '_'){
        auto iden = GetIdentifier();
        if(!iden.compare("void")){
          if(EXPECT(++_void > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("unsigned")){
          if(EXPECT(++_unsigned > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("signed")){
          if(EXPECT(++_signed > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("char")){
          if(EXPECT(++_char > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("short")){
          if(EXPECT(++_short > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("int")){
          if(EXPECT(++_int > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("long")){
          if(EXPECT(++_long > 2, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("bool")){
          if(EXPECT(++_bool > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("float")){
          __abort(); // check how they go with long
          if(EXPECT(++_float > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("double")){
          __abort(); // check how they go with long
          if(EXPECT(++_double > 1, false)){
            errprint_exit("too many types");
          }
        }
        else if(!iden.compare("const")){
          __abort();
        }
        else{
          CheckCTypeLogic();
          if(TypeID != (TypeList_t::nr_t)-1){
            CurrentExpand.i -= iden.size();
            return TypeID;
          }
          /* lets resolve iden */
          if(!iden.compare("struct")){
            while(1){
              SkipTillCode();
              if(gc() == '{'){
                ic_unsafe();
                if(TypeID == (TypeList_t::nr_t)-1){
                  /* we gonna define struct without typename */
                  TypeID = DeclareType();
                }
                else{ /* should work if this was not inside else too */
                  IWantToDefineType(TypeID);
                }

                ScopeIn(ScopeEnum::Type);
                ScopeStack.i->TypeID = TypeID;

                return TypeID;
              }
              else if(TypeID == (TypeList_t::nr_t)-1){
                TypeID = StructIdentifierGetType(GetIdentifier());
              }
              else{
                __abort();
                //TypeID = IdentifierMapGet(TypeName);
                break;
              }
            }
          }
          else{
            auto Identifier = IdentifierMapGet(iden);
            if(Identifier == NULL){
              CurrentExpand.i -= iden.size();
              return (TypeList_t::nr_t)-1;
            }
            if(Identifier->Type != Identifier_t::Type_t::Type){
              CurrentExpand.i -= iden.size();
              return (TypeList_t::nr_t)-1;
            }
            return Identifier->TypeID;
          }
          __abort(); // check if in upside
          return TypeID;
        }
      }
      else if(gc() == '*'){
        __abort();
      }
      else{
        printstderr("what is this character? \"%c\" %lx\n", gc(), gc());
        errprint_exit("");
        break;
      }

      SkipTillCode();
    }

    __abort();
    __unreachable();
  #else
    #error ?
  #endif
}

void AddTypeToUnit(TypeList_t::nr_t TypeID){
  ScopeUnitInc(ScopeData_t::UnitEnum::Type);
  auto u = (ScopeData_t::UnitData_Type_t *)ScopeUnitData();
  u->tid = TypeID;
  TypeReference(u->tid); /* will dereferenced at unit free */
}

bool TryProcessUnitIdentifierAsType(){
  auto scope = ScopeStack.i;
  auto TypeID = SimplifyType();
  if(TypeID == (TypeList_t::nr_t)-1){
    return false;
  }
  SWAP(scope, ScopeStack.i);
  AddTypeToUnit(TypeID);
  SWAP(scope, ScopeStack.i);
  return true;
}

bool Compile(){
  while(1){
    SkipTillCode();
    if(gc() == '#'){
      #include "PreprocessorStart.h"
    }
    else if(STR_ischar_char(gc()) || gc() == '_'){

      if(TryProcessUnitIdentifierAsType()){
        continue;
      }

      auto iden = GetIdentifier();
      if(!iden.compare("void")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      #if set_support_c99
        else if(!iden.compare("auto")){
          /* TOOD this is type right? put it to type places. */
          /* useless keyword in c99 */
          __abort();
        }
      #endif
      else if(!iden.compare("restrict")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("register")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("nullptr")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("break")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("case")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("const")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("continue")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("default")){
        print_ExpandTrace();
        __abort();
      }
      #if set_support_c99
        else if(!iden.compare("do")){
          print_ExpandTrace();
          __abort();
        }
      #endif
      else if(!iden.compare("if")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("else")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("enum")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("extern")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("false")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("true")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("for")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("goto")){
        print_ExpandTrace();
        __abort();
      }
      #if set_support_c99
        else if(!iden.compare("inline")){
          /* TOOD this is type right? put it to type places. */
          /* useless keyword */
          __abort();
        }
      #endif
      else if(!iden.compare("return")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("sizeof")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("static")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("struct")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("switch")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("typedef")){
        ScopeUnitInc(ScopeData_t::UnitEnum::Typedef);
      }
      else if(!iden.compare("typeof")){
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("union")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("volatile")){
        /* TOOD this is type right? put it to type places. */
        print_ExpandTrace();
        __abort();
      }
      else if(!iden.compare("while")){
        print_ExpandTrace();
        __abort();
      }
      else{
        SkipTillCode();
        ScopeUnitInc(ScopeData_t::UnitEnum::UnknownIdentifier);
        auto u = (ScopeData_t::UnitData_UnknownIdentifier_t *)ScopeUnitData();
        u->str = iden;
      }
    }
    else if(gc() == '['){
      __abort();
    }
    else if(gc() == ';'){
      ic_unsafe();
      ScopeUnitDone();
    }
    else [[unlikely]] {
      errprint_exit("cant identify %lx %c", gc(), gc());
      __unreachable();
    }
  }

  return 0;
}
