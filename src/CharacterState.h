#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
namespace characterstate {
enum class Category { Attributes, Personality, Body, Skills, States, Wounds };
enum class Storage { Integer, Real };
struct FieldInfo { uint32_t id; const wchar_t* label; const wchar_t* help; const wchar_t* unit; Category category; Storage storage; double minimum,maximum,step; bool hasMinimum,hasMaximum,editable; };
struct Value { uint32_t id; double value; };
struct Edit { uint32_t id; double value; };
struct ByteCell { uint32_t va; std::vector<unsigned char> bytes; };
struct WoundInfo { unsigned index,type,severity,location,side,bleeding,healing,treatmentTick,penalty; std::wstring details; };
struct Snapshot { DWORD pid=0; uint64_t processCreationTime=0; uint64_t playerIdentity=0; std::wstring name; std::vector<Value> values; std::vector<ByteCell> memory; std::vector<WoundInfo> wounds; };
struct Result { bool ok=false,rolledBack=false,rollbackFailed=false,nativeLimitsRejected=false; std::wstring error; };
using HashFileFn=std::function<std::string(const std::wstring&)>;
using MainCheckFn=std::function<bool(HANDLE,uintptr_t,DWORD,std::wstring&)>;
void Configure(HashFileFn hash,MainCheckFn mainCheck);
const std::vector<FieldInfo>& Metadata();
bool Read(HANDLE process,uintptr_t base,DWORD gameThread,Snapshot& out,std::wstring& error);
bool Apply(HANDLE writer,uintptr_t base,DWORD gameThread,const Snapshot& expected,const std::vector<Edit>& edits,Result& out);
}
namespace characterstate {
namespace detail {
inline HashFileFn hashFile;
inline MainCheckFn mainCheck;
inline constexpr char kHash[]="c6ed73642c1b1cb1f86b818c7586224398c2f0554f6114ced31bc40ff071806d";
inline constexpr uint32_t kName=0xA72A6E8,kSkills=0xA72A992,kSkillNames=0xA72A708;
inline const std::array<const char*,30> skillEnglish={"DODGE","AGRICULTURE","SHIELD","KNIFE","SWORD","CLUB","AXE","SPEAR","BOW","CROSSBOW","UNARMED","PHYSICIAN","CLIMBING","STEALTH","COOKERY","SKIING","TIMBERCRAFT","SWIMMING","FISHING","NETMAKING","WEATHERLORE","CARPENTRY","HERBLORE","HIDEWORKING","TRACKING","TRAPPING","BUILDING","TEXTILECRAFT","CLOTHCRAFT","BOWYER"};
// Skill names occupy fixed 12-byte slots; TEXTILECRAFT uses all 12 bytes.
// A full-width name has no terminator inside its slot. Never inspect slot i+1.
inline bool ValidSkillNames(const std::vector<unsigned char>& names,unsigned* badSlot=nullptr){
 if(names.size()<12*skillEnglish.size()){if(badSlot)*badSlot=unsigned(skillEnglish.size());return false;}
 for(unsigned i=0;i<skillEnglish.size();++i){size_t len=strlen(skillEnglish[i]);const auto*name=names.data()+12*i;
 if(len>12||memcmp(name,skillEnglish[i],len)||(len<12&&name[len]!=0)){if(badSlot)*badSlot=i;return false;}}
 return true;
}
struct Binding { uint32_t id,va; unsigned size; bool floating; };
inline std::vector<Binding> Bindings(){
 std::vector<Binding> b={{1,0xA72B1AA,1,false},{2,0xA72B1B1,1,false},{3,0xA72B1AE,1,false},{4,0xA72B1AB,1,false},{5,0xA72B1AF,1,false},{6,0xA72B1B5,1,false},{7,0xA72B1BB,1,false},{8,0xA72B1B2,1,false},{9,0xA72B1B6,1,false},{10,0xA72B1BA,1,false},{11,0xA72B1B7,1,false},{20,0xA72B19C,4,false},{21,0xA72B198,4,false},{200,0xA72B188,4,true},{201,0xA72B17C,4,false},{202,0xA72BCE8,1,false},{204,0xA72B194,4,false},{205,0xA72B18C,4,false},{206,0xA72B1A8,1,false},{207,0xA72B0D4,4,false},{208,0xA72B0D8,4,false}};
 for(unsigned i=0;i<30;++i)b.push_back({100+i,kSkills+i,1,false});
 for(unsigned i=0;i<20;++i)b.push_back({3000+i,0xA72B144+i,1,false});
 return b;
}
inline bool RawRead(HANDLE p,uintptr_t base,uint32_t va,void* out,size_t size){SIZE_T n=0;return ReadProcessMemory(p,reinterpret_cast<void*>(base+va-0x400000),out,size,&n)&&n==size;}
inline const ByteCell* Cell(const Snapshot& s,uint32_t va){for(auto& c:s.memory)if(c.va==va)return &c;return nullptr;}
inline uint32_t Word(const Snapshot& s,uint32_t va){auto*c=Cell(s,va);uint32_t v=0;if(c&&c->bytes.size()<=4)memcpy(&v,c->bytes.data(),c->bytes.size());return v;}
// Exact clean3.89.1 sub_56DCE0 predicate. The game's "checksum failure"
// message is triggered by combinations of skill/attribute levels and days.
struct NativeAcceptance { unsigned namedSkills=0,highSkills=0;uint32_t days=0;bool allAttributesMax=false;unsigned reasons=0; };
inline unsigned NativeRejection(unsigned named,unsigned high,bool allMax,uint32_t days){
 return ((high>named/2&&days<30)?1u:0u)|((high==named&&days<100)?2u:0u)|((allMax&&days<30)?4u:0u);
}
inline bool EvaluateAcceptance(const Snapshot& s,const std::map<uint32_t,std::vector<unsigned char>>& changes,NativeAcceptance& out,std::wstring& error){
 out={};auto read=[&](uint32_t va,unsigned size,uint32_t& value){auto it=changes.find(va);const auto* cell=Cell(s,va);const auto* bytes=it!=changes.end()?&it->second:(cell?&cell->bytes:nullptr);if(!bytes||bytes->size()!=size)return false;value=0;memcpy(&value,bytes->data(),size);return true;};
 const auto* names=Cell(s,kSkillNames);if(!names||names->bytes.size()!=600||!read(0xA72B1C4,4,out.days)){error=L"Не удалось проверить игровые ограничения характеристик и навыков. Обновите данные.";return false;}
 // Native code tests strlen(name)>0: its truth value is exactly first byte!=0.
 // All 50 native slots participate, including named slots outside the 30 UI rows.
 for(unsigned i=0;i<50;++i){uint32_t level=0;if(!read(kSkills+i,1,level)){error=L"Не удалось прочитать все 50 игровых ячеек навыков. Обновите данные.";return false;}if(names->bytes[12*i]){++out.namedSkills;if(level>=95)++out.highSkills;}}
 out.allAttributesMax=true;for(uint32_t va:{0xA72B1AEu,0xA72B1AFu,0xA72B1B1u,0xA72B1B2u,0xA72B1B5u,0xA72B1B6u,0xA72B1B7u,0xA72B1BAu,0xA72B1BBu,0xA72B1AAu,0xA72B1ABu}){uint32_t value=0;if(!read(va,1,value)){error=L"Не удалось проверить все характеристики персонажа. Обновите данные.";return false;}out.allAttributesMax=out.allAttributesMax&&value>=18;}
 out.reasons=NativeRejection(out.namedSkills,out.highSkills,out.allAttributesMax,out.days);return true;
}
inline bool CheckAcceptance(const Snapshot& s,const std::map<uint32_t,std::vector<unsigned char>>& changes,std::wstring& error,bool* rejected=nullptr){
 if(rejected)*rejected=false;NativeAcceptance x;if(!EvaluateAcceptance(s,changes,x,error))return false;if(!x.reasons)return true;if(rejected)*rejected=true;
 error=L"План не применён: встроенная проверка игры отклонит сочетание значений. Игровой стаж: "+std::to_wstring(x.days)+L" дн.; навыков от 95%: "+std::to_wstring(x.highSkills)+L" из "+std::to_wstring(x.namedSkills)+L".";
 if(x.reasons&1)error+=L" До 30 дней нельзя иметь больше половины навыков от 95%.";
 if(x.reasons&2)error+=L" До 100 дней нельзя иметь все навыки от 95%.";
 if(x.reasons&4)error+=L" До 30 дней нельзя иметь все 11 характеристик от 18.";
 error+=L" Уменьшите значения в плане. Либо отдельно используйте кнопку стажа 100 дней в главном окне и обновите данные редактора. Стаж автоматически не меняется.";return false;
}
inline double Number(const Snapshot& s,uint32_t id){for(auto&v:s.values)if(v.id==id)return v.value;return NAN;}
inline uint64_t Fingerprint(const unsigned char* p,size_t n,uint64_t h=1469598103934665603ULL){for(size_t i=0;i<n;++i)h=(h^p[i])*1099511628211ULL;return h;}
inline bool VerifyProcess(HANDLE p,uintptr_t base,DWORD tid,uint64_t& creation,std::wstring& error){
 if(!hashFile||!mainCheck||!p||!base||!tid){error=L"Редактор не подключён к проверенной игре.";return false;}
 DWORD exit=0;if(!GetExitCodeProcess(p,&exit)||exit!=STILL_ACTIVE){error=L"Игра уже завершилась.";return false;}
 FILETIME ct,et,kt,ut;if(!GetProcessTimes(p,&ct,&et,&kt,&ut)){error=L"Не удалось проверить экземпляр процесса.";return false;}creation=(uint64_t(ct.dwHighDateTime)<<32)|ct.dwLowDateTime;
 wchar_t path[32768];DWORD length=32768;if(!QueryFullProcessImageNameW(p,0,path,&length)||hashFile(std::wstring(path,length))!=kHash){error=L"Нужен чистый EXE UnReal World 3.89.1.";return false;}
 unsigned char mz[2];if(!RawRead(p,base,0x400000,mz,2)||mz[0]!='M'||mz[1]!='Z'){error=L"Базовый адрес процесса не подтверждён.";return false;}
 // Independent, non-entry instruction operands; hooked prologues are irrelevant.
 struct Anchor{uint32_t va;std::vector<unsigned char> bytes;};
 const Anchor checks[]={ {0x4E6813,{0x0f,0xb6,0x05,0xaa,0xb1,0x72,0x0a}}, {0x4E6905,{0x0f,0xb6,0x05,0xba,0xb1,0x72,0x0a}}, {0x4E699A,{0x66,0x0f,0x6e,0x05,0x9c,0xb1,0x72,0x0a}} };
 for(auto&a:checks){std::vector<unsigned char> expected=a.bytes;uint32_t operand=0;memcpy(&operand,expected.data()+expected.size()-4,4);operand+=uint32_t(base-0x400000);memcpy(expected.data()+expected.size()-4,&operand,4);std::vector<unsigned char> got(a.bytes.size());if(!RawRead(p,base,a.va,got.data(),got.size())||got!=expected){error=L"Операнды полей персонажа не соответствуют проверенной версии.";return false;}}
 HANDLE t=OpenThread(THREAD_QUERY_INFORMATION,FALSE,tid);bool okay=t&&GetProcessIdOfThread(t)==GetProcessId(p);if(t)CloseHandle(t);if(!okay){error=L"Поток не принадлежит выбранной игре.";return false;}return true;
}
struct StoppedThread { HANDLE handle=nullptr;bool stopped=false;StoppedThread(DWORD id,DWORD expectedPid){if(!expectedPid)return;handle=OpenThread(THREAD_SUSPEND_RESUME|THREAD_QUERY_INFORMATION,FALSE,id);if(handle&&GetProcessIdOfThread(handle)==expectedPid){DWORD previous=SuspendThread(handle);if(previous!=DWORD(-1)){stopped=previous==0;if(!stopped)ResumeThread(handle);}}}~StoppedThread(){if(stopped)ResumeThread(handle);if(handle)CloseHandle(handle);} };
inline bool SnapshotStopped(HANDLE p,uintptr_t base,DWORD tid,uint64_t creation,Snapshot& s,std::wstring& error){
 if(!mainCheck(p,base,tid,error))return false;s={};s.pid=GetProcessId(p);s.processCreationTime=creation;
 std::map<uint32_t,unsigned> ranges;
 for(auto&b:Bindings())ranges[b.va]=b.size;
 for(unsigned i=0;i<50;++i)ranges[kSkills+i]=1;
 ranges[kName]=12;ranges[kSkillNames]=600;ranges[0xA72B1A9]=1;ranges[0xA72B1B9]=1;ranges[0xA72B1C4]=4;ranges[0xA72B8F4]=1;
 for(unsigned i=0;i<20;++i){for(uint32_t a:{0xA72B130,0xA72B0E0,0xA72B11C,0xA72B0F4,0xA72B108,0xA72B15C})ranges[a+i]=1;ranges[0xA72B904+4*i]=4;}
 for(auto&r:ranges){ByteCell c{r.first,std::vector<unsigned char>(r.second)};if(!RawRead(p,base,c.va,c.bytes.data(),c.bytes.size())){error=L"Не удалось прочитать запись персонажа.";return false;}s.memory.push_back(c);}
 auto*n=Cell(s,kName);auto end=std::find(n->bytes.begin(),n->bytes.end(),0);if(end==n->bytes.begin()||end==n->bytes.end()||Word(s,0xA72B1C4)<1||Word(s,0xA72B1A9)<1||Word(s,0xA72B1A9)>2){error=L"Загруженный персонаж игрока не подтверждён.";return false;}
 for(auto it=n->bytes.begin();it!=end;++it)s.name.push_back(wchar_t(*it));
 auto*names=Cell(s,kSkillNames);
 unsigned badSkill=0;if(!ValidSkillNames(names->bytes,&badSkill)){error=L"Порядок навыков персонажа не совпадает с проверенной игрой (ячейка "+std::to_wstring(badSkill+1)+L").";return false;}
 s.playerIdentity=Fingerprint(n->bytes.data(),n->bytes.size());s.playerIdentity=Fingerprint(names->bytes.data(),names->bytes.size(),s.playerIdentity);for(auto a:{0xA72B1A9,0xA72B1B9})s.playerIdentity=Fingerprint(Cell(s,a)->bytes.data(),1,s.playerIdentity);
 for(auto&b:Bindings()){uint32_t raw=Word(s,b.va);double value;if(b.floating){float f;memcpy(&f,&raw,4);value=f;}else value=b.size==1?double(raw):double(int32_t(raw));if(!std::isfinite(value)){error=L"Поле персонажа содержит недопустимое значение.";return false;}s.values.push_back({b.id,value});}
 double energy=std::max(500.0,Number(s,201)),stomach=Number(s,202);double factor=std::min(1.5,1600.0/energy);double internal=std::min(255.0,std::ceil(factor*(255-stomach)));s.values.push_back({203,std::min(100.0,std::ceil(std::max(0.0,internal-30)/225*100))});
 for(unsigned i=0;i<20;++i)s.wounds.push_back({i,Word(s,0xA72B130+i),Word(s,0xA72B0E0+i),Word(s,0xA72B11C+i),Word(s,0xA72B0F4+i),Word(s,0xA72B108+i),Word(s,0xA72B15C+i),Word(s,0xA72B904+4*i),Word(s,0xA72B144+i),L""});
 for(auto&w:s.wounds)w.details=w.type?(L"Код типа: "+std::to_wstring(w.type)+L"; тяжесть: "+std::to_wstring(w.severity)+L"; код части тела: "+std::to_wstring(w.location)+L"; код стороны: "+std::to_wstring(w.side)+L"; кровотечение: "+std::to_wstring(w.bleeding)+L"; состояние заживления: "+std::to_wstring(w.healing)):L"Пустая ячейка: активного ранения или недуга нет.";
 return true;
}
struct Write { uint32_t va;std::vector<unsigned char> before,after; };
using ReadBytesFn=std::function<bool(uint32_t,void*,size_t)>;
using WriteBytesFn=std::function<bool(uint32_t,const void*,size_t)>;
inline bool Commit(const std::vector<Write>& writes,const ReadBytesFn& read,const WriteBytesFn& write,Result& result){
 result={};if(writes.empty()){result.ok=true;return true;}
 for(auto&w:writes){std::vector<unsigned char> now(w.before.size());if(!read(w.va,now.data(),now.size())||now!=w.before){result.error=L"Значения изменились. Обновите окно и повторите.";return false;}}
 size_t maximum=0;for(auto&w:writes)maximum=(std::max)(maximum,w.after.size());
 std::vector<unsigned char> scratch(maximum); // Allocate before the first write.
 size_t attempted=0;bool failed=false;
 try {for(auto&w:writes){++attempted;if(!write(w.va,w.after.data(),w.after.size())){failed=true;break;}if(!read(w.va,scratch.data(),w.after.size())||memcmp(scratch.data(),w.after.data(),w.after.size())){failed=true;break;}}}catch(...){failed=true;}
 if(!failed){result.ok=true;return true;}
 result.rolledBack=true;
 while(attempted){const auto&w=writes[--attempted];bool okay=false;try{okay=write(w.va,w.before.data(),w.before.size());okay=read(w.va,scratch.data(),w.before.size())&&!memcmp(scratch.data(),w.before.data(),w.before.size())&&okay;}catch(...){okay=false;}if(!okay)result.rollbackFailed=true;}
 result.error=result.rollbackFailed?L"Ошибка записи; полный откат не подтверждён. Не сохраняйте игру.":L"Ошибка записи. Исходные значения восстановлены.";return false;
}
}
inline void Configure(HashFileFn h,MainCheckFn m){detail::hashFile=std::move(h);detail::mainCheck=std::move(m);}
inline const std::vector<FieldInfo>& Metadata(){
 static const std::vector<FieldInfo> fields=[](){std::vector<FieldInfo> f;
 const wchar_t* attrs[]={L"Сила",L"Выносливость",L"Ловкость рук",L"Подвижность",L"Скорость",L"Зрение",L"Слух",L"Обоняние и вкус",L"Осязание",L"Интеллект",L"Воля"};
 const wchar_t* effects[]={
  L"Физическая сила: участвует в метании и расчёте основы боевых навыков.",
  L"Выносливость: участвует в расчёте утомления и последствий кровопотери.",
  L"Ловкость рук: участвует в проверках неловких действий с предметами и в метании.",
  L"Подвижность: участвует в проверках потери равновесия и удержания при лазании.",
  L"Определяет базовый темп ходьбы и бега. Фактический темп также зависит от состояния, груза и местности.",
  L"Зрение: участвует в обнаружении видимых объектов и в метании.",
  L"Слух: участвует в проверке, услышит ли персонаж шум с учётом его источника и расстояния.",
  L"Обоняние и вкус: помогают замечать готовность и подгорание еды; входят в основу травничества и выслеживания.",
  L"Осязание: входит в основу навыков ножа, арбалета, рыбалки и травничества.",
  L"Интеллект: входит в основу знаний и ремёсел, например земледелия, травничества и плетения сетей.",
  L"Воля: участвует в проверке удержания от рвоты и входит в основу рыбалки и земледелия."};
 static std::array<std::wstring,11> attrHelp;
 for(unsigned i=0;i<11;++i){attrHelp[i]=std::wstring(effects[i])+L" Диапазон редактора 1–18; 18 — верхнее значение при создании. Текущие навыки и склонности не пересчитываются. До 30 игровых дней нельзя одновременно установить все 11 характеристик на 18: применяется общая проверка игры.";f.push_back({i+1,attrs[i],attrHelp[i].c_str(),L"",i<9?Category::Attributes:Category::Personality,Storage::Integer,1,18,1,true,true,true});}
 f.push_back({20,L"Рост",L"Рост в карточке персонажа, в дюймах. Масса тела и навыки задаются отдельно. Диапазон 36–96 выбран для редактора, это не предел игры.",L"дюйм",Category::Body,Storage::Integer,36,96,1,true,true,true});
 f.push_back({21,L"Масса тела",L"Масса тела без груза, в фунтах. Учитывается при расчёте нагрузки и допустимого переносимого веса. Диапазон 50–500 выбран для редактора, это не предел игры.",L"фунт",Category::Body,Storage::Integer,50,500,1,true,true,true});
 const wchar_t* skills[]={L"Уклонение",L"Земледелие",L"Щит",L"Нож",L"Меч",L"Дубина",L"Топор",L"Копьё",L"Лук",L"Арбалет",L"Рукопашный бой",L"Врачевание",L"Лазание",L"Скрытность",L"Кулинария",L"Лыжи",L"Работа с деревом",L"Плавание",L"Рыбная ловля",L"Плетение сетей",L"Погодоведение",L"Плотничество",L"Травничество",L"Выделка шкур",L"Выслеживание",L"Ловушки",L"Строительство",L"Работа с волокнами",L"Изготовление одежды",L"Изготовление луков"};
 for(unsigned i=0;i<30;++i)f.push_back({100+i,skills[i],L"Текущее мастерство навыка. Диапазон редактора 0–100%; исходное мастерство, склонность и отметка улучшения не меняются. До 30 игровых дней нельзя иметь больше половины навыков от 95%, до 100 дней — все навыки от 95%. Проверяется весь план вместе с остальными значениями.",L"%",Category::Skills,Storage::Integer,0,100,1,true,true,true});
 auto state=[&](uint32_t id,const wchar_t*label,const wchar_t*help,const wchar_t*unit,Storage type,double min,double max,double step,bool editable=true){f.push_back({id,label,help,unit,Category::States,type,min,max,step,true,true,editable});};
 state(200,L"Утомление",L"Накопленное утомление. Игровое ограничение — 0–100. Ноль означает отдых.",L"%",Storage::Real,0,100,1);
 state(201,L"Запас энергии",L"Питательный резерв. Игра ограничивает накопление сверху 5000. Голод также зависит от наполненности желудка.",L"ед.",Storage::Integer,0,5000,100);
 state(202,L"Наполненность желудка",L"Уровень наполненности: 0 — пусто, 255 — полно. Это не процент голода.",L"0–255",Storage::Integer,0,255,5);
 state(203,L"Голод на экране",L"Вычисляется игрой из запаса энергии и наполненности желудка. Меняйте составляющие выше.",L"%",Storage::Integer,0,100,1,false);
 state(204,L"Кровопотеря",L"Показатель кровопотери; предел редактора при применении — 4 × выносливость. Уменьшение не останавливает кровотечение из ран.",L"ед.",Storage::Integer,0,72,1);
 state(205,L"Штраф от ранений",L"Суммарное последствие активных записей ран. Для лечения выберите конкретную рану; общий счётчик отдельно не редактируется.",L"ед.",Storage::Integer,0,5100,1,false);
 state(206,L"Истощение от голода",L"Накопленный штраф голодания. Редактор допускает 0–99; 100 и выше связаны с гибелью. Сам запас пищи не восстанавливает.",L"ед.",Storage::Integer,0,99,1);
 state(207,L"Жажда",L"Внутренний счётчик: 0 — нет жажды. От 150 начинается экранный индикатор, 3000 соответствует 100%. Диапазон редактора 0–3000.",L"ед.",Storage::Integer,0,3000,100);
 state(208,L"Потребность во сне",L"Счётчик бодрствования; менее 500 — бодрость, 2000 — крайняя усталость. Диапазон 0–2000 выбран редактором.",L"ед.",Storage::Integer,0,2000,100);
 static std::array<std::wstring,20> wounds;for(unsigned i=0;i<20;++i){wounds[i]=L"Рана/недуг "+std::to_wstring(i+1)+L": штраф";f.push_back({3000+i,wounds[i].c_str(),L"Штраф можно только уменьшить. Значение 0 лечит выбранную рану и сбрасывает связанные с ней последствия, кровотечение и срок лечения. Не создаёт новые раны.",L"ед.",Category::Wounds,Storage::Integer,0,255,1,true,true,true});}
 return f;}();return fields;
}
inline bool Read(HANDLE process,uintptr_t base,DWORD thread,Snapshot& out,std::wstring& error){
 uint64_t creation=0;if(!detail::VerifyProcess(process,base,thread,creation,error))return false;detail::StoppedThread stop(thread,GetProcessId(process));if(!stop.stopped){error=L"Не удалось безопасно остановить основной поток игры.";return false;}return detail::SnapshotStopped(process,base,thread,creation,out,error);
}
namespace detail {
inline bool SameSnapshot(const Snapshot&a,const Snapshot&b){
 if(a.pid!=b.pid||a.processCreationTime!=b.processCreationTime||a.playerIdentity!=b.playerIdentity||a.memory.size()!=b.memory.size())return false;
 for(size_t i=0;i<a.memory.size();++i)if(a.memory[i].va!=b.memory[i].va||a.memory[i].bytes!=b.memory[i].bytes)return false;return true;
}
inline bool Plan(const Snapshot& current,const std::vector<Edit>& edits,std::vector<Write>& writes,std::wstring& error,bool* nativeRejected=nullptr){
 if(nativeRejected)*nativeRejected=false;writes.clear();std::set<uint32_t> ids;std::map<uint32_t,std::vector<unsigned char>> changes;bool woundEdit=false;
 auto set=[&](uint32_t va,uint32_t value,unsigned size){auto*c=Cell(current,va);if(!c||c->bytes.size()!=size)return false;std::vector<unsigned char> data(size);memcpy(data.data(),&value,size);changes[va]=data;return true;};
 double endurance=Number(current,2);for(auto&e:edits)if(e.id==2)endurance=e.value;
 int penalty=int(Number(current,205)),blood=int(Number(current,204));int woundPenalty=0,bleeding=0;
 for(auto&e:edits){
  if(!ids.insert(e.id).second){error=L"Один параметр указан дважды.";return false;}
  auto it=std::find_if(Metadata().begin(),Metadata().end(),[&](const FieldInfo&f){return f.id==e.id;});
  if(it==Metadata().end()||!it->editable||!std::isfinite(e.value)||(it->hasMinimum&&e.value<it->minimum)||(it->hasMaximum&&e.value>it->maximum)||(it->storage==Storage::Integer&&e.value!=std::floor(e.value))){error=L"Недопустимое значение или параметр только для чтения.";return false;}
  if(e.id>=3000&&e.id<3020){
   unsigned i=e.id-3000;if(i>=current.wounds.size()){error=L"Запись раны не подтверждена.";return false;}const auto&w=current.wounds[i];if(!w.type){error=L"В выбранной ячейке нет активной раны или недуга.";return false;}
   if(e.value>w.penalty){error=L"Для раны разрешено только уменьшение штрафа или лечение до 0.";return false;}
   if(e.value>0&&!w.healing){error=L"Срок заживления этой записи истёк; разрешено только полное лечение до 0.";return false;}
   woundEdit=true;unsigned reduction=w.penalty-unsigned(e.value);woundPenalty+=int(reduction);
   if(!set(0xA72B144+i,unsigned(e.value),1)){error=L"Поле раны не подтверждено.";return false;}
   if(e.value==0||w.healing==0){
    // Same record side effects as native3.89.1 sub4D7CB0 after its penalty reduction.
    for(uint32_t va:{0xA72B130+i,0xA72B15C+i,0xA72B108+i,0xA72B144+i})if(!set(va,0,1))return false;
    if(!set(0xA72B904+4*i,0,4))return false;
    if(w.type==25&&!set(0xA72B8F4,0,1))return false;
    bleeding+=int(w.bleeding);
   }
   continue;
  }
  if(e.id==204&&e.value>4*endurance){error=L"Кровопотеря должна быть не выше 4×выносливость.";return false;}
  auto bindings=Bindings();auto b=std::find_if(bindings.begin(),bindings.end(),[&](const Binding& b){return b.id==e.id;});if(b==bindings.end()){error=L"Адрес поля не подтверждён.";return false;}
  uint32_t value=uint32_t(e.value);if(b->floating){float f=float(e.value);memcpy(&value,&f,4);}if(!set(b->va,value,b->size)){error=L"Размер поля не подтверждён.";return false;}
 }
 if(woundEdit){
  //026: explicit bloodloss reset is allowed only with complete healing of
  // every active record, including ailments with an already-zero penalty.
  bool resetBlood=false;
  if(ids.count(204)){
   auto bloodEdit=std::find_if(edits.begin(),edits.end(),[](const Edit&e){return e.id==204;});
   bool allHealed=bloodEdit!=edits.end()&&bloodEdit->value==0;
   for(const auto&w:current.wounds)if(w.type){
    auto heal=std::find_if(edits.begin(),edits.end(),[&](const Edit&e){return e.id==3000+w.index;});
    allHealed=allHealed&&heal!=edits.end()&&heal->value==0;
   }
   if(!allHealed){error=L"Сброс кровопотери вместе с лечением разрешён только при полном лечении всех активных ран и недугов.";return false;}
   resetBlood=true;
  }
  unsigned sum=0;for(auto&w:current.wounds)if(w.type)sum+=w.penalty;
  if(penalty<0||unsigned(penalty)!=sum||penalty<woundPenalty||blood<bleeding){error=L"Связанные счётчики ран не согласованы; лечение не выполнено.";return false;}
  if(!set(0xA72B18C,uint32_t(penalty-woundPenalty),4)||!set(0xA72B194,resetBlood?0u:uint32_t(blood-bleeding),4))return false;
 }
 if(!CheckAcceptance(current,changes,error,nativeRejected))return false;
 for(auto&kv:changes){auto*c=Cell(current,kv.first);if(c&&c->bytes!=kv.second)writes.push_back({kv.first,c->bytes,kv.second});}
 return true;
}
}
inline bool Apply(HANDLE writer,uintptr_t base,DWORD thread,const Snapshot& expected,const std::vector<Edit>& edits,Result& result){
 result={};uint64_t creation=0;if(!detail::VerifyProcess(writer,base,thread,creation,result.error))return false;
 detail::StoppedThread stop(thread,GetProcessId(writer));if(!stop.stopped){result.error=L"Не удалось безопасно остановить основной поток игры.";return false;}
 Snapshot current;if(!detail::SnapshotStopped(writer,base,thread,creation,current,result.error))return false;
 if(!detail::SameSnapshot(current,expected)){result.error=L"Персонаж или значения изменились. Обновите окно и повторите.";return false;}
 std::vector<detail::Write> writes;if(!detail::Plan(current,edits,writes,result.error,&result.nativeLimitsRejected))return false;
 for(auto&w:writes){MEMORY_BASIC_INFORMATION mi{};uintptr_t address=base+w.va-0x400000;if(!VirtualQueryEx(writer,(void*)address,&mi,sizeof(mi))||mi.State!=MEM_COMMIT||(mi.Protect&(PAGE_GUARD|PAGE_NOACCESS))||!(mi.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))||address+w.after.size()>(uintptr_t)mi.BaseAddress+mi.RegionSize){result.error=L"Целевая область персонажа недоступна для обычной записи.";return false;}}
 auto read=[&](uint32_t va,void*out,size_t n){return detail::RawRead(writer,base,va,out,n);};
 auto write=[&](uint32_t va,const void*data,size_t n){SIZE_T used=0;return WriteProcessMemory(writer,(void*)(base+va-0x400000),data,n,&used)&&used==n;};
 return detail::Commit(writes,read,write,result);
}
}
