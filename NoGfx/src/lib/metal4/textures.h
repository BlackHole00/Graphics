#ifndef MTL4_TEXTURES_H
#define MTL4_TEXTURES_H

#include <gpu/gpu.h>

#include <lib/common/page.h>
#include <lib/common/handle_map.h>
#include <lib/common/type_traits.h>
#include <Metal/Metal.h>

typedef CmnHandle Mtl4Texture;

typedef struct Mtl4TextureViews {
	GpuTextureDesc textureDescriptors[7];
	id<MTLTexture> textures[7];

	struct Mtl4TextureViews* newBucket;
} Mtl4TextureViews;

typedef struct Mtl4TextureMetadata {
	id<MTLTexture>		texture;
	GpuTextureDescriptor	descriptor;

	Mtl4TextureViews*	relatedViews;
} Mtl4TextureMetadata;

typedef struct Mtl4TextureStorage {
	CmnPage		textureMetadataPage;
	CmnPage		textureViewsPage;

	CmnArena	textureMedatadaArena;
	CmnPool		textureViewsPool;

	CmnHandleMap<Mtl4TextureMetadata>	textures;
	CmnMutex				mutex;
} Mtl4TextureStorage;
extern Mtl4TextureStorage gMtl4TextureStorage;

void mtl4InitTextureStorage(GpuResult* result);
void mtl4FiniTextureStorage(void);

GpuTextureSizeAlign mtl4TextureSizeAlign(const GpuTextureDesc* desc, GpuResult* result);
GpuTexture mtl4CreateTexture(const GpuTextureDesc* desc, void* ptrGpu, GpuResult* result);
GpuTextureDescriptor mtl4TextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);
GpuTextureDescriptor mtl4RWTextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);

void mtl4DestroyTexture(Mtl4Texture texture);

MTLTextureDescriptor* mtl4GpuTextureDescToMtl(const GpuTextureDesc* desc, MTLResourceOptions resourceOptions);
MTLTextureViewDescriptor* mtl4GpuViewDescToMtl(id<MTLTexture> referenceTexture, const GpuViewDesc* desc);

#endif // MTL4_TEXTURES_H

