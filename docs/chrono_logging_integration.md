# chrono_log ハンドラ組み込みガイド

`chrono_log` は C 版 chrono ランタイムに含まれる軽量なロガーです。デフォルトでは標準エラーへ WARN 以上を出力しますが、ハンドラやログレベルを差し替えることで、既存のアプリケーションのログ基盤へ統合できます。本メモでは主要な API と典型的な組み込み手順をまとめます。

## 1. ハンドラの登録

```c
#include "chrono_logging.h"

static void app_log_handler(ChronoLogLevel_C level,
                            ChronoLogCategory_C category,
                            const char *message,
                            void *user_data) {
    AppLogger *logger = (AppLogger *)user_data;
    app_logger_write(logger, level, category, message);
}

void chrono_logging_attach(AppLogger *logger) {
    chrono_log_set_handler(app_log_handler, logger);
}
```

- `chrono_log_set_handler` により、全コンポーネントが同一ハンドラを共有します。
- `chrono_log_get_handler` で現在のハンドラとユーザーデータを取得可能です。
- ハンドラを `NULL` に戻すと標準の stderr 出力へ復帰します。

## 2. ログレベル／カテゴリ制御

- `chrono_log_set_level(level)` でグローバルな最小出力レベルを変更できます。
- `chrono_log_is_enabled(level, category)` を利用すると、ハンドラ側でフィルタリングする前に早期リターンが可能です。
- Coupled 拘束など特定モジュールの警告レベルを変えたい場合は、`chrono_coupled_constraint2d_set_condition_warning_log_level` でカテゴリ別レベルを指定できます。

```c
chrono_log_set_level(CHRONO_LOG_LEVEL_INFO); // INFO 以上を許可
chrono_coupled_constraint2d_set_condition_warning_log_level(
    constraint, CHRONO_LOG_LEVEL_WARNING, CHRONO_LOG_CATEGORY_CONSTRAINT);
```

## 3. 出力先の切り替え例

### 3.1 stderr のまま最小構成

追加設定なしで `chrono_log_write` が `[WARN] (constraint) ...` の形式で stderr に出力されます。短時間のツール実行や CI スモークに適しています。

### 3.2 ファイル出力

```c
typedef struct {
    FILE *fp;
} FileLogger;

static void file_log_handler(ChronoLogLevel_C level,
                             ChronoLogCategory_C category,
                             const char *message,
                             void *user_data) {
    FileLogger *logger = (FileLogger *)user_data;
    if (!logger->fp) {
        return;
    }
    fprintf(logger->fp, "[%s] (%s) %s\n",
            chrono_log_level_name(level),
            chrono_log_category_name(category),
            message);
    fflush(logger->fp);
}

void chrono_logging_attach_file(FILE *fp) {
    static FileLogger logger;
    logger.fp = fp;
    chrono_log_set_handler(file_log_handler, &logger);
}
```

### 3.3 既存ロガー（例: spdlog）へフォワード

```c++
#include <spdlog/spdlog.h>

static void spdlog_handler(ChronoLogLevel_C level,
                           ChronoLogCategory_C category,
                           const char *message,
                           void * /*user*/) {
    auto *logger = spdlog::get("chrono");
    if (!logger) {
        return;
    }
    switch (level) {
        case CHRONO_LOG_LEVEL_ERROR:
            logger->error("[{}] {}", chrono_log_category_name(category), message);
            break;
        case CHRONO_LOG_LEVEL_WARNING:
            logger->warn("[{}] {}", chrono_log_category_name(category), message);
            break;
        default:
            logger->info("[{}] {}", chrono_log_category_name(category), message);
            break;
    }
}

void chrono_logging_attach_spdlog() {
    chrono_log_set_handler(spdlog_handler, nullptr);
}
```

## 4. Coupled 条件警告との連携

- `chrono_coupled_constraint2d_set_condition_warning_callback` で条件数閾値を超えた際のイベントをアプリ内へ渡せます。
- `ChronoCoupledConditionWarningEvent_C` には条件数、しきい値、アクティブ式数、ドロップ適用の有無が含まれます。
- ハンドラ登録後は `enable_logging` のオン/オフ、`log_cooldown`、`max_drop` などをポリシー経由で調整してください。

```c
static void coupled_warning_callback(const ChronoCoupledConstraint2D_C *constraint,
                                     const ChronoCoupledConditionWarningEvent_C *event,
                                     void *user_data) {
    chrono_log_write(event->level,
                     event->category,
                     "Coupled warning cond=%.3e threshold=%.3e active=%d recover=%s",
                     event->condition_number,
                     event->threshold,
                     event->active_equations,
                     event->recovery_applied ? "done" : "pending");
}

void coupled_attach_warning_logger(ChronoCoupledConstraint2D_C *constraint) {
    chrono_coupled_constraint2d_set_condition_warning_callback(constraint,
                                                               coupled_warning_callback,
                                                               NULL);
}
```

## 5. 運用ヒント

- CI や週次ジョブでログを解析するときは、`tools/run_coupled_benchmark.py` を使用すると CSV （`data/coupled_benchmark_metrics.csv`）と警告通知（GitHub Actions の `::warning::`）を同時に取得できます。
- 長時間テスト（`tests/test_coupled_constraint_endurance`）の CSV には、ドロップ回数や再解ステップまでの時間など詳細メトリクスを付加しており、ログと組み合わせることで異常検知が容易になります。
- アプリ側でログファイルをローテーションする場合は、`chrono_log_set_handler` で常に最新の `FILE*` を渡すか、ハンドラ内部でローテーション検知を実装してください。
- 1 つの `chrono_log_set_handler` しか登録できないため、複数のログ先へ届けたい場合はテスト (`tests/test_coupled_logging_integration.c`) のように「デマルチハンドラ」を作成し、その中で複数の処理（ファイル記録＋既存ロガーなど）を呼び出してください。
