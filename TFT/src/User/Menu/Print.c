#include "Print.h"
#include "includes.h"

// File list number per page
#define NUM_PER_PAGE 5
// error labels for files/Volume errors
const int16_t labelVolumeError[3] = {LABEL_READ_TFTSD_ERROR, LABEL_READ_U_DISK_ERROR, LABEL_READ_ONBOARDSD_ERROR};
static bool list_mode = true;

const GUI_RECT titleRect = {10, (TITLE_END_Y - BYTE_HEIGHT) / 2, LCD_WIDTH - 10, (TITLE_END_Y - BYTE_HEIGHT) / 2 + BYTE_HEIGHT};

const GUI_RECT gcodeRect[NUM_PER_PAGE] = {
#ifdef PORTRAIT_MODE
  {BYTE_WIDTH/2+0*SPACE_X_PER_ICON, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   1*SPACE_X_PER_ICON-BYTE_WIDTH/2, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+1*SPACE_X_PER_ICON, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   2*SPACE_X_PER_ICON-BYTE_WIDTH/2, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+2*SPACE_X_PER_ICON, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   3*SPACE_X_PER_ICON-BYTE_WIDTH/2, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+0*SPACE_X_PER_ICON, 2*ICON_HEIGHT+1*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   1*SPACE_X_PER_ICON-BYTE_WIDTH/2, 2*ICON_HEIGHT+1*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+1*SPACE_X_PER_ICON, 2*ICON_HEIGHT+1*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   2*SPACE_X_PER_ICON-BYTE_WIDTH/2, 2*ICON_HEIGHT+1*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},
#else
  {BYTE_WIDTH/2+0*SPACE_X_PER_ICON, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   1*SPACE_X_PER_ICON-BYTE_WIDTH/2, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+1*SPACE_X_PER_ICON, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   2*SPACE_X_PER_ICON-BYTE_WIDTH/2, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+2*SPACE_X_PER_ICON, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   3*SPACE_X_PER_ICON-BYTE_WIDTH/2, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+3*SPACE_X_PER_ICON, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   4*SPACE_X_PER_ICON-BYTE_WIDTH/2, 1*ICON_HEIGHT+0*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},

  {BYTE_WIDTH/2+0*SPACE_X_PER_ICON, 2*ICON_HEIGHT+1*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2,
   1*SPACE_X_PER_ICON-BYTE_WIDTH/2, 2*ICON_HEIGHT+1*SPACE_Y+ICON_START_Y+(SPACE_Y-BYTE_HEIGHT)/2+BYTE_HEIGHT},
#endif
};

void normalNameDisp(const GUI_RECT *rect, uint8_t *name)
{
  if (name == NULL) return;

  GUI_ClearPrect(rect);
  GUI_SetRange(rect->x0, rect->y0, rect->x1, rect->y1);
  GUI_DispStringInPrect(rect, name);
  GUI_CancelRange();
}

// update files menu in icon mode
void gocdeIconDraw(void)
{
  uint8_t i = 0;
  uint8_t baseIndex = infoFile.curPage * NUM_PER_PAGE;
  ITEM curItem = {ICON_NULL, LABEL_NULL};

  // draw folders
  for (i = 0; (i + baseIndex < infoFile.folderCount) && (i < NUM_PER_PAGE); i++)
  {
    curItem.icon = ICON_FOLDER;
    menuDrawItem(&curItem, i);
    normalNameDisp(&gcodeRect[i], (uint8_t*)infoFile.folder[i + baseIndex]);
  }

  // draw gcode files
  for (; (i + baseIndex < infoFile.folderCount + infoFile.fileCount) && (i < NUM_PER_PAGE); i++)
  {
    restoreFileExtension(i + baseIndex - infoFile.folderCount);  // restore filename extension if filename extension feature is disabled

    if (EnterDir(infoFile.file[i + baseIndex - infoFile.folderCount]) == false)  // always use short filename for file path
      break;
    // if model preview bmp exists, display bmp directly without writing to flash
    if (infoMachineSettings.firmwareType == FW_REPRAPFW || !model_DirectDisplay(getIconStartPoint(i), infoFile.title))
    {
      curItem.icon = ICON_FILE;
      menuDrawItem(&curItem, i);
    }
    ExitDir();

    hideFileExtension(i + baseIndex - infoFile.folderCount);  // hide filename extension if filename extension feature is disabled
    normalNameDisp(&gcodeRect[i], (uint8_t*)infoFile.file[i + baseIndex - infoFile.folderCount]);  // always use short filename
  }

  // clear blank icons
  for (; (i < NUM_PER_PAGE); i++)
  {
    curItem.icon = ICON_NULL;
    menuDrawItem(&curItem, i);
  }
}

// update items in list mode
void gocdeListDraw(LISTITEM * item, uint16_t index, uint8_t itemPos)
{
  if (index < infoFile.folderCount)  // folder
  {
    item->icon = CHARICON_FOLDER;
    item->titlelabel.index = LABEL_DYNAMIC;
    item->itemType = LIST_LABEL;
    setDynamicLabel(itemPos, infoFile.folder[index]);
  }
  else if (index < (infoFile.folderCount + infoFile.fileCount))  // gcode file
  {
    item->icon = CHARICON_FILE;
    item->itemType = LIST_LABEL;
    item->titlelabel.index = LABEL_DYNAMIC;
    setDynamicLabel(itemPos, hideFileExtension(index - infoFile.folderCount));  // hide filename extension if filename extension feature is disabled
  }
}

// start print
void startPrint(void)
{
  OPEN_MENU(menuBeforePrinting);
}

// open selected file/folder
bool printPageItemSelected(uint16_t index)
{
  bool hasUpdate = true;

  if (index < infoFile.folderCount)  // folder
  {
    if (EnterDir(infoFile.folder[index]) == false)
    {
      hasUpdate = false;
    }
    else
    {
      scanPrintFiles();
      infoFile.curPage = 0;
    }
  }
  else if (index < infoFile.folderCount + infoFile.fileCount)  // gcode file
  {
    infoFile.fileIndex = index - infoFile.folderCount;
    char * filename = restoreFileExtension(infoFile.fileIndex);  // restore filename extension if filename extension feature is disabled

    if (infoHost.connected != true || EnterDir(infoFile.file[infoFile.fileIndex]) == false)  // always use short filename for file path
    {
      hasUpdate = false;
    }
    else
    {
      // load model preview in flash if icon exists
      setPrintModelIcon(infoFile.source < BOARD_SD && model_DecodeToFlash(infoFile.title));

      char temp_info[FILE_NUM + 50];
      sprintf(temp_info, (char *)textSelect(LABEL_START_PRINT), (uint8_t *)(filename));  // display short or long filename

      // confirm file selction
      setDialogText(LABEL_PRINT, (uint8_t *)temp_info, LABEL_CONFIRM, LABEL_CANCEL);
      showDialog(DIALOG_TYPE_QUESTION, startPrint, ExitDir, NULL);

      hasUpdate = false;
    }
  }

  return hasUpdate;
}

void menuPrintFromSource(void)
{
  MENUITEMS printIconItems = {
    // title
    LABEL_NULL,
    // icon                          label
    {
      {ICON_NULL,                    LABEL_NULL},
      {ICON_NULL,                    LABEL_NULL},
      {ICON_NULL,                    LABEL_NULL},
      {ICON_NULL,                    LABEL_NULL},
      {ICON_NULL,                    LABEL_NULL},
      {ICON_PAGE_UP,                 LABEL_PAGE_UP},
      {ICON_PAGE_DOWN,               LABEL_PAGE_DOWN},
      {ICON_BACK,                    LABEL_BACK},
    }
  };

  KEY_VALUES key_num = KEY_IDLE;
  uint8_t update = 1;  // 0: no update, 1: update with title bar, 2: update without title bar
  uint8_t pageCount = (infoFile.folderCount + infoFile.fileCount + (NUM_PER_PAGE - 1)) / NUM_PER_PAGE;

  GUI_Clear(infoSettings.bg_color);
  GUI_DispStringInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, LABEL_LOADING);

  if (mountFS() == true && scanPrintFiles() == true)
  {
    if (MENU_IS_NOT(menuPrintFromSource))  // Menu index be modify when "scanPrintFilesGcodeFs". (echo,error,warning popup windows)
    {
      return;
    }
    if (list_mode != true)
    {
      printIconItems.title.address = (uint8_t*)infoFile.title;
      menuDrawPage(&printIconItems);
    }
  }
  else
  {
    if (infoFile.source == BOARD_SD)  // error when the filesystem selected from TFT not available
      GUI_DispStringInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, (uint8_t*)requestCommandInfo.cmd_rev_buf);
    else
      GUI_DispStringInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, labelVolumeError[infoFile.source]);
    Delay_ms(1000);
    CLOSE_MENU();
  }

  while (MENU_IS(menuPrintFromSource))
  {
    if (list_mode != true)  // select item from icon view
    {
      key_num = menuKeyGetValue();
      pageCount = (infoFile.folderCount + infoFile.fileCount + (NUM_PER_PAGE - 1)) / NUM_PER_PAGE;

      // read encoder position and change key index to page up/down

      switch (key_num)
      {
        case KEY_ICON_5:
        case KEY_DECREASE:
          if (infoFile.curPage > 0)
          {
            infoFile.curPage--;
            update = 2;  // request no title bar update
          }
          break;

        case KEY_ICON_6:
        case KEY_INCREASE:
          if (infoFile.curPage + 1 < pageCount)
          {
            infoFile.curPage++;
            update = 2;  // request no title bar update
          }
          break;

        case KEY_ICON_7:
          infoFile.curPage = 0;

          if (IsRootDir() == true)
          {
            clearInfoFile();
            CLOSE_MENU();
            break;
          }
          else
          {
            ExitDir();
            scanPrintFiles();
            update = 1;
          }
          break;

        case KEY_IDLE:
          break;

        default:
          if (printPageItemSelected(infoFile.curPage * NUM_PER_PAGE + key_num))
            update = 1;
          break;
      }
    }
    else // select item from list view
    {
      key_num = listViewGetSelectedIndex();

      switch (key_num)
      {
        case KEY_BACK:
          if (IsRootDir() == true)
          {
            clearInfoFile();
            CLOSE_MENU();
          }
          else
          {
            ExitDir();
            scanPrintFiles();
            update = 1;
          }
          break;

        case KEY_PAGEUP:
        case KEY_PAGEDOWN:
        case KEY_IDLE:
          break;

        default:
          if (printPageItemSelected(key_num))
            update = 1;
          break;
      }
    }

    // refresh file menu
    if (update != 0)
    {
      if (list_mode != true)
      {
        printIconItems.title.address = (uint8_t *)infoFile.title;
        gocdeIconDraw();

        if (update != 2)  // update title only when entering/exiting to/from directory
          menuDrawTitle((uint8_t *)infoFile.title);
      }
      else
      { // title bar is also drawn by listViewCreate
        listViewCreate((LABEL){.address = (uint8_t *)infoFile.title}, NULL, infoFile.folderCount + infoFile.fileCount,
                       &infoFile.curPage, false, NULL, gocdeListDraw);
      }

      Scroll_CreatePara(&scrollLine, (uint8_t *)infoFile.title, &titleRect);

      update = 0;  // finally reset update request
    }

    GUI_SetBkColor(infoSettings.title_bg_color);
    Scroll_DispString(&scrollLine, LEFT);
    GUI_SetBkColor(infoSettings.bg_color);

    #ifdef SD_CD_PIN
      if (isVolumeExist(infoFile.source) != true)
      {
        resetInfoFile();
        CLOSE_MENU();
      }
    #endif

    loopProcess();
  }
}

void menuPrint(void)
{
  if (infoMachineSettings.firmwareType == FW_REPRAPFW)
  {
    list_mode = infoSettings.files_list_mode;
    infoFile.source = BOARD_SD;
    REPLACE_MENU(menuPrintFromSource);
    goto selectEnd;
  }

  MENUITEMS sourceSelItems = {
    // title
    LABEL_PRINT,
    // icon                          label
    {
      {ICON_ONTFT_SD,                LABEL_TFTSD},
      #ifdef U_DISK_SUPPORT
        {ICON_U_DISK,                  LABEL_U_DISK},
        #define ONBOARD_SD_INDEX 2
      #else
        {ICON_NULL,                    LABEL_NULL},
        #define ONBOARD_SD_INDEX 1
      #endif
      {ICON_NULL,                    LABEL_NULL},
      {ICON_NULL,                    LABEL_NULL},
      {ICON_SCREEN_INFO,             LABEL_PREVIOUS_PRINT_DATA},
      {ICON_NULL,                    LABEL_NULL},
      {ICON_NULL,                    LABEL_NULL},
      {ICON_BACK,                    LABEL_BACK},
    }
  };

  KEY_VALUES key_num = KEY_IDLE;

  sourceSelItems.items[ONBOARD_SD_INDEX].icon = (infoMachineSettings.onboardSD == ENABLED) ? ICON_ONBOARD_SD : ICON_NULL;
  sourceSelItems.items[ONBOARD_SD_INDEX].label.index = (infoMachineSettings.onboardSD == ENABLED) ? LABEL_ONBOARDSD : LABEL_NULL;

  menuDrawPage(&sourceSelItems);

  while (MENU_IS(menuPrint))
  {
    key_num = menuKeyGetValue();
    switch (key_num)
    {
      case KEY_ICON_0:
        list_mode = infoSettings.files_list_mode;  // follow list mode setting in TFT SD card
        infoFile.source = TFT_SD;
        OPEN_MENU(menuPrintFromSource);
        OPEN_MENU(menuPrintRestore);
        goto selectEnd;

      #ifdef U_DISK_SUPPORT
        case KEY_ICON_1:
          list_mode = infoSettings.files_list_mode;  // follow list mode setting in TFT USB stick
          infoFile.source = TFT_UDISK;
          OPEN_MENU(menuPrintFromSource);
          OPEN_MENU(menuPrintRestore);
          goto selectEnd;
        case KEY_ICON_2:
      #else
        case KEY_ICON_1:
      #endif
        if (infoMachineSettings.onboardSD == ENABLED)
        {
          list_mode = true;  // force list mode in Onboard sd card
          infoFile.source = BOARD_SD;
          OPEN_MENU(menuPrintFromSource);  // TODO: fix here,  onboard sd card PLR feature
          goto selectEnd;
        }
        break;

      case KEY_ICON_4:
        if (infoPrintSummary.name[0] != 0)
          printInfoPopup();
        break;

      case KEY_ICON_7:
        CLOSE_MENU();
        return;

      default:
        break;
    }
    loopProcess();
  }

selectEnd:
  if (!infoHost.printing)  // prevent reset if printing from other source
  {
    resetInfoFile();
    powerFailedSetDriverSource(getCurFileSource());
  }
}
