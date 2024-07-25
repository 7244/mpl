auto rdatato = rdataend - 1;

if((uintptr_t)rdatato > (uintptr_t)rdata){
  bool InsidePreprocessor = false;
  bool InsideQuote = false;

  /* dont make this bool. x86 assembly has small and fast `inc` instruction. */
  uintptr_t SpaceCombo = 0;

  #ifdef set_SimplifyFile_Info
    Line = 0;

    struct sdata_element_t{
      void operator=(const uint8_t){}
    };
    struct sdata_t{
      const uint8_t *ptr;
      sdata_element_t operator*(){
        return {};
      }
      bool operator!=(const sdata_t &){
        return false;
      }
    }sdata = {
      .ptr = sdatabegin
    };
  #endif
  auto spp = [&](){
    #ifdef set_SimplifyFile_Info
      sdata.ptr++;
      if(sdata.ptr == coffset){
        rdata = rdatato;
      }
    #else
      sdata++;
    #endif
  };

  auto LastEndLine = sdata;

  while(EXPECT(rdata < rdatato, true)){
    if(rdata[0] == '\\' && rdata[1] == '\n'){
      #ifdef set_SimplifyFile_Info
        Line++;
      #endif
      rdata += 2;
    }
    else if(rdata[0] == '\r'){
      rdata += 1;
    }
    else if(rdata[0] == '#' && !InsidePreprocessor){
      InsidePreprocessor = true;
      SpaceCombo = 0;
      *sdata = *rdata++;
      spp();
    }
    else if(rdata[0] == '\n'){
      #ifdef set_SimplifyFile_Info
        Line++;
      #endif
      if(InsidePreprocessor){
        *sdata = *rdata++;
        spp();
        LastEndLine = sdata;
        InsidePreprocessor = false;
      }
      else{
        while(*++rdata == ' ' || rdata[0] == '\t');
        InsidePreprocessor = rdata[0] == '#';
        if(InsidePreprocessor && LastEndLine != sdata){
          *sdata = '\n';
          spp();
          LastEndLine = sdata;
        }
      }
      SpaceCombo = 0;
    }
    else if(rdata[0] == '"' && !InsidePreprocessor){
      if(SpaceCombo){
        *sdata = ' ';
        spp();
        SpaceCombo = 0;
      }
      *sdata = *rdata++;
      spp();
      InsideQuote ^= 1;
    }
    else if(rdata[0] == '\t' && !InsideQuote){
      SpaceCombo++;
      rdata += 1;
    }
    else if(rdata[0] == ' ' && !InsideQuote){
      SpaceCombo++;
      rdata += 1;
    }
    else if(rdata[0] == '/' && rdata[1] == '/' && !InsideQuote){
      InsidePreprocessor = false;
      rdata += 2;
      while(EXPECT(rdata < rdatato, true)){
        if(rdata[0] == '\\' && rdata[1] == '\n'){
          #ifdef set_SimplifyFile_Info
            Line++;
          #endif
          rdata += 2;
        }
        else if(rdata[0] == '\n'){
          #ifdef set_SimplifyFile_Info
            Line++;
          #endif
          ++rdata;
          break;
        }
        else{
          ++rdata;
        }
      }
    }
    else if(rdata[0] == '/' && rdata[1] == '*' && !InsideQuote){
      rdata += 2;
      if(InsidePreprocessor){
        while(EXPECT(rdata < rdatato, true)){
          if(rdata[0] == '\n'){
            #ifdef set_SimplifyFile_Info
              Line++;
            #endif
            rdata += 1;
            if(*(rdata - 2) != '\\'){
              *sdata = '\n';
              spp();
              InsidePreprocessor = false;
              break;
            }
          }
          else if(rdata[0] == '*'){
            ++rdata;
            if(rdata[0] == '/'){
              ++rdata;
              goto gt_end;
            }
          }
          else{
            ++rdata;
          }
        }
      }
      while(EXPECT(rdata < rdatato, true)){
        if(0);
        #ifdef set_SimplifyFile_Info
          else if(rdata[0] == '\n'){
            Line++;
            rdata += 1;
          }
        #endif
        else if(rdata[0] == '*'){
          ++rdata;
          if(rdata[0] == '/'){
            ++rdata;
            break;
          }
        }
        else{
          ++rdata;
        }
      }
    }
    else{
      if(SpaceCombo){
        *sdata = ' ';
        spp();
        SpaceCombo = 0;
      }
      *sdata = *rdata++;
      spp();
    }

    gt_end:;
  }

  if(*rdatato != '\\'){
    *sdata = *rdatato;
    spp();
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
