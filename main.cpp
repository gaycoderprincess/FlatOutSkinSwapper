#include <windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
#include <d3d9.h>
#include <d3dx9.h>
#include "toml++/toml.hpp"
#include "nya_commonhooklib.h"
#include "../nya-common-fouc/fo2versioncheck.h"

#include "game.h"

bool bLogging = false;
bool bReplaceAllCarsTextures = false;
int nRefreshKey = VK_INSERT;

void WriteLog(const std::string& str) {
	if (!bLogging) return;

	static auto file = std::ofstream("FlatOutSkinSwapper_gcp.log");

	file << str;
	file << "\n";
	file.flush();
}

PDIRECT3DTEXTURE9 LoadTextureWithDDSCheck(const char* filename) {
	std::ifstream fin(filename, std::ios::in | std::ios::binary );
	if (!fin.is_open()) return nullptr;

	fin.seekg(0, std::ios::end);

	size_t size = fin.tellg();
	if (size <= 0x4C) return nullptr;

	auto data = new char[size];
	fin.seekg(0, std::ios::beg);
	fin.read(data, size);

	if (data[0] != 'D' || data[1] != 'D' || data[2] != 'S') {
		delete[] data;
		return nullptr;
	}

	if (data[0x4C] == 0x18) {
		data[0x4C] = 0x20;
		WriteLog("Loading " + (std::string)filename + " with DDS pixelformat hack");
	}

	PDIRECT3DTEXTURE9 texture;
	auto hr = D3DXCreateTextureFromFileInMemory(g_pd3dDevice, data, size, &texture);
	delete[] data;
	if (hr != S_OK) {
		WriteLog("Failed to load " + (std::string)filename);
		return nullptr;
	}
	return texture;
}

// Simple helper function to load an image into a DX9 texture with common settings
PDIRECT3DTEXTURE9 LoadTexture(const char* filename) {
	if (!std::filesystem::exists(filename)) return nullptr;

	// Load texture from disk
	PDIRECT3DTEXTURE9 texture;
	auto hr = D3DXCreateTextureFromFileA(g_pd3dDevice, filename, &texture);
	if (hr != S_OK) {
		return LoadTextureWithDDSCheck(filename);
	}
	return texture;
}

std::vector<Player*> aPlayers;

bool bFirstFrameToReplaceTextures = false;
void OnLocalPlayerInitialized() {
	aPlayers.clear();
	bFirstFrameToReplaceTextures = true;
}

void __fastcall CollectAPlayer(Player* pPlayer) {
	aPlayers.push_back(pPlayer);
}

uintptr_t CollectLocalPlayerASM_jmp = 0x43F555;
void __attribute__((naked)) CollectLocalPlayerASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, ebx\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov [ebx+0x2B4], edx\n\t"
		"mov ecx, [ebp+0x68C]\n\t"
		"jmp %0\n\t"
			:
			:  "m" (CollectLocalPlayerASM_jmp), "i" (CollectAPlayer)
	);
}

uintptr_t CollectLocalPlayerASM2_jmp = 0x43F7C1;
void __attribute__((naked)) CollectLocalPlayerASM2() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, edi\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov eax, edi\n\t"
		"mov dword ptr [edi+0x2B4], 1\n\t"
		"jmp %0\n\t"
			:
			:  "m" (CollectLocalPlayerASM2_jmp), "i" (CollectAPlayer)
	);
}

uintptr_t CollectLocalPlayerASM3_jmp = 0x43F9FB;
void __attribute__((naked)) CollectLocalPlayerASM3() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, edi\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov [edi+0x2C4], edx\n\t"
		"mov eax, [ebx]\n\t"
		"jmp %0\n\t"
			:
			:  "m" (CollectLocalPlayerASM3_jmp), "i" (CollectAPlayer)
	);
}

uintptr_t CollectLocalPlayerASM4_jmp = 0x43FF1B;
void __attribute__((naked)) CollectLocalPlayerASM4() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov [esi+0x328], eax\n\t"
		"mov [esi+0x2B4], eax\n\t"
		"jmp %0\n\t"
			:
			:  "m" (CollectLocalPlayerASM4_jmp), "i" (CollectAPlayer)
	);
}

uintptr_t CollectLocalPlayerASM5_jmp = 0x43FD60;
void __attribute__((naked)) CollectLocalPlayerASM5() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov [esi+0x2C4], edx\n\t"
		"mov eax, [ebx+0x34]\n\t"
		"jmp %0\n\t"
			:
			:  "m" (CollectLocalPlayerASM5_jmp), "i" (CollectAPlayer)
	);
}

uintptr_t CollectLocalPlayerASM6_jmp = 0x4400B0;
void __attribute__((naked)) CollectLocalPlayerASM6() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov [esi+0x2B4], eax\n\t"
		"mov ecx, [ebx+0x30]\n\t"
		"jmp %0\n\t"
			:
			:  "m" (CollectLocalPlayerASM6_jmp), "i" (CollectAPlayer)
	);
}

// this hook is just used to collect all player pointers in a race, could prolly be moved somewhere that makes more sense?
// it being here is mostly just a leftover from an earlier attempt but works fine so why bother >w<
Player* pTargetPlayerForLoadGameTexture = nullptr;
auto LoadGameTexture = (DevTexture*(__stdcall*)(int, const char*, int, int))0x4EFE40;
DevTexture* __stdcall LoadGameTextureHooked(int a1, const char *src, int a3, int a4) {
	// just return the original function if this player is already in the list
	for (auto& player : aPlayers) {
		if (player == pTargetPlayerForLoadGameTexture) return LoadGameTexture(a1, src, a3, a4);
	}
	aPlayers.push_back(pTargetPlayerForLoadGameTexture);
	return LoadGameTexture(a1, src, a3, a4);
}

uintptr_t LoadSkinTextureASM_jmp = 0x423AAA;
void __attribute__((naked)) LoadSkinTextureASM() {
	__asm__ (
		"mov eax, [esp+0x3C]\n\t"
		"mov eax, [eax+0x3728]\n\t"
		"mov %2, eax\n\t"
		"call %0\n\t"
		"mov [ebp+0], eax\n\t"
		"jmp %1\n\t"
			:
			:  "i" (LoadGameTextureHooked), "m" (LoadSkinTextureASM_jmp), "m" (pTargetPlayerForLoadGameTexture)
	);
}

IDirect3DTexture9* TryLoadCustomTexture(std::string path) {
	// load tga first, game initially has tga in the path
	if (auto tex = LoadTexture(path.c_str())) return tex;

	// then load dds if tga doesn't exist
	for (int i = 0; i < 3; i++) path.pop_back();
	path += "dds";
	if (auto tex = LoadTexture(path.c_str())) return tex;

	// load png as a last resort
	for (int i = 0; i < 3; i++) path.pop_back();
	path += "png";
	if (auto tex = LoadTexture(path.c_str())) return tex;
	return nullptr;
}

void ReplaceTextureWithCustom(DevTexture* pTexture, const char* path) {
	if (auto texture = TryLoadCustomTexture(path)) {
		if (pTexture->pD3DTexture) pTexture->pD3DTexture->Release();
		pTexture->pD3DTexture = texture;
		WriteLog("Replaced texture " + (std::string)path + " with loose files");
	}
}

void ReplaceTextureWithCustom(DevTexture* pTexture) {
	ReplaceTextureWithCustom(pTexture, pTexture->sPath);
}

void SetCarTexturesToCustom(Car* car) {
	if (!car) return;

	int i = 0;
	while (&car->pTextureNodes[i] != car->pTextureNodesEnd) {
		if (auto node = car->pTextureNodes[i]) {
			if (auto texture = node->pTexture) {
				ReplaceTextureWithCustom(texture);
			}
		}
		i++;
	}

	ReplaceTextureWithCustom(car->pSkinDamaged);
	ReplaceTextureWithCustom(car->pLightsDamaged);
	ReplaceTextureWithCustom(car->pLightsGlow);
	ReplaceTextureWithCustom(car->pLightsGlowLit);
}

bool bKeyPressed;
bool bKeyPressedLast;
bool IsKeyJustPressed(int key) {
	if (key <= 0) return false;
	if (key >= 255) return false;

	bKeyPressedLast = bKeyPressed;
	bKeyPressed = (GetAsyncKeyState(key) & 0x8000) != 0;
	return bKeyPressed && !bKeyPressedLast;
}

struct tMenuCarData {
	int carId;
	int skinId;
	DevTexture* pTexture;
};
std::vector<tMenuCarData> aMenuCarSkins;
int nLastMenuCarId = -1;
int nLastMenuCarSkinId = -1;

void OnExitMenu() {
	aMenuCarSkins.clear();
	nLastMenuCarId = -1;
	nLastMenuCarSkinId = -1;
}

auto RenderRace = (void(__stdcall*)(void*, void*, int, void*))0x47C570;
void __stdcall RenderRaceHooked(void* a1, void* a2, int a3, void* a4) {
	OnExitMenu();
	if (!aPlayers.empty() && (bFirstFrameToReplaceTextures || IsKeyJustPressed(nRefreshKey))) {
		if (bReplaceAllCarsTextures) {
			for (auto& player : aPlayers) {
				SetCarTexturesToCustom(player->pCar);
			}
		}
		else SetCarTexturesToCustom(aPlayers[0]->pCar);
		bFirstFrameToReplaceTextures = false;
	}
	RenderRace(a1, a2, a3, a4);
}

auto InitPlayers = (void(__stdcall*)(void*))0x43EFC0;
void __stdcall InitPlayersHooked(void* a1) {
	OnLocalPlayerInitialized();
	return InitPlayers(a1);
}

std::string GenerateCarSkinPath(int car, int skin) {
	std::string path = "data/cars/car_";
	path += std::to_string(car);
	path += "/skin";
	path += std::to_string(skin);
	path += ".tga";
	return path;
}

DevTexture* GetTextureForMenuCarSkin(int car, int skin) {
	for (auto& data : aMenuCarSkins) {
		if (car == data.carId && skin == data.skinId) {
			return data.pTexture;
		}
	}
	return nullptr;
}

auto UpdateMenuCarSkin = (void(__stdcall*)(MenuCar*))0x460890;
void __stdcall UpdateMenuCarSkinHooked(MenuCar* a1) {
	UpdateMenuCarSkin(a1);
	if (a1->pSkin && (nLastMenuCarId != a1->nCarId || nLastMenuCarSkinId != a1->nSkinId || IsKeyJustPressed(nRefreshKey))) {
		if (auto textureToWrite = GetTextureForMenuCarSkin(a1->nCarId, a1->nSkinId)) {
			ReplaceTextureWithCustom(textureToWrite, GenerateCarSkinPath(a1->nCarId, a1->nSkinId).c_str());
			nLastMenuCarId = a1->nCarId;
			nLastMenuCarSkinId = a1->nSkinId;
		}
	}
}

void __fastcall CollectMenuCarData(MenuCar* menuCar, DevTexture* pTexture) {
	for (auto& data : aMenuCarSkins) {
		if (menuCar->nCarId == data.carId && menuCar->nSkinId == data.skinId) {
			data.pTexture = pTexture;
			return;
		}
	}
	aMenuCarSkins.push_back({menuCar->nCarId, menuCar->nSkinId, pTexture});
	while (aMenuCarSkins.size() > 3) aMenuCarSkins.erase(aMenuCarSkins.begin());
}

uintptr_t LoadMenuCarSkinASM_jmp = 0x460B98;
void __attribute__((naked)) LoadMenuCarSkinASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"mov edx, eax\n\t"
		"call %0\n\t"
		"popad\n\t"

		"mov [edi+0x24], eax\n\t"
		"mov ecx, [esi+0x1E48]\n\t"
		"mov eax, [esi+0x14]\n\t"
		"jmp %1\n\t"
			:
			:  "i" (CollectMenuCarData), "m" (LoadMenuCarSkinASM_jmp)
	);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			DoFlatOutVersionCheck(FO2Version::FO1_1_1);

			auto config = toml::parse_file("FlatOutSkinSwapper_gcp.toml");
			bReplaceAllCarsTextures = config["main"]["replace_all_cars"].value_or(false);
			nRefreshKey = config["main"]["reload_key"].value_or(VK_INSERT);
			bLogging = config["main"]["logging"].value_or(false);

			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x43F549, &CollectLocalPlayerASM);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x43F7B5, &CollectLocalPlayerASM2);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x43F9F3, &CollectLocalPlayerASM3);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x43FF0F, &CollectLocalPlayerASM4);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x43FD57, &CollectLocalPlayerASM5);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4400A7, &CollectLocalPlayerASM6);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x460B8C, &LoadMenuCarSkinASM);
			LoadGameTexture = (DevTexture*(__stdcall*)(int, const char*, int, int))NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x423AA2, &LoadSkinTextureASM);
			RenderRace = (void(__stdcall*)(void*, void*, int, void*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x45309D, &RenderRaceHooked);
			InitPlayers = (void(__stdcall*)(void*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x43EE43, &InitPlayersHooked);
			UpdateMenuCarSkin = (void(__stdcall*)(MenuCar*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x465E97, &UpdateMenuCarSkinHooked);
		} break;
		default:
			break;
	}
	return TRUE;
}