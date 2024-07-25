ic_unsafe();
SkipEmptyInLine();

auto Identifier = GetIdentifier();
if(!Identifier.compare("eof")){
  return 0;
}
else if(!Identifier.compare("error")){
  const uint8_t *p;
  const uint8_t *s;
  ReadLineBeautifully0(&p, &s);
  errprint_exit("error: #error %.*s", (uintptr_t)s - (uintptr_t)p, p);
  ic_endline();
}
else if(!Identifier.compare("warning")){
  const uint8_t *p;
  const uint8_t *s;
  ReadLineBeautifully0(&p, &s);
  printwi("warning: #warning %.*s", (uintptr_t)s - (uintptr_t)p, p);
  ic_endline();
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
      _DeexpandFile();
      _Deexpand();
    }
  }
  else{
    __abort();
  }
}
else if(!Identifier.compare("define")){
  SkipEmptyInLine();
  auto defiden = GetIdentifier();

  auto dmid = DefineDataMap.find(defiden);
  if(dmid != DefineDataMap.end()){
    if(settings.Wmacro_redefined){
      printwi("macro is redefined \"%.*s\"", (uintptr_t)defiden.size(), defiden.data());
      printstderr("previously defined at\n");
      auto &dd = DefineDataList[dmid->second];
      print_TraceDataLink(dd.TraceCount, dd.DataLinkID);
      printstderr("\n");
    }
    DefineDataMap.erase(defiden);
  }

  auto ddid = DefineDataList.NewNode();
  auto &d = DefineDataList[ddid];

  ExpandSync();
  #if __sanit
    d.DataLinkID.sic();
  #endif
  d.TraceCount = 0;
  for(uintptr_t eti = 0; ++eti < ExpandTrace.Usage();){
    d.TraceCount++;
    auto tdlid = d.DataLinkID;
    d.DataLinkID = DataLink.NewNode();
    DataLink[d.DataLinkID].dlid = tdlid;
    DataLink[d.DataLinkID].DataID = ExpandTrace[eti].DataID;
    DataLink[d.DataLinkID].Offset = ExpandTrace[eti].i;
  }

  d.isfunc = false;
  if(gc() == '('){
    d.isfunc = true;
    ic_unsafe();
    GetDefineParams(d.Inputs, d.va_args);
  }
  else if(ischar_empty()){
    ic_unsafe();
  }
  else if(gc() != '\n'){
    __abort();
  }

  SkipEmptyInLine();
  d.op = &gc();
  while(gc() != '\n'){
    ic_unsafe();
  }
  d.os = &gc();
  ic();

  DefineDataMap[defiden] = ddid;
}
else if(!Identifier.compare("ifdef")){
  PreprocessorIf(0, 0);
}
else if(!Identifier.compare("ifndef")){
  PreprocessorIf(0, 1);
}
else if(!Identifier.compare("if")){
  PreprocessorIf(0, 2);
}
else if(!Identifier.compare("elif")){
  PreprocessorIf(1, 2);
}
else if(!Identifier.compare("else")){
  PreprocessorIf(2, 3);
}
else if(!Identifier.compare("endif")){
  PreprocessorIf(3, 0);
}
else if(!Identifier.compare("undef")){
  SkipEmptyInLine();
  auto defiden = GetIdentifier();
  SkipCurrentEmptyLine();
  if(DefineDataMap.find(defiden) == DefineDataMap.end()){
    if(settings.Wmacro_not_defined){
      printwi("warning, Wmacro-not-defined %.*s",
        (uintptr_t)defiden.size(), defiden.data()
      );
    }
  }
  else{
    DefineDataMap.erase(defiden);
  }
}
else{
  errprint_exit("unknown preprocessor identifier. %.*s",
    (uintptr_t)Identifier.size(), Identifier.data()
  );
}
