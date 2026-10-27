#pragma once
struct ItemEntry {uint32_t id;std::string name;};
std::vector<ItemEntry> g_items;
int g_inventoryBefore=0;uint32_t g_groundBefore=0;int g_groundEntity=-1;int g_created=0;
std::vector<uint16_t> g_groundCounts;
std::string Lower(std::string s){for(char& ch:s)if(ch>='A'&&ch<='Z')ch+=32;return s;}
std::string ItemName(uint32_t id){
 if(!id||id>99900)return {};
 uintptr_t at=id<50000?0x58F9FF8+172*id:0x59216A8+172*(id-50000);
 return TextAt(at+9,80);
}
int InventoryCount(const std::string& name){
 uint32_t ids[100]{},counts[100]{};
 if(!Read(0xA72ACD0,ids,sizeof(ids))||!Read(0xA72AE60,counts,sizeof(counts)))return -1;
 int sum=0;for(int i=0;i<100;++i)if(ids[i]&&Lower(ItemName(ids[i]))==Lower(name))sum+=counts[i];return sum;
}
void LoadItems(){
 g_items.clear();SendMessageW(g_item,CB_RESETCONTENT,0,0);
 std::vector<unsigned char> bytes(400*172);
 if(!Read(0x58F9FF8,bytes.data(),bytes.size()))return;
 for(uint32_t id=1;id<400;++id){
  auto at=bytes.data()+id*172;if(!at[0])continue;
  size_t n=0;while(n<80&&at[9+n])++n;if(!n||n>49)continue;
  std::string name((char*)at+9,n);bool valid=true;
  for(unsigned char c:name)if(c<32||c>126||c=='*')valid=false;
  if(!valid)continue;g_items.push_back({id,name});
 }
 std::sort(g_items.begin(),g_items.end(),[](const ItemEntry&a,const ItemEntry&b){return Lower(a.name)<Lower(b.name);});
 for(const auto& item:g_items){auto pos=SendMessageW(g_item,CB_ADDSTRING,0,(LPARAM)Wide(item.name).c_str());SendMessageW(g_item,CB_SETITEMDATA,pos,item.id);}
 SetWindowTextW(g_item,L"Nettle leaves");
 Log(L"Прочитано прототипов предметов: "+std::to_wstring(g_items.size()));
}
bool GroundCounts(std::vector<uint16_t>& counts){
 std::vector<unsigned char> data(9951*36);if(!Read(0xA6D28A8,data.data(),data.size()))return false;
 uint32_t x=NumberAt(0xA6D1C9C),y=NumberAt(0xA72BD40);counts.assign(9951,0);
 for(int i=0;i<9951;++i){auto at=data.data()+i*36;uint16_t xx=0,yy=0,n=0;uint32_t item=0;
  memcpy(&xx,at,2);memcpy(&yy,at+2,2);memcpy(&n,at+8,2);memcpy(&item,at+12,4);
  if(at[4]==1&&xx==x&&yy==y&&item==g_itemId)counts[i]=n;
 }return true;
}
void Step(int phase,ULONGLONG now,int timeout=6000){g_phase=phase;g_due=now+160;g_stepDeadline=now+timeout;}
bool SendChecked(wchar_t key){if(SendKey(key))return true;Stop(L"Не удалось передать клавишу. Дальнейший ввод остановлен.");return false;}
bool TypeChecked(const NativeState& state,NativeStage expected,const std::string& value,int next,ULONGLONG now){
 if(state.stage!=expected)return false;
 std::string typed=TextAt(0xA89B9A8,100);
 if(g_typed!=value){g_typed=value;g_typedIndex=0;}
 if(typed!=value.substr(0,g_typedIndex))return false;
 wchar_t ch=g_typedIndex<value.size()?(unsigned char)value[g_typedIndex]:L'\r';
 if(!SendChecked(ch))return false;
 if(ch==L'\r'){g_typed.clear();g_typedIndex=0;Step(next,now,next==1?120000:6000);}
 else{++g_typedIndex;g_due=now+200;g_stepDeadline=now+6000;}
 return true;
}
bool SelectGame(){
 int selected=(int)SendMessageW(g_games,CB_GETCURSEL,0,0);
 if(selected<0||selected>=(int)g_available.size()){Refresh();return false;}
 return Attach(g_available[selected]);
}
void Begin(bool native,bool item=false){
 if(g_busy||!SelectGame())return;
 if(TextAt(kPlayer,32).empty()){Log(L"Сначала загрузите живого персонажа.");return;}
 auto state=ProbeNative();
 if(state.stage!=NativeStage::Main){Log(L"Нужен основной игровой экран. Закройте меню/диалоги и повторите. Ввод не отправлен.");return;}
 if(TextAt(0xA72B055,32)==TextAt(0x58F6F8,32)){Log(L"Нужна локальная карта: нажмите Enter в игре.");return;}
 int choice=(int)SendMessageW(g_choices,CB_GETCURSEL,0,0);int type=(int)SendMessageW(g_choices,CB_GETITEMDATA,choice,0);
 if(!native&&!item&&!Supported(type)){Log(L"Выберите тип задания.");return;}
 if(!native&&!item){unsigned minimum=MinimumQuestDays(type),day=NumberAt(0xA72B1C4);
  if(day<minimum){Log(L"Этот тип задания доступен с игрового дня "+std::to_wstring(minimum)+L". Сейчас день "+std::to_wstring(day)+L". Ввод не отправлен.");return;}
 }
 g_itemMode=item;g_created=0;g_groundEntity=-1;
 if(item){
  wchar_t text[128]{},qty[16]{};GetWindowTextW(g_item,text,128);GetWindowTextW(g_quantity,qty,16);
  g_itemName.clear();for(wchar_t c:std::wstring(text)){if(c<32||c>126){Log(L"Выберите английское название из списка.");return;}g_itemName+=(char)c;}
  wchar_t* end=nullptr;long count=wcstol(qty,&end,10);
  if(!*qty||*end||count<1||count>100){Log(L"Количество: целое число от1 до100.");return;}g_itemCount=(int)count;
  g_itemId=0;for(const auto& entry:g_items)if(Lower(entry.name)==Lower(g_itemName)){g_itemId=entry.id;g_itemName=entry.name;break;}
  if(!g_itemId){Log(L"Предмет не найден в текущих прототипах игры. Нажмите «Обновить предметы».");return;}
  // Native key y takes the exact prototype ID, avoiding the substring-name factory.
  g_inventoryBefore=InventoryCount(g_itemName);
  if(g_inventoryBefore<0||!GroundCounts(g_groundCounts)){Log(L"Не удалось прочитать инвентарь/предметы.");return;}
 }
 if(IsIconic(g_game))ShowWindow(g_game,SW_RESTORE);SetForegroundWindow(g_game);
 if(GetForegroundWindow()!=g_game){Log(L"Не удалось переключиться на игру. Повторите.");return;}
 g_previousLayout=GetKeyboardLayout(g_thread);g_english=LoadKeyboardLayoutW(L"00000409",0);DWORD_PTR ignored=0;
 if(!g_english||!SendMessageTimeoutW(g_game,WM_INPUTLANGCHANGEREQUEST,0,(LPARAM)g_english,SMTO_ABORTIFHUNG,500,&ignored)){LayoutBack();Log(L"Не удалось включить английскую раскладку.");return;}
 g_type=native?0:type;g_plan.Reset(type);g_before=NumberAt(kCounter);g_phase=0;g_hadFirst=false;g_busy=true;
 auto now=GetTickCount64();g_due=now+500;g_stepDeadline=now+6000;g_deadline=now+(item?60000+5000*g_itemCount:180000);g_typed.clear();g_typedIndex=0;
 for(HWND h:{g_begin,g_native,g_refresh,g_games,g_add,g_item,g_quantity,g_update})EnableWindow(h,FALSE);
 Status(item?L"Создаю предметы. Сохраняйте фокус на игре.":L"Открываю генератор. Дождитесь подтверждения выбора NPC.");
 Log(item?L"Добавление: "+Wide(g_itemName)+L" × "+std::to_wstring(g_itemCount):L"Запрос типа "+std::to_wstring(g_type));
}
void Tick(){
 if(!g_busy)return;auto now=GetTickCount64();if(now<g_due)return;
 if(WaitForSingleObject(g_process,0)!=WAIT_TIMEOUT){Stop(L"Процесс игры закрыт.");return;}
 if(GetForegroundWindow()!=g_game){Stop(L"Остановлено: окно игры потеряло фокус. Открытый диалог завершите вручную.");return;}
 if(now>g_deadline||now>g_stepDeadline){Log(L"Ожидался этап "+std::to_wstring(g_phase)+L". Возврат на основной экран может потребовать Esc, если остался выбор цели.");Stop(L"Ожидаемое состояние игры не подтверждено. Ввод остановлен; новое задание/выдача не подтверждены.");return;}
 auto state=ProbeNative();
 static int phase=-1,stage=-1;static uint32_t call=0;
 if(phase!=g_phase||stage!=(int)state.stage||call!=state.debugCall){
  phase=g_phase;stage=(int)state.stage;call=state.debugCall;
  Log(L"[state] phase="+std::to_wstring(phase)+L" stage="+std::to_wstring(stage)+L" call="+std::to_wstring(call)+L" frame="+std::to_wstring(state.debugFrame));
 }
 if(g_phase==0){if(state.stage!=NativeStage::Main)return;if(SendChecked(L'*'))Step(10,now);return;}
 if(g_phase==10){if(state.stage!=NativeStage::Minus)return;Log(L"Вход в служебную функцию подтверждён.");if(SendChecked(L'-'))Step(11,now);return;}
 if(g_phase==11){if(state.stage!=NativeStage::LetterJ)return;if(SendChecked(L'j'))Step(12,now);return;}
 if(g_phase==12){if(state.stage!=NativeStage::Command)return;if(SendChecked(g_itemMode?L'y':L'\r'))Step(13,now);return;}
 if(g_phase==13){TypeChecked(state,g_itemMode?NativeStage::ItemName:NativeStage::DebugText,g_itemMode?std::to_string(g_itemId):"dq",g_itemMode?20:1,now);return;}
 if(g_phase==1){
  if(state.stage==NativeStage::Main){Stop(L"Человек не выбран или выбор отменён. Повторите и укажите клетку с NPC.");return;}
  if(state.stage==NativeStage::Npc){if(g_type==0){Stop(L"CHOOSE TARGET открыт. Укажите NPC и отвечайте Y/N в игре.");return;}if(!g_hadFirst){g_hadFirst=true;Log(L"Игра открыла CHOOSE TARGET. Теперь укажите человека и подтвердите Enter.");Status(L"Укажите NPC в игровом окне и нажмите Enter.");}return;}
  if(state.stage!=NativeStage::QuestQuestion)return;
  static const uint32_t sites[]={0x5770B0,0x5770C6,0x5770DC,0x5770F2,0x577108,0x57711E,0x577134,0x57714A,0x577160,0x577176,0x57718C,0x5771A2,0x5771B5,0x5771C8,0x5771DB,0x5771EE,0x577201,0x577214};
  if(g_plan.index>=std::size(sites)||state.debugCall!=sites[g_plan.index])return;
  std::string prompt=TextAt(kPrompt,512);auto answer=g_plan.Next(QuestionActive(),prompt);
  if(answer!=Answer::Yes&&answer!=Answer::No)return;
  bool yes=answer==Answer::Yes;Log((yes?L"Да: ":L"Нет: ")+Wide(prompt));
  if(g_type==0){Stop(L"Штатный выбор задания открыт. Отвечайте Y/N в игре.");return;}
  if(!SendChecked(yes?L'y':L'n'))return;
  g_due=now+450;if(yes){Step(2,now,30000);Status(L"Ожидаю подтверждение создания задания...");}return;
 }
 if(g_phase==2){auto counter=NumberAt(kCounter),id=NumberAt(kQuest);auto title=TextAt(kQuest+31,160);
  if(counter>g_before&&id>g_before&&!title.empty()){Log(L"Создано предложение задания #"+std::to_wstring(id)+L": "+Wide(title));Stop(L"Предложение создано у NPC. Поговорите с ним и примите просьбу — тогда она появится в F2.");}
  else if(state.stage==NativeStage::Main){if(g_type==70)Log(L"Для камней сауны генератор дополнительно ищет подходящую баню в поселении.");Stop(L"Генератор вернул управление без нового задания: проверьте сезон, NPC и условия задания.");}return;
 }
 if(g_phase==20){
  if(state.stage!=NativeStage::Main)return;
  std::vector<uint16_t> after;if(!GroundCounts(after))return;
  int delta=0;g_groundEntity=-1;
  for(size_t i=0;i<after.size();++i){delta+=int(after[i])-int(g_groundCounts[i]);if(after[i]>g_groundCounts[i])g_groundEntity=(int)i;}
  if(delta!=1){Stop(L"Игра не подтвердила создание ровно одного предмета. Подбор отменён.");return;}
  ++g_created;g_groundCounts=std::move(after);
  if(g_created<g_itemCount){g_typed.clear();g_typedIndex=0;Step(0,now);return;}
  Log(L"Создано под ногами: "+Wide(g_itemName)+L" × "+std::to_wstring(g_created)+L". Подбираю в инвентарь.");
  if(SendChecked(L'g'))Step(21,now);return;
 }
 if(g_phase==21){
  if(state.stage==NativeStage::Main){int count=InventoryCount(g_itemName);if(count>=g_inventoryBefore+g_itemCount){Stop(L"Предметы добавлены в инвентарь.");}return;}
  if(state.stage==NativeStage::PickupCount){
   uint32_t index=NumberAt(0x6EA934)+NumberAt(0x5E2C08),row[8]{};
   if(index<500&&Read(0x6EEDA8+32*index,row,sizeof(row))&&row[2]==g_itemId&&(int)row[1]==g_groundEntity)Step(22,now);
   return;
  }
  if(state.stage!=NativeStage::Pickup)return;
  uint32_t rows[500][8]{};if(!Read(0x6EEDA8,rows,sizeof(rows)))return;
  int first=(int)NumberAt(0x6F78DC),last=(int)NumberAt(0x5E23E4);
  if(first<0||last<first||last>=500)return;
  int target=-1;for(int i=first;i<=last;++i)if(rows[i][2]==g_itemId&&(int)rows[i][1]==g_groundEntity){target=i;break;}
  if(target<0){Stop(L"Созданный предмет не виден на странице подбора. Он остался под ногами; подберите вручную.");return;}
  for(int i=0;i<500;++i)if(rows[i][4]){Stop(L"В подборе уже есть выбранные предметы. Продолжите подбор вручную.");return;}
  static const wchar_t keys[]=L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  wchar_t key=keys[target%52];if(SendChecked(key))Step(22,now);return;
 }
 if(g_phase==22){
  if(state.stage==NativeStage::PickupCount){
   if(g_itemCount<=9){if(SendChecked(wchar_t(L'0'+g_itemCount)))g_due=now+250;}
   else if(SendChecked(L' '))Step(24,now);
   return;
  }
  if(state.stage==NativeStage::Main){if(InventoryCount(g_itemName)>=g_inventoryBefore+g_itemCount)Stop(L"Предметы добавлены в инвентарь.");return;}
  if(state.stage!=NativeStage::Pickup)return;
  uint32_t rows[500][8]{};if(!Read(0x6EEDA8,rows,sizeof(rows)))return;
  bool selected=false;for(const auto& row:rows)if(row[4]){
   if(row[2]!=g_itemId||(int)row[1]!=g_groundEntity){Stop(L"Подбор остановлен: выбран другой предмет.");return;}selected=true;
  }
  if(selected&&SendChecked(L' '))Step(23,now,15000);return;
 }
 if(g_phase==23){
  if(state.stage!=NativeStage::Main)return;
  int count=InventoryCount(g_itemName);
  if(count>=g_inventoryBefore+g_itemCount){Log(Wide(g_itemName)+L": было "+std::to_wstring(g_inventoryBefore)+L", стало "+std::to_wstring(count));Stop(L"Предметы добавлены в инвентарь.");}
  else Stop(L"Подбор не подтверждён. Проверьте грузоподъёмность; предметы могут лежать под ногами.");
 }
 if(g_phase==24){TypeChecked(state,NativeStage::PickupCountText,std::to_string(g_itemCount),22,now);return;}
}
