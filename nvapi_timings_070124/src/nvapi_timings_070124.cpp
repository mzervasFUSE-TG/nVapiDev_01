#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "SDKDDKver.h"
#include <string>
#include "nvapi.h"


NvAPI_Status ApplyCustomDisplay();
NvAPI_Status GetConnectedDisplays(NvU32* displayIds, NvU32* noDisplays);
void loadCustomDisplay(NV_CUSTOM_DISPLAY* customDisplay);


int _tmain(int argc, _TCHAR* argV[]) {
	NvAPI_Status ret = NVAPI_OK;

	//Initialize the NVAPI library
	ret = NvAPI_Initialize();
	if (ret != NVAPI_OK) {
		printf("NvAPI_Initialize failed = 0x%x\n", ret);
		return 1;
	}

	for (NvU32 q = 0; q < 50; q++) printf("/");
	printf("\n");

	ret = ApplyCustomDisplay();
	if (ret != NVAPI_OK) {
		(void)getchar();
		return 1;
	}

	for (NvU32 q = 0; q < 50; q++) printf("/");
	printf("\n");

	printf("\nCustom_Timing successfull!!\nPress any key to exit...\n");
	(void)getchar();
	return 0;
	 

}

void loadCustomDisplay(NV_CUSTOM_DISPLAY* cd) {
	cd->version = NV_CUSTOM_DISPLAY_VER;
	cd->width = 3840;
	cd->height = 2160;
	cd->depth = 32;
	cd->colorFormat = NV_FORMAT_A8R8G8B8;
	cd->srcPartition.x = 0;
	cd->srcPartition.y = 0;
	cd->srcPartition.w = 1;
	cd->srcPartition.h = 1;
	cd->xRatio = 1;
	cd->yRatio = 1;

}


NvAPI_Status GetConnectedDisplays(NvU32* displayIds, NvU32* noDisplays) {

	NvAPI_Status ret = NVAPI_OK;

	NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };
	NvU32 gpuCount = 0;
	NvU32 noDisplay = 0;

	//Enumerate physical GPUs in the system
	ret = NvAPI_EnumPhysicalGPUs(nvGPUHandle, &gpuCount);
	if (ret != NVAPI_OK) {
		printf("NvAPIEnumPhysical GPUs failed = 0x%x\n", ret);
		return ret;
	}

	for (NvU32 Count = 0; Count < gpuCount; Count++) {
		if (nvGPUHandle[Count] == NULL) {
			printf("NvAPI_EnumPhysicalGPUs returned a NULL handle at index %d\n", Count);
			return NVAPI_ERROR;
		}

		NvU32 dispIdCount = 0;
		
		ret = NvAPI_GPU_GetConnectedDisplayIds(nvGPUHandle[Count], NULL, &dispIdCount, 0);
		if (ret != NVAPI_OK) {
			printf("NvAPI_GPU_GetConnectedDisplayIds (first call) failed = 0x%x\n", ret);
			return ret;

		}

		if (dispIdCount > 0) {
			NV_GPU_DISPLAYIDS* dispIds = (NV_GPU_DISPLAYIDS*)malloc(sizeof(NV_GPU_DISPLAYIDS) * dispIdCount);
			if (!dispIds) {
				return NVAPI_OUT_OF_MEMORY;
			}

			for (NvU32 dispIndex = 0; dispIndex < dispIdCount; dispIds[dispIndex].version = NV_GPU_DISPLAYIDS_VER, ++dispIndex) {
				dispIds[dispIndex].version = NV_GPU_DISPLAYIDS_VER;
			}

			ret = NvAPI_GPU_GetConnectedDisplayIds(nvGPUHandle[Count], dispIds, &dispIdCount, 0);
			if (ret != NVAPI_OK) {
				printf("NvAPI_GPU_GetConnectedDisplayIds (second call) failed = 0x%x\n", ret);
				free(dispIds);
				return ret;
			}

			for (NvU32 dispIndex = 0; dispIndex < dispIdCount; ++dispIndex) {
				if (noDisplay >= NVAPI_MAX_DISPLAYS) {
					printf("Exceeded maximum number of diaplay IDS\n");
					free(dispIds);
					return NVAPI_ERROR;
				}
				displayIds[noDisplay] = dispIds[dispIndex].displayId;
				noDisplay++;
			}

			free(dispIds);

			
		}
	}

	*noDisplays = noDisplay;
	if (noDisplay == 0) {
		printf("No displays found.\n");
	}
	else {
		printf("Found %d displays.\n", noDisplay);
	}

	return ret;

}


NvAPI_Status ApplyCustomDisplay() {
	NvAPI_Status ret = NVAPI_OK;
	NvU32 noDisplays = 0;
	NvU32 displayIds[NVAPI_MAX_DISPLAYS] = { 0 };

	ret = GetConnectedDisplays(displayIds, &noDisplays);
	if (ret != NVAPI_OK) {
		printf("\Call to GetConnectedDisplays() failed\n");
		return ret;
	}

	printf("\nNumber of Displays in the system = %2d\n", noDisplays);

	NV_CUSTOM_DISPLAY* cd = (NV_CUSTOM_DISPLAY*)malloc(sizeof(NV_CUSTOM_DISPLAY) * noDisplays);
	if (!cd) {
		return NVAPI_OUT_OF_MEMORY;
	}

	float rr = 59.94;
	NV_TIMING_FLAG flag = { 0 };
	NV_TIMING_INPUT timing = { 0 };

	timing.version = NV_TIMING_INPUT_VER;

	for (NvU32 count = 0; count < noDisplays; count++) {
		loadCustomDisplay(&cd[count]);
		timing.height = cd[count].height;
		timing.width = cd[count].width;
		timing.rr = rr;
		timing.flag = flag;

		// Get the current timing for each display
		ret = NvAPI_DISP_GetTiming(displayIds[count], &timing, &cd[count].timing);
		if (ret != NVAPI_OK) {
			printf("NvAPI_DISP_GetTiming() failed = %d for display %d\n", ret, count);
			free(cd);
			return ret;
		}

	}

	printf("\nCustom Timing to be tried %d * %d @ %0.2f hz\n", cd[0].width, cd[0].height, rr);

	for (NvU32 count = 1; count < noDisplays; count++) {
		ret = NvAPI_DISP_TryCustomDisplay(&displayIds[count], 1, &cd[count]);
		if (ret != NVAPI_OK) {
			printf("NvAPI_DISP_TryCustomDisplay() failed for display %d = %d\n", count, ret);
		}
		else {
			printf("Custom display applied to display %d\n", count);
		}

		ret = NvAPI_DISP_SaveCustomDisplay(&displayIds[count], 1, true, true);
		if (ret != NVAPI_OK) {
			printf("NvAPI_DISP_SAveCustomDisplay() failed for display %d = %d\n", count, ret);
		}
		else {
			printf("Custom display saved for display %d\n", count);
		}

	}

	free(cd);
	return ret;


}

