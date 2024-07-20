if(file_input_size){
  bool InsidePreprocessor = false;
  bool InsideQuote = false;

  /* dont make this bool. x86 assembly has small and fast `inc` instruction. */
  uintptr_t SpaceCombo = 0;

  uintptr_t LastEndLine = 0;

  uintptr_t i = 0;

  #ifdef set_SimplifyFile_Info
    Line = 0;

    struct p_t{
      uint8_t p;
      uint8_t &operator[](uintptr_t){
        return p;
      }
    }p;
    uintptr_t p_size = 0;
  #endif
  auto ps = [&]<uintptr_t plus>() -> uintptr_t {
    auto r = p_size;
    if constexpr(plus != 0){
      p_size += plus;
      #ifdef set_SimplifyFile_Info
        if(p_size == coffset){
          i = file_input_size;
        }
      #endif
    }
    return r;
  };

  uintptr_t orig_size = file_input_size - 1;
  while(EXPECT(i < orig_size, true)){
    if(orig_p[i] == '\\' && orig_p[i + 1] == '\n'){
      #ifdef set_SimplifyFile_Info
        Line++;
      #endif
      i += 2;
    }
    else if(orig_p[i] == '\r'){
      i += 1;
    }
    else if(orig_p[i] == '#' && !InsidePreprocessor){
      InsidePreprocessor = true;
      SpaceCombo = 0;
      p[ps.operator()<1>()] = orig_p[i++];
    }
    else if(orig_p[i] == '\n'){
      #ifdef set_SimplifyFile_Info
        Line++;
      #endif
      if(InsidePreprocessor){
        p[ps.operator()<1>()] = orig_p[i++];
        LastEndLine = p_size;
        InsidePreprocessor = false;
      }
      else{
        while(orig_p[++i] == ' ' || orig_p[i] == '\t');
        InsidePreprocessor = orig_p[i] == '#';
        if(InsidePreprocessor && LastEndLine != p_size){
          p[ps.operator()<1>()] = '\n';
          LastEndLine = p_size;
        }
      }
      SpaceCombo = 0;
    }
    else if(orig_p[i] == '"' && !InsidePreprocessor){
      if(SpaceCombo){
        p[ps.operator()<1>()] = ' ';
        SpaceCombo = 0;
      }
      p[ps.operator()<1>()] = orig_p[i++];
      InsideQuote ^= 1;
    }
    else if(orig_p[i] == '\t' && !InsideQuote){
      SpaceCombo++;
      i += 1;
    }
    else if(orig_p[i] == ' ' && !InsideQuote){
      SpaceCombo++;
      i += 1;
    }
    else if(orig_p[i] == '/' && orig_p[i + 1] == '/' && !InsideQuote){
      InsidePreprocessor = false;
      i += 2;
      while(EXPECT(i < orig_size, true)){
        if(orig_p[i] == '\\' && orig_p[i + 1] == '\n'){
          #ifdef set_SimplifyFile_Info
            Line++;
          #endif
          i += 2;
        }
        else if(orig_p[i] == '\n'){
          #ifdef set_SimplifyFile_Info
            Line++;
          #endif
          ++i;
          break;
        }
        else{
          ++i;
        }
      }
    }
    else if(orig_p[i] == '/' && orig_p[i + 1] == '*' && !InsideQuote){
      i += 2;
      while(EXPECT(i < orig_size, true)){
        if(orig_p[i] == '\n'){
          #ifdef set_SimplifyFile_Info
            Line++;
          #endif
          i += 1;
          if(InsidePreprocessor){
            p[ps.operator()<1>()] = '\n';
          }
          InsidePreprocessor = false;
        }
        else if(orig_p[i] == '\\' && orig_p[i + 1] == '\n'){
          #ifdef set_SimplifyFile_Info
            Line++;
          #endif
          i += 2;
        }
        else if(orig_p[i] == '*' && orig_p[i + 1] == '/'){
          i += 2;
          break;
        }
        else{
          ++i;
        }
      }
    }
    else{
      if(SpaceCombo){
        p[ps.operator()<1>()] = ' ';
        SpaceCombo = 0;
      }
      p[ps.operator()<1>()] = orig_p[i++];
    }
  }

  if(orig_p[orig_size] != '\\'){
    p[ps.operator()<1>()] = orig_p[orig_size];
  }
  else{
    /* tcc gives `error: declaration expected` */
    /* gcc gives `warning: backslash-newline at end of file` */
    /* clang doesnt say anything */
  }
}

#ifdef set_SimplifyFile_Info
  #undef set_SimplifyFile_Info
#endif
