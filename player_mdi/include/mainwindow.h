#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QMdiArea>

class VideoPlayer;

/**
 * @brief H.264プレイヤー用のメインMDIアプリケーションウィンドウ。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief メインウィンドウを生成する。
     * @param parent 親ウィジェット。
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief メインウィンドウを破棄する。
     */
    ~MainWindow();

protected:
    /**
     * @brief ウィンドウクローズイベントを処理する。
     * @param event Qtのクローズイベント。
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief ビットストリームファイルを開き、新しいプレイヤー子ウィンドウを作成する。
     */
    void openFile();

    /**
     * @brief 現在アクティブな子ウィンドウを閉じる。
     */
    void closeCurrentWindow();

    /**
     * @brief 子ウィンドウをカスケード表示に並べる。
     */
    void cascade();

    /**
     * @brief 子ウィンドウをタイル表示に並べる。
     */
    void tile();

    /**
     * @brief 子ウィンドウ状態に合わせてウィンドウメニュー項目を更新する。
     */
    void updateWindowMenu();

    /**
     * @brief デコーダ/プレイヤーの状態文字列を更新する。
     * @param message 状態メッセージ。
     */
    void onDecoderStatusChanged(const QString &message);

    /**
     * @brief デコード進捗に応じて進捗バーを更新する。
     * @param current 現在の進捗値。
     * @param total 合計進捗値。
     */
    void onDecodingProgress(int current, int total);

private:
    /**
     * @brief 中央のMDI領域を作成し設定する。
     */
    void createCentralWidget();

    /**
     * @brief アプリケーションのメニューバーを作成する。
     */
    void createMenuBar();

    /**
     * @brief メインツールバーを作成する。
     */
    void createToolBar();

    /**
     * @brief ステータスバーのウィジェットを作成する。
     */
    void createStatusBar();

    /**
     * @brief 開いているすべての子ウィンドウを閉じる。
     */
    void closeAllWindows();

    /**
     * @brief アプリケーションを終了する。
     */
    void exit();

    QMdiArea *mdiArea;
    QMenu *windowMenu;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QLabel *decoderStatusLabel;
};

#endif // MAINWINDOW_H