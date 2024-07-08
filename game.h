auto& g_pd3dDevice = *(IDirect3DDevice9**)0x6C00D8;

class DevTexture {
public:
	uint8_t _0[0x20];
	char* sPath;
	uint8_t _24[0x2C];
	PDIRECT3DTEXTURE9 pD3DTexture;
};

class tTextureNode {
public:
	DevTexture* pTexture;
};

class Car {
public:
	uint8_t _0[0x10EC];
	tTextureNode** pTextureNodes;
	tTextureNode** pTextureNodesEnd;
	uint8_t _10F4[0x25A4];
	DevTexture* pSkinDamaged;
	DevTexture* pLightsDamaged;
	DevTexture* pLightsGlow;
	DevTexture* pLightsGlowLit;
};

class Player {
public:
	uint8_t _0[0x2C0];
	Car* pCar;
};