#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
namespace itemcatalogue {
constexpr uint32_t kBaseCount=400,kDynamicFirst=50000,kDynamicCount=49900;
constexpr size_t kRecordSize=172,kNameOffset=9,kNameCapacity=40;
using Record=std::array<unsigned char,kRecordSize>;
inline bool ValidId(uint32_t id){return (id>0&&id<kBaseCount)||(id>=kDynamicFirst&&id<kDynamicFirst+kDynamicCount);}
inline uintptr_t Address(uint32_t id){return !ValidId(id)?0:id<kDynamicFirst?0x58F9FF8u+kRecordSize*id:0x59216A8u+kRecordSize*(id-kDynamicFirst);}
inline bool Category(unsigned type){switch(type){case 1:case 2:case 3:case 5:case 6:case 7:case 8:case 9:case 10:case 11:case 12:case 13:case 14:return true;default:return false;}}
// Both banks contain native172-byte records. Names occupy bytes9..48, not80bytes.
// Byte0==0 denotes an unused slot. Reject meta-prototypes and malformed records.
inline bool Decode(uint32_t id,const Record& r,std::string& name,unsigned& category){
 name.clear();category=0;if(!ValidId(id)||!Category(r[0]))return false;
 size_t n=0;for(;n<kNameCapacity&&r[kNameOffset+n];++n){auto c=r[kNameOffset+n];if(c<32||c==127||(c>=128&&c<160)||c=='*')return false;}
 if(!n||n==kNameCapacity)return false;
 name.assign(reinterpret_cast<const char*>(r.data()+kNameOffset),n);category=r[0];return true;
}
inline std::string Lower(std::string text){for(auto& c:text)if(c>='A'&&c<='Z')c+=32;return text;}
inline bool SameRecord(uint32_t id,const Record& expected,const Record& actual){std::string n;unsigned c=0;return expected==actual&&Decode(id,actual,n,c);}
// Native3.89.1 quality: byte165 low3 bits; zero means ordinary level3.
// Renderer451770 omits this property for plants, carcasses and special kinds55/69.
inline unsigned Quality(const Record& r){
 uint32_t kind=0;memcpy(&kind,r.data()+4,4);
 if(!Category(r[0])||r[0]==11||r[0]==12||kind==55||kind==69)return 0;
 unsigned q=r[165]&7;return q==0?3:q<=5?q:0;
}
inline const wchar_t* QualityLabel(unsigned q){switch(q){
 case 1:return L"1 — низкое";case 2:return L"2 — пониженное";case 3:return L"3 — обычное";
 case 4:return L"4 — высокое";case 5:return L"5 — высшее";default:return L"—";}}
// Pickup44C6DA..44C70C interns a base prototype via4C0AC0. Its ID can change.
// Do not use the item name as identity. Allow only identical bytes, except the
// native default quality encodings0/0x80/0x83, which comparator4C0230 equates.
inline bool PickupEquivalent(const Record& expected,const Record& actual){
 for(size_t i=0;i<kRecordSize;++i){
  if(expected[i]==actual[i])continue;
  auto ordinary=[](unsigned v){return v==0||v==0x80||v==0x83;};
  if(i!=165||!ordinary(expected[i])||!ordinary(actual[i]))return false;
 }return true;
}
inline bool PickupCommitted(int before,int after,uint64_t groundBefore,uint64_t groundAfter,int created,int requested){
 return before>=0&&after>=0&&requested>=1&&requested<=100&&created==requested&&
        uint64_t(after)==uint64_t(before)+unsigned(requested)&&groundAfter==groundBefore;
}
inline std::string FamilyRussian(const std::string& input){
 auto name=Lower(input);
 static const std::pair<const char*,const char*> direct[]={
 {"water","вода"},{"birch-bark strip","берестяная полоса"},{"ember-roasted turnip","печёная в углях репа"},
 {"grey hat","серая шляпа"},{"moss trousers","моховые штаны"},{"tree lichen overcoat","пальто из древесного лишайника"}};
 for(auto& p:direct)if(name==p.first)return p.second;
 static const std::pair<const char*,const char*> materials[]={{"linen","льняная"},{"hemp","конопляная"},{"nettle","крапивная"},{"wool","шерстяная"},{"woollen","шерстяная"},{"sinew","сухожильная"}};
 for(auto& p:materials){if(name==std::string(p.first)+" yarn")return std::string(p.second)+" нить";if(name==std::string(p.first)+" cloth")return std::string(p.second)+" ткань";}
 static const std::pair<const char*,const char*> animals[]={
 {"forest reindeer","лесного северного оленя"},{"reindeer","северного оленя"},{"elk","лося"},{"stag","оленя"},{"bear","медведя"},{"lynx","рыси"},{"wolf","волка"},{"fox","лисы"},{"arctic fox","песца"},{"squirrel","белки"},{"hare","зайца"},{"otter","выдры"},{"beaver","бобра"},{"polecat","хорька"},{"pine marten","лесной куницы"},{"marten","куницы"},{"badger","барсука"},{"wolverine","росомахи"},{"mink","норки"},{"seal","тюленя"},{"ringed seal","кольчатой нерпы"},{"sheep","овцы"},{"cow","коровы"},{"bull","быка"},{"pig","свиньи"},{"horse","лошади"},{"dog","собаки"},{"capercaillie","глухаря"},{"black grouse","тетерева"},{"hazel grouse","рябчика"},{"willow grouse","белой куропатки"},{"goose","гуся"},{"duck","утки"},{"swan","лебедя"},{"crane","журавля"},{"raven","ворона"},{"owl","совы"},{"eagle","орла"},{"pike","щуки"},{"perch","окуня"},{"pike-perch","судака"},{"salmon","лосося"},{"trout","форели"},{"bream","леща"},{"roach","плотвы"},{"burbot","налима"},{"whitefish","сига"},{"grayling","хариуса"}};
 std::string cooked;
 static const std::pair<const char*,const char*> methods[]={{"roasted ","Жареное мясо "},{"smoked ","Копчёное мясо "},{"dried ","Сушёное мясо "},{"salted ","Солёное мясо "}};
 for(auto& p:methods)if(name.compare(0,strlen(p.first),p.first)==0){name.erase(0,strlen(p.first));cooked=p.second;break;}
 for(auto& p:animals){std::string a=p.first;if(name==a+" cut"||(!cooked.empty()&&name==a))return (cooked.empty()?"Кусок мяса ":cooked)+p.second;
  if(cooked.empty()){if(name==a+" fur")return std::string("Мех ")+p.second;if(name==a+" leather")return std::string("Кожа ")+p.second;if(name==a+" skin")return std::string("Шкура ")+p.second;if(name==a+" rawhide")return std::string("Сыромять ")+p.second;}}
 return {};
}
inline std::wstring SearchAliases(unsigned category,const std::string& name){
 std::wstring out;auto n=Lower(name);
 if(category==5)out+=L" шкура шкуры кожа skin hide leather";
 if(n.find("fur")!=std::string::npos)out+=L" мех меховая меховой меховые fur";
 if(n.find("yarn")!=std::string::npos)out+=L" пряжа нить нитки yarn thread";
 if(n.find("cut")!=std::string::npos&&category==7)out+=L" мясо кусок еда meat cut";
 return out;
}
}
