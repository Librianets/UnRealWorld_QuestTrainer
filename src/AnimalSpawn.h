#pragma once
// Native setnpc, locked to the clean 3.89.1 SHA in Attach. No game-memory writes.
#include "AnimalPolicy.h"
HWND g_animalWindow=nullptr,g_animalSearch=nullptr,g_animalList=nullptr,g_animalStatus=nullptr,g_animalRun=nullptr;
bool g_animalActive=false,g_animalTest=false,g_animalCommitted=false,g_animalTargetCommitted=false,g_animalRecovery=false;
uint32_t g_animalWantedX=0,g_animalWantedY=0;unsigned g_animalMoves=0;
uint32_t g_animalSpecies=0;uintptr_t g_animalFrame=0;
animalspawn::Snapshot g_animalStart,g_animalBefore;
void AnimalForget(){g_animalActive=false;g_animalFrame=0;if(g_animalRun)EnableWindow(g_animalRun,TRUE);if(g_animalSearch)EnableWindow(g_animalSearch,TRUE);if(g_animalList)EnableWindow(g_animalList,TRUE);}
void AnimalText(const std::wstring& s){if(g_animalStatus)SetWindowTextW(g_animalStatus,s.c_str());}
void AnimalPause(const wchar_t* reason){g_animalRecovery=true;std::wstring text=std::wstring(reason)+L" Ожидался вид "+std::to_wstring(g_animalSpecies)+L" ("+std::wstring(animalspawn::Find(g_animalSpecies)?animalspawn::Find(g_animalSpecies)->ru:L"неизвестен")+L")."+L" Не нажимайте Esc или пустой Enter в MC ID: игра создаст неверное существо. Автоматического повтора нет. Если запрос уже открыт, безопаснее закрыть игру без сохранения и восстановить резервную копию.";AnimalText(text);Log(text);}
bool AnimalLatchCritical(){
 if(!g_animalActive||g_animalTargetCommitted||g_phase<305)return g_animalTargetCommitted;
 auto n=ProbeNative();g_animalTargetCommitted=animalspawn::CriticalBoundary(g_animalActive,g_animalTargetCommitted,g_phase,n.stage==NativeStage::Main,n.stage==NativeStage::AnimalTarget&&n.debugFrame==g_animalFrame);
 return g_animalTargetCommitted;
}
void AnimalFinish(const wchar_t* s){AnimalLatchCritical();if(g_animalTargetCommitted&&g_animalActive){AnimalPause(L"Проверка состояния после открытия выбора клетки не пройдена. Автоматический ввод приостановлен.");return;}AnimalText(s);Stop(s);}
void AnimalComplete(const wchar_t* s){g_animalTargetCommitted=false;AnimalText(s);Stop(s);}
void AnimalRequestStop(){bool was=g_animalTargetCommitted;AnimalLatchCritical();if(!was&&g_animalTargetCommitted){AnimalPause(L"Не подтверждена безопасная отмена выбора клетки.");return;}if(g_animalRecovery){AnimalPause(L"Операция уже приостановлена. Используйте повторную проверку либо восстановите резервную копию после закрытия игры без сохранения.");return;}if(g_animalTargetCommitted){AnimalText(L"После подтверждения клетки штатная команда не умеет отменяться. Завершаю ввод выбранного вида; не нажимайте Esc в игре.");Log(L"Остановка запрошена после подтверждения клетки: завершаю одну уже начатую операцию.");return;}AnimalFinish(L"Ввод остановлен до подтверждения клетки. В CHOOSE TARGET можно нажать Esc.");}
bool AnimalCall(uint32_t call,uint32_t target){unsigned char b[5]{};int32_t rel=0;if(!Read(call,b,5)||b[0]!=0xE8)return false;memcpy(&rel,b+1,4);return uint32_t(call+5+rel)==target;}
bool AnimalCode(uint32_t id){const auto* s=animalspawn::Find(id);if(!s)return false;
 const uint32_t calls[][2]={{0x5754A3,0x4F7820},{0x5754C2,0x50F5B0},{0x5754F0,0x4CECB0},{0x575509,0x40D650},{0x57554A,0x40D2F0},{0x57558B,0x41A550},{0x50F719,0x4D3BF0},{0x4F7DD4,0x4D3BF0}};
 for(const auto& c:calls)if(!AnimalCall(c[0],c[1]))return false;
 uint8_t index=0;uint32_t entry=0;if(!Read(0x4CF0DC+id-500,&index,1)||index>=55||!Read(0x4CF000+index*4,&entry,4))return false;
 return entry==g_base+s->dispatch-0x400000&&AnimalCall(s->dispatch,s->constructor);
}
bool AnimalTerrain(animalspawn::Snapshot& s){s.terrainSafe=false;if(s.tx>3071||s.ty>2047)return true;unsigned char tile=0;uint32_t flags=0;if(!Read(0xCF9448+3073*s.ty+s.tx,&tile,1)||!Read(0x5EC79C+600*tile,&flags,4))return false;s.terrainSafe=animalspawn::Terrain(tile,flags);return true;}
bool AnimalRead(animalspawn::Snapshot& out,bool target){
 if(!g_process||GetProcessId(g_process)!=g_pid||WaitForSingleObject(g_process,0)!=WAIT_TIMEOUT)return false;
 TimeStoppedThread stop;if(!stop.stopped)return false;
 animalspawn::Snapshot s;char map[64]{},player[32]{};
 if(!Read(0xA72B055,map,sizeof(map))||!Read(kPlayer,player,sizeof(player))||!memchr(map,0,sizeof(map))||!memchr(player,0,sizeof(player))||
    !Read(0xA6D1C9C,&s.x,4)||!Read(0xA72BD40,&s.y,4)||!Read(0x634C160,s.ids.data(),s.ids.size()*4))return false;
 s.map=map;s.player=player;
 for(size_t i=0;i<s.ids.size();++i)if(s.ids[i]){unsigned char kind=0;if(!Read(0x6151400+1384*i,&kind,1))return false;if(!kind)s.malformedActive=true;}
 std::vector<unsigned char> raw(9951*36);
 if(!Read(0xA6D28A8,raw.data(),raw.size()))return false;s.world.resize(9951);
 for(size_t i=0;i<s.world.size();++i){auto* p=raw.data()+i*36;auto& w=s.world[i];memcpy(&w.x,p,2);memcpy(&w.y,p+2,2);w.type=p[4];memcpy(&w.count,p+8,2);memcpy(&w.ref,p+12,4);}
 if(target){if(!Read(0xA72BD3C,&s.tx,4)||!Read(0xA6D1C34,&s.ty,4))return false;
  if(!AnimalTerrain(s))return false;
 }
 out=std::move(s);return true;
}
bool AnimalDesired(animalspawn::Snapshot& s){if(!AnimalRead(s,false))return false;TimeStoppedThread stop;if(!stop.stopped)return false;s.tx=g_animalWantedX;s.ty=g_animalWantedY;return AnimalTerrain(s);}
bool AnimalChoose(animalspawn::Snapshot& s){
 TimeStoppedThread stop;if(!stop.stopped)return false;
 const int offsets[][2]={{1,0},{0,1},{-1,0},{0,-1},{1,1},{-1,1},{-1,-1},{1,-1}};
 for(const auto& d:offsets){s.tx=s.x+d[0];s.ty=s.y+d[1];if(!AnimalTerrain(s))return false;if(!animalspawn::Validate(s,g_animalSpecies,true)){g_animalWantedX=s.tx;g_animalWantedY=s.ty;return true;}}return false;
}
void AnimalBegin(){
 if(g_animalTest||g_editorTest){AnimalText(L"Тест окна: игровые действия отключены.");return;}
 if(g_busy||g_skillFrozen){AnimalText(L"Сначала остановите другие операции и разморозьте очки.");return;}
 LRESULT selected=SendMessageW(g_animalList,LB_GETCURSEL,0,0);if(selected==LB_ERR){AnimalText(L"Выберите животное из списка.");return;}
 uint32_t id=(uint32_t)SendMessageW(g_animalList,LB_GETITEMDATA,selected,0);
 if(!animalspawn::Find(id)||!SelectGame())return;
 if(ProbeNative().stage!=NativeStage::Main){AnimalText(L"Закройте меню и диалоги: нужен основной экран локальной карты.");return;}
 animalspawn::Snapshot before;if(!AnimalCode(id)||!AnimalRead(before,false)){AnimalText(L"Не удалось проверить код или прочитать состояние игры. Ввод не отправлен.");return;}
 if(auto reason=animalspawn::Validate(before,id,false)){AnimalText(reason);return;}
 g_animalSpecies=id;if(!AnimalChoose(before)){AnimalText(L"Рядом нет свободной обычной земли (Ground / Surface of the Earth). Команда не открыта.");return;}
 if(!EnsureGameForeground()||ProbeNative().stage!=NativeStage::Main){AnimalText(L"Основной экран не подтверждён после переключения окна.");return;}
 g_previousLayout=GetKeyboardLayout(g_thread);g_english=LoadKeyboardLayoutW(L"00000409",0);DWORD_PTR ignored=0;
 if(!g_english||!SendMessageTimeoutW(g_game,WM_INPUTLANGCHANGEREQUEST,0,(LPARAM)g_english,SMTO_ABORTIFHUNG,500,&ignored)){LayoutBack();AnimalText(L"Не удалось включить английскую раскладку.");return;}
 g_animalStart=std::move(before);g_animalSpecies=id;g_animalFrame=0;g_animalCommitted=false;g_animalTargetCommitted=false;g_animalRecovery=false;g_animalMoves=0;g_animalActive=true;g_busy=true;
 g_typed.clear();g_typedIndex=0;auto now=GetTickCount64();g_deadline=now+180000;Step(300,now);
 for(HWND h:{g_animalRun,g_animalSearch,g_animalList,g_begin,g_native,g_refresh,g_games,g_itemButton,g_update,g_days,g_courseButton})EnableWindow(h,FALSE);
 AnimalText(L"Тренер сам выбирает свободную соседнюю землю и вводит вид. Не нажимайте клавиши и не перемещайте курсор в игре до результата.");
 Log(L"Создание одного животного: "+std::wstring(animalspawn::Find(id)->ru)+L" ["+std::to_wstring(id)+L"].");
}
bool AnimalFreshTarget(animalspawn::Snapshot& out){
 if(!AnimalCode(g_animalSpecies)||!AnimalRead(out,true)){AnimalFinish(L"Не удалось заново прочитать код или клетку. Ввод остановлен. Не нажимайте Esc или пустой Enter в MC ID.");return false;}
 if(!animalspawn::SamePlace(g_animalStart,out)){AnimalFinish(L"Карта или персонаж изменились. Ввод остановлен. Не нажимайте Esc или пустой Enter в MC ID.");return false;}
 if(auto reason=animalspawn::Validate(out,g_animalSpecies,true)){AnimalFinish(reason);return false;}return true;
}
void AnimalResume(){
 if(!g_animalActive||!g_animalRecovery||!g_animalTargetCommitted)return;
 auto n=ProbeNative();if(n.stage!=NativeStage::AnimalSpecies||n.debugFrame!=g_animalFrame){AnimalText(L"Для возобновления нужен тот же запрос MC ID; Enter и Esc не нажимайте.");return;}
 animalspawn::Snapshot fresh;if(!AnimalFreshTarget(fresh))return;
 std::string desired=std::to_string(g_animalSpecies),typed=TextAt(0xA89B9A8,100);
 if(fresh.tx!=g_animalBefore.tx||fresh.ty!=g_animalBefore.ty||fresh.ids!=g_animalBefore.ids||typed.size()>desired.size()||desired.substr(0,typed.size())!=typed){AnimalText(L"Возобновление запрещено: клетка, список или введённые цифры отличаются. Не нажимайте Esc/Enter; восстановите резервную копию.");return;}
 g_typedIndex=typed.size();g_animalRecovery=false;auto now=GetTickCount64();g_deadline=now+60000;Step(306,now,30000);AnimalText(L"Проверки повторены. Завершаю тот же выбранный вид.");
}
bool AnimalKey(wchar_t key){if(SendKey(key))return true;if(g_animalTargetCommitted){AnimalPause(L"Не удалось полностью передать клавишу.");}else AnimalFinish(L"Ввод остановлен до подтверждения клетки. В CHOOSE TARGET можно нажать Esc.");return false;}
void AnimalTick(){
 if(!g_animalActive)return;auto now=GetTickCount64();if(now<g_due)return;
 if(!g_process||WaitForSingleObject(g_process,0)!=WAIT_TIMEOUT){AnimalComplete(L"Игра закрыта. Результат создания не подтверждён.");return;}
 auto state=ProbeNative();
 static int lastPhase=-1,lastStage=-1;static uint32_t lastX=0,lastY=0;
 if(lastPhase!=g_phase||lastStage!=(int)state.stage||lastX!=state.animalX||lastY!=state.animalY){lastPhase=g_phase;lastStage=(int)state.stage;lastX=state.animalX;lastY=state.animalY;Log(L"[animal] phase="+std::to_wstring(g_phase)+L" stage="+std::to_wstring((int)state.stage)+L" cursor="+std::to_wstring(state.animalX)+L","+std::to_wstring(state.animalY)+L" desired="+std::to_wstring(g_animalWantedX)+L","+std::to_wstring(g_animalWantedY));}
 if((g_phase==307||g_animalRecovery)&&state.stage==NativeStage::Main){
  animalspawn::Snapshot after;uint32_t species=0;bool okay=AnimalRead(after,false);int slot=okay?animalspawn::AddedSlot(g_animalBefore,after):-1;
  if(slot>=0)okay=Read(0x6151400+1384*slot+188,&species,4)&&animalspawn::Readback(g_animalBefore,after,g_animalSpecies,species);else okay=false;
  AnimalComplete(okay?L"Создано одно животное на свободной соседней клетке. Вид и запись в мире подтверждены.":L"Результат неоднозначен. Проверьте мир; при неверном существе восстановите резервную копию. Автоматического повторения нет.");return;
 }
 if(g_animalRecovery){g_due=now+500;return;}
 if(now>g_deadline||now>g_stepDeadline){AnimalFinish(g_animalTargetCommitted?L"Ожидаемое состояние после выбора клетки не подтверждено.":L"Ожидание остановлено до выбора клетки. Если открыт CHOOSE TARGET, нажмите Esc.");return;}
 if(g_phase==307){g_due=now+160;return;}
 if(!EnsureGameForeground()){
  if(g_animalTargetCommitted){AnimalText(L"Жду возвращения фокуса игре, чтобы завершить выбранный вид. Не нажимайте Esc в MC ID.");g_due=now+500;g_stepDeadline=now+30000;return;}
  AnimalFinish(L"Фокус потерян до выбора клетки. Если открыт CHOOSE TARGET, его можно закрыть через Esc.");return;
 }
 state=ProbeNative();
 if(g_animalFrame&&state.debugFrame&&state.debugFrame!=g_animalFrame){AnimalFinish(L"Служебный диалог изменился. Ввод остановлен.");return;}
 if(g_phase==300){if(state.stage==NativeStage::Main&&AnimalKey(L'*'))Step(301,now);return;}
 if(g_phase==301){if(state.stage==NativeStage::Minus&&state.debugFrame){g_animalFrame=state.debugFrame;if(AnimalKey(L'-'))Step(302,now);}return;}
 if(g_phase==302){if(state.stage==NativeStage::LetterJ&&state.debugFrame==g_animalFrame&&AnimalKey(L'j'))Step(303,now);return;}
 if(g_phase==303){if(state.stage==NativeStage::Command&&state.debugFrame==g_animalFrame&&AnimalKey(L'\r'))Step(304,now);return;}
 if(g_phase==304){if(state.debugFrame==g_animalFrame&&TypeChecked(state,NativeStage::DebugText,"setnpc",305,now)){if(g_phase==305)Log(L"Тренер выбирает проверенную соседнюю клетку. Не управляйте курсором вручную.");}return;}
 if(g_phase==305){
  if(state.stage==NativeStage::Main){AnimalComplete(L"Игра вернулась на основной экран до подтверждения тренером. Результат не подтверждён; проверьте мир. Автоматического повтора нет.");return;}
  if(state.stage==NativeStage::AnimalSpecies){g_animalTargetCommitted=true;AnimalPause(L"Клетка подтверждена вручную раньше тренера.");return;}
  if(state.stage!=NativeStage::AnimalTarget||state.debugFrame!=g_animalFrame||!state.animalCursor)return;
  animalspawn::Snapshot fresh;if(!AnimalDesired(fresh)||!animalspawn::SamePlace(g_animalStart,fresh)){AnimalFinish(L"Не удалось проверить выбранную клетку до подтверждения. CHOOSE TARGET можно закрыть через Esc.");return;}
  if(auto reason=animalspawn::Validate(fresh,g_animalSpecies,true)){AnimalFinish(reason);return;}
  if(state.animalX!=g_animalWantedX||state.animalY!=g_animalWantedY){
   if(++g_animalMoves>192){AnimalFinish(L"Курсор не достиг соседней клетки; CHOOSE TARGET можно закрыть через Esc.");return;}
   WORD key=state.animalX<g_animalWantedX?VK_RIGHT:state.animalX>g_animalWantedX?VK_LEFT:state.animalY<g_animalWantedY?VK_DOWN:VK_UP;
   if(!SendVirtual(key)){AnimalFinish(L"Не удалось переместить курсор; CHOOSE TARGET можно закрыть через Esc.");return;}g_due=now+160;g_stepDeadline=now+6000;return;
  }
  auto finalState=ProbeNative();if(finalState.stage!=NativeStage::AnimalTarget||finalState.debugFrame!=g_animalFrame||!finalState.animalCursor||finalState.animalX!=fresh.tx||finalState.animalY!=fresh.ty||GetForegroundWindow()!=g_game)return;
  g_animalBefore=std::move(fresh);g_animalTargetCommitted=true;
  if(AnimalKey(L'\r')){g_typedIndex=0;Step(306,now,30000);}return;
 }
 if(g_phase==306){
  if(state.stage==NativeStage::AnimalTarget)return;
  if(state.stage!=NativeStage::AnimalSpecies||state.debugFrame!=g_animalFrame){AnimalPause(L"После подтверждения клетки запрос вида не подтверждён.");return;}
  std::string value=std::to_string(g_animalSpecies),typed=TextAt(0xA89B9A8,100);
  if(typed!=value.substr(0,g_typedIndex)){AnimalPause(L"Текст запроса изменён вручную или клавиша не обработана.");return;}
  if(g_typedIndex<value.size()){if(AnimalKey((unsigned char)value[g_typedIndex])){++g_typedIndex;g_due=now+200;}return;}
  animalspawn::Snapshot fresh;if(!AnimalFreshTarget(fresh))return;
  if(fresh.tx!=g_animalBefore.tx||fresh.ty!=g_animalBefore.ty||fresh.ids!=g_animalBefore.ids){AnimalPause(L"Клетка или список животных изменились после подтверждения.");return;}
  auto finalState=ProbeNative();if(finalState.stage!=NativeStage::AnimalSpecies||finalState.debugFrame!=g_animalFrame||GetForegroundWindow()!=g_game||TextAt(0xA89B9A8,100)!=value){g_due=now+200;return;}
  g_animalBefore=std::move(fresh);g_animalCommitted=true;if(AnimalKey(L'\r'))Step(307,now,15000);return;
 }
}
std::wstring AnimalFold(const std::wstring& text){if(text.empty())return {};int count=LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_LOWERCASE,text.data(),(int)text.size(),nullptr,0,nullptr,nullptr,0);if(count<=0)return text;std::wstring result(count,0);if(!LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_LOWERCASE,text.data(),(int)text.size(),result.data(),count,nullptr,nullptr,0))return text;return result;}
void AnimalFilter(){wchar_t input[128]{};GetWindowTextW(g_animalSearch,input,128);std::wstring query=AnimalFold(input);
 SendMessageW(g_animalList,LB_RESETCONTENT,0,0);for(const auto& s:animalspawn::SpeciesList()){
  std::wstring label=std::wstring(s.ru)+L" | "+Wide(s.en)+L" ["+std::to_wstring(s.id)+L"]",fold=AnimalFold(label);
  if(fold.find(query)==std::wstring::npos)continue;auto n=SendMessageW(g_animalList,LB_ADDSTRING,0,(LPARAM)label.c_str());SendMessageW(g_animalList,LB_SETITEMDATA,n,s.id);
 }if(SendMessageW(g_animalList,LB_GETCOUNT,0,0)>0)SendMessageW(g_animalList,LB_SETCURSEL,0,0);
}
LRESULT CALLBACK AnimalWindowProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_CREATE:{g_animalWindow=h;
 auto ctl=[&](const wchar_t* c,const wchar_t* title,DWORD style,int x,int y,int width,int height,int id){auto v=CreateWindowExW(!wcscmp(c,L"EDIT")?WS_EX_CLIENTEDGE:0,c,title,WS_CHILD|WS_VISIBLE|style,x,y,width,height,h,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);SendMessageW(v,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return v;};
 ctl(L"STATIC",L"Поиск по русскому / English названию или ID:",0,12,12,680,22,1);g_animalSearch=ctl(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,12,38,680,28,2);
 g_animalList=ctl(L"LISTBOX",L"",LBS_NOTIFY|LBS_SORT|WS_VSCROLL|WS_BORDER|WS_TABSTOP,12,76,680,290,3);
 g_animalRun=ctl(L"BUTTON",L"Создать одно рядом",WS_TABSTOP,12,378,260,34,4);ctl(L"BUTTON",L"Остановить",WS_TABSTOP,288,378,150,34,5);ctl(L"BUTTON",L"Повторить проверку",WS_TABSTOP,454,378,238,34,8);
 ctl(L"STATIC",L"Тренер сам выбирает свободную соседнюю клетку земли (Ground / Surface of the Earth). Не нажимайте клавиши в игре до результата. Создаётся одно животное. Пол, поведение и приручённость определяет игра; хищник может напасть. Изменение сохраняется в мире. Перед опытом сохраните резервную копию персонажа.",0,12,426,680,70,6);
 g_animalStatus=ctl(L"EDIT",L"Выберите вид. Во время создания не управляйте игрой; курсор и цифры вводит тренер.",ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,12,514,680,110,7);AnimalFilter();return 0;}
 case WM_COMMAND:if(LOWORD(w)==2&&HIWORD(w)==EN_CHANGE)AnimalFilter();else if(LOWORD(w)==4)AnimalBegin();else if(LOWORD(w)==5&&g_animalActive)AnimalRequestStop();else if(LOWORD(w)==8)AnimalResume();return 0;
 case WM_CLOSE:if(g_animalActive){AnimalRequestStop();if(g_animalActive)return 0;}DestroyWindow(h);return 0;
 case WM_DESTROY:g_animalWindow=g_animalSearch=g_animalList=g_animalStatus=g_animalRun=nullptr;return 0;
 }return DefWindowProcW(h,m,w,l);}
void OpenAnimalWindow(){if(g_animalWindow){ShowWindow(g_animalWindow,SW_RESTORE);SetForegroundWindow(g_animalWindow);return;}
 WNDCLASSW wc{};wc.lpfnWndProc=AnimalWindowProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"URWAnimalSpawn019";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&wc);
 CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,L"Животные рядом — русский / English",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|(g_animalTest?0:WS_VISIBLE),CW_USEDEFAULT,CW_USEDEFAULT,728,675,g_window,nullptr,wc.hInstance,nullptr);
}
void AnimalWindowSelfTest(){unsigned checks=0,failures=0;std::vector<unsigned> failed;auto check=[&](bool v){++checks;if(!v){++failures;failed.push_back(checks);}};
 OpenAnimalWindow();check(g_animalWindow!=nullptr);check(SendMessageW(g_animalList,LB_GETCOUNT,0,0)==39);
 SetWindowTextW(g_animalSearch,L"bear");check(SendMessageW(g_animalList,LB_GETCOUNT,0,0)==1);check(SendMessageW(g_animalList,LB_GETITEMDATA,0,0)==605);
 SetWindowTextW(g_animalSearch,L"лось");check(SendMessageW(g_animalList,LB_GETCOUNT,0,0)==1);check(SendMessageW(g_animalList,LB_GETITEMDATA,0,0)==660);
 SetWindowTextW(g_animalSearch,L"700");check(SendMessageW(g_animalList,LB_GETCOUNT,0,0)==0);SetWindowTextW(g_animalSearch,L"");
 SendMessageW(g_animalWindow,WM_COMMAND,4,0);check(!g_animalActive&&!g_busy&&!g_process);check(CharacterText(g_animalStatus).find(L"отключены")!=std::wstring::npos);
 g_animalActive=true;g_busy=true;SendMessageW(g_window,WM_COMMAND,11,0);check(g_animalActive&&g_busy&&!g_process);SendMessageW(g_window,WM_COMMAND,13,0);check(g_animalActive&&g_busy&&!g_process);
 g_animalTargetCommitted=true;SendMessageW(g_window,WM_COMMAND,15,0);check(g_animalActive&&g_busy);SendMessageW(g_animalWindow,WM_CLOSE,0,0);check(g_animalWindow&&g_animalActive);SendMessageW(g_window,WM_CLOSE,0,0);check(IsWindow(g_window)&&g_animalActive);
 g_animalTargetCommitted=false;g_animalRecovery=false;g_phase=305;AnimalRequestStop();check(g_animalTargetCommitted&&g_animalRecovery&&g_animalActive&&g_busy&&!g_process);AnimalRequestStop();check(CharacterText(g_animalStatus).find(L"приостановлена")!=std::wstring::npos);SendMessageW(g_animalWindow,WM_COMMAND,8,0);check(g_animalRecovery&&g_busy&&!g_process);
 g_animalTargetCommitted=false;g_animalRecovery=false;g_phase=0;AnimalForget();g_busy=false;
 RECT client{},button{};GetClientRect(g_window,&client);GetWindowRect(GetDlgItem(g_window,35),&button);MapWindowPoints(nullptr,g_window,(POINT*)&button,2);check(button.top>=0&&button.bottom<=client.bottom&&button.right<=client.right);
 SendMessageW(g_animalWindow,WM_CLOSE,0,0);check(g_animalWindow==nullptr);
 std::ofstream out("TRAINER_ANIMAL_GUI_TEST.json");out<<"{\"checks\":"<<checks<<",\"failures\":"<<failures<<",\"game_input\":false,\"failed_checks\":[";for(size_t i=0;i<failed.size();++i){if(i)out<<',';out<<failed[i];}out<<"]}";out.close();PostQuitMessage(failures?2:0);
}
