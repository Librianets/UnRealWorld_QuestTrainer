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
   if(hash!="251d3eb86e0ee4fb6be64d2308442b885e8a4bc70ca60787b9df4ab71107bb11"&&hash!="73310b80255b78dfac69a61f301a7b29d4f63d1f8461eeabb3a75b2559e3573e"&&hash!="0def38528e9f78bf31b1c69e17f996005cffdb33959e8c4a5435dd83545d58ec"&&hash!="c21f8eb7d7ecec575bc646b669a71a749014475efc334bb57513be1afd8d1407"&&hash!="e7ead9f9ee4a3bf93898a480be59fc64bd4dfcf4195815c1d122873a30b831fb"&&hash!="90552c12d8e1262d65e9504e40ad98647b49207c1bba0a7439947e5432de5693"&&hash!="2de5ff8336f68225ee467a1f0ad78348dd980ed138ba56c3309ead0d3e530935"&&hash!="fe2ca5c00569728fe70cfd9433fead6e0f055f68a29798603f685aa69c8fdb4c"&&hash!="e8d28f0b3ca5101609e3d3ef044ecdf0480f0f0a9f801377106fb6ba40fc64e1"&&hash!="458a1e79b9bd1b734245209bc7027b7f0915080bccf0bc3e3238a1b95f28cf70"&&hash!="bf4464c1c39c6f5d8a61a518468b88ecd7b7a33265871d8941ae519fa69c9147"&&hash!="1e04a22bcad5a541867bb8d4b28e7905d6db00e99aaa73664830d1e37d8ad668"&&hash!="afeb7e49a1c786183743ba841542226e6b16108be77041ef45679fff6053d195"&&hash!="c3e08dbabd375415bb46d9d6eca4d3e16ffb37d7bc231312915cc9aca6e57e23"&&hash!="ae0399d508a476412f44d1746f73837b5cf124334d73bb2c61ad009e4b172580"&&hash!="bd093c7a4e40e7392a1780cb3fc473f111b88d4159a25867454a4b10319a4280"&&hash!="f66d0c0c3418d0b1336acbd2527421b7845400237c25e01897c724876ad19be0"&&hash!="5c97b2b133605ca5c3ea840bf0af0ca1faa74954542b0aac8643977ad5bd001e"&&hash!="2f9e4acac6561ec56c490081514e9d0cbcd993ea7da0b5d2854796f68621e01b"&&hash!="f5609511b23f6832152a9582d8a08ddc34734d8b7d89558abc63e5c185829480"&&hash!="a9120a5c0c7d5d41d2adfc79c6c9321853aec8e23123532624df1cb37dce003a"&&hash!="f6c115a0fe954a193b320202880338b98e2ee4692dea97ca71f08e8cf38c3f70"&&hash!="7796843da0cc547d16b492d71b5da74bdbf758cd7cbf4601ba4530d2f6616b88"&&hash!="cc3d358b68f04303a8045f778843186b67f07eff4e211aad0ebda4b19b538b41"&&hash!="5d72dac0be6a9a10830396db4ed734372b358270f23118619623655a30a768cf"&&hash!="eaf2f0a521865703a5388a64eadbbf85cd54f8a6514cbc65a334a9b8ee943708"&&hash!="7942b3cdefefd943f1789d477fe8d0f226aff0ef883c09b7a420a19f3596b56d"&&hash!="d734cefd1038a68ef6e8a6f1ecbf392b0d117c29c17f8262a26e04c47ab155ef"&&hash!="b498a68b123f24dbafdd0320558439004994728c0095acd8158ac5ea5ecd9c19"&&hash!="cfa2a51790d5c33d9714ff48d38c2cb3f6a5b9415ba8736379ccf1952bea3f71"&&hash!="463808387f59d1cdee8dd5680aec62b1a5d24d3945b1db5edb7435b2d4d46139"&&hash!="cda19c7ccc9224f6e382930b0016449d096c0ee29706820effe317be1beb5a23"&&hash!="653033af7eff76357cf1ba6803c80b5bc467196cc998f9e6cefdaac6aa0948b3"&&
      hash!="e2c7e04a2a28c4ce9e243896d7fa0655837c0a8493cd5d8cdd3063f338ec16b7"&&
      hash!="17b2e24e877a49971b936e311d3d95d840df8225c9c04af30c8230cccb42d052"&&
      hash!="5eb156d73d7b38f2f97b393630c26c5ef58164f5094c7189df9fe8d1cd1c14c3"&&
      hash!="ee89585163cbf0c006deefc690d670ba6aba1093b03792099270b5fc3d521319"&&
      hash!="507033b79b2c8dfffd699772c7ad858526dcf1fa579ff66843f890f1e3c7294d"&&
      hash!="d038ef48f044a2474aa4027257d0585f5adb26841196f586bc79f184ff7e8ece"&&
      hash!="e70dcecb0a0b892a349a77b721ceba7eecb292cb7607a5a7c0853a0c672501f5"&&
      hash!="17eca3112040eb0677759f700a4f8b25b32896b3f21b9c2ab5e150546938bb0a"&&
      hash!="a6958a184ff3682f62cdd43bf4f59df3e6a620af054a6a803430c4a8be920c62"&&
      hash!="6f6fe297e152c096985242ddd8e5e6c2118721cb84e3982fd48fb798d0eefda7"&&
      hash!="cde4e9ad9015aa4ad5862b4c6be6c155914746dbd2c6ad3624f37b8fb3a2442a"&&
      hash!="db0e25263eac21fc0fce0a9c1da1a4aabe17d868482607fb63c9b7d49d9fe4ca"&&
      hash!="1bc1f5899b9321b73ddb484e03b4e011d2dfe500d08d2b6c39c8b3c2fb3f1796"&&
      hash!="cf6514ddeaf39ac3112147a731ca7244046fdbb401e4aa2c0001a786d7db058e"){
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
