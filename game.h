auto& g_pd3dDevice = *(IDirect3DDevice9**)0x6C00D8;

class DevTexture {
public:
	uint8_t _0[0x20];
	char* sPath; // +20
	uint8_t _24[0x2C];
	PDIRECT3DTEXTURE9 pD3DTexture; // +50
};

class tTextureNode {
public:
	DevTexture* pTexture;
};

class Car {
public:
	uint8_t _0[0x10EC];
	tTextureNode** pTextureNodes; // +10EC
	tTextureNode** pTextureNodesEnd; // +10F0
	uint8_t _10F4[0x25A4];
	DevTexture* pSkinDamaged; // +3698
	DevTexture* pLightsDamaged; // +369C
	DevTexture* pLightsGlow; // +36A0
	DevTexture* pLightsGlowLit; // +36A4
};

class Player {
public:
	uint8_t _0[0x2C0];
	Car* pCar; // +2C0
};

class MenuCar {
public:
	int nCarId; // +0
	uint8_t _4[0x4];
	uint32_t nModelSize; // +8
	void* pModel; // +C
	uint8_t _10[0x4];
	int nSkinId; // +14
	uint8_t _18[0x4];
	uint32_t nSkinSize; // +1C
	void* pSkin; // +20
};