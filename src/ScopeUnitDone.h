if(!s.UnitIndex){
  break;
}

if(s.UnitArray[0].ue == ScopeData_t::UnitEnum::Typedef){
  if(s.UnitIndex != 3){
    errprint_exit("keyword typedef requires 3 unit.");
  }
  if(s.UnitArray[1].ue != ScopeData_t::UnitEnum::Type){
    errprint_exit("typedef requires Type as Unit1. got %x", (uintptr_t)s.UnitArray[1].ue);
  }
  if(s.UnitArray[2].ue != ScopeData_t::UnitEnum::UnknownIdentifier){
    errprint_exit("typedef requires UnknownIdentifier as Unit2");
  }
  IdentifierPointType(((ScopeData_t::UnitData_UnknownIdentifier_t *)ScopeUnitData(2))->str, ((ScopeData_t::UnitData_Type_t *)ScopeUnitData(1))->tid);
}
else{
  __abort();
}
