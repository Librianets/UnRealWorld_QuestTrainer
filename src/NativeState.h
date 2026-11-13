#pragma once
// SHA-locked3.89.1. Optional skill-counter write requires an explicit writer handle
// and a proven live skill-allocation input frame, while the thread is suspended.
enum class NativeStage { Unknown, Main, Minus, LetterJ, Command, DebugText,
                         Npc, QuestQuestion, ItemName, ItemQuantity, Pickup,
                         PickupCount, PickupCountText, DayCount, Course, CourseReward, SkillAllocation, AnimalTarget, AnimalSpecies };
struct NativeState { uintptr_t chatFrame=0; uint32_t animalX=0,animalY=0;bool animalCursor=false;uintptr_t animalFrame=0;uint32_t animalReturn=0;uintptr_t skillFrame=0;int skillPoints=-1;bool skillWriteAttempted=false,skillWriteOkay=false;NativeStage stage=NativeStage::Unknown; uintptr_t debugFrame=0;uint32_t debugCall=0,readerCall=0,hookReturns=0;
                     std::vector<uint32_t> returns; };
NativeState ProbeNative(HANDLE skillWriter=nullptr,int freezeTarget=0){
 NativeState out;uintptr_t provenDebugFrame=0;bool readerConflict=false,courseReward=false,skillReader=false;HANDLE thread=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,g_thread);
 if(!thread)return out;
 if(SuspendThread(thread)==DWORD(-1)){CloseHandle(thread);return out;}
 // Resume before returning on every path. Context/stack are only observed.
 CONTEXT c{};c.ContextFlags=CONTEXT_CONTROL;
 if(GetThreadContext(thread,&c)){
  NativeReturns native;
  if(!ReadHookReturns(thread,c.Esp,native)){ResumeThread(thread);CloseHandle(thread);return out;}
  out.hookReturns=(uint32_t)native.size();
  uintptr_t fp=c.Ebp;
  for(int n=0;n<96 && fp>=c.Esp && fp-c.Esp<0x1000000;++n){
   uint32_t frame[2]{};if(!ReadRaw(fp,frame,sizeof(frame)))break;
   uint32_t ret=NativeReturn(native,fp+4,frame[1])-static_cast<uint32_t>(g_base)+0x400000;
   out.returns.push_back(ret);if(ret==0x55DC04)courseReward=true;
   if(ret==0x57D735)out.debugFrame=fp;
   if(frame[0]<=fp||frame[0]-fp>0x1000000)break;fp=frame[0];
  }
  // FPO functions between SDL and the game do not all form an EBP chain.
  // Scan only active stack, capped at the debug call slot (before its 135KB locals).
  uintptr_t limit=out.debugFrame?((out.debugFrame&~uintptr_t(15))-0x21CD0):c.Esp+0x8000;
  limit=std::min(limit,uintptr_t(c.Esp)+0x8000);
  for(uintptr_t page=c.Esp;page<limit;){
   MEMORY_BASIC_INFORMATION mi{};if(!VirtualQueryEx(g_process,(void*)page,&mi,sizeof(mi)))break;
   uintptr_t end=std::min<uintptr_t>(limit,(uintptr_t)mi.BaseAddress+mi.RegionSize);
   if(end<=page||mi.State!=MEM_COMMIT||(mi.Protect&(PAGE_NOACCESS|PAGE_GUARD)))break;
   std::vector<uint32_t> data((end-page)/4);
   if(!data.empty()&&ReadRaw(page,data.data(),data.size()*4))
    for(size_t i=0;i<data.size();++i){uint32_t va=NativeReturn(native,page+4*i,data[i])-static_cast<uint32_t>(g_base)+0x400000;
     if(va>=0x400000&&va<0x58C000)out.returns.push_back(va);
     // Reward55D2C0 resets the course index before waiting for a choice.
     // Prove its live frame with the native security cookie, not a loose
     // return-address-shaped word in old stack data.
     if(va==0x55DC04){
      uintptr_t rewardFrame=page+4*i-4;uint32_t cookieAt=0,cookie=0,stored=0;
      if(Read(0x55D2CA,&cookieAt,4)&&ReadRaw(cookieAt,&cookie,4)&&
         ReadRaw(rewardFrame-4,&stored,4)&&stored==(cookie^(uint32_t)rewardFrame))courseReward=true;
     }
     // A word that resembles a return address can be old data in native locals.
     // Anchor the current command reader using4D1C00's security cookie and
     // its verified EBP parent4D3BF0. Never infer Main from an arbitrary word.
     if(va==0x4D3C01){
      uintptr_t inputFrame=page+4*i-4;uint32_t frames[2]{},cookieAt=0,cookie=0,stored=0;
      uint32_t readerFrame=0,caller=0,parent=0;
      if(ReadRaw(inputFrame,frames,8)&&Read(0x4D1C0A,&cookieAt,4)&&
         ReadRaw(cookieAt,&cookie,4)&&ReadRaw(inputFrame-4,&stored,4)&&stored==(cookie^(uint32_t)inputFrame)){
       readerFrame=frames[0];
       if(readerFrame>inputFrame&&readerFrame-inputFrame<0x2000&&ReadRaw(readerFrame+4,&caller,4)){
        caller=NativeReturn(native,readerFrame+4,caller)-(uint32_t)g_base+0x400000;
        if(out.readerCall&&out.readerCall!=caller)readerConflict=true;
        out.readerCall=caller;
        if(caller==0x4D12B4){
         uint32_t menu=0,ret=0,chat=0,secret=0,chatCookie=0;
         if(ReadRaw(readerFrame,&menu,4)&&menu>readerFrame&&menu-readerFrame<0x2000&&
            ReadRaw(menu+4,&ret,4)&&NativeReturn(native,menu+4,ret)==g_base+0x1427FA&&
            ReadRaw(menu,&chat,4)&&chat>menu&&chat-menu<0x10000&&
            Read(0x5E200C,&secret,4)&&ReadRaw(chat-4,&chatCookie,4)&&chatCookie==(secret^chat))out.chatFrame=chat;
         uint32_t menuFrame=0,menuCaller=0,allocationFrame=0,allocationCaller=0;
         uint32_t allocationCookieAt=0,allocationSecret=0,menuCookie=0,allocationCookie=0;
         if(ReadRaw(readerFrame,&menuFrame,4)&&menuFrame>readerFrame&&menuFrame-readerFrame<0x2000&&
            ReadRaw(menuFrame+4,&menuCaller,4)&&NativeReturn(native,menuFrame+4,menuCaller)==g_base+0xE0ED7&&
            ReadRaw(menuFrame,&allocationFrame,4)&&allocationFrame>menuFrame&&allocationFrame-menuFrame<0x2000&&
            ReadRaw(allocationFrame+4,&allocationCaller,4)&&NativeReturn(native,allocationFrame+4,allocationCaller)==g_base+0x121BA&&
            Read(0x4E0B4A,&allocationCookieAt,4)&&ReadRaw(allocationCookieAt,&allocationSecret,4)&&
            ReadRaw(menuFrame-4,&menuCookie,4)&&menuCookie==(allocationSecret^menuFrame)&&
            ReadRaw(allocationFrame-4,&allocationCookie,4)&&allocationCookie==(allocationSecret^allocationFrame)){
          out.skillFrame=allocationFrame;skillReader=true;
         }
        }
        // setnpc: prove the live input reader -> modal -> debug frame chain.
        // The modal security cookie prevents matching stale return-shaped stack data.
        if(caller==0x50F71E||caller==0x4F7DD9){
         uint32_t modal=0,modalReturn=0,debug=0,debugReturn=0,secret=0,modalCookie=0;
         bool text=caller==0x50F71E;
         if(ReadRaw(readerFrame,&modal,4)&&modal>readerFrame&&modal-readerFrame<0x2000&&
            ReadRaw(modal+4,&modalReturn,4)&&NativeReturn(native,modal+4,modalReturn)==g_base+(text?0x1754C7:0x1754A8)&&
            ReadRaw(modal,&debug,4)&&debug>modal&&debug-modal<0x40000&&
            ReadRaw(debug+4,&debugReturn,4)&&NativeReturn(native,debug+4,debugReturn)==g_base+0x17D735&&
            Read(0x5E200C,&secret,4)){
          uintptr_t cookieBase=text?modal:((modal&~uintptr_t(63))-0xB4);
          uintptr_t cookieSlot=text?modal-4:cookieBase+0xB0;
          if(ReadRaw(cookieSlot,&modalCookie,4)&&modalCookie==(secret^(uint32_t)cookieBase)){
           out.animalFrame=debug;out.animalReturn=text?0x5754C7:0x5754A8;
           if(!text){uint32_t x=0,y=0;out.animalCursor=ReadRaw(cookieBase+0x14,&out.animalX,4)&&ReadRaw(cookieBase+0x20,&out.animalY,4)&&Read(0x2785454,&x,4)&&Read(0x2D86894,&y,4)&&x==out.animalX&&y==out.animalY;}
          }
         }
        }
        if(caller>=0x56FE50&&caller<0x57CC80&&ReadRaw(readerFrame,&parent,4)){
         uint32_t ret=0;if(parent>readerFrame&&ReadRaw(parent+4,&ret,4)&&NativeReturn(native,parent+4,ret)==g_base+0x17D735)provenDebugFrame=parent;
        }
       }
      }
     }
     // SDL can use EBP as an ordinary register. Recover the native debug frame
     // from its fixed aligned local allocation and verify its *actual* caller.
     if(!out.debugFrame&&va>=0x56FE55&&va<0x57CC80){
      uintptr_t aligned=page+4*i+0x21CD4;
      if(!(aligned&15))for(unsigned delta=0;delta<16;delta+=4){
       uint32_t ret=0;if(ReadRaw(aligned+delta+4,&ret,4)&&NativeReturn(native,aligned+delta+4,ret)==g_base+0x17D735){out.debugFrame=aligned+delta;break;}
      }
     }
    }
   page=end;
  }
  if(readerConflict){out.chatFrame=0;out.animalFrame=0;out.animalReturn=0;out.readerCall=0;provenDebugFrame=0;skillReader=false;out.skillFrame=0;}
  if(provenDebugFrame)out.debugFrame=provenDebugFrame;
  if(out.animalFrame&&!readerConflict)out.debugFrame=out.animalFrame;
  auto has=[&](uint32_t va){return std::find(out.returns.begin(),out.returns.end(),va)!=out.returns.end();};
  if(out.debugFrame){
   uint32_t call=0;uintptr_t slot=(out.debugFrame&~uintptr_t(15))-0x21CD4;
   if(slot>=c.Esp&&ReadRaw(slot,&call,4)){
    call=NativeReturn(native,slot,call)-static_cast<uint32_t>(g_base)+0x400000;
    out.debugCall=call;
    switch(call){
     case 0x56FE55:if(out.readerCall==call)out.stage=NativeStage::Minus;break;
     case 0x56FE67:if(out.readerCall==call)out.stage=NativeStage::LetterJ;break;
     case 0x56FE79:if(out.readerCall==call)out.stage=NativeStage::Command;break;
     case 0x56FEA2:out.stage=NativeStage::DebugText;break;
     case 0x5754A8:if(out.animalFrame==out.debugFrame&&out.animalReturn==call)out.stage=NativeStage::AnimalTarget;break;
     case 0x5754C7:if(out.animalFrame==out.debugFrame&&out.animalReturn==call)out.stage=NativeStage::AnimalSpecies;break;
     case 0x57708A:out.stage=NativeStage::Npc;break;
     case 0x57B607:out.stage=NativeStage::ItemName;break;
     case 0x57B569:out.stage=NativeStage::DayCount;break;
     default:if(QuestionActive())out.stage=NativeStage::QuestQuestion;break;
    }
   }
  }else{
   // Distinguish an inventory/question from the top-level command reader.
   auto it=std::find_if(out.returns.begin(),out.returns.end(),[](uint32_t va){return
    va==0x57D0AB||va==0x44CCC1||va==0x44CDBB;});
   if(it!=out.returns.end()){
    out.stage=*it==0x57D0AB?(out.readerCall==0x57D0AB?NativeStage::Main:NativeStage::Unknown):NativeStage::Pickup;
    if(out.stage==NativeStage::Pickup&&has(0x4FBF5B))out.stage=NativeStage::PickupCount;
    if(out.stage==NativeStage::Pickup&&has(0x4FC08F))out.stage=NativeStage::PickupCountText;
   }
  }
 }
 if(out.readerCall==0x57D0AB){out.stage=NativeStage::Main;out.debugFrame=0;out.debugCall=0;}
 if(out.readerCall==0x55DF45){out.stage=NativeStage::Course;out.debugFrame=0;out.debugCall=0;}
 if(courseReward&&out.readerCall==0x4D12B4)out.stage=NativeStage::CourseReward;
 if(skillReader&&out.skillFrame&&out.readerCall==0x4D12B4){
  out.stage=NativeStage::SkillAllocation;
  if(g_skillCounter&&!ReadRaw(g_skillCounter,&out.skillPoints,4))out.skillPoints=-1;
  if(skillWriter&&freezeTarget>0&&freezeTarget<=100&&out.skillPoints>=0&&out.skillPoints<=100&&out.skillPoints!=freezeTarget){
   out.skillWriteAttempted=true;SIZE_T used=0;
   out.skillWriteOkay=WriteProcessMemory(skillWriter,(void*)g_skillCounter,&freezeTarget,4,&used)&&used==4;
  }
 }
 ResumeThread(thread);CloseHandle(thread);return out;
}
