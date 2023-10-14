#include <menu.h>
#include <FFAT.h>  // ESP32 adat partició
#include <vector>

using namespace Menu;
using namespace std;
const char* fmtitle = "FFat partíció";

//minimalist SD Card driver (using arduino SD)
//we avoid allocating memory here, instead we read all info from SD
template<typename SDC>
class FSO {
public:
  using Type=SDC;
  Type& sdc;
  //idx_t selIdx=0;//preserve selection context, because we preserve folder ctx too
  //we should use filename instead! idx is useful for delete operations thou...

  File dir;

  FSO(Type& sdc):sdc(sdc) {}
  virtual ~FSO() {dir.close();}
  //open a folder
  bool goFolder(String folderName) {
    dir.close();
    Serial.println("reopen dir, context");
    dir=sdc.open(folderName.c_str());
    return dir;
  }
  //count entries on folder (files and dirs)
  long count() {
    Serial.print("count:");
    dir.rewindDirectory();
    int cnt=0;
    while(true) {
      File file=dir.openNextFile();
      if (!file) {
        file.close();
        break;
      }
      file.close();
      cnt++;
    }
    Serial.println(cnt);
    return cnt;
  }

  //get entry index by filename
  long entryIdx(String name) {
    dir.rewindDirectory();
    int cnt=0;
    while(true) {
      File file=dir.openNextFile();
      if (!file) {
        file.close();
        break;
      }
      if(name==file.name()) {
        file.close();
        return cnt;
      }
      file.close();
      cnt++;
    }
    return 0;//stay at menu start if not found
  }

  //get folder content entry by index
  String entry(long idx) {
    dir.rewindDirectory();
    idx_t cnt=0;
    while(true) {
      File file=dir.openNextFile();
      if (!file) {
        file.close();
        break;
      }
      if(idx==cnt++) {
        String n=String(file.name())+(file.isDirectory()?"/":"");
        file.close();
        return n;
      }
      file.close();
    }
    return "";
  }

};
//////////////////////////////////////////////////////////////////////
// SD Card cached menu
template<typename SDC,idx_t maxSz>
class CachedFSO:public FSO<SDC> {
public:
  using Type=SDC;
  long cacheStart=0;
  String cache[maxSz];
  long size=0;//folder size (count of files and folders)
  CachedFSO(Type& sdc):FSO<SDC>(sdc) {}
  void refresh(long start=0) {
    if (start<0) start=0;
    // Serial.print("Refreshing from:");
    // Serial.println(start);
    cacheStart=start;
    FSO<SDC>::dir.rewindDirectory();
    size=0;
    while(true) {
      File file=FSO<SDC>::dir.openNextFile();
      if (!file) {
        file.close();
        break;
      }
      if (start<=size&&size<start+maxSz)
        cache[size-start]=String(file.name())+(file.isDirectory()?"/":"");
      file.close();
      size++;
    }
  }

  prompt& operator[](idx_t i) const override {return *(prompt*)this;}//this will serve both as menu and as its own prompt

  //open a folder
  bool goFolder(String folderName) {
    if (!FSO<SDC>::goFolder(folderName)) return false;
    refresh();
    return true;
  }
  long count() {return size;}

  long entryIdx(String name) {
    idx_t sz=min(count(),(long)maxSz);
    for(int i=0;i<sz;i++)
      if (name==cache[i]) return i+cacheStart;
    long at=FSO<SDC>::entryIdx(name);
    //put cache around the missing item
    refresh(at-(maxSz>>1));
    return at;
  }
  String entry(long idx) {
    if (0>idx||idx>=size) return "";
    if (cacheStart<=idx&&idx<(cacheStart+maxSz)) return cache[idx-cacheStart];
    refresh(idx-(maxSz>>1));
    return entry(idx);
  }
};

template<typename FS>
class FileMenu : public menuNode, public FS {
public:
  String folderName="/";//set this to other folder when needed
  String selectedFolder="/";
  String selectedFile="";
  //FileMenu(const char* name, int maxFiles) : menuNode(name), maxFiles(maxFiles) {}
  inline FileMenu(typename FS::Type& sd,constText* title,const char* at,Menu::action act=doNothing,Menu::eventMask mask=noEvent)
    :menuNode(title,0,NULL,act,mask,
      wrapStyle,(systemStyles)(_menuData|_canNav))
    ,FS(sd) {
    dir = FFat.open("/");
        

    //menuNode(fmtitle,sz,prompts.data(),a,e,style,ss);
  }

  inline void begin() {
    FS::goFolder(folderName);
  }
 
  result sysHandler(SYS_FUNC_PARAMS) override {
    if (event == enterEvent) {
      Serial.print("Selected file: ");
      Serial.println(fileList[nav.sel - 1]);
      // Implement your file handling logic here
      // You can access the selected file using fileList[nav.sel - 1]
    }

    return proceed;
  }

private:
    std::vector<prompt*> prompts;
  int maxFiles;
  File dir,file;
  char fileList[10][50];  // Maximum 10 files with 50 characters for each file name
  int fileCount = 0;

};

