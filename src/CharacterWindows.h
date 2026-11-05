#pragma once
#include <map>
#include <cmath>
#include <cwctype>
#include <cerrno>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include "CharacterState.h"

// Editing is local until the explicit Apply button. Opening/refreshing never
// obtains a write handle. CharacterState owns identity/state validation.
bool g_editorTest=false,g_editorAutoTest=false;
struct CharacterWindow {
 HWND window=nullptr,list=nullptr,group=nullptr,search=nullptr,value=nullptr,help=nullptr,status=nullptr,name=nullptr;
 HWND minus=nullptr,plus=nullptr,minimum=nullptr,maximum=nullptr,stage=nullptr,resetField=nullptr,apply=nullptr,refresh=nullptr,discard=nullptr,close=nullptr;
 unsigned kind=0;bool filling=false,valid=false;uint32_t selected=0;
 characterstate::Snapshot snapshot;std::map<uint32_t,double> staged;std::vector<uint32_t> rows;
};
std::array<CharacterWindow,3> g_characterWindows{};
const wchar_t* CharacterCategory(characterstate::Category c){using C=characterstate::Category;
 switch(c){case C::Attributes:return L"Характеристики";case C::Personality:return L"Личность";case C::Body:return L"Телосложение";case C::Skills:return L"Навыки";case C::States:return L"Состояние";case C::Wounds:return L"Раны";}return L"";}
bool CharacterCategoryAllowed(unsigned kind,characterstate::Category c){using C=characterstate::Category;return kind==0?(c==C::Attributes||c==C::Personality||c==C::Body):kind==1?c==C::Skills:(c==C::States||c==C::Wounds);}
std::wstring CharacterText(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(n+1,L'\0');GetWindowTextW(h,s.data(),n+1);s.resize(n);return s;}
std::wstring CharacterLower(std::wstring s){if(!s.empty())CharLowerBuffW(s.data(),(DWORD)s.size());return s;}
const characterstate::FieldInfo* CharacterField(uint32_t id){for(const auto& f:characterstate::Metadata())if(f.id==id)return &f;return nullptr;}
const characterstate::Value* CharacterValue(const CharacterWindow& w,uint32_t id){for(const auto& v:w.snapshot.values)if(v.id==id)return &v;return nullptr;}
const characterstate::WoundInfo* CharacterWound(const CharacterWindow& w,uint32_t id){for(const auto& wound:w.snapshot.wounds)if(id==3000+wound.index)return &wound;return nullptr;}
bool CharacterIsHealingAction(const CharacterWindow& w,uint32_t id,double value){const auto* wound=CharacterWound(w,id);return wound&&wound->type!=0&&value==0;}
characterstate::FieldInfo CharacterEffectiveField(const CharacterWindow& w,const characterstate::FieldInfo& original){
 auto f=original;
 if(f.id==204){const auto* endurance=CharacterValue(w,2);if(endurance){auto pending=w.staged.find(2);double limit=4*(pending==w.staged.end()?endurance->value:pending->second);f.maximum=std::min(f.maximum,limit);}}
 if(f.category==characterstate::Category::Wounds){const auto* v=CharacterValue(w,f.id);const auto* wound=CharacterWound(w,f.id);f.editable=v&&wound&&wound->type!=0;if(f.editable){f.maximum=wound->healing?std::min(f.maximum,v->value):0;f.hasMaximum=true;}}
 return f;
}
// Native real fields are float32. Nine significant digits survive a text/float
// round trip; fixed four decimals silently changed values merely by staging.
std::wstring CharacterNumber(double v){if(!std::isfinite(v))return L"—";if(v==0)return L"0";std::wostringstream s;s.imbue(std::locale::classic());s<<std::defaultfloat<<std::setprecision(std::numeric_limits<float>::max_digits10)<<v;return s.str();}
bool CharacterParse(const std::wstring& raw,double& value){std::wstring s=raw;for(auto& c:s)if(c==L',')c=L'.';const wchar_t* begin=s.c_str();while(iswspace(*begin))++begin;if(!*begin)return false;wchar_t* end=nullptr;errno=0;double n=wcstod(begin,&end);if(end==begin||errno==ERANGE||!std::isfinite(n))return false;while(iswspace(*end))++end;if(*end)return false;value=n;return true;}
std::wstring CharacterRange(const characterstate::FieldInfo& f){if(!f.editable)return L"Только чтение";if(f.hasMinimum&&f.hasMaximum)return CharacterNumber(f.minimum)+L" … "+CharacterNumber(f.maximum);if(f.hasMinimum)return L"от "+CharacterNumber(f.minimum);if(f.hasMaximum)return L"до "+CharacterNumber(f.maximum);return L"Не установлен";}
bool CharacterValidate(const characterstate::FieldInfo& f,double value,std::wstring& error){
 if(!f.editable){error=L"Этот показатель вычисляет игра; изменение недоступно.";return false;}
 if(!std::isfinite(value)){error=L"Введите конечное число.";return false;}
 if(f.storage==characterstate::Storage::Integer&&value!=std::trunc(value)){error=L"Для этого параметра нужно целое число.";return false;}
 if((f.hasMinimum&&value<f.minimum)||(f.hasMaximum&&value>f.maximum)){error=L"Допустимый диапазон: "+CharacterRange(f)+L".";return false;}
 return true;
}
void CharacterSetStatus(CharacterWindow& w,const std::wstring& text){SetWindowTextW(w.status,text.c_str());}
void CharacterControls(CharacterWindow& w){
 const auto* f=CharacterField(w.selected);const auto* v=CharacterValue(w,w.selected);bool edit=w.valid&&f&&v&&CharacterEffectiveField(w,*f).editable&&!g_busy&&!g_skillFrozen;
 for(HWND h:{w.value,w.minus,w.plus,w.stage,w.resetField})EnableWindow(h,edit);
 EnableWindow(w.minimum,edit&&f->hasMinimum);EnableWindow(w.maximum,edit&&f->hasMaximum);
 EnableWindow(w.apply,w.valid&&!w.staged.empty()&&!g_busy&&!g_skillFrozen&&!g_editorTest);
 EnableWindow(w.discard,!w.staged.empty());
}
void CharacterDetails(CharacterWindow& w){
 const auto* f=CharacterField(w.selected);const auto* v=CharacterValue(w,w.selected);
 if(f){auto effective=CharacterEffectiveField(w,*f);std::wstring text=std::wstring(f->label)+L" — "+(f->help?f->help:L"");if(effective.editable)text+=L"\r\nДиапазон: "+CharacterRange(effective)+L". Шаг: "+CharacterNumber(f->step)+L".";if(f->category==characterstate::Category::Wounds)for(const auto& wound:w.snapshot.wounds)if(f->id==3000+wound.index){text+=L"\r\n"+wound.details;break;}SetWindowTextW(w.help,text.c_str());}
 else SetWindowTextW(w.help,L"Выберите параметр. Изменения сначала добавляются в план и записываются только кнопкой «Применить».");
 if(v){auto it=w.staged.find(v->id);SetWindowTextW(w.value,CharacterNumber(it==w.staged.end()?v->value:it->second).c_str());}else SetWindowTextW(w.value,L"");
 CharacterControls(w);
}
void CharacterFill(CharacterWindow& w){
 if(!w.list)return;w.filling=true;SendMessageW(w.list,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(w.list);w.rows.clear();
 int choice=(int)SendMessageW(w.group,CB_GETCURSEL,0,0);int category=choice>0?(int)SendMessageW(w.group,CB_GETITEMDATA,choice,0):-1;auto search=CharacterLower(CharacterText(w.search));int selected=-1;
 for(const auto& f:characterstate::Metadata()){
  if(!CharacterCategoryAllowed(w.kind,f.category)||(category>=0&&(int)f.category!=category))continue;
  if(!search.empty()&&CharacterLower(std::wstring(f.label)+L" "+(f.help?f.help:L"")).find(search)==std::wstring::npos)continue;
  const auto* v=CharacterValue(w,f.id);int i=(int)w.rows.size();w.rows.push_back(f.id);LVITEMW item{};item.mask=LVIF_TEXT|LVIF_PARAM;item.iItem=i;item.pszText=const_cast<wchar_t*>(f.label);item.lParam=f.id;ListView_InsertItem(w.list,&item);
  std::wstring current=v?CharacterNumber(v->value):L"—";ListView_SetItemText(w.list,i,1,current.data());auto it=w.staged.find(f.id);std::wstring after=it==w.staged.end()?L"":CharacterNumber(it->second);ListView_SetItemText(w.list,i,2,after.data());auto range=CharacterRange(CharacterEffectiveField(w,f));ListView_SetItemText(w.list,i,3,range.data());std::wstring unit=f.unit?f.unit:L"";ListView_SetItemText(w.list,i,4,unit.data());
  if(f.id==w.selected)selected=i;
 }
 if(selected<0&&!w.rows.empty()){selected=0;w.selected=w.rows[0];}if(selected>=0){ListView_SetItemState(w.list,selected,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);ListView_EnsureVisible(w.list,selected,FALSE);}else w.selected=0;
 SendMessageW(w.list,WM_SETREDRAW,TRUE,0);InvalidateRect(w.list,nullptr,TRUE);w.filling=false;CharacterDetails(w);
}
void CharacterStage(CharacterWindow& w,double value){const auto* f=CharacterField(w.selected);const auto* current=CharacterValue(w,w.selected);if(!w.valid||!f||!current)return;std::wstring error;if(!CharacterValidate(CharacterEffectiveField(w,*f),value,error)){CharacterSetStatus(w,error);return;}if(value==current->value&&!CharacterIsHealingAction(w,f->id,value))w.staged.erase(f->id);else w.staged[f->id]=value;CharacterFill(w);CharacterSetStatus(w,L"Запланировано изменений: "+std::to_wstring(w.staged.size())+L". Пока ничего не записано в игру.");}
bool CharacterMainCheck(HANDLE process,uintptr_t base,DWORD thread,std::wstring& error){
 if(!g_process||GetProcessId(process)!=g_pid||base!=g_base||thread!=g_thread){error=L"Подключение к игре изменилось. Обновите данные.";return false;}
 if(g_busy||g_skillFrozen){error=L"Сначала остановите автоматику и разморозьте очки.";return false;}
 if(ProbeNative().stage!=NativeStage::Main){error=L"Нужна локальная карта без открытых меню и диалогов.";return false;}return true;
}
void CharacterRefresh(CharacterWindow& w){
 if(g_busy||g_skillFrozen){CharacterSetStatus(w,L"Сначала остановите автоматику и разморозьте очки.");return;}
 if(!w.staged.empty()&&MessageBoxW(w.window,L"Обновление сбросит запланированные изменения. Обновить данные?",L"Обновить данные",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
 w.staged.clear();w.valid=false;w.snapshot={};
 if(g_editorTest){w.snapshot.name=L"Тест интерфейса";for(const auto& f:characterstate::Metadata()){double v=f.hasMinimum?f.minimum:0;if(f.hasMaximum&&v>f.maximum)v=f.maximum;w.snapshot.values.push_back({f.id,v});}w.valid=true;SetWindowTextW(w.name,L"Тест интерфейса: вымышленные данные, запись в игру отключена.");CharacterFill(w);CharacterSetStatus(w,L"Открытие и изменение плана не обращаются к игре.");return;}
 if(!SelectGame()){CharacterFill(w);CharacterSetStatus(w,L"Не удалось подключиться. Выберите запущенную игру в основном окне.");return;}
 std::wstring error;characterstate::Snapshot snapshot;
 if(!characterstate::Read(g_process,g_base,g_thread,snapshot,error)){CharacterFill(w);SetWindowTextW(w.name,L"Снимок персонажа недоступен");CharacterSetStatus(w,error);return;}
 w.snapshot=std::move(snapshot);w.valid=true;SetWindowTextW(w.name,(L"Персонаж: "+w.snapshot.name+L". Изменения требуют отдельного применения.").c_str());CharacterFill(w);CharacterSetStatus(w,L"Данные прочитаны. Игра и файлы сохранений не изменены.");
}
void CharacterApply(CharacterWindow& w){
 if(g_editorTest||!w.valid||w.staged.empty())return;
 if(g_busy||g_skillFrozen){CharacterSetStatus(w,L"Сначала остановите автоматику и разморозьте очки.");return;}
 std::vector<characterstate::Edit> edits;for(const auto& e:w.staged){const auto* f=CharacterField(e.first);std::wstring error;if(!f||!CharacterValidate(CharacterEffectiveField(w,*f),e.second,error)){CharacterSetStatus(w,error.empty()?L"Параметр больше недоступен.":error);return;}edits.push_back({e.first,e.second});}
 HANDLE writer=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,w.snapshot.pid);
 if(!writer){CharacterSetStatus(w,L"Не удалось получить доступ для записи. Изменения не применены.");return;}
 characterstate::Result result;bool okay=characterstate::Apply(writer,g_base,g_thread,w.snapshot,edits,result);CloseHandle(writer);
 if(!okay||!result.ok){std::wstring message=L"Не применено: "+result.error;if(result.rolledBack&&!result.rollbackFailed)message+=L" Предыдущие значения восстановлены.";if(result.rollbackFailed){message+=L" Не удалось полностью восстановить значения; обновите данные и проверьте персонажа.";w.valid=false;}CharacterSetStatus(w,message);Log(message);CharacterControls(w);if(result.nativeLimitsRejected)MessageBoxW(w.window,message.c_str(),L"Ограничения игры",MB_OK|MB_ICONINFORMATION);return;}
 w.staged.clear();std::wstring error;characterstate::Snapshot fresh;
 if(characterstate::Read(g_process,g_base,g_thread,fresh,error)){w.snapshot=std::move(fresh);w.valid=true;CharacterFill(w);CharacterSetStatus(w,L"Применено изменений: "+std::to_wstring(edits.size())+L". Значения повторно прочитаны из игры.");}
 else{w.valid=false;w.snapshot.values.clear();CharacterFill(w);CharacterSetStatus(w,L"Изменения применены; повторное чтение недоступно: "+error);}
 Log(L"Редактор персонажа: применено изменений "+std::to_wstring(edits.size())+L".");
}
HWND CharacterControl(CharacterWindow& w,const wchar_t* cls,const wchar_t* title,DWORD style,int id){HWND h=CreateWindowExW(!wcscmp(cls,L"EDIT")?WS_EX_CLIENTEDGE:0,cls,title,WS_CHILD|WS_VISIBLE|style,0,0,1,1,w.window,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);SendMessageW(h,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return h;}
void CharacterLayout(CharacterWindow& w){RECT r{};GetClientRect(w.window,&r);int x=r.right,y=r.bottom;auto move=[](HWND h,int a,int b,int c,int d){MoveWindow(h,a,b,std::max(c,1),std::max(d,1),TRUE);};
 move(w.name,12,10,x-24,24);move(w.group,12,40,215,200);move(w.search,239,40,x-251,26);move(w.list,12,78,x-24,y-335);move(w.help,12,y-245,x-24,84);
 move(GetDlgItem(w.window,1100),12,y-150,128,24);move(w.value,142,y-154,105,26);move(w.minus,255,y-155,34,28);move(w.plus,295,y-155,34,28);move(w.minimum,337,y-155,82,28);move(w.maximum,427,y-155,82,28);move(w.stage,517,y-155,112,28);move(w.resetField,637,y-155,150,28);
 move(w.status,12,y-114,x-24,34);move(w.apply,12,y-72,180,32);move(w.refresh,202,y-72,170,32);move(w.discard,382,y-72,190,32);move(w.close,x-132,y-72,120,32);move(GetDlgItem(w.window,1101),12,y-29,x-24,22);
}
LRESULT CALLBACK CharacterWindowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){auto* w=(CharacterWindow*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);if(msg==WM_NCCREATE){auto* c=(CREATESTRUCTW*)lp;w=(CharacterWindow*)c->lpCreateParams;w->window=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,(LONG_PTR)w);}if(!w)return DefWindowProcW(hwnd,msg,wp,lp);
 switch(msg){case WM_CREATE:{
  w->name=CharacterControl(*w,L"STATIC",L"",0,1110);w->group=CharacterControl(*w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,1111);w->search=CharacterControl(*w,L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,1112);SendMessageW(w->search,EM_SETCUEBANNER,TRUE,(LPARAM)L"Поиск по названию или описанию");
  SendMessageW(w->group,CB_ADDSTRING,0,(LPARAM)L"Все разделы");for(auto c:{characterstate::Category::Attributes,characterstate::Category::Personality,characterstate::Category::Body,characterstate::Category::Skills,characterstate::Category::States,characterstate::Category::Wounds})if(CharacterCategoryAllowed(w->kind,c)){auto i=SendMessageW(w->group,CB_ADDSTRING,0,(LPARAM)CharacterCategory(c));SendMessageW(w->group,CB_SETITEMDATA,i,(LPARAM)c);}SendMessageW(w->group,CB_SETCURSEL,0,0);
  w->list=CharacterControl(*w,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_TABSTOP,1113);ListView_SetExtendedListViewStyle(w->list,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);const wchar_t* titles[]={L"Параметр",L"Сейчас",L"Запланировано",L"Допустимые значения",L"Единицы"};int widths[]={225,100,125,170,100};for(int i=0;i<5;++i){LVCOLUMNW col{};col.mask=LVCF_TEXT|LVCF_WIDTH;col.pszText=const_cast<wchar_t*>(titles[i]);col.cx=widths[i];ListView_InsertColumn(w->list,i,&col);}
  w->help=CharacterControl(*w,L"STATIC",L"",0,1114);CharacterControl(*w,L"STATIC",L"Новое значение:",0,1100);w->value=CharacterControl(*w,L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,1115);SendMessageW(w->value,EM_SETLIMITTEXT,48,0);
  w->minus=CharacterControl(*w,L"BUTTON",L"−",WS_TABSTOP,1120);w->plus=CharacterControl(*w,L"BUTTON",L"+",WS_TABSTOP,1121);w->minimum=CharacterControl(*w,L"BUTTON",L"Минимум",WS_TABSTOP,1122);w->maximum=CharacterControl(*w,L"BUTTON",L"Максимум",WS_TABSTOP,1123);w->stage=CharacterControl(*w,L"BUTTON",L"В план",WS_TABSTOP,1124);w->resetField=CharacterControl(*w,L"BUTTON",L"Сбросить параметр",WS_TABSTOP,1125);
  w->status=CharacterControl(*w,L"STATIC",L"",0,1126);w->apply=CharacterControl(*w,L"BUTTON",L"Применить изменения",WS_TABSTOP,1130);w->refresh=CharacterControl(*w,L"BUTTON",L"Обновить данные",WS_TABSTOP,1131);w->discard=CharacterControl(*w,L"BUTTON",L"Сбросить весь план",WS_TABSTOP,1132);w->close=CharacterControl(*w,L"BUTTON",L"Закрыть",WS_TABSTOP,1133);CharacterControl(*w,L"STATIC",L"Кнопки меняют план. «Применить» проверяет весь набор значений и ограничения игры по стажу.",0,1101);SetTimer(hwnd,1,250,nullptr);CharacterLayout(*w);return 0;}
 case WM_SIZE:CharacterLayout(*w);return 0;case WM_GETMINMAXINFO:((MINMAXINFO*)lp)->ptMinTrackSize={850,600};return 0;
 case WM_TIMER:CharacterControls(*w);return 0;
 case WM_NOTIFY:if(((NMHDR*)lp)->hwndFrom==w->list&&((NMHDR*)lp)->code==LVN_ITEMCHANGED&&!w->filling){int i=ListView_GetNextItem(w->list,-1,LVNI_SELECTED);w->selected=i>=0&&i<(int)w->rows.size()?w->rows[i]:0;CharacterDetails(*w);}return 0;
 case WM_COMMAND:{int id=LOWORD(wp);if(w->filling)return 0;if((id==1112&&HIWORD(wp)==EN_CHANGE)||(id==1111&&HIWORD(wp)==CBN_SELCHANGE)){CharacterFill(*w);return 0;}
  if(id==1130)CharacterApply(*w);else if(id==1131)CharacterRefresh(*w);else if(id==1132){w->staged.clear();CharacterFill(*w);CharacterSetStatus(*w,L"План очищен. Значения игры не изменены.");}else if(id==1133)SendMessageW(hwnd,WM_CLOSE,0,0);else if(id==1125){w->staged.erase(w->selected);CharacterFill(*w);CharacterSetStatus(*w,L"Изменение выбранного параметра отменено.");}
  else if(id>=1120&&id<=1124){const auto* f=CharacterField(w->selected);if(!f)return 0;auto effective=CharacterEffectiveField(*w,*f);double n=0;if(id==1122&&effective.hasMinimum)n=effective.minimum;else if(id==1123&&effective.hasMaximum)n=effective.maximum;else{if(!CharacterParse(CharacterText(w->value),n)){CharacterSetStatus(*w,L"Введите число; допустимы точка или запятая.");return 0;}if(id==1120)n-=effective.step;if(id==1121)n+=effective.step;}CharacterStage(*w,n);}return 0;}
 case WM_CLOSE:if(!w->staged.empty()&&!g_editorAutoTest&&MessageBoxW(hwnd,L"Закрыть окно и сбросить неприменённые изменения?",L"Неприменённые изменения",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;DestroyWindow(hwnd);return 0;
 case WM_DESTROY:KillTimer(hwnd,1);w->window=nullptr;w->valid=false;w->staged.clear();w->snapshot={};return 0;
 }return DefWindowProcW(hwnd,msg,wp,lp);
}
void OpenCharacterWindow(unsigned kind){if(kind>=g_characterWindows.size())return;if(g_busy||g_skillFrozen){Log(L"Сначала остановите автоматику и разморозьте очки.");return;}auto& w=g_characterWindows[kind];if(w.window){if(!g_editorAutoTest){ShowWindow(w.window,SW_SHOW);SetForegroundWindow(w.window);}return;}w.kind=kind;WNDCLASSW c{};c.lpfnWndProc=CharacterWindowProc;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"URWTrainerCharacterWindow";c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&c);const wchar_t* titles[]={L"Персонаж: характеристики, личность и телосложение",L"Навыки персонажа",L"Состояние персонажа и раны"};CreateWindowExW(0,c.lpszClassName,titles[kind],WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,920,700,g_window,nullptr,c.hInstance,&w);if(w.window){CharacterRefresh(w);if(!g_editorAutoTest)ShowWindow(w.window,SW_SHOW);}}
int CharacterModelTests(int& checks){int failed=0;auto check=[&](bool okay){++checks;if(!okay)++failed;};double n=99;check(CharacterParse(L" 12,5 ",n)&&n==12.5);check(CharacterParse(L"-4",n)&&n==-4);for(const wchar_t* bad:{L"",L" ",L"NaN",L"inf",L"1e9999",L"2xyz",L"1,2,3",L"1 2"})check(!CharacterParse(bad,n));
 check(CharacterNumber(10)==L"10");check(CharacterNumber(1000)==L"1000");check(CharacterNumber(-0.0)==L"0");
 for(float original:{0.0f,10.0f,1000.0f,0.6666666865348816f,0.0000012345678f,99.999992f}){
  double decoded=0;check(CharacterParse(CharacterNumber(double(original)),decoded)&&float(decoded)==original);
  double stepped=double(original)+1;check(CharacterParse(CharacterNumber(stepped),decoded)&&float(decoded)==float(stepped));
 }
 double twice=0;check(CharacterParse(CharacterNumber(0.6666666865348816),twice));
 check(CharacterParse(CharacterNumber(twice+1),twice));check(CharacterParse(CharacterNumber(twice-1),twice)&&float(twice)==float(0.6666666865348816));
 characterstate::FieldInfo f{999,L"Test",L"",L"",characterstate::Category::Skills,characterstate::Storage::Integer,0,100,1,true,true,true};std::wstring e;check(CharacterValidate(f,0,e));check(CharacterValidate(f,100,e));check(!CharacterValidate(f,-1,e));check(!CharacterValidate(f,101,e));check(!CharacterValidate(f,1.25,e));f.storage=characterstate::Storage::Real;check(CharacterValidate(f,1.25,e));f.editable=false;check(!CharacterValidate(f,50,e));check(CharacterNumber(3.5)==L"3.5");check(CharacterNumber(0)==L"0");check(CharacterCategoryAllowed(0,characterstate::Category::Personality));check(!CharacterCategoryAllowed(0,characterstate::Category::States));check(CharacterCategoryAllowed(2,characterstate::Category::Wounds));
 CharacterWindow w;w.snapshot.values={{2,10},{3000,0},{3001,7}};characterstate::WoundInfo active{};active.index=0;active.type=1;active.healing=1;w.snapshot.wounds.push_back(active);characterstate::WoundInfo expired{};expired.index=1;expired.type=1;expired.penalty=7;w.snapshot.wounds.push_back(expired);
 auto wound=*CharacterField(3000);check(CharacterEffectiveField(w,wound).editable);check(CharacterEffectiveField(w,wound).maximum==0);check(CharacterIsHealingAction(w,3000,0));check(!CharacterIsHealingAction(w,3000,1));check(CharacterEffectiveField(w,*CharacterField(3001)).maximum==0);check(!CharacterEffectiveField(w,*CharacterField(3002)).editable);check(CharacterEffectiveField(w,*CharacterField(204)).maximum==40);w.staged[2]=12;check(CharacterEffectiveField(w,*CharacterField(204)).maximum==48);return failed;}
void CharacterWindowSelfTest(){int checks=0,failed=0;auto check=[&](bool x){++checks;if(!x)++failed;};
 for(unsigned kind=0;kind<3;++kind){OpenCharacterWindow(kind);auto& w=g_characterWindows[kind];check(w.window!=nullptr);check(w.valid);check(!IsWindowEnabled(w.apply));check(w.staged.empty());check(ListView_GetItemCount(w.list)==(int)w.rows.size());
  for(uint32_t id:w.rows){const auto* f=CharacterField(id);if(f&&f->editable){w.selected=id;CharacterDetails(w);auto* v=CharacterValue(w,id);if(v){double target=v->value+f->step;if(f->hasMaximum&&target>f->maximum)target=v->value-f->step;std::wstring err;if(CharacterValidate(*f,target,err)&&target!=v->value){CharacterStage(w,target);check(w.staged.count(id)==1);check(CharacterValue(w,id)->value==v->value);check(!IsWindowEnabled(w.apply));CharacterApply(w);check(w.staged.count(id)==1);CharacterStage(w,v->value);check(w.staged.empty());}}break;}}
  SendMessageW(w.window,WM_CLOSE,0,0);check(w.window==nullptr);
 }
 std::ofstream out("TRAINER_EDITOR_SELFTEST.json");out<<"{\"checks\":"<<checks<<",\"failures\":"<<failed<<",\"fixture_only\":true,\"game_write_handles_opened\":0,\"windows\":3}\n";PostMessageW(g_window,WM_CLOSE,0,0);
}
