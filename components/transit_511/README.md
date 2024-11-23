# Transit 511.org


```yaml

transit_511:
    sources:
        - http://
        - http://
    refresh_every: 5min
    on_refresh:
        lambda: etas.debug_print();

http_request:
  verify_ssl: false
  timeout: 5s
  # one more than timeout to prevent crashes when timeout > watchdog
  watchdog_timeout: 6s
```
