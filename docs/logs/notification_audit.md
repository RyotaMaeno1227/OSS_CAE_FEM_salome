# Notification Audit Log

外部 Webhook / メールで通知した場合の記録テンプレート。

## Entries
```
- date: 2025-11-10 09:30 JST
  channel: webhook
  endpoint: https://hooks.example/chrono
  payload: archive_failure_rate_slack.json
  status: 200
  notes: nightly digest; fallback during Slack outage
```
```
- date: 2025-11-10 12:45 JST
  channel: email
  endpoint: incident@chrono.example (SMTP)
  payload: out/mail.eml
  status: sent
  notes: WARN ratio exceeded; CC to ops lead
```

> Appendix C.4 のテンプレに従い、新しい通知ごとに追記してください。
