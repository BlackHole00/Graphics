#ifndef MTL4_TEXTURES_H
#define MTL4_TEXTURES_H

#include <gpu/gpu.h>

#include <lib/common/page.h>
#include <lib/common/handle_map.h>
#include <lib/common/type_traits.h>
#include <lib/common/storage_sync.h>
#include <Metal/Metal.h>

typedef CmnHandle Mtl4Texture;
static_assert(sizeof(Mtl4Texture) == sizeof(GpuTexture), "Mtl4Texture and GpuTexture must be compatible");

typedef struct Mtl4TextureViews {
	id<MTLTexture> views[7];
	GpuViewDesc viewsDescriptors[7];

	struct Mtl4TextureViews* nextBucket;
} Mtl4TextureViews;

typedef struct Mtl4TextureMetadata {
	// Atomic, Settable once
	bool scheduledForDeletion;
	
	// Final
	id<MTLTexture>		texture;
	// Final
	GpuTextureDescriptor	descriptor;

	// Locked by relatedViewsMutex
	Mtl4TextureViews*	relatedViews;
	CmnRWMutex		relatedViewsMutex;
} Mtl4TextureMetadata;

typedef struct Mtl4TextureStorage {
	CmnPage		textureMetadataPage;
	CmnPage		textureViewsPage;

	CmnArena	textureMedatadaArena;
	CmnPool		textureViewsPool;

	CmnHandleMap	<Mtl4TextureMetadata>	textures;
	CmnStorageSync	sync;
} Mtl4TextureStorage;
extern Mtl4TextureStorage gMtl4TextureStorage;

void mtl4InitTextureStorage(GpuResult* result);
void mtl4FiniTextureStorage(void);

GpuTextureSizeAlign mtl4TextureSizeAlign(const GpuTextureDesc* desc, GpuResult* result);
GpuTexture mtl4CreateTexture(const GpuTextureDesc* desc, void* ptrGpu, GpuResult* result);
GpuTextureDescriptor mtl4TextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);
GpuTextureDescriptor mtl4RWTextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);

void mtl4FreeTexture(Mtl4Texture texture);
bool mtl4IsTextureScheduledForDeletion(Mtl4Texture texture);

Mtl4TextureMetadata* mtl4AcquireTextureMetadataFrom(Mtl4Texture texture);
void mtl4ReleaseTextureMetadata(void);

bool mtl4FindTextureViewIn(Mtl4TextureMetadata* metadata, const GpuViewDesc* desc, Mtl4TextureViews** bucket, size_t* index);

inline Mtl4Texture mtl4GpuTextureToHadle(GpuTexture texture) {
	return *(Mtl4Texture*)&texture;
}

inline GpuTexture mtl4HandleToGpuTexture(Mtl4Texture handle) {
	return *(GpuTexture*)&handle;
}

inline GpuTextureDescriptor mtl4TextureResourceIdToDescriptor(MTLResourceID id) {
	GpuTextureDescriptor desc;
	desc.data[0] = id._impl;
	desc.data[1] = 0;
	desc.data[2] = 0;
	desc.data[3] = 0;

	return desc;
}

MTLTextureDescriptor* mtl4GpuTextureDescToMtl(const GpuTextureDesc* desc, MTLResourceOptions resourceOptions);
MTLTextureViewDescriptor* mtl4GpuViewDescToMtl(id<MTLTexture> referenceTexture, const GpuViewDesc* desc);

void mtl4AssociateViewToTexture(Mtl4TextureMetadata* metadata, id<MTLTexture> view, const GpuViewDesc* desc, GpuResult* result);
// NOTE: Not thread safe. Requires external locking.
void mtl4FreeAssociatedTextureViews(Mtl4TextureMetadata* metadata);

// NOTE: Requires deletion lock on gMtl4TextureStorage.sync
void mtl4FreeTexture(Mtl4Texture texture);

#endif // MTL4_TEXTURES_H

