#include <psp2/io/fcntl.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/message_dialog.h>

#include <taihen.h>

#include <vita2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf

#define SCE_PHOTOIMPORT_DIALOG_CATEGORY_DEFAULT			(0x00000007U)
#define SCE_PHOTOIMPORT_DIALOG_CATEGORY_ALBUM_ALL		(0x00000001U)
#define SCE_PHOTOIMPORT_DIALOG_CATEGORY_ALBUM_CAMERA		(0x00000002U)
#define SCE_PHOTOIMPORT_DIALOG_CATEGORY_ALBUM_SCREENSHOT	(0x00000004U)

#define SCE_PHOTOIMPORT_DIALOG_MAX_FS_PATH			(1024)

#define SCE_PHOTOIMPORT_DIALOG_MAX_PHOTO_TITLE_LENGTH		(64)

#define SCE_PHOTOIMPORT_DIALOG_MAX_PHOTO_TITLE_SIZE	(SCE_PHOTOIMPORT_DIALOG_MAX_PHOTO_TITLE_LENGTH*4)

#define SCE_PHOTOIMPORT_DIALOG_MAX_ITEM_NUM		(1)

typedef enum ScePhotoImportDialogFormatType {
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_UNKNOWN = 0,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_JPEG,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_PNG,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_GIF,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_BMP,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_TIFF,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_MPO
} ScePhotoImportDialogFormatType;

typedef SceInt32 ScePhotoImportDialogMode;
#define SCE_PHOTOIMPORT_DIALOG_MODE_DEFAULT		(0)

typedef enum ScePhotoImportDialogOrientation {
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_UNKNOWN = 0,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_TOP_LEFT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_TOP_RIGHT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_BOTTOM_RIGHT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_BOTTOM_LEFT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_LEFT_TOP,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_RIGHT_TOP,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_RIGHT_BOTTOM,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_LEFT_BOTTOM
} ScePhotoImportDialogOrientation;

typedef struct ScePhotoImportDialogFileDataSub {
	SceUInt32 width;
	SceUInt32 height;
	ScePhotoImportDialogFormatType format;
	ScePhotoImportDialogOrientation orientation;
	SceChar8 reserved[32];
} ScePhotoImportDialogFileDataSub;

typedef struct ScePhotoImportDialogFileData {
	SceChar8 fileName[SCE_PHOTOIMPORT_DIALOG_MAX_FS_PATH];
	SceChar8 photoTitle[SCE_PHOTOIMPORT_DIALOG_MAX_PHOTO_TITLE_SIZE];
	SceChar8 reserved[32];
} ScePhotoImportDialogFileData;

typedef struct ScePhotoImportDialogItemData {
	ScePhotoImportDialogFileData fileData;
	ScePhotoImportDialogFileDataSub dataSub;
	SceChar8 reserved[32];
} ScePhotoImportDialogItemData;

typedef struct ScePhotoImportDialogResult {
	SceInt32 result;
	SceUInt32 importedItemNum;
	SceChar8 reserved[32];
} ScePhotoImportDialogResult;

typedef struct ScePhotoImportDialogParam {
	SceUInt32 sdkVersion;
	SceCommonDialogParam commonParam;
	ScePhotoImportDialogMode mode;
	SceUInt32 visibleCategory;
	SceUInt32 itemCount;
	ScePhotoImportDialogItemData *itemData;
	SceChar8 reserved[32];
} ScePhotoImportDialogParam;

int scePhotoImportDialogInit( const ScePhotoImportDialogParam *param );
SceCommonDialogStatus scePhotoImportDialogGetStatus( void );
int scePhotoImportDialogGetResult( ScePhotoImportDialogResult* result );
int scePhotoImportDialogTerm( void );
int scePhotoImportDialogAbort( void );

static inline void scePhotoImportDialogParamInit( ScePhotoImportDialogParam *param ){

	memset( param, 0x0, sizeof(ScePhotoImportDialogParam) );
	_sceCommonDialogSetMagicNumber( &param->commonParam );
	param->sdkVersion = 0x03150021;
}

static ScePhotoImportDialogItemData s_itemData[SCE_PHOTOIMPORT_DIALOG_MAX_ITEM_NUM];

const char *get_format_type_string(int format_type) {
	static char *FormatType[] = {
		"unknown",
		"jpeg",
		"png",
		"gif",
		"bmp",
		"tiff",
		"mp0",
	};
	return FormatType[format_type];
}

#define UpdateVita2d() ({ \
	vita2d_end_drawing(); \
	vita2d_common_dialog_update(); \
	vita2d_swap_buffers(); \
	sceDisplayWaitVblankStart(); \
	vita2d_start_drawing(); \
	vita2d_clear_screen(); \
})

static int SelectPhotoImage(){
	int ret;

	ScePhotoImportDialogParam pidParam;
	scePhotoImportDialogParamInit(&pidParam);

	pidParam.mode = SCE_PHOTOIMPORT_DIALOG_MODE_DEFAULT;
	pidParam.itemData = s_itemData;
	pidParam.visibleCategory = SCE_PHOTOIMPORT_DIALOG_CATEGORY_DEFAULT;
	pidParam.itemCount = 1;

	SceCommonDialogColor BgColor;

	BgColor.r = 0;
	BgColor.g = 0;
	BgColor.b = 0;
	BgColor.a = 0xFF;

	pidParam.commonParam.bgColor = (SceCommonDialogColor*)&BgColor;

	ret = scePhotoImportDialogInit(&pidParam);

	if (ret < 0) {
		printf("scePhotoImportDialogInit Failed : 0x%X\n", ret);
		return ret;
	}

	while (1) {

		UpdateVita2d();

		ret = scePhotoImportDialogGetStatus();

		switch (ret) {
			case SCE_COMMON_DIALOG_STATUS_FINISHED:
				break;
			case SCE_COMMON_DIALOG_STATUS_NONE:
			case SCE_COMMON_DIALOG_STATUS_RUNNING:
				continue;
			default:
				printf("INFO: GetStatus : 0x%08X\n", ret);
		}

		ScePhotoImportDialogResult result;

		memset( &result, 0x0, sizeof(ScePhotoImportDialogResult) );
		scePhotoImportDialogGetResult(&result);


		if (result.result == SCE_COMMON_DIALOG_RESULT_OK) {
			for (int i = 0; i < result.importedItemNum && i < SCE_PHOTOIMPORT_DIALOG_MAX_ITEM_NUM; ++i) {

				printf("[info] path   : %s\n", s_itemData[i].fileData.fileName);
				printf("[info] format : %s\n", get_format_type_string(s_itemData[i].dataSub.format) );

			}
		}else if (result.result == SCE_COMMON_DIALOG_RESULT_USER_CANCELED){

			printf("[info] User canceled.\n");

		}else if (result.result == SCE_COMMON_DIALOG_RESULT_ABORTED){

			printf("[info] Aborted.\n");

		}

		ret = result.result;

		scePhotoImportDialogTerm();

		break;

	}

	return ret;
}


int main(int argc, char *argv[]) {

	vita2d_init();
	psvDebugScreenInit();

	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);

	SceCommonDialogConfigParam config;
	sceCommonDialogConfigParamInit(&config);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&config.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&config.enterButtonAssign);
	sceCommonDialogSetConfigParam(&config);

	int ret = 0;
	int count[3];
	vita2d_texture *s_image = NULL;

	printf("CustomBootSplash Image Changer\n\n");
	printf("Please press one of the keys\n\n");

	sceAppUtilPhotoMount();

	while (1) {

		SceCtrlData ctrl;
		do {
			sceCtrlPeekBufferPositive(0, &ctrl, 1);
		} while (ctrl.buttons == 0);

		ret = SelectPhotoImage();
		psvDebugScreenSet();

		if(ret == SCE_COMMON_DIALOG_RESULT_USER_CANCELED){

			printf("[info] Select 960x544 Photo Image Please\n\n");

			continue;

		}else if(ret < 0){

			printf("[error] SelectPhotoImage : 0x%X\n\n");

			continue;
		}

		if(s_itemData[0].dataSub.width != 960 || s_itemData[0].dataSub.height != 544){

			printf("[info] Unsupported image size : %dx%d\n\n", s_itemData[0].dataSub.width, s_itemData[0].dataSub.height );

			printf("[info] Select 960x544 Photo Image Please\n\n");

			continue;
		}

		if(s_itemData[0].dataSub.format == SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_BMP){

			s_image = vita2d_load_BMP_file((const char *)s_itemData[0].fileData.fileName);

		}else if(s_itemData[0].dataSub.format == SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_PNG){

			s_image = vita2d_load_PNG_file((const char *)s_itemData[0].fileData.fileName);

		}else if(s_itemData[0].dataSub.format == SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_JPEG){

			s_image = vita2d_load_JPEG_file((const char *)s_itemData[0].fileData.fileName);

		}else{

			printf("[info] Unsupported format.\n\n");

			continue;
		}

		if( (s_image->data_UID < 0) || (s_image->palette_UID < 0) || (s_image->depth_UID < 0) ){

			printf("[error] image load failed : 0x%X\n\n", (s_image->data_UID < 0) ? s_image->data_UID : (s_image->palette_UID < 0) ? s_image->palette_UID : (s_image->depth_UID < 0) ? s_image->depth_UID : 0 );

			continue;
		}

		if( s_image == NULL ){

			printf("[error] image load failed.\n\n");

			continue;
		}

		vita2d_draw_texture(s_image, 0, 0);
		sceKernelDelayThread(3*1000*1000);

		vita2d_end_drawing();
		vita2d_swap_buffers();
		sceDisplayWaitVblankStart();
		sceDisplayWaitVblankStart();
		vita2d_start_drawing();

		SceDisplayFrameBuf get_framebuf;
		get_framebuf.size = sizeof(get_framebuf);

		char *new_image_p = NULL;

		SceUID new_image_work = sceKernelAllocMemBlock("new_image_work", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 0x200000, NULL);
		sceKernelGetMemBlockBase(new_image_work, (void**)&new_image_p);

		sceDisplayGetFrameBuf(&get_framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

		memset(count, 0, sizeof(count) );

		for(int f=0;f<(544);f++){
			memcpy(new_image_p+count[0], get_framebuf.base+count[0]+count[1], 960*4);

			count[0] += 960*4;
			count[1] += ((get_framebuf.pitch-960)*4);
		}

		SceUID fd = sceIoOpen("ur0:tai/boot_splash.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);

		sceIoWrite(fd, new_image_p, (960*544*4) );

		sceIoClose(fd);

		sceKernelFreeMemBlock(new_image_work);

		sceKernelDelayThread(2*1000*1000);

		break;
	}

	sceAppUtilPhotoUmount();

	return 0;
}
