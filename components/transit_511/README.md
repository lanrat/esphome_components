# Transit 511.org

A custom component to retrieve Transit ETAs from 511.org

```yaml
transit_511:
    id: transit_id
    sources:
        - http://...
        - http://...
    refresh_every: 5min
    max_response_buffer_size: 8k
    on_refresh:
        lambda: id(transit_id).debug_print();

http_request:
  verify_ssl: false
  timeout: 5s
  # one more than timeout to prevent crashes when timeout > watchdog
  watchdog_timeout: 6s
```
