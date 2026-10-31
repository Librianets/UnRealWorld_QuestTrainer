#pragma once
#include <string>
#include <vector>
namespace qtrainer {
struct Question { int type; const char* source; const wchar_t* label; };
inline const std::vector<Question>& Questions(){static const std::vector<Question> q={
 {37,"Homeland escort",L"37 — Сопровождение домой"},
 {72,"Collect branches",L"72 — Сбор веток"},
 {52,"Forest spirit?",L"52 — Лесной дух"},
 {51,"water folk?",L"51 — Водяной народ"},
 {51,"water folk?",L"51 — Водяной народ"},
 {71,"Messenger?",L"71 — Посыльный"},
 {50,"Forest maids?",L"50 — Лесные девы"},
 {100,"Treasuretale?",L"100 — Не реализовано"},
 {38,"Disoriented?",L"38 — Заблудившийся"},
 {35,"Animal Forest Cover",L"35 — Животное в лесном укрытии"},
 {40,"Items wanted",L"40 — Нужные предметы"},
 {70,"Stones for sauna",L"70 — Камни для сауны"},
 {15,"Bird thief",L"15 — Похититель птиц"},
 {3,"Fetch Herbs",L"3 — Сбор трав"},
 {10,"Old man's punt",L"10 — Долблёнка старика"},
 {5,"Adventurere's Axe",L"5 — Потерянная вещь странника"},
 {30,"Homeland Robbers",L"30 — Разбойники"},
 {20,"Bewitched wolf",L"20 — Заколдованный волк"}};return q;}
// SocQuestCreator3.89.1: native minimum played-days checks are not bypassed by forcequest.
inline unsigned MinimumQuestDays(int id){
 switch(id){case 5:case 37:case 71:return 2;case 10:return 3;case 15:return 10;
 case 20:case 50:return 20;case 30:case 51:case 52:return 30;default:return 0;}
}
// Native F5 courses are linear. Never complete unselected prerequisites.
inline bool CourseSelection(int current,int total,const std::vector<bool>& checked,std::vector<int>& queue){
 queue.clear();if(current<0||current>=total||checked.size()!=static_cast<size_t>(total))return false;
 bool gap=false;for(int i=current;i<total;++i){if(!checked[i])gap=true;else{if(gap){queue.clear();return false;}queue.push_back(i);}}
 return !queue.empty();
}
inline bool Supported(int id){if(id==100)return false;for(const auto& q:Questions())if(q.type==id)return true;return false;}
inline std::vector<int> Choices(){std::vector<int> out;for(const auto& q:Questions())if(Supported(q.type)){bool exists=false;for(int id:out)if(id==q.type)exists=true;if(!exists)out.push_back(q.type);}return out;}
inline bool Matches(const std::string& prompt,const char* label){return label&&(prompt==label||prompt==std::string("?")+label);}
enum class Answer { Wait, No, Yes, Finished };
struct Plan {
 int type=0;size_t index=0;bool finished=false;
 bool Reset(int value){type=value;index=0;finished=false;return Supported(type);}
 Answer Next(bool questionActive,const std::string& prompt){
  if(finished)return Answer::Finished;
  if(!Supported(type)||!questionActive||index>=Questions().size())return Answer::Wait;
  const auto& q=Questions()[index];if(!Matches(prompt,q.source))return Answer::Wait;
  ++index;if(q.type==type){finished=true;return Answer::Yes;}return Answer::No;
 }
};
}
