#include "model.h"
#include "anim_sprite3d.h"
#include "shader.h"
#include "camera.h"
#include <algorithm>
#include <cmath>
#include <map>

// クォータニオン球面線形補間
static XMFLOAT4 QuatSlerp(const XMFLOAT4& q1, const XMFLOAT4& q2, float t)
{
	XMVECTOR v1 = XMLoadFloat4(&q1);
	XMVECTOR v2 = XMLoadFloat4(&q2);
	XMVECTOR result = XMQuaternionSlerp(v1, v2, t);
	XMFLOAT4 qResult;
	XMStoreFloat4(&qResult, result);
	return qResult;
}

// クォータニオンから回転行列への変換
static XMMATRIX QuatToMatrix(const XMFLOAT4& q)
{
	XMVECTOR quat = XMLoadFloat4(&q);
	return XMMatrixRotationQuaternion(quat);
}

void AnimSprite3D::InitializeBones()
{
	if (!m_Model || !m_Model->AiScene)
	{
		hal::dout << "AnimSprite3D: Model or AiScene is null" << std::endl;
		return;
	}

	m_BoneCount = 0;
	m_AiBones.clear();
	m_BoneOffsetMatrices.clear();

	hal::dout << "InitializeBones: mNumMeshes=" << m_Model->AiScene->mNumMeshes << std::endl;

	for (unsigned int m = 0; m < m_Model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = m_Model->AiScene->mMeshes[m];
		hal::dout << "  Mesh " << m << ": mNumBones=" << mesh->mNumBones << std::endl;
		
		for (unsigned int b = 0; b < mesh->mNumBones; b++)
		{
			aiBone* bone = mesh->mBones[b];
			
			bool found = false;
			for (const auto& existingBone : m_AiBones)
			{
				if (strcmp(existingBone->mName.data, bone->mName.data) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found && m_BoneCount < BoneMatrices::MAX_BONES)
			{
				m_AiBones.push_back(bone);
				m_BoneCount++;
				hal::dout << "    Added bone: " << bone->mName.data << std::endl;
			}
		}
	}

	hal::dout << "AnimSprite3D: Initialized " << m_BoneCount << " bones" << std::endl;

	for (int i = 0; i < BoneMatrices::MAX_BONES; i++)
	{
		m_BoneMatrices.matrices[i] = XMMatrixIdentity();
	}
}

XMFLOAT3 AnimSprite3D::InterpolateVec3(const std::vector<KeyVec3>& keys, double time)
{
	if (keys.empty())
	{
		return XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	if (keys.size() == 1)
	{
		return keys[0].value;
	}

	// 時間の範囲外チェック
	if (time <= keys.front().time)
	{
		return keys.front().value;
	}
	if (time >= keys.back().time)
	{
		return keys.back().value;
	}

	// キーフレームを二分探索で見つける
	int keyIndex = 0;
	for (size_t i = 0; i + 1 < keys.size(); i++)
	{
		if (time >= keys[i].time && time < keys[i + 1].time)
		{
			keyIndex = (int)i;
			break;
		}
	}

	const KeyVec3& key1 = keys[keyIndex];
	const KeyVec3& key2 = keys[keyIndex + 1];

	double timeDiff = key2.time - key1.time;
	if (timeDiff <= 0.0)
	{
		return key1.value;
	}

	float t = (float)((time - key1.time) / timeDiff);
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	XMFLOAT3 result;
	result.x = key1.value.x + (key2.value.x - key1.value.x) * t;
	result.y = key1.value.y + (key2.value.y - key1.value.y) * t;
	result.z = key1.value.z + (key2.value.z - key1.value.z) * t;

	return result;
}

XMFLOAT4 AnimSprite3D::InterpolateQuat(const std::vector<KeyQuat>& keys, double time)
{
	if (keys.empty())
	{
		return XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	if (keys.size() == 1)
	{
		return keys[0].value;
	}

	// 時間の範囲外チェック
	if (time <= keys.front().time)
	{
		return keys.front().value;
	}
	if (time >= keys.back().time)
	{
		return keys.back().value;
	}

	// キーフレームを二分探索で見つける
	int keyIndex = 0;
	for (size_t i = 0; i + 1 < keys.size(); i++)
	{
		if (time >= keys[i].time && time < keys[i + 1].time)
		{
			keyIndex = (int)i;
			break;
		}
	}

	const KeyQuat& key1 = keys[keyIndex];
	const KeyQuat& key2 = keys[keyIndex + 1];

	double timeDiff = key2.time - key1.time;
	if (timeDiff <= 0.0)
	{
		return key1.value;
	}

	float t = (float)((time - key1.time) / timeDiff);
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	XMFLOAT4 result = QuatSlerp(key1.value, key2.value, t);
	
	return result;
}

void AnimSprite3D::UpdateAnimation(float dt)
{
	// ブレンド処理
	if (m_BlendState.isBlending)
	{
		m_BlendState.blendElapsed += dt;
		
		if (m_BlendState.blendElapsed >= m_BlendState.blendDuration)
		{
			// ブレンド完了：新しいアニメーションに切り替え
			m_BlendState.isBlending = false;
			SetAnimationClip(m_BlendState.targetClip);
			PlayAnimation(m_AnimState.loop);
			hal::dout << "Animation blend completed" << std::endl;
		}
	}

	if (!m_AnimState.play || !m_AnimState.clip)
	{
		return;
	}

	// dt（秒単位）をティック単位に変換して時間を進める
	double ticksPerSecond = m_AnimState.clip->tps;
	if (ticksPerSecond <= 0.0)
	{
		ticksPerSecond = 24.0;  // デフォルト値
	}

	m_AnimState.time += dt * ticksPerSecond;

	// アニメーション終了判定
	if (m_AnimState.time >= m_AnimState.clip->duration)
	{
		if (m_AnimState.loop)
		{
			double duration = m_AnimState.clip->duration;
			if (duration > 0.0)
			{
				// ループ処理：時間を持ち越さず、正確にリセット
				m_AnimState.time = fmod(m_AnimState.time, duration);
				if (m_AnimState.time < 0.0)
					m_AnimState.time += duration;
			}
		}
		else
		{
			m_AnimState.time = m_AnimState.clip->duration;
			m_AnimState.play = false;
		}
	}

	UpdateBoneMatrices();
}

void AnimSprite3D::UpdateBoneMatrices()
{
	// ブレンド中の場合は特別処理
	if (m_BlendState.isBlending)
	{
		double blendT = m_BlendState.blendElapsed / m_BlendState.blendDuration;
		if (blendT > 1.0) blendT = 1.0;
		
		// 前のアニメーション状態から骨行列を計算
		UpdateBoneMatricesForState(m_BlendState.previousState, m_BoneMatrices);
		
		// 新しいアニメーション状態から骨行列を計算
		BoneMatrices targetMatrices;
		AnimationState tempState;
		tempState.clip = &m_BlendState.targetClip;
		tempState.time = 0.0;  // 新しいアニメーションは開始位置から
		UpdateBoneMatricesForState(tempState, targetMatrices);
		
		// 2つの行列をブレンド（各成分を線形補間）
		float blendF = (float)blendT;
		for (int i = 0; i < BoneMatrices::MAX_BONES; i++)
		{
			// 行列の各成分を線形補間
			XMMATRIX from = m_BoneMatrices.matrices[i];
			XMMATRIX to = targetMatrices.matrices[i];
			
			// 行列を浮動小数点配列に変換
			float fromM[16], toM[16];
			XMStoreFloat4x4((XMFLOAT4X4*)fromM, from);
			XMStoreFloat4x4((XMFLOAT4X4*)toM, to);
			
			// 各成分を線形補間
			float blendM[16];
			for (int j = 0; j < 16; j++)
			{
				blendM[j] = fromM[j] * (1.0f - blendF) + toM[j] * blendF;
			}
			
			// ブレンド結果を行列に戻す
			m_BoneMatrices.matrices[i] = XMLoadFloat4x4((const XMFLOAT4X4*)blendM);
		}
		return;
	}

	if (!m_AnimState.clip || m_AnimState.clip->tracks.empty())
	{
		// アニメーションがない場合はアイデンティティ行列で初期化
		for (int i = 0; i < BoneMatrices::MAX_BONES; i++)
		{
			m_BoneMatrices.matrices[i] = XMMatrixIdentity();
		}
		return;
	}

	const AnimationClip& clip = *m_AnimState.clip;
	double time = m_AnimState.time;

	// 時間の範囲チェック
	if (time < 0.0) time = 0.0;
	if (time > clip.duration) time = clip.duration;

	// トラックのサイズ分だけ行列を更新
	int trackSize = (int)clip.tracks.size();
for (int i = 0; i < trackSize && i < BoneMatrices::MAX_BONES; i++)
	{
		const BoneKeyframes& keyframes = clip.tracks[i];

		XMFLOAT3 trans = InterpolateVec3(keyframes.trans, time);
		XMFLOAT4 rot = InterpolateQuat(keyframes.rot, time);
		XMFLOAT3 scale = InterpolateVec3(keyframes.scale, time);

		XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX rotMat = QuatToMatrix(rot);
		XMMATRIX transMat = XMMatrixTranslation(trans.x, trans.y, trans.z);

		m_BoneMatrices.matrices[i] = scaleMat * rotMat * transMat;
	}

	// 未使用のボーン行列はアイデンティティで初期化
	for (int i = trackSize; i < BoneMatrices::MAX_BONES; i++)
	{
		m_BoneMatrices.matrices[i] = XMMatrixIdentity();
	}
}

void AnimSprite3D::Draw(void)
{
	if (m_Model)
	{
		XMFLOAT4 drawColor = m_UseOriginalColor ? m_OriginalColor : m_Color;
		bool shouldApplyColorReplace = !m_UseOriginalColor;

		// デバッグ：アニメーション状態を確認
		static int drawCount = 0;
		if (++drawCount % 300 == 0)  // 5秒ごと（60FPS*5）
		{
			hal::dout << "Draw: play=" << m_AnimState.play 
					  << " time=" << m_AnimState.time
					  << " boneCount=" << m_BoneCount << std::endl;
		}

		// アニメーション対応描画関数を使用
		ModelAnimationDraw(
			m_Model,
			GetPos(),
			GetRot(),
			GetScale(),
			m_BoneMatrices,
			drawColor,
			shouldApplyColorReplace
		);
	}
	else
	{
		hal::dout << "AnimSprite3D::Draw() : Model not loaded" << std::endl;
	}
}

// Assimpアニメーション情報からAnimationClipを生成
AnimationClip AnimSprite3D::ExtractAnimationFromAssimp(const aiAnimation* aiAnim)
{
    AnimationClip clip;
    
    clip.duration = aiAnim->mDuration;
    clip.tps = aiAnim->mTicksPerSecond;
    
    if (clip.tps == 0.0)
    {
        clip.tps = 24.0;
    }
    
    hal::dout << "ExtractAnimationFromAssimp: duration=" << clip.duration 
              << " tps=" << clip.tps 
              << " numChannels=" << aiAnim->mNumChannels 
              << " boneCount=" << m_BoneCount << std::endl;
    
    // ボーン情報がない場合、チャネル情報から動的にトラックを作成
    int maxBoneIndex = m_BoneCount;
    
    // チャネル情報からボーンインデックスを探索
    for (unsigned int c = 0; c < aiAnim->mNumChannels; c++)
    {
        aiNodeAnim* nodeAnim = aiAnim->mChannels[c];
        int boneIndex = FindBoneIndex(nodeAnim->mNodeName.data);
        
        // ボーンインデックスが見つからない場合、チャネルインデックスを使用
        if (boneIndex < 0)
        {
            boneIndex = c;
        }
        
        // トラックサイズを動的に拡張
        if (boneIndex >= (int)clip.tracks.size())
        {
            clip.tracks.resize(boneIndex + 1);
        }
        
        if (boneIndex >= maxBoneIndex)
        {
            maxBoneIndex = boneIndex + 1;
        }
        
        BoneKeyframes& keyframes = clip.tracks[boneIndex];
        
        // 位置キーフレーム
        keyframes.trans.resize(nodeAnim->mNumPositionKeys);
        for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; k++)
        {
            const aiVectorKey& key = nodeAnim->mPositionKeys[k];
            keyframes.trans[k].time = key.mTime;
            keyframes.trans[k].value = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
        }
        
        // 回転キーフレーム
        keyframes.rot.resize(nodeAnim->mNumRotationKeys);
        for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; k++)
        {
            const aiQuatKey& key = nodeAnim->mRotationKeys[k];
            keyframes.rot[k].time = key.mTime;
            keyframes.rot[k].value = XMFLOAT4(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
        }
        
        // スケーリングキーフレーム
        keyframes.scale.resize(nodeAnim->mNumScalingKeys);
        for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; k++)
        {
            const aiVectorKey& key = nodeAnim->mScalingKeys[k];
            keyframes.scale[k].time = key.mTime;
            keyframes.scale[k].value = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
        }
        
        hal::dout << "  Channel " << c << ": '" << nodeAnim->mNodeName.data 
                  << "' -> boneIndex=" << boneIndex
                  << " pos=" << nodeAnim->mNumPositionKeys
                  << " rot=" << nodeAnim->mNumRotationKeys
                  << " scale=" << nodeAnim->mNumScalingKeys << std::endl;
    }
    
    hal::dout << "  Final trackSize=" << clip.tracks.size() << std::endl;
    
    return clip;
}

// ボーン名からインデックスを探す
int AnimSprite3D::FindBoneIndex(const char* boneName)
{
    // ボーン一覧で検索
    for (int i = 0; i < m_BoneCount; i++)
    {
        if (strcmp(m_AiBones[i]->mName.data, boneName) == 0)
        {
            return i;
        }
    }
    
    // ボーンがない場合、ノード階層から検索
    // ノード名をインデックスとして使用（簡易的な方法）
    if (m_Model && m_Model->AiScene && m_Model->AiScene->mRootNode)
    {
        // ルートノードから検索して、ノード階層でのインデックスを返す
        // 簡略化: チャネル名が見つかった場合は常に同じインデックスを返す
        static std::map<std::string, int> nodeIndexMap;
        
        std::string nodeNameStr(boneName);
        if (nodeIndexMap.find(nodeNameStr) == nodeIndexMap.end())
        {
            // 初めてこのノード名が出現した場合、新しいインデックスを割り当て
            int newIndex = nodeIndexMap.size();
            nodeIndexMap[nodeNameStr] = newIndex;
            hal::dout << "FindBoneIndex: New node '" << boneName << "' assigned index " << newIndex << std::endl;
        }
        
        return nodeIndexMap[nodeNameStr];
    }
    
    return -1;
}

// ============================================================
// 複数アニメーション対応: FBX内のアニメーションを名前で直接再生
// ============================================================

bool AnimSprite3D::PlayAnimationByName(const char* animName, bool loop)
{
    if (!m_Model || !m_Model->AiScene || !animName)
    {
        hal::dout << "PlayAnimationByName: Invalid model" << std::endl;
        return false;
    }

    // 現在再生中のアニメーションと同じ名前の場合は無視
    if (m_AnimState.play && m_AnimState.currentAnimName == animName)
    {
        hal::dout << "PlayAnimationByName: Animation '" << animName << "' is already playing, ignoring call" << std::endl;
        return true;  // 既に再生中なので成功として扱う
    }

    hal::dout << "PlayAnimationByName: Looking for '" << animName << "', numAnimations=" << m_Model->AiScene->mNumAnimations << std::endl;

    // FBX内から名前でアニメーションを検索
    for (unsigned int i = 0; i < m_Model->AiScene->mNumAnimations; i++)
    {
        aiAnimation* aiAnim = m_Model->AiScene->mAnimations[i];
        hal::dout << "  Animation " << i << ": name='" << aiAnim->mName.data 
                  << "' duration=" << aiAnim->mDuration 
                  << " channels=" << aiAnim->mNumChannels << std::endl;
        
        if (strcmp(aiAnim->mName.data, animName) == 0)
        {
            // アニメーションが見つかった
            AnimationClip clip = ExtractAnimationFromAssimp(aiAnim);
            hal::dout << "  -> Extracted: tps=" << clip.tps << " duration=" << clip.duration << " tracks=" << clip.tracks.size() << std::endl;
            
            // 別のアニメーションが再生中の場合、ブレンド遷移を開始
            if (m_AnimState.play && m_AnimState.currentAnimName != animName)
            {
                hal::dout << "Animation: Blending from '" << m_AnimState.currentAnimName << "' to '" << animName << "'" << std::endl;
                
                // 現在の状態をスナップショット
                m_BlendState.previousState = m_AnimState;
                m_BlendState.targetClip = clip;
                m_BlendState.isBlending = true;
                m_BlendState.blendElapsed = 0.0;
                m_BlendState.blendDuration = 0.3;  // 0.3秒でブレンド
            }
            else
            {
                // 初回起動時は通常の再生開始
                SetAnimationClip(clip);
                PlayAnimation(loop);
            }
            
            m_AnimState.currentAnimName = animName;  // 現在再生中のアニメーション名を保存
            m_AnimState.loop = loop;
            
            hal::dout << "Animation '" << animName << "' started" << std::endl;
            return true;
        }
    }

    hal::dout << "Animation '" << animName << "' not found" << std::endl;
    return false;
}

unsigned int AnimSprite3D::GetAnimationCount() const
{
    if (!m_Model || !m_Model->AiScene)
    {
        return 0;
    }
    return m_Model->AiScene->mNumAnimations;
}

const char* AnimSprite3D::GetAnimationName(unsigned int index) const
{
    if (!m_Model || !m_Model->AiScene || index >= m_Model->AiScene->mNumAnimations)
    {
        return nullptr;
    }
    return m_Model->AiScene->mAnimations[index]->mName.data;
}

bool AnimSprite3D::PlayAnimationByIndex(unsigned int index, bool loop)
{
    if (!m_Model || !m_Model->AiScene || index >= m_Model->AiScene->mNumAnimations)
    {
        hal::dout << "AnimSprite3D::PlayAnimationByIndex() - Invalid animation index: " << index << std::endl;
        return false;
    }

    aiAnimation* aiAnim = m_Model->AiScene->mAnimations[index];
    AnimationClip clip = ExtractAnimationFromAssimp(aiAnim);
    SetAnimationClip(clip);
    PlayAnimation(loop);
    m_AnimState.currentAnimName = aiAnim->mName.data;  // アニメーション名を保存

    hal::dout << "AnimSprite3D::PlayAnimationByIndex() - Playing animation at index " << index 
              << " (name: " << aiAnim->mName.data << ")" << std::endl;
    return true;
}

// ============================================================
// ブレンド用補助関数
// ============================================================

void AnimSprite3D::UpdateBoneMatricesForState(const AnimationState& state, BoneMatrices& outMatrices)
{
	if (!state.clip || state.clip->tracks.empty())
	{
		// アニメーションがない場合はアイデンティティ行列で初期化
		for (int i = 0; i < BoneMatrices::MAX_BONES; i++)
		{
			outMatrices.matrices[i] = XMMatrixIdentity();
		}
		return;
	}

	const AnimationClip& clip = *state.clip;
	double time = state.time;

	// 時間の範囲チェック
	if (time < 0.0) time = 0.0;
	if (time > clip.duration) time = clip.duration;

	// トラックのサイズ分だけ行列を更新
	int trackSize = (int)clip.tracks.size();
	for (int i = 0; i < trackSize && i < BoneMatrices::MAX_BONES; i++)
	{
		const BoneKeyframes& keyframes = clip.tracks[i];

		XMFLOAT3 trans = InterpolateVec3(keyframes.trans, time);
		XMFLOAT4 rot = InterpolateQuat(keyframes.rot, time);
		XMFLOAT3 scale = InterpolateVec3(keyframes.scale, time);

		XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX rotMat = QuatToMatrix(rot);
		XMMATRIX transMat = XMMatrixTranslation(trans.x, trans.y, trans.z);

		outMatrices.matrices[i] = scaleMat * rotMat * transMat;
	}

	// 未使用のボーン行列はアイデンティティで初期化
	for (int i = trackSize; i < BoneMatrices::MAX_BONES; i++)
	{
		outMatrices.matrices[i] = XMMatrixIdentity();
	}
}
