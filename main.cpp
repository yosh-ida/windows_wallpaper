#include "DxLib.h"

#define _DRAW_STAGE
#define _DRAW_SKYDOME
//#define _DRAW_3DMODEL
//#define _DRAW_FOG
//#define _DRAW_EFFECT

#ifdef _DRAW_EFFECT
#include "P3DEngine.h"
#if _DEBUG
#pragma comment(lib, "P3DEngineD.lib")
#else
#pragma comment(lib, "P3DEngine.lib")
#endif
P3DEngine* pP3D;
#endif

BOOL CALLBACK find_worker(HWND wnd, LPARAM lp)
{
	HWND *pworker = (HWND*)lp;

	if (!FindWindowExA(wnd, 0, "SHELLDLL_DefView", 0)) {
		return TRUE;
	}

	*pworker = FindWindowExA(0, wnd, "WorkerW", 0);
	if (*pworker) {
		//log_window("wallpaper is ", *pworker);
		//log_window("its parent is ", wnd);
		return FALSE;
	}

	return TRUE;
}

HWND wp_id()
{
	HWND progman;
	HWND worker;

	progman = FindWindowA("Progman", 0);

	if (!progman) return 0;

	/*
	 * this is basically all the magic. it's an undocumented window message that
	 * forces windows to spawn a window with class "WorkerW" behind deskicons
	 */

	SendMessageA(progman, 0x052C, 0xD, 0);
	SendMessageA(progman, 0x052C, 0xD, 1);

	EnumWindows(find_worker, (LPARAM)&worker);

	if (!worker)
	{
		SendMessageA(progman, 0x052C, 0, 0);
		EnumWindows(find_worker, (LPARAM)&worker);
	}

	return worker;
}

#define	RETURN_WITH_TERMINATE(x)	{ DxLib_End(); return x; }
#define ERROR_GET_WALLPAPER_HANDLE	-1000
#define	ERROR_GET_WALLPAPER_RECT	-1001
#define	ERROR_DXLIB_SETTING			-1002
#define	ERROR_DXLIB_INIT			-1003
#define	ERROR_LOAD_3DMODEL			-1004
#define	ERROR_DRAW_3DMODEL			-1005

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
	default:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int checkModelPositon()
{
	ChangeWindowMode(TRUE);
#ifdef _DRAW_EFFECT
	SetUseDirect3D9Ex(FALSE);
#endif
	if (DxLib_Init() == -1) return ERROR_DXLIB_INIT;
	SetDrawScreen(DX_SCREEN_BACK);

	SetCameraPositionAndTarget_UpVecY(VGet(0, 0, 0), VGet(0, 0, 300));

	int mh = MV1LoadModel("aincrad.mqo");
	if (mh == -1) RETURN_WITH_TERMINATE(ERROR_LOAD_3DMODEL);

	MV1SetPosition(mh, VGet(0, 0, 300));
	MV1SetWireFrameDrawFlag(mh, TRUE);
	
	char buf[256];
	float x = 0.0f, y = 0.0f, z = 300.0f;
	float speed = 1.0f;

#ifdef _DRAW_EFFECT
	P3DInitData data;
	data.pDevice = (IDirect3DDevice9 *)GetUseDirect3DDevice9();
	data.effectFilePath = "Assets";
	pP3D = new P3DEngine(data);

	pP3D->Load("Explosion.p3b");
	D3DXVECTOR3 locate = D3DXVECTOR3(0, 0, 300);
	P3DEffect *pEffect = pP3D->Play("Explosion.p3b", locate, FALSE);
	D3DXVECTOR3 scale = D3DXVECTOR3(1, 1, 1);
	pP3D->SetScaling(pEffect, scale);
#endif
	while (1)
	{
		GetHitKeyStateAll(buf);
		if (buf[KEY_INPUT_ESCAPE])
			break;
		if (buf[KEY_INPUT_RIGHT])
			x += speed;
		if (buf[KEY_INPUT_LEFT])
			x -= speed;
		if (buf[KEY_INPUT_UP])
			z += speed;
		if (buf[KEY_INPUT_DOWN])
			z -= speed;
		if (buf[KEY_INPUT_W])
			y += speed;
		if (buf[KEY_INPUT_Q])
			y -= speed;
		MV1SetPosition(mh, VGet(x, y, z));
		ClearDrawScreen();
		if (MV1DrawModel(mh) == -1) RETURN_WITH_TERMINATE(ERROR_DRAW_3DMODEL);
#ifdef _DRAW_EFFECT
		pP3D->Draw();
		RefreshDxLibDirect3DSetting();
#endif
		ScreenFlip();
		WaitTimer(1000 / 60);
	}
	
	MV1DeleteModel(mh);
	
	DxLib_End();
	return 0;
}

int dxlib_main()
{
	HWND hWnd = wp_id();
	if (hWnd == NULL) return ERROR_GET_WALLPAPER_HANDLE;
	RECT lprc;
	if (GetWindowRect(hWnd, &lprc) == FALSE) return ERROR_GET_WALLPAPER_RECT;
	
	LONG windowX = lprc.right - lprc.left;
	LONG windowY = lprc.bottom - lprc.top;
	SetGraphMode(windowX, windowY, 32);

	ChangeWindowMode(TRUE);

	//	SetUseWindow �Őݒ肵���E�C���h�E�̃��b�Z�[�W���[�v������DX���C�u�����ōs�����ǂ�����ݒ肷��
	//	TRUE:DX���C�u�����ōs��( �f�t�H���g ) FALSE:DX���C�u�����ł͍s��Ȃ�
	if (SetUserWindowMessageProcessDXLibFlag(FALSE) == -1) return ERROR_DXLIB_SETTING;

	//	DX���C�u�����Ŏg�p����E�C���h�E�̃n���h�����Z�b�g����( DxLib_Init �����s����ȑO�ł̂ݗL�� )
	if (SetUserWindow(hWnd) == -1) return ERROR_DXLIB_SETTING;
	
	//	DX���C�u�����̃E�C���h�E�֘A�̋@�\���g�p���Ȃ����ǂ�����ݒ肷��
	//	TRUE:�g�p���Ȃ� FALSE:�g�p����( �f�t�H���g )
	if (SetNotWinFlag(TRUE) == -1) return ERROR_DXLIB_SETTING;

	//	DX���C�u�����̕`��@�\���g�����ǂ�����ݒ肷��
	//	TRUE:�g�p���Ȃ� FALSE:�g�p����( �f�t�H���g )
	if (SetNotDrawFlag(FALSE) == -1) return ERROR_DXLIB_SETTING;

	//	DX���C�u�����̃T�E���h�@�\���g�����ǂ�����ݒ肷��
	//	TRUE:�g�p���Ȃ� FALSE:�g�p����( �f�t�H���g )
	if (SetNotSoundFlag(TRUE) == -1) return ERROR_DXLIB_SETTING;

	//	DX���C�u�����̓��͏�Ԃ̎擾�@�\���g�����ǂ�����ݒ肷��
	//	TRUE:�g�p���Ȃ� FALSE:�g�p����( �f�t�H���g )
	if (SetNotInputFlag(TRUE) == -1) return ERROR_DXLIB_SETTING;

	//SetBackgroundColor(0, 0, 255);
#ifdef _DRAW_EFFECT
	if (SetUseDirect3D9Ex(FALSE) == -1) return ERROR_DXLIB_SETTING;
#endif
	if (DxLib_Init() == -1) return ERROR_DXLIB_INIT;

	SetDrawScreen(DX_SCREEN_BACK);
	SetCameraPositionAndTarget_UpVecY(VGet(0, 0, 0), VGet(0, 0, 400));

#ifdef _DRAW_3DMODEL
	int mh = MV1LoadModel("res\\model\\aincrad.mqo");
	if (mh == -1) RETURN_WITH_TERMINATE(ERROR_LOAD_3DMODEL);
	MV1SetPosition(mh, VGet(0.0f, 0.0f, 600.0f));
	//MV1SetWireFrameDrawFlag(mh, TRUE);
#endif
#ifdef _DRAW_SKYDOME
	int sky = MV1LoadModel("res\\skydome\\askyY_ao.x");
	if (sky == -1) RETURN_WITH_TERMINATE(ERROR_LOAD_3DMODEL);
	MV1SetPosition(sky, VGet(0.0f, 0.0f, 600.0f));
	//MV1SetScale(sky, VGet(2, 2, 2));
	VECTOR sky_rotate = VGet(0, 0, 0);
	MV1SetRotationXYZ(sky, sky_rotate);
	MV1SetUseZBuffer(sky, FALSE);
#endif
#ifdef _DRAW_FOG
	if (SetFogEnable(TRUE) == -1) return -1;
	if (SetFogStartEnd(800, 2000) == -1) return -1;
	if (SetFogColor(128, 128, 128) == -1) return -1;
#endif
#ifdef _DRAW_STAGE
	int stage = MV1LoadModel("res\\stage\\���ʘOver0.93\\���ʘO.x");
	if (stage == -1) RETURN_WITH_TERMINATE(ERROR_LOAD_3DMODEL);
	MV1SetPosition(stage, VGet(0.0f, -100.0f, 300.0f));
	MV1SetScale(stage, VGet(10, 10, 10));
#endif

	constexpr int wait_time = 1000 / 60;

#ifdef _DRAW_EFFECT
	//Prominence3D���C�u�����̏���
	P3DInitData data;
	data.pDevice = (IDirect3DDevice9 *)GetUseDirect3DDevice9();
	data.effectFilePath = "Assets";
	pP3D = new P3DEngine(data);

	//�G�t�F�N�g�f�[�^�̓ǂݍ���
	pP3D->Load("Explosion.p3b");
	//�G�t�F�N�g���Đ�����
	D3DXVECTOR3 locate = D3DXVECTOR3(0,0,300);
	P3DEffect *pEffect  = pP3D->Play("Explosion.p3b", locate, FALSE);
	D3DXVECTOR3 scale = D3DXVECTOR3(1, 1, 1);
	pP3D->SetScaling(pEffect, scale);
#endif

	while (1)
	{
		ClearDrawScreen();
#ifdef _DRAW_SKYDOME
		sky_rotate = VAdd(sky_rotate, VGet(0, DX_PI_F / 19200, 0));
		//sky_rotate = VAdd(sky_rotate, VGet(0, DX_PI_F / 1920, 0));
		MV1SetRotationXYZ(sky, sky_rotate);
		if (MV1DrawModel(sky) == -1) RETURN_WITH_TERMINATE(ERROR_DRAW_3DMODEL);
#endif
#ifdef _DRAW_STAGE
		if (MV1DrawModel(stage) == -1) RETURN_WITH_TERMINATE(ERROR_DRAW_3DMODEL);
#endif
#ifdef _DRAW_3DMODEL
		if (MV1DrawModel(mh) == -1) RETURN_WITH_TERMINATE(ERROR_DRAW_3DMODEL);
#endif

#ifdef _DRAW_EFFECT
		pP3D->Draw();
		RefreshDxLibDirect3DSetting();
#endif
		ScreenFlip();
		WaitTimer(wait_time);
		//static int i = 0;
		//if (i++ >= 60 * 10) break;
	}
#ifdef _DRAW_EFFECT
	delete(pP3D);
#endif
#ifdef _DRAW_3DMODEL
	MV1DeleteModel(mh);
#endif
#ifdef _DRAW_SKYDOME
	MV1DeleteModel(sky);
#endif
#ifdef _DRAW_STAGE
	MV1DeleteModel(stage);
#endif
	DxLib_End();
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//return checkModelPositon();
	return dxlib_main();
}