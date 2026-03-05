#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
using namespace DirectX;



enum class BILLBOARD_ICON
{
	NONE,		// 表示なし
	ALERT,		// ！ (発見)
	QUESTION,	// ？ (警戒・不審)
	STUN,		// 星 (気絶)
	GHOST,		// お化けマーク (プレイヤー位置など)
	SEARCH,
	CHECK,
	DESTINATION,	// 目的地マーカー（下向き矢印）
};

class Billboard
{
public:
	Billboard();
	Billboard(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT3 rot, bool isDoubleSided = false);
	~Billboard();

	// 初期化
	void Initialize(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT3 rot, bool isDoubleSided = false);

	void Update(void);
	void Draw(void);

	// --- 便利機能 ---

	// 定義したアイコンに切り替える関数
	void SetIcon(BILLBOARD_ICON type);

	// 任意の画像パスを指定したい場合用
	void SetTexture(const char* texturePath);


	// --- UVアニメーション ---
	// frameCount: 横方向のコマ数, interval: 1コマあたりの秒数
	void SetUVAnimation(int frameCount, float interval);
	// UVアニメーションを無効化する
	void DisableUVAnimation();


	// --- 基本セッター・ゲッター ---
	void SetPos(XMFLOAT3 pos) { m_Pos = pos; }
	XMFLOAT3 GetPos(void) { return m_Pos; }

	void SetSize(XMFLOAT2 size) { m_Size = size; }
	XMFLOAT2 GetSize(void) { return m_Size; }

	void SetRotation(XMFLOAT3 rot) { m_Rot = rot; }
	XMFLOAT3 GetRotation(void) { return m_Rot; }

	void SetColor(XMFLOAT4 color) { m_Color = color; }
	XMFLOAT4 GetColor(void) { return m_Color; }

	// ライティング無効化オプション
	void SetIgnoreLighting(bool ignore) { m_IgnoreLighting = ignore; }
	bool GetIgnoreLighting(void) const { return m_IgnoreLighting; }

	// 壁越し半透明オプション（false にすると壁の裏でも透過しない）
	void SetWallFadeEnabled(bool enable) { m_WallFadeEnabled = enable; }
	bool GetWallFadeEnabled(void) const { return m_WallFadeEnabled; }

	// ビルボードモード切り替え（true=カメラ追従、false=固定板ポリゴン）
	void SetBillboardMode(bool enable) { m_IsBillboardMode = enable; }
	bool GetBillboardMode(void) const { return m_IsBillboardMode; }

private:
	ID3D11Buffer* m_VertexBuffer;
	ID3D11ShaderResourceView* m_Texture;
	std::string m_TexturePath; // 現在ロード済みのテクスチャパス（再ロード防止用）

	XMFLOAT3 m_Pos;
	XMFLOAT2 m_Size;
	XMFLOAT3 m_Rot;
	XMFLOAT4 m_Color;

	bool m_IsDoubleSided;
	bool m_IgnoreLighting; // ライティング無効化フラグ
	int m_VertexCount;

	// 現在のアイコンタイプを覚えておく
	BILLBOARD_ICON m_CurrentIconType;

	// UVアニメーション
	bool  m_UVAnimEnabled;   // UVアニメーション有効フラグ
	int   m_UVFrameCount;    // 横方向のコマ数
	int   m_UVCurrentFrame;  // 現在のコマ番号
	float m_UVInterval;      // 1コマあたりの秒数
	float m_UVTimer;         // 経過時間

	void CreateBuffer(void);
	void CreateBufferWithUV(float uMin, float uMax); // UV指定バッファ作成

	bool m_IsBillboardMode;    // true=ビルボード、false=固定板ポリゴン
	bool m_WallFadeEnabled;    // true=壁越しで半透明、false=壁越しでも不透明
};