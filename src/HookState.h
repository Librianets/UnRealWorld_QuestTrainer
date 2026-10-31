#pragma once
// Read-only adapter for the already loaded translation hook. Never calls remote code.
struct HookStateAdapter {uintptr_t module=0;uint32_t size=0,tlsIndex=0;std::string error;};
HookStateAdapter g_hookState;
bool ReadRaw(uintptr_t address,void* out,size_t bytes){SIZE_T used=0;return
 ReadProcessMemory(g_process,(void*)address,out,bytes,&used)&&used==bytes;}
bool LoadHookState(){
 g_hookState={};HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,g_pid);
 if(snap==INVALID_HANDLE_VALUE)return false;
 MODULEENTRY32W m{};m.dwSize=sizeof(m);
 if(Module32FirstW(snap,&m))do{
  if(!_wcsicmp(m.szModule,L"URWTraceHook.dll")||!_wcsicmp(m.szModule,L"rus_URWTraceHook.dll")){
   g_hookState.module=(uintptr_t)m.modBaseAddr;g_hookState.size=m.modBaseSize;
   // These builds share the audited 116-byte Frame layout, not a public DLL ABI.
   std::string hash=HashFile(m.szExePath);
   if(hash!="653033af7eff76357cf1ba6803c80b5bc467196cc998f9e6cefdaac6aa0948b3"&&
      hash!="e2c7e04a2a28c4ce9e243896d7fa0655837c0a8493cd5d8cdd3063f338ec16b7"&&
      hash!="17b2e24e877a49971b936e311d3d95d840df8225c9c04af30c8230cccb42d052"&&
      hash!="5eb156d73d7b38f2f97b393630c26c5ef58164f5094c7189df9fe8d1cd1c14c3"&&
      hash!="ee89585163cbf0c006deefc690d670ba6aba1093b03792099270b5fc3d521319"&&
      hash!="507033b79b2c8dfffd699772c7ad858526dcf1fa579ff66843f890f1e3c7294d"&&
      hash!="d038ef48f044a2474aa4027257d0585f5adb26841196f586bc79f184ff7e8ece"&&
      hash!="e70dcecb0a0b892a349a77b721ceba7eecb292cb7607a5a7c0853a0c672501f5"&&
      hash!="17eca3112040eb0677759f700a4f8b25b32896b3f21b9c2ab5e150546938bb0a"&&
      hash!="a6958a184ff3682f62cdd43bf4f59df3e6a620af054a6a803430c4a8be920c62"&&
      hash!="6f6fe297e152c096985242ddd8e5e6c2118721cb84e3982fd48fb798d0eefda7"){
    g_hookState.error="Unsupported loaded translation-hook build";break;
   }
   // Resolve the hook's private TLS index from a checked x86 instruction pair:
   // push [g_tls]; call [IAT TlsGetValue]. ASLR is read from the loaded image.
   IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS32 nt{};
   if(!ReadRaw(g_hookState.module,&dos,sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||
      !ReadRaw(g_hookState.module+dos.e_lfanew,&nt,sizeof(nt))||nt.Signature!=IMAGE_NT_SIGNATURE){g_hookState.error="Invalid hook PE headers";break;}
   uint32_t importRva=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
   uintptr_t tlsIat=0;
   for(unsigned d=0;d<128&&!tlsIat;++d){
    IMAGE_IMPORT_DESCRIPTOR desc{};if(!ReadRaw(g_hookState.module+importRva+sizeof(desc)*d,&desc,sizeof(desc))||!desc.Name)break;
    uintptr_t names=g_hookState.module+(desc.OriginalFirstThunk?desc.OriginalFirstThunk:desc.FirstThunk);
    for(unsigned i=0;i<4096;++i){
     uint32_t rva=0;if(!ReadRaw(names+4*i,&rva,4)||!rva)break;if(rva&IMAGE_ORDINAL_FLAG32)continue;
     char name[64]{};if(ReadRaw(g_hookState.module+rva+2,name,sizeof(name)-1)&&!strcmp(name,"TlsGetValue")){tlsIat=g_hookState.module+desc.FirstThunk+4*i;break;}
    }
   }
   std::vector<uintptr_t> globals;
   uintptr_t sections=g_hookState.module+dos.e_lfanew+24+nt.FileHeader.SizeOfOptionalHeader;
   for(unsigned i=0;i<nt.FileHeader.NumberOfSections&&tlsIat;++i){
    IMAGE_SECTION_HEADER sec{};if(!ReadRaw(sections+i*sizeof(sec),&sec,sizeof(sec)))break;
    if(!(sec.Characteristics&IMAGE_SCN_MEM_EXECUTE)||sec.Misc.VirtualSize>g_hookState.size)continue;
    std::vector<unsigned char> code(sec.Misc.VirtualSize);
    if(!ReadRaw(g_hookState.module+sec.VirtualAddress,code.data(),code.size()))continue;
    for(size_t j=0;j+12<=code.size();++j){uint32_t at=0;
     if(code[j]!=0xFF||code[j+1]!=0x35||code[j+6]!=0xFF||code[j+7]!=0x15)continue;
     memcpy(&at,code.data()+j+8,4);if(at!=tlsIat)continue;
     memcpy(&at,code.data()+j+2,4);
     if(at>=g_hookState.module&&at+4<=g_hookState.module+g_hookState.size)globals.push_back(at);
    }
   }
   std::sort(globals.begin(),globals.end());globals.erase(std::unique(globals.begin(),globals.end()),globals.end());
   uint32_t index=0;
   if(globals.size()!=1||!ReadRaw(globals.front(),&index,4)||index>=1088)g_hookState.error="Hook TLS index not verified";
   else g_hookState.tlsIndex=index;
   break;
  }
 }while(Module32NextW(snap,&m));
 CloseHandle(snap);return g_hookState.error.empty();
}
using NativeReturns=std::vector<std::pair<uintptr_t,uint32_t>>;
bool ReadHookReturns(HANDLE thread,uintptr_t sp,NativeReturns& out){
 if(!g_hookState.module)return true;
 struct ThreadBasic {LONG status;void* teb;HANDLE process,thread;ULONG_PTR affinity;LONG priority,basePriority;};
 auto query=(LONG(NTAPI*)(HANDLE,int,void*,ULONG,ULONG*))GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationThread");
 ThreadBasic info{};if(!query||query(thread,0,&info,sizeof(info),nullptr)<0)return false;
 uintptr_t teb=(uintptr_t)info.teb;uint32_t self=0,stackTop=0,state=0;
 // Verify that NtQueryInformationThread returned the x86 TEB under WOW64.
 if(!ReadRaw(teb+0x18,&self,4)||self!=teb||!ReadRaw(teb+4,&stackTop,4)||sp>=stackTop)return false;
 if(g_hookState.tlsIndex<64){if(!ReadRaw(teb+0xE10+4*g_hookState.tlsIndex,&state,4))return false;}
 else{uint32_t slots=0;if(!ReadRaw(teb+0xF94,&slots,4)||!slots||!ReadRaw(slots+4*(g_hookState.tlsIndex-64),&state,4))return false;}
 if(!state)return true;
 uint32_t header[2]{};if(!ReadRaw(state,header,sizeof(header))||(header[0]&0xFF)||header[1]>512)return false;
 constexpr size_t stride=116;
 std::vector<unsigned char> frames(header[1]*stride);
 if(!frames.empty()&&!ReadRaw(state+8,frames.data(),frames.size()))return false;
 for(unsigned i=0;i<header[1];++i){
  uint32_t fields[6]{};memcpy(fields,frames.data()+i*stride,sizeof(fields));
  uintptr_t slot=fields[1];uint32_t original=fields[2],site=fields[3],raw=0,bridge=0;
  if(slot<sp||slot+4>stackTop||original!=site)continue;
  if(!ReadRaw(slot,&raw,4)||raw<g_hookState.module||raw+4>g_hookState.module+g_hookState.size)continue;
  // The saved continuation must still be the actual ReturnBridge entry.
  if(!ReadRaw(raw,&bridge,4)||bridge!=0x609C006A)continue;
  if(original>=g_base&&original<g_base+0x18C000)out.push_back({slot,original});
 }
 return true;
}
uint32_t NativeReturn(const NativeReturns& values,uintptr_t slot,uint32_t raw){
 for(const auto& value:values)if(value.first==slot)return value.second;
 return raw;
}
