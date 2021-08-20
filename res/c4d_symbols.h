#ifndef HLL_C4D_SYMBOLS_H__
#define HLL_C4D_SYMBOLS_H__

enum
{
	_DUMMY_ELEMENT_ = 10000,

	DLG_HLL,
	
	// Disabled during listen
	ETXT_HOSTNAME,
	ETXT_PORT,

	// Clickable/Editable UI gadgets
	BTN_LISTEN,
	BTN_SETCAMERA,
	BTN_MAPPING,
	LV_CLIENTS,
	BTN_COPY_URI,
	CHK_CHECK_FOR_UPDATES,
	ETXT_POLLRATE,
	BTN_CHECK_FOR_UPDATE_NOW,
	BTN_DONATE_PAYPAL,
	CHK_USE_GLOBAL_POSROT,
	VEC_ORIENTATION_X,
	VEC_ORIENTATION_Y,
	VEC_ORIENTATION_Z,
	_HLL_UI_GADGETS_END_,
	
	// Non-clickable/Editable UI gadgets
	IMG_BANNER,
	
	// Strings changed by the program
	TXT_CAMERANAME,
	TXT_STATUS,
	TXT_FTIME,
	TXT_AVGTIME,
	TXT_MAXTIME,
	
	// Static UI Strings
	STR_HOST,
	STR_PORT,
	STR_START_LISTEN,
	STR_STOP_LISTEN,
	STR_CLIENT,
	STR_DISCONNECT,
	STR_NO_CAMERA,
	STR_TARGET_CSGO,
	STR_TARGET_CINEMA4D,
	STR_UPDATE,
	STR_CHECK_FOR_UPDATES,
	STR_CHECK_NOW,
	STR_HELP_STRING,
	STR_SUPPORT_HLL,
	STR_DONATE_PAYPAL,
	STR_ASK_AUTO_UPDATE,
	STR_VERSION_UPTODATE,
	STR_VERSION_OUTDATED,
	
	// Statusbar/Log Strings
	STR_NO_REPORT,
	STR_STARTING_SERVER,
	STR_UNEXPECTED_ERROR,
	STR_CAM_NO_SELECT,
	STR_NOT_A_CAMERA,
	STR_ACTIVE_CLIENT,
	STR_LOADED_HLL,
	STR_FAIL_LOAD_HLL,
	STR_USER_SETTINGS_ERROR,
	
	// Server Messages
	STR_SERVER_CONNECT,
	STR_SERVER_CLIENT_DISCONNECT,
	STR_SERVER_DISCONNECT,
	STR_SERVER_RENAME,
	STR_SERVER_LISTEN_SUCCESS,
	STR_SERVER_LISTEN_FAILED,
	STR_SERVER_LISTEN_CLOSED,
	STR_SERVER_BAD_DATASTART,
	STR_SERVER_BAD_DATASTOP,
	
	// Client Messages
	STR_CLIENT_CONNECT,
	STR_CLIENT_DISCONNECT,
	STR_CLIENT_RENAME,
	STR_CLIENT_BAD_DATASTART,
	STR_CLIENT_LIVE_TRANSMIT,
	STR_CLIENT_LIVE_RECEIVE,
	STR_CLIENT_INACTIVE
};

#define _HLL_UI_GADGETS_BEGIN_ BTN_LISTEN


#endif // !HLL_C4D_SYMBOLS_H__
