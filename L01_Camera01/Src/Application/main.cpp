﻿#include "main.h"

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// エントリーポイント
// アプリケーションはこの関数から進行する
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_  HINSTANCE, _In_ LPSTR , _In_ int)
{
	// メモリリークを知らせる
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// COM初期化
	if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
	{
		CoUninitialize();

		return 0;
	}

	// mbstowcs_s関数で日本語対応にするために呼ぶ
	setlocale(LC_ALL, "japanese");

	//===================================================================
	// 実行
	//===================================================================
	Application::Instance().Execute();

	// COM解放
	CoUninitialize();

	return 0;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション更新開始
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::KdBeginUpdate()
{
	// 入力状況の更新
	KdInputManager::Instance().Update();

	// 空間環境の更新
	KdShaderManager::Instance().WorkAmbientController().Update();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション更新終了
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::KdPostUpdate()
{
	// 3DSoundListnerの行列を更新
	KdAudioManager::Instance().SetListnerMatrix(KdShaderManager::Instance().GetCameraCB().mView.Invert());
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション更新の前処理
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::PreUpdate()
{
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション更新
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::Update()
{
	//カメラ行列の更新
	{
		//拡縮
		Math::Matrix _mScale = Math::Matrix::CreateScale(1);

		//どれだけ傾けているか
		Math::Matrix _mRotationX = Math::Matrix::CreateRotationX(
			DirectX::XMConvertToRadians(45));

		static float _yRot = 0;

		Math::Matrix _mRotationY = Math::Matrix::CreateRotationY(
			DirectX::XMConvertToRadians(_yRot));
		//_yRot += 0.5;

		//カメラの座標
		Math::Matrix _mTrans = Math::Matrix::CreateTranslation(0, 6.0f, -5.0f);

		//カメラの「ワールド行列」を作成し適応させる
		Math::Matrix _worldMat = _mScale * _mRotationX * _mTrans *_mRotationY* _mHamuWorld;//W=S*R*T
		m_spCamera->SetCameraMatrix(_worldMat);

		//ハム太郎の更新
		{
			//キャラクターの移動速度(マネしちゃだめ)
			float			moveSpd = 0.05f;
			Math::Vector3	nowPos	= _mHamuWorld.Translation();

			//移動したい「方向ベクトル」::向き及び大きさ(矢印)
			//			=　絶対に長さが「１」でなければならない
			Math::Vector3 moveVec = Math::Vector3::Zero;
			
			if (GetAsyncKeyState('W')) { moveVec.z = 1.0f; }
			if (GetAsyncKeyState('A')) { moveVec.x = -1.0f;}
			if (GetAsyncKeyState('S')) { moveVec.z = -1.0f;}
			if (GetAsyncKeyState('D')) { moveVec.x = 1.0f; }

			//正規化
			moveVec.Normalize();
			moveVec *= moveSpd;
			nowPos	+= moveVec;

			//キャラクターのワールド行列を創る処理
			_mHamuWorld = Math::Matrix::CreateTranslation(nowPos);
		}
	}
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション更新の後処理
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::PostUpdate()
{
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション描画開始
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::KdBeginDraw(bool usePostProcess)
{
	KdDirect3D::Instance().ClearBackBuffer();

	KdShaderManager::Instance().WorkAmbientController().Draw();

	if (!usePostProcess) return;
	KdShaderManager::Instance().m_postProcessShader.Draw();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション描画終了
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::KdPostDraw()
{
	// BackBuffer -> 画面表示
	KdDirect3D::Instance().WorkSwapChain()->Present(0, 0);
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション描画の前処理
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::PreDraw()
{
	m_spCamera->SetToShader();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション描画
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::Draw()
{
	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	// 光を遮るオブジェクト(不透明な物体や2Dキャラ)はBeginとEndの間にまとめてDrawする
	KdShaderManager::Instance().m_StandardShader.BeginGenerateDepthMapFromLight();
	{
	}
	KdShaderManager::Instance().m_StandardShader.EndGenerateDepthMapFromLight();


	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	// 陰影のあるオブジェクト(不透明な物体や2Dキャラ)はBeginとEndの間にまとめてDrawする
	KdShaderManager::Instance().m_StandardShader.BeginLit();
	{
		//Math::Matrix _mat = Math::Matrix::Identity;//Identity(単位値)
		//_mat._43 = 5.0f;
		
		
		//static float _HamuZ = 5.0;
		//Math::Matrix _mat = Math::Matrix::CreateTranslation(0, 0, 5.0f);
		
		KdShaderManager::Instance().
			m_StandardShader.DrawPolygon(*m_spPoly,_mHamuWorld);
		//_HamuZ -= 0.01f;

		KdShaderManager::Instance().
			m_StandardShader.DrawModel(*m_spModel);
	}
	KdShaderManager::Instance().m_StandardShader.EndLit();


	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	// 陰影のないオブジェクト(透明な部分を含む物体やエフェクト)はBeginとEndの間にまとめてDrawする
	KdShaderManager::Instance().m_StandardShader.BeginUnLit();
	{
	}
	KdShaderManager::Instance().m_StandardShader.EndUnLit();


	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	// 光源オブジェクト(自ら光るオブジェクトやエフェクト)はBeginとEndの間にまとめてDrawする
	KdShaderManager::Instance().m_postProcessShader.BeginBright();
	{
	}
	KdShaderManager::Instance().m_postProcessShader.EndBright();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション描画の後処理
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::PostDraw()
{
	// 画面のぼかしや被写界深度処理の実施
	KdShaderManager::Instance().m_postProcessShader.PostEffectProcess();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 2Dスプライトの描画
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::DrawSprite()
{
	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	// 2Dの描画はこの間で行う
	KdShaderManager::Instance().m_spriteShader.Begin();
	{
	}
	KdShaderManager::Instance().m_spriteShader.End();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション初期設定
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
bool Application::Init(int w, int h)
{
	//===================================================================
	// ウィンドウ作成
	//===================================================================
	if (m_window.Create(w, h, "3D GameProgramming", "Window") == false) {
		MessageBoxA(nullptr, "ウィンドウ作成に失敗", "エラー", MB_OK);
		return false;
	}

	//===================================================================
	// フルスクリーン確認
	//===================================================================
	bool bFullScreen = false;
	if (MessageBoxA(m_window.GetWndHandle(), "フルスクリーンにしますか？", "確認", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
		bFullScreen = true;
	}

	//===================================================================
	// Direct3D初期化
	//===================================================================

	// デバイスのデバッグモードを有効にする
	bool deviceDebugMode = false;
#ifdef _DEBUG
	deviceDebugMode = true;
#endif

	// Direct3D初期化
	std::string errorMsg;
	if (KdDirect3D::Instance().Init(m_window.GetWndHandle(), w, h, deviceDebugMode, errorMsg) == false) {
		MessageBoxA(m_window.GetWndHandle(), errorMsg.c_str(), "Direct3D初期化失敗", MB_OK | MB_ICONSTOP);
		return false;
	}

	// フルスクリーン設定
	if (bFullScreen) {
		HRESULT hr;

		hr = KdDirect3D::Instance().SetFullscreenState(TRUE, 0);
		if (FAILED(hr))
		{
			MessageBoxA(m_window.GetWndHandle(), "フルスクリーン設定失敗", "Direct3D初期化失敗", MB_OK | MB_ICONSTOP);
			return false;
		}
	}

	//===================================================================
	// シェーダー初期化
	//===================================================================
	KdShaderManager::Instance().Init();

	//===================================================================
	// オーディオ初期化
	//===================================================================
	KdAudioManager::Instance().Init();

	//===================================================================
	//カメラ初期化
	//===================================================================
	m_spCamera = std::make_shared<KdCamera>();


	//===================================================================
	//ハムスター初期化
	//===================================================================
	m_spPoly = std::make_shared<KdSquarePolygon>();
	m_spPoly->SetMaterial("Asset/Data/LessonData/Character/Hamu.png");
	m_spPoly->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	//===================================================================
	//地形初期化
	//===================================================================
	m_spModel = std::make_shared<KdModelData>();
	m_spModel->Load("Asset/Data/LessonData/Terrain/Terrain.gltf");

	return true;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// アプリケーション実行
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void Application::Execute()
{
	KdCSVData windowData("Asset/Data/WindowSettings.csv");
	const std::vector<std::string>& sizeData = windowData.GetLine(0);

	//===================================================================
	// 初期設定(ウィンドウ作成、Direct3D初期化など)
	//===================================================================
	if (Application::Instance().Init(atoi(sizeData[0].c_str()), atoi(sizeData[1].c_str())) == false) {
		return;
	}

	//===================================================================
	// ゲームループ
	//===================================================================

	// 時間
	m_fpsController.Init();

	// ループ
	while (1)
	{
		// 処理開始時間Get
		m_fpsController.UpdateStartTime();

		// ゲーム終了指定があるときはループ終了
		if (m_endFlag)
		{
			break;
		}

		//=========================================
		//
		// ウィンドウ関係の処理
		//
		//=========================================

		// ウィンドウのメッセージを処理する
		m_window.ProcessMessage();

		// ウィンドウが破棄されてるならループ終了
		if (m_window.IsCreated() == false)
		{
			break;
		}

		if (GetAsyncKeyState(VK_ESCAPE))
		{
//			if (MessageBoxA(m_window.GetWndHandle(), "本当にゲームを終了しますか？",
//				"終了確認", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			{
				End();
			}
		}
		//=========================================
		//
		// アプリケーション更新処理
		//
		//=========================================

		KdBeginUpdate();
		{
			PreUpdate();

			Update();

			PostUpdate();
		}
		KdPostUpdate();

		//=========================================
		//
		// アプリケーション描画処理
		//
		//=========================================

		KdBeginDraw();
		{
			PreDraw();

			Draw();

			PostDraw();

			DrawSprite();
		}
		KdPostDraw();

		//=========================================
		//
		// フレームレート制御
		//
		//=========================================

		m_fpsController.Update();
	}

	//===================================================================
	// アプリケーション解放
	//===================================================================
	Release();
}

// アプリケーション終了
void Application::Release()
{
	KdInputManager::Instance().Release();

	KdShaderManager::Instance().Release();

	KdAudioManager::Instance().Release();

	KdDirect3D::Instance().Release();

	// ウィンドウ削除
	m_window.Release();
}
