bool InsidePreprocessor = false;
bool InsideQuote = false;

/* dont make this bool. x86 assembly has small and fast `inc` instruction. */
uintptr_t SpaceCombo = 0;

uintptr_t LastEndLine = 0;

uintptr_t tp_size = file_input_size - 1;
for(uintptr_t i = 0; EXPECT(i < tp_size, true);){
  if(tp[i] == '\\' && tp[i + 1] == '\n'){
    i += 2;
  }
  else if(tp[i] == '\r'){
    i += 1;
  }
  else if(tp[i] == '#' && !InsidePreprocessor){
    InsidePreprocessor = true;
    SpaceCombo = 0;
    p[p_size++] = tp[i++];
  }
  else if(tp[i] == '\n'){
    if(InsidePreprocessor){
      p[p_size++] = tp[i++];
      LastEndLine = p_size;
      InsidePreprocessor = false;
    }
    else{
      i++;
      while(tp[i] == ' ' || tp[i] == '\t'){
        i++;
      }
      InsidePreprocessor = tp[i] == '#';
      if(InsidePreprocessor && LastEndLine != p_size){
        p[p_size++] = '\n';
        LastEndLine = p_size;
      }
    }
    SpaceCombo = 0;
  }
  else if(tp[i] == '"' && !InsidePreprocessor){
    if(SpaceCombo){
      p[p_size++] = ' ';
      SpaceCombo = 0;
    }
    p[p_size++] = tp[i++];
    InsideQuote ^= 1;
  }
  else if(tp[i] == '\t' && !InsideQuote){
    SpaceCombo++;
    i++;
  }
  else if(tp[i] == ' ' && !InsideQuote){
    SpaceCombo++;
    i++;
  }
  else if(tp[i] == '/' && tp[i + 1] == '/' && !InsideQuote){
    InsidePreprocessor = false;
    i += 2;
    while(EXPECT(i < tp_size, true)){
      if(tp[i] == '\\' && tp[i + 1] == '\n'){
        i += 2;
      }
      else if(tp[i] == '\n'){
        ++i;
        break;
      }
      else{
        ++i;
      }
    }
  }
  else if(tp[i] == '/' && tp[i + 1] == '*' && !InsideQuote){
    i += 2;
    while(EXPECT(i < tp_size, true)){
      if(tp[i] == '\\' && tp[i + 1] == '\n'){
        i += 2;
      }
      else if(tp[i] == '*' && tp[i + 1] == '/'){
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
      p[p_size++] = ' ';
      SpaceCombo = 0;
    }
    p[p_size++] = tp[i++];
  }
}

if(tp[tp_size] != '\\'){
  p[p_size++] = tp[tp_size];
}
else{
  /* tcc gives `error: declaration expected` */
  /* gcc gives `warning: backslash-newline at end of file` */
  /* clang doesnt say anything */
}
