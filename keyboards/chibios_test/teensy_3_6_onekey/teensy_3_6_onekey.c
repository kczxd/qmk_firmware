#include "teensy_3_6_onekey.h"

/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

/**
 * @brief   Card monitor timer.
 */
static virtual_timer_t tmr;
static bool init_done = FALSE;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
static event_source_t inserted_event, removed_event;

int local_fatfs_init(void);

/**
 * @brief   Insertion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p BaseBlockDevice object
 *
 * @notapi
 */
static void tmrfunc(void *p) {
  BaseBlockDevice *bbdp = p;

  chSysLockFromISR();
  if (cnt > 0) {
    if (blkIsInserted(bbdp)) {
      if (--cnt == 0) {
        chEvtBroadcastI(&inserted_event);
      }
    }
    else
      cnt = POLLING_INTERVAL;
  }
  else {
    if (!blkIsInserted(bbdp)) {
      cnt = POLLING_INTERVAL;
      chEvtBroadcastI(&removed_event);
    }
  }
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, bbdp);
  chSysUnlockFromISR();
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] p         pointer to an object implementing @p BaseBlockDevice
 *
 * @notapi
 */
static void tmr_init(void *p) {

  chEvtObjectInit(&inserted_event);
  chEvtObjectInit(&removed_event);
  chSysLock();
  cnt = POLLING_INTERVAL;
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, p);
  chSysUnlock();
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/**
 * @brief FS object.
 */
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool fs_ready = FALSE;

static FRESULT scan_files(char *path) {
  static FILINFO fno;
  FRESULT res;
  DIR dir;
  size_t i;
  char *fn;
  char buf[80];

  debug("Entering scan_files\n");
  if(!init_done) local_fatfs_init();

  if(!fs_ready) {
    debug("Attempt to mount card\n");
    res = f_mount(&SDC_FS, "/", 1);
    if (res != FR_OK) {
      sdcDisconnect(&SDCD1);
      debug("Failed to mount card!\n");
      return res;
    }
    fs_ready = TRUE;
  }

  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    debug("opendir succeeded\n");
    i = strlen(path);
    while (((res = f_readdir(&dir, &fno)) == FR_OK) && fno.fname[0]) {
      if (FF_FS_RPATH && fno.fname[0] == '.')
        continue;
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {
        *(path + i) = '/';
        strcpy(path + i + 1, fn);
        res = scan_files(path);
        *(path + i) = '\0';
        if (res != FR_OK)
          break;
      }
      else {
        sprintf(buf, "%s/%s\r\n", path, fn);
        debug(buf);
      }
    }
  }
  else {
    debug("opendir FAILED\n");
  }
  return res;
}

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * Card insertion event.
 */
static void InsertHandler(eventid_t id) {
  FRESULT err;

  debug("Insertion detected\n");
  (void)id;
  /*
   * On insertion SDC initialization and FS mount.
   */
  if (sdcConnect(&SDCD1)){
    debug("Connect failed\n");
    return;
  }
  debug("Connect succeeded, attempting to mount\n");

  err = f_mount(&SDC_FS, "/", 1);
  if (err != FR_OK) {
    debug("Mount failed\n");
    sdcDisconnect(&SDCD1);
    return;
  }
  debug("Mount succeeded\n");
  fs_ready = TRUE;
}

/*
 * Card removal event.
 */
static void RemoveHandler(eventid_t id) {
  debug("Removal detected\n");
  (void)id;
  sdcDisconnect(&SDCD1);
  fs_ready = FALSE;
}

int local_fatfs_init(void) {
 // FRESULT err;

  static const evhandler_t evhndl[] = {
    InsertHandler,
    RemoveHandler
  };

  debug("entering local_fatfs_init\n");
  event_listener_t el0, el1;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   * - lwIP subsystem initialization using the default configuration.
   */
  //halInit();
  //chSysInit();

  /*
   * Activates the SDC driver 1 using default
   * configuration.
   */
  debug("doing sdcStart\n");
  sdcStart(&SDCD1, NULL);

  /*
   * Activates the card insertion monitor.
   */
  debug("doing tmr_init\n");
  tmr_init(&SDCD1);

  debug("register events\n");
  chEvtRegister(&inserted_event, &el0, 0);
  chEvtRegister(&removed_event, &el1, 1);

  init_done = true;


/*
  debug("done with init, try connect\n");
  if (sdcConnect(&SDCD1)){
    debug("Connect failed\n");
    return -1;
  }

  debug("Mounting drive\n");
  err = f_mount(&SDC_FS, "/", 1);
  if (err != FR_OK) {
    char buf[64];
    sprintf(buf, "Mount failed code %d\n", (int)err);
    debug(buf);
    sdcDisconnect(&SDCD1);
    return -1;
  }
  fs_ready = TRUE;
*/

  while (true) {
    chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
  }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
//  FRESULT local_result;
//  char debug_string[64];
  debug_enable=true;
  debug("Main entry\n");

  switch (keycode) {
    case KC_CAPS:
      if (record->event.pressed) {
        debug("Keypress, about to scan\n");
        //local_result = scan_files("/");
        //sprintf(debug_string, "Result: %d\n", (int)local_result);
        //debug(debug_string);
        local_fatfs_init();
        //debug("fatfs init\n");
      }
      break;
    case KC_C:
      scan_files("/");
      break;
  }
  return true;
}
