#pragma once

#include <d3d11.h>
#include "direct3d.h"
#include "texture.h"
#include "component.h"
#include "model.h"
#include "sprite3d.h"
#include "debug_ostream.h"
#include <DirectXMath.h>
#include <vector>
#include <string>
using namespace DirectX;

// アニメーション用キーフレーム構造体
struct KeyVec3 { double time; XMFLOAT3 value; };
struct KeyQuat { double time; XMFLOAT4 value; };  // w, x, y, z

// ボーンのキーフレーム情報
struct BoneKeyframes {
	std::vector<KeyVec3> trans;    // 平行移動キーフレーム
	std::vector<KeyVec3> scale;    // スケーリングキーフレーム
	std::vector<KeyQuat> rot;      // 回転キーフレーム
};

// アニメーションクリップ
struct AnimationClip {
	double duration = 0.0;         // アニメーション長（単位: ティック）
	double tps = 24.0;             // ティックスパーセコンド
	std::vector<BoneKeyframes> tracks;  // ボーンごとのキーフレーム
};

// アニメーション再生状態
struct AnimationState {
	const AnimationClip* clip = nullptr;
	double time = 0.0;             // 現在の再生位置（単位: ティック）
	bool play = false;             // 再生中フラグ
	bool loop = true;              // ループフラグ
	std::string currentAnimName = "";  // 現在再生中のアニメーション名
};

// マテリアル用定数バッファ
struct MaterialCB {
	XMFLOAT4 overrideColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	int useOverride = 0;           // オーバーライド使用フラグ
	int hasTex = 0;                // テクスチャ使用フラグ
	int pad[2];
};

// アニメーション遷移（ブレンド）状態
struct AnimationBlendState {
	bool isBlending = false;          // ブレンド中フラグ
	double blendDuration = 0.3;       // ブレンド時間（秒）
	double blendElapsed = 0.0;        // ブレンド経過時間（秒）
	AnimationClip targetClip;         // 遷移先アニメーション
	AnimationState previousState;     // 遷移前のアニメーション状態（スナップショット）
};

class AnimSprite3D : public Sprite3D
{
protected:
	AnimationState m_AnimState;
	AnimationClip m_AnimClip;
	AnimationBlendState m_BlendState;  // アニメーション遷移用
	BoneMatrices m_BoneMatrices;
	int m_BoneCount = 0;

	// Assimp用ボーンマッピング
	std::vector<aiBone*> m_AiBones;
	std::vector<XMMATRIX> m_BoneOffsetMatrices;

	// ノードアニメーション用
	std::vector<aiNode*> m_AnimatedNodes;
	std::vector<XMMATRIX> m_NodeTransforms;

public:
	AnimSprite3D(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
		: Sprite3D(pos, scale, rot, pass)
	{
		// Sprite3D のコンストラクタでモデルが読み込まれた後、ボーン情報を初期化
		InitializeBones();
	}

	~AnimSprite3D()
	{
		// Sprite3D の デストラクタが呼ばれます
	}

	// ボーン情報の初期化
	void InitializeBones();

	// アニメーションの設定
	// ※外部からの手動アニメーション設定は不要（FBX内のアニメーションを使用）
	/*
	void SetAnimation(const AnimationClip& clip)
	{
		m_AnimClip = clip;
		m_AnimState.clip = &m_AnimClip;
		m_AnimState.time = 0.0;
		m_AnimState.play = false;
	}
	*/

	// アニメーション再生制御
	// アニメーション再生開始（ローカルで設定済みのアニメーションを再生）
	void PlayAnimation(bool loop = true)
	{
		m_AnimState.play = true;
		m_AnimState.loop = loop;
		m_AnimState.time = 0.0;
		// currentAnimName は SetAnimationClip で設定されるか、各再生関数で設定される
	}

	void StopAnimation()
	{
		m_AnimState.play = false;
	}

	void PauseAnimation()
	{
		m_AnimState.play = false;
	}

	void ResumeAnimation()
	{
		if (m_AnimState.clip != nullptr)
			m_AnimState.play = true;
	}

	bool IsAnimationPlaying() const { return m_AnimState.play; }

	// アニメーション時間を更新（毎フレーム呼び出し）
	void UpdateAnimation(float dt);

	// ボーン行列を更新
	void UpdateBoneMatrices();

	// 補間関数
	static XMFLOAT3 InterpolateVec3(const std::vector<KeyVec3>& keys, double time);
	static XMFLOAT4 InterpolateQuat(const std::vector<KeyQuat>& keys, double time);

	// アニメーション付きで描画
	virtual void Draw(void);

	// マテリアルカラーのオーバーライド
	void SetMaterialOverrideColor(const XMFLOAT4& color)
	{
		Sprite3D::SetColor(color);
	}

	void ResetMaterialOverride()
	{
		Sprite3D::ResetColor();
	}

	// ============================================================
	// Assimpアニメーション抽出（内部用）
	// ============================================================
	
	// Assimpアニメーションから自動抽出
	AnimationClip ExtractAnimationFromAssimp(const aiAnimation* aiAnim);

	// ボーン名からインデックスを検索
	int FindBoneIndex(const char* boneName);

	// ============================================================
	// 複数アニメーション対応: FBX内のアニメーションを名前で直接再生
	// ============================================================

	// FBX内から名前でアニメーションを探して再生（推奨方法）
	bool PlayAnimationByName(const char* animName, bool loop = true);

	// 利用可能なアニメーション数を取得
	unsigned int GetAnimationCount() const;

	// インデックスからアニメーション名を取得
	const char* GetAnimationName(unsigned int index) const;

	// インデックスからアニメーションを再生
	bool PlayAnimationByIndex(unsigned int index, bool loop = true);

	// ブレンド時間の設定（秒単位）
	void SetAnimationBlendDuration(double duration)
	{
		m_BlendState.blendDuration = duration;
	}

	// 現在のブレンド状態を取得
	bool IsAnimationBlending() const { return m_BlendState.isBlending; }

private:
	// 内部用：FBX内のアニメーションから AnimationClip を作成・設定
	void SetAnimationClip(const AnimationClip& clip)
	{
		m_AnimClip = clip;
		m_AnimState.clip = &m_AnimClip;
		m_AnimState.time = 0.0;
		m_AnimState.play = false;
	}

	// ブレンド用補助関数：特定の状態から骨行列を計算
	void UpdateBoneMatricesForState(const AnimationState& state, BoneMatrices& outMatrices);
};
