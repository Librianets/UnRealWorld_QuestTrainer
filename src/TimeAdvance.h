#pragma once
// Native time, not the debug calendar jump. No game-memory writes or remote calls.
namespace timeadvance {
constexpr unsigned kMaximumDays=7,kBatchMinutes=240;
struct State {
 std::array<char,12> name{};unsigned char age=0,sex=0,posture=0,starvation=0,stomach=0;
 std::array<unsigned char,20> wounds{},bleeding{};
 int32_t minutes=0,days=0,blood=0,injury=0,energy=0,thirst=0,sleep=0;
 float fatigue=0;
};
inline bool SamePlayer(const State& a,const State& b){return a.name==b.name&&a.age==b.age&&a.sex==b.sex;}
inline bool Valid(const State& s){return s.name[0]&&(s.sex==1||s.sex==2)&&s.age>=1&&s.age<=120&&s.minutes>=0&&s.days>=0&&s.energy>=0&&s.energy<=5000&&s.thirst>=0&&s.sleep>=0&&s.blood>=0&&s.injury>=0&&std::isfinite(s.fatigue)&&s.fatigue>=0&&s.fatigue<=100;}
// Conservative automation limits, NOT the game's hard bounds. A four-hour
// batch remains a real action: survival is never disabled by this trainer.
inline const wchar_t* Danger(const State& s){
 if(!Valid(s))return L"Не удалось подтвердить состояние живого персонажа.";
 if(s.posture==2)return L"Нельзя ждать, находясь в воде.";
 if(s.blood||s.injury||s.starvation)return L"Есть кровопотеря, последствия ран или истощение. Сначала восстановите персонажа.";
 for(unsigned i=0;i<20;++i)if(s.wounds[i]||s.bleeding[i])return L"Есть активная рана или недуг. Сначала восстановите персонажа.";
 if(s.energy<1000)return L"Запас питания ниже безопасного порога тренера (1000). Поешьте.";
 if(s.thirst>=750)return L"Жажда достигла порога тренера (750). Попейте.";
 if(s.sleep>=1000)return L"Потребность во сне достигла порога тренера (1000). Поспите.";
 if(s.fatigue>=50)return L"Утомление достигло порога тренера (50%). Отдохните.";
 return nullptr;
}
inline bool Days(const std::wstring& raw,unsigned& days){if(raw.empty()||raw.size()>2)return false;unsigned n=0;for(auto c:raw){if(c<L'0'||c>L'9')return false;n=n*10+unsigned(c-L'0');}if(n<1||n>kMaximumDays)return false;days=n;return true;}
inline bool WaitCookie(uintptr_t fp,uintptr_t esp,uint32_t parent,uint32_t cookie,uint32_t secret,const uint32_t* args,uintptr_t base){
 uintptr_t aligned=fp&~uintptr_t(63);return aligned>=0x3174&&aligned-0x3174>=esp&&parent>fp&&parent-fp<0x100000&&cookie==(secret^uint32_t(aligned-0x3174))&&args[0]==base+0x19CBC0&&args[1]==0&&args[2]==0x24;
}
enum class Batch {Pending,Complete,Interrupted,Invalid};
inline Batch Result(int32_t start,int32_t current,bool main){int64_t elapsed=int64_t(current)-start;if(elapsed<0||elapsed>kBatchMinutes)return Batch::Invalid;if(!main)return Batch::Pending;if(elapsed==kBatchMinutes)return Batch::Complete;return elapsed>0?Batch::Interrupted:Batch::Pending;}
}
bool g_timeTest=false;
HWND g_timeWindow=nullptr,g_timeDays=nullptr,g_timeRun=nullptr,g_timeStatus=nullptr;
bool g_timeActive=false,g_timeWaiting=false,g_timeSeenWait=false;
unsigned g_timeRequested=0;timeadvance::State g_timeIdentity{};
int32_t g_timeStart=0,g_timeBatchStart=0;ULONGLONG g_timeNext=0,g_timeBatchDeadline=0,g_timeUnknownSince=0;
void TimeText(const std::wstring& text){if(g_timeStatus)SetWindowTextW(g_timeStatus,text.c_str());}
void TimeForget(){g_timeActive=false;g_timeWaiting=false;if(g_timeRun)EnableWindow(g_timeRun,TRUE);if(g_timeDays)EnableWindow(g_timeDays,TRUE);}
struct TimeStoppedThread {
 HANDLE handle=nullptr;bool stopped=false;
 TimeStoppedThread(){if(!g_process||GetProcessId(g_process)!=g_pid)return;handle=OpenThread(THREAD_SUSPEND_RESUME|THREAD_QUERY_INFORMATION|THREAD_GET_CONTEXT,FALSE,g_thread);if(handle&&GetProcessIdOfThread(handle)==g_pid){DWORD prior=SuspendThread(handle);if(prior!=DWORD(-1)){stopped=prior==0;if(!stopped)ResumeThread(handle);}}}
 ~TimeStoppedThread(){if(stopped)ResumeThread(handle);if(handle)CloseHandle(handle);}
 TimeStoppedThread(const TimeStoppedThread&)=delete;TimeStoppedThread& operator=(const TimeStoppedThread&)=delete;
};
bool TimeRead(timeadvance::State& s){
 if(!g_process||GetProcessId(g_process)!=g_pid||WaitForSingleObject(g_process,0)!=WAIT_TIMEOUT)return false;
 TimeStoppedThread stop;if(!stop.stopped)return false;
 bool ok=Read(0xA72A6E8,s.name.data(),s.name.size())&&Read(0xA72B1B9,&s.age,1)&&Read(0xA72B1A9,&s.sex,1)&&Read(0xA72BCE6,&s.posture,1)&&Read(0xA72B1A8,&s.starvation,1)&&Read(0xA72BCE8,&s.stomach,1)&&Read(0xA72B130,s.wounds.data(),20)&&Read(0xA72B108,s.bleeding.data(),20)&&Read(0xA72B8E0,&s.minutes,4)&&Read(0xA72B1C4,&s.days,4)&&Read(0xA72B194,&s.blood,4)&&Read(0xA72B18C,&s.injury,4)&&Read(0xA72B17C,&s.energy,4)&&Read(0xA72B0D4,&s.thirst,4)&&Read(0xA72B0D8,&s.sleep,4)&&Read(0xA72B188,&s.fatigue,4);
 return ok&&timeadvance::Valid(s);
}
// A live native wait frame: exact call return, arguments and stack cookie.
// This is read-only progress evidence; it never authorizes blind Escape input.
bool TimeWaitFrame(){try {
 TimeStoppedThread stop;if(!stop.stopped)return false;HANDLE thread=stop.handle;
 bool found=false;CONTEXT c{};c.ContextFlags=CONTEXT_CONTROL;NativeReturns native;
 uint32_t secret=0;int32_t active=0;
 if(GetThreadContext(thread,&c)&&ReadHookReturns(thread,c.Esp,native)&&Read(0x5E200C,&secret,4)&&Read(0x217F54C,&active,4)&&active==1){
  // Cookie is [aligned EBP - 4] ^ (aligned EBP - 0x3174).
  for(uintptr_t page=c.Esp,limit=uintptr_t(c.Esp)+0x18000;page<limit&&!found;){
   MEMORY_BASIC_INFORMATION mi{};if(!VirtualQueryEx(g_process,(void*)page,&mi,sizeof(mi)))break;
   uintptr_t end=std::min<uintptr_t>(limit,(uintptr_t)mi.BaseAddress+mi.RegionSize);if(end<=page||mi.State!=MEM_COMMIT||(mi.Protect&(PAGE_NOACCESS|PAGE_GUARD)))break;
   std::vector<uint32_t> words((end-page)/4);
   if(!words.empty()&&ReadRaw(page,words.data(),words.size()*4))for(size_t i=0;i<words.size();++i){
    uintptr_t slot=page+4*i;if(NativeReturn(native,slot,words[i])!=g_base+0x17D8CC)continue;
    uintptr_t fp=slot-4,aligned=fp&~uintptr_t(63);if(aligned<0x3174||aligned-0x3174<c.Esp)continue;
    uint32_t parent=0,cookie=0,args[3]{};
    if(ReadRaw(fp,&parent,4)&&ReadRaw(aligned-4,&cookie,4)&&ReadRaw(fp+8,args,sizeof(args))&&timeadvance::WaitCookie(fp,c.Esp,parent,cookie,secret,args,g_base)){found=true;break;}
   }page=end;
  }
 }
 return found;
 }catch(...){return false;}
}
bool TimeCode(){
 unsigned char bytes[5]{};const unsigned char expected[]={0xE8,0x74,0xAF,0xF0,0xFF};
 // The first six bytes can be the supported translation detour. Inspect the
 // unchanged allocation/stack-probe instructions immediately after them.
 unsigned char body[10]{};const unsigned char wait[]={0xB8,0x74,0x31,0x00,0x00,0xE8,0x80,0x28,0x10,0x00};
 unsigned char cookieCode[9]{};const unsigned char cookieExpected[]={0x33,0xC4,0x89,0x84,0x24,0x70,0x31,0x00,0x00};uint32_t cookieGlobal=0;
 char title[12]{};return Read(0x488851,&cookieGlobal,4)&&cookieGlobal==g_base+0x1E200C&&Read(0x488855,cookieCode,9)&&!memcmp(cookieCode,cookieExpected,9)&&Read(0x57D8C7,bytes,5)&&!memcmp(bytes,expected,5)&&Read(0x488846,body,10)&&!memcmp(body,wait,10)&&Read(0x59CBC0,title,12)&&!memcmp(title,"STANDING BY",12);
}
std::wstring TimeProgress(int32_t now){int64_t elapsed=std::max<int64_t>(0,int64_t(now)-g_timeStart);return L"Прошло "+std::to_wstring(elapsed/60)+L" ч "+std::to_wstring(elapsed%60)+L" мин из "+std::to_wstring(g_timeRequested*24)+L" ч.";}
void TimeFinish(const std::wstring& why){
 bool wasWaiting=g_timeWaiting;timeadvance::State s{};std::wstring text=why;if(TimeRead(s))text+=L" "+TimeProgress(s.minutes);
 if(wasWaiting)text+=L" Новых команд не будет. Если ожидание ещё идёт, для немедленной отмены нажмите Esc в игре.";
 TimeText(text);Stop(text.c_str());
}
void TimeBegin(){
 if(g_timeTest||g_editorTest||g_browserTest){TimeText(L"Проверка интерфейса: игровые команды отключены.");return;}
 if(g_busy||g_skillFrozen){TimeText(L"Сначала остановите другую автоматику и разморозьте очки.");return;}
 wchar_t raw[16]{};GetWindowTextW(g_timeDays,raw,16);unsigned days=0;if(!timeadvance::Days(raw,days)){TimeText(L"Введите целое число дней от 1 до 7.");return;}
 if(!SelectGame()||!TimeCode()){TimeText(L"Не подтверждена совместимость с чистой игрой 3.89.1.");return;}
 if(ProbeNative().stage!=NativeStage::Main){TimeText(L"Нужен основной экран локальной карты; закройте меню и диалоги.");return;}
 if(TextAt(0xA72B055,32)==TextAt(0x58F6F8,32)){TimeText(L"Перейдите на локальную карту клавишей Enter.");return;}
 timeadvance::State state{};if(!TimeRead(state)){TimeText(L"Не удалось прочитать живого персонажа.");return;}if(auto why=timeadvance::Danger(state)){TimeText(why);return;}
 if(int64_t(state.minutes)+days*1440>INT32_MAX){TimeText(L"Недопустимое значение игрового времени.");return;}
 if(!EnsureGameForeground()||ProbeNative().stage!=NativeStage::Main){TimeText(L"Не удалось подтвердить основной экран после переключения. Ввод не отправлен.");return;}
 g_previousLayout=GetKeyboardLayout(g_thread);g_english=LoadKeyboardLayoutW(L"00000409",0);DWORD_PTR ignored=0;
 if(!g_english||!SendMessageTimeoutW(g_game,WM_INPUTLANGCHANGEREQUEST,0,(LPARAM)g_english,SMTO_ABORTIFHUNG,500,&ignored)){LayoutBack();TimeText(L"Не удалось включить английскую раскладку.");return;}
 g_timeIdentity=state;g_timeStart=g_timeBatchStart=state.minutes;g_timeRequested=days;g_timeWaiting=false;g_timeActive=true;g_busy=true;g_timeNext=GetTickCount64()+300;
 EnableWindow(g_timeRun,FALSE);EnableWindow(g_timeDays,FALSE);
 for(HWND h:{g_itemButton,g_begin,g_native,g_refresh,g_games,g_add,g_item,g_quantity,g_update,g_days,g_courseButton})EnableWindow(h,FALSE);
 TimeText(L"Идёт обычное игровое ожидание. Не переключайте окно; еда, сон и опасности сохраняются.");Log(L"Ход времени: "+std::to_wstring(days)+L" дн., штатные ожидания по 4 часа. Календарь напрямую не меняется.");
}
void TimeTick(){
 if(!g_timeActive)return;auto now=GetTickCount64();if(now<g_timeNext)return;g_timeNext=now+150;
 if(!g_process||WaitForSingleObject(g_process,0)!=WAIT_TIMEOUT){TimeFinish(L"Игра закрыта.");return;}
 if(!EnsureGameForeground()){TimeFinish(L"Не удалось вернуть фокус игре после коротких повторов.");return;}
 timeadvance::State current{};if(!TimeRead(current)||!timeadvance::SamePlayer(current,g_timeIdentity)){TimeFinish(L"Персонаж или состояние игры изменились.");return;}
 if(auto why=timeadvance::Danger(current)){TimeFinish(why);return;}
 auto state=ProbeNative();bool main=state.stage==NativeStage::Main;
 if(g_timeWaiting){
  auto result=timeadvance::Result(g_timeBatchStart,current.minutes,main);
  if(result==timeadvance::Batch::Invalid){TimeFinish(L"Неожиданное изменение игрового времени. Повторного ввода не будет.");return;}
  if(result==timeadvance::Batch::Interrupted){g_timeWaiting=false;TimeFinish(L"Игра прервала ожидание. Разберитесь с причиной перед новым запуском.");return;}
  if(result==timeadvance::Batch::Complete){g_timeWaiting=false;g_timeBatchStart=current.minutes;g_timeNext=now+400;TimeText(TimeProgress(current.minutes));Log(TimeProgress(current.minutes));return;}
  if(state.readerCall&&!main){TimeFinish(L"Игра открыла диалог или другой экран. Ответьте вручную.");return;}
  if(now>g_timeBatchDeadline){TimeFinish(L"Истекло время подтверждения ожидания. Уже отправленная команда не повторяется.");return;}
  if(!main&&TimeWaitFrame()){g_timeSeenWait=true;g_timeUnknownSince=0;return;}
  if(!g_timeUnknownSince)g_timeUnknownSince=now;
  if(now-g_timeUnknownSince>3000){TimeFinish(L"Не удалось подтвердить выполняемое ожидание. Ввод остановлен.");return;}
  return;
 }
 if(!main||current.minutes!=g_timeBatchStart){TimeFinish(L"Основной экран или ожидаемое игровое время изменились. Ввод не отправлен.");return;}
 if(int64_t(current.minutes)-g_timeStart==int64_t(g_timeRequested)*1440){TimeFinish(L"Запрошенное игровое время прошло.");return;}
 // Recheck immediately before the one-shot key. Never replay on failure.
 if(GetForegroundWindow()!=g_game||ProbeNative().stage!=NativeStage::Main||!TimeCode()){TimeFinish(L"Состояние изменилось перед вводом. Команда не отправлена.");return;}
 g_timeWaiting=true;g_timeSeenWait=false;g_timeUnknownSince=0;g_timeBatchDeadline=now+180000;
 if(!SendKey(L'-'))TimeFinish(L"Не удалось отправить команду ожидания. Повторного ввода не будет.");
}
LRESULT CALLBACK TimeWindowProc(HWND h,UINT msg,WPARAM w,LPARAM l){
 switch(msg){case WM_CREATE:{g_timeWindow=h;
  auto make=[&](const wchar_t* cls,const wchar_t* title,DWORD style,int x,int y,int width,int height,int id){HWND c=CreateWindowExW(!wcscmp(cls,L"EDIT")?WS_EX_CLIENTEDGE:0,cls,title,WS_CHILD|WS_VISIBLE|style,x,y,width,height,h,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return c;};
  make(L"STATIC",L"Реальное ожидание: игра обрабатывает время, питание, сон и события. Подходит для ожидания сушки и других процессов, если выполнены их игровые условия.",0,14,14,580,48,1);
  make(L"STATIC",L"Дней (1–7):",0,14,76,110,24,2);g_timeDays=make(L"EDIT",L"1",ES_NUMBER|WS_TABSTOP,130,72,70,26,3);SendMessageW(g_timeDays,EM_SETLIMITTEXT,2,0);
  g_timeRun=make(L"BUTTON",L"Начать ожидание",WS_TABSTOP,218,70,180,32,4);make(L"BUTTON",L"Остановить",WS_TABSTOP,410,70,180,32,5);
  make(L"STATIC",L"Ожидание идёт шагами по 4 часа. Персонаж не получает еду, воду или сон автоматически. При ранах, истощении, жажде ≥ 750, потребности во сне ≥ 1000, утомлении ≥ 50% или запасе питания < 1000 новые шаги прекращаются. Это осторожные пороги тренера.",0,14,122,580,86,6);
  make(L"STATIC",L"«Остановить» отменяет следующие шаги. Текущее ожидание завершает игра; для немедленной отмены нажмите Esc в игре. Диалоги и опасности не пропускаются. После еды/воды/сна задайте оставшееся число дней заново.",0,14,220,580,66,7);
  g_timeStatus=make(L"STATIC",L"Нужен живой персонаж на локальной карте. Сначала выберите безопасное место и сохраните резервную копию.",0,14,302,580,64,8);return 0;}
 case WM_COMMAND:if(LOWORD(w)==4)TimeBegin();else if(LOWORD(w)==5&&g_timeActive)TimeFinish(L"Остановлено пользователем.");return 0;
 case WM_CLOSE:if(g_timeActive)TimeFinish(L"Окно ожидания закрыто пользователем.");DestroyWindow(h);return 0;
 case WM_DESTROY:g_timeWindow=g_timeDays=g_timeRun=g_timeStatus=nullptr;return 0;
 }return DefWindowProcW(h,msg,w,l);
}
void OpenTimeWindow(){if(g_timeWindow){ShowWindow(g_timeWindow,SW_SHOWNORMAL);SetForegroundWindow(g_timeWindow);return;}
 if(g_busy){Log(L"Сначала остановите текущую автоматику.");return;}
 WNDCLASSW wc{};wc.lpfnWndProc=TimeWindowProc;wc.hInstance=GetModuleHandleW(nullptr);wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"URWTrainerTime017";RegisterClassW(&wc);
 CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,L"Ход игрового времени — URW Trainer",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|(g_timeTest?0:WS_VISIBLE),CW_USEDEFAULT,CW_USEDEFAULT,628,414,g_window,nullptr,wc.hInstance,nullptr);
}

void TimeWindowSelfTest(){unsigned checks=0,failures=0;auto check=[&](bool ok){++checks;if(!ok)++failures;};
 OpenTimeWindow();check(g_timeWindow!=nullptr);check(g_timeDays!=nullptr);check(IsWindowEnabled(g_timeRun));check(!g_timeActive&&!g_process);
 wchar_t text[32]{};GetWindowTextW(g_timeDays,text,32);check(std::wstring(text)==L"1");
 for(int id:{1,2,6,7,8}){HWND c=GetDlgItem(g_timeWindow,id);RECT actual{};GetClientRect(c,&actual);std::wstring caption=CharacterText(c);RECT measured{0,0,actual.right,0};HDC dc=GetDC(c);auto old=SelectObject(dc,(HFONT)SendMessageW(c,WM_GETFONT,0,0));DrawTextW(dc,caption.c_str(),-1,&measured,DT_CALCRECT|DT_WORDBREAK);SelectObject(dc,old);ReleaseDC(c,dc);check(measured.bottom<=actual.bottom);}
 SendMessageW(g_timeWindow,WM_COMMAND,4,0);check(!g_timeActive&&!g_busy&&!g_process);check(CharacterText(g_timeStatus).find(L"отключены")!=std::wstring::npos);
 SendMessageW(g_timeWindow,WM_COMMAND,5,0);check(!g_timeActive&&!g_process);SendMessageW(g_timeWindow,WM_CLOSE,0,0);check(g_timeWindow==nullptr);
 std::ofstream out("TRAINER_TIME_SELFTEST.json");out<<"{\"checks\":"<<checks<<",\"failures\":"<<failures<<",\"fixture_only\":true,\"game_input_sent\":false,\"game_handles_opened\":0}\n";PostMessageW(g_window,WM_CLOSE,0,0);
}
