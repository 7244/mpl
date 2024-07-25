uintptr_t tp_size = file_input_size - 1;
for(uintptr_t i = 0; EXPECT(i < tp_size, true);){
  if(tp[i] == '\\' && tp[i + 1] == '\n'){
    i += 2;
    continue;
  }
  if(tp[i] == '\r'){
    i += 1;
    continue;
  }
  p[p_size++] = tp[i++];
}

if(tp[tp_size] != '\\'){
  p[p_size++] = tp[tp_size];
}
else{
  /* tcc gives `error: declaration expected` */
  /* gcc gives `warning: backslash-newline at end of file` */
  /* clang doesnt say anything */
}
