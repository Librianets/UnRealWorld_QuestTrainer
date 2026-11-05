#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
namespace animalspawn {
struct Species {uint32_t id;const wchar_t* ru;const char* en;uint32_t dispatch,constructor;};
inline const std::array<Species,39>& SpeciesList(){static const std::array<Species,39> data={{
 {600,L"Заяц","Hare",0x004CEE14,0x004C8760},
 {601,L"Лиса","Fox",0x004CEE1E,0x004C89C0},
 {602,L"Росомаха","Glutton (wolverine)",0x004CEE28,0x004C8D90},
 {603,L"Песец","Arctic fox",0x004CEE32,0x004C8C20},
 {604,L"Белка","Squirrel",0x004CEE3C,0x004CB4D0},
 {605,L"Медведь","Bear",0x004CEE46,0x004C90C0},
 {608,L"Рысь","Lynx",0x004CEE5A,0x004CB630},
 {609,L"Корова / бык","Cow / bull",0x004CEE64,0x004CB390},
 {611,L"Волк","Wolf",0x004CEE6E,0x004CB900},
 {612,L"Горностай","Ermine",0x004CEE78,0x004CAD60},
 {613,L"Кабан","Wild boar",0x004CEE82,0x004CA8F0},
 {614,L"Дикая свинья","Wild sow",0x004CEE8C,0x004CAC30},
 {615,L"Свинья","Pig",0x004CEE96,0x004CB090},
 {616,L"Овца / баран","Sheep / ram",0x004CEEA0,0x004CAA90},
 {618,L"Собака","Dog",0x004CEEAA,0x004CBBD0},
 {619,L"Северный олень","Reindeer",0x004CEEB4,0x004CBE70},
 {620,L"Лесной северный олень","Forest reindeer",0x004CEEBE,0x004CC000},
 {621,L"Лошадь","Horse",0x004CEEC8,0x004CC1C0},
 {622,L"Змея","Snake",0x004CEED2,0x004CC360},
 {623,L"Бобр","Beaver",0x004CEEDC,0x004CDD40},
 {624,L"Барсук","Badger",0x004CEEE6,0x004CE610},
 {625,L"Ласка","Weasel",0x004CEEF0,0x004CAE20},
 {626,L"Лесная куница","Pine-marten",0x004CEEFA,0x004CAED0},
 {627,L"Хорёк","Polecat",0x004CEF01,0x004CAFD0},
 {628,L"Тетерев","Black grouse",0x004CEF08,0x004CC5D0},
 {629,L"Лебедь","Swan",0x004CEF24,0x004CD440},
 {630,L"Кряква","Mallard",0x004CEF2B,0x004CD670},
 {631,L"Хохлатая чернеть","Tufted duck",0x004CEF40,0x004CD800},
 {632,L"Глухарь","Capercaillie",0x004CEF47,0x004CCFC0},
 {633,L"Рябчик","Hazel grouse",0x004CEF0F,0x004CC850},
 {634,L"Ворон","Raven",0x004CEF16,0x004CCB00},
 {635,L"Белая куропатка","Willow grouse",0x004CEF1D,0x004CCD60},
 {636,L"Гоголь","Goldeneye",0x004CEF32,0x004CD6F0},
 {637,L"Ястреб-тетеревятник","Goshawk",0x004CEF39,0x004CD890},
 {638,L"Филин","Eagle owl",0x004CEF4E,0x004CDAE0},
 {639,L"Гагара","Kuikka (diver)",0x004CEF63,0x004CD770},
 {650,L"Серый тюлень","Grey seal",0x004CEF55,0x004CDFF0},
 {651,L"Кольчатая нерпа","Ringed seal",0x004CEF5C,0x004CE280},
 {660,L"Лось","Elk",0x004CEE50,0x004CB1B0},
}};return data;}
inline const Species* Find(uint32_t id){for(const auto& v:SpeciesList())if(v.id==id)return &v;return nullptr;}
struct World {uint16_t x=0,y=0,count=0;uint8_t type=0;uint32_t ref=0;};
struct Snapshot {std::string map,player;uint32_t x=0,y=0,tx=0,ty=0;bool terrainSafe=false,malformedActive=false;
 std::array<uint32_t,500> ids{};std::vector<World> world;
};
// Unknown input after opening Target is potentially past its commit boundary.
inline bool CriticalBoundary(bool active,bool committed,int phase,bool main,bool targetSame){return committed||(active&&phase>=305&&!main&&!targetSame);}
inline bool Terrain(uint8_t id,uint32_t flags){return (id==58||id==250)&&flags==2;}
inline bool Empty(const World& w){return w.type==0||w.type==33;}
inline bool SamePlace(const Snapshot& a,const Snapshot& b){return a.map==b.map&&a.player==b.player&&a.x==b.x&&a.y==b.y;}
inline const wchar_t* Validate(const Snapshot& s,uint32_t species,bool target){
 if(!Find(species))return L"Неизвестный вид животного.";
 if(s.player.empty()||s.map.empty()||s.map=="BIGMAP"||s.x>3071||s.y>2047)return L"Нужен живой персонаж на локальной карте.";
 if(s.malformedActive)return L"В активном списке есть пустая запись существа. Восстановите резервную копию персонажа.";
 for(size_t i=0;i<s.ids.size();++i)if(s.ids[i])for(size_t j=0;j<i;++j)if(s.ids[j]==s.ids[i])return L"Найдены повторяющиеся ID существ. Восстановите резервную копию персонажа.";
 bool free=false;for(auto id:s.ids)if(!id)free=true;
 if(!free)return L"Все 500 мест животных заняты. Создание запрещено.";
 if(s.world.size()!=9951)return L"Таблица объектов не прочитана полностью.";
 free=false;for(const auto& w:s.world)if(Empty(w))free=true;
 if(!free)return L"Все 9951 мест объектов заняты. Создание запрещено.";
 if(target){
  if(s.tx>3071||s.ty>2047)return L"Выбранная клетка вне локальной карты.";
  int dx=int(s.tx)-int(s.x),dy=int(s.ty)-int(s.y);
  if(dx<-1||dx>1||dy<-1||dy>1||(!dx&&!dy))return L"Выберите соседнюю клетку, не клетку персонажа.";
  if(!s.terrainSafe)return L"Выберите свободную клетку земли рядом: Ground / Surface of the Earth. Вода, постройки и другие покрытия пока не поддерживаются.";
  for(const auto& w:s.world)if(!Empty(w)&&w.x==s.tx&&w.y==s.ty)return L"Выбранная клетка занята объектом. Создание запрещено.";
 }
 return nullptr;
}
inline int AddedSlot(const Snapshot& before,const Snapshot& after){
 if(!SamePlace(before,after))return -1;
 int found=-1;
 for(size_t i=0;i<before.ids.size();++i){if(before.ids[i]==after.ids[i])continue;
  if(before.ids[i]||after.ids[i]<1000||found!=-1)return -1;
  for(auto old:before.ids)if(old&&old==after.ids[i])return -1;
  found=(int)i;
 }
 return found;
}
inline bool Readback(const Snapshot& a,const Snapshot& b,uint32_t species,uint32_t actualSpecies){
 int slot=AddedSlot(a,b);if(slot<0||!Find(species)||actualSpecies!=species||a.world.size()!=9951||b.world.size()!=9951)return false;
 int matches=0;for(size_t i=0;i<b.world.size();++i){const auto& w=b.world[i];
  if(w.type==4&&w.ref==uint32_t(slot+1000)){
   if(!Empty(a.world[i])||w.count!=1||w.x!=a.tx||w.y!=a.ty)return false;
   ++matches;
  }
 }
 return matches==1;
}
}
