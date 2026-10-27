#pragma once
struct ItemEntry {uint32_t id;std::string name;std::wstring label;};
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
static std::wstring Utf8Label(const std::string& s){
 if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0);
 if(n<=0)return {};std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),out.data(),n);return out;
}
static std::string ItemRussian(const std::string& source){
 static const std::pair<const char*,const char*> labels[]={
  {"Ango","анго"},
  {"Ango head","наконечник анго"},
  {"apron","фартук"},
  {"Arrow","стрела"},
  {"arrow shaft","древко стрелы"},
  {"arrowhead","наконечник стрелы"},
  {"Axe haft","топорище"},
  {"Bag","мешок"},
  {"Bandage","повязка"},
  {"Barley","ячмень"},
  {"Barley grains","зёрна ячменя"},
  {"barley-flour","ячменная мука"},
  {"Bastard Sword","полуторный меч"},
  {"Battleaxe","боевой топор"},
  {"Battleaxe head","головка боевого топора"},
  {"Battlesword","боевой меч"},
  {"Bear pipe","дудник лесной"},
  {"Bear pipe flowers","цветы дудника лесного"},
  {"Bear pipe leaves","листья дудника лесного"},
  {"Bear pipe root","корень дудника лесного"},
  {"Bearpaw mushroom","гриб медвежья лапа"},
  {"Birch twig","берёзовая ветка"},
  {"Birch withe","берёзовая вица"},
  {"Birch-bark basket","берестяная корзина"},
  {"Birch-bark box","берестяной короб"},
  {"Birch-bark cap","берестяная шапка"},
  {"Birch-bark necklace","берестяное ожерелье"},
  {"Birch-bark pot","берестяной горшок"},
  {"Birch-bark rope","берестяная верёвка"},
  {"Birch-bark shoes","лапти"},
  {"Black currant bush","куст чёрной смородины"},
  {"black currants","чёрную смородину"},
  {"Black ear mushroom","чёрное ушко"},
  {"Block of wood","чурбак"},
  {"blueberries","чернику"},
  {"Blueberry shrub","куст черники"},
  {"Blunt arrow","стрела с тупым наконечником"},
  {"Board","доска"},
  {"Bog flower","подбел многолистный"},
  {"Bog flower leaves","листья подбела многолистного"},
  {"Bogbean","вахта трёхлистная"},
  {"Bogbean leaves","листья вахты трёхлистной"},
  {"Bogbean root","корень вахты трёхлистной"},
  {"bone arrowhead","костяной наконечник стрелы"},
  {"bone fishhook","костяной рыболовный крючок"},
  {"bone sewing needle","костяная швейная игла"},
  {"bone tipped arrow","стрела с костяным наконечником"},
  {"Bowstring","тетива"},
  {"Branch","ветка"},
  {"Bream","лещ"},
  {"Broad axe","плотницкий топор"},
  {"Broad axe head","головка плотницкого топора"},
  {"Broad bean","боб"},
  {"broad beans","бобы"},
  {"Broad knife","широкий нож"},
  {"broadhead","широкий наконечник стрелы"},
  {"Broadhead arrow","стрела с широким наконечником"},
  {"Broadsword","широкий меч"},
  {"bronze ball pendant","бронзовая подвеска-шарик"},
  {"bronze bear pendant","бронзовая подвеска с медведем"},
  {"bronze bezel ring","бронзовое кольцо со щитком"},
  {"bronze bracelet","бронзовый браслет"},
  {"bronze brooch","бронзовая брошь"},
  {"bronze flower-ornamented comb","бронзовый гребень с цветами"},
  {"bronze horse-ornamented comb","бронзовый гребень с конями"},
  {"Browncoat mushroom","бурый кафтан"},
  {"Buckler","баклер"},
  {"Burbot","налим"},
  {"Burdock","лопух"},
  {"Burdock leaves","листья лопуха"},
  {"Burdock root","корень лопуха"},
  {"Carving axe","резчицкий топор"},
  {"Carving axe head","головка резчицкого топора"},
  {"Clayweed","мать-и-мачеха"},
  {"Clayweed leaves","листья мать-и-мачехи"},
  {"Clayweed seeds","семена мать-и-мачехи"},
  {"cloak","плащ"},
  {"Cloth cord","тканевый шнур"},
  {"cloudberries","морошку"},
  {"Cloudberry bramble","куст морошки"},
  {"Club","дубина"},
  {"Cord","шнур"},
  {"cowl","капюшон"},
  {"cranberries","клюкву"},
  {"Cranberry shrub","куст клюквы"},
  {"Crossbow","арбалет"},
  {"crowberries","водянику"},
  {"Crowberry shrub","куст водяники"},
  {"Dog pipe","купырь лесной"},
  {"Dog pipe leaves","листья купыря лесного"},
  {"Dog pipe root","корень купыря лесного"},
  {"dress","платье"},
  {"Egg","яйцо"},
  {"Firewood","дрова"},
  {"Fisher's knife","рыбацкий нож"},
  {"Fishing rod","удочка"},
  {"Flat rye bread","ржаная лепёшка"},
  {"Flax","лён"},
  {"Flax seeds","семена льна"},
  {"footrags","портянки"},
  {"Fur boots","меховая обувь"},
  {"Fur cap","меховая шапка"},
  {"Fur cloak","меховой плащ"},
  {"Fur hood","меховой капюшон"},
  {"Fur leggings","меховые поножи"},
  {"Fur mittens","меховые варежки"},
  {"Fur overcoat","шуба"},
  {"Fur shirt","меховая рубаха"},
  {"Golden rod","золотарник"},
  {"Golden rod flowers","цветы золотарника"},
  {"Golden rod leaves","листья золотарника"},
  {"Grainflail","молотильный цеп"},
  {"Hairy mushroom","мохнатый гриб"},
  {"Handaxe","ручной топор"},
  {"Handaxe head","головка ручного топора"},
  {"Heather","вереск"},
  {"Heather flowers","цветы вереска"},
  {"Heavy Crossbow","тяжёлый арбалет"},
  {"Hemp","конопля"},
  {"Hemp leaves","листья конопли"},
  {"Hemp seeds","семена конопли"},
  {"Hunting bow","охотничий лук"},
  {"Hunting Horn","охотничий рог"},
  {"Hunting knife","охотничий нож"},
  {"Iron coudes","железные налокотники"},
  {"iron fishhook","железный рыболовный крючок"},
  {"Iron helm","железный шлем"},
  {"Iron Kneecops","железные наколенники"},
  {"iron sewing needle","железная швейная игла"},
  {"Iron spectacle helm","железный шлем с полумаской"},
  {"Javelin","дротик"},
  {"Kaumolais knife","каумолайский нож"},
  {"Kaumolais spear","каумолайское копьё"},
  {"Kaumolais spear head","наконечник каумолайского копья"},
  {"Knife","нож"},
  {"Lake reed","тростник"},
  {"Lake reed root","корень тростника"},
  {"Lamellar cuirass","ламеллярный панцирь"},
  {"Lamellar forearm guards","ламеллярные наручи"},
  {"Lamellar hauberk","ламеллярный доспех"},
  {"Lamellar rerebraces","ламеллярные наплечники"},
  {"Lamellar shin guards","ламеллярные наголенники"},
  {"Lavaret","сиг"},
  {"Leather belt","кожаный пояс"},
  {"Leather boots","кожаные сапоги"},
  {"Leather cap","кожаная шапка"},
  {"Leather cord","кожаный шнур"},
  {"Leather cuirass","кожаный панцирь"},
  {"Leather forearm guards","кожаные наручи"},
  {"Leather leggings","кожаные поножи"},
  {"Leather rope","кожаная верёвка"},
  {"Leather shin guards","кожаные наголенники"},
  {"Leather shirt","кожаная рубаха"},
  {"Leather shoes","кожаные башмаки"},
  {"Leather trousers","кожаные штаны"},
  {"leggings","поножи"},
  {"Linen apron","льняной фартук"},
  {"Linen cowl","льняной капюшон"},
  {"Linen dress","льняное платье"},
  {"Linen footrags","льняные портянки"},
  {"Linen shirt","льняная рубаха"},
  {"Linen trousers","льняные штаны"},
  {"Linen tunic","льняная туника"},
  {"Linen undergarment","льняное исподнее"},
  {"Linen undershirt","льняная нижняя рубаха"},
  {"Linen veil","льняная вуаль"},
  {"lingonberries","бруснику"},
  {"Lingonberry shrub","куст брусники"},
  {"Lippo","липпо"},
  {"Log","бревно"},
  {"Long Mail Cowl","длинный кольчужный капюшон"},
  {"Long Mail Hauberk","длинная кольчуга"},
  {"Longbow","длинный лук"},
  {"Loop snare","силок"},
  {"Mace","булава"},
  {"Mail Cowl","кольчужный капюшон"},
  {"Mail Habergeon","короткая кольчуга"},
  {"Mail Hauberk","кольчуга"},
  {"Mail Leggings","кольчужные поножи"},
  {"Mail mittens","кольчужные рукавицы"},
  {"Marsh calla","белокрыльник"},
  {"Marsh calla leaves","листья белокрыльника"},
  {"Marsh calla root","корень белокрыльника"},
  {"Maul","молот"},
  {"Meadsweet","таволга"},
  {"Meadsweet flowers","цветы таволги"},
  {"Meadsweet leaves","листья таволги"},
  {"Milkweed","иван-чай"},
  {"Milkweed leaves","листья иван-чая"},
  {"Milkweed root","корень иван-чая"},
  {"Mother pipe","дягиль"},
  {"Mother pipe flowers","цветы дягиля"},
  {"Mother pipe leaves","листья дягиля"},
  {"Mother pipe root","корень дягиля"},
  {"Net","сеть"},
  {"Netting needle","сетевязальная игла"},
  {"Nettle","крапива"},
  {"Nettle apron","Крапивный фартук"},
  {"Nettle cowl","Крапивный капюшон"},
  {"Nettle leaves","листья крапивы"},
  {"Nettle seeds","семена крапивы"},
  {"Nettle shirt","Крапивная рубаха"},
  {"Nettle trousers","Крапивные штаны"},
  {"Nettle tunic","Крапивная туника"},
  {"Njerpez scimitar","ньерпская сабля"},
  {"Noaidi's mushroom","гриб нойды"},
  {"northern bilberries","голубику"},
  {"Northern bilberry shrub","куст голубики"},
  {"Northern bow","северный лук"},
  {"Northern knife","северный нож"},
  {"Northern spear","северное копьё"},
  {"Northern spear head","наконечник северного копья"},
  {"overcoat","кафтан"},
  {"Paddle","весло"},
  {"Paw-board fox trap","лисья ловушка с зажимом лапы"},
  {"Pea","горох"},
  {"peas","горох"},
  {"Perch","окунь"},
  {"Pike","щука"},
  {"Pike-perch","судак"},
  {"Pot","горшок"},
  {"Punt","долблёнка"},
  {"Raft","плот"},
  {"raspberries","малину"},
  {"Raspberry bush","куст малины"},
  {"Redlegger mushroom","гриб красноножка"},
  {"Ringed mushroom","кольчатый гриб"},
  {"Riverpig","кувшинка"},
  {"Riverpig root","корень кувшинки"},
  {"Roach","плотва"},
  {"Rock","камень"},
  {"Rope","верёвка"},
  {"Roseroot","золотой корень"},
  {"Roseroot leaves","листья золотого корня"},
  {"Roseroot root","корень золотого корня"},
  {"Roundshield","круглый щит"},
  {"Rye","рожь"},
  {"Rye bread","ржаной хлеб"},
  {"Rye grains","зёрна ржи"},
  {"rye-flour","ржаная мука"},
  {"Salmon","лосось"},
  {"Salt","соль"},
  {"Sand mushroom","песочный гриб"},
  {"Sauna scoop","банный ковш"},
  {"Scimitar","сабля"},
  {"Sesta","шест"},
  {"Shingle","лучина"},
  {"Shingle basket","корзина из лучины"},
  {"shirt","рубаха"},
  {"Shortbow","короткий лук"},
  {"Shortsword","короткий меч"},
  {"Shovel","лопата"},
  {"Shovel blade","полотно лопаты"},
  {"Shovel haft","черенок лопаты"},
  {"Sickle","серп"},
  {"silver bird bracelet","серебряный браслет с птицей"},
  {"silver chain bracelet","серебряный цепочный браслет"},
  {"silver ring","серебряное кольцо"},
  {"silver sun-symbol pendant","серебряная подвеска с солнцем"},
  {"Ski pole","лыжная палка"},
  {"Skis","лыжи"},
  {"Skramasaksi","скрамасакс"},
  {"Slender tree trunk","тонкий древесный ствол"},
  {"small bronze brooch","малая бронзовая брошь"},
  {"Small knife","малый нож"},
  {"Small trident","малый трезубец"},
  {"Small trident head","наконечник малого трезубца"},
  {"Soft mushroom","мягкий гриб"},
  {"Sorrel","щавель"},
  {"Sorrel leaves","листья щавеля"},
  {"Sorrel seeds","семена щавеля"},
  {"Spear","копьё"},
  {"Spear haft","древко копья"},
  {"spear head","наконечник копья"},
  {"Spindle","веретено"},
  {"Splitting axe","колун"},
  {"Splitting axe head","головка колуна"},
  {"Spruce quick-bow","простой еловый лук"},
  {"Spruce twig","еловая ветка"},
  {"Spruce withe","еловая вица"},
  {"Staff","посох"},
  {"Stone","камень"},
  {"stone arrowhead","каменный наконечник стрелы"},
  {"Stone knife","каменный нож"},
  {"stone tipped arrow","стрела с каменным наконечником"},
  {"Stone-axe","каменный топор"},
  {"Stonecrop","очиток"},
  {"Stonecrop leaves","листья очитка"},
  {"strawberries","землянику"},
  {"Strawberry shrub","куст земляники"},
  {"Tellervo's gift mushroom","дар Теллерво"},
  {"Torch","факел"},
  {"Tree trunk","ствол дерева"},
  {"Trident","трезубец"},
  {"Trident head","наконечник трезубца"},
  {"trousers","штаны"},
  {"Trout","форель"},
  {"tunic","туника"},
  {"Turnip","репа"},
  {"Turnip seeds","семена репы"},
  {"Ukko's mushroom","гриб Укко"},
  {"undergarment","исподнее"},
  {"undershirt","нижняя рубаха"},
  {"vasta","банный веник"},
  {"veil","вуаль"},
  {"Water lily","кувшинка"},
  {"Water lily root","корень кувшинки"},
  {"Wide trident","широкий трезубец"},
  {"Wide trident head","наконечник широкого трезубца"},
  {"Wolf pipe","кокорыш"},
  {"Wolf pipe leaves","листья кокорыша"},
  {"Wolf pipe root","корень кокорыша"},
  {"Wood slat","деревянная рейка"},
  {"Wooden bowl","деревянная миска"},
  {"Wooden canteen","деревянная фляга"},
  {"Wooden cup","деревянная чашка"},
  {"wooden fishhook","деревянный рыболовный крючок"},
  {"Wooden mug","деревянная кружка"},
  {"Wooden shovel","деревянная лопата"},
  {"Wooden stake","деревянный кол"},
  {"Wooden tub","деревянная кадка"},
  {"Woodsman's axe","топор лесоруба"},
  {"Woodsman's axe head","головка топора лесоруба"},
  {"Woollen apron","шерстяной фартук"},
  {"Woollen cloak","шерстяной плащ"},
  {"Woollen dress","шерстяное платье"},
  {"Woollen footrags","шерстяные портянки"},
  {"Woollen leggings","шерстяные поножи"},
  {"Woollen nalbound cap","связанная иглой шерстяная шапка"},
  {"Woollen nalbound cowl","связанный иглой шерстяной капюшон"},
  {"Woollen nalbound knee socks","связанные иглой шерстяные гольфы"},
  {"Woollen nalbound mittens","связанные иглой шерстяные варежки"},
  {"Woollen nalbound socks","связанные иглой шерстяные носки"},
  {"Woollen overcoat","шерстяной кафтан"},
  {"Woollen shirt","шерстяная рубаха"},
  {"Woollen trousers","шерстяные штаны"},
  {"Woollen tunic","шерстяная туника"},
  {"Woollen undergarment","шерстяное исподнее"},
  {"Woollen undershirt","шерстяная нижняя рубаха"},
  {"Woollen veil","шерстяная вуаль"},
  {"Yarrow","тысячелистник"},
  {"Yarrow flowers","цветы тысячелистника"},
  {"Yarrow leaves","листья тысячелистника"},
  {"Yarrow seeds","семена тысячелистника"},
  {"Yellow fingers mushroom","жёлтый пальчик"},
  {"Yellowcoat mushroom","жёлтый кафтан"},
 };
 for(const auto& row:labels)if(source==row.first)return row.second;
 return {};
}
static bool SameItemLabel(const std::wstring& a,const std::wstring& b){return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL;}
static std::wstring ItemLabel(const std::string& source){auto ru=Utf8Label(ItemRussian(source));return ru.empty()?Wide(source):ru+L" | "+Wide(source);}
static const ItemEntry* ResolveItem(const std::wstring& text,uint32_t selected,bool& ambiguous){
 ambiguous=false;const ItemEntry* found=nullptr;
 for(const auto& item:g_items){
  if(SameItemLabel(text,item.label)||SameItemLabel(text,Wide(item.name))||SameItemLabel(text,Utf8Label(ItemRussian(item.name)))){
   if(selected==item.id)return &item;
   if(found)ambiguous=true;else found=&item;
  }
 }
 return ambiguous?nullptr:found;
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
  if(!valid)continue;g_items.push_back({id,name,{}});
 }
 for(auto& item:g_items){item.label=ItemLabel(item.name);
  if(std::count_if(g_items.begin(),g_items.end(),[&](const ItemEntry& other){return Lower(item.name)==Lower(other.name);})>1)item.label+=L" [#"+std::to_wstring(item.id)+L"]";
 }
 std::sort(g_items.begin(),g_items.end(),[](const ItemEntry&a,const ItemEntry&b){auto order=CompareStringOrdinal(a.label.c_str(),-1,b.label.c_str(),-1,TRUE);return order==CSTR_EQUAL?a.id<b.id:order==CSTR_LESS_THAN;});
 for(const auto& item:g_items){auto pos=SendMessageW(g_item,CB_ADDSTRING,0,(LPARAM)item.label.c_str());SendMessageW(g_item,CB_SETITEMDATA,pos,item.id);}
 SendMessageW(g_item,CB_SETDROPPEDWIDTH,650,0);
 SetWindowTextW(g_item,ItemLabel("Nettle leaves").c_str());
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
 if(ch==L'\r'){g_typed.clear();g_typedIndex=0;Step(next,now,next==1?120000:15000);}
 else{++g_typedIndex;g_due=now+200;g_stepDeadline=now+6000;}
 return true;
}
bool SelectGame(){
 int selected=(int)SendMessageW(g_games,CB_GETCURSEL,0,0);
 if(selected<0||selected>=(int)g_available.size()){Refresh();return false;}
 return Attach(g_available[selected]);
}
void Begin(bool native,bool item=false,bool days=false){
 if(g_busy||!SelectGame())return;
 if(TextAt(kPlayer,32).empty()){Log(L"Сначала загрузите живого персонажа.");return;}
 auto state=ProbeNative();
 if(state.stage!=NativeStage::Main){Log(L"Нужен основной игровой экран. Закройте меню/диалоги и повторите. Ввод не отправлен.");return;}
 if(TextAt(0xA72B055,32)==TextAt(0x58F6F8,32)){Log(L"Нужна локальная карта: нажмите Enter в игре.");return;}
 int choice=(int)SendMessageW(g_choices,CB_GETCURSEL,0,0);int type=(int)SendMessageW(g_choices,CB_GETITEMDATA,choice,0);
 if(!native&&!item&&!days&&!Supported(type)){Log(L"Выберите тип задания.");return;}
 if(!native&&!item&&!days){unsigned minimum=MinimumQuestDays(type),day=NumberAt(0xA72B1C4);
  if(day<minimum){Log(L"Этот тип задания доступен с игрового дня "+std::to_wstring(minimum)+L". Сейчас день "+std::to_wstring(day)+L". Ввод не отправлен.");return;}
 }
 g_daysMode=days;
 if(days){g_daysBefore=NumberAt(0xA72B1C4);g_minutesBefore=NumberAt(0xA72B8E0);
  if(g_daysBefore>=100){Log(L"У персонажа уже "+std::to_wstring(g_daysBefore)+L" игровых дней. Изменения не нужны.");return;}
  g_dayDelta=100-g_daysBefore;
 }
 g_itemMode=item;g_created=0;g_groundEntity=-1;
 if(item){
  wchar_t text[384]{},qty[16]{};GetWindowTextW(g_item,text,384);GetWindowTextW(g_quantity,qty,16);
  wchar_t* end=nullptr;long count=wcstol(qty,&end,10);
  if(!*qty||*end||count<1||count>100){Log(L"Количество: целое число от1 до100.");return;}g_itemCount=(int)count;
  int selection=(int)SendMessageW(g_item,CB_GETCURSEL,0,0);uint32_t selected=selection>=0?(uint32_t)SendMessageW(g_item,CB_GETITEMDATA,selection,0):0;
  bool ambiguous=false;const ItemEntry* entry=ResolveItem(text,selected,ambiguous);
  if(!entry){Log(ambiguous?L"У этого названия несколько прототипов. Выберите строку с нужным ID из списка.":L"Предмет не найден. Выберите русский или английский вариант из списка.");return;}
  g_itemId=entry->id;g_itemName=entry->name;g_itemLabel=entry->label;
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
 for(HWND h:{g_begin,g_native,g_refresh,g_games,g_add,g_item,g_quantity,g_update,g_days})EnableWindow(h,FALSE);
 Status(days?L"Пропускаю дни штатной командой игры. Сохраняйте фокус.":item?L"Создаю предметы. Сохраняйте фокус на игре.":L"Открываю генератор. Дождитесь подтверждения выбора NPC.");
 Log(days?L"Переход с игрового дня "+std::to_wstring(g_daysBefore)+L" на день 100 (пропустить "+std::to_wstring(g_dayDelta)+L").":item?L"Добавление: "+g_itemLabel+L" × "+std::to_wstring(g_itemCount):L"Запрос типа "+std::to_wstring(g_type));
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
 if(g_phase==12){if(state.stage!=NativeStage::Command)return;if(SendChecked(g_daysMode?L'd':g_itemMode?L'y':L'\r'))Step(13,now);return;}
 if(g_phase==13){TypeChecked(state,g_daysMode?NativeStage::DayCount:g_itemMode?NativeStage::ItemName:NativeStage::DebugText,g_daysMode?std::to_string(g_dayDelta):g_itemMode?std::to_string(g_itemId):"dq",g_daysMode?30:g_itemMode?20:1,now);return;}
 if(g_phase==30){
  if(state.stage!=NativeStage::Main)return;
  uint32_t day=NumberAt(0xA72B1C4),minutes=NumberAt(0xA72B8E0);
  if(day==100&&minutes==g_minutesBefore+1440*g_dayDelta)Stop(L"Готово: игровой день 100. Календарь и время продвинуты самой игрой. Теперь создайте задание и примите его разговором для F2.");
  else Stop(L"Не удалось подтвердить переход до 100 дней. Проверьте игровой день; автоматический повтор не выполняется.");
  return;
 }
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
  Log(L"Создано под ногами: "+g_itemLabel+L" × "+std::to_wstring(g_created)+L". Подбираю в инвентарь.");
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
  if(count>=g_inventoryBefore+g_itemCount){Log(g_itemLabel+L": было "+std::to_wstring(g_inventoryBefore)+L", стало "+std::to_wstring(count));Stop(L"Предметы добавлены в инвентарь.");}
  else Stop(L"Подбор не подтверждён. Проверьте грузоподъёмность; предметы могут лежать под ногами.");
 }
 if(g_phase==24){TypeChecked(state,NativeStage::PickupCountText,std::to_string(g_itemCount),22,now);return;}
}
