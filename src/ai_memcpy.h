#ifndef AI_MEMORY_COPY_H_
#define AI_MEMORY_COPY_H_

// #include "ai.h"
// #include "ai_array.h"
// #include "ai_memory.h"
// #include "aiructures.h"
// #include "ai_types.h"

// Dimensions, 1D/2D/3D/2D2A/A22D/A2A/memset1D/memset2D/memset3D(3D is 2D fact)
typedef enum {
	aiMemcpy1D = 0, // 1D-to-1D
	aiMemcpy2D = 1, // 2D-to-2D
	aiMemcpy3D = 2, // 3D-to-3D
	aiMemcpy2D2A = 3, // 2D-to-A
	aiMemcpyA22D = 4, // A-to-2D
	aiMemcpyA2A = 5, // A-to-A
	aiMemset1D = 30, // set-1D
	aiMemset2D = 31, // set-2D
	aiMemset3D = 32 // set-3D
} aiMemcpyDimension;

typedef enum {
	aiMemcpyHostToHost = 0, ///< Host-to-Host Copy
	aiMemcpyHostToDevice = 1, ///< Host-to-Device Copy
	aiMemcpyDeviceToHost = 2, ///< Device-to-Host Copy
	aiMemcpyDeviceToDevice = 3, ///< Device-to-Device Copy
	aiMemcpyDefault = 4 ///< Runtime will automatically determine copy-kind based on virtual addresses.
} aiMemcpyKind;

/** Default array allocation flag.*/
#define aiArrayDefault 0x00
#define aiArrayLayered 0x01
#define aiArraySurfaceLoadStore 0x02
#define aiArrayCubemap 0x04
#define aiArrayTextureGather 0x08

typedef enum aiChannelFormatKind_enum {
	aiChannelFormatKindSigned = 0,
	aiChannelFormatKindUnsigned = 1,
	aiChannelFormatKindFloat = 2,
	aiChannelFormatKindNone = 3
} aiChannelFormatKind;

typedef enum aiArrayFormat_enum {
	AI_AD_FORMAT_UNSIGNED_INT8 = 0x01,
	AI_AD_FORMAT_UNSIGNED_INT16 = 0x02,
	AI_AD_FORMAT_UNSIGNED_INT32 = 0x03,
	AI_AD_FORMAT_SIGNED_INT8 = 0x08,
	AI_AD_FORMAT_SIGNED_INT16 = 0x09,
	AI_AD_FORMAT_SIGNED_INT32 = 0x0a,
	AI_AD_FORMAT_HALF = 0x10,
	AI_AD_FORMAT_FLOAT = 0x20
} aiArrayFormat;

typedef struct {
	size_t x;
	size_t y;
	size_t z;
} aiPos;
typedef struct {
	void *ptr;
	size_t pitch;
	size_t xsize;
	size_t ysize;
} aiPitchInfo;

typedef struct {
	size_t width; // Width in elements when referring to array memory, in bytes when referring to linear memory
	size_t height;
	size_t depth;
} aiExtent;

typedef struct {
	int x;
	int y;
	int z;
	int w;
	aiChannelFormatKind f;
} aiChannelFormatDesc;

typedef struct {
	aiCtx *pContext;
	void *hImageObj; // device mem struct
	aiChannelFormatDesc sDesc; // pixel: Channel bits & datakind
	unsigned int uiType;
	unsigned int uiWidth;
	unsigned int uiHeight;
	unsigned int uiDepth;
	aiArrayFormat eFormat;
	unsigned int uiNumChannels; // pixel: eg.rgb =3
	bool bIsDrv;
	unsigned int uiTextureType;
	unsigned int flags;
} aiArray;

typedef struct {
	size_t width;
	size_t height;
	size_t depth;
	aiArrayFormat eFormat;
	unsigned int uiNumChannels;
	unsigned int flags;
} aiArray3DDescriptor;

typedef struct {
	aiArray *srcArray;
	aiPos srcPos;
	aiPitchInfo srcPtrInfo;
	aiArray *dstArray;
	aiPos dstPos;
	aiPitchInfo dstPtrInfo;
	aiExtent extent;
	aiMemcpyKind kind;
	int value;
	size_t valueSize;
} aiMemcpy3DParams;

typedef struct aiCoord3D {
	size_t c[3];
} aiCoord3D;
typedef struct aiBufferRect {
	size_t mipMapLevel;
	size_t rowPitch; //!< Calculated row pitch for the buffer rect
	size_t slicePitch; //!< Calculated slice pitch for the buffer rect
	// size_t start;       //!< Start offset for the copy region
	// size_t end;         //!< Relative end offset from start for the copy region
} aiBufferRect;

//DeviceToHost: psSrcBuffer->psSrcShadowGtt->pvDstPtr
//HostToDevice: pvSrcPtr->psDstShadowGtt->psDstBuffer
//DeviceToDevice: psSrcBuffer->psDstBuffer (how to use shadowgtt?)
typedef struct aiMemCpyCmd3D {
	union {
		aiMemory *psSrcBuffer; // dev mem
		aiMemory *psDstShadowGtt; // gtt mem, Transfer from pvSrcPtr
	};
	union {
		aiMemory *psDstBuffer; // dev mem
		aiMemory *psSrcShadowGtt; // gtt mem, Transfer to pvDstPtr
	};
	void *pvSrcPtr; // host mem
	void *pvDstPtr; // host mem

	size_t srcOffset;
	size_t dstOffset;

	aiCoord3D srcOrigin; //!< Origin of the source region. ==offset
	aiCoord3D dstOrigin; //!< Origin of the destination region. ==offset
	aiCoord3D size; //!< Size of the region to copy.

	aiBufferRect srcRect; //!< Source buffer rectangle information
	aiBufferRect dstRect; //!< Destination buffer rectangle information

	int setValue;
	size_t valueSize;
} aiMemCpyCmd3D;

typedef struct aiDmaCmd {
	aiMemCpyCmd3D memcpy3D;
	enum aiMemcpyKind_enum kind; // api do
	enum aiMemcpyDimension_enum dimens;
	aiDevice *dstDevice, *srcDevice; // peer, api do
	bool bBlocking;
} aiDmaCmd;

struct IDmaCopy {
	aiMemcpy3DParams m_aiParms3D; // callback func do
	aiStream *mream; // api do

	aiDmaCmd m_dmaCmd; // memSubmitCmd do,then submit this to stream

	aiResult (*memcpyGenCmd)(struct IDmaCopy *dmaCopy, void *dst, const void *src, size_t count);
	aiResult (*memcpyGenCmd2D)(struct IDmaCopy *dmaCopy, void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height);
	aiResult (*memcpyGenCmd2D2A)(struct IDmaCopy *dmaCopy, aiArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch,
				     size_t width, size_t height);
	aiResult (*memcpyGenCmdA2A)(struct IDmaCopy *dmaCopy, aiArray *dst, size_t wOffsetDst, size_t hOffsetDst, const aiArray *src,
				    size_t wOffsetSrc, size_t hOffsetSrc, size_t width, size_t height);
	aiResult (*memcpyGenCmdA22D)(struct IDmaCopy *dmaCopy, void *dst, size_t dpitch, const aiArray *src, size_t wOffset, size_t hOffset,
				     size_t width, size_t height);
	aiResult (*memcpyGenCmd3D)(struct IDmaCopy *dmaCopy, const aiMemcpy3DParams *p);

	aiResult (*memsetGenCmd)(struct IDmaCopy *dmaCopy, void *devPtr, int value, size_t valueSize, size_t count);
	aiResult (*memsetGenCmd2D)(struct IDmaCopy *dmaCopy, void *devPtr, size_t pitch, int value, size_t valueSize, size_t width, size_t height);
	aiResult (*memsetGenCmd3D)(struct IDmaCopy *dmaCopy, aiPitchInfo pitchedPtrInfo, int value, size_t valueSize, aiExtent extent);

	aiResult (*memcpyFromSymbol)(struct IDmaCopy *dmaCopy, void *dst, const void *symbol, size_t count, size_t offset);
	aiResult (*memcpyToSymbol)(struct IDmaCopy *dmaCopy, const void *symbol, const void *src, size_t count, size_t offset);

	// aiResult (*memGenDmaCmd)(aiDmaCmd* dmaCmd, aiMemcpy3DPeerParms* aiParms3D);
	aiResult (*memSubmitDmaCmd)(struct IDmaCopy *dmaCopy);
};

// device default NULL, mean the current stream device
aiResult aiDmaCopyInit(struct IDmaCopy *dmaCopy, enum aiMemcpyKind_enum kind, aiStream *stream, aiDevice *dstDevice, aiDevice *srcDevice); // api do

// some facts:
// 1.api's height will not be 0, otherwise it will not be copied; but array's height can be 0, indicating 1D
// 2.array has no pitch
// 3.pitchPtr or ptr has no offset (referring to parameters)

// 4.aimemory* devmem is the same as void* devptr, and devptr's offset only needs devptr-devmem (subtract, this must be the very beginning)

///// .c
static inline void memCmdSet(struct IDmaCopy *dmaCopy, enum aiMemcpyDimension_enum dimens)
{
	dmaCopy->m_dmaCmd.dimens = dimens;
	dmaCopy->m_dmaCmd.bBlocking = (!dmaCopy->mream) ? true : false;

	if (!dmaCopy->m_aiParms3D.extent.height)
		dmaCopy->m_aiParms3D.extent.height = 1; // least 1
	if (!dmaCopy->m_aiParms3D.extent.depth)
		dmaCopy->m_aiParms3D.extent.depth = 1; // least 1

	if (dmaCopy->m_aiParms3D.dstPtrInfo.ptr && !dmaCopy->m_aiParms3D.dstPtrInfo.pitch)
		dmaCopy->m_aiParms3D.dstPtrInfo.pitch = dmaCopy->m_aiParms3D.extent.width;
	if (dmaCopy->m_aiParms3D.srcPtrInfo.ptr && !dmaCopy->m_aiParms3D.srcPtrInfo.pitch)
		dmaCopy->m_aiParms3D.srcPtrInfo.pitch = dmaCopy->m_aiParms3D.extent.width;
}

static aiResult mallocShadowGtt(aiDmaCmd *dmaCmd)
{
	aiMemCpyCmd3D *psCpy = &dmaCmd->memcpy3D;
	size_t sizeBytes = 0;

	if (dmaCmd->kind == aiMemcpyHostToDevice && psCpy->pvSrcPtr && !psCpy->psSrcBuffer) {
		sizeBytes = psCpy->dstRect.slicePitch * psCpy->size.c[2];
		return aiAllocDeviceMemWrapper(&psCpy->psDstShadowGtt, sizeBytes, dmaCmd->srcDevice, AI_HEAPTYPE_GENERAL, aiDefaultMemoryFlagGTT());
	}
	if (dmaCmd->kind == aiMemcpyDeviceToHost && psCpy->pvDstPtr && !psCpy->psDstBuffer) {
		sizeBytes = psCpy->srcRect.slicePitch * psCpy->size.c[2];
		return aiAllocDeviceMemWrapper(&psCpy->psSrcShadowGtt, sizeBytes, dmaCmd->dstDevice, AI_HEAPTYPE_GENERAL, aiDefaultMemoryFlagGTT());
	}

	AI_DPF_ERROR("wrong with srcptr or dstptr");
	return AI_ERROR_INVALID_VALUE;
}

static aiResult dmaCmdCpyHostWithGtt(aiMemCpyCmd3D *memCpy3D, void *pvDst, void *pvSrc)
{
	aiCoord3D size = memCpy3D->size;
	aiCoord3D originSrc = memCpy3D->srcOrigin;
	aiCoord3D originDst = memCpy3D->dstOrigin;
	aiBufferRect rectSrc = memCpy3D->srcRect;
	aiBufferRect rectDst = memCpy3D->dstRect;
	int ui32SrcOffset = 0, ui32DstOffset = 0;

	for (size_t z = 0; z < size.c[2]; ++z) {
		for (size_t y = 0; y < size.c[1]; ++y) {
			ui32SrcOffset = originSrc.c[0] + ((originSrc.c[1] + y) * rectSrc.rowPitch) + ((originSrc.c[2] + z) * rectSrc.slicePitch);
			ui32DstOffset = originDst.c[0] + ((originDst.c[1] + y) * rectDst.rowPitch) + ((originDst.c[2] + z) * rectDst.slicePitch);
			memcpy(((char *)pvDst + ui32DstOffset), ((char *)pvSrc + ui32SrcOffset), size.c[0]);
		}
	}

	return AI_SUCCESS;
}

static void transferHostToGtt(aiDmaCmd *dmaCmd)
{
	aiMemCpyCmd3D *psCpy = &dmaCmd->memcpy3D;
	void *pvDstShadowGttPtr = aiMemoryGetHostPtr(psCpy->psDstShadowGtt);

	dmaCmdCpyHostWithGtt(psCpy, pvDstShadowGttPtr, psCpy->pvSrcPtr);
}

static void transferGttToHost(aiCommand *psCommand)
{
	aiDmaCopyCommand *psDmaCopyCmd = (aiDmaCopyCommand *)psCommand;
	aiMemCpyCmd3D *psCpy = &psDmaCopyCmd->dmaCmd.memcpy3D;

	void *pvSrcShadowGttPtr = aiMemoryGetHostPtr(psCpy->psSrcShadowGtt);
	dmaCmdCpyHostWithGtt(psCpy, psCpy->pvDstPtr, pvSrcShadowGttPtr);
}

static aiResult getSymbolInfo(const void *symbol, size_t sizeBytes, size_t offset, void **device_ptr)
{
	size_t *sym_size = nullptr;

	//t odo: get this from module
	// HIP_RETURN_ONFAIL(PlatformState::instance().getStatGlobalVar(symbol, ihipGetDevice(), device_ptr, sym_size));
	(void)symbol;

	if ((offset + sizeBytes) > *sym_size)
		return AI_ERROR_INVALID_VALUE;

	*device_ptr = (unsigned char *)*device_ptr + offset;

	return AI_SUCCESS;
}

static aiResult memcpyGenCmd(struct IDmaCopy *dmaCopy, void *dst, const void *src, size_t count)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstPtrInfo.ptr = dst;
	aiParms3D->srcPtrInfo.ptr = (void *)src;

	aiParms3D->extent.width = count;

	memCmdSet(dmaCopy, aiMemcpy1D);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}

static aiResult memcpyFromSymbol(struct IDmaCopy *dmaCopy, void *dst, const void *symbol, size_t count, size_t offset)
{
	void *devicePtr = nullptr;

	if (getSymbolInfo(symbol, count, offset, &devicePtr) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;

	return memcpyGenCmd(dmaCopy, dst, devicePtr, count);
}

static aiResult memcpyToSymbol(struct IDmaCopy *dmaCopy, const void *symbol, const void *src, size_t count, size_t offset)
{
	void *devicePtr = nullptr;

	if (getSymbolInfo(symbol, count, offset, &devicePtr) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;

	return memcpyGenCmd(dmaCopy, devicePtr, src, count);
}

static aiResult memcpyGenCmd2D(struct IDmaCopy *dmaCopy, void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstPtrInfo.ptr = dst;
	aiParms3D->dstPtrInfo.pitch = dpitch;
	aiParms3D->srcPtrInfo.ptr = (void *)src;
	aiParms3D->srcPtrInfo.pitch = spitch;

	aiParms3D->extent.width = width;
	aiParms3D->extent.height = height;

	memCmdSet(dmaCopy, aiMemcpy2D);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}

static aiResult mememcpyGenCmd2D2A(struct IDmaCopy *dmaCopy, aiArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch,
				   size_t width, size_t height)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstArray = dst;
	aiParms3D->dstPos.x = wOffset;
	aiParms3D->dstPos.y = hOffset;
	aiParms3D->srcPtrInfo.ptr = (void *)src;
	aiParms3D->srcPtrInfo.pitch = spitch;

	aiParms3D->extent.width = width;
	aiParms3D->extent.height = height;

	memCmdSet(dmaCopy, aiMemcpy2D2A);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}

static aiResult memcpyGenCmdA2A(struct IDmaCopy *dmaCopy, aiArray *dst, size_t wOffsetDst, size_t hOffsetDst, const aiArray *src, size_t wOffsetSrc,
				size_t hOffsetSrc, size_t width, size_t height)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstArray = dst;
	aiParms3D->dstPos.x = wOffsetDst;
	aiParms3D->dstPos.y = hOffsetDst;
	aiParms3D->srcArray = (aiArray *)src;
	aiParms3D->srcPos.x = wOffsetSrc;
	aiParms3D->srcPos.y = hOffsetSrc;

	aiParms3D->extent.width = width;
	aiParms3D->extent.height = height;

	memCmdSet(dmaCopy, aiMemcpyA2A);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}

static aiResult memcpyGenCmdA22D(struct IDmaCopy *dmaCopy, void *dst, size_t dpitch, const aiArray *src, size_t wOffset, size_t hOffset, size_t width,
				 size_t height)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstPtrInfo.ptr = dst;
	aiParms3D->dstPtrInfo.pitch = dpitch;
	aiParms3D->srcArray = (aiArray *)src;
	aiParms3D->srcPos.x = wOffset;
	aiParms3D->srcPos.y = hOffset;

	aiParms3D->extent.width = width;
	aiParms3D->extent.height = height;

	memCmdSet(dmaCopy, aiMemcpyA22D);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}

static aiResult memcpyGenCmd3D(struct IDmaCopy *dmaCopy, const aiMemcpy3DParams *p)
{
	dmaCopy->m_aiParms3D = *p;

	memCmdSet(dmaCopy, aiMemcpy3D);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}
static aiResult memsetGenCmd(struct IDmaCopy *dmaCopy, void *devPtr, int value, size_t valueSize, size_t count)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstPtrInfo.ptr = devPtr;
	aiParms3D->value = value;
	aiParms3D->valueSize = valueSize;
	aiParms3D->extent.width = count;

	memCmdSet(dmaCopy, aiMemset1D);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}
static aiResult memsetGenCmd2D(struct IDmaCopy *dmaCopy, void *devPtr, size_t pitch, int value, size_t valueSize, size_t width, size_t height)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstPtrInfo.ptr = devPtr;
	aiParms3D->dstPtrInfo.pitch = pitch;
	aiParms3D->value = value;
	aiParms3D->valueSize = valueSize;

	aiParms3D->extent.width = width;
	aiParms3D->extent.height = height;

	memCmdSet(dmaCopy, aiMemset2D);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}
static aiResult memsetGenCmd3D(struct IDmaCopy *dmaCopy, aiPitchInfo pitchedPtrInfo, int value, size_t valueSize, aiExtent extent)
{
	aiMemcpy3DParams *aiParms3D = &dmaCopy->m_aiParms3D;

	aiParms3D->dstPtrInfo = pitchedPtrInfo;
	aiParms3D->value = value;
	aiParms3D->valueSize = valueSize;

	aiParms3D->extent = extent;

	memCmdSet(dmaCopy, aiMemset3D);

	return dmaCopy->memSubmitDmaCmd(dmaCopy);
}

static aiResult iMemcpy3D_ValidatekindStream(aiMemcpyKind kind, aiStream *stream)
{
	if (kind < aiMemcpyHostToHost || kind > aiMemcpyDefault)
		return AI_ERROR_INVALID_VALUE;

	if (!aiStreamIsValid(stream)) {
		return AI_ERROR_INVALID_VALUE;
	}

	return AI_SUCCESS;
}

static aiResult iMemcpy3D_ValidateArray(aiArray *arr, const aiPos *pos, const aiExtent *extent)
{
	size_t FormatSize = aiArrayGetElementSize(arr);

	if ((extent->width + pos->x) > (arr->uiWidth * FormatSize)) {
		return AI_ERROR_INVALID_VALUE;
	}
	if (arr->uiHeight == 0) {
		// 1D hipArray
		if (extent->height + pos->y > 1)
			return AI_ERROR_INVALID_VALUE;
	} else if ((extent->height + pos->y) > (arr->uiHeight)) {
		// 2D hipArray
		return AI_ERROR_INVALID_VALUE;
	}

	return AI_SUCCESS;
}

static aiResult iMemcpy3D_validatePtr(const aiPitchInfo *pitchedPtrInfo, const aiExtent *extent)
{
	if (pitchedPtrInfo->pitch == 0 || pitchedPtrInfo->pitch < extent->width) {
		return AI_ERROR_INVALID_VALUE;
	}

	return AI_SUCCESS;
}

static aiResult iMemcpy3D_validateParms(const aiMemcpy3DParams *p)
{
	// Passing more than one non-zero source or destination will cause aiMemcpy3D() to return an error.
	//((!p->srcArray) && (!p->srcPtr.ptr)) right if memset
	if (!p || (p->srcArray && p->srcPtrInfo.ptr) || (p->dstArray && p->dstPtrInfo.ptr) || (!p->dstArray && !p->dstPtrInfo.ptr))
		return AI_ERROR_INVALID_VALUE;

	// array is valid
	if (((p->srcArray) && (p->dstArray)) && (aiArrayGetElementSize(p->dstArray) != aiArrayGetElementSize(p->dstArray)))
		return AI_ERROR_INVALID_VALUE;
	if (p->srcArray && iMemcpy3D_ValidateArray(p->srcArray, &p->srcPos, &p->extent) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;
	if (p->dstArray && iMemcpy3D_ValidateArray(p->dstArray, &p->dstPos, &p->extent) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;

	// ptr is valid
	if (p->dstPtrInfo.ptr && iMemcpy3D_validatePtr(&p->dstPtrInfo, &p->extent) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;
	if (p->srcPtrInfo.ptr && iMemcpy3D_validatePtr(&p->srcPtrInfo, &p->extent) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;

	return AI_SUCCESS;
}

// judge the legality of parameters and generate dma command
static aiResult memGenDmaCmd(aiDmaCmd *dmaCmd, aiMemcpy3DParams *aiParms3D)
{
	if (iMemcpy3D_validateParms(aiParms3D) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;

	aiMemory *pDevMemDst = nullptr, *pDevMemSrc = nullptr;
	void *pCpuDst = nullptr, *pCpuSrc = nullptr;
	size_t offsetDst = 0, offsetSrc = 0;
	size_t sizeBytes = aiParms3D->extent.width * aiParms3D->extent.height * aiParms3D->extent.depth;

	dmaCmd->memcpy3D.size.c[0] = aiParms3D->extent.width;
	dmaCmd->memcpy3D.size.c[1] = aiParms3D->extent.height;
	dmaCmd->memcpy3D.size.c[2] = aiParms3D->extent.depth;
	dmaCmd->memcpy3D.dstOrigin.c[0] = aiParms3D->dstPos.x;
	dmaCmd->memcpy3D.dstOrigin.c[1] = aiParms3D->dstPos.y;
	dmaCmd->memcpy3D.dstOrigin.c[2] = aiParms3D->dstPos.z;
	dmaCmd->memcpy3D.srcOrigin.c[0] = aiParms3D->srcPos.x;
	dmaCmd->memcpy3D.srcOrigin.c[1] = aiParms3D->srcPos.y;
	dmaCmd->memcpy3D.srcOrigin.c[2] = aiParms3D->srcPos.z;
	dmaCmd->memcpy3D.dstRect.rowPitch = aiParms3D->dstPtrInfo.ptr ? aiParms3D->dstPtrInfo.pitch : aiParms3D->extent.width; // default
	dmaCmd->memcpy3D.srcRect.rowPitch = aiParms3D->srcPtrInfo.ptr ? aiParms3D->srcPtrInfo.pitch : aiParms3D->extent.width; // default

	pDevMemDst = aiGetMemoryObjectOffset(dmaCmd->dstDevice, aiParms3D->dstPtrInfo.ptr, &offsetDst);
	pDevMemSrc = aiGetMemoryObjectOffset(dmaCmd->srcDevice, aiParms3D->srcPtrInfo.ptr, &offsetSrc);
	if (!pDevMemDst) {
		if (aiParms3D->dstArray) {
			pDevMemDst = (aiMemory *)((aiImage *)aiParms3D->dstArray->hImageObj)->psImageMem;
			dmaCmd->memcpy3D.dstRect.rowPitch = aiParms3D->extent.width; // array has no pitch
		} else {
			pCpuDst = aiParms3D->dstPtrInfo.ptr;
			dmaCmd->memcpy3D.dstRect.rowPitch = aiParms3D->dstPtrInfo.pitch;
		}
	}
	if (!pDevMemSrc) {
		if (aiParms3D->srcArray) {
			pDevMemSrc = (aiMemory *)((aiImage *)aiParms3D->srcArray->hImageObj)->psImageMem;
			dmaCmd->memcpy3D.srcRect.rowPitch = aiParms3D->extent.width; // array has no pitch
		} else if (aiParms3D->srcPtrInfo.ptr) { //maybe memset, src ptr can be null
			pCpuSrc = aiParms3D->srcPtrInfo.ptr;
			dmaCmd->memcpy3D.srcRect.rowPitch = aiParms3D->srcPtrInfo.pitch;
		}
	}

	if ((pDevMemDst && (sizeBytes > (pDevMemDst->size - offsetDst) || dmaCmd->dstDevice->deviceId != pDevMemDst->psMemDesc->deviceId)) ||
	    (pDevMemSrc && (sizeBytes > (pDevMemSrc->size - offsetSrc) || dmaCmd->srcDevice->deviceId != pDevMemSrc->psMemDesc->deviceId))) {
		return AI_ERROR_INVALID_VALUE;
	}
	if (pCpuDst && pCpuSrc) {
		if (dmaCmd->kind != aiMemcpyHostToHost && dmaCmd->kind != aiMemcpyDefault) {
			return AI_ERROR_INVALID_VALUE;
		}
	}

	dmaCmd->memcpy3D.dstOffset = offsetDst;
	dmaCmd->memcpy3D.srcOffset = offsetSrc;
	dmaCmd->memcpy3D.psDstBuffer = pDevMemDst;
	dmaCmd->memcpy3D.psSrcBuffer = pDevMemSrc;
	dmaCmd->memcpy3D.pvDstPtr = pCpuDst;
	dmaCmd->memcpy3D.pvSrcPtr = pCpuSrc;

	// now api has no slicePitch
	dmaCmd->memcpy3D.dstRect.slicePitch = dmaCmd->memcpy3D.dstRect.rowPitch * dmaCmd->memcpy3D.size.c[1];
	dmaCmd->memcpy3D.srcRect.slicePitch = dmaCmd->memcpy3D.srcRect.rowPitch * dmaCmd->memcpy3D.size.c[1];

	if (aiParms3D->valueSize) {
		dmaCmd->memcpy3D.setValue = aiParms3D->value;
		dmaCmd->memcpy3D.valueSize = aiParms3D->valueSize;
	}

	return AI_SUCCESS;
}

static aiResult memSubmitDmaCmd(struct IDmaCopy *dmaCopy)
{
	aiResult status;
	aiDmaCopyCommandCreateInfo createInfo = { 0 };
	aiDmaCopyCommand *psDmaCopyCmd = nullptr;

	status = memGenDmaCmd(&dmaCopy->m_dmaCmd, &dmaCopy->m_aiParms3D); // trans parms to cmd
	if (status != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;

	if (dmaCopy->m_dmaCmd.kind == aiMemcpyHostToDevice) {
		status = mallocShadowGtt(&dmaCopy->m_dmaCmd);
		if (status != AI_SUCCESS)
			return AI_ERROR_INVALID_VALUE;

		transferHostToGtt(&dmaCopy->m_dmaCmd);
	}
	if (dmaCopy->m_dmaCmd.kind == aiMemcpyDeviceToHost) {
		status = mallocShadowGtt(&dmaCopy->m_dmaCmd);
		if (status != AI_SUCCESS)
			return AI_ERROR_INVALID_VALUE;

		createInfo.commandCallback = transferGttToHost;
	}

	createInfo.pDmaCmd = &dmaCopy->m_dmaCmd;

	status = aiDmaCopyCommandCreate(&psDmaCopyCmd, dmaCopy->mream, &createInfo);
	if (status != AI_SUCCESS) {
		AI_DPF_ERROR("Failed to create dma copy command object handle.");
		return status;
	}

	dmaCopy->mream->aiRecordCommands(dmaCopy->mream, (aiCommand *)psDmaCopyCmd);

	return AI_SUCCESS;
}

aiResult aiDmaCopyInit(struct IDmaCopy *dmaCopy, enum aiMemcpyKind_enum kind, aiStream *stream, aiDevice *dstDevice, aiDevice *srcDevice)
{
	if (iMemcpy3D_ValidatekindStream(kind, stream) != AI_SUCCESS)
		return AI_ERROR_INVALID_VALUE;

	memset(dmaCopy, 0, sizeof(struct IDmaCopy));
	dmaCopy->mream = stream;
	dmaCopy->m_dmaCmd.kind = kind;
	if (!dstDevice)
		dstDevice = stream->pContext->pDevice;
	if (!srcDevice)
		srcDevice = stream->pContext->pDevice;
	dmaCopy->m_dmaCmd.dstDevice = dstDevice;
	dmaCopy->m_dmaCmd.srcDevice = srcDevice;

	dmaCopy->memcpyGenCmd = memcpyGenCmd;
	dmaCopy->memcpyGenCmd2D = memcpyGenCmd2D;
	dmaCopy->memcpyGenCmd2D2A = mememcpyGenCmd2D2A;
	dmaCopy->memcpyGenCmdA2A = memcpyGenCmdA2A;
	dmaCopy->memcpyGenCmdA22D = memcpyGenCmdA22D;
	dmaCopy->memcpyGenCmd3D = memcpyGenCmd3D;
	dmaCopy->memsetGenCmd = memsetGenCmd;
	dmaCopy->memsetGenCmd2D = memsetGenCmd2D;
	dmaCopy->memsetGenCmd3D = memsetGenCmd3D;
	dmaCopy->memcpyFromSymbol = memcpyFromSymbol;
	dmaCopy->memcpyToSymbol = memcpyToSymbol;
	dmaCopy->memSubmitDmaCmd = memSubmitDmaCmd;

	return AI_SUCCESS;
}
#endif