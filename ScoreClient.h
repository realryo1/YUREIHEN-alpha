#pragma once

// スコアをサーバーへ送信する（同期版・ブロッキング）
// 戻り値: 成功なら true
bool Score_SendToServer(int score);

// スコアをサーバーへ非同期で送信する（別スレッドで実行、即座に返る）
void Score_SendToServerAsync(int score);

// 非同期送信が完了したか（送信中なら false）
bool Score_IsAsyncSendDone(void);

// 非同期送信の結果を取得する（完了前に呼ぶと false）
bool Score_GetAsyncSendResult(void);