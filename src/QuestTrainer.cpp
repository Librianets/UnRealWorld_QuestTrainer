#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <commctrl.h>
#include <filesystem>
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cwctype>
#include <cerrno>
#include <functional>
#include "CharacterState.h"
#include "QuestPlan.h"
#pragma comment(lib,"bcrypt.lib")
#pragma comment(lib,"comctl32.lib")
using namespace qtrainer;
namespace {
constexpr wchar_t kClass[]=L"URWQuestTrainerWindow";
constexpr char kHash[]="c6ed73642c1b1cb1f86b818c7586224398c2f0554f6114ced31bc40ff071806d";
constexpr uintptr_t kPrompt=0x65A8CF8,kMenuCount=0x6354A34,kMenuKeys=0x634EFD8;
constexpr uintptr_t kPlayer=0xA783C10,kCounter=0xA72B0CC,kQuest=0xA82D3B0;
HWND g_window,g_log,g_games,g_choices,g_status,g_begin,g_native,g_refresh;
HWND g_item,g_quantity,g_add,g_update,g_days;
HWND g_itemButton=nullptr,g_itemWindow=nullptr,g_itemSearch=nullptr,g_itemType=nullptr,g_itemList=nullptr,g_itemInfo=nullptr;
bool g_browserTest=false;
bool g_itemFilling=false,g_itemDescending=false;int g_itemSort=0;
void RefreshItemBrowser(bool types=false);void OpenItemWindow();
HWND g_skillLock=nullptr,g_skillUnlock=nullptr,g_skillInfo=nullptr;
HANDLE g_skillWriter=nullptr;uintptr_t g_skillCounter=0;bool g_skillFrozen=false;
int g_skillTarget=0;ULONGLONG g_skillLastSeen=0;unsigned g_skillWrites=0;
void StopSkillFreeze(const wchar_t* reason=nullptr);
void TimeForget();
void AnimalForget();
HWND g_courseButton,g_courseWindow=nullptr,g_courseList=nullptr,g_courseInfo=nullptr,g_courseRun=nullptr;
bool g_courseBusy=false,g_courseFilling=false;
void RefreshCourse(bool selectCurrent=false);
HANDLE g_process=nullptr;HWND g_game=nullptr;DWORD g_pid=0,g_thread=0;uintptr_t g_base=0;
HKL g_previousLayout=nullptr,g_english=nullptr;bool g_busy=false,g_hadFirst=false;
int g_type=3,g_boot=0,g_index=0,g_phase=0;ULONGLONG g_due=0,g_deadline=0;uint32_t g_before=0;
Plan g_plan;
bool g_daysMode=false;uint32_t g_daysBefore=0,g_dayDelta=0,g_minutesBefore=0;
bool g_itemMode=false;std::string g_itemName;std::wstring g_itemLabel;int g_itemCount=1;uint32_t g_itemId=0;
ULONGLONG g_stepDeadline=0;std::string g_typed;size_t g_typedIndex=0;
struct Game {DWORD pid;HWND hwnd;std::wstring path;};std::vector<Game> g_available;
std::wstring Wide(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(1252,0,s.data(),(int)s.size(),nullptr,0);std::wstring o(n,0);MultiByteToWideChar(1252,0,s.data(),(int)s.size(),o.data(),n);return o;}
void Log(const std::wstring& s){SYSTEMTIME st;GetLocalTime(&st);wchar_t stamp[32];swprintf_s(stamp,L"[%02u:%02u:%02u] ",st.wHour,st.wMinute,st.wSecond);auto line=std::wstring(stamp)+s+L"\r\n";SendMessageW(g_log,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);SendMessageW(g_log,EM_REPLACESEL,FALSE,(LPARAM)line.c_str());}
void Status(const wchar_t* s){SetWindowTextW(g_status,s);}
std::string HashFile(const std::wstring& path){
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return {};
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::string result;DWORD cb=0,objSize=0;std::vector<BYTE> object;std::array<BYTE,32> digest{};std::array<BYTE,65536> data{};
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objSize,sizeof(objSize),&cb,0)>=0){
  object.resize(objSize);if(BCryptCreateHash(alg,&hash,object.data(),objSize,nullptr,0,0)>=0){bool okay=true;DWORD used=0;
   while(true){if(!ReadFile(file,data.data(),(DWORD)data.size(),&used,nullptr)){okay=false;break;}if(!used)break;if(BCryptHashData(hash,data.data(),used,0)<0){okay=false;break;}}
   if(okay&&BCryptFinishHash(hash,digest.data(),(ULONG)digest.size(),0)>=0){std::ostringstream s;s<<std::hex<<std::setfill('0');for(auto b:digest)s<<std::setw(2)<<(int)b;result=s.str();}
  }
 }
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(file);return result;
}
bool Read(uintptr_t va,void* out,size_t size){SIZE_T used=0;return g_process&&g_base&&ReadProcessMemory(g_process,(void*)(g_base+va-0x400000),out,size,&used)&&used==size;}
uint32_t NumberAt(uintptr_t va){uint32_t v=0;Read(va,&v,sizeof(v));return v;}
std::string TextAt(uintptr_t va,size_t length){std::vector<char> data(length+1,0);if(!Read(va,data.data(),length))return {};return std::string(data.data());}
// Recover the native scalar from matching print/decrement/increment/setup
// operands.3.89.1 moved this data compared with the3.89 IDA export.
bool ResolveSkillCounter(){
 unsigned char show[6]{},dec[6]{},inc[6]{},init[6]{};
 if(!Read(0x411D5A,show,6)||!Read(0x41202D,dec,6)||!Read(0x4120D1,inc,6)||!Read(0x41216F,init,6))return false;
 if(show[0]!=0xFF||show[1]!=0x35||dec[0]!=0xFF||dec[1]!=0x0D||inc[0]!=0xFF||inc[1]!=0x05||init[0]!=0x89||init[1]!=0x15)return false;
 uint32_t ptr=0;memcpy(&ptr,show+2,4);
 if(memcmp(show+2,dec+2,4)||memcmp(show+2,inc+2,4)||memcmp(show+2,init+2,4)||ptr<g_base||ptr+4>g_base+0x200000)return false;
 MEMORY_BASIC_INFORMATION mi{};
 if(!VirtualQueryEx(g_process,(void*)ptr,&mi,sizeof(mi))||mi.State!=MEM_COMMIT||(mi.Protect&(PAGE_NOACCESS|PAGE_GUARD))||!(mi.Protect&(PAGE_READWRITE|PAGE_WRITECOPY)))return false;
 g_skillCounter=ptr;return true;
}
bool QuestionActive(){int count=(int)NumberAt(kMenuCount);if(count<3||count>32)return false;bool yes=false,no=false,esc=false;for(int i=0;i<count;++i){int key=(int)NumberAt(kMenuKeys+232*i);yes|=key==89;no|=key==78;esc|=key==27;}return yes&&no&&esc;}
#include "HookState.h"
void LayoutBack(){if(g_previousLayout&&IsWindow(g_game)&&GetKeyboardLayout(g_thread)==g_english){DWORD_PTR ignored=0;SendMessageTimeoutW(g_game,WM_INPUTLANGCHANGEREQUEST,0,(LPARAM)g_previousLayout,SMTO_ABORTIFHUNG,500,&ignored);}g_previousLayout=nullptr;}
void Stop(const wchar_t* reason){AnimalForget();TimeForget();StopSkillFreeze();g_busy=false;g_courseBusy=false;LayoutBack();EnableWindow(g_begin,TRUE);EnableWindow(g_native,TRUE);EnableWindow(g_refresh,TRUE);EnableWindow(g_games,TRUE);EnableWindow(g_itemButton,TRUE);EnableWindow(g_add,TRUE);EnableWindow(g_item,TRUE);EnableWindow(g_quantity,TRUE);EnableWindow(g_update,TRUE);EnableWindow(g_days,TRUE);EnableWindow(g_courseButton,TRUE);if(g_courseWindow){EnableWindow(g_courseRun,TRUE);RefreshCourse();}Status(reason);Log(reason);}
void Disconnect(){AnimalForget();TimeForget();StopSkillFreeze();g_skillCounter=0;LayoutBack();if(g_process)CloseHandle(g_process);g_process=nullptr;g_game=nullptr;g_base=0;g_pid=0;}
bool Attach(const Game& game){
 Disconnect();if(HashFile(game.path)!=kHash){Log(L"Неподдерживаемый EXE: требуется чистая UnReal World 3.89.1.");return false;}
 g_process=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|SYNCHRONIZE,FALSE,game.pid);if(!g_process){Log(L"Не удалось открыть процесс для чтения.");return false;}
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,game.pid);MODULEENTRY32W m{};m.dwSize=sizeof(m);
 if(snapshot!=INVALID_HANDLE_VALUE){if(Module32FirstW(snapshot,&m))g_base=(uintptr_t)m.modBaseAddr;CloseHandle(snapshot);}
 if(!g_base){Disconnect();Log(L"Не удалось определить базовый адрес EXE.");return false;}
 g_game=game.hwnd;g_pid=game.pid;g_thread=GetWindowThreadProcessId(g_game,nullptr);
 ResolveSkillCounter();
 if(!LoadHookState()){Log(L"Не удалось проверить совместимость с загруженным хуком: "+Wide(g_hookState.error));return false;}return true;
}
BOOL CALLBACK FindWindow(HWND hwnd,LPARAM){
 if(!IsWindowVisible(hwnd))return TRUE;wchar_t cls[96]{};GetClassNameW(hwnd,cls,96);if(wcscmp(cls,L"SDL_app"))return TRUE;
 DWORD pid=0;GetWindowThreadProcessId(hwnd,&pid);HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!p)return TRUE;
 wchar_t path[32768]{};DWORD length=_countof(path);bool okay=QueryFullProcessImageNameW(p,0,path,&length)!=FALSE;CloseHandle(p);
 if(okay&&HashFile(path)==kHash)g_available.push_back({pid,hwnd,path});return TRUE;
}
void Refresh(){if(g_busy)return;g_available.clear();EnumWindows(FindWindow,0);SendMessageW(g_games,CB_RESETCONTENT,0,0);
 for(const auto& g:g_available){std::wstring label=L"PID "+std::to_wstring(g.pid)+L" — "+g.path;SendMessageW(g_games,CB_ADDSTRING,0,(LPARAM)label.c_str());}
 if(!g_available.empty()){SendMessageW(g_games,CB_SETCURSEL,0,0);Status(L"Игра найдена. Загрузите персонажа и откройте локальную карту.");}
 else Status(L"Поддерживаемая игра не найдена. Сначала запустите UnReal World 3.89.1.");
 Log(L"Найдено окон игры: "+std::to_wstring(g_available.size()));
}
template<class Valid,class Active,class Request,class Pause>
bool RetryForeground(Valid valid,Active active,Request request,Pause pause){
 for(unsigned attempt=0;attempt<4;++attempt){
  if(!valid())return false;if(active())return true;
  request();pause();if(!valid())return false;if(active())return true;
 }
 return false;
}
bool EnsureGameForeground(){
 auto valid=[](){DWORD pid=0;return IsWindow(g_game)&&GetWindowThreadProcessId(g_game,&pid)==g_thread&&pid==g_pid&&g_process&&WaitForSingleObject(g_process,0)==WAIT_TIMEOUT;};
 return RetryForeground(valid,[](){return GetForegroundWindow()==g_game;},[](){if(IsIconic(g_game))ShowWindow(g_game,SW_RESTORE);SetForegroundWindow(g_game);},[](){Sleep(80);});
}
bool SendKey(wchar_t ch){
 if(!IsWindow(g_game)||GetForegroundWindow()!=g_game)return false;
 if(GetAsyncKeyState(VK_CONTROL)<0||GetAsyncKeyState(VK_MENU)<0||GetAsyncKeyState(VK_SHIFT)<0)return false;
 WORD vk=ch==L'*'?VK_MULTIPLY:ch==L'-'?VK_SUBTRACT:ch==L'\r'?VK_RETURN:(WORD)(VkKeyScanExW(ch,g_english)&0xff);SHORT mapping=VkKeyScanExW(ch,g_english);if(ch!=L'\r'&&mapping==-1)return false;
 bool shift=ch!=L'*'&&ch!=L'-'&&ch!=L'\r'&&(HIBYTE(mapping)&1);std::vector<INPUT> events;
 auto key=[&](WORD code,bool up){INPUT in{};in.type=INPUT_KEYBOARD;in.ki.wVk=code;in.ki.wScan=(WORD)MapVirtualKeyW(code,MAPVK_VK_TO_VSC);if(up)in.ki.dwFlags=KEYEVENTF_KEYUP;events.push_back(in);};
 if(shift)key(VK_SHIFT,false);key(vk,false);if(SendInput((UINT)events.size(),events.data(),sizeof(INPUT))!=events.size())return false;
 Sleep(45);events.clear();key(vk,true);if(shift)key(VK_SHIFT,true);return SendInput((UINT)events.size(),events.data(),sizeof(INPUT))==events.size();
}
bool SendVirtual(WORD vk){
 if(!IsWindow(g_game)||GetForegroundWindow()!=g_game||GetAsyncKeyState(VK_CONTROL)<0||GetAsyncKeyState(VK_MENU)<0||GetAsyncKeyState(VK_SHIFT)<0)return false;
 INPUT k{};k.type=INPUT_KEYBOARD;k.ki.wVk=vk;k.ki.wScan=(WORD)MapVirtualKeyW(vk,MAPVK_VK_TO_VSC);
 if(SendInput(1,&k,sizeof(k))!=1)return false;Sleep(45);k.ki.dwFlags=KEYEVENTF_KEYUP;return SendInput(1,&k,sizeof(k))==1;
}
#include "NativeState.h"
#include "TrainerFlow.h"
#include "CharacterWindows.h"
#include "TimeAdvance.h"
#include "AnimalSpawn.h"
void CheckUpdate(){
 if(g_busy||g_skillFrozen){Log(L"Сначала остановите автоматику и разморозьте очки.");return;}
 wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,_countof(exe));std::wstring path=exe;
 auto at=path.find_last_of(L"\\/");if(at==std::wstring::npos)return;
 std::wstring script=path.substr(0,at+1)+L"URW_TrainerUpdater.ps1";
 if(GetFileAttributesW(script.c_str())==INVALID_FILE_ATTRIBUTES){Log(L"Не найден URW_TrainerUpdater.ps1. Распакуйте полный архив тренера.");return;}
 std::wstring args=L"-NoProfile -ExecutionPolicy Bypass -File \""+script+L"\" -CurrentVersion 0.23 -TrainerPath \""+path+L"\" -TrainerPid "+std::to_wstring(GetCurrentProcessId());
 if((INT_PTR)ShellExecuteW(g_window,L"open",L"powershell.exe",args.c_str(),path.substr(0,at).c_str(),SW_HIDE)<=32)Log(L"Не удалось запустить проверку обновлений.");
 else Log(L"Проверяю последнюю версию тренера на GitHub...");
}
HWND Control(const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id){HWND v=CreateWindowExW(!wcscmp(cls,L"EDIT")?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,g_window,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);SendMessageW(v,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return v;}
LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
 switch(msg){case WM_CREATE:{g_window=hwnd;characterstate::Configure(HashFile,CharacterMainCheck);
  g_log=Control(L"EDIT",L"",ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,12,12,450,385,1);SendMessageW(g_log,EM_SETLIMITTEXT,1000000,0);
  Control(L"STATIC",L"UnReal World 3.89.1 — тренер 0.23",0,478,12,290,24,2);
  Control(L"STATIC",L"1. Загрузите персонажа. Нужна локальная карта, рядом с NPC; диалоги закрыты.",0,478,42,285,46,3);
  g_games=Control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,478,94,285,200,10);
  g_refresh=Control(L"BUTTON",L"Найти игру",WS_TABSTOP,478,126,285,30,11);
  Control(L"STATIC",L"2. Выберите задание:",0,478,168,285,22,4);
  g_choices=Control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,478,194,285,350,12);
  for(int id:Choices()){auto it=std::find_if(Questions().begin(),Questions().end(),[&](const Question& q){return q.type==id;});auto pos=SendMessageW(g_choices,CB_ADDSTRING,0,(LPARAM)it->label);SendMessageW(g_choices,CB_SETITEMDATA,pos,id);if(id==72)SendMessageW(g_choices,CB_SETCURSEL,pos,0);}
  g_begin=Control(L"BUTTON",L"Создать задание",WS_TABSTOP,478,232,285,34,13);
  g_native=Control(L"BUTTON",L"Открыть штатное меню вручную",WS_TABSTOP,478,276,285,30,14);
  Control(L"BUTTON",L"Остановить автоматику",WS_TABSTOP,478,316,285,30,15);
  Control(L"STATIC",L"3. Укажите NPC в игре. После создания поговорите с ним и примите задание. Условия игры и ограничения генератора сохраняются.",0,478,358,285,65,5);
  g_itemButton=Control(L"BUTTON",L"Добавление предметов...",WS_TABSTOP,478,440,285,36,30);
  Control(L"STATIC",L"Отдельное окно: тип и поиск RU / EN.",0,478,480,285,22,16);
  Control(L"BUTTON",L"Животные рядом...",WS_TABSTOP,478,514,285,32,35);
  g_days=Control(L"BUTTON",L"Довести стаж до 100 дней",WS_TABSTOP,478,608,285,30,23);
  Control(L"BUTTON",L"Ход времени (N дней)...",WS_TABSTOP,478,558,285,34,34);
  g_update=Control(L"BUTTON",L"Проверить обновления тренера",WS_TABSTOP,478,650,285,30,22);
  Control(L"BUTTON",L"Персонаж...",WS_TABSTOP,12,414,144,36,31);
  Control(L"BUTTON",L"Навыки...",WS_TABSTOP,166,414,144,36,32);
  Control(L"BUTTON",L"Состояние / раны...",WS_TABSTOP,320,414,142,36,33);
  Control(L"STATIC",L"Очки повышения навыков:",0,12,480,450,22,29);
  g_skillLock=Control(L"BUTTON",L"Закрепить очки",WS_TABSTOP,12,508,215,32,26);
  g_skillUnlock=Control(L"BUTTON",L"Разморозить",WS_TABSTOP,237,508,225,32,27);EnableWindow(g_skillUnlock,FALSE);
  g_skillInfo=Control(L"STATIC",L"Откройте распределение навыков в игре. После разблокировки потратьте оставшиеся очки.",0,12,550,450,44,28);
  g_courseButton=Control(L"BUTTON",L"Учебные задания F5...",WS_TABSTOP,12,608,450,34,24);
  Control(L"STATIC",L"Отдельный список этапов курса с отметками выполнения.",0,12,650,450,30,25);
  g_status=Control(L"STATIC",L"",0,12,694,750,48,6);SetTimer(hwnd,1,60,nullptr);SetTimer(hwnd,2,30,nullptr);
  if(g_animalTest){SetTimer(hwnd,6,20,nullptr);return 0;}
  if(g_timeTest){SetTimer(hwnd,5,20,nullptr);return 0;}
  if(g_editorTest){SetTimer(hwnd,4,20,nullptr);return 0;}
  if(g_browserTest){g_items={{25,"Club",ItemLabel("Club"),1},{302,"Arrow",L"Стрела | Arrow [#302]",1},{303,"Arrow",L"Стрела | Arrow [#303]",1},{1,"Nettle leaves",ItemLabel("Nettle leaves"),11}};SetTimer(hwnd,3,100,nullptr);return 0;}
  Log(L"Тренер 0.23. Штатный ввод; фиксация очков включается отдельно.");Refresh();if(SelectGame())LoadItems();return 0;}
 case WM_TIMER:if(w==6){KillTimer(hwnd,6);AnimalWindowSelfTest();}else if(w==5){KillTimer(hwnd,5);TimeWindowSelfTest();}else if(w==4){KillTimer(hwnd,4);if(g_editorAutoTest)CharacterWindowSelfTest();else OpenCharacterWindow(0);}else if(w==3){KillTimer(hwnd,3);OpenItemWindow();}else if(w==2)TickSkillFreeze();else if(g_animalActive)AnimalTick();else if(g_timeActive)TimeTick();else Tick();return 0;
 case WM_COMMAND:if(g_animalActive&&LOWORD(w)!=15)return 0;switch(LOWORD(w)){case 11:Refresh();if(SelectGame())LoadItems();break;case 13:Begin(false);break;case 14:Begin(true);break;case 30:OpenItemWindow();break;case 31:OpenCharacterWindow(0);break;case 32:OpenCharacterWindow(1);break;case 33:OpenCharacterWindow(2);break;case 34:OpenTimeWindow();break;case 35:OpenAnimalWindow();break;case 21:if(!g_busy&&SelectGame())LoadItems();break;case 22:CheckUpdate();break;case 23:Begin(false,false,true);break;case 24:OpenCourseWindow();break;case 26:StartSkillFreeze();break;case 27:StopSkillFreeze(L"Очки разморожены. Распределите оставшиеся повышения в игре.");break;case 15:if(g_animalActive){AnimalRequestStop();break;}if(g_timeActive){TimeFinish(L"Остановлено пользователем.");break;}Stop(L"Автоматика остановлена. Завершите открытый диалог в игре вручную.");break;}return 0;
 case WM_CLOSE:if(g_animalActive){AnimalRequestStop();if(g_animalActive)return 0;}break;
 case WM_DESTROY:if(g_animalWindow)DestroyWindow(g_animalWindow);KillTimer(hwnd,6);if(g_timeWindow)DestroyWindow(g_timeWindow);KillTimer(hwnd,1);KillTimer(hwnd,2);KillTimer(hwnd,4);KillTimer(hwnd,5);for(auto& editor:g_characterWindows)if(editor.window)DestroyWindow(editor.window);if(g_itemWindow)DestroyWindow(g_itemWindow);if(g_courseWindow)DestroyWindow(g_courseWindow);Disconnect();PostQuitMessage(0);return 0;
 }return DefWindowProcW(hwnd,msg,w,l);
}
bool InputSelfTest(std::string& received){
 wchar_t file[32768]{};GetModuleFileNameW(nullptr,file,_countof(file));std::wstring path=file;path.resize(path.find_last_of(L"\\/")+1);
 HMODULE dll=LoadLibraryExW((path+L"test_game\\SDL2.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!dll)return false;
 auto init=(int(__cdecl*)(unsigned))GetProcAddress(dll,"SDL_Init");
 auto create=(void*(__cdecl*)(const char*,int,int,int,int,unsigned))GetProcAddress(dll,"SDL_CreateWindow");
 auto destroy=(void(__cdecl*)(void*))GetProcAddress(dll,"SDL_DestroyWindow");
 auto poll=(int(__cdecl*)(void*))GetProcAddress(dll,"SDL_PollEvent");
 auto start=(void(__cdecl*)())GetProcAddress(dll,"SDL_StartTextInput");
 auto quit=(void(__cdecl*)())GetProcAddress(dll,"SDL_Quit");
 if(!init||!create||!destroy||!poll||!start||!quit){FreeLibrary(dll);return false;}
 if(init(0x20)!=0){FreeLibrary(dll);return false;}
 HWND old=GetForegroundWindow();void* window=create("URW trainer input self-test",200,200,480,120,4);if(!window){quit();FreeLibrary(dll);return false;}
 EnumWindows([](HWND h,LPARAM)->BOOL{DWORD pid=0;GetWindowThreadProcessId(h,&pid);wchar_t name[96]{};GetClassNameW(h,name,96);if(pid==GetCurrentProcessId()&&!wcscmp(name,L"SDL_app"))g_game=h;return TRUE;},0);
 SetForegroundWindow(g_game);g_thread=GetCurrentThreadId();g_previousLayout=GetKeyboardLayout(g_thread);g_english=LoadKeyboardLayoutW(L"00000409",0);
 SendMessageW(g_game,WM_INPUTLANGCHANGEREQUEST,0,(LPARAM)g_english);Sleep(150);start();bool okay=true;
 for(wchar_t ch:std::wstring(L"*-j\rdq\r")){
  size_t before=received.size();if(!SendKey(ch)){okay=false;break;}
  auto deadline=GetTickCount64()+1500;
  while(GetTickCount64()<deadline){alignas(8) unsigned char event[64]{};
   while(poll(event)){unsigned type=0;memcpy(&type,event,4);if(type==0x303)received+=(const char*)(event+12);if(type==0x300){int key=0;memcpy(&key,event+20,4);if(key==13)received+='\r';}}
   if(received.size()>before)break;Sleep(10);
  }
  if(received.size()==before){okay=false;break;}Sleep(120);
 }
 for(WORD vk:{WORD(VK_F5),WORD(VK_ESCAPE)}){
  bool seen=false;if(!SendVirtual(vk)){okay=false;break;}auto until=GetTickCount64()+1000;
  while(GetTickCount64()<until&&!seen){alignas(8) unsigned char event[64]{};while(poll(event)){unsigned type=0;int sym=0;memcpy(&type,event,4);memcpy(&sym,event+20,4);if(type==0x300&&sym==(vk==VK_F5?1073741886:27))seen=true;}if(!seen)Sleep(10);}
  okay=okay&&seen;
 }
 LayoutBack();destroy(window);g_game=nullptr;quit();FreeLibrary(dll);if(IsWindow(old))SetForegroundWindow(old);
 return okay&&received=="*-j\rdq\r";
}
int SelfTest(bool native=true){int failed=0,checks=0;auto check=[&](bool v){++checks;if(!v)++failed;};
 check(MinimumQuestDays(15)==10);check(MinimumQuestDays(72)==0);check(MinimumQuestDays(70)==0);check(MinimumQuestDays(5)==2);check(MinimumQuestDays(51)==30);check(!ItemRussian("Nettle leaves").empty());check(ItemRussian("Unknown mod prototype").empty());
 g_items={{302,"Arrow",L"Стрела | Arrow [#302]"},{303,"Arrow",L"Стрела | Arrow [#303]"},{1,"Nettle leaves",ItemLabel("Nettle leaves")}};
 bool ambiguous=false;auto* chosen=ResolveItem(L"Arrow",303,ambiguous);check(chosen&&chosen->id==303);
 chosen=ResolveItem(L"Arrow",0,ambiguous);check(!chosen&&ambiguous);
 chosen=ResolveItem(Utf8Label(ItemRussian("Nettle leaves")),0,ambiguous);check(chosen&&chosen->id==1&&!ambiguous);

 check(ItemSearchMatches(g_items[0],L"ARROW"));check(ItemSearchMatches(g_items[0],L"стрЕЛ"));
 check(!ItemSearchMatches(g_items[0],L"дубина"));
 ItemEntry club{25,"Club",ItemLabel("Club"),1};
 check(ItemSearchMatches(club,L"fine club"));check(ItemSearchMatches(club,L"FINE CL"));
 check(!ItemSearchMatches(club,L"fine arrow"));check(ItemSearchMatches(club,L"дубин"));
 check(ItemSearchMatches(club,L"25"));check(!ItemSearchMatches(club,L"999"));
 check(ItemCategory(1)==L"Оружие");check(ItemCategory(2)==L"Одежда и доспехи");
 check(ItemCategory(255)==L"Тип 255");
 check(ItemSearchMatches(club,L"добротная дубина"));
 check(CompareItems(club,ItemEntry{1,"Nettle leaves",ItemLabel("Nettle leaves"),11},0)<0);check(CompareItems(g_items[0],g_items[1],3)<0);check(CompareItems(g_items[0],g_items[1],2)<0);
 g_items.clear();
 check(Choices().size()==16);check(!Supported(100));check(!Supported(-1));check(!Supported(0));check(Supported(3));check(Supported(5));
 for(int id:Choices()){bool selected=false;for(const auto& q:Questions()){check(q.source!=nullptr);check(Matches(std::string("?")+q.source,q.source));check(!Matches("?A different question",q.source));if(q.type==id){selected=true;break;}}check(selected);}
 for(int id:Choices()){
  Plan plan;check(plan.Reset(id));check(plan.Next(false,"?Homeland escort")==Answer::Wait);check(plan.index==0);
  check(plan.Next(true,"?Unrelated dialogue")==Answer::Wait);check(plan.index==0);
  bool yes=false;
  for(const auto& q:Questions()){
   auto answer=plan.Next(true,std::string("?")+q.source);
   if(q.type==id){check(answer==Answer::Yes);yes=true;break;}check(answer==Answer::No);
  }
  check(yes);check(plan.Next(true,"?Homeland escort")==Answer::Finished);
 }
 std::vector<int> cq;
 check(CourseSelection(1,4,{true,true,false,false},cq)&&cq==std::vector<int>{1});
 check(CourseSelection(1,4,{false,true,true,false},cq)&&cq==std::vector<int>({1,2}));
 check(!CourseSelection(1,4,{false,false,true,false},cq)&&cq.empty());
 check(!CourseSelection(1,4,{false,true,false,true},cq)&&cq.empty());
 check(!CourseSelection(-1,4,{true,true,true,true},cq));
 check(!CourseSelection(4,4,{true,true,true,true},cq));
 check(!CourseSelection(0,4,{true},cq));check(!CourseSelection(0,4,{false,false,false,false},cq));
 check(CourseSelection(3,4,{false,false,false,true},cq)&&cq==std::vector<int>{3});
 for(int id=1;id<=49;++id)check((CourseRussian(id)!=nullptr)==(id!=23));
 Plan invalid;check(!invalid.Reset(100));check(invalid.Next(true,"?Treasuretale?")==Answer::Wait);
 check(!Matches("?Fetch Herbs from an unrelated dialogue","Fetch Herbs"));
 check(std::wstring(L"*-j\rdq\r").size()==7);check(HashFile(L"a-file-that-does-not-exist").empty());
 ItemEntry fur{50043,"Squirrel fur",ItemLabel("Squirrel fur"),5};check(ItemSearchMatches(fur,L"мех"));check(ItemSearchMatches(fur,L"fur"));check(!ItemSearchMatches(fur,L"bear"));check(ItemRussian("Forest reindeer fur")=="Мех лесного северного оленя");
 failed+=CharacterModelTests(checks);
 {int requests=0,pauses=0;bool active=false;
  check(RetryForeground([](){return true;},[](){return true;},[&](){++requests;},[&](){++pauses;})&&requests==0&&pauses==0);
  check(!RetryForeground([](){return false;},[](){return true;},[&](){++requests;},[&](){++pauses;})&&requests==0);
  check(RetryForeground([](){return true;},[&](){return active;},[&](){active=++requests==2;},[&](){++pauses;})&&requests==2&&pauses==2);
  requests=pauses=0;check(!RetryForeground([](){return true;},[](){return false;},[&](){++requests;},[&](){++pauses;})&&requests==4&&pauses==4);
  requests=pauses=0;check(!RetryForeground([&](){return requests==0;},[](){return false;},[&](){++requests;},[&](){++pauses;})&&requests==1&&pauses==1);
 }
 std::string received;bool input=false;if(native){input=InputSelfTest(received);check(input);}
 std::ofstream out("TRAINER_SELFTEST.json");out<<"{\"checks\":"<<checks<<",\"failures\":"<<failed<<",\"native_sdl_input\":"<<(native?(input?"true":"false"):"null")<<",\"input_hex\":\"";
 for(unsigned char c:received)out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;out<<"\"}\n";return failed?1:0;
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR command,int show){
 INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&controls);
 std::wstring args=command;auto end=args.find_last_not_of(L" \t\r\n");args=end==std::wstring::npos?L"":args.substr(0,end+1);
 g_animalTest=args==L"/animal-selftest";
 g_timeTest=args==L"/time-selftest";
 g_browserTest=args==L"/browser-test";g_editorTest=args==L"/editor-test"||args==L"/editor-selftest";g_editorAutoTest=args==L"/editor-selftest";
 if(args==L"/selftest")return SelfTest();
 if(args==L"/selftest-core")return SelfTest(false);
 if(args==L"/animal-probe"){
  EnumWindows(FindWindow,0);bool attached=!g_available.empty()&&Attach(g_available.front());auto n=attached?ProbeNative():NativeState{};animalspawn::Snapshot v;
  bool read=attached&&AnimalRead(v,true),code=attached&&AnimalCode(600);unsigned activeFree=0,worldFree=0;if(read){for(auto id:v.ids)activeFree+=id==0;for(const auto& o:v.world)worldFree+=animalspawn::Empty(o);}
  std::ofstream out("TRAINER_ANIMAL_PROBE.json");out<<"{\"attached\":"<<(attached?"true":"false")<<",\"pid\":"<<g_pid<<",\"stage\":"<<(int)n.stage<<",\"reader_call\":"<<n.readerCall<<",\"debug_call\":"<<n.debugCall<<",\"debug_frame\":"<<n.debugFrame<<",\"animal_frame\":"<<n.animalFrame<<",\"animal_return\":"<<n.animalReturn<<",\"cursor_valid\":"<<(n.animalCursor?"true":"false")<<",\"cursor_x\":"<<n.animalX<<",\"cursor_y\":"<<n.animalY<<",\"animal_target\":"<<(n.stage==NativeStage::AnimalTarget?"true":"false")<<",\"animal_species\":"<<(n.stage==NativeStage::AnimalSpecies?"true":"false")<<",\"read\":"<<(read?"true":"false")<<",\"code\":"<<(code?"true":"false")<<",\"player_x\":"<<v.x<<",\"player_y\":"<<v.y<<",\"target_x\":"<<v.tx<<",\"target_y\":"<<v.ty<<",\"terrain_safe\":"<<(v.terrainSafe?"true":"false")<<",\"active_free\":"<<activeFree<<",\"world_free\":"<<worldFree<<",\"target_valid\":"<<(read&&!animalspawn::Validate(v,600,true)?"true":"false")<<",\"game_input\":false}\n";out.close();Disconnect();return attached&&code?0:2;
 }
 if(args==L"/time-probe"){
  EnumWindows(FindWindow,0);bool attached=!g_available.empty()&&Attach(g_available.front());timeadvance::State state{};bool code=attached&&TimeCode(),read=attached&&TimeRead(state),wait=attached&&TimeWaitFrame();auto native=attached?ProbeNative():NativeState{};
  std::ofstream out("TRAINER_TIME_PROBE.json");out<<"{\"attached\":"<<(attached?"true":"false")<<",\"pid\":"<<g_pid<<",\"hook_module\":"<<g_hookState.module<<",\"time_code\":"<<(code?"true":"false")<<",\"state_read\":"<<(read?"true":"false")<<",\"wait_frame\":"<<(wait?"true":"false")<<",\"main\":"<<(native.stage==NativeStage::Main?"true":"false")<<",\"minutes\":"<<state.minutes<<",\"read_only\":true}\n";Disconnect();return attached&&code?0:2;
 }
 if(args==L"/probe"){
  EnumWindows(FindWindow,0);if(g_available.empty()||!Attach(g_available.front()))return 2;
  auto state=ProbeNative();std::ofstream out("NATIVE_STATE.json");
  out<<"{\"skill_counter\":"<<g_skillCounter<<",\"skill_frame\":"<<state.skillFrame<<",\"skill_points\":"<<state.skillPoints<<",\"hook_module\":"<<g_hookState.module<<",\"hook_tls\":"<<g_hookState.tlsIndex<<",\"hook_returns\":"<<state.hookReturns<<",\"stage\":"<<(int)state.stage<<",\"debug_frame\":"<<state.debugFrame<<",\"reader_call\":"<<state.readerCall<<",\"debug_call\":"<<state.debugCall<<",\"returns\":[";
  bool first=true;for(auto va:state.returns){if(!first)out<<',';out<<va;first=false;}out<<"],\"player\":\""<<TextAt(kPlayer,32)<<"\",\"time\":"<<NumberAt(0xA72B8E0)<<"}\n";
  LoadItems();std::ofstream items("NATIVE_ITEMS.tsv");for(const auto& item:g_items)items<<item.id<<'\t'<<item.category<<'\t'<<item.name<<'\n';
  Disconnect();return 0;
 }
 WNDCLASSW c{};c.lpfnWndProc=WindowProc;c.hInstance=instance;c.lpszClassName=kClass;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&c);
 HWND window=CreateWindowExW(0,kClass,L"URW Quest Trainer 0.23",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,795,800,nullptr,nullptr,instance,nullptr);if(!window)return 1;if(!g_editorAutoTest&&!g_timeTest&&!g_animalTest)ShowWindow(window,show);
 MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(g_animalWindow&&IsDialogMessageW(g_animalWindow,&msg))continue;if(g_timeWindow&&IsDialogMessageW(g_timeWindow,&msg))continue;if(g_itemWindow&&IsDialogMessageW(g_itemWindow,&msg))continue;bool editorHandled=false;for(auto& editor:g_characterWindows)if(editor.window&&IsDialogMessageW(editor.window,&msg)){editorHandled=true;break;}if(editorHandled)continue;TranslateMessage(&msg);DispatchMessageW(&msg);}return (int)msg.wParam;
}
