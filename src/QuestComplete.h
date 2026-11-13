#pragma once
// 3.89.1 SHA-locked native DEBUG COMPLETE QUEST, no remote calls or quest writes.
// The temporary debug scalar only exposes the game's own menu action. It is
// restored before selecting that action; the native handler owns all effects.
HANDLE g_completeWriter=nullptr;
uint32_t g_completeNpc=0,g_completeId=0,g_completeOldDebug=0;
bool g_completeFlag=false,g_completeSent=false,g_completeRestorePending=false;
ULONGLONG g_completeDeadline=0;
constexpr uintptr_t kCompleteDebug=0xA6D28A0;
bool CompleteDebugWrite(uint32_t value){
 SIZE_T used=0;uint32_t actual=~value;return g_completeWriter&&WriteProcessMemory(g_completeWriter,(void*)(g_base+kCompleteDebug-0x400000),&value,4,&used)&&used==4&&Read(kCompleteDebug,&actual,4)&&actual==value;
}
bool CompleteRestore(){
 if(!g_completeFlag)return true;
 if(g_process&&WaitForSingleObject(g_process,0)==WAIT_OBJECT_0){g_completeFlag=false;return true;}
 bool restored=false;for(int i=0;i<3&&!restored;++i){restored=CompleteDebugWrite(g_completeOldDebug);if(!restored)Sleep(10);}
 if(!restored){if(!g_completeRestorePending)Log(L"Не удалось восстановить служебный флаг. Повторяю восстановление; новые действия и закрытие тренера заблокированы. Не сохраняйте тестовую игру.");return false;}
 g_completeFlag=false;Log(L"Служебный флаг восстановлен.");return true;
}
bool QuestCompleteCancel(){
 if(!CompleteRestore()){g_completeRestorePending=true;g_completeActive=true;g_busy=true;return false;}
 g_completeRestorePending=false;g_completeActive=false;g_completeSent=false;
 if(g_completeWriter){CloseHandle(g_completeWriter);g_completeWriter=nullptr;}return true;
}
bool CompleteIdentity(bool allowDone=false){
 uint8_t accepted=0;
 return NumberAt(0xA620630)==g_completeNpc&&NumberAt(0xA80EFF0)==g_completeId&&
  NumberAt(0x615190C+1384*g_completeNpc)==g_completeId&&
  Read(0xA81DF1E,&accepted,1)&&accepted&&
  (allowDone||(!NumberAt(0xA81DF24)&&!NumberAt(0x6151910+1384*g_completeNpc)));
}
int CompleteOption(){
 int count=(int)NumberAt(kMenuCount);if(count<1||count>32)return -1;
 for(int i=0;i<10;++i){
  if(TextAt(0xA612900+412*i,100)!="[QUEST COMPLETED]")continue;
  int key=(int)NumberAt(0xA612898+412*i);
  // Native chat maps the chosen row directly into its ten option records.
  if(i<count&&key>=49&&key<=58&&NumberAt(kMenuKeys+232*i)==(uint32_t)key)return key;
 }
 return -1;
}
void QuestCompleteBegin(){
 if(g_busy||g_skillFrozen){Log(L"Сначала остановите другую автоматику.");return;}
 if(!SelectGame())return;
 TimeStoppedThread setupGuard;if(!setupGuard.stopped){Log(L"Не удалось приостановить игровой поток для проверки разговора.");return;}
 auto state=ProbeNative();
 if(!state.chatFrame||state.readerCall!=0x4D12B4){Log(L"Откройте в игре разговор с заказчиком по уже принятому заданию, затем нажмите служебную сдачу.");return;}
 g_completeNpc=NumberAt(0xA620630);g_completeId=NumberAt(0xA80EFF0);
 if(g_completeNpc>=500||!g_completeId||!CompleteIdentity()){Log(L"В этом разговоре нет активного принятого задания либо оно уже завершено.");return;}
 int last=-1;
 for(int i=0;i<29;++i)if(TextAt(0xA80F45C+2076*i,100).find("[completed]")!=std::string::npos)last=i;
 if(last<0){Log(L"У задания нет штатной заключительной реплики [completed]. Сдача не поддерживается.");return;}
 const uint8_t expected[]={0x83,0x3d};uint8_t code[7]{};uint32_t operand=0;
 if(!Read(0x53AFC5,code,7)||memcmp(code,expected,2)||code[6]!=0){Log(L"Проверка native пути сдачи не пройдена.");return;}
 memcpy(&operand,code+2,4);
 if(operand!=g_base+kCompleteDebug-0x400000){Log(L"Адрес служебного флага не подтверждён.");return;}
 g_completeOldDebug=NumberAt(kCompleteDebug);
 if(g_completeOldDebug!=0){Log(L"Служебный режим игры уже включён. Выключите его перед сдачей через тренер.");return;}
 g_completeWriter=OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_WRITE,FALSE,g_pid);
 if(!g_completeWriter){Log(L"Не удалось открыть кратковременную запись служебного флага.");return;}
 g_completeFlag=true;
 if(!CompleteDebugWrite(1)){QuestCompleteCancel();return;}
 g_completeActive=true;g_completeSent=false;g_busy=true;g_completeDeadline=GetTickCount64()+90000;
 Log(L"Служебная сдача: NPC #"+std::to_wstring(g_completeNpc)+L", квест #"+std::to_wstring(g_completeId)+L", TALKLINE "+std::to_wstring(last)+L". Условия выполнения пропускаются.");
 Log(L"Закройте разговор через Esc и снова поговорите с ТЕМ ЖЕ заказчиком. Тренер выберет штатный DEBUG COMPLETE QUEST. Тайм-аут: 90 секунд.");
 Log(L"Для «Птичьего вора» путь начинается с реплики 27; реплика 25 о признании вора пропускается. Награда и дальнейшие ответы — вручную в игре.");
 Status(L"Служебная сдача: повторите разговор с тем же заказчиком. Остановить автоматику — отмена.");
}
void QuestCompleteTick(){
 if(!g_completeActive)return;
 if(g_completeRestorePending){Stop(L"Служебный флаг восстановлен после повторной попытки; сдача отменена.");return;}
 if(!g_process||WaitForSingleObject(g_process,0)==WAIT_OBJECT_0){Stop(L"Игра закрыта. Служебная сдача остановлена.");return;}
 if(GetTickCount64()>g_completeDeadline){Stop(L"Время ожидания служебной сдачи истекло. Завершение не подтверждено.");return;}
 if(g_completeSent&&CompleteIdentity(true)&&NumberAt(0xA81DF24)&&NumberAt(0x6151910+1384*g_completeNpc)){Stop(L"Игра подтвердила завершение задания. Прочитайте заключительные реплики и получите награду вручную.");return;}
 auto state=ProbeNative();
 if(!state.chatFrame)return;
 if(!CompleteIdentity(g_completeSent)){Stop(L"NPC или задание изменились; служебная сдача отменена.");return;}
 if(g_completeSent){
  if(NumberAt(0xA81DF24)&&NumberAt(0x6151910+1384*g_completeNpc))
   Stop(L"Игра подтвердила завершение задания. Прочитайте заключительные реплики и получите награду вручную.");
  return;
 }
 int key=CompleteOption();if(key<0)return;
 if(!CompleteRestore()){Stop(L"Сдача остановлена: служебный флаг не восстановлен.");return;}
 if(GetForegroundWindow()!=g_game)return;
 // Freeze only the final guard and queued key-down/key-up (45ms); release
 // before waiting for native effects. Nested ProbeNative balances its suspend.
 TimeStoppedThread dispatchGuard;if(!dispatchGuard.stopped)return;
 if(GetForegroundWindow()!=g_game)return;
 state=ProbeNative();if(!state.chatFrame||!CompleteIdentity()||CompleteOption()!=key)return;
 g_english=LoadKeyboardLayoutW(L"00000409",0);
 // All native completion keys are digits. VK_1..VK_9/VK_OEM_1 only; the last
 // native ':' row is refused rather than depending on keyboard punctuation.
 if(key>57){Stop(L"Штатный пункт сдачи за пределами цифровых клавиш. Выберите DEBUG COMPLETE QUEST вручную.");return;}
 if(SendVirtual((WORD)key)){g_completeSent=true;g_completeDeadline=GetTickCount64()+600000;Log(L"Выбран штатный пункт завершения. Прочитайте и продолжите игровые окна (Space/ответ); ожидаю подтверждение игры до 10 минут. Служебный флаг уже восстановлен.");}
}

// Lifecycle tests use a private page in this test process, never a game process.
int QuestCompleteSelfTest(){
 int checks=0,failures=0;auto check=[&](bool okay){++checks;if(!okay)++failures;};
 auto page=(uint32_t*)VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!page)return 2;
 g_base=(uintptr_t)page-kCompleteDebug+0x400000;
 g_pid=GetCurrentProcessId();g_process=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,g_pid);
 auto writer=[](){return OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_WRITE,FALSE,GetCurrentProcessId());};
 *page=1;g_completeOldDebug=0;g_completeFlag=true;g_completeActive=true;g_completeWriter=writer();
 check(QuestCompleteCancel());check(*page==0);check(!g_completeFlag&&!g_completeActive&&!g_completeWriter);
 *page=1;g_completeFlag=true;g_completeActive=true;g_completeWriter=nullptr;
 check(!QuestCompleteCancel());check(*page==1&&g_completeFlag&&g_completeActive&&g_completeRestorePending&&g_busy);
 // Retry retains the original snapshot and restores once a writer is available.
 g_completeWriter=writer();check(QuestCompleteCancel());check(*page==0);check(!g_completeRestorePending&&!g_completeFlag&&!g_completeWriter);
 // A failed wait/read handle is not mistaken for a terminated process.
 CloseHandle(g_process);g_process=(HANDLE)(uintptr_t)0x12345678;
 *page=1;g_completeFlag=true;g_completeWriter=writer();
 check(!CompleteRestore());check(g_completeFlag);
 g_process=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,g_pid);
 check(QuestCompleteCancel());check(*page==0&&!g_completeFlag);
 CloseHandle(g_process);g_process=nullptr;g_base=0;g_busy=false;VirtualFree(page,0,MEM_RELEASE);
 std::ofstream out("TRAINER_QUEST_SELFTEST.json");out<<"{\"checks\":"<<checks<<",\"failures\":"<<failures<<",\"private_memory_only\":true}\n";
 return failures?2:0;
}
